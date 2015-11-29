/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file banio_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests cases for raid6 incomplete write 
 *   the test cases in this file should cover all the code path in function fbe_xor_rbld_data_stamps_r6
 *
 * @version
 *   20/11/2013 - Created. Geng, Han
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_logical_drive_interface.h"
#include "fbe/fbe_api_bvd_interface.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe_test_esp_utils.h"
#include "sep_rebuild_utils.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe_test.h"
#include "fbe/fbe_api_sector_trace_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_raid_error.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* banio_short_desc = "this test tries to evaluate all the typical incomplete write cases on R6";
char* banio_long_desc =
"\n\
this test tries to evaluate all the typical incomplete write cases on R6\n\
for each incomplete write, we would check the blks on fatal data drives.\n\
It might have new data, old data or invalidated data.\n\
we would also write new data to the fatal data drives and then check the proper event logs\n\
the test cases in this file should cover all the code path in function fbe_xor_rbld_data_stamps_r6 \n\
Description last updated: 11/21/2013.\n";
//------------------------------------------------------------------------------

#define BANIO_R6_IW_MAX_ERROR_CASES_TABLES 5

// luns per rg for the extended test. 
#define BANIO_LUNS_PER_RAID_GROUP 1

// Number of chunks each LUN will occupy.
#define BANIO_CHUNKS_PER_LUN 3

#define BANIO_WAIT_FOR_VERIFY 30000
#define BANIO_WAIT_MSEC 50000

#define BANIO_STRIPE_ELEMENT_SIZE 128
//------------------------------------------------------------------------------

fbe_u32_t banio_library_debug_flags = (FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING      | 
                                       FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                       FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING|
                                       FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING |
                                       FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING |
                                       FBE_RAID_LIBRARY_DEBUG_FLAG_VERIFY_TRACING |
                                       FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING);
fbe_u32_t banio_object_debug_flags = (FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING |
                                      FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING |
                                      FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING |
                                      FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING);


typedef enum fbe_banio_iw_orig_stripe_state_e
{
    ORIG_STATE_MR3 = 0, 
    ORIG_STATE_468
}fbe_banio_iw_orig_stripe_state_t;

typedef enum fbe_banio_fatal_blk_data_e
{
    BANIO_FATAL_BLK_DATA_INVALID = 0,
    BANIO_FATAL_BLK_DATA_OLD,
    BANIO_FATAL_BLK_DATA_NEW,
    BANIO_FATAL_BLK_DATA_LOST,
    BANIO_FATAL_BLK_DATA_LAST,
} fbe_banio_fatal_blk_data_t;


typedef struct fbe_banio_r6_iw_error_case_s
{
    fbe_u8_t *description_p;            /*!< Description of test case. */
    fbe_u32_t pass_count;
    fbe_u32_t rg_width;
    fbe_banio_iw_orig_stripe_state_t orig_state;
    fbe_u16_t orig_468_ws_bitmap; // each bit for one pos
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_u16_t iw_bitmap;
    fbe_u16_t fatal_bitmap;
    fbe_u32_t expected_io_error_count;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_banio_fatal_blk_data_t expected_fatal_blk_data;
    fbe_test_event_log_test_result_t event_log_result;
}fbe_banio_r6_iw_case_t;




fbe_banio_r6_iw_case_t banio_iw_r6_test_cases_single_fault[] = 
{
    // AR608994 -- old code could not cover this case 
    {
        "original stripe: 468\n\
         action: issue RCW strip write and got incomplete write on single parity disk.\n\
         expectation: the sector on the fatal drive could be recovered successfully\n", 
        __LINE__,                       
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0x54, //0b0101 0100             // orig_468_ws_bitmap
        0,                              // lba
        4 * BANIO_STRIPE_ELEMENT_SIZE,  // blocks
        0x02, //0b0000 0010,              // iw_bitmap
        0x04, //0b0000 0100,              // fatal_bitmap
        0,                                // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, // block_qualifier
        BANIO_FATAL_BLK_DATA_OLD,         // expected_fatal_blk_data;
        {
            // time:15:40:26  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0x2f
            // time:15:40:26  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x2f
            0x1, 
            {
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    
    // AR608994 -- old code could not cover this case 
    {
        "original stripe: 468\n\
         action: issue 468 write to the first two stripe elements and only one parity disk updated successfully.\n\
         expectation: the sector on the fatal drive could be recovered successfully\n", 
        __LINE__,
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0x54, //0b0101 0100             // orig_468_ws_bitmap
        0,                              // lba
        2 * BANIO_STRIPE_ELEMENT_SIZE,  // blocks
        0xC1, //0b1100 0001,            // iw_bitmap
        0x10, //0b0001 0000,            // fatal_bitmap
        0,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, // block_qualifier
        BANIO_FATAL_BLK_DATA_OLD,       // expected_fatal_blk_data;
        {
            // time:15:39:40  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x2f
            0x1, 
            {
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

	 // AR608994 -- old code could not cover this case 
    {
        "original stripe: 468\n\
         action: issue RCW write to the first three stripe elements and only one parity disk updated successfully.\n\
         expectation: the sector on the fatal drive could be recovered successfully\n", 
        __LINE__,
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0x54, //0b0101 0100             // orig_468_ws_bitmap
        0x100,                          // lba
        3 * BANIO_STRIPE_ELEMENT_SIZE,  // blocks
        0x39, //0b0011 1001,            // iw_bitmap
        0x04, //0b0000 0100,            // fatal_bitmap
        0,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, // block_qualifier
        BANIO_FATAL_BLK_DATA_OLD,       // expected_fatal_blk_data;
        {
            // time:15:39:40  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x2f
            0x1, 
            {
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    
    // AR608994 -- old code could not cover this case 
    {
        "original stripe: 468\n\
         action: issue MR3 full strip write and got incomplete write on single parity disk.\n\
         expectation: the sector on the fatal drive could be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0x54, //0b0101 0100             // orig_468_ws_bitmap
        0,                              // lba
        6 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x01, //0b0000 0001,              // iw_bitmap
        0x04, //0b0000 0100,              // fatal_bitmap
        0,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, // block_qualifier
        BANIO_FATAL_BLK_DATA_NEW,       // expected_fatal_blk_data;
        {
            // time:15:40:26  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0x2f
            // time:15:40:26  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x2f
            0x2, 
            {
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

	// AR608994 -- old code could not cover this case 
    {
        "original stripe: 468\n\
         action: issue RCW strip write and got incomplete write on single parity disk.\n\
         expectation: the sector on the fatal drive could be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0x54, //0b0101 0100             // orig_468_ws_bitmap
        0,                              // lba
        4 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x01, //0b0000 0001,              // iw_bitmap
        0x04, //0b0000 0100,              // fatal_bitmap
        0,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, // block_qualifier
        BANIO_FATAL_BLK_DATA_OLD,       // expected_fatal_blk_data;
        {
            // time:15:40:26  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0x2f
            // time:15:40:26  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x2f
            0x1, 
            {
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    
    // AR608994 -- old code could not cover this case 
    {
        "original stripe: MR3\n\
         action: issue 468 write to the first three stripe element and got incomplete write on single parity disk.\n\
         expectation: the sector on the fatal drive could be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_MR3,                 // orig_state
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        0,                              // lba
        2 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x01, //0b0000 0001,              // iw_bitmap
        0x10, //0b0001 0000,              // fatal_bitmap
        0,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, // block_qualifier
        BANIO_FATAL_BLK_DATA_OLD,       // expected_fatal_blk_data;
        {
            // time:08:23:58  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0x2f
            // time:08:23:58  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x2f
            0x2, 
            {
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

	// AR608994 -- old code could not cover this case 
    {
        "original stripe: MR3\n\
         action: issue RCW write to the first three stripe element and got incomplete write on single parity disk.\n\
         expectation: the sector on the fatal drive could be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_MR3,                 // orig_state
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        0,                              // lba
        3 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x01, //0b0000 0001,              // iw_bitmap
        0x10, //0b0001 0000,              // fatal_bitmap
        0,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, // block_qualifier
        BANIO_FATAL_BLK_DATA_OLD,       // expected_fatal_blk_data;
        {
            // time:08:23:58  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0x2f
            // time:08:23:58  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x2f
            0x2, 
            {
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

    {
        "original stripe: MR3\n\
         action: issue 468 write to the first stripe element on one fatal data disk and got incomplete write on single parity disk.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_MR3,                 // orig_state
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        0,                              // lba
        1 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x01, //0b0000 0001,              // iw_bitmap
        0x80, //0b1000 0000,              // fatal_bitmap
        1,                                // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:15:28:56  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:15:28:56  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:15:28:56  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:15:28:56  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:15:28:56  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:15:28:56  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            0x6, 
            {
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

    {
        "original stripe: 468\n\
         action: issue 468 write to the first stripe element on one fatal data disk and got incomplete write on single parity disk.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0x54, //0b0101 0100             // orig_468_ws_bitmap
        0,                              // lba
        1 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x01, //0b0000 0001,              // iw_bitmap
        0x80, //0b1000 0000,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:08:09:18  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:08:09:18  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:08:09:18  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:08:09:18  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            0x4, 
            {
                // Parity Sector Reconstructed RG: %2 Pos: %3 LBA: %4 Blocks: %5 Err info: %6 Extra info: %7
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    {
        "original stripe: MR3\n\
         action: issue MR3 write to the first stripe and got incomplete write on single data disk.\n\
         expectation: the sector on the fatal drive could be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_MR3,                 // orig_state
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        0,                              // lba
        6 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x80, //0b1000 0000,              // iw_bitmap
        0x40, //0b0100 0000,              // fatal_bitmap
        0,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, // block_qualifier
        BANIO_FATAL_BLK_DATA_NEW,       // expected_fatal_blk_data;
        {
            // time:08:29:25  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x7 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0x2f
            0x1, 
            {
                { 
                    0x7,                                            /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

    {
        "original stripe: MR3\n\
         action: issue MR3 write to the first stripe and got incomplete write on double data disks.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_MR3,                 // orig_state
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        0,                              // lba
        6 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0xC0, //0b1100 0000,              // iw_bitmap
        0x10, //0b0001 0000,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:08:36:18  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x4 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:08:36:18  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x4 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:08:36:18  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0x22f
            // time:08:36:18  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x7 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0x22f
            // time:08:36:18  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0x22f
            // time:08:36:18  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0x22f
            0x6, 
            {
                { 
                    0x4, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x4, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,            /* expected error code */
                    (VR_TS),                                            /* expected error info */
                    0x7f,                                               /* expected lba */
                    0x1                                                 /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,            /* expected error code */
                    (VR_TS),                                            /* expected error info */
                    0x7f,                                               /* expected lba */
                    0x1                                                 /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

    {
        "original stripe: 468\n\
         action: issue RCW write to the first stripe and got incomplete write both on data disks and one parity disk.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        16,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0x54, //0b0101 0100             // orig_468_ws_bitmap
        0x400,                              // lba
        3 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0xC1, //0b1100 0001,              // iw_bitmap
        0x40, //0b0100 0000,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:09:12:13  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:09:12:13  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:09:12:13  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x5 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:09:12:13  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x6 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:09:12:13  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x6 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:09:12:13  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x7 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            0x6, 
            {
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x5, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

    {
        "original stripe: 468\n\
         action: issue RCW write to the first stripe and got incomplete write both on data disks and one parity disk.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0x54, //0b0101 0100             // orig_468_ws_bitmap
        0,                              // lba
        3 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0xC1, //0b1100 0001,              // iw_bitmap
        0x40, //0b0100 0000,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:09:12:13  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:09:12:13  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:09:12:13  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x5 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:09:12:13  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x6 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:09:12:13  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x6 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:09:12:13  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x7 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            0x6, 
            {
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x5, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

    {
        "original stripe: 468\n\
         action: issue 468 write to the stripe and got incomplete write on both parity disks.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        16,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0x54, //0b0101 0100             // orig_468_ws_bitmap
        0x400,                              // lba
        3 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x03, //0b0000 0011,              // iw_bitmap
        0x80, //0b1000 0000,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:09:12:35  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x5 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:09:12:35  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:09:12:35  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:09:12:35  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:09:12:35  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa
            // time:09:12:35  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa

            0x6, 
            {
                { 
                    0x5, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

    {
        "original stripe: 468\n\
         action: issue RCW write to the stripe and got incomplete write on both parity disks.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0x54, //0b0101 0100             // orig_468_ws_bitmap
        0,                              // lba
        3 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x03, //0b0000 0011,              // iw_bitmap
        0x80, //0b1000 0000,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:09:12:35  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x5 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:09:12:35  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:09:12:35  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:09:12:35  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:09:12:35  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa
            // time:09:12:35  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa

            0x6, 
            {
                { 
                    0x5, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    
    {
        "original stripe: MR3\n\
         action: issue 468 write to the first stripe and only one data disk updated successfully\n\
         expectation: the sector on the fatal drive could be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_MR3,                 // orig_state
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        0,                              // lba
        2 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x83, //0b1000 0011,              // iw_bitmap
        0x80, //0b1000 0000,              // fatal_bitmap
        0,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, // block_qualifier
        BANIO_FATAL_BLK_DATA_OLD,       // expected_fatal_blk_data;
        {
            //  time:09:12:56  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0x2f
            //  time:09:12:56  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x2f
            0x2, 
            {
                { 
                    0x6, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

    {
        "original stripe: MR3\n\
         action: issue RCW write to the first stripe and only one data disk updated successfully\n\
         expectation: the sector on the fatal drive could be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_MR3,                 // orig_state
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        0,                              // lba
        3 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0xC3, //0b1100 0011,              // iw_bitmap
        0x80, //0b1000 0000,              // fatal_bitmap
        0,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, // block_qualifier
        BANIO_FATAL_BLK_DATA_OLD,       // expected_fatal_blk_data;
        {
            //  time:09:12:56  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0x2f
            //  time:09:12:56  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x2f
            0x2, 
            {
                { 
                    0x5, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x5, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },


    {
        "original stripe: MR3\n\
         action: issue MR3 write to the first stripe and got incomplete write on single data disk.\n\
         expectation: the sector on the fatal drive could be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_MR3,                 // orig_state
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        0,                              // lba
        6 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x82, //0b1000 0010,              // iw_bitmap
        0x40, //0b0100 0000,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:15:56:47  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:15:56:47  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:15:56:47  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x2 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:15:56:47  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x3 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:15:56:47  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x4 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:15:56:47  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x5 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:15:56:47  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x6 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:15:56:47  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x6 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:15:56:47  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x7 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            0x9, 
            {
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_TS),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_TS),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x4, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x5, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    
    /* terminator */
    {
        0,0,0,0,0,0,0,
    }
};

fbe_banio_r6_iw_case_t banio_iw_r6_test_cases_double_fault[] = 
{
    {
        "original stripe: 468\n\
         action: issue MR3 write to the first stripe and got incomplete write on single parity disk.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0x54, //0b0101 0100             // orig_468_ws_bitmap
        0,                              // lba
        6 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x01, //0b0000 0001,              // iw_bitmap
        0xC0, //0b1100 0000,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:10:02:37  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:10:02:37  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:10:02:37  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:10:02:37  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x2 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:10:02:37  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x4 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:10:02:37  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x6 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:02:37  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x6 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:02:37  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:02:37  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa

            0x9, 
            {
                // Parity Sector Reconstructed RG: %2 Pos: %3 LBA: %4 Blocks: %5 Err info: %6 Extra info: %7
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x4, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

    {
        "original stripe: MR3\n\
         action: issue 468 write to the first stripe and got incomplete write data disks.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_MR3,                 // orig_state
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        0,                              // lba
        2 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0xC0, //0b1100 0000,              // iw_bitmap
        0x0C, //0b0000 1100,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:10:03:04  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:03:04  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:03:04  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:03:04  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa

            // time:10:03:04  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:10:03:04  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x7 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:10:03:04  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa
            // time:10:03:04  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa
            
            0x8, 
            {
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

    {
        "original stripe: MR3\n\
         action: issue RCW write to the first stripe and got incomplete write data disks.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_MR3,                 // orig_state
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        0,                              // lba
        3 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0xC0, //0b1100 0000,              // iw_bitmap
        0x0C, //0b0000 1100,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:10:03:04  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:03:04  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:03:04  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:03:04  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa

            // time:10:03:04  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:10:03:04  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x7 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:10:03:04  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa
            // time:10:03:04  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa
            
            0x8, 
            {
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    
    {
        "original stripe: MR3\n\
         action: issue MR3 write to the first stripe and got incomplete on both parity disks.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        ORIG_STATE_MR3,                 // orig_state
        0,                              // lba
        6 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x03, //0b0000 0011,              // iw_bitmap
        0x0C, //0b0000 1100,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:10:03:30  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:03:30  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:03:30  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:03:30  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:10:03:30  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x4 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:10:03:30  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x5 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:10:03:30  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:10:03:30  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x7 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:10:03:30  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa
            // time:10:03:30  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa

            0xa, 
            {
                // Parity Sector Reconstructed RG: %2 Pos: %3 LBA: %4 Blocks: %5 Err info: %6 Extra info: %7
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x4, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x5, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    
    // new added
    {
        "original stripe: 468\n\
         action: issue 468 write to the first stripe and got incomplete write on all data disks.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0xF0, //0b1111 0000             // orig_468_ws_bitmap
        0,                              // lba
        2 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0xC0, //0b1100 0000,              // iw_bitmap
        0x0C, //0b0000 1100,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:08:09:44  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:08:09:44  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:08:09:44  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:08:09:44  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:08:09:44  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:08:09:44  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x7 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:08:09:44  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0x22f
            // time:08:09:44  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0x22f
            0x8, 
            {
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },

    {
        "original stripe: 468\n\
         action: issue RCW write to the first stripe and got incomplete write on all data disks.\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0xF0, //0b1111 0000             // orig_468_ws_bitmap
        0,                              // lba
        4 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0xF0, //0b1111 0000,              // iw_bitmap
        0x0C, //0b0000 1100,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:08:09:44  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:08:09:44  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:08:09:44  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:08:09:44  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0x22f
            // time:08:09:44  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x4 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:08:09:44  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x5 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:08:09:44  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:08:09:44  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x7 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0x22f
            // time:08:09:44  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0x22f
            // time:08:09:44  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0x22f
            0xA, 
            {
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x4, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x5, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x6, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_WS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    
    
    {
        "original stripe: 468\n\
         action: issue MR3 write to the first stripe and got incomplete on data disks and on one parity disk. two fatal pos on data disk\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0xF0, //0b1111 0000             // orig_468_ws_bitmap
        0,                              // lba
        6 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0xF2, //0b1111 0010,              // iw_bitmap
        0x0C, //0b0000 1100,              // fatal_bitmap
        1,                                // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:14:43:35  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:14:43:35  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:14:43:35  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:14:43:35  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:14:43:35  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:14:43:35  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:14:43:35  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa
            0x7, 
            {
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_TS),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    
    {
        "original stripe: 468\n\
         action: issue MR3 write to the first stripe and got incomplete on one parity disk. two fatal pos on data disk\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_468,                 // orig_state
        0xF0, //0b1111 0000             // orig_468_ws_bitmap
        0,                              // lba
        6 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x02, //0b0000 0010,              // iw_bitmap
        0x0C, //0b0000 1100,              // fatal_bitmap
        1,                                // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:15:01:43  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:15:01:43  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x8 Extra info: 0xa
            // time:15:01:43  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:15:01:43  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x2 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:15:01:43  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:15:01:43  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x3 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:15:01:43  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa
            0x7, 
            {
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_WS),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (VR_TS),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x2, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x3, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    {
        "original stripe: MR3\n\
         action: issue MR3 write to the first stripe and got incomplete on data disks. one fatal pos is on parity disk and the other one is on data disk\n\
         expectation: the sector on the fatal drive could NOT be recovered successfully\n", // description_p 
        __LINE__,                       // pass_count
        8,                              // rg_width
        ORIG_STATE_MR3,                 // orig_state
        0x00, //0b0000 0000             // orig_468_ws_bitmap
        0,                              // lba
        6 * BANIO_STRIPE_ELEMENT_SIZE,                        // blocks
        0x40, //0b0100 0000,              // iw_bitmap
        0x81, //0b1000 0001,              // fatal_bitmap
        1,                              // expected_io_error_count;
        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, // block_status
        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, // block_qualifier
        BANIO_FATAL_BLK_DATA_LOST,      // expected_fatal_blk_data;
        {
            // time:16:14:20  msg_id:0x61680002  msg_string:Sector Reconstructed RG: 0x12d Pos: 0x6 LBA: 0x7f Blocks: 0x1 Err info: 0x4 Extra info: 0xa
            // time:16:14:20  msg_id:0xe1688008  msg_string:Uncorrectable Sector RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:16:14:20  msg_id:0xe1688001  msg_string:Data Sector Invalidated RAID Group: 0x12d Position: 0x7 LBA: 0x7f Blocks: 0x1 Error info: 0x0 Extra info: 0xa
            // time:16:14:20  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x0 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa
            // time:16:14:20  msg_id:0x61680003  msg_string:Parity Sector Reconstructed RG: 0x12d Pos: 0x1 LBA: 0x7f Blocks: 0x1 Err info: 0x0 Extra info: 0xa
            0x5, 
            {
                { 
                    0x6, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,        /* expected error code */
                    (VR_TS),                                        /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,  /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x7, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,    /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x0, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
                { 
                    0x1, /* expected fru position */ 
                    SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                    (0),                                            /* expected error info */
                    0x7f,                                           /* expected lba */
                    0x1                                             /* expected blocks */
                },
            }
        }
    },
    /* terminator */
    {
        0,0,0,0,0,0,0,
    },
};

fbe_banio_r6_iw_case_t *banio_r6_iw_error_cases[BANIO_R6_IW_MAX_ERROR_CASES_TABLES] =
{
    &banio_iw_r6_test_cases_single_fault[0],
    &banio_iw_r6_test_cases_double_fault[0],
    NULL 
};

//  RG configuration for the R6 
fbe_test_rg_configuration_array_t banio_rg_config_g[] = 
{
    // raid6
    {
        //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
        {6, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 0, 0},
        {8, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 1, 0},
        {16, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 2, 0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

#define BANIO_RDGEN_CONTEXT_INDEX_ORIG_IO          0
#define BANIO_RDGEN_CONTEXT_INDEX_IW_IO            1
#define BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_READ   2
#define BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_WRITE  3

static fbe_api_rdgen_context_t rdgen_context[8] = {0, };
fbe_api_terminator_device_handle_t  banio_drive_info[16] = {0,};                   //  drive info needed for reinsertion 
extern fbe_u32_t    sep_rebuild_utils_number_physical_objects_g;
extern fbe_u32_t    sep_rebuild_utils_next_hot_spare_slot_g;

//------------------------------------------------------
void banio_setup(void);
void banio_cleanup(void);
void banio_test(void);
static void banio_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
static fbe_status_t banio_iterate_test_table(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p[], fbe_u32_t num_raid_groups);
static fbe_u32_t banio_get_table_case_count(fbe_banio_r6_iw_case_t *iw_case_p);
static fbe_u32_t banio_get_test_case_count(fbe_banio_r6_iw_case_t *iw_case_p[]);
static void banio_run_test_cases_for_all_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                                     fbe_banio_r6_iw_case_t *iw_case_p,
                                                     fbe_u32_t num_raid_groups,
                                                     fbe_u32_t overall_test_case_index,
                                                     fbe_u32_t total_test_case_count,
                                                     fbe_u32_t table_test_case_index,
                                                     fbe_u32_t table_test_case_count);
static fbe_status_t banio_run_one_test_case_on_one_rg(fbe_test_rg_configuration_t *rg_config_p,
                                               fbe_banio_r6_iw_case_t *iw_case_p);

static fbe_bool_t banio_is_rg_valid_for_error_case(fbe_banio_r6_iw_case_t *iw_case_p,
                                                   fbe_test_rg_configuration_t *rg_config_p);

static void banio_wait_for_lun_verify_complete(fbe_test_rg_configuration_t* config_p);

static fbe_status_t banio_prepare_orig_strip(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p);

static fbe_status_t banio_inject_iw_error(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p);
static fbe_status_t banio_issue_write_io(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p);
static fbe_status_t banio_wait_iw(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p);
static fbe_status_t banio_disable_iw_error(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p);
static fbe_status_t banio_fail_rg(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p);
static fbe_status_t banio_restore_rg(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p);
static fbe_status_t banio_read_data_on_fatal_data_disk(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p);
static fbe_status_t banio_write_data_on_fatal_data_disk(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p);
static fbe_status_t banio_validate_verify_report(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p);
static fbe_status_t banio_validates_event_log_message(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p);


static fbe_u32_t banio_ones_count(fbe_u16_t bitmap);



static void banio_send_io(fbe_test_rg_configuration_t *rg_config_p,
                          fbe_api_rdgen_context_t *context_p,
                          fbe_rdgen_operation_t rdgen_operation,
                          fbe_rdgen_pattern_t pattern,
                          fbe_data_pattern_sp_id_t sp_id,
                          fbe_lba_t lba,
                          fbe_block_count_t blks,
                          fbe_u32_t pass_count);
static void banio_itoa(fbe_u16_t short_count, fbe_u8_t *buf, fbe_u32_t base);

static void banio_test_inject_error_record(fbe_test_rg_configuration_t*           in_rg_config_p,
                                           fbe_u16_t                              pos_bitmap,
                                           fbe_lba_t                              in_lba,
                                           fbe_lba_t                              in_blocks,
                                           fbe_u32_t                              in_error_limit,
                                           fbe_u32_t                              in_delay_time, /* for IO delay error */
                                           fbe_api_logical_error_injection_type_t in_err_type,
                                           fbe_u32_t                              in_opcode,
                                           fbe_u16_t                              in_rg_width);

static void banio_test_wait_for_error_injection_complete(fbe_test_rg_configuration_t* in_rg_config_p, fbe_u16_t in_err_record_cnt);

static void banio_test_disable_error_inject(fbe_test_rg_configuration_t*      in_rg_config_p);

static void banio_validate_io_on_fatal_disk_op_read(fbe_api_rdgen_context_t *context_p, fbe_banio_r6_iw_case_t *iw_case_p);
static void banio_validate_io_on_fatal_disk_op_write(fbe_api_rdgen_context_t *context_p, fbe_banio_r6_iw_case_t *iw_case_p);

static void banio_set_dbg_flags(fbe_object_id_t raid_group_id, fbe_u32_t obj_dbg_flags, fbe_u32_t lib_dbg_flags);

//------------------------------------------------------

/*!**************************************************************
 * banio_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
void banio_setup(void)
{
    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    mumford_the_magician_common_setup();

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_array_t *array_p = NULL;

        if (extended_testing_level == 0)
        {
            /* Qual.
             */
            array_p = &banio_rg_config_g[0];
        }
        else
        {
            /* Extended. 
             */
            array_p = &banio_rg_config_g[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(&array_p[0][0]);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
        elmo_load_sep_and_neit();
    }

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    return;
}

/*!**************************************************************
 * banio_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the banio test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *
 ****************************************************************/
void banio_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    fbe_test_sep_util_restore_sector_trace_level();

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!**************************************************************
 * banio_test()
 ****************************************************************
 * @brief
 *  Run an I/O error retry test.
 *
 * @param raid_type      
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
void banio_test()
{
    fbe_u32_t                   extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Run the error cases for all raid types supported
     */
    mut_printf(MUT_LOG_TEST_STATUS, "extended_testing_level = %d\n", extended_testing_level);
    if (extended_testing_level == 0)
    {
        /* Qual.
        */
        rg_config_p = &banio_rg_config_g[0][0];
    }
    else
    {
        /* Extended. 
        */
        rg_config_p = &banio_rg_config_g[0][0];
    }
        
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, banio_r6_iw_error_cases, 
                                                  banio_test_rg_config,
                                                  BANIO_LUNS_PER_RAID_GROUP,
                                                  BANIO_CHUNKS_PER_LUN,
                                                  FBE_FALSE);
    return;
}


/*!**************************************************************
 * banio_test_rg_config()
 ****************************************************************
 * @brief
 *  Run an IW write test case
 *
 * @param .               
 *     rg_config_p -- the RG configuration on which the test run
 *     context_p   -- could be typecast to (fbe_banio_r6_iw_case_t **)
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
static void banio_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_banio_r6_iw_case_t  **error_cases_p = NULL;
    error_cases_p = (fbe_banio_r6_iw_case_t **) context_p; 

    mut_printf(MUT_LOG_TEST_STATUS, "running banio test");

    /*  Disable automatic sparing so that no spares swap-in.
     */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    big_bird_write_background_pattern();

    // wait for LUN zero and system verify complete
    banio_wait_for_lun_verify_complete(rg_config_p);
    
    /* Run test cases for this set of raid groups with this element size.
    */
    banio_iterate_test_table(rg_config_p, error_cases_p, raid_group_count);

    /*  Enable automatic sparing at the end of the test */
    status = fbe_api_control_automatic_hot_spare(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure we did not get any trace errors.  We do this in between each
     * raid group test since it stops the test sooner. 
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}

/*!**************************************************************
 * banio_get_table_case_count()
 ****************************************************************
 * @brief
 *  get the test cases count in one test table
 *
 * @param .               
 *     iw_case_p -- the pointer of the first test case in the test table
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
static fbe_u32_t banio_get_table_case_count(fbe_banio_r6_iw_case_t *iw_case_p)
{
    fbe_u32_t test_case_index = 0;

    /* Loop over all the cases in this table until we hit a terminator.
     */
    while (iw_case_p[test_case_index].blocks != 0)
    {
        test_case_index++;
    }
    return test_case_index;
}

/*!**************************************************************
 * banio_get_test_case_count()
 ****************************************************************
 * @brief
 *  get the whole test cases count in all test tables
 *
 * @param .               
 *     iw_case_p[] -- the pointer of the first test table 
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
static fbe_u32_t banio_get_test_case_count(fbe_banio_r6_iw_case_t *iw_case_p[])
{
    fbe_u32_t test_case_count = 0;
    fbe_u32_t table_index = 0;
    fbe_u32_t test_case_index;

    /* Loop over all the tables until we hit a terminator.
     */
    while (iw_case_p[table_index] != NULL)
    {
        test_case_index = 0;
        /* Add the count for this table.
         */
        test_case_count += banio_get_table_case_count(&iw_case_p[table_index][0]);
        table_index++;
    }
    return test_case_count;
}

/*!**************************************************************
 * banio_iterate_test_table()
 ****************************************************************
 * @brief
 *  Run a retry test for all test cases passed in,
 *  and for all raid groups that are part of the input rg_config_p.
 *  
 * @param rg_config_p - Raid group table.
 * @param iw_case_p - Table of test cases.
 * @param num_raid_groups - Number of raid groups.
 * 
 * @return fbe_status_t
 *
 * @author
 *
 ****************************************************************/
static fbe_status_t banio_iterate_test_table(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_banio_r6_iw_case_t *iw_case_p[],
                                             fbe_u32_t num_raid_groups)
{
    fbe_u32_t test_index = 0;
    fbe_u32_t table_test_index;
    fbe_u32_t total_test_case_count = banio_get_test_case_count(iw_case_p);
    fbe_u32_t start_table_index = 0;
    fbe_u32_t table_index = 0;
    fbe_u32_t start_test_index = 0;
    fbe_u32_t table_test_count;
    fbe_u32_t max_case_count = FBE_U32_MAX;
    fbe_u32_t test_case_executed_count = 0;
    fbe_u32_t repeat_count = 1;
    fbe_u32_t current_iteration = 0;
    /* The user can optionally choose a table index or test_index to start at.
     * By default we continue from this point to the end. 
     */
    fbe_test_sep_util_get_cmd_option_int("-start_table", &start_table_index);
    fbe_test_sep_util_get_cmd_option_int("-start_index", &start_test_index);
    fbe_test_sep_util_get_cmd_option_int("-total_cases", &max_case_count);
    mut_printf(MUT_LOG_TEST_STATUS, "banio start_table: %d start_index: %d total_cases: %d\n",start_table_index, start_test_index, max_case_count);

    /* Loop over all the tests for this raid type. */
    while (iw_case_p[table_index] != NULL)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Starting table %d\n", __FUNCTION__, table_index);

        /* If we have not yet reached the start index, get the next table.
         */
        if (table_index < start_table_index)
        {
            table_index++;
            continue;
        }
        table_test_index = 0;
        table_test_count = banio_get_table_case_count(&iw_case_p[table_index][0]);

        /* Loop until we hit a terminator.
         */ 
        while (iw_case_p[table_index][table_test_index].blocks != 0)
        {
            fbe_time_t start_time = fbe_get_time();
            fbe_time_t time;
            fbe_time_t msec_duration;

            if (start_test_index == iw_case_p[table_index][table_test_index].pass_count)
            {
                /* Found a match.  We allow the start_test_index to also match
                 *                 a line number in a test case. 
                 */
                start_test_index = 0;
            }
            else if (table_test_index < start_test_index)
            {
                /* If we have not yet reached the start index, get the next test case.
                 */ 
                table_test_index++;
                test_index++;
                continue;
            }
            current_iteration = 0;
            repeat_count = 1;
            fbe_test_sep_util_get_cmd_option_int("-repeat_case_count", &repeat_count);
            while (current_iteration < repeat_count)
            {
                /* Run this test case for all raid groups.
                 */
                banio_run_test_cases_for_all_raid_groups(rg_config_p, 
                                                         &iw_case_p[table_index][table_test_index], 
                                                         num_raid_groups,
                                                         test_index, total_test_case_count,
                                                         table_test_index,
                                                         table_test_count);
                current_iteration++;
                if (repeat_count > 1)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "banio completed test case iteration [%d of %d]", current_iteration, repeat_count);
                }
            }
            time = fbe_get_time();
            msec_duration = (time - start_time);
            mut_printf(MUT_LOG_TEST_STATUS,
                       "table %d test %d took %lld seconds (%lld msec)",
                       table_index, table_test_index,
                       (long long)(msec_duration / FBE_TIME_MILLISECONDS_PER_SECOND),
                       (long long)msec_duration);
            table_test_index++;
            test_index++;
            test_case_executed_count++;

            /* If the user chose a limit on the max test cases and we are at the limit, just return. 
             */  
            if (test_case_executed_count >= max_case_count)
            {
                return FBE_STATUS_OK;
            }
        }    /* End loop over all tests in this table. */

        /* After the first test case reset start_test_index, so for all the remaining tables we will 
         * start at the beginning. 
         */
        start_test_index = 0;
        table_index++;
    }    /* End loop over all tables. */
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * banio_is_rg_valid_for_error_case()
 ****************************************************************
 * @brief
 *  whether the RG configuration is suitable for this test case
 *
 * @param .               
 *     iw_case_p   -- the specific incomplete write test case
 *     rg_config_p -- the rg configuration
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
fbe_bool_t banio_is_rg_valid_for_error_case(fbe_banio_r6_iw_case_t *iw_case_p,
                                            fbe_test_rg_configuration_t *rg_config_p)
{
    return (iw_case_p->rg_width == rg_config_p->width);
}


/*!**************************************************************
 * banio_run_test_cases_for_all_raid_groups()
 ****************************************************************
 * @brief
 *     iterate each raid group configuration, run all the test cases on this RG
 *
 * @param .               
 *     rg_config_p -- the rg configuration
 *     iw_case_p   -- the specific incomplete write test case
 *     num_raid_groups -- RG numbers
 *     overall_test_case_index -- the test case index over all the IW test cases
 *     total_test_case_count  -- the over all test cases counter
 *     table_test_case_index  -- the starting test case index over the test table
 *     table_test_case_count  -- the test cases count in the test table
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
static void banio_run_test_cases_for_all_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                                     fbe_banio_r6_iw_case_t *iw_case_p,
                                                     fbe_u32_t num_raid_groups,
                                                     fbe_u32_t overall_test_case_index,
                                                     fbe_u32_t total_test_case_count,
                                                     fbe_u32_t table_test_case_index,
                                                     fbe_u32_t table_test_case_count)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t CSX_MAYBE_UNUSED extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();

    mut_printf(MUT_LOG_TEST_STATUS, "test_case: %s", iw_case_p->description_p); 
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    mut_printf(MUT_LOG_TEST_STATUS, "starting table case: (%d of %d) overall: (%d of %d) line: %d for raid type %s (0x%x)", 
               table_test_case_index, table_test_case_count,
               overall_test_case_index, total_test_case_count, 
               iw_case_p->pass_count, raid_type_string_p, rg_config_p->raid_type);

    for (rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        // whether we could run this test case in this RG
        if (banio_is_rg_valid_for_error_case(iw_case_p, &rg_config_p[rg_index]))
        {
            status = banio_run_one_test_case_on_one_rg(&rg_config_p[rg_index], iw_case_p);
            if (status != FBE_STATUS_OK)
            {
                MUT_FAIL_MSG("test case failed\n");
            }
        }
    }

    return;
}

/*!****************************************************************************
 *  banio_wait_for_lun_verify_complete
 ******************************************************************************
 *
 * @brief
 *   wait for LUN verify complete
 *
 * @param   
 *      config_p -- rg configuration
 *
 * @return  None 
 *
 *****************************************************************************/
static void banio_wait_for_lun_verify_complete(fbe_test_rg_configuration_t* config_p)
{
    fbe_u32_t lun_index;
    fbe_object_id_t lun_object_id;
    fbe_status_t        status;
    fbe_api_lun_get_lun_info_t lun_info;
    fbe_lun_verify_report_t verify_report;

    for (lun_index = 0; lun_index < config_p->number_of_luns_per_rg; lun_index++)
    {
        fbe_lun_number_t lun_number =  config_p->logical_unit_configuration_list[lun_index].lun_number;

        MUT_ASSERT_INT_EQUAL(config_p->logical_unit_configuration_list[lun_index].raid_group_id, config_p->raid_group_id);
        status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* First make sure we finished the zeroing.
        */
        fbe_test_sep_util_wait_for_lun_zeroing_complete(lun_number, 60000, FBE_PACKAGE_ID_SEP_0);

        /* Then make sure we finished the verify.  This waits for pass 1 to complete.
        */
        fbe_test_sep_util_wait_for_lun_verify_completion(lun_object_id, &verify_report, 1);


        /* Next make sure that the LUN was requested to do initial verify and that it performed it already.
        */
        status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_already_run, 1);
        MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_requested, 1);
    }
}

/*!****************************************************************************
 *  banio_send_io
 ******************************************************************************
 *
 * @brief
 *   send one IO to the first LUN of the rg
 *
 * @param   
 *      rg_config_p -- rg configuration
 *      context_p -- rdgen context used to run this IO
 *      rdgen_operation -- rdgen io operation code
 *      pattern -- rdgen IO pattern
 *      sp_id -- SP ID to which this IO is going to 
 *      lba -- IO starting LBA
 *      blks -- IO block counts
 *      pass_count -- used to generate the proper blk content with FBE_RDGEN_PATTERN_LBA_PASS
 *
 * @return  None 
 *
 *****************************************************************************/
static void banio_send_io(fbe_test_rg_configuration_t *rg_config_p,
                          fbe_api_rdgen_context_t *context_p,
                          fbe_rdgen_operation_t rdgen_operation,
                          fbe_rdgen_pattern_t pattern,
                          fbe_data_pattern_sp_id_t sp_id,
                          fbe_lba_t lba,
                          fbe_block_count_t blks,
                          fbe_u32_t pass_count)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_block_count_t elsz;
    fbe_u32_t idx; 

    /*  */
    elsz = fbe_test_sep_util_get_element_size(rg_config_p);

    /* Get Lun object IDs */
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    /* Initialize rdgen context */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  rdgen_operation,
                                                  FBE_LBA_INVALID, /* use max capacity */
                                                  elsz /* max blocks.  We override this later with regions.  */);

    /* status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification, peer_options); */

    /* Create one region in rdgen context for this IO.
     */
    idx = context_p->start_io.specification.region_list_length; 
    context_p->start_io.specification.region_list[idx].lba = lba;
    context_p->start_io.specification.region_list[idx].blocks = blks;
    context_p->start_io.specification.region_list[idx].pattern = pattern;
    context_p->start_io.specification.region_list[idx].pass_count = pass_count;
    context_p->start_io.specification.region_list[0].sp_id = sp_id; 
    context_p->start_io.specification.region_list_length++;

    /* Set flag to use regions */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_USE_REGIONS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Send IO */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}


/*!****************************************************************************
 *  banio_prepare_orig_strip
 ******************************************************************************
 *
 * @brief
 *   generate the orig strip 
 *
 * @param   
 *      rg_config_p -- rg configuration
 *      iw_case_p -- incomplete write test case
 *
 * @return  None 
 *
 *****************************************************************************/
static fbe_status_t banio_prepare_orig_strip(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    fbe_status_t status;
    fbe_u32_t data_disk_pos;
    fbe_u32_t data_disks;
    fbe_lba_t lba;
    data_disks = rg_config_p->width - 2;

    // issue a full stripe MR3 write to the first stripe
    banio_send_io(rg_config_p, 
                  &rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_ORIG_IO], 
                  FBE_RDGEN_OPERATION_WRITE_ONLY, 
                  FBE_RDGEN_PATTERN_LBA_PASS, 
                  FBE_DATA_PATTERN_SP_ID_A, 
                  0, 
                  BANIO_STRIPE_ELEMENT_SIZE * data_disks,
                  iw_case_p->pass_count);

    fbe_api_rdgen_wait_for_ios(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_ORIG_IO], FBE_PACKAGE_ID_SEP_0, 1);
    status = fbe_api_rdgen_test_context_destroy(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_ORIG_IO]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (iw_case_p->orig_state == ORIG_STATE_468)
    {
        // make the write stamps on parity likes "1010..0101"
        for (data_disk_pos = 0; data_disk_pos < data_disks; data_disk_pos++)
        {
            if (iw_case_p->orig_468_ws_bitmap & (1 << (data_disk_pos + 2)))
            {
                lba = (data_disks - data_disk_pos - 1) * BANIO_STRIPE_ELEMENT_SIZE;
                // issue a full stripe MR3 write to the first stripe
                banio_send_io(rg_config_p, 
                              &rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_ORIG_IO], 
                              FBE_RDGEN_OPERATION_WRITE_ONLY, 
                              FBE_RDGEN_PATTERN_LBA_PASS, 
                              FBE_DATA_PATTERN_SP_ID_A, 
                              lba,
                              BANIO_STRIPE_ELEMENT_SIZE,
                              iw_case_p->pass_count);
                fbe_api_rdgen_wait_for_ios(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_ORIG_IO], FBE_PACKAGE_ID_SEP_0, 1);
                status = fbe_api_rdgen_test_context_destroy(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_ORIG_IO]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *  banio_itoa
 ******************************************************************************
 *
 * @brief
 *   change a short integer to binary string
 *
 * @param   
 *      short_count -- the short integer
 *      buf -- the string buffer 
 *      base -- only support base=2
 *
 * @return  None 
 *
 *****************************************************************************/
static void banio_itoa(fbe_u16_t short_count, fbe_u8_t *buf, fbe_u32_t base)
{
    fbe_u32_t i;
    MUT_ASSERT_INT_EQUAL(base, 2);

    for (i = 0; i < 16; i++)
    {
        buf[i] = (short_count & 0x8000) ? '1' : '0';
        short_count <<= 1;
    }
    buf[16] = 0;
}

/*!**************************************************************
 * banio_test_inject_error_record()
 ****************************************************************
 * @brief
 *  Injects an error record
 * 
 * @param in_rg_config_p    - raid group config information
 * @param in_position       - disk position in RG to inject error
 * @param in_lba            - LBA on disk to inject error
 * @param in_blocks         - Blocks on disks to inject error
 * @param in_err_type       - error type to be injected
 * @param in_error_limit    - error times
 * @param in_delay_time     - only used for FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN, unit ms
 * @param in_opcode         - opcode the error injection applied
 * @param in_rg_width       - width of RG
 *
 * @return None.
 *
 ****************************************************************/
static void banio_test_inject_error_record(fbe_test_rg_configuration_t*           in_rg_config_p,
                                           fbe_u16_t                              pos_bitmap,
                                           fbe_lba_t                              in_lba,
                                           fbe_lba_t                              in_blocks,
                                           fbe_u32_t                              in_error_limit,
                                           fbe_u32_t                              in_delay_time, /* for IO delay error */
                                           fbe_api_logical_error_injection_type_t in_err_type,
                                           fbe_u32_t                              in_opcode,
                                           fbe_u16_t                              in_rg_width)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_logical_error_injection_record_t    error_record = {0};

    //  Get the RG object ID    
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    //  Enable the error injection service
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    //  Setup error record for error injection
    //  Inject error on given disk position and LBA
    error_record.pos_bitmap = pos_bitmap;                 //  Injecting error on given disk position
    error_record.width      = in_rg_width;                      //  RG width
    error_record.lba        = in_lba;                           //  Physical address to begin errors
    error_record.blocks     = in_blocks;                                //  Blocks to extend errors
    error_record.opcode     = in_opcode;                                //  Opcodes to error
    error_record.err_type   = in_err_type;   // Error type
    error_record.err_mode   = FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT;          // Error mode
    error_record.err_count  = 0;                                //  Errors injected so far
    error_record.err_limit  = in_error_limit;                   //  Limit of errors to inject
    error_record.skip_count = 0;                                //  Limit of errors to skip
    error_record.skip_limit = 0;                                //  Limit of errors to inject
    error_record.err_adj    = 1;                                //  Error adjacency bitmask
    error_record.start_bit  = 0;                                //  Starting bit of an error
    error_record.num_bits   = 0;                                //  Number of bits of an error
    error_record.bit_adj    = 0;                                //  Indicates if erroneous bits adjacent
    error_record.crc_det    = 0;                                //  Indicates if CRC detectable
    
	if (in_err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN
		|| in_err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP)
	{
        error_record.err_limit  = in_delay_time;
	}

    //  Create error injection record
    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Enable error injection on the RG
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection: pos 0x%04x, lba 0x%llx, bloks %llu \n", __FUNCTION__, pos_bitmap, in_lba, in_blocks);
    status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Check error injection stats
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);

    return;

}   // End banio_test_inject_error_record()


/*!****************************************************************************
 *  banio_inject_iw_error
 ******************************************************************************
 *
 * @brief
 *   inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE according to iw_case_p->iw_bitmap
 *
 * @param   
 *      rg_config_p -- rg configuration
 *      iw_case_p -- incomplete write test case
 *
 * @return  None 
 *
 *****************************************************************************/
static fbe_status_t banio_inject_iw_error(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    banio_test_inject_error_record(rg_config_p, 
                                   iw_case_p->iw_bitmap,                   /* position */
                                   0,                                         /* lba */
                                   BANIO_STRIPE_ELEMENT_SIZE,                 /* blocks */
                                   banio_ones_count(iw_case_p->iw_bitmap), /* error_limit, useless in this case */
                                   0,                                         /* delay */
                                   FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE,
                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                   rg_config_p->width);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *  banio_issue_write_io
 ******************************************************************************
 *
 * @brief
 *   issue write IO to the first strip so as to create the inconsistent strip
 *
 * @param   
 *      rg_config_p -- rg configuration
 *      iw_case_p -- incomplete write test case
 *
 * @return  None 
 *
 *****************************************************************************/
static fbe_status_t banio_issue_write_io(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    banio_send_io(rg_config_p, 
                  &rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_IW_IO], 
                  FBE_RDGEN_OPERATION_WRITE_ONLY, 
                  FBE_RDGEN_PATTERN_LBA_PASS, 
                  FBE_DATA_PATTERN_SP_ID_A, 
                  iw_case_p->lba, 
                  iw_case_p->blocks,
                  iw_case_p->pass_count + 1);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * banio_test_wait_for_error_injection_complete()
 ****************************************************************
 *
 * @brief   Wait for the error injection to complete
 *
 * @param   in_rg_config_p      - raid group config information
 * @param   in_err_record_cnt   - number of errors injected
 *
 * @return  None
 *
 ****************************************************************/
static void banio_test_wait_for_error_injection_complete(fbe_test_rg_configuration_t*    in_rg_config_p,
                                                         fbe_u16_t                       in_err_record_cnt)
{
    fbe_status_t                                        status;
    fbe_u32_t                                           sleep_count;
    fbe_u32_t                                           sleep_period_ms;
    fbe_u32_t                                           max_sleep_count;
    fbe_object_id_t                                     rg_object_id;
    fbe_api_logical_error_injection_get_stats_t         stats; 

    //  Get RG object ID
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    //  Intialize local variables
    sleep_period_ms = 1;
    max_sleep_count = (SEP_REBUILD_UTILS_MAX_ERROR_INJECT_WAIT_TIME_SECS * 1000) / sleep_period_ms;

    //  Wait for errors to be injected
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all errors to be injected(detected) ==\n", __FUNCTION__);
    for (sleep_count = 0; sleep_count < max_sleep_count; sleep_count++)
    {
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if(stats.num_errors_injected >= in_err_record_cnt)
        {
            // Break out of the loop
            mut_printf(MUT_LOG_TEST_STATUS, "== %s found %llu errors injected ==\n", __FUNCTION__, (unsigned long long)stats.num_errors_injected);
            break;
        }
                        
        // Sleep 
        fbe_api_sleep(sleep_period_ms);
    }

    //  The error injection took too long and timed out
    if (sleep_count >= max_sleep_count)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection failed ==\n", __FUNCTION__);
        MUT_ASSERT_TRUE(sleep_count < max_sleep_count);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection successful ==\n", __FUNCTION__);

    return;
} // End banio_test_wait_for_error_injection_complete()


/*!****************************************************************************
 *  banio_wait_iw
 ******************************************************************************
 *
 * @brief
 *   validate IW hit times
 *
 * @param   
 *      rg_config_p -- rg configuration
 *      iw_case_p -- incomplete write test case
 *
 * @return  None 
 *
 *****************************************************************************/
static fbe_status_t banio_wait_iw(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    banio_test_wait_for_error_injection_complete(rg_config_p, banio_ones_count(iw_case_p->iw_bitmap));
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *  banio_ones_count
 ******************************************************************************
 *
 * @brief
 *   get the ones count in bitmap
 *
 * @param   
 *      bitmap -- the integer going to be evaluated
 *
 * @return  None 
 *
 *****************************************************************************/
static fbe_u32_t banio_ones_count(fbe_u16_t bitmap)
{
    fbe_u32_t cnt = 0;
    while (bitmap)
    {
        if (bitmap & 0x1)
        {
            cnt++;
        }

        bitmap >>= 1;
    }

    return cnt;
}

/*!**************************************************************
 * banio_test_disable_error_inject()
 ****************************************************************
 * @brief
 *  disable error inject service
 * 
 * @param in_rg_config_p    - raid group config information
 *
 * @return None.
 *
 ****************************************************************/
static void banio_test_disable_error_inject(fbe_test_rg_configuration_t*      in_rg_config_p)
{
    fbe_status_t                                status;
    fbe_object_id_t rg_object_id;

    /* Disable error injection on the RG.
     */
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_api_logical_error_injection_disable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    // disable error injection
    status = fbe_api_logical_error_injection_disable_class(in_rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * banio_disable_iw_error()
 ****************************************************************
 *
 * @brief   disable the incomplete write error injection 
 *
 * @param   rg_config_p - raid group config information
 * @param   iw_case_p   - the IW test case
 *
 * @return  None
 *
 ****************************************************************/
static fbe_status_t banio_disable_iw_error(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    banio_test_disable_error_inject(rg_config_p);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * banio_fail_rg()
 ****************************************************************
 *
 * @brief   
 *          fail the RG, the IO issued on step3 would be failed with shutdown error and left an inconsistent strip on the RG
 *          reinsert all the drives except the one defined in the fbe_banio_r6_iw_case_t->fatal_bitmap
 *          wait for the RG and LUN get ready
 * 
 * @param   rg_config_p - raid group config information
 * @param   iw_case_p   - the IW test case
 *
 * @return  None
 *
 ****************************************************************/
static fbe_status_t banio_fail_rg(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    fbe_status_t status;
    fbe_u8_t string[20];
    fbe_object_id_t  raid_group_id;
    fbe_u16_t fatal_bitmap;
    fbe_u32_t pos;
    fbe_u32_t dead_count = 0;
    fbe_object_id_t lun_object_id;
    
    
    for (pos = 0; pos < 16; pos++)
    {
        banio_drive_info[pos] = 0;
    }

    //  Get the RG object ID    
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);
    
    // add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to block EVAL_RL in order to simulate the enclosure power failure case
    mut_printf(MUT_LOG_TEST_STATUS, "%s: add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to block EVAL_RL ", __FUNCTION__);
    status = fbe_api_scheduler_add_debug_hook(raid_group_id, 
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                                              FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED, 
                                              NULL, 
                                              NULL, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // remove all the disks requied by iw_case_p->fatal_bitmap
    banio_itoa(iw_case_p->fatal_bitmap, string, 2);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: remove the drives according to fatal_bitmap: %s\n", __FUNCTION__, string);

    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fatal_bitmap = iw_case_p->fatal_bitmap;
    pos = 0;
    while (fatal_bitmap)
    {
        if (fatal_bitmap & 1)
        {
            sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
            sep_rebuild_utils_remove_drive_and_verify(rg_config_p, pos, sep_rebuild_utils_number_physical_objects_g, &banio_drive_info[pos]);
            dead_count++;
        }
        else
        {
            banio_drive_info[pos] = 0;
        }

        pos++;
        fatal_bitmap >>= 1;
    }

    // pull out one or two drives to fail the RG
    pos = 0;
    while (dead_count <= 2)
    {
        if ((iw_case_p->fatal_bitmap & (1 << pos)) == 0)
        {
            sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
            sep_rebuild_utils_remove_drive_and_verify(rg_config_p, pos, sep_rebuild_utils_number_physical_objects_g, &banio_drive_info[pos]);
            dead_count++;
        }
        pos++;
    }

    // del hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to continue the EVAL_RL
    // in this step, the quienced incomplete write MR3 IO would be drained.
    mut_printf(MUT_LOG_TEST_STATUS, "%s: del hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to continue the EVAL_RL\n", __FUNCTION__);
    fbe_api_scheduler_del_debug_hook(raid_group_id, 
                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                                     FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED,
                                     NULL,
                                     NULL,
                                     SCHEDULER_CHECK_STATES,
                                     SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
	// verify the raid group get to fail state in reasonable time 
    mut_printf(MUT_LOG_HIGH, "%s: wait for rg %d get to fail in 30s\n", __FUNCTION__, raid_group_id);
	status = fbe_api_wait_for_object_lifecycle_state(raid_group_id, FBE_LIFECYCLE_STATE_FAIL, 30000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // reinsert all the drives except the ones in fatal_bitmap
    for (pos = 0; pos < 16; pos++)
    {
        if (((iw_case_p->fatal_bitmap & (1 << pos)) == 0) && banio_drive_info[pos])
        {
            sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
            sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, pos, sep_rebuild_utils_number_physical_objects_g, &banio_drive_info[pos]);
        }
    }

    // wait LUN to be ready
    /* Get Lun object IDs */
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);
    
    mut_printf(MUT_LOG_HIGH, "%s: wait for LUN get to READY in 30s\n", __FUNCTION__);
	status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY, 300000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * banio_read_data_on_fatal_data_disk()
 ****************************************************************
 *
 * @brief   
 *          verify the data on the first fatal disk by read. for each case, the data might have NEW, OLD or INVALIDATED data.
 * 
 * @param   rg_config_p      - raid group config information
 * @param   iw_case_p   - the IW test case
 *
 * @return  None
 *
 ****************************************************************/
static fbe_status_t banio_read_data_on_fatal_data_disk(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    fbe_u32_t pos;
    fbe_status_t status;
    fbe_lba_t lba;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: read the sectors on the first fatal data drive\n", __FUNCTION__);

    for (pos = 0; pos < 16; pos++)
    {
        if ((iw_case_p->fatal_bitmap & (1 << pos)) != 0 && ((0x3 & (1 << pos)) == 0))
        {
            lba = (rg_config_p->width - 1 - pos) * BANIO_STRIPE_ELEMENT_SIZE;
            
            if (iw_case_p->expected_fatal_blk_data == BANIO_FATAL_BLK_DATA_NEW)
            {
                // read one sector on the first fatal drive
                banio_send_io(rg_config_p, 
                              &rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_READ], 
                              FBE_RDGEN_OPERATION_READ_CHECK,
                              FBE_RDGEN_PATTERN_LBA_PASS, 
                              FBE_DATA_PATTERN_SP_ID_A, 
                              lba + BANIO_STRIPE_ELEMENT_SIZE - 1,
                              1,
                              iw_case_p->pass_count + 1);
            }
            else if (iw_case_p->expected_fatal_blk_data == BANIO_FATAL_BLK_DATA_OLD)
            {
                // read one sector on the first fatal drive
                banio_send_io(rg_config_p, 
                              &rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_READ], 
                              FBE_RDGEN_OPERATION_READ_CHECK,
                              FBE_RDGEN_PATTERN_LBA_PASS, 
                              FBE_DATA_PATTERN_SP_ID_A, 
                              lba + BANIO_STRIPE_ELEMENT_SIZE - 1,
                              1,
                              iw_case_p->pass_count);
            }
            else // iw_case_p->expected_fatal_blk_data == BANIO_FATAL_BLK_DATA_INVALID
            {
                // read one sector on the first fatal drive
                banio_send_io(rg_config_p, 
                              &rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_READ], 
                              FBE_RDGEN_OPERATION_READ_ONLY,
                              FBE_RDGEN_PATTERN_LBA_PASS, 
                              FBE_DATA_PATTERN_SP_ID_A, 
                              lba + BANIO_STRIPE_ELEMENT_SIZE - 1,
                              1,
                              iw_case_p->pass_count);
            }

            // only validate the data on the first fatal drive
            break;
        }
    }
    fbe_api_rdgen_wait_for_ios(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_READ], FBE_PACKAGE_ID_SEP_0, 1);

    banio_validate_io_on_fatal_disk_op_read(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_READ], iw_case_p);

    status = fbe_api_rdgen_test_context_destroy(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_READ]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * banio_write_data_on_fatal_data_disk()
 ****************************************************************
 *
 * @brief   
 *          verify the data on the first fatal disk by write. degraded write to the fatal drive would verify and reconstruct the strip on disk 
 * 
 * @param   rg_config_p      - raid group config information
 * @param   iw_case_p   - the IW test case
 *
 * @return  None
 *
 ****************************************************************/
static fbe_status_t banio_write_data_on_fatal_data_disk(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    fbe_u32_t pos;
    fbe_status_t status;
    fbe_lba_t lba;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: write the sectors on the first fatal drive\n", __FUNCTION__);

    for (pos = 0; pos < 16; pos++)
    {
        if ((iw_case_p->fatal_bitmap & (1 << pos)) != 0 && ((0x3 & (1 << pos)) == 0))
        {
            lba = (rg_config_p->width - 1 - pos) * BANIO_STRIPE_ELEMENT_SIZE;
            
            // write one sector on the first fatal drive
            banio_send_io(rg_config_p, 
                          &rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_WRITE], 
                          FBE_RDGEN_OPERATION_WRITE_ONLY,
                          FBE_RDGEN_PATTERN_LBA_PASS, 
                          FBE_DATA_PATTERN_SP_ID_A, 
                          lba + BANIO_STRIPE_ELEMENT_SIZE - 1,
                          1,
                          iw_case_p->pass_count + 2);

            // only validate the data on the first fatal drive
            break;
        }
    }
    fbe_api_rdgen_wait_for_ios(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_WRITE], FBE_PACKAGE_ID_SEP_0, 1);

    banio_validate_io_on_fatal_disk_op_write(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_WRITE], iw_case_p);
    
    status = fbe_api_rdgen_test_context_destroy(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_VERIFY_IO_WRITE]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * banio_validate_verify_report()
 ****************************************************************
 *
 * @brief   
 *          validate verify report
 * 
 * @param   rg_config_p      - raid group config information
 * @param   iw_case_p   - the IW test case
 *
 * @return  None
 *
 ****************************************************************/
static fbe_status_t banio_validate_verify_report(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    fbe_status_t status;
    fbe_object_id_t                 rg_object_id;
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_u32_t wait_msecs = 0;
    fbe_object_id_t lun_object_id;
    fbe_lun_verify_report_t verify_report;
    
    // wait for verify complete
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    while ((raid_group_info.ro_verify_checkpoint != FBE_LBA_INVALID) ||
           (raid_group_info.rw_verify_checkpoint != FBE_LBA_INVALID) ||
           (raid_group_info.error_verify_checkpoint != FBE_LBA_INVALID) ||
           (raid_group_info.incomplete_write_verify_checkpoint != FBE_LBA_INVALID) ||
           (raid_group_info.system_verify_checkpoint != FBE_LBA_INVALID) ||
           (raid_group_info.b_is_event_q_empty == FBE_FALSE))
    {
        if (wait_msecs > BANIO_WAIT_FOR_VERIFY)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s: rg: 0x%x ro chkpt: 0x%llx  rw chkpt: 0x%llx  err chkpt: 0x%llx iw chkpt: 0x%llx sys chkpt: 0x%llx", 
                       __FUNCTION__, rg_object_id,
                       raid_group_info.ro_verify_checkpoint,
                       raid_group_info.rw_verify_checkpoint, 
                       raid_group_info.error_verify_checkpoint,
                       raid_group_info.incomplete_write_verify_checkpoint,
                       raid_group_info.system_verify_checkpoint);
            MUT_FAIL_MSG("failed waiting for verify to finish");
        }
        fbe_api_sleep(200);
        wait_msecs += 200;
        status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    // get verify report
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    /* Now get the verify report for the (1) LUN associated with the raid group
     */
    status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, &verify_report);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // trace the error counters
    mut_printf(MUT_LOG_HIGH, "%s: ----------------------------------------------",  __FUNCTION__);
    mut_printf(MUT_LOG_HIGH, "%s: correctable_lba_stamp = %d", __FUNCTION__,                    verify_report.totals.correctable_lba_stamp);
    mut_printf(MUT_LOG_HIGH, "%s: correctable_media = %d", __FUNCTION__,                        verify_report.totals.correctable_media);
    mut_printf(MUT_LOG_HIGH, "%s: correctable_multi_bit_crc = %d", __FUNCTION__,                verify_report.totals.correctable_multi_bit_crc);   
    mut_printf(MUT_LOG_HIGH, "%s: correctable_shed_stamp = %d", __FUNCTION__,                   verify_report.totals.correctable_shed_stamp);      
    mut_printf(MUT_LOG_HIGH, "%s: correctable_single_bit_crc = %d", __FUNCTION__,               verify_report.totals.correctable_single_bit_crc);  
    mut_printf(MUT_LOG_HIGH, "%s: correctable_soft_media = %d", __FUNCTION__,                   verify_report.totals.correctable_soft_media);      
    mut_printf(MUT_LOG_HIGH, "%s: uncorrectable_lba_stamp = %d", __FUNCTION__,                  verify_report.totals.uncorrectable_lba_stamp);     
    mut_printf(MUT_LOG_HIGH, "%s: uncorrectable_media = %d", __FUNCTION__,                      verify_report.totals.uncorrectable_media);         
    mut_printf(MUT_LOG_HIGH, "%s: uncorrectable_multi_bit_crc = %d", __FUNCTION__,              verify_report.totals.uncorrectable_multi_bit_crc); 
    mut_printf(MUT_LOG_HIGH, "%s: uncorrectable_shed_stamp = %d", __FUNCTION__,                 verify_report.totals.uncorrectable_shed_stamp);    
    mut_printf(MUT_LOG_HIGH, "%s: uncorrectable_single_bit_crc = %d", __FUNCTION__,             verify_report.totals.uncorrectable_single_bit_crc);
    mut_printf(MUT_LOG_HIGH, "%s: correctable_coherency = %d", __FUNCTION__,                    verify_report.totals.correctable_coherency);
    mut_printf(MUT_LOG_HIGH, "%s: correctable_time_stamp = %d", __FUNCTION__,                   verify_report.totals.correctable_time_stamp);     
    mut_printf(MUT_LOG_HIGH, "%s: correctable_write_stamp = %d", __FUNCTION__,                  verify_report.totals.correctable_write_stamp);     
    mut_printf(MUT_LOG_HIGH, "%s: uncorrectable_coherency = %d", __FUNCTION__,                  verify_report.totals.uncorrectable_coherency);    
    mut_printf(MUT_LOG_HIGH, "%s: uncorrectable_time_stamp = %d", __FUNCTION__,                 verify_report.totals.uncorrectable_time_stamp);
    mut_printf(MUT_LOG_HIGH, "%s: uncorrectable_write_stamp = %d", __FUNCTION__,                verify_report.totals.uncorrectable_write_stamp);   

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * banio_validates_event_log_message()
 ****************************************************************
 *
 * @brief   
 *          validates event log message
 * 
 * @param   rg_config_p - raid group config information
 * @param   iw_case_p   - the IW test case
 *
 * @return  None
 *
 ****************************************************************/
static fbe_status_t banio_validates_event_log_message(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    fbe_status_t status;
    fbe_u32_t total_events_copied = 0;
    fbe_object_id_t     raid_group_id;
    fbe_event_log_get_event_log_info_t events_array[FBE_EVENT_LOG_MESSAGE_COUNT];
    
    //  Get the RG object ID    
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);
    
    
    status = fbe_api_get_all_events_logs(events_array, &total_events_copied, FBE_EVENT_LOG_MESSAGE_COUNT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "== Error: failed to get all event logs  == \n");

#if BANIO_DBG
    for (event_log_index = 0; event_log_index < total_events_copied; event_log_index++)
    {
        mut_printf(MUT_LOG_HIGH, "time:%02d:%02d:%02d  msg_id:0x%x  msg_string:%s",
                   events_array[event_log_index].event_log_info.time_stamp.hour,
                   events_array[event_log_index].event_log_info.time_stamp.minute,
                   events_array[event_log_index].event_log_info.time_stamp.second,
                   events_array[event_log_index].event_log_info.msg_id,
                   events_array[event_log_index].event_log_info.msg_string);
    }
#endif
    mumford_the_magician_validates_event_log_message(&iw_case_p->event_log_result, raid_group_id, FBE_TRUE); /* TRUE = verify message present */

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * banio_validate_io_on_fatal_disk_op_read()
 ****************************************************************
 *
 * @brief   
 *          validate read IO status 
 * 
 * @param   context_p - the rdgen IO context
 * @param   iw_case_p   - the IW test case
 *
 * @return  None
 *
 ****************************************************************/
static void banio_validate_io_on_fatal_disk_op_read(fbe_api_rdgen_context_t *context_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    fbe_raid_verify_error_counts_t *io_error_counts_p = &context_p->start_io.statistics.verify_errors;

    mut_printf(MUT_LOG_HIGH, "%s: ----------------read---------------------", __FUNCTION__);
    mut_printf(MUT_LOG_HIGH, "%s: error_count = %d", __FUNCTION__, context_p->start_io.statistics.error_count);
    mut_printf(MUT_LOG_HIGH, "%s: block_status = %d", __FUNCTION__, context_p->start_io.status.block_status);
    mut_printf(MUT_LOG_HIGH, "%s: block_qualifier = %d", __FUNCTION__, context_p->start_io.status.block_qualifier);
    mut_printf(MUT_LOG_HIGH, "%s: c_coh_count = %d", __FUNCTION__, io_error_counts_p->c_coh_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_coh_count = %d", __FUNCTION__, io_error_counts_p->u_coh_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_crc_multi_count = %d", __FUNCTION__, io_error_counts_p->c_crc_multi_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_crc_multi_count = %d", __FUNCTION__, io_error_counts_p->u_crc_multi_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_crc_single_count = %d", __FUNCTION__, io_error_counts_p->c_crc_single_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_crc_single_count = %d", __FUNCTION__, io_error_counts_p->u_crc_single_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_soft_media_count = %d", __FUNCTION__, io_error_counts_p->c_soft_media_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_ss_count = %d", __FUNCTION__, io_error_counts_p->c_ss_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_ss_count = %d", __FUNCTION__, io_error_counts_p->u_ss_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_ts_count = %d", __FUNCTION__, io_error_counts_p->c_ts_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_ts_count = %d", __FUNCTION__, io_error_counts_p->u_ts_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_ws_count = %d", __FUNCTION__, io_error_counts_p->c_ws_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_ws_count = %d", __FUNCTION__, io_error_counts_p->u_ws_count);
    mut_printf(MUT_LOG_HIGH, "%s: retryable_errors = %d", __FUNCTION__, io_error_counts_p->retryable_errors);
    mut_printf(MUT_LOG_HIGH, "%s: non_retryable_errors = %d", __FUNCTION__, io_error_counts_p->non_retryable_errors);
    mut_printf(MUT_LOG_HIGH, "%s: shutdown_errors = %d", __FUNCTION__, io_error_counts_p->shutdown_errors);
    mut_printf(MUT_LOG_HIGH, "%s: c_media_count = %d", __FUNCTION__, io_error_counts_p->c_media_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_media_count = %d", __FUNCTION__, io_error_counts_p->u_media_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_crc_count = %d", __FUNCTION__, io_error_counts_p->c_crc_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_crc_count = %d", __FUNCTION__, io_error_counts_p->u_crc_count);

    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, iw_case_p->expected_io_error_count);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, iw_case_p->block_status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, iw_case_p->block_qualifier);
}

/*!**************************************************************
 * banio_validate_io_on_fatal_disk_op_write()
 ****************************************************************
 *
 * @brief   
 *          validate write IO status 
 * 
 * @param   context_p - the rdgen IO context
 * @param   iw_case_p   - the IW test case
 *
 * @return  None
 *
 ****************************************************************/
static void banio_validate_io_on_fatal_disk_op_write(fbe_api_rdgen_context_t *context_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    fbe_raid_verify_error_counts_t *io_error_counts_p = &context_p->start_io.statistics.verify_errors;

    mut_printf(MUT_LOG_HIGH, "%s: -----------------write--------------------", __FUNCTION__);
    mut_printf(MUT_LOG_HIGH, "%s: error_count = %d", __FUNCTION__, context_p->start_io.statistics.error_count);
    mut_printf(MUT_LOG_HIGH, "%s: block_status = %d", __FUNCTION__, context_p->start_io.status.block_status);
    mut_printf(MUT_LOG_HIGH, "%s: block_qualifier = %d", __FUNCTION__, context_p->start_io.status.block_qualifier);
    mut_printf(MUT_LOG_HIGH, "%s: c_coh_count = %d", __FUNCTION__, io_error_counts_p->c_coh_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_coh_count = %d", __FUNCTION__, io_error_counts_p->u_coh_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_crc_multi_count = %d", __FUNCTION__, io_error_counts_p->c_crc_multi_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_crc_multi_count = %d", __FUNCTION__, io_error_counts_p->u_crc_multi_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_crc_single_count = %d", __FUNCTION__, io_error_counts_p->c_crc_single_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_crc_single_count = %d", __FUNCTION__, io_error_counts_p->u_crc_single_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_soft_media_count = %d", __FUNCTION__, io_error_counts_p->c_soft_media_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_ss_count = %d", __FUNCTION__, io_error_counts_p->c_ss_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_ss_count = %d", __FUNCTION__, io_error_counts_p->u_ss_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_ts_count = %d", __FUNCTION__, io_error_counts_p->c_ts_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_ts_count = %d", __FUNCTION__, io_error_counts_p->u_ts_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_ws_count = %d", __FUNCTION__, io_error_counts_p->c_ws_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_ws_count = %d", __FUNCTION__, io_error_counts_p->u_ws_count);
    mut_printf(MUT_LOG_HIGH, "%s: retryable_errors = %d", __FUNCTION__, io_error_counts_p->retryable_errors);
    mut_printf(MUT_LOG_HIGH, "%s: non_retryable_errors = %d", __FUNCTION__, io_error_counts_p->non_retryable_errors);
    mut_printf(MUT_LOG_HIGH, "%s: shutdown_errors = %d", __FUNCTION__, io_error_counts_p->shutdown_errors);
    mut_printf(MUT_LOG_HIGH, "%s: c_media_count = %d", __FUNCTION__, io_error_counts_p->c_media_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_media_count = %d", __FUNCTION__, io_error_counts_p->u_media_count);
    mut_printf(MUT_LOG_HIGH, "%s: c_crc_count = %d", __FUNCTION__, io_error_counts_p->c_crc_count);
    mut_printf(MUT_LOG_HIGH, "%s: u_crc_count = %d", __FUNCTION__, io_error_counts_p->u_crc_count);
    
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    
}


/*!**************************************************************
 * banio_restore_rg()
 ****************************************************************
 *
 * @brief   
 *          restore the RG for the next test_case
 * 
 * @param   rg_config_p - raid group config information
 * @param   iw_case_p   - the IW test case
 *
 * @return  None
 *
 ****************************************************************/
static fbe_status_t banio_restore_rg(fbe_test_rg_configuration_t *rg_config_p, fbe_banio_r6_iw_case_t *iw_case_p)
{
    fbe_object_id_t  raid_group_id;
    fbe_u32_t pos;
    
    //  Get the RG object ID    
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);
    
    // reinsert all the drives except the ones in fatal_bitmap
    for (pos = 0; pos < 16; pos++)
    {
        if (((iw_case_p->fatal_bitmap & (1 << pos)) != 0) && banio_drive_info[pos])
        {
            sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
            sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, pos, sep_rebuild_utils_number_physical_objects_g, &banio_drive_info[pos]);
        }
    }
    // wait until rebuild finished
   return fbe_test_sep_util_wait_for_raid_group_to_rebuild(raid_group_id);
}

/*!**************************************************************
 * banio_set_dbg_flags()
 ****************************************************************
 *
 * @brief   
 *          set object level and raid library dbg flags for one RG
 * 
 * @param   raid_group_id - raid group id
 * @param   obj_dbg_flags   - object level dbg flags
 * @param   lib_dbg_flags   - raid library level dbg flags
 *
 * @return  None
 *
 ****************************************************************/
static void banio_set_dbg_flags(fbe_object_id_t raid_group_id, fbe_u32_t obj_dbg_flags, fbe_u32_t lib_dbg_flags)
{
    fbe_status_t    status;

    status = fbe_api_raid_group_set_group_debug_flags(raid_group_id, obj_dbg_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_raid_group_set_library_debug_flags(raid_group_id, lib_dbg_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}

/*!**************************************************************
 * banio_run_one_test_case_on_one_rg()
 ****************************************************************
 * @brief
 *  run one specific test case on one specific rg
 *
 * @param 
 * config_p     -- the rg configuration on which the test case run
 * iw_case_p    -- the IW write case
 *
 * @return None.
 *
 * @author
 *  11/20/2013 - Created. Geng.Han
 *
 ****************************************************************/
fbe_status_t banio_run_one_test_case_on_one_rg(fbe_test_rg_configuration_t *config_p,
                                               fbe_banio_r6_iw_case_t *iw_case_p)
{
    fbe_status_t        status;
    fbe_object_id_t     raid_group_id;
    fbe_u8_t string[16];
    fbe_object_id_t lun_object_id;
    
    //  Get the RG object ID    
    fbe_api_database_lookup_raid_group_by_number(config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);
    
    status = fbe_api_database_lookup_lun_by_number(config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    // open some debug flags for this RG
    banio_set_dbg_flags(raid_group_id, banio_object_debug_flags, banio_library_debug_flags);

    // wait for LUN zero and system verify complete
    // banio_wait_for_lun_verify_complete(config_p);

    // Make sure verify is not running before we begin this test
    status = fbe_test_sep_util_wait_for_raid_group_verify(raid_group_id, BANIO_WAIT_FOR_VERIFY);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Clear following old statistics:
     *  1. sep event logs
     *  2. verify report
     *  3. sep event logs statistics
     */
    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "== %s Failed to clear SEP event log! == ");
    status = fbe_api_lun_clear_verify_report(lun_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_clear_event_log_statistics(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "== %s Failed to clear SEP event log statistics! == ");

    // step1: generate the orig strip 
    mut_printf(MUT_LOG_HIGH, "%s: generate the orig strip, width = %d, orig_state = %s\n", __FUNCTION__, config_p->width, iw_case_p->orig_state == ORIG_STATE_MR3 ? "MR3" : "468");
    status = banio_prepare_orig_strip(config_p, iw_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step2: inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE according to iw_case_p->iw_bitmap
    banio_itoa(iw_case_p->iw_bitmap, string, 2);
    mut_printf(MUT_LOG_HIGH, "%s: inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE on iw_bitmap:%s\n", __FUNCTION__, string);
    status = banio_inject_iw_error(config_p, iw_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step3: issue write IO to the first strip so as to create the inconsistent strip
    mut_printf(MUT_LOG_HIGH, "%s: issue write IO to lba:0x%llx, blocks:0x%llx\n", __FUNCTION__, iw_case_p->lba, iw_case_p->blocks);
    status = banio_issue_write_io(config_p, iw_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step4: validate IW hit times
    mut_printf(MUT_LOG_HIGH, "%s: validate IW hit counts, expected times = %d\n", __FUNCTION__, banio_ones_count(iw_case_p->iw_bitmap));
    status = banio_wait_iw(config_p, iw_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step5: disable error injection FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE
    mut_printf(MUT_LOG_TEST_STATUS, "%s: disable error injection FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE", __FUNCTION__);
    status = banio_disable_iw_error(config_p, iw_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step6: 6_1> fail the RG, the IO issued on step3 would be failed with shutdown error and left an inconsistent strip on the RG
    //        6_2> reinsert all the drives except the one defined in the fbe_banio_r6_iw_case_t->fatal_bitmap
    //        6_3> wait for the RG and LUN get ready
    mut_printf(MUT_LOG_TEST_STATUS, "%s: fail the RG", __FUNCTION__);
    status = banio_fail_rg(config_p, iw_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step7: verify the data on the first fatal disk by read. for each case, the data might have NEW, OLD or INVALIDATED data.
    mut_printf(MUT_LOG_TEST_STATUS, "%s: verify the data on the first fatal disk by read\n", __FUNCTION__);
    status = banio_read_data_on_fatal_data_disk(config_p, iw_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step8: verify the data on the first fatal disk by write. degraded write to the fatal drive would verify and reconstruct the strip on disk 
    mut_printf(MUT_LOG_TEST_STATUS, "%s: verify the data on the first fatal disk by write\n", __FUNCTION__);
    status = banio_write_data_on_fatal_data_disk(config_p, iw_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step9: validates event log message
    mut_printf(MUT_LOG_TEST_STATUS, "%s: validate the event log\n", __FUNCTION__);
    status = banio_validates_event_log_message(config_p, iw_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    // step10: restore the RG for the next test_case
    mut_printf(MUT_LOG_TEST_STATUS, "%s: restore the RG", __FUNCTION__);
    status = banio_restore_rg(config_p, iw_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step11: validate verify report
    mut_printf(MUT_LOG_TEST_STATUS, "%s: validate the verify report -- after restore", __FUNCTION__);
    status = banio_validate_verify_report(config_p, iw_case_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_rdgen_wait_for_ios(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_IW_IO], FBE_PACKAGE_ID_SEP_0, 1);
    status = fbe_api_rdgen_test_context_destroy(&rdgen_context[BANIO_RDGEN_CONTEXT_INDEX_IW_IO]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_HIGH, "%s: end\n", __FUNCTION__);
    return FBE_STATUS_OK;
}



