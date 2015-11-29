/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file harry_potter_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains tests for both normal and error path handling to a raw mirror.
 * 
 *  In production code, a raw mirror is used to access the system LUNs non-paged metadata
 *  and their configuration tables.  There is no object interface for this data.  It is
 *  accessed early in boot before any objects have been created.  
 * 
 *  These tests send I/O to a raw mirror via NEIT's raw mirror service.
 * 
 *  For error handling, raw mirror drives are removed before or during testing.
 *  In some cases, NEIT's Logical Error Injection (LEI) service is used to inject
 *  errors on raw mirror drives.
 * 
 *  The raw mirror service is used to test fbe_raid_raw_mirror_library
 *  and associated RAID library code.
 * 
 * @version
 *  10/2011  Susan Rundbaken  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_raw_mirror_service_interface.h"
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
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"

/*************************
 *    DEFINITIONS
 *************************/

char * harry_potter_short_desc = "Raw mirror read and write I/O test.";
char * harry_potter_long_desc ="\
The Harry Potter scenario tests I/O to the raw mirror.\n\
A raw mirror is used to access the system LUNs non-paged metadata and their configuration tables.\n\
NEIT's raw mirror service will be used by the test to drive I/O to a raw mirror.\n\
STEP 1: Load the physical configuration and SEP/NEIT packages.\n\
        - The physical configuration and SEP/NEIT packages are loaded for access to the system drives and\n\
          raw mirror service.\n\
        - When NEIT is loaded, the raw mirror service will be initialized which will create a raw mirror for testing.\n\
STEP 2: Write with no magic number on raw mirror drives (all drives).\n\
STEP 3: Read-check test with no previous magic/sequence number (all drives).\n\
STEP 4: Write with magic number/sequence number same on all drives.\n\
STEP 5: Read-check with magic number/sequence number same on all drives.\n\
STEP 6: Simple write test to make sure raw mirror initialized properly\n\
STEP 7: Simple read-check test to confirm raw mirror initialized properly\n\
STEP 8: Read-check with sequence number mismatch on 1 drive\n\
STEP 9: Read-check with magic number mismatch on 1 drive\n\
STEP 10: Read-check with sequence number mismatch on 1 drive; media error on another drive\n\
STEP 11: Read-check with media number mismatch on 1 drive; media error on another drive\n\
STEP 12: Write-verify with media error on 1 drive\n\
STEP 13: Read-check with correctable magic number error on 1 drive; sequence number mismatch on another drive\n\
STEP 14: Read-check with correctable magic number error on 1 drive; sequence number mismatch on same drive\n\
STEP 15: Read-check with correctable magic number error on 1 drive; sequence number mismatch on same drive at different LBA\n\
STEP 16: Read-check with uncorrectable magic number\n\
STEP 17: Read-check with uncorrectable magic number and CRC error; different LBA\n\
STEP 18: Read-check with uncorrectable magic number and CRC error; same LBA\n\
STEP 19: Read-check with uncorrectable magic number and CRC error at same LBA; correctable magic number error at different LBA\n\
STEP 20: Read-check with uncorrectable media error\n\
STEP 21: Read-check with correctable CRC error\n\
STEP 22: Read-check with correctable magic number error on 1 drive; CRC error on another drive\n\
STEP 23: Read-check with correctable magic number error on 1 drive; CRC error on same drive and LBA\n\
STEP 24: Read-check with correctable seq number error on 1 drive; CRC error on same drive and different LBA\n\
STEP 25: Simple write test to make sure raw mirror initialized properly\n\
STEP 26: Simple read-check test to confirm raw mirror initialized properly\n\
STEP 27: Drive pull while writes in progress\n\
STEP 28: Write test with drive removed.\n\
STEP 29: Read-check test with drive removed.\n\
STEP 30: Write test with drive removed\n\
STEP 31: Read-check test with previous drive reinserted\n\
STEP 32: Write test with 2 drives removed including primary\n\
STEP 33: Read-check test with previous drives reinserted\n\
STEP 34: Write test with 2 drives removed not including primary\n\
STEP 35: Read-check test with previous drives reinserted\n\
STEP 36: Read-check test with 1 drive removed (the primary) and sequence number mismatch on another drive\n\
STEP 37: Read-check test with 1 drive removed (the primary) and magic number mismatch on another drive\n\
STEP 38: Destroy the configuration.\n\
\n\
Description last updated: 12/2011.\n";


/*!*******************************************************************
 * @def HARRY_POTTER_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define HARRY_POTTER_TEST_DRIVE_CAPACITY             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY


/*!*******************************************************************
 * @def HARRY_POTTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 1 encl + 15 pdo) 
 *
 *********************************************************************/
#define HARRY_POTTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS 18 

/*!*******************************************************************
 * @def HARRY_POTTER_TEST_RAW_MIRROR_WIDTH
 *********************************************************************
 * @brief Width of raw mirror
 *
 *********************************************************************/
#define HARRY_POTTER_TEST_RAW_MIRROR_WIDTH  FBE_RAW_MIRROR_WIDTH 

/*!*******************************************************************
 * @def HARRY_POTTER_TEST_RAW_MIRROR_BUS
 *********************************************************************
 * @brief Bus used for raw mirrors
 *
 *********************************************************************/
#define HARRY_POTTER_TEST_RAW_MIRROR_BUS 0

/*!*******************************************************************
 * @def HARRY_POTTER_TEST_RAW_MIRROR_ENCLOSURE
 *********************************************************************
 * @brief Enclosure used for raw mirrors
 *
 *********************************************************************/
#define HARRY_POTTER_TEST_RAW_MIRROR_ENCLOSURE 0

/*!*******************************************************************
 * @var harry_potter_test_contexts
 *********************************************************************
 * @brief This structure contains our context for running raw mirror I/O.
 *
 *********************************************************************/
static fbe_api_raw_mirror_service_context_t harry_potter_test_context;

/*!*******************************************************************
 * @var harry_potter_local_drive_handle
 *********************************************************************
 * @brief This structure contains our drive handle for the local SP for
 *          removing and reinserting drives.
 *
 *********************************************************************/
fbe_api_terminator_device_handle_t  harry_potter_local_drive_handle[HARRY_POTTER_TEST_RAW_MIRROR_WIDTH];

/*!*******************************************************************
 * @var harry_potter_peer_drive_handle
 *********************************************************************
 * @brief This structure contains our drive handle for the peer SP for
 *          removing and reinserting drives.
 *
 *********************************************************************/
fbe_api_terminator_device_handle_t  harry_potter_peer_drive_handle[HARRY_POTTER_TEST_RAW_MIRROR_WIDTH];

/*!*******************************************************************
 * @def HARRY_POTTER_WAIT_MSEC
 *********************************************************************
 * @brief Max number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define HARRY_POTTER_WAIT_MSEC 120000

/*!*******************************************************************
 * @def HARRY_POTTER_MAX_BASIC_TESTS
 *********************************************************************
 * @brief Max number of basic tests.
 *
 *********************************************************************/
#define HARRY_POTTER_MAX_BASIC_TESTS 4

/*!*******************************************************************
 * @def HARRY_POTTER_MAX_LEI_TESTS
 *********************************************************************
 * @brief Max number of Logical Error Injection tests.
 *
 *********************************************************************/
#define HARRY_POTTER_MAX_LEI_TESTS 19

/*!*******************************************************************
 * @def HARRY_POTTER_MAX_DRIVE_REMOVAL_TESTS
 *********************************************************************
 * @brief Max number of Logical Error Injection tests.
 *
 *********************************************************************/
#define HARRY_POTTER_MAX_DRIVE_REMOVAL_TESTS 13

/*!*******************************************************************
 * @def HARRY_POTTER_MAX_LEI
 *********************************************************************
 * @brief Max number of Logical Error injection records.
 *
 *********************************************************************/
#define HARRY_POTTER_MAX_LEI  3

/*!*******************************************************************
 * @def HARRY_POTTER_MAX_DRIVE_REMOVE
 *********************************************************************
 * @brief Max number of raw mirror drives to remove.
 *
 *********************************************************************/
#define HARRY_POTTER_MAX_DRIVE_REMOVE  3

/*!********************************************************************* 
 * @struct harry_potter_test_record_t
 *  
 * @brief 
 *   Contains the test parameters and expected results for an I/O request.
 *
 **********************************************************************/
typedef struct harry_potter_test_record_s
{
    /* Test parameters. */
    fbe_raw_mirror_service_operation_t          raw_mirror_operation;
    fbe_data_pattern_t                          pattern;
    fbe_u32_t                                   sequence_id;
    fbe_u32_t                                   block_size;
    fbe_u32_t                                   block_count;
    fbe_lba_t                                   start_lba;
    fbe_u64_t                                   sequence_num;
    fbe_bool_t                                  dmc_expected_b;
    fbe_payload_block_operation_status_t        expected_block_status;
    fbe_payload_block_operation_qualifier_t     expected_block_qualifier;
    fbe_u32_t                                   max_passes;
    fbe_u32_t                                   num_threads;

    /* Test results */
    fbe_bool_t                                  errors_expected;
    fbe_u32_t                                   dead_err_count;
    fbe_u32_t                                   u_rm_magic_count;
    fbe_u32_t                                   c_rm_magic_count;
    fbe_u32_t                                   c_rm_seq_count;
    fbe_u16_t                                   rm_magic_bitmap;
    fbe_u16_t                                   rm_seq_bitmap;
    fbe_u32_t                                   c_media_count;
    fbe_u32_t                                   u_media_count;
    fbe_u32_t                                   c_crc_count;
    fbe_u32_t                                   u_crc_count;
}
harry_potter_test_record_t;


/*!********************************************************************* 
 * @struct harry_potter_lei_test_params_t
 *  
 * @brief 
 *   Contains the Logical Error Injection test parameters for a LEI test.
 *
 **********************************************************************/
typedef struct harry_potter_lei_test_params_s
{
    fbe_u32_t                                   position_bitmap;
    fbe_lba_t                                   lba;
    fbe_block_count_t                           num_blocks;
    fbe_api_logical_error_injection_type_t      error_type;
    fbe_u32_t                                   num_objects_enabled;
    fbe_u32_t                                   num_records;
    fbe_u16_t                                   err_adj_bitmask;
}
harry_potter_lei_test_params_t;


/*!********************************************************************* 
 * @struct harry_potter_test_case_t
 *  
 * @brief 
 *   Contains the test description for a test case.
 *
 **********************************************************************/
typedef struct harry_potter_test_case_s
{
    harry_potter_lei_test_params_t      lei_params[HARRY_POTTER_MAX_LEI];
    fbe_u32_t                           drives_to_remove[HARRY_POTTER_MAX_DRIVE_REMOVE];
    fbe_bool_t                          remove_drives_before_io_b;
    fbe_bool_t                          remove_drives_after_io_b;
    fbe_bool_t                          reinsert_drives_before_io_b;
    fbe_bool_t                          reinsert_drives_after_io_b;
    fbe_bool_t                          flush_data_b;
    fbe_bool_t                          run_io_test_repeatedly_b;
    harry_potter_test_record_t          test_record;
}
harry_potter_test_case_t;


/*!*******************************************************************
 * @var harry_potter_test_basis_tests
 *********************************************************************
 * @brief These are basic tests for raw mirror I/O testing.
 *
 *********************************************************************/
static harry_potter_test_case_t  harry_potter_basic_tests[HARRY_POTTER_MAX_BASIC_TESTS] =
{   
    /* Test1: Write test with no magic number/sequence number. */
    {
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {   /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             FBE_LBA_INVALID, /* invalid sequence number means no magic number set either */        
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
             /* Expected errors */
             FBE_FALSE,  /* Errors */
             0,          /* Dead errs */
             0,          /* Uncorr magic cnt */      
             0,          /* Corr magic cnt */    
             0,          /* Corr seq cnt */  
             0x0,        /* Magic bitmap */  
             0x0,        /* Seq bitmap */
             0,          /* Corr media cnt */    
             0,          /* Uncorr media cnt */      
             0,          /* Corr CRC cnt */  
             0,          /* Uncorr CRC cnt */
        },
    },

    /* Test2: Read-check test with no previous magic number/sequence number. */

    {
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {   /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             FBE_LBA_INVALID,         
             FBE_TRUE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            22,         /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x7,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test3: Simple write test with valid magic number and sequence number. */

    {
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {   /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             5,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test4: Simple read-check test with valid magic number and sequence number. */

    {
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {   /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             5,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },
};



/*!*******************************************************************
 * @var harry_potter_test_lei_tests
 *********************************************************************
 * @brief These are Logical Error Injection test cases for raw mirror I/O testing.
 *
 *********************************************************************/
static harry_potter_test_case_t  harry_potter_test_lei_tests[HARRY_POTTER_MAX_LEI_TESTS] =
{   
    /* Test1: Simple write test to make sure raw mirror initialized properly. */                                                                        
    {
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
        /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
            Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             5,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test2: Simple read-check test to make sure raw mirror initialized properly. */                                                                        

    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             5,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test3: Error injection test; sequence number mismatch. */                                              
    {   
        {/*  Pos bitmap     LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x1,           0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH, 1,            1,           0},
            {FBE_U32_MAX,   0,     0,        0,                                                        0,            0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             5,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            1,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x1,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test4: Error injection test; magic number mismatch. */                                              
    {   
        {/*  Pos bitmap     LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x1,           0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 1,            1,         0},
            {FBE_U32_MAX,   0,     0,        0,                                                          0,            0,         0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             5,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            1,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x1,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test5: Error injection test; sequence number mismatch on one drive, media error on another. */                                              
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH, 1,            1,           0},
            {0x2,        2,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,         2,            2,           0},
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             5,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            1,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x1,        /* Seq bitmap */
            1,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test6: Error injection test; magic number mismatch on one drive, media error on another. */                                              
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 1,            1,         0},
            {0x2,        2,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,           2,            2,         0},
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             5,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            1,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x1,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            1,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test7: Error injection test; write-verify plus media error. */                                              
    {   
        {/*  Pos bitmap     LBA    Blocks    Error                                              Num Objects   Num Records  Err Adj */
            {0x1,           0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,  1,            1,           0},
            {FBE_U32_MAX,   0,     0,        0,                                                 0,            0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_VERIFY,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             9,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test8: Error injection test; magic number mismatch plus sequence number mismatch, different drives, same LBA. */                                              
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH,   1,            1,         0},
            {0x4,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 2,            2,         0},
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             9,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            1,          /* Corr magic cnt */    
            1,          /* Corr seq cnt */  
            0x4,        /* Magic bitmap */  
            0x1,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test9: Error injection test; magic number mismatch plus sequence number mismatch, same drive and LBA. */                                              
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH,   1,            1,         0},
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 1,            2,         0},
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             9,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            1,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x1,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test10: Error injection test; magic number mismatch plus sequence number mismatch, same drive and different LBA. */                                              
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 1,            1,         0},
            {0x1,        1,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH,   1,            2,         0},
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             9,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            1,          /* Corr magic cnt */    
            1,          /* Corr seq cnt */  
            0x1,        /* Magic bitmap */  
            0x1,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test11: Error injection test; uncorrectable magic number. */                                              
    {   
        {/*  Pos bitmap     LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x7,           0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 3,            1,         0x7},
            {FBE_U32_MAX,   0,     0,        0,                                                          0,            0,         0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             FBE_LBA_INVALID,         
             FBE_TRUE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            1,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x7,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test12: Error injection test; uncorrectable magic number plus CRC error; different LBA. */
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x7,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 3,            1,         0x7},
            {0x1,        1,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,                   3,            2,         0x7},
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             FBE_LBA_INVALID,         
             FBE_TRUE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            1,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x7,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            1,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test13: Error injection test; uncorrectable magic number plus CRC error; same LBA. */
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x7,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 3,            1,         0x7},
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,                   3,            2,         0x7},
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             FBE_LBA_INVALID,         
             FBE_TRUE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            1,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x6,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            1,          /* Uncorr CRC cnt */
        },
    },

    /* Test14: Error injection test; uncorrectable magic number plus CRC error at same LBA plus correctable magic number
     *         at different LBA
     */
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x7,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 3,            1,         0x7},
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,                   3,            2,         0x7},
            {0x1,        1,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 3,            3,         0x0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             FBE_LBA_INVALID,         
             FBE_TRUE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            1,          /* Uncorr magic cnt */      
            1,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x7,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            1,          /* Uncorr CRC cnt */
        },
    },

    /* Test15: Error injection test; uncorrectable media error. */
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                             Num Objects   Num Records  Err Adj */
            {0x7,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 3,            1,           0x7},
            {FBE_U32_MAX,0,     0,        0,                                                0,            0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             FBE_LBA_INVALID, /* uncorrectable; so will not check for valid seq num */         
             FBE_TRUE,    /* DMC expected for uncorrectable */
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,    /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            1,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test16: Error injection test; correctable CRC error. */
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                     Num Objects   Num Records  Err Adj */
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 1,            1,           0},
            {FBE_U32_MAX,0,     0,        0,                                        0,            0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x100,    
             520,  
             22,   
             0,           
             FBE_LBA_INVALID,         
             FBE_TRUE, /* checksum error will cause a DMC in raw mirror service */
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            1,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test17: Error injection test; magic number mismatch on one drive, CRC error on another drive. */                                              
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 1,            1,         0},
            {0x2,        2,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,                   2,            2,         0},
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             9,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            1,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x1,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            1,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test18: Error injection test; magic number mismatch on one drive, CRC error on same drive and LBA. */                                              
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 1,            1,         0},
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,                   1,            2,         0},
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             9,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             /* Note: CRC error takes precendence over raw mirror mismatch on same position/LBA */
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,  
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            1,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test19: Error injection test; magic number mismatch on one drive, CRC error on same drive and different LBA. */                                              
    {   
        {/*  Pos bitmap  LBA    Blocks    Error                                                     Num Objects   Num Records  Err Adj */
            {0x1,        0,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH, 1,            1,         0},
            {0x1,        1,     1,        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,                   1,            2,         0},
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             9,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            1,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x1,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            1,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },
};


/*!*******************************************************************
 * @var harry_potter_test_drive_removal_tests
 *********************************************************************
 * @brief These are drive removal test cases for raw mirror I/O testing.
 *
 *********************************************************************/
static harry_potter_test_case_t  harry_potter_test_drive_removal_tests[HARRY_POTTER_MAX_DRIVE_REMOVAL_TESTS] =
{   
    /* Test1: Simple write test to make sure raw mirror initialized properly. */                                                                        
    {
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
        /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
            Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             5,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test2: Simple read-check test to make sure raw mirror initialized properly. */                                                                        
    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {FBE_U32_MAX, 0, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             5,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test3: Raw mirror drive-pull while I/O in progress. */                                              
    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {0, 1, FBE_U32_MAX},
        FBE_FALSE, /* remove drives before I/O */
        FBE_TRUE,  /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_TRUE,  /* reinsert drives after I/O */
        FBE_TRUE,  /* flush data */
        FBE_TRUE,  /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY,
             FBE_DATA_PATTERN_LBA_PASS,
             0x55,    
             520,  
             22,   
             0,           
             3,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID, /* don't care about the status here; can vary for I/O that runs repeatedly */
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID,
             0, /* run forever */
             5,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            FBE_U32_MAX,/* Dead errs (the number can vary since I/O runs repeatedly) */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test4: Write test with a drive removed. */                                              
    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {0, FBE_U32_MAX, 0},
        FBE_TRUE,  /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_TRUE,  /* reinsert drives after I/O */
        FBE_TRUE,  /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY,
             FBE_DATA_PATTERN_LBA_PASS,
             0x65,    
             520,  
             22,   
             0,           
             6,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test5: Read-check test with drive removed. */                                                                        
    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to reinsert */
            {0, FBE_U32_MAX, 0},
        FBE_TRUE,  /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_TRUE,  /* reinsert drives after I/O */
        FBE_TRUE,  /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x65,    
             520,  
             22,   
             0,           
             6,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test6: Write test with drive removed. */                                              
    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {0, FBE_U32_MAX, 0},
        FBE_TRUE,  /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY,
             FBE_DATA_PATTERN_LBA_PASS,
             0x75,    
             520,  
             22,   
             0,           
             7,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test7: Read-check test with drive reinserted. */                                                                        
    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to reinsert */
            {0, FBE_U32_MAX, 0},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_TRUE,  /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_TRUE,  /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x75,    
             520,  
             22,   
             0,           
             7,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            22,         /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x1,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test8: Write test with 2 drives removed including primary. */                                              
    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to remove */
            {0, 1, FBE_U32_MAX},
        FBE_TRUE,  /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */        
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY,
             FBE_DATA_PATTERN_LBA_PASS,
             0x85,    
             520,  
             22,   
             0,           
             8,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test9: Read-check test with drives reinserted. */                                                                        
    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to reinsert */
            {0, 1, FBE_U32_MAX},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_TRUE,  /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_TRUE,  /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x85,    
             520,  
             22,   
             0,           
             8,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            22,         /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x3,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test10: Write test with 2 drives removed not including primary. */                                              
    {
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },         
        /*  Drives to remove */
            {1, 2, FBE_U32_MAX},
        FBE_TRUE,  /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_FALSE, /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             9,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
             1,
             1,
            /* Expected errors */
            FBE_FALSE,  /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test11: Read-check test with drives reinserted. */                                                                        
    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error   Num Objects     Num Records  Err Adj*/
            {FBE_U32_MAX,       0,     0,       0,      0,              0,           0},
        },
        /*  Drives to reinsert */
            {1, 2, FBE_U32_MAX},
        FBE_FALSE, /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_TRUE,  /* reinsert drives before I/O */
        FBE_FALSE, /* reinsert drives after I/O */
        FBE_TRUE,  /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             9,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            22,         /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x6,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test12: Read-check test with 1 drive removed (the primary) and sequence number mismatch on another drive. */ 
    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error                                                      Num Objects     Num Records  Err Adj*/
            {0x4,               0,     1,       FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH,  1,              1,           0},
            {FBE_U32_MAX,       0,     0,       0,                                                         0,              0,           0},
        },
        /*  Drives to remove */
            {0, FBE_U32_MAX, 0},
        FBE_TRUE,  /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_TRUE,  /* reinsert drives after I/O */
        FBE_TRUE,  /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             9,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            0,          /* Corr magic cnt */    
            1,          /* Corr seq cnt */  
            0x0,        /* Magic bitmap */  
            0x4,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },

    /* Test13: Read-check test with 1 drive removed (the primary) and magic number mismatch on another drive. */ 
    {   
        {/* LEI params */
            /* Pos bitmap       LBA    Blocks   Error                                                        Num Objects     Num Records  Err Adj*/
            {0x4,               0,     1,       FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH,  1,              1,           0},
            {FBE_U32_MAX,       0,     0,       0,                                                           0,              0,           0},
        },
        /*  Drives to reinsert */
            {0, FBE_U32_MAX, 0},
        FBE_TRUE,  /* remove drives before I/O */
        FBE_FALSE, /* remove drives after I/O */
        FBE_FALSE, /* reinsert drives before I/O */
        FBE_TRUE,  /* reinsert drives after I/O */
        FBE_TRUE,  /* flush data */
        FBE_FALSE, /* run I/O test repeatedly until stopped */
        {
            /* Operation, Pattern, Sequence ID, Block size, Block count, Start LBA, Sequence num, DMC expected, 
                Block status, Block qualifier, Num IOs, Num threads */
             FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
             FBE_DATA_PATTERN_LBA_PASS,
             0x95,    
             520,  
             22,   
             0,           
             9,         
             FBE_FALSE,
             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH,
             1,
             1,
            /* Expected errors */
            FBE_TRUE,   /* Errors */
            0,          /* Dead errs */
            0,          /* Uncorr magic cnt */      
            1,          /* Corr magic cnt */    
            0,          /* Corr seq cnt */  
            0x4,        /* Magic bitmap */  
            0x0,        /* Seq bitmap */
            0,          /* Corr media cnt */    
            0,          /* Uncorr media cnt */      
            0,          /* Corr CRC cnt */  
            0,          /* Uncorr CRC cnt */
        },
    },
};


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static void harry_potter_test_run_basic_tests(void);
static void harry_potter_test_run_lei_tests(void);
static void harry_potter_test_run_drive_removal_tests(void);
static void harry_potter_test_load_physical_config(void);
static void harry_potter_run_test(fbe_api_raw_mirror_service_context_t *context_p,
                                  harry_potter_test_case_t test_case);
static void harry_potter_test_remove_drive(fbe_u32_t position_to_remove);
static void harry_potter_test_reinsert_drive(fbe_api_raw_mirror_service_context_t *context_p,
                                             fbe_u32_t position_to_insert);
static void harry_potter_test_wait_for_raw_mirror_pvd_ready(fbe_api_raw_mirror_service_context_t *context_p,
                                                            fbe_u32_t position_to_insert);
static void harry_potter_test_inject_error_record(
                                fbe_u16_t                                   position_bitmap,
                                fbe_lba_t                                   lba,
                                fbe_block_count_t                           num_blocks,
                                fbe_api_logical_error_injection_type_t      error_type,
                                fbe_u32_t                                   num_objects_enabled,
                                fbe_u32_t                                   num_records,
                                fbe_u16_t                                   err_adj_bitmask);
static void harry_potter_test_wait_for_error_injection_complete(
                                fbe_u16_t position_bitmap,
                                fbe_u16_t err_record_cnt);


/*!**************************************************************
 *  harry_potter_test()
 ****************************************************************
 * @brief
 *  Run a series of I/O tests to a raw mirror.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 10/2011   Created.   Susan Rundbaken
 *
 ****************************************************************/
void harry_potter_test(void)
{
    harry_potter_test_run_basic_tests();

    harry_potter_test_run_lei_tests();

    harry_potter_test_run_drive_removal_tests();

    return;
}
/******************************************
 * end harry_potter_test()
 ******************************************/

/*!**************************************************************
 *  harry_potter_setup()
 ****************************************************************
 * @brief
 *  Setup the Harry Potter test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
void harry_potter_setup(void)
{    
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        harry_potter_test_load_physical_config();
        sep_config_load_sep_and_neit();
    }

    /* The following utility function does some setup for hardware. 
     */
    fbe_test_common_util_test_setup_init();
}
/**************************************
 * end harry_potter_setup()
 **************************************/

/*!**************************************************************
 *  harry_potter_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the Harry Potter test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 10/2011  Created. Susan Rundbaken 
 *
 ****************************************************************/
void harry_potter_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end harry_potter_cleanup()
 ******************************************/

/*!**************************************************************
 *  harry_potter_test_run_basic_tests()
 ****************************************************************
 * @brief
 *  Run a series of basic I/O tests to a raw mirror.
 *  Tests include the following:
 *      - Write with no magic number on raw mirror drives (all drives).
 *	    - Read-check test with no previous magic/sequence number (all drives).
 *      - Write with magic number/sequence number same on all drives.
 *      - Read-check with magic number/sequence number same on all drives.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 11/2011   Created.   Susan Rundbaken
 *
 ****************************************************************/
void harry_potter_test_run_basic_tests(void)
{
    fbe_u32_t iter;

    mut_printf(MUT_LOG_HIGH, "***************");
    mut_printf(MUT_LOG_HIGH, "Run Basic Tests");
    mut_printf(MUT_LOG_HIGH, "***************");

    for (iter = 1; iter <= HARRY_POTTER_MAX_BASIC_TESTS; iter++)
    {
        mut_printf(MUT_LOG_HIGH, "Run Basic test: %d\n",iter);
        harry_potter_run_test(&harry_potter_test_context,
                              harry_potter_basic_tests[iter-1]);
    }

    return;
}
/******************************************
 * end harry_potter_test_run_basic_tests()
 ******************************************/

/*!**************************************************************
 *  harry_potter_test_run_lei_tests()
 ****************************************************************
 * @brief
 *  Run a series of I/O tests to a raw mirror using the Logical Error Injection Service (LEI)
 *  to inject errors on the raw mirror drives.
 *  Tests include the following:
 *	    - Simple write test to make sure raw mirror initialized properly
 *	    - Simple read-check test to confirm raw mirror initialized properly
 *      - Read-check with sequence number mismatch on 1 drive
 *      - Read-check with magic number mismatch on 1 drive
 *      - Read-check with sequence number mismatch on 1 drive; media error on another drive
 *      - Read-check with media number mismatch on 1 drive; media error on another drive
 *      - Write-verify with media error on 1 drive
 *      - Read-check with correctable magic number error on 1 drive; sequence number mismatch on another drive
 *      - Read-check with correctable magic number error on 1 drive; sequence number mismatch on same drive
 *      - Read-check with correctable magic number error on 1 drive; sequence number mismatch on same drive at different LBA
 *      - Read-check with uncorrectable magic number
 *      - Read-check with uncorrectable magic number and CRC error; different LBA
 *      - Read-check with uncorrectable magic number and CRC error; same LBA
 *	    - Read-check with uncorrectable magic number and CRC error at same LBA; correctable magic number error at different LBA
 *      - Read-check with uncorrectable media error
 *      - Read-check with correctable CRC error
 *      - Read-check with correctable magic number error on 1 drive; CRC error on another drive
 *      - Read-check with correctable magic number error on 1 drive; CRC error on same drive and LBA
 *      - Read-check with correctable seq number error on 1 drive; CRC error on same drive and different LBA 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 11/2011   Created.   Susan Rundbaken
 *
 ****************************************************************/
void harry_potter_test_run_lei_tests(void)
{
    fbe_u32_t iter;

    mut_printf(MUT_LOG_HIGH, "***************");
    mut_printf(MUT_LOG_HIGH, "\nRun LEI Tests");
    mut_printf(MUT_LOG_HIGH, "***************");

    for (iter = 1; iter <= HARRY_POTTER_MAX_LEI_TESTS; iter++)
    {
        mut_printf(MUT_LOG_HIGH, "Run LEI test: %d\n",iter);
        harry_potter_run_test(&harry_potter_test_context,
                              harry_potter_test_lei_tests[iter-1]);
    }

    return;
}
/******************************************
 * end harry_potter_test_run_lei_tests()
 ******************************************/

/*!**************************************************************
 *  harry_potter_test_run_drive_removal_tests()
 ****************************************************************
 * @brief
 *  Run a series of I/O tests to a raw mirror with drive(s) removed.
 *  Tests include the following:
 *	    - Simple write test to make sure raw mirror initialized properly
 *	    - Simple read-check test to confirm raw mirror initialized properly
 *      - Drive pull while writes in progress
 *	    - Write test with drive removed.
 *      - Read-check test with drive removed.
 *	    - Write test with drive removed
 *	    - Read-check test with previous drive reinserted
 *	    - Write test with 2 drives removed including primary
 *	    - Read-check test with previous drives reinserted
 *	    - Write test with 2 drives removed not including primary
 *	    - Read-check test with previous drives reinserted
 *	    - Read-check test with 1 drive removed (the primary) and sequence number mismatch on another drive
 *	    - Read-check test with 1 drive removed (the primary) and magic number mismatch on another drive
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 11/2011   Created.   Susan Rundbaken
 *
 ****************************************************************/
void harry_potter_test_run_drive_removal_tests(void)
{
    fbe_u32_t iter;

    mut_printf(MUT_LOG_HIGH, "***************");
    mut_printf(MUT_LOG_HIGH, "Run Drive Removal Tests");
    mut_printf(MUT_LOG_HIGH, "***************");

    for (iter = 1; iter <= HARRY_POTTER_MAX_DRIVE_REMOVAL_TESTS; iter++)
    {
        mut_printf(MUT_LOG_HIGH, "Run drive removal test: %d\n",iter);
        harry_potter_run_test(&harry_potter_test_context,
                              harry_potter_test_drive_removal_tests[iter-1]);
    }

    return;
}
/******************************************
 * end harry_potter_test_run_drive_removal_tests()
 ******************************************/

/*!**************************************************************
 *  harry_potter_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Harry Potter test physical configuration.
 *  This code comes from Larry T Lobster physical setup.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/
static void harry_potter_test_load_physical_config(void)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      encl0_0_handle;
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;


    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                            2, /* portal */
                                            0, /* backend number */ 
                                            &port0_handle);

    /* Insert enclosures to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, HARRY_POTTER_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(HARRY_POTTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == HARRY_POTTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} 
/******************************************
 * end harry_potter_test_load_physical_config()
 ******************************************/

/*!**************************************************************
 *  harry_potter_run_test()
 ****************************************************************
 * @brief
 *  Run the specified test to a raw mirror.
 *
 * @param context_p - context structure to initialize.
 * @param test_case - test to run.
 *
 * @return None.
 *
 * @author
 * 11/2011   Created.   Susan Rundbaken
 *
 ****************************************************************/
static void harry_potter_run_test(fbe_api_raw_mirror_service_context_t *context_p,
                                  harry_potter_test_case_t test_case)
{
    fbe_status_t status;
    fbe_u32_t iter;


    /*  If specified, inject errors.
     */
    if (test_case.lei_params[0].position_bitmap != FBE_U32_MAX)
    {
        /*  Disable error injection records in the error injection table (initializes records)
         */
        status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        /*  Enable the error injection service
         */
        status = fbe_api_logical_error_injection_enable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        /*  Inject the specified errors.
         */
        for (iter = 0; iter < HARRY_POTTER_MAX_LEI; iter++)
        {
            if (test_case.lei_params[iter].position_bitmap == FBE_U32_MAX)
            {
                break;
            }
    
            harry_potter_test_inject_error_record(test_case.lei_params[iter].position_bitmap,
                                                  test_case.lei_params[iter].lba,
                                                  test_case.lei_params[iter].num_blocks,
                                                  test_case.lei_params[iter].error_type,
                                                  test_case.lei_params[iter].num_objects_enabled,
                                                  test_case.lei_params[iter].num_records,
                                                  test_case.lei_params[iter].err_adj_bitmask);
        }
    }

    /* Initialize context for issuing I/O to the raw mirror for the specified test case.
     */
    status = fbe_api_raw_mirror_service_test_context_init(&harry_potter_test_context,
                             test_case.test_record.raw_mirror_operation,
                             test_case.test_record.pattern,
                             test_case.test_record.sequence_id,
                             test_case.test_record.block_size,
                             test_case.test_record.block_count,
                             test_case.test_record.start_lba,
                             test_case.test_record.sequence_num,
                             test_case.test_record.dmc_expected_b,
                             test_case.test_record.max_passes,
                             test_case.test_record.num_threads);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If specified, remove drives.
     */
    if (test_case.remove_drives_before_io_b)
    {
        for (iter = 0; iter < HARRY_POTTER_MAX_DRIVE_REMOVE; iter++)
        {
            if (test_case.drives_to_remove[iter] == FBE_U32_MAX)
            {
                break;
            }

            harry_potter_test_remove_drive(test_case.drives_to_remove[iter]);
        }
    }

    /* If specified, reinsert drives.
     */
    if (test_case.reinsert_drives_before_io_b)
    {
        for (iter = 0; iter < HARRY_POTTER_MAX_DRIVE_REMOVE; iter++)
        {
            if (test_case.drives_to_remove[iter] == FBE_U32_MAX)
            {
                break;
            }

            harry_potter_test_reinsert_drive(context_p, test_case.drives_to_remove[iter]);
        }
    }

    /* Run the I/O test.
     */
    status = fbe_api_raw_mirror_service_run_test(context_p, FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    /* If I/O is running repeatedly, let it run for a few seconds before proceeding.
     */
    if (test_case.test_record.max_passes == 0)
    {
        fbe_api_sleep(5000);
    }

    /* If specified, wait for error injections to complete.
     */
    if (test_case.lei_params[0].position_bitmap != FBE_U32_MAX)
    {
        for (iter = 0; iter < HARRY_POTTER_MAX_LEI; iter++)
        {
            if (test_case.lei_params[iter].position_bitmap == FBE_U32_MAX)
            {
                break;
            }
    
            harry_potter_test_wait_for_error_injection_complete(test_case.lei_params[iter].position_bitmap,
                                                                test_case.lei_params[iter].num_records);
        }
    
        /*  Disable error injection on raw mirror.
         */
        status = fbe_api_logical_error_injection_disable_object(LEI_RAW_MIRROR_OBJECT_ID, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        /*  Disable the error injection service.
         */
        status = fbe_api_logical_error_injection_disable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* If specified, remove drives now.
     */
    if (test_case.remove_drives_after_io_b)
    {
        for (iter = 0; iter < HARRY_POTTER_MAX_DRIVE_REMOVE; iter++)
        {
            if (test_case.drives_to_remove[iter] == FBE_U32_MAX)
            {
                break;
            }

            harry_potter_test_remove_drive(test_case.drives_to_remove[iter]);
        }
    }

    /* If I/O is running repeatedly, stop I/O now. 
     */
    if (test_case.test_record.max_passes == 0)
    {
        /* Let the I/O run for a few more seconds.
         */
        fbe_api_sleep(5000);

        /* Stop the test.
         */
        mut_printf(MUT_LOG_HIGH, "%s: stop test", __FUNCTION__);
        status = fbe_api_raw_mirror_service_stop_test(context_p, FBE_PACKAGE_ID_NEIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Check status of test
     */
    mut_printf(MUT_LOG_HIGH, "Check results");
    if (test_case.test_record.errors_expected)
    {
        MUT_ASSERT_INT_NOT_EQUAL(harry_potter_test_context.start_io.err_count, 0);
    }
    if (test_case.test_record.dead_err_count == FBE_U32_MAX)
    {
        /* Note that if the dead_err_count is set to "max", that means the number of dead errors can vary.
         * This is related primarily to I/O that runs repeatedly during which dead errors can occur.
         */
        MUT_ASSERT_INT_NOT_EQUAL(harry_potter_test_context.start_io.dead_err_count, 0);
    }
    MUT_ASSERT_INT_EQUAL(harry_potter_test_context.start_io.verify_errors.verify_errors_counts.u_rm_magic_count,
                         test_case.test_record.u_rm_magic_count);
    MUT_ASSERT_INT_EQUAL(harry_potter_test_context.start_io.verify_errors.verify_errors_counts.c_rm_magic_count,
                         test_case.test_record.c_rm_magic_count);
    MUT_ASSERT_INT_EQUAL(harry_potter_test_context.start_io.verify_errors.verify_errors_counts.c_rm_seq_count,
                         test_case.test_record.c_rm_seq_count);
    MUT_ASSERT_INT_EQUAL(harry_potter_test_context.start_io.verify_errors.raw_mirror_magic_bitmap,
                         test_case.test_record.rm_magic_bitmap);
    MUT_ASSERT_INT_EQUAL(harry_potter_test_context.start_io.verify_errors.raw_mirror_seq_bitmap,
                         test_case.test_record.rm_seq_bitmap);
    MUT_ASSERT_INT_EQUAL(harry_potter_test_context.start_io.verify_errors.verify_errors_counts.c_media_count,
                         test_case.test_record.c_media_count);
    MUT_ASSERT_INT_EQUAL(harry_potter_test_context.start_io.verify_errors.verify_errors_counts.u_media_count,
                         test_case.test_record.u_media_count);
    MUT_ASSERT_INT_EQUAL(harry_potter_test_context.start_io.verify_errors.verify_errors_counts.c_crc_count,
                         test_case.test_record.c_crc_count);
    MUT_ASSERT_INT_EQUAL(harry_potter_test_context.start_io.verify_errors.verify_errors_counts.u_crc_count,
                         test_case.test_record.u_crc_count);

    if (test_case.test_record.expected_block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(test_case.test_record.expected_block_status, context_p->start_io.block_status);
        MUT_ASSERT_INT_EQUAL(test_case.test_record.expected_block_qualifier, context_p->start_io.block_qualifier);
    }

    /* If specified, reinsert drives now.
     */
    if (test_case.reinsert_drives_after_io_b)
    {
        for (iter = 0; iter < HARRY_POTTER_MAX_DRIVE_REMOVE; iter++)
        {
            if (test_case.drives_to_remove[iter] == FBE_U32_MAX)
            {
                break;
            }

            harry_potter_test_reinsert_drive(context_p, test_case.drives_to_remove[iter]);
        }
    }

    /* If specified, flush data.
     */
    if (test_case.flush_data_b)
    {
        /* Destroy the test context.
         */
        status = fbe_api_raw_mirror_service_test_context_destroy(context_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Initialize new context for the flush.
         */
        status = fbe_api_raw_mirror_service_test_context_init(&harry_potter_test_context,
                                 FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_VERIFY,
                                 test_case.test_record.pattern,
                                 test_case.test_record.sequence_id,
                                 test_case.test_record.block_size,
                                 test_case.test_record.block_count,
                                 test_case.test_record.start_lba,
                                 test_case.test_record.sequence_num,
                                 FBE_FALSE, /* DMC should not be detected */
                                 1 /* max I/Os to issue */,
                                 1 /* num threads to use */);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Send the flush to the raw mirror.
         */
        mut_printf(MUT_LOG_HIGH, "%s: flush data", __FUNCTION__);
        status = fbe_api_raw_mirror_service_run_test(context_p, FBE_PACKAGE_ID_NEIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, context_p->start_io.block_status);
        MUT_ASSERT_INT_EQUAL(FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, context_p->start_io.block_qualifier);

    
        /* Destroy the test context.
         */
        status = fbe_api_raw_mirror_service_test_context_destroy(context_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


        /* Initialize new context doing a read-check to confirm the flush.
         */
        status = fbe_api_raw_mirror_service_test_context_init(&harry_potter_test_context,
                                 FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK,
                                 test_case.test_record.pattern,
                                 test_case.test_record.sequence_id,
                                 test_case.test_record.block_size,
                                 test_case.test_record.block_count,
                                 test_case.test_record.start_lba,
                                 test_case.test_record.sequence_num,
                                 FBE_FALSE, /* DMC should not be detected */
                                 1 /* max I/Os to issue */,
                                 1 /* num threads to use */);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Run a read-check test confirming the data just written.
         */
        mut_printf(MUT_LOG_HIGH, "%s: data flushed", __FUNCTION__);
        status = fbe_api_raw_mirror_service_run_test(context_p, FBE_PACKAGE_ID_NEIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, context_p->start_io.block_status);
        MUT_ASSERT_INT_EQUAL(FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, context_p->start_io.block_qualifier);
    
        /* Confirm the verify counts.
         */
        mut_printf(MUT_LOG_HIGH, "%s: after flush: check results", __FUNCTION__);
        MUT_ASSERT_INT_EQUAL(harry_potter_test_context.start_io.err_count, 0);
    }

    /* Destroy the context.
     */
    status = fbe_api_raw_mirror_service_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (test_case.reinsert_drives_after_io_b ||
        test_case.reinsert_drives_before_io_b)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: wait for rebuilds", __FUNCTION__);
        status = fbe_test_sep_util_wait_for_degraded_bitmask(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG, 
                                                             0x0, /* not degraded */ FBE_TEST_WAIT_TIMEOUT_MS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: wait for rebuilds...Complete.", __FUNCTION__);
    }
    return;
}
/******************************************
 * end harry_potter_run_test()
 ******************************************/

/*!**************************************************************
 *          harry_potter_test_remove_drive()
 ****************************************************************
 *
 * @brief   Remove the specified drive from the raw mirror.
 *
 * @param   position_to_remove - The position in the raw mirror to remove.               
 *
 * @return  None.
 *
 * @author
 *  11/2011  Susan Rundbaken  - Created
 *
 ****************************************************************/
static void harry_potter_test_remove_drive(fbe_u32_t position_to_remove)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_object_id_t         pvd_object_id;


    mut_printf(MUT_LOG_HIGH, "%s: remove system drive in slot %d", __FUNCTION__, position_to_remove);

    /* See if in simulation environment */
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(HARRY_POTTER_TEST_RAW_MIRROR_BUS,      
                                             HARRY_POTTER_TEST_RAW_MIRROR_ENCLOSURE,
                                             position_to_remove, /* slot */
                                             &harry_potter_local_drive_handle[position_to_remove]);
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            status = fbe_test_pp_util_pull_drive(HARRY_POTTER_TEST_RAW_MIRROR_BUS,      
                                                 HARRY_POTTER_TEST_RAW_MIRROR_ENCLOSURE,
                                                 position_to_remove /* slot */,
                                                 &harry_potter_peer_drive_handle[position_to_remove]);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }
    }
    else{
    
        /* Not in simulation */
        status = fbe_test_pp_util_pull_drive_hw(HARRY_POTTER_TEST_RAW_MIRROR_BUS, 
                                                HARRY_POTTER_TEST_RAW_MIRROR_ENCLOSURE, 
                                                position_to_remove /* slot */);
    }

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure system drive is in the failure state.
     */

    /* !@TODO: should be able to use fbe_api_provision_drive_get_obj_id_by_location() but this utility function
     *          is failing intermittedly
     */
    pvd_object_id = position_to_remove + 1;
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          pvd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                          HARRY_POTTER_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_HIGH, "%s: drive in slot %d successfully removed", __FUNCTION__, position_to_remove);

    return;
}
/******************************************
 * end harry_potter_test_remove_drive()
 ******************************************/

/*!***************************************************************************
 *          harry_potter_test_reinsert_drive()
 *****************************************************************************
 *
 * @brief   Reinsert the specified raw mirror drive.
 *
 * @param   context_p - context structure to use for test.
 * @param   position_to_insert - The position in the raw mirror to insert.               
 *
 * @return None.
 *
 * @author
 *  11/2011  Susan Rundbaken  - Created
 *
 *****************************************************************************/

static void harry_potter_test_reinsert_drive(fbe_api_raw_mirror_service_context_t *context_p,
                                             fbe_u32_t position_to_insert)
{
    fbe_status_t            status = FBE_STATUS_OK;


    mut_printf(MUT_LOG_HIGH, "%s: reinsert system drive in slot %d", __FUNCTION__, position_to_insert);

    /* See if in simulation environment */
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(HARRY_POTTER_TEST_RAW_MIRROR_BUS,          
                                                 HARRY_POTTER_TEST_RAW_MIRROR_ENCLOSURE,  
                                                 position_to_insert, /* slot */       
                                                 harry_potter_local_drive_handle[position_to_insert]);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            status = fbe_test_pp_util_reinsert_drive(HARRY_POTTER_TEST_RAW_MIRROR_BUS,          
                                                     HARRY_POTTER_TEST_RAW_MIRROR_ENCLOSURE,  
                                                     position_to_insert, /* slot */       
                                                     harry_potter_peer_drive_handle[position_to_insert]);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }

    }
    else
    {
        /* Not in simulation */
        status = fbe_test_pp_util_reinsert_drive_hw(HARRY_POTTER_TEST_RAW_MIRROR_BUS, 
                                                    HARRY_POTTER_TEST_RAW_MIRROR_ENCLOSURE, 
                                                    position_to_insert /* slot */);
    }

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for PVD to be ready.
     */
    harry_potter_test_wait_for_raw_mirror_pvd_ready(context_p, position_to_insert);

    mut_printf(MUT_LOG_HIGH, "%s: drive in slot %d successfully reinserted", __FUNCTION__, position_to_insert);

    return;
}
/******************************************
 * end harry_potter_test_reinsert_drive()
 ******************************************/

/*!***************************************************************************
 *          harry_potter_test_wait_for_raw_mirror_pvd_ready()
 *****************************************************************************
 *
 * @brief  Wait for the given PVD object and edge to the raw mirror to be ready. 
 *
 * @param   context_p - context structure to use for test.
 * @param   position_to_insert - the PVD position in raw mirror.               
 *
 * @return None.
 *
 * @author
 *  11/2011  Susan Rundbaken  - Created
 *
 *****************************************************************************/

static void harry_potter_test_wait_for_raw_mirror_pvd_ready(fbe_api_raw_mirror_service_context_t *context_p,
                                                            fbe_u32_t position_to_insert)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_object_id_t         pvd_object_id;

    mut_printf(MUT_LOG_HIGH, "%s: wait for drive in slot %d to be ready", __FUNCTION__, position_to_insert);

    /* Make sure PVD object is ready.
     */
    /* !@TODO: should be able to use fbe_api_provision_drive_get_obj_id_by_location() but this utility function
     *          is failing intermittedly
     */
    pvd_object_id = position_to_insert + 1;
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          pvd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                          HARRY_POTTER_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_HIGH, "%s: wait for drive in slot %d edge to raw mirror to be ready", __FUNCTION__, position_to_insert);

    /* Make sure PVD edge to raw mirror is enabled.
     */
    context_p->start_io.pvd_edge_index = position_to_insert;
    context_p->start_io.timeout_ms = HARRY_POTTER_WAIT_MSEC;
    context_p->start_io.pvd_edge_enabled_b = FBE_FALSE;
    status = fbe_api_raw_mirror_service_wait_pvd_edge_enabled(context_p, FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/******************************************
 * end harry_potter_test_wait_for_raw_mirror_pvd_ready()
 ******************************************/

/*!**************************************************************
 * harry_potter_test_inject_error_record()
 ****************************************************************
 * @brief
 *  Injects an error record
 * 
 * @param position_bitmap   - disk position(s) in Raw Mirror to inject error
 * @param lba               - LBA on disk to inject error
 * @param num_blocks        - number of blocks to extend error
 * @param error_type        - type of error
 * @param num_objects_enabled - Number of objects enabled so far
 * @param num_records         - Number of LEI error records so far
 * @param err_adj_bitmask     - Bitmask of positions with adjacent errors
 *
 * @return None.
 *
 ****************************************************************/
static void harry_potter_test_inject_error_record(
                                    fbe_u16_t                               position_bitmap,
                                    fbe_lba_t                               lba,
                                    fbe_block_count_t                       num_blocks,
                                    fbe_api_logical_error_injection_type_t  error_type,
                                    fbe_u32_t                               num_objects_enabled,
                                    fbe_u32_t                               num_records,
                                    fbe_u16_t                               err_adj_bitmask)
{
    fbe_status_t                                status;
    fbe_api_logical_error_injection_record_t    error_record = {0};
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_u32_t                                   position;
    fbe_object_id_t                             pvd_object_id;

    
    /*  Enable error injection on the raw mirror.
     *  This is a dummy object for the LEI since the raw mirror does not have an object interface.
     */
    status = fbe_api_logical_error_injection_enable_object(LEI_RAW_MIRROR_OBJECT_ID, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Setup error record for error injection.
     *  Inject error on given disk position and LBA.
     */
    error_record.pos_bitmap = position_bitmap;                      /*  Injecting error on given disk position */
    error_record.width      = HARRY_POTTER_TEST_RAW_MIRROR_WIDTH;   /*  raw mirror width */
    error_record.lba        = lba;                              /*  Physical address to begin errors */
    error_record.blocks     = num_blocks;                       /*  Blocks to extend errors */
    error_record.err_type   = error_type;                       /* Error type */
    error_record.err_mode   = FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS;  /* Error mode */
    error_record.err_count  = 0;                                /*  Errors injected so far */
    error_record.err_limit  = HARRY_POTTER_TEST_RAW_MIRROR_WIDTH;   /*  Limit of errors to inject */
    error_record.skip_count = 0;                                /*  Limit of errors to skip */
    error_record.skip_limit = 0;                                /*  Limit of errors to inject */
    error_record.err_adj    = err_adj_bitmask;                  /*  Error adjacency bitmask */
    error_record.start_bit  = 0;                                /*  Starting bit of an error */
    error_record.num_bits   = 0;                                /*  Number of bits of an error */
    error_record.bit_adj    = 0;                                /*  Indicates if erroneous bits adjacent */
    error_record.crc_det    = 0;                                /*  Indicates if CRC detectable */

    /*  Create error injection record 
     */
    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Get the object ID for the raw mirror PVD corresponding to the incoming position(s). 
     */
    for (position = 0; position < HARRY_POTTER_TEST_RAW_MIRROR_WIDTH; position++)
    {
        if ((1 << position) & position_bitmap)
        {
            /* Enable error injection on the PVD at this position.
             */
            status = fbe_api_provision_drive_get_obj_id_by_location(0, /* bus */
                                                                    0, /* enclosure */
                                                                    position, /* slot */
                                                                    &pvd_object_id);

            mut_printf(MUT_LOG_TEST_STATUS,
		       "== %s Enable error injection: pos %d, lba %llu ==",
		        __FUNCTION__, position, (unsigned long long)lba);
            status = fbe_api_logical_error_injection_enable_object(pvd_object_id, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    /*  Check error injection stats
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, num_records);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, num_objects_enabled + 1);  /* add 1 for the raw mirror object */

    return;
}
/******************************************
 * end harry_potter_test_inject_error_record()
 ******************************************/

/*!**************************************************************
 * harry_potter_test_wait_for_error_injection_complete()
 ****************************************************************
 *
 * @brief   Wait for the error injection to complete
 * 
 * @param   position_bitmap  - disk position(s) in Raw Mirror to wait for error
 * @param   err_record_cnt   - number of errors injected
 *
 * @return  None
 *
 ****************************************************************/
static void harry_potter_test_wait_for_error_injection_complete(
                                fbe_u16_t position_bitmap,
                                fbe_u16_t err_record_cnt)
{
    fbe_status_t                                        status;
    fbe_u32_t                                           sleep_count;
    fbe_u32_t                                           sleep_period_ms;
    fbe_u32_t                                           max_sleep_count;
    fbe_api_logical_error_injection_get_stats_t         stats; 
    fbe_u32_t                                           position;
    fbe_object_id_t                                     pvd_object_id;


    /*  Intialize local variables.
     */
    sleep_period_ms = 100;
    max_sleep_count = HARRY_POTTER_WAIT_MSEC / sleep_period_ms;

    /*  Wait for errors to be injected on the raw mirror's PVD objects.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all errors to be injected(detected) ==\n", __FUNCTION__);

    for (sleep_count = 0; sleep_count < max_sleep_count; sleep_count++)
    {
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if(stats.num_errors_injected >= err_record_cnt)
        {
            /* Break out of the loop.
             */
            mut_printf(MUT_LOG_TEST_STATUS,
		       "== %s found %llu errors injected ==\n",
		       __FUNCTION__,
		       (unsigned long long)stats.num_errors_injected);
            break;
        }
                        
        fbe_api_sleep(sleep_period_ms);
    }

    /*  The error injection took too long and timed out.
     */
    if (sleep_count >= max_sleep_count)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection failed ==\n", __FUNCTION__);
        MUT_ASSERT_TRUE(sleep_count < max_sleep_count);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection successful ==\n", __FUNCTION__);

    /*  Disable error injection on the PVD(s).
     */
    for (position = 0; position < HARRY_POTTER_TEST_RAW_MIRROR_WIDTH; position++)
    {
        if ((1 << position) & position_bitmap)
        {
            /* Disable error injection on the PVD at this position.
             */
            status = fbe_api_provision_drive_get_obj_id_by_location(0, /* bus */
                                                                    0, /* enclosure */
                                                                    position, /* slot */
                                                                    &pvd_object_id);

            mut_printf(MUT_LOG_TEST_STATUS, "== %s Disable error injection: pos %d ==", __FUNCTION__, position);
            status = fbe_api_logical_error_injection_disable_object(pvd_object_id, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    return;

} 
/******************************************
 * end harry_potter_test_wait_for_error_injection_complete()
 ******************************************/


/***************************
 * end file harry_potter_test.c
 ***************************/

