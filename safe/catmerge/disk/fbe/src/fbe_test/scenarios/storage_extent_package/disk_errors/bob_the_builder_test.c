/***************************************************************************
 * Copyright (C) EMC Corporation 2009, 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file bob_the_builder_test.c
 ***************************************************************************
 *
 * @brief
 *
 *  The Bob the Builder test scenario is used to validate remapping
 *  capabilities and behaviour when RAID groups are subjected to hard
 *  and soft media errors.
 *
 * @version
 *   9/14/2009:  Created. Peter Puhov
 *   8/23/2010:  Modified. Mike Polia
 *
 ***************************************************************************/

/*************************
 *   TEST DESCRIPTION
 *************************/
char * bob_the_builder_short_desc = "I/O induced remap logic verification";
char * bob_the_builder_long_desc =
"The I/O induced remap Scenario (BobTheBuilder) tests the ability of the RAID to react to certain type of media errors. \n"
"When the unrecoverable media error occurs during READ operation raid will mark the appropriate chunks as needing to be verified. \n"
"The test verifies that verify occurred and that the appropriate number of errors were found/fixed. \n"
"It covers the following cases: \n"
"	- A drive had a media error during read operation. \n"
"The bad block needs to be remapped. It is done by setting needs verify on the chunk at the RAID level. \n"
"The verify at the raid level will cause the bad blocks to be remapped\n"
"\n"
"STEP 1: configure all raid groups and LUNs.\n"
"        - We test 520 and 4160 block size drives.\n"
"        - All raid groups have 3 LUNs each\n"
"        - For Raid Test level 0, we only test one raid group for each raid type\n"
"STEP 2: For each error case and each raid group :\n" 
"        - Write background pattern\n"
"        - Clear verify report \n"
"        - Disable all the error injection records \n"
"        - Add our error injection records \n"
"        - Enable injection on raid group \n"
"        - Send read request to error location\n"
"        - Check that number of validations are as expected \n"
"        - Make sure there were no unexpected errors \n"
"        - Stop logical error injection \n"
"        - Check that correctable media errors equal the number of media error blocks.\n"
"        - Check that uncorrectable media errors are as expected\n"
"        - Check that the number of read and write verify errors is equal to the number of errors we injected.\n"
"        - Destroy error injection objects\n"
"\n"
"Test Specific Switches:\n"
"        -raid0_only - only test raid 0\n"
"        -raid10_only - only test raid 10\n"
"        -raid1_only -  only test raid 1\n"
"        -raid3_only -  only test raid 3\n"
"        -raid5_only -  only test raid 5\n"
"        -raid6_only -  only test raid 6 \n"
"\n"
"Description last updated: 10/12/2011.\n";

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
#include "sep_hook.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "neit_utils.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"

/************************************************************
 *  MACROS, DEFINES, CONSTANTS
 ************************************************************/

/*!*******************************************************************
 * @def BOB_THE_BUILDER_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define BOB_THE_BUILDER_LUNS_PER_RAID_GROUP 3


/*!*******************************************************************
 * @def BOB_THE_BUILDER_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define BOB_THE_BUILDER_CHUNKS_PER_LUN 3


/*!*******************************************************************
 * @def BOB_THE_BUILDER_VERIFY_WAIT_SECONDS
 *********************************************************************
 * @brief Number of seconds we should wait for the verify to finish.
 *
 *********************************************************************/
#define BOB_THE_BUILDER_VERIFY_WAIT_SECONDS 60


/*!*******************************************************************
 * @def ERROR_TYPE_LIST_LENGTH
 *********************************************************************
 * @brief Max number of error types in the error type list.
 *
 *********************************************************************/
#define ERROR_TYPE_LIST_LENGTH  2

/*!*******************************************************************
 * @def BOB_THE_BUILDER_NUM_520_DRIVES
 *********************************************************************
 * @brief Max number of 520 block drives that needs to be created for 
 * this test
 *
 *********************************************************************/
#define BOB_THE_BUILDER_NUM_520_DRIVES 5

/*!*******************************************************************
 * @def BOB_THE_BUILDER_NUM_4160_DRIVES
 *********************************************************************
 * @brief Max number of 4160 block drives that needs to be created for 
 * this test
 *
 *********************************************************************/
#define BOB_THE_BUILDER_NUM_4160_DRIVES 5

/*!*******************************************************************
 * @def BOB_THE_BUILDER_NUM_RG_TYPES
 *********************************************************************
 * @brief Number of RG Types that is being tested. In this all the
 * RG Types are being tested
 *
 *********************************************************************/
#define BOB_THE_BUILDER_NUM_RG_TYPES 6

/************************************************************
 *  TYPEDEFS
 ************************************************************/
/*!*******************************************************************
 * @struct bob_the_builder_expected_results_t
 *********************************************************************
 *
 * @brief
 * This is the typedef that defines the expected error values for the 
 * specified error case
 *
 *********************************************************************/
typedef struct bob_the_builder_expected_results_s
{
    fbe_bool_t b_correctable;
    fbe_u32_t num_validations; /* Number of validations that occurs */
    fbe_u32_t num_c_media_error; /* Number of correctable media Errors */
    fbe_u32_t num_u_media_error; /* Number of uncorrectable media Errors */
    fbe_u64_t num_read_media_errors_injected; /* Number of read errors injected */ 
    fbe_u64_t num_write_verify_blocks_remapped; /* Number of write verify commands for remap */
} bob_the_builder_expected_results_t;

/*!*******************************************************************
 * @struct bob_the_builder_error_case_t
 *********************************************************************
 *
 * @brief
 * This is the typedef that defines an error test case. There is a
 * separate error test case for each different RAID group
 * configuration being tested.
 *
 *********************************************************************/
typedef struct bob_the_builder_error_case_s
{
    /*! Where and how much to read while errors are injected.
     */
    fbe_lba_t lba_to_read;
    fbe_block_count_t blocks_to_read;
    
    /*! What we are looking for. 
     */
    bob_the_builder_expected_results_t soft_expected_results; 
    bob_the_builder_expected_results_t hard_expected_results;

    /*! Error to inject.
     */
    fbe_api_logical_error_injection_record_t record;
} bob_the_builder_error_case_t;


/*!*******************************************************************
 * @struct error_type_list_t
 *********************************************************************
 *
 * @brief
 *
 * This is the typedef that defines each element of a list of
 * different error types that may be injected during a given error
 * test case.
 *
 *********************************************************************/
typedef struct error_type_list_s
{
    fbe_api_logical_error_injection_type_t err_type;
    const char *description;
} error_type_list_t;


/************************************************************
 *  FUNCTION PROTOTYPES
 ************************************************************/

static void bob_the_builder_media_error_test(fbe_raid_group_type_t raid_type,
                                             fbe_class_id_t class_id,
                                             fbe_object_id_t lun_object_id,
                                             fbe_object_id_t rg_object_id,                                             
                                             bob_the_builder_error_case_t *error_case_p);

static fbe_status_t bob_the_builder_check_for_errors(fbe_raid_group_type_t raid_type,
                                                     fbe_object_id_t rg_object_id,
                                                     fbe_api_logical_error_injection_get_stats_t *stats_p);


/**********************************
 *   DATA STRUCTURE DECLARATIONS
 *********************************/

static bob_the_builder_error_case_t bob_the_builder_raid0_error_cases[] = 
{
    {
        0, /* lba_to_read */
        1, /* blocks_to_read */
        { 
            FBE_TRUE, /* Correctable Error */
            0, /* Number of validations that occurs */
            1, /* Number of correctable media Errors */
            0, /* Number of uncorrectable media Errors */
            /* 1 for Host read + 1 read from the library + (1 verify read fail * number of blocks err injected) */
            3, /* Number of read errors injected */ 
            1, /* Number of write verify commands for remap */
        },
        { 
          FBE_FALSE, /* UnCorrectable Error */
          2, /* Number of validations that occurs */
          0, /* Number of correctable media Errors */
          1, /* Number of uncorrectable media Errors */
          /* 1 for Host read + 1 read from the library + (number of blocks err injected  * (1 verify read + 1 Mining mode)) */
          4, /* Number of read errors injected */ 
          1, /* Number of write verify commands for remap */
        }, 
        { 0x1, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at front of 4k boundary*/
        0, /* lba_to_read */
        4, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_FALSE, 5, 0, 4, 10, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x0, 0x4, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at end of 4k boundary*/
        4, /* lba_to_read */
        4, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_FALSE, 5, 0, 4, 10, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x4, 0x4, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at middle of 4k boundary*/
        2, /* lba_to_read */
        4, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_FALSE, 5, 0, 4, 10, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x2, 0x4, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject across 4k boundary*/
        4, /* lba_to_read */
        8, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_FALSE, 5, 0, 4, 10, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x6, 0x4, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    {  /* large IO test. non direct IO path. */
        0, /* lba_to_read */
        FBE_RAID_SECTORS_PER_ELEMENT+1, /* blocks_to_read */
        { FBE_TRUE,  0, 4, 0, 5, 4}, /* See comments for the first case above */
        { FBE_FALSE, 5, 0, 4, 9, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x0, 0x4, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    {FBE_U32_MAX, FBE_U32_MAX    /* Terminator. */},
};

static bob_the_builder_error_case_t bob_the_builder_raid1_error_cases[] = 
{
    {
        0, /* host lba */
        1, /* blocks_to_read */
        {
            FBE_TRUE, /* Correctable Error */
            0, /* Number of validations that occurs */
            1, /* Number of correctable media Errors */
            0, /* Number of uncorrectable media Errors */
            /* 1 for host read + 1 read from the library + (1 verify read fail * number of blocks err injected) */
            3, /* Number of read errors injected */ 
            1, /* Number of write verify commands for remap */
        },
        { 
            FBE_TRUE, 2, 1, 0, 3, 1 /* See comments for the first case above */
        }, 
        { 0x1, 0x10, 0x0, 0x1,
          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
          0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at front of 4k boundary*/
        0, /* host lba */
        4, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_TRUE, 5, 4, 0, 6, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x0, 0x4,
          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
          0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at end of 4k boundary*/
        4, /* host lba */
        4, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_TRUE, 5, 4, 0, 6, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x4, 0x4,
          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
          0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject in middle of 4k boundary*/
        2, /* host lba */
        4, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_TRUE, 5, 4, 0, 6, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x2, 0x4,
          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
          0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject across 4k boundary*/
        4, /* host lba */
        8, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_TRUE, 5, 4, 0, 6, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x6, 0x4,
          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
          0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    {FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};


static bob_the_builder_error_case_t bob_the_builder_raid3_error_cases[] = 
{
    {
        0, /* lba_to_read */
        1, /* blocks_to_read */
        {
            FBE_TRUE, /* Correctable Error */
            0, /* Number of validations that occurs */
            1, /* Number of correctable media Errors */
            0, /* Number of uncorrectable media Errors */
            /* 1 from the host read + (1 from the verify path * number of blocks err injected)(Add 1 here if direct IO is enabled )*/
            3, /* Number of read errors injected */  
            1, /* Number of write verify commands for remap */ /* Number of blocks as part of verify */
        },
        {
            FBE_TRUE, /* Correctable Error */
            2, /* Number of validations that occurs */
            1, /* Number of correctable media Errors */
            0, /* Number of uncorrectable media Errors */
            /* 1 from the host read + 1 for R5_rd_vr + (1 for r5_vr * number of blocks err injected)(Add 1 here if direct IO is enabled )*/
            4, /* Number of read errors injected */  
            1, /* Number of write verify commands for remap */ /* Number of blocks as part of verify */
        },
        { 0x10, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at front of 4k boundary*/
        0, /* lba_to_read */
        1, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x10, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject in middle of 4k boundary*/
        2, /* lba_to_read */
        2, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x10, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at end 4k boundary*/
        6, /* lba_to_read */
        2, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x10, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject across 4k boundary*/
        7, /* lba_to_read */
        2, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x10, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* large IO test. non direct IO path. */
        0, /* lba_to_read */
        FBE_RAID_SECTORS_PER_ELEMENT+1, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 17, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 18, 16}, /* See comments for the first case above */
        { 0x10, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
{FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};


static bob_the_builder_error_case_t bob_the_builder_raid5_error_cases[] = 
{
    {
        0, /* lba_to_read */
        1, /* blocks_to_read */
        {
           FBE_TRUE, /* Correctable Error */
           0, /* Number of validations that occurs */
           1, /* Number of correctable media Errors */
           0, /* Number of uncorrectable media Errors */
            /* 1 from the host read + 1 from the library +(1 from the verify path * number of blocks err injected)*/
           3, /* Number of read errors injected */ 
           1, /* Number of write verify commands for remap */
        },
        {
            FBE_TRUE, /* Correctable Error */
            2, /* Number of validations that occurs */
            1, /* Number of correctable media Errors */
            0, /* Number of uncorrectable media Errors */
            /* 1 from the host read + + 1 from the library+ 1 for R5_rd_vr + (1 for r5_vr * number of blocks err injected)*/
            4, /* Number of read errors injected */  
            1, /* Number of write verify commands for remap */ /* Number of blocks as part of verify */
        },
        { 0x4, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at front of 4k boundary*/
        0, /* lba_to_read */
        1, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x4, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject in middle of 4k boundary*/
        2, /* lba_to_read */
        2, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x4, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at end of 4k boundary*/
        6, /* lba_to_read */
        2, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x4, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject across 4k boundary*/
        7, /* lba_to_read */
        2, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x4, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    {  /* large IO test. non direct IO path. */
        0, /* lba_to_read */
        FBE_RAID_SECTORS_PER_ELEMENT+1, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 17, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 18, 16}, /* See comments for the first case above */
        { 0x4, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    {FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};


static bob_the_builder_error_case_t bob_the_builder_raid6_error_cases[] = 
{
    {
        0, /* lba_to_read */
        1, /* blocks_to_read */
        {
            FBE_TRUE, /* Correctable Error */
            0, /* Number of validations that occurs */
            1, /* Number of correctable media Errors */
            0, /* Number of uncorrectable media Errors */
            /* 1 from the host read + (1 from the verify path * number of blocks err injected)(Add 1 here if direct IO is enabled )*/
            3, /* Number of read errors injected */  
            1, /* Number of write verify commands for remap */ /* Number of blocks as part of verify */
        },
        {
            FBE_TRUE, /* Correctable Error */
            2, /* Number of validations that occurs */
            1, /* Number of correctable media Errors */
            0, /* Number of uncorrectable media Errors */
            /* 1 from the host read + 1 for R5_rd_vr + (1 for r5_vr * number of blocks err injected)(Add 1 here if direct IO is enabled )*/
            4, /* Number of read errors injected */  
            1, /* Number of write verify commands for remap */ /* Number of blocks as part of verify */
        },
        { 0x8, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at front of 4k boundary*/
        0, /* lba_to_read */
        1, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x8, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject in middle of 4k boundary*/
        2, /* lba_to_read */
        2, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x8, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at end of 4k boundary*/
        6, /* lba_to_read */
        2, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x8, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject across 4k boundary*/
        7, /* lba_to_read */
        2, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 18, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 19, 16}, /* See comments for the first case above */
        { 0x8, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    {  /* large IO test. non direct IO path. */
        0, /* lba_to_read */
        FBE_RAID_SECTORS_PER_ELEMENT+1, /* blocks_to_read */
        { FBE_TRUE, 0, 16, 0, 17, 16}, /* See comments for the first case above */
        { FBE_TRUE, 17, 16, 0, 18, 16}, /* See comments for the first case above */
        { 0x8, 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
            0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    {FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};


static bob_the_builder_error_case_t bob_the_builder_raid10_error_cases[] = 
{
    {
        0, /* lba_to_read */
        1, /* blocks_to_read */
        {
            FBE_TRUE, /* Correctable Error */
            0, /* Number of validations that occurs */
            1, /* Number of correctable media Errors */
            0, /* Number of uncorrectable media Errors */
            /* 1 for host read + 1 read from the library + (1 verify read fail * number of blocks err injected) */
            3, /* Number of read errors injected */ 
            1, /* Number of write verify commands for remap */
        },
        { FBE_TRUE, 2, 1, 0, 3, 1}, /* See comments for the first case above */
        { 0x1, 0x10, 0x0, 0x1,
          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
          0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at front of 4k boundary*/
        0, /* host lba */
        4, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_TRUE, 5, 4, 0, 6, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x0, 0x4,
          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
          0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject at end of 4k boundary*/
        4, /* host lba */
        4, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_TRUE, 5, 4, 0, 6, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x4, 0x4,
          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
          0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject in middle of 4k boundary*/
        2, /* host lba */
        4, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_TRUE, 5, 4, 0, 6, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x2, 0x4,
          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
          0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    { /* inject across 4k boundary*/
        4, /* host lba */
        8, /* blocks_to_read */
        { FBE_TRUE, 0, 4, 0, 6, 4}, /* See comments for the first case above */
        { FBE_TRUE, 5, 4, 0, 6, 4}, /* See comments for the first case above */
        { 0x1, 0x10, 0x6, 0x4,
          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED,
          0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
    },
    {FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};

typedef struct bob_the_builder_test_configuration_s
{
    const char *name;
    bob_the_builder_error_case_t *error_config;
    fbe_test_rg_configuration_t raid_group_config[FBE_TEST_MAX_RAID_GROUP_COUNT];
} bob_the_builder_test_configuration_t;

bob_the_builder_test_configuration_t bob_the_builder_test_config[BOB_THE_BUILDER_NUM_RG_TYPES] = 
{
    /*** NOTE 1: If you change the width of RG, make sure the define BOB_THE_BUILDER_NUM_520_DRIVES is also changed as applicable */
    /*** NOTE 2: The block size will be changed for the 4k test */
    {
        "RAID 0",
        &bob_the_builder_raid0_error_cases[0],
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,    FBE_CLASS_ID_STRIPER,   520,            4,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },

    {
        "RAID 1",
        &bob_the_builder_raid1_error_cases[0],
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {2,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,    FBE_CLASS_ID_MIRROR,    520,            6,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },

    {
        "RAID 3",
        &bob_the_builder_raid3_error_cases[0],
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,    FBE_CLASS_ID_PARITY,    520,            8,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },

    {
        "RAID 5",
        &bob_the_builder_raid5_error_cases[0],
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,    FBE_CLASS_ID_PARITY,    520,            0,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },

    {
        "RAID 6",
        &bob_the_builder_raid6_error_cases[0],
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,    FBE_CLASS_ID_PARITY,    520,            2,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },

    {
        "RAID 1_0",
        &bob_the_builder_raid10_error_cases[0],
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {4,        0x2a000,    FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            10,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },
};


/*!*******************************************************************
 * @var error_type_list
 *********************************************************************
 * @brief This is a list of the types or errors that may be injected
 * into the RAID group configurations being tested.
 *
 *********************************************************************/
static error_type_list_t error_type_list[] =
{
    {FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR, "soft media error"},
    {FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, "hard media error"}
};


/*!**************************************************************
 * class_id_name()
 ****************************************************************
 * @brief
 *
 * Equivalent names for the various RAID class IDs. Must be kept
 * in-sync with fbe_class_id_t.
 *
 * @param - class_id - RAID class. One of FBE_CLASS_ID_PARITY,
 * FBE_CLASS_ID_STRIPER, FBE_CLASS_ID_MIRROR.
 *
 * @return On success, this function returns the readable name
 * corresponding to the class_id value. Otherwise it returns a string
 * indicating that the class_id value is unknown.
 *
 * @author
 *  8/31/2010 - Created. Mike Polia
 *
 ****************************************************************/
static const char *class_id_name(fbe_class_id_t class_id)
{
    switch (class_id)
    {
    case FBE_CLASS_ID_MIRROR:
        return "FBE_CLASS_ID_MIRROR";
        break;

    case FBE_CLASS_ID_STRIPER:
        return "FBE_CLASS_ID_STRIPER";
        break;

    case FBE_CLASS_ID_PARITY:
        return "FBE_CLASS_ID_PARITY";
        break;

    default:
        return "UNKNOWN CLASS ID";
    }
}
/***************************************************************
 * end class_id_name()
 ***************************************************************/


/*!**************************************************************
 * bob_the_builder_get_test_count()
 ****************************************************************
 * @brief
 *  Count all the test cases, stop when we hit the terminator.
 *
 * @param - error_case_p - Ptr to error case array, terminated with
 *                         FBE_U32_MAX
 *
 * @return Count of entries in table.
 *
 * @author
 *  7/28/2010 - Created. Rob Foley
 *  8/18/2010 - Modified. Mike Polia
 *
 ****************************************************************/
static fbe_u32_t bob_the_builder_get_test_count(bob_the_builder_error_case_t *error_case_p)
{
    fbe_u32_t count = 0;

    /* Count each entry and look for the terminator.
     */
    while (error_case_p->lba_to_read != FBE_U32_MAX)
    {
        count++;
        error_case_p++;
    }
    return count;
}
/******************************************
 * end bob_the_builder_get_test_count()
 ******************************************/

/*!**************************************************************
 * error_test_prelude()
 ****************************************************************
 * @brief
 *
 *  This function performs pre-test housekeeping for any of the RAID
 *  group test cases and test variants.
 *
 * @param - raid_type - The RAID type being tested. One of R0, R1, R3,
 * R5, R6, R1+0.
 *
 * @param - lun_object_id - The LUN object ID returned by
 * fbe_api_database_lookup_lun_by_number() in the function
 * run_all_test_variants().
 *
 * @param - rg_object_id - The RAID group object ID returned by
 * fbe_api_database_lookup_raid_group_by_number() in the function
 * run_all_test_variants().
 *
 * @param - stats_p - Current logical error injection statistics are
 * returned to the caller using thi parameter.
 *
 * @param - error_cases_p - A corresponding error case for this RAID
 * group being tested.
 *
 * @return On success this function returns FBE_STATUS_OK. Otherwise,
 * one of the other FBE_STATUS values is returned to indicate the
 * fault.
 *
 * @author
 *  8/18/2010 - Created. Mike Polia
 *
 ****************************************************************/
static fbe_status_t error_test_prelude(fbe_raid_group_type_t raid_type,
                                       fbe_object_id_t lun_object_id,
                                       fbe_object_id_t rg_object_id,
                                       fbe_api_logical_error_injection_get_stats_t *stats_p,
                                       bob_the_builder_error_case_t *error_case_p)
{
    fbe_status_t status;

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    /* Clear the verify report.
     */
    status = fbe_api_lun_clear_verify_report(lun_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Disable all the records.
     */
    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Add our own records.
     */
    status = fbe_api_logical_error_injection_create_record(&error_case_p->record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Enable error injection ==");

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Raid 10s need to enable on each of the mirrors.
     */
    if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;

        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* Now enable injection for each mirror.
         */
        for (index = 0; 
             index < downstream_object_list.number_of_downstream_objects; 
             index++)
        {
            status = fbe_api_logical_error_injection_enable_object(downstream_object_list.downstream_object_list[index], 
                                                                   FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
    else
    {
        status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    bob_the_builder_check_for_errors(raid_type, rg_object_id, stats_p);

    return status;
}
/***************************************************************
 * end error_test_prelude()
 ***************************************************************/


/*!**************************************************************
 * bob_the_builder_check_for_errors
 ****************************************************************
 * @brief
 *
 *  After a call to the function fbe_api_rdgen_send_one_io(), this
 *  function is called to examine the error count returned in the
 *  context structure. Any deviation from expected values results in a
 *  MUT assertion.
 *
 * @param - rg_object_id - The RAID group object ID returned by
 * fbe_api_database_lookup_raid_group_by_number() in the function
 * run_all_test_variants().
 *
 * @param - stats_p - Current logical error injection statistics
 * examined and compared against expected values. Unexpected values
 * result in a fault return value.
 *
 * @return On success this function returns FBE_STATUS_OK. Otherwise,
 * one of the other FBE_STATUS values is returned to indicate that
 * errors are present.
 *
 * @author
 *  8/18/2010 - Created. Mike Polia
 *
 ****************************************************************/
static fbe_status_t bob_the_builder_check_for_errors(fbe_raid_group_type_t raid_type,
                                                     fbe_object_id_t rg_object_id,
                                                     fbe_api_logical_error_injection_get_stats_t *stats_p)
{
    fbe_status_t status;

    /* Make sure there were no errors.
     */

    if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;

        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* Wait for the verify on each mirrored pair.
         */
        for (index = 0; 
             index < downstream_object_list.number_of_downstream_objects; 
             index++)
        {
            status = abby_cadabby_wait_for_raid_group_verify(downstream_object_list.downstream_object_list[index], 
                                                             BOB_THE_BUILDER_VERIFY_WAIT_SECONDS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }
    else
    {
        status = abby_cadabby_wait_for_raid_group_verify(rg_object_id, BOB_THE_BUILDER_VERIFY_WAIT_SECONDS);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Make sure that we received enough errors.
     */
    status = fbe_api_logical_error_injection_get_stats(stats_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats_p->b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats_p->num_records, 1);

    if (raid_type != FBE_RAID_GROUP_TYPE_RAID10)
    {
        MUT_ASSERT_INT_EQUAL(stats_p->num_objects_enabled, 1);
        MUT_ASSERT_INT_EQUAL(stats_p->num_objects, 1);
    }
    else
    {
        MUT_ASSERT_INT_EQUAL(stats_p->num_objects_enabled, 2);
        MUT_ASSERT_INT_EQUAL(stats_p->num_objects, 2);
    }

    MUT_ASSERT_INT_EQUAL(stats_p->num_objects_enabled, stats_p->num_objects);

    mut_printf(MUT_LOG_TEST_STATUS, "%s stats_p->num_objects %d, stats_p->num_objects_enabled %d", 
               __FUNCTION__, stats_p->num_objects, stats_p->num_objects_enabled);

    /*! check that errors occurred, and that errors were validated.
     */
    MUT_ASSERT_TRUE(stats_p->num_failed_validations == 0);

    return status;
}
/*!**************************************************************
 * check_for_errors()
 ***************************************************************/


/*!**************************************************************
 * stop_logical_error_injection()
 ****************************************************************
 * @brief
 *
 *  Stops the logical error injection prior to gathering the test
 *  verification statistics.
 *
 * @param - verify_report_p - After the logical error injection is
 * stopped, the verification reported is passed back to the caller via
 * this pointer.
 *
 * @param - raid_type - The RAID type being tested. One of R0, R1, R3,
 * R5, R6, R1+0.
 *
 * @param - rg_object_id - The RAID group object ID returned by
 * fbe_api_database_lookup_raid_group_by_number() in the function
 * run_all_test_variants().
 *
 * @param - class_id - RAID class. One of FBE_CLASS_ID_PARITY,
 * FBE_CLASS_ID_STRIPER, FBE_CLASS_ID_MIRROR.
 *
 * @param - lun_object_id - The LUN object ID returned by
 * fbe_api_database_lookup_lun_by_number() in the function
 * run_all_test_variants().
 *
 * @return On success this function returns FBE_STATUS_OK. Otherwise,
 * one of the other FBE_STATUS values is returned to indicate the
 * fault.
 *
 * @author
 *  8/18/2010 - Created. Mike Polia
 *
 ****************************************************************/
static fbe_status_t stop_logical_error_injection(fbe_lun_verify_report_t *verify_report_p,
                                                 fbe_raid_group_type_t raid_type,
                                                 fbe_object_id_t rg_object_id,
                                                 fbe_class_id_t class_id,
                                                 fbe_object_id_t lun_object_id)
{
    fbe_status_t status;

    fbe_api_logical_error_injection_get_stats_t stats, *stats_p = &stats;

    /* Stop logical error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== disable error injection for class %s ==", class_id_name(class_id));

    if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;

        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* Now enable injection for each mirror.
         */
        for (index = 0; 
             index < downstream_object_list.number_of_downstream_objects; 
             index++)
        {
            status = fbe_api_logical_error_injection_disable_object(downstream_object_list.downstream_object_list[index], 
                                                                    FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
    else
    {
        status = fbe_api_logical_error_injection_disable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_get_stats(stats_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats_p->b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats_p->num_objects_enabled, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "%s stats_p->num_objects %d, stats_p->num_objects_enabled %d", 
               __FUNCTION__, stats_p->num_objects, stats_p->num_objects_enabled);

    status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, verify_report_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
/***************************************************************
 * end stop_logical_error_injection()
 ***************************************************************/


/*!**************************************************************
 * get_object_stats()
 ****************************************************************
 * @brief
 *
 *  After the logical error injection is stopped, and the verification
 *  statistics are gathered, this function is used to gather the RAID
 *  group object statistics.
 *
 * @param - raid_group_error_stats_p - The RAID group object
 * statistics are returned to the caller via this pointer.
 *
 * @param - raid_type - The RAID type being tested. One of R0, R1, R3,
 * R5, R6, R1+0.
 *
 * @param - rg_object_id - The RAID group object ID returned by
 * fbe_api_database_lookup_raid_group_by_number() in the function
 * run_all_test_variants().
 *
 * @return On success this function returns FBE_STATUS_OK. Otherwise,
 * one of the other FBE_STATUS values is returned to indicate the
 * fault.
 *
 * @author
 *  8/18/2010 - Created. Mike Polia
 *
 ****************************************************************/
static fbe_status_t get_object_stats(fbe_api_logical_error_injection_get_object_stats_t *raid_group_error_stats_p,
                                     fbe_raid_group_type_t raid_type,
                                     fbe_object_id_t rg_object_id)
{
    fbe_status_t status;

    if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* Raid-10s need to get the stats from each mirror. 
         */
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;

        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* Zero the statistics before accumulating them from the object.
         */
        memset(raid_group_error_stats_p, 0, sizeof(fbe_api_logical_error_injection_get_object_stats_t));

        /* Get the stats for each mirror.
         */
        for (index = 0; 
             index < downstream_object_list.number_of_downstream_objects; 
             index++)
        {
            status = fbe_api_logical_error_injection_accum_object_stats(raid_group_error_stats_p, downstream_object_list.downstream_object_list[index]);
        }
    }
    else
    {
        /* Get the stats for this object.
         */
        status = fbe_api_logical_error_injection_get_object_stats(raid_group_error_stats_p, rg_object_id);
    }

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
/***************************************************************
 * end get_object_stats()
 ***************************************************************/


/*!**************************************************************
 * get_correctable_media_errors_count()
 ****************************************************************
 * @brief
 *
 *  After the logical error injection is stopped, and the verification
 *  statistics are gathered, this function is used to gather the count
 *  of correctable media errors that occurred. The count is extracted
 *  from the verification report (verify_report_p).
 *
 * @param - raid_type - The RAID type being tested. One of R0, R1, R3,
 * R5, R6, R1+0.
 *
 * @param - class_id - RAID class. One of FBE_CLASS_ID_PARITY,
 * FBE_CLASS_ID_STRIPER, FBE_CLASS_ID_MIRROR.
 *
 * @param - err_type - Specifies either a soft or hard media
 * error. Should match the type of media error injected.
 *
 * @param - verify_report_p - After the logical error injection is
 * stopped, the verification reported is passed back to the caller via
 * this pointer.
 *
 * @return On success, this function returns the count of correctable
 * media errors that were reported in the verify_report_p. Otherwise,
 * a MUT failure message is displayed and the test is abruptly halted.
 *
 * @author
 *  8/18/2010 - Created. Mike Polia
 *
 ****************************************************************/
static fbe_u32_t get_correctable_media_errors_count(fbe_raid_group_type_t raid_type, 
                                                    fbe_class_id_t class_id, 
                                                    fbe_api_logical_error_injection_type_t err_type, 
                                                    fbe_lun_verify_report_t *verify_report_p)
{
    if (err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR)
    {
        return verify_report_p->current.correctable_soft_media;
    }
    else if (err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR)
    {
        switch (class_id)
        {
        case FBE_CLASS_ID_PARITY:
        case FBE_CLASS_ID_MIRROR:
            return verify_report_p->current.correctable_media;
            break;

        case FBE_CLASS_ID_STRIPER:
            return verify_report_p->current.correctable_media;

        default:
            MUT_FAIL_MSG("Invalid class ID.");
            return 0;
        }
    }
    else
    {
        MUT_FAIL_MSG("Invalid media error type.");
        return 0;
    }
}
/***************************************************************
 * end get_correctable_media_errors_count()
 ***************************************************************/


/*!**************************************************************
 * get_test_count
 ****************************************************************
 * @brief
 *
 * This function returns the count of tests configurations that should
 * be used, based on the maximum value between the extended test level
 * and the RAID test level.
 *
 * @param - error_cases_p - This contains the count of all possible
 * test cases configurations available for use.
 *
 * @return - This function returns the count of tests configurations
 * that should be used, based on the maximum value between the
 * extended test level and the RAID test level.
 *
 * @author
 *  9/17/2010 - Created. Mike Polia
 *
 ****************************************************************/
static fbe_u32_t get_test_count(bob_the_builder_error_case_t *error_cases_p)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_extended_testing_level();
    fbe_u32_t raid_test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Highest one wins.
     */
    test_level = FBE_MAX(test_level, raid_test_level);

    switch (test_level)
    {
    case 1:
        /* Run a few tests.
         */
        return 2;

        /* Base case, run 1 test.
         */
    case 0:
        return 1;

        /* The default is to run all tests across the
         * selected RAID groups.
         */
    default:
        return bob_the_builder_get_test_count(error_cases_p);
    }
}
/******************************************
 * get_test_count
 ******************************************/


/*!**************************************************************
 * get_raid_group_count
 ****************************************************************
 * @brief
 *
 * This function returns the count of RAID groups that should be used,
 * based on the value of the RAID test level.
 *
 * @param - first_config_p - Beginning of list of all possible RAID
 * groups that can be used.
 *
 * @return - This function returns the count of RAID groups that
 * should be used, based on the the RAID test level.
 *
 * @author
 *  9/17/2010 - Created. Mike Polia
 *
 ****************************************************************/
static fbe_u32_t get_raid_group_count(fbe_test_rg_configuration_t *first_config_p)
{
    fbe_u32_t raid_test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    switch (raid_test_level)
    {
    case 1:
        /* Use all possible RAID groups.
         */
        return fbe_test_get_rg_array_length(first_config_p);

        /* Base case, use only 1 RAID groups.
         */
    case 0:
        return 1; /* we have only 1 raid group entry per type after remove the usage of 512 block size configuration */

        /* The default is to use one RAID group only.
         */
    default:
        return 1;
    }
}
/******************************************
 * get_raid_group_count
 ******************************************/


/*!**************************************************************
 * bob_the_builder_run_all_test_variants()
 ****************************************************************
 * @brief
 *
 * This is the control function that selects and runs all the
 * combinations of RAID group tests, given the RAID group
 * configuration, associated test cases, possible media errors, and
 * expected results.
 *
 * @param - test_config_p - One of list_of_tests_that_bob_can_run[].
 *
 * @return None.
 *
 * @author
 *  8/18/2010 - Created. Mike Polia
 *
 ****************************************************************/
static void bob_the_builder_run_all_test_variants(fbe_test_rg_configuration_t * rg_config_p, void * context_p)
{
    bob_the_builder_error_case_t *orig_error_cases_p = (bob_the_builder_error_case_t *) context_p;
    fbe_u32_t test_index = 0;
    fbe_u32_t error_type_idx;    
    fbe_object_id_t rg_object_id;
    fbe_object_id_t lun_object_id;
    fbe_status_t status;
    bob_the_builder_error_case_t *error_cases_p;

    while (rg_config_p->width != FBE_U32_MAX)
    {    
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    
        /* Let system verify complete before we proceed. */
        fbe_test_sep_util_wait_for_initial_verify(rg_config_p);
    
        /* Loop over the desired error types.
         */
        for (error_type_idx = 0; error_type_idx < ERROR_TYPE_LIST_LENGTH; error_type_idx++)
        {
            error_cases_p = orig_error_cases_p;
            while(error_cases_p->lba_to_read != FBE_U32_MAX)
            {
                /* At this point, we know the RAID group type and the
                 * error type (soft vs. hard). So, we can specify the calc
                 * functions for the particular error type that
                 * corresponds to the RAID group type being tested.
                 */
    
                /* Write the background pattern to seed this element size
                 * before starting the test.
                 */
                big_bird_write_background_pattern();
    
                error_cases_p->record.err_type = error_type_list[error_type_idx].err_type;
    
                fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    
                /* It will give first LUN number's object-id in RG.
                 */
                fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    
                mut_printf(MUT_LOG_TEST_STATUS, "== Testing %s, blksz:%d test # %d, RG ID %d. ==",
                           error_type_list[error_type_idx].description, rg_config_p->block_size, test_index, rg_config_p->raid_group_id);
    
                /* When calling the test function, we need to specify
                 * the calc functions for the particular error type
                 * (soft vs. hard) that corresponds to the RAID group
                 * type being tested.
                 */
    
                bob_the_builder_media_error_test(rg_config_p->raid_type,
                                                 rg_config_p->class_id, 
                                                 lun_object_id,
                                                 rg_object_id,                                                 
                                                 error_cases_p);
    
                /* Destroy all the objects the logical error
                 * injection service is tracking.  This will allow
                 * us to start fresh when we start the next test
                 * with number of errors and number of objects
                 * reported by lei.
                 */
                status = fbe_api_logical_error_injection_destroy_objects();
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                test_index++;
                error_cases_p++;
            }
        }
        rg_config_p++;
    }

    return;
}
/******************************************
 * bob_the_builder_run_all_test_variants
 ******************************************/


/*!**************************************************************
 * bob_the_builder_media_error_test()
 ****************************************************************
 * @brief
 *
 * This is the guts of the media error test. For any offered combo of
 * RAID group type and corresponding error case, hard and soft media
 * errors are injected concurrent with I/O read operations. The
 * resulting RAID group behaviour is compared against expected
 * results. Any faulty comparison causes the test to abort.
 *
 * @param - raid_type - The RAID type being tested. One of R0, R1, R3,
 * R5, R6, R1+0.
 *
 * @param - class_id - RAID class. One of FBE_CLASS_ID_PARITY,
 * FBE_CLASS_ID_STRIPER, FBE_CLASS_ID_MIRROR.
 *
 * @param - lun_object_id - The LUN object ID returned by
 * fbe_api_database_lookup_lun_by_number() in the function
 * run_all_test_variants().
 *
 * @param - rg_object_id - The RAID group object ID returned by
 * fbe_api_database_lookup_raid_group_by_number() in the function
 * run_all_test_variants().
 *
 * @param - error_case_p - One of the error test cases listed in the
 * file bob_the_build_test_cases.h.
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *  8/18/2010 - Modified. Mike Polia
 *
 ****************************************************************/
static void bob_the_builder_media_error_test(fbe_raid_group_type_t raid_type,
                                             fbe_class_id_t class_id,                                             
                                             fbe_object_id_t lun_object_id,
                                             fbe_object_id_t rg_object_id,                                             
                                             bob_the_builder_error_case_t *error_case_p)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t context;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_lun_verify_report_t verify_report;
    fbe_api_logical_error_injection_get_object_stats_t raid_group_error_stats;
    fbe_u32_t correctable_media_errors;
    fbe_object_id_t        hook_object_id = rg_object_id;
    bob_the_builder_expected_results_t *expected_results;
    fbe_u64_t num_errors_injected;

    if(error_case_p->record.err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR)
    {
        expected_results = &error_case_p->soft_expected_results;
    }
    else
    {
        expected_results = &error_case_p->hard_expected_results;
    }

    status = error_test_prelude(raid_type, lun_object_id, rg_object_id, &stats, error_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_api_base_config_downstream_object_list_t downstream_object_list;

        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
        hook_object_id = downstream_object_list.downstream_object_list[0];
    }
    
    status = fbe_api_scheduler_add_debug_hook(hook_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                              FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                              0,0,
                                              SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Send I/O.
     */
    status = fbe_api_rdgen_send_one_io(&context,
                                       lun_object_id,
                                       FBE_CLASS_ID_INVALID,
                                       FBE_PACKAGE_ID_SEP_0,
                                       FBE_RDGEN_OPERATION_READ_ONLY,
                                       FBE_RDGEN_LBA_SPEC_FIXED,
                                       error_case_p->lba_to_read,
                                       error_case_p->blocks_to_read,
                                       FBE_RDGEN_OPTIONS_DISABLE_RANDOM_SG_ELEMENT,  /* makes IO path predictable for num reads injected */
                                       0, 0, /* no expiration or abort time */
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);

    status = fbe_test_wait_for_debug_hook(hook_object_id, 
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                         FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                         SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    status = fbe_api_scheduler_del_debug_hook(hook_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                              FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                              0,0,
                                              SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if(expected_results->b_correctable)
    {
        MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count,0 );
        MUT_ASSERT_INT_EQUAL(context.start_io.statistics.aborted_error_count, 0);
        MUT_ASSERT_INT_EQUAL(context.start_io.statistics.io_failure_error_count, 0);
        MUT_ASSERT_INT_EQUAL(context.start_io.statistics.media_error_count, 0);
    }
    else
    {
        MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count,1 );
        MUT_ASSERT_INT_EQUAL(context.start_io.statistics.aborted_error_count, 0);
        MUT_ASSERT_INT_EQUAL(context.start_io.statistics.io_failure_error_count, 0);
        MUT_ASSERT_INT_EQUAL(context.start_io.statistics.media_error_count, 1);
    }
    status = bob_the_builder_check_for_errors(raid_type, rg_object_id, &stats);

    MUT_ASSERT_UINT64_EQUAL_MSG(stats.num_validations, expected_results->num_validations,
                                "Unexpected number of validation\n");

    status = stop_logical_error_injection(&verify_report, raid_type, rg_object_id, class_id, lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    correctable_media_errors = get_correctable_media_errors_count(raid_type, class_id, error_case_p->record.err_type, &verify_report);

    MUT_ASSERT_INT_EQUAL_MSG(expected_results->num_c_media_error, correctable_media_errors, 
                             "Unexpected number of correctable media errors detected \n");
    
    /* Expect uncorrectable media errors only for non-redundant RAID groups with hard media errors.
     */
    MUT_ASSERT_UINT64_EQUAL_MSG(verify_report.current.uncorrectable_media, expected_results->num_u_media_error,
                                "Unexpected number of uncorrectable media errors detected \n");

    status = get_object_stats(&raid_group_error_stats, raid_type, rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS,
               "raid_group_error_stats: num_read_media_errors_injected %lld, num_write_verify_blocks_remapped %lld, num_errors_injected %lld, num_validations %lld",
               (long long)raid_group_error_stats.num_read_media_errors_injected,
               (long long)raid_group_error_stats.num_write_verify_blocks_remapped,
               (long long)raid_group_error_stats.num_errors_injected,
               (long long)raid_group_error_stats.num_validations);

    MUT_ASSERT_UINT64_EQUAL_MSG(expected_results->num_read_media_errors_injected, raid_group_error_stats.num_read_media_errors_injected,
                                "Unexpected number of read media errors detected \n");

    MUT_ASSERT_UINT64_EQUAL_MSG(expected_results->num_write_verify_blocks_remapped, raid_group_error_stats.num_write_verify_blocks_remapped, 
                                "Unexpected number of write verify errors detected \n");

    num_errors_injected = expected_results->num_read_media_errors_injected + expected_results->num_write_verify_blocks_remapped;
    MUT_ASSERT_UINT64_EQUAL_MSG(num_errors_injected, raid_group_error_stats.num_errors_injected, // raid_group_error_stats.num_read_media_errors_injected + raid_group_error_stats.num_write_verify_blocks_remapped, //
                                "Unexpected number of errors injected detected \n");

    /* Restore the sector trace level to it's default.
     */
    fbe_test_sep_util_restore_sector_trace_level();

    return;
}
/******************************************
 * end bob_the_builder_media_error_test()
 ******************************************/

/*!**************************************************************
 * bob_the_builder_test()
 ****************************************************************
 * @brief
 *
 *  This function loops through the
 *  list_of_tests_that_bob_can_run[]. For each of the tests in the
 *  list, the associated RAID group configuration is set-up, all the
 *  variants of that test are run, and finally the test clean-up is
 *  performed (e.g. destroy RAID group, etc.).
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *  8/18/2010 - Modified. Mike Polia
 *
 ****************************************************************/
void bob_the_builder_test(void)
{
    int i = 0;
    fbe_test_rg_configuration_t *rg_config_list_p;

    for ( i = 0; i < BOB_THE_BUILDER_NUM_RG_TYPES; i++ )
    {
        rg_config_list_p = &bob_the_builder_test_config[i].raid_group_config[0];

        mut_printf(MUT_LOG_TEST_STATUS, "!!! Now Running: %s !!!", bob_the_builder_test_config[i].name);

        
        if (i + 1 >= BOB_THE_BUILDER_NUM_RG_TYPES) {
            /* Now create the raid groups and run the tests
            */
            fbe_test_run_test_on_rg_config(rg_config_list_p, bob_the_builder_test_config[i].error_config, 
                                           bob_the_builder_run_all_test_variants,
                                           BOB_THE_BUILDER_LUNS_PER_RAID_GROUP,
                                           BOB_THE_BUILDER_CHUNKS_PER_LUN);
        }
        else {
            /* Now create the raid groups and run the tests
            */
            fbe_test_run_test_on_rg_config_with_time_save(rg_config_list_p, bob_the_builder_test_config[i].error_config, 
                                           bob_the_builder_run_all_test_variants,
                                           BOB_THE_BUILDER_LUNS_PER_RAID_GROUP,
                                           BOB_THE_BUILDER_CHUNKS_PER_LUN,
                                           FBE_FALSE);
        }

        /* Make sure we did not get any trace errors.  We do this in between each
         * raid group test since it stops the test sooner. 
         */
        fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
        
    }

    return;
}
/******************************************
 * end bob_the_builder_test()
 ******************************************/

static void bob_the_builder_run_encrypted_test(fbe_test_rg_configuration_t * rg_config_p, void * context_p)
{
    fbe_object_id_t pdo_id;
    fbe_object_id_t lun_object_id;
    fbe_status_t status;
    fbe_protocol_error_injection_error_record_t record;
    fbe_protocol_error_injection_record_handle_t record_handle = NULL;
    fbe_api_rdgen_context_t context;
    fbe_u32_t i;

    /* Get the drive that is part of RG */
    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[0].bus,
                                                              rg_config_p->rg_disk_set[0].enclosure,
                                                              rg_config_p->rg_disk_set[0].slot,
                                                              &pdo_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* It will give first LUN number's object-id in RG.
     */
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Initiate Error Injection of Media Error */
     fbe_test_neit_utils_init_error_injection_record(&record);

    /* Put PDO object ID into the error record
     */
    record.object_id = pdo_id;

    /* Fill in the fields in the error record that are common to all
     * the error cases.
     */
    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.port_status =
        FBE_PORT_REQUEST_STATUS_SUCCESS;

    /* Inject the error
     */
    record.lba_start = 0;
    record.lba_end = 0xFFFFFFFF ;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        FBE_SCSI_WRITE_16;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = 
        0x03 /* FBE_SCSI_SENSE_KEY_MEDIUM_ERROR */;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = 
        ( 0x011 )/* (FBE_SCSI_ASC_READ_DATA_ERROR )*/;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = 
        0x00; 
    record.num_of_times_to_insert = 1;
    record.reassign_count = 0;

    for(i =0; i <PROTOCOL_ERROR_INJECTION_MAX_ERRORS ; i++)
    {
       record.b_active[i] = FBE_TRUE;
    } 

     record.b_test_for_media_error = FBE_TRUE;
    
    /* Add the error record
     */
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle);	
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

     /* Start the error injection 
     */
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Fill it with IO */
     /* Send I/O.
     */
    status = fbe_api_rdgen_send_one_io(&context,
                                       lun_object_id,
                                       FBE_CLASS_ID_INVALID,
                                       FBE_PACKAGE_ID_SEP_0,
                                       FBE_RDGEN_OPERATION_WRITE_ONLY,
                                       FBE_RDGEN_LBA_SPEC_FIXED,
                                       0,
                                       100,
                                       FBE_RDGEN_OPTIONS_INVALID,
                                       0, 0, /* no expiration or abort time */
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);

    /* Validate that we inserted an error */
    status = fbe_api_protocol_error_injection_get_record(record_handle, &record);
    MUT_ASSERT_INT_NOT_EQUAL_MSG(record.times_inserted, 0, "Unexpected number of errors inserted");

    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
}

/*!**************************************************************
 * bob_the_builder_encrypted_test()
 ****************************************************************
 * @brief
 *
 *  This function loops through the
 *  list_of_tests_that_bob_can_run[]. For each of the tests in the
 *  list, the associated RAID group configuration is set-up, all the
 *  variants of that test are run, and finally the test clean-up is
 *  performed (e.g. destroy RAID group, etc.).
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *  8/18/2010 - Modified. Mike Polia
 *
 ****************************************************************/
void bob_the_builder_encrypted_test(void)
{
    fbe_test_rg_configuration_t *rg_config_list_p;

   rg_config_list_p = &bob_the_builder_test_config[1].raid_group_config[0];

        mut_printf(MUT_LOG_TEST_STATUS, "!!! Now Running: %s !!!", bob_the_builder_test_config[1].name);

        
        /* Now create the raid groups and run the tests
        */
            fbe_test_run_test_on_rg_config_with_time_save(rg_config_list_p, bob_the_builder_test_config[1].error_config, 
                                           bob_the_builder_run_encrypted_test,
                                           BOB_THE_BUILDER_LUNS_PER_RAID_GROUP,
                                           BOB_THE_BUILDER_CHUNKS_PER_LUN,
                                           FBE_FALSE);
        
        /* Make sure we did not get any trace errors.  We do this in between each
         * raid group test since it stops the test sooner. 
         */
        fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
        

    return;
}
/******************************************
 * end bob_the_builder_encrypted_test()
 ******************************************/

void bob_the_builder_test_520(void)
{
    bob_the_builder_test();
}

void bob_the_builder_test_4k(void)
{
    bob_the_builder_test();
}

/*!**************************************************************
 * bob_the_builder_setup_520()
 ****************************************************************
 * @brief
 *  Setup the bob_the_builder test for 520 bps drives.
 *  This satisfies the requirement for a MUT set-up function.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *  8/18/2010 - Modified. Mike Polia
 *
 ****************************************************************/
void bob_the_builder_setup_520(void)
{
    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_pp_util_create_physical_config_for_disk_counts(BOB_THE_BUILDER_NUM_520_DRIVES,
                                                                0 /* 4k drives */,
                                                                TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);

        elmo_load_sep_and_neit();
    }

    fbe_test_disable_background_ops_system_drives();

    /* Disable Service timeouts and HC for debugging purposes.
       DO NOT CHECK IN
    */
#if 0
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_HEALTH_CHECK_ENABLED, FBE_FALSE /*disable*/);  
    fbe_api_drive_configuration_set_service_timeout(-1);
    fbe_api_drive_configuration_set_remap_service_timeout(-1);
    fbe_test_set_timeout_ms(3600*1000);   
#endif

    return;
}
/**************************************
 * end bob_the_builder_setup()
 **************************************/


/*!**************************************************************
 * bob_the_builder_setup_4k()
 ****************************************************************
 * @brief
 *  Setup the bob_the_builder test for 4K bps drives.
 *  This satisfies the requirement for a MUT set-up function.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  6/4/2014 - Created. Wayne Garrett
 *
 ****************************************************************/
void bob_the_builder_setup_4k(void)
{
    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {        
        fbe_test_rg_configuration_t *rg_config_list_p;
        fbe_u32_t i;

        /* change config to use 4K drives */
        for ( i = 0; i < BOB_THE_BUILDER_NUM_RG_TYPES; i++ )
        {
            rg_config_list_p = &bob_the_builder_test_config[i].raid_group_config[0];

            while (rg_config_list_p->width != FBE_U32_MAX){
                rg_config_list_p->block_size = 4160;
                rg_config_list_p++;
            }
        }

        fbe_test_pp_util_create_physical_config_for_disk_counts(0 /*520 drives*/,
                                                                BOB_THE_BUILDER_NUM_4160_DRIVES,
                                                                TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
        elmo_load_sep_and_neit();
    }

    fbe_test_disable_background_ops_system_drives();

    /* Disable Service timeouts and HC for debugging purposes.
       DO NOT CHECK IN
    */
#if 0
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_HEALTH_CHECK_ENABLED, FBE_FALSE /*disable*/);  
    fbe_api_drive_configuration_set_service_timeout(-1);
    fbe_api_drive_configuration_set_remap_service_timeout(-1);
    fbe_test_set_timeout_ms(3600*1000);   
#endif

    return;
}
/**************************************
 * end bob_the_builder_setup()
 **************************************/

/*!**************************************************************
 * bob_the_builder_encrypted_setup()
 ****************************************************************
 * @brief
 *  bob the builder sp encryption setup
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/
void bob_the_builder_encrypted_setup(void)
{
     /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_pp_util_create_physical_config_for_disk_counts(BOB_THE_BUILDER_NUM_520_DRIVES,
                                                                BOB_THE_BUILDER_NUM_4160_DRIVES,
                                                                TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);

        elmo_load_sep_and_neit();
        sep_config_load_kms(NULL);
        fbe_test_sep_util_enable_kms_encryption();
    }

    return;
}
/*************************************************
 * end bob_the_builder_encrypted_setup()
 *************************************************/

/*!**************************************************************
 * bob_the_builder_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the bob_the_builder test.
 *  This satisfies the requirement for a MUT clean-up function.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *  8/18/2010 - Modified. Mike Polia
 *
 ****************************************************************/
void bob_the_builder_cleanup(void)
{
    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    fbe_test_sep_util_disable_trace_limits();

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end bob_the_builder_cleanup()
 ******************************************/

/*!**************************************************************
 * bob_the_builder_encrypted_cleanup()
 ****************************************************************
 * @brief
 *  bob the builder sp encryption setup
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/
void bob_the_builder_encrypted_cleanup(void)
{
    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    fbe_test_sep_util_disable_trace_limits();

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_kms();
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
    
}
/*************************************************
 * end bob_the_builder_encrypted_cleanup()
 *************************************************/

/**********************************
 * end file bob_the_builder_test.c
 *********************************/
