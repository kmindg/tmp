/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file ugly_duckling_test.c
 ***************************************************************************
 *
 * @brief
 *    This file contains tests to check the handling of metadata errors in PVD
 * when background operation is in progress(e.g. Zeroing)
 *
 * @version
 *    03/19/2012 - Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*
 * INCLUDE FILES:
 */

#include "mut.h"                                    // define mut framework interfaces and structures
#include "fbe_test_configurations.h"                // define test configuration interfaces and structures
#include "fbe_test_common_utils.h"                  // define common utility functions 
#include "fbe_test_package_config.h"                // define package load/unload interfaces
#include "fbe/fbe_api_discovery_interface.h"        // define discovery interfaces and structures
#include "fbe/fbe_api_logical_error_injection_interface.h" 
                                                    // define logical error injection interfaces and structures
#include "fbe/fbe_api_provision_drive_interface.h"  // define provision drive interfaces and structures
#include "fbe/fbe_api_sim_server.h"                 // define sim server interfaces
#include "fbe/fbe_api_utils.h"                      // define fbe utility interfaces
#include "sep_tests.h"                              // define sep test interfaces
#include "sep_utils.h"                              // define sep utility interfaces
#include "sep_hook.h"                               // defines debug hooks
#include "pp_utils.h"                               // define physical package util functions
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "xorlib_api.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_private_space_layout.h"
#include "sep_rebuild_utils.h"
#include "neit_utils.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe_metadata.h"
#include "fbe/fbe_api_trace_interface.h"
/*
 * NAMED CONSTANTS:
 */

// Required minimum disk to perform test
#define UGLY_DUCKLING_4160_DISK_COUNT                       0 // number of minimum disk having 4160 block size
#define UGLY_DUCKLING_520_DISK_COUNT                        5 // number of minimum disk having 520 block size

// miscellaneous constants
#define UGLY_DUCKLING_POLLING_INTERVAL             100  // polling interval (in ms)
#define UGLY_DUCKLING_PVD_READY_WAIT_TIME           20  // max. time (in seconds) to wait for pvd to become ready
#define UGLY_DUCKLING_VERIFY_PASS_WAIT_TIME         30  // max. time (in seconds) to wait for verify pass to complete
#define UGLY_DUCKLING_CHUNK_SIZE                  0x800
#define UGLY_DUCKLING_TEST_LUNS_PER_RAID_GROUP    1
#define UGLY_DUCKLING_TEST_CHUNKS_PER_LUN       65
#define UGLY_DUCKLING_TEST_RAID_GROUP_COUNT     1
#define UGLY_DUCKLING_MAX_IO_COUNT              10
#define UGLY_DUCKLING_CHECKPOINT_WAIT_IN_SEC 120
#define UGLY_DUCKLING_WAIT_SEC             120
#define UGLY_DUCKLING_USER_CHUNKS_IN_ONE_MD_BLOCK 256  /* 512 / 2 (sizeof paged metadata region (2 byte for PVD)*/

static fbe_api_rdgen_context_t test_contexts[UGLY_DUCKLING_TEST_LUNS_PER_RAID_GROUP * UGLY_DUCKLING_TEST_RAID_GROUP_COUNT];


/* This structure is copy from xorlib_sector_invalidate_info_t only the fields that 
 * we care about. Since this structure is buried in a header file inside it is easier/cleaner
 * to just duplicate the structure than to pull the header file and the associated library files
 */
typedef struct ugly_duckling_invalid_sector_info_s
{
    fbe_u32_t dont_care[12];                                          /*   0 */
    fbe_u32_t first_reason;                                       /*  12 */
    fbe_u32_t first_who;                                          /*  13 */
    fbe_u32_t last_reason;                                        /*  14 */
    fbe_u32_t last_who;                                           /*  15 */
    xorlib_sector_invalid_reason_t  reason;                     /*  16 */
    xorlib_sector_invalid_who_t     who;                        /*  17 */
}ugly_duckling_invalid_sector_info_t;

/*!*******************************************************************
 * @struct ugly_duckling_io_profile_s
 *********************************************************************
 * @brief This is an i/o profile of particular test.
 *
 *********************************************************************/
typedef struct ugly_duckling_io_profile_s
{
    fbe_lba_t                   start_lba;
    fbe_block_count_t           block_count;
    fbe_rdgen_operation_t       rdgen_operation;
    fbe_rdgen_pattern_t         rdgen_pattern;
}
ugly_duckling_io_profile_t;

typedef enum ugly_duckling_test_index_e
{
    UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_WRITE_IO_TEST = 0,
    UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_READ_IO_TEST = 1,
    UGLY_DUCKLING_TEST_VERIFY_INVALIDATE_IO_TEST = 2,

    /*!@note Add new test index here.*/
    UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_LAST,
}
ugly_duckling_test_index_t;

/*!*******************************************************************
 * @struct ugly_duckling_io_configuration_s
 *********************************************************************
 * @brief This is a single test case for this test.
 *
 *********************************************************************/
typedef struct ugly_duckling_io_configuration_s
{
    ugly_duckling_test_index_t          test_index;
    fbe_raid_group_number_t         raid_group_id;
    ugly_duckling_io_profile_t    rdgen_io_config[UGLY_DUCKLING_MAX_IO_COUNT];
}
ugly_duckling_io_configuration_t;

#define UGLY_DUCKLING_INVALID_IO_CONFIGURATION {FBE_LBA_INVALID, 0x0, FBE_RDGEN_OPERATION_INVALID, FBE_RDGEN_PATTERN_INVALID}

/*
 * TYPEDEFS:
 */
typedef void (*ugly_duckling_error_function_t)(fbe_object_id_t in_pvd_object_id, 
                                               ugly_duckling_io_configuration_t *io_config,
                                               fbe_test_rg_configuration_t *rg_config_ptr,
                                               fbe_u16_t error_count);

// media error case structure used by ugly duckling test
typedef struct ugly_duckling_media_error_case_s
{
    ugly_duckling_error_function_t          test_function;                                                                 // 
    fbe_u16_t  err_count;                // logical error count

} ugly_duckling_media_error_case_t;



typedef struct ugly_duckling_test_configuration_s
{
    const char *name;
    ugly_duckling_media_error_case_t *error_config;
    ugly_duckling_io_configuration_t *io_config;
    fbe_u16_t rg_position_to_inject_error;
    fbe_test_rg_configuration_t raid_group_config[FBE_TEST_MAX_RAID_GROUP_COUNT];
} ugly_duckling_test_configuration_t;


/*!*******************************************************************
 * @struct ugly_duckling_io_configuration
 *********************************************************************
 * @brief ugly_duckling IO configuration table
 *
 *********************************************************************/
ugly_duckling_io_configuration_t ugly_duckling_io_configuration_advanced[UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_LAST] =
{
    /* IO test name index                       ,Raid Group number */
    {UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_WRITE_IO_TEST, 0,
     /* normal ZOD IO test with different block size */   
     {
      /* start lba   block count                        rdgen operation                         rdgen pattern */  
      {0x0,          UGLY_DUCKLING_CHUNK_SIZE / 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0xC00,        UGLY_DUCKLING_CHUNK_SIZE / 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x1400,       UGLY_DUCKLING_CHUNK_SIZE,            FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      UGLY_DUCKLING_INVALID_IO_CONFIGURATION,
     }, 
    },
    /* IO test name index                       ,Raid Group number */
    {UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_READ_IO_TEST, 0,
     /* ZOD IO test with partial zeroed lba range */
     {
      /* start lba   block count                        rdgen operation                         rdgen pattern */           
      {0x0,             UGLY_DUCKLING_CHUNK_SIZE / 2,       FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      UGLY_DUCKLING_INVALID_IO_CONFIGURATION,
     },
    },
    /* IO test name index                       ,Raid Group number */
    {UGLY_DUCKLING_TEST_VERIFY_INVALIDATE_IO_TEST, 0,
     /* ZOD IO test with partial zeroed lba range */
     {
      /* start lba   block count                        rdgen operation                         rdgen pattern */           
      {0x0,             UGLY_DUCKLING_CHUNK_SIZE / 2,       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      UGLY_DUCKLING_INVALID_IO_CONFIGURATION,
     },
    },
};

/*!*******************************************************************
 * @struct ugly_duckling_io_configuration
 *********************************************************************
 * @brief ugly_duckling IO configuration table
 *
 *********************************************************************/
ugly_duckling_io_configuration_t ugly_duckling_io_configuration_basic[UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_LAST] =
{
    /* IO test name index                       ,Raid Group number */
    {UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_WRITE_IO_TEST, 0,
     /* normal ZOD IO test with different block size */   
     {
      /* start lba   block count                        rdgen operation                         rdgen pattern */  
      {0x0,          UGLY_DUCKLING_CHUNK_SIZE / 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      UGLY_DUCKLING_INVALID_IO_CONFIGURATION,
     }, 
    },
    /* IO test name index                       ,Raid Group number */
    {UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_READ_IO_TEST, 0,
     /* ZOD IO test with partial zeroed lba range */
     {
      /* start lba   block count                        rdgen operation                         rdgen pattern */           
      {0x0,             UGLY_DUCKLING_CHUNK_SIZE / 2,       FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      UGLY_DUCKLING_INVALID_IO_CONFIGURATION,
     },
    },
    /* IO test name index                       ,Raid Group number */
    {UGLY_DUCKLING_TEST_VERIFY_INVALIDATE_IO_TEST, 0,
     /* ZOD IO test with partial zeroed lba range */
     {
      /* start lba   block count                        rdgen operation                         rdgen pattern */           
      {0x0,             UGLY_DUCKLING_CHUNK_SIZE / 2,       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      UGLY_DUCKLING_INVALID_IO_CONFIGURATION,
     },
    },
};

/*
 * FORWARD REFERENCES:
 */

static void 
ugly_duckling_metadata_error_read_operation_test(fbe_object_id_t in_pvd_object_id,
                                                 ugly_duckling_io_configuration_t *io_config,
                                                 fbe_test_rg_configuration_t *rg_config_ptr,
                                                 fbe_u16_t error_count);
static void 
ugly_duckling_metadata_error_write_operation_test(fbe_object_id_t                                 in_pvd_object_id,
                                                  ugly_duckling_io_configuration_t *io_config,
                                                  fbe_test_rg_configuration_t *rg_config_ptr,
                                                  fbe_u16_t error_count);

static void 
ugly_duckling_metadata_error_bgz_operation_test(fbe_object_id_t                                 in_pvd_object_id,
                                               ugly_duckling_io_configuration_t *io_config,
                                               fbe_test_rg_configuration_t *rg_config_ptr,
                                               fbe_u16_t error_count);


static void 
ugly_duckling_metadata_soft_media_bgz_operation_test(fbe_object_id_t                                 in_pvd_object_id,
                                                     ugly_duckling_io_configuration_t *io_config,
                                                     fbe_test_rg_configuration_t *rg_config_ptr,
                                                     fbe_u16_t error_count);
static void 
ugly_duckling_metadata_error_zero_opcode_operation_test(fbe_object_id_t                                 in_pvd_object_id,
                                                        ugly_duckling_io_configuration_t *io_config,
                                               fbe_test_rg_configuration_t *rg_config_ptr,
                                               fbe_u16_t error_count);

static void 
ugly_duckling_metadata_error_permission_request_test(fbe_object_id_t                                 in_pvd_object_id,
                                                     ugly_duckling_io_configuration_t *io_config,
                                               fbe_test_rg_configuration_t *rg_config_ptr,
                                               fbe_u16_t error_count);

static void 
ugly_duckling_metadata_error_verify_invalidate_operation_test(fbe_object_id_t in_pvd_object_id,
                                                              ugly_duckling_io_configuration_t *io_config,
                                                        fbe_test_rg_configuration_t *rg_config_ptr,
                                                        fbe_u16_t error_count);

static void 
ugly_duckling_metadata_error_lba_checksum_error_test(fbe_object_id_t in_pvd_object_id,
                                                     ugly_duckling_io_configuration_t *io_config,
                                                     fbe_test_rg_configuration_t *rg_config_ptr,
                                                     fbe_u16_t error_count);
static void 
ugly_duckling_metadata_error_disk_zero_error_test(fbe_object_id_t in_pvd_object_id,
                                                  ugly_duckling_io_configuration_t *all_io_config,
                                                  fbe_test_rg_configuration_t *rg_config_ptr,
                                                  fbe_u16_t error_count);
static void 
ugly_duckling_metadata_retryable_error_disk_zero_error_test(fbe_object_id_t in_pvd_object_id,
                                                            ugly_duckling_io_configuration_t *all_io_config,
                                                            fbe_test_rg_configuration_t *rg_config_ptr,
                                                            fbe_u16_t error_count);
static void 
ugly_duckling_metadata_retryable_error_bgz_error_test(fbe_object_id_t in_pvd_object_id,
                                                      ugly_duckling_io_configuration_t *all_io_config,
                                                      fbe_test_rg_configuration_t *rg_config_ptr,
                                                      fbe_u16_t error_count);

static fbe_status_t ugly_duckling_wait_for_verify_invalidate_checkpoint(fbe_object_id_t pvd_object_id,
                                                                        fbe_u32_t wait_seconds,
                                                                        fbe_lba_t value);
static void ugly_duckling_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);

static void ugly_duckling_disable_background_operations(fbe_object_id_t pvd_object_id);

static void ugly_duckling_perform_error_injection(fbe_object_id_t pvd_object_id,
                                                  fbe_lba_t lba,
                                                  fbe_u16_t error_count,
                                                  fbe_api_logical_error_injection_type_t err_type,
                                                  fbe_block_count_t blocks);
static fbe_status_t ugly_duckling_wait_for_error_verify_checkpoint(fbe_object_id_t raid_object_id,
                                                                   fbe_lba_t value);
static void ugly_duckling_get_expected_invalid_lba_block_count(fbe_lba_t start_lba, fbe_block_count_t start_block_count,
                                                               fbe_lba_t *pre_start_lba, fbe_block_count_t *pre_block_count,
                                                               fbe_lba_t *post_start_lba, fbe_block_count_t *post_block_count);
static fbe_status_t ugly_duckling_check_for_invalid_pattern(fbe_object_id_t in_pvd_object_id,
                                                            fbe_lba_t pre_start_lba, fbe_block_count_t pre_block_count,
                                                            fbe_lba_t post_start_lba, fbe_block_count_t post_block_count);
static fbe_status_t ugly_duckling_validate_pattern(fbe_object_id_t in_pvd_object_id,
                                                   fbe_lba_t start_lba, fbe_block_count_t block_count);
static void ugly_duckling_init_error_record(fbe_api_logical_error_injection_record_t *error_record,
                                            fbe_lba_t lba,
                                            fbe_u16_t err_to_be_injected,
                                            fbe_api_logical_error_injection_type_t err_type,
                                            fbe_block_count_t blocks);

static void ugly_duckling_add_error_verify_checkpoint_hook(fbe_object_id_t raid_group_object_id, fbe_lba_t checkpoint);
static void ugly_duckling_del_error_verify_checkpoint_hook(fbe_object_id_t raid_group_object_id, fbe_lba_t checkpoint);
static void ugly_duckling_wait_for_error_verify_checkpoint_hook(fbe_object_id_t raid_group_object_id, fbe_lba_t checkpoint);
static void ugly_duckling_add_verify_invalidate_checkpoint_hook(fbe_object_id_t in_pvd_object_id, fbe_lba_t checkpoint);
static void ugly_duckling_del_verify_invalidate_checkpoint_hook(fbe_object_id_t in_pvd_object_id, fbe_lba_t checkpoint);
static void ugly_duckling_wait_for_verify_invalidate_checkpoint_hook(fbe_object_id_t in_pvd_object_id, fbe_lba_t checkpoint);
static void ugly_duckling_get_io_configuration(ugly_duckling_test_index_t index, ugly_duckling_io_configuration_t *io_info, 
                                               ugly_duckling_io_configuration_t **io_config);
static void ugly_duckling_add_pvd_block_operation_hook(fbe_object_id_t in_pvd_object_id, fbe_payload_block_operation_opcode_t opcode);
static void ugly_duckling_del_pvd_block_operation_hook(fbe_object_id_t in_pvd_object_id, fbe_payload_block_operation_opcode_t opcode);
static void ugly_duckling_wait_for_pvd_block_operation_counter(fbe_object_id_t in_pvd_object_id, 
                                                               fbe_payload_block_operation_opcode_t opcode,
                                                               fbe_u64_t counter);
static void ugly_duckling_pull_drive(fbe_test_rg_configuration_t*        in_rg_config_p,
                                     fbe_u32_t                           in_position,
                                     fbe_api_terminator_device_handle_t* out_drive_info_p);
static void ugly_duckling_reinsert_drive(fbe_test_rg_configuration_t*        in_rg_config_p,
                                         fbe_u32_t                           in_position,
                                         fbe_api_terminator_device_handle_t* in_drive_info_p);
static void ugly_duckling_wait_for_rg_degraded(fbe_test_rg_configuration_t *current_rg_config_p, fbe_u32_t position);
static void ugly_duckling_wait_for_rg_normal(fbe_test_rg_configuration_t *current_rg_config_p, fbe_u32_t position);
static void ugly_duckling_add_pvd_non_mcr_region_permission_request_hook(fbe_object_id_t in_pvd_object_id,
                                                                         fbe_lba_t checkpoint);
static void ugly_duckling_del_pvd_non_mcr_region_permission_request_hook(fbe_object_id_t in_pvd_object_id,
                                                                         fbe_lba_t checkpoint);
static void ugly_duckling_wait_for_pvd_non_mcr_region_permission_request_counter(fbe_object_id_t in_pvd_object_id, 
                                                                                 fbe_u64_t counter,
                                                                                 fbe_lba_t checkpoint);
static fbe_status_t ugly_duckling_wait_for_zero_checkpoint(fbe_object_id_t pvd_object_id,
                                                           fbe_u32_t wait_seconds,
                                                           fbe_lba_t value);
static void ugly_duckling_add_pvd_stop_zero_completion_hook(fbe_object_id_t in_pvd_object_id);
static void ugly_duckling_del_pvd_stop_zero_completion_hook(fbe_object_id_t in_pvd_object_id);
static void ugly_duckling_wait_for_pvd_stop_zero_completion_hook(fbe_object_id_t in_pvd_object_id);
static void ugly_duckling_disable_error_injection(fbe_object_id_t in_pvd_object_id);
static void ugly_duckling_inject_error_during_bgz(fbe_object_id_t in_pvd_object_id,
                                                  fbe_u16_t error_count,
                                                  fbe_api_logical_error_injection_type_t error_type);
static void ugly_duckling_disable_protocol_error_injection(fbe_object_id_t pdo_object_id,
                                                           fbe_protocol_error_injection_record_handle_t handle_p);
static void ugly_duckling_del_pvd_zero_checkpoint_hook(fbe_object_id_t in_pvd_object_id, 
                                                       fbe_lba_t zero_checkpoint);
static void ugly_duckling_wait_for_pvd_zero_checkpoint_counter(fbe_object_id_t in_pvd_object_id, 
                                                               fbe_lba_t zero_checkpoint,
                                                               fbe_u64_t counter);
static void ugly_duckling_add_pvd_zero_checkpoint_hook(fbe_object_id_t in_pvd_object_id,
                                                       fbe_lba_t zero_checkpoint);
static void ugly_duckling_test_read_error(fbe_object_id_t in_pvd_object_id,
                                          ugly_duckling_io_configuration_t *all_io_config,
                                          fbe_test_rg_configuration_t *rg_config_ptr,
                                          fbe_u16_t error_count,
                                          fbe_bool_t b_corrupt_paged);
static void ugly_duckling_read_corrupt_paged_test(fbe_object_id_t in_pvd_object_id,
                                                  ugly_duckling_io_configuration_t *all_io_config,
                                                  fbe_test_rg_configuration_t *rg_config_ptr,
                                                  fbe_u16_t error_count);
static void ugly_duckling_perform_corrupt_paged_injection(fbe_object_id_t pvd_object_id,
                                                          fbe_lba_t lba,
                                                          fbe_u16_t err_count,
                                                          fbe_block_count_t blocks);

/*
 * GLOBAL DATA:
 */

// Error table for R1 group
ugly_duckling_media_error_case_t ugly_duckling_R1_disk_media_error_cases[] =
{
    //  test basic hard media error in metadata region during read
    {
        ugly_duckling_metadata_error_read_operation_test,
        2
    },
    // test basic hard media error in metadata region during background zeroing
    {
        ugly_duckling_metadata_error_bgz_operation_test,
        1
    },

    // test basic hard media error in metadata region during write
    {
        ugly_duckling_metadata_error_write_operation_test,
        1
    },
    // test basic hard media error in metadata region during write
    {
        ugly_duckling_metadata_error_verify_invalidate_operation_test,
        2
    },

    // test basic hard media error in metadata region during background zeroing
    {
        ugly_duckling_metadata_soft_media_bgz_operation_test,
        2
    },
    // test basic hard media error in metadata region during background zeroing
    {
        ugly_duckling_metadata_error_zero_opcode_operation_test,
        2
    },
    // test basic hard media error in metadata region during background zeroing
    {
        ugly_duckling_metadata_error_permission_request_test,
        2
    },
    // test basic logical error in metadata region during background zeroing
    {
        ugly_duckling_metadata_error_lba_checksum_error_test,
        1
    },

    // test metadata error during marking for disk zero
    {
        ugly_duckling_metadata_error_disk_zero_error_test,
        3
    },

    // test metadata retryable error during marking for disk zero
    {
        ugly_duckling_metadata_retryable_error_disk_zero_error_test,
        1
    },

    // test metadata retryable error during background zero
    {
        ugly_duckling_metadata_retryable_error_bgz_error_test,
        3
    },

    // test corrupt paged on read
    {
        ugly_duckling_read_corrupt_paged_test,
        1
    },

    {
        NULL,
        NULL
    }
};

// Error table for R5 group
ugly_duckling_media_error_case_t ugly_duckling_R5_disk_media_error_cases[] =
{
    // test basic hard media error in metadata region during read
    {
        ugly_duckling_metadata_error_read_operation_test,
        2
    },
    // test basic hard media error in metadata region during write
    {
        ugly_duckling_metadata_error_write_operation_test,
        1
    },
    {
        NULL,
        NULL
    }
};

// Error table for R0 group
ugly_duckling_media_error_case_t ugly_duckling_R0_disk_media_error_cases[] =
{
    // test basic hard media error in metadata region during read
    {
        ugly_duckling_metadata_error_read_operation_test,
        2
    },
    // test basic hard media error in metadata region during write
    {
        ugly_duckling_metadata_error_write_operation_test,
        1
    },
    {
        NULL,
        NULL
    }
};

// Error table for R3 group
ugly_duckling_media_error_case_t ugly_duckling_R3_disk_media_error_cases[] =
{
    // test basic hard media error in metadata region during read
    {
        ugly_duckling_metadata_error_read_operation_test,
        2
    },
    // test basic hard media error in metadata region during write
    {
        ugly_duckling_metadata_error_write_operation_test,
        1
    },
    {
        NULL,
        NULL
    }
};

// Error table for R6 group
ugly_duckling_media_error_case_t ugly_duckling_R6_disk_media_error_cases[] =
{
    // test basic hard media error in metadata region during read
    {
        ugly_duckling_metadata_error_read_operation_test,
        2
    },
    // test basic hard media error in metadata region during write
    {
        ugly_duckling_metadata_error_write_operation_test,
        1
    },
    {
        NULL,
        NULL
    }
};

// Error table for R10 group
ugly_duckling_media_error_case_t ugly_duckling_R10_disk_media_error_cases[] =
{
    // test basic hard media error in metadata region during read
    {
        ugly_duckling_metadata_error_read_operation_test,
        2
    },
    // test basic hard media error in metadata region during write
    {
        ugly_duckling_metadata_error_write_operation_test,
        1
    },
    {
        NULL,
        NULL
    }
};


/*!*******************************************************************
 * @var ugly_duckling_test_configuration_t
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
ugly_duckling_test_configuration_t ugly_duckling_test_config[] = 
{
    /*** NOTE: If you change the width of RG, make sure the define ugly_duckling_NUM_520_DRIVES is also changed as applicable */
    {
        "RAID 1",
        &ugly_duckling_R1_disk_media_error_cases[0],
        &ugly_duckling_io_configuration_advanced[0],
        0,
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {2,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,    FBE_CLASS_ID_MIRROR,    520,            6,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },

    {
        "RAID 3",
        &ugly_duckling_R3_disk_media_error_cases[0],
        &ugly_duckling_io_configuration_basic[0],
        1,
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,    FBE_CLASS_ID_PARITY,    520,            8,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },

    {
        "RAID 5",
        &ugly_duckling_R5_disk_media_error_cases[0],
        &ugly_duckling_io_configuration_basic[0],
        1,
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,    FBE_CLASS_ID_PARITY,    520,            0,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },
    {
        "RAID 6",
        &ugly_duckling_R6_disk_media_error_cases[0],
        &ugly_duckling_io_configuration_basic[0],
        2,
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,    FBE_CLASS_ID_PARITY,    520,            2,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },

    {
        "RAID 1_0",
        &ugly_duckling_R10_disk_media_error_cases[0],
        &ugly_duckling_io_configuration_basic[0],
        0,
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {4,        0x2a000,    FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            10,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },
    {
        "RAID 0",
        &ugly_duckling_R0_disk_media_error_cases[0],
        &ugly_duckling_io_configuration_basic[0],
        1,
        {
            /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,    FBE_CLASS_ID_STRIPER,    520,            4,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        }
    },
    {
        /* Terminator */
        NULL, NULL, NULL, NULL
    },
};



/*
 * TEST DESCRIPTION:
 */

char* ugly_duckling_short_desc = "PVD Metadata Error injection during Zeroing";
char* ugly_duckling_long_desc =
    "\n"
    "\n"
    "The Ugly Duckling Scenario tests the handling of Metadata Errors during user operation\n"
    " while zeroing is going on\n"
    "\n"
    "It covers the all the block opcode that the PVD receives\n"
    "\n"
    "Dependencies: \n"
    "    - NEIT must be able to inject hard and soft media errors on PVD objects\n"
    "\n"
    "Starting Config: \n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 1 520 SAS drive (PDO-B)\n"
    "    [PP] 2 logical drives (LDO-A, LDO-B)\n"
    "    [SEP] 2 provision drives (PVD-A, PVD-B)\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package configuration\n"
    "    - Create all provision drives (PVD) with an I/O edge attached to a logical drive (LD)\n"
    "\n"
    "STEP 2: Set-up NEIT to inject media errors on targeted 520 SAS drive (PVD-B)\n"
    "    - Create error injection record for each media error test case\n"
    "    - Set-up edge hook to inject media errors on selected drive\n"
    "    - Enable error injection on selected drive\n"
    "\n"
    "STEP 3: Run the appropriate user operation on targeted 520 SAS drive (PVD-B)\n"
    "    - Based on the operation that is requested by the user, validate that\n"
    "    - metadata error is handled correctly \n"
    "\n"
    "STEP 4: Disable error injection on targeted 520 SAS drive (PVD-B)\n"
    "    - Remove edge hook and disable error injection on selected drive\n"
    "    - Check that collected error injection statistics are as expected\n" 
    "\n";


/*!****************************************************************************
 *  ugly_duckling_metadata_error_write_operation_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate the handling of metadata error during write operation
 * when Zeroing is in progress. So the idea is to write invalid pattern for the
 * region not covered by the IO and send a remap required to RAID to perform
 * error verify of the region that was invalidated and kick off verify invalidate 
 * to check if there are other region that needs to be invalidated
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Write invalidate pattern to the edges not covered by the IO
 *          2. Remap required status is sent to raid which makes RAID mark for Error
 *             verify
 *          3. Verify Invalidate operation is kicked off to check the other regions
 * @author
 *    03/17/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_metadata_error_write_operation_test(fbe_object_id_t  in_pvd_object_id,
                                                  ugly_duckling_io_configuration_t *all_io_config,
                                                  fbe_test_rg_configuration_t *rg_config_ptr,
                                                  fbe_u16_t error_count)
{
    fbe_status_t status;
    fbe_object_id_t raid_group_object_id = FBE_OBJECT_ID_INVALID;        
    fbe_lba_t                       rounded_lba;
    fbe_lba_t                       pre_start_lba;
    fbe_block_count_t               pre_block_count;
    fbe_lba_t                       post_start_lba;
    fbe_block_count_t               post_block_count;
    fbe_api_provision_drive_info_t  provision_drive_info;
    ugly_duckling_io_profile_t *io_profile = NULL;
    ugly_duckling_io_configuration_t *io_config = NULL;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};

    ugly_duckling_get_io_configuration(UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_WRITE_IO_TEST, all_io_config, &io_config);

    /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(in_pvd_object_id, 
                                              &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== inject paged metadata errors on pvd obj: 0x%x meta lba: 0x%llx blocks: %d\n",
               in_pvd_object_id, provision_drive_info.paged_metadata_lba, 1);

    io_profile = &io_config->rdgen_io_config[0];

    /*! @note When the lba to inject to is invlaid we are done with all the error cases.
     */
    while(io_profile->start_lba != FBE_LBA_INVALID)
    {

        /* First disable all background operations  for this tests. */
        ugly_duckling_disable_background_operations(in_pvd_object_id);

        ugly_duckling_perform_error_injection(in_pvd_object_id, 
                                              provision_drive_info.paged_metadata_lba, 
                                              error_count,
                                              FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                                              1);

        /* Need to clear the MD cache so that the read or write operation goes to the drive so
         * that error gets injected
         */
        status = fbe_api_base_config_metadata_paged_clear_cache(in_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_provision_drive_set_zero_checkpoint(in_pvd_object_id, provision_drive_info.default_offset);

        fbe_test_get_rg_object_ids(rg_config_ptr, &rg_object_ids[0]);

        raid_group_object_id = rg_object_ids[0];

        /* Since the background processing is always aligned on the chunk, get the start LBA of the chunk this IO LBA is part of
         */
        rounded_lba = (io_profile->start_lba / UGLY_DUCKLING_CHUNK_SIZE) * UGLY_DUCKLING_CHUNK_SIZE;

        ugly_duckling_add_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);

        /* Need to clear the MD cache so that the read or write operation goes to the drive so
         * that error gets injected
         */
        status = fbe_api_base_config_metadata_paged_clear_cache(in_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if((io_profile->rdgen_operation != FBE_RDGEN_OPERATION_INVALID) &&
           (io_profile->start_lba != FBE_LBA_INVALID))
        {
            // Perform the requested I/O and make sure there were no errors
            status = fbe_api_rdgen_send_one_io(&test_contexts[0],
                                               raid_group_object_id,
                                               FBE_CLASS_ID_INVALID,
                                               FBE_PACKAGE_ID_SEP_0,
                                               io_profile->rdgen_operation,
                                               io_profile->rdgen_pattern,
                                               io_profile->start_lba,
                                               io_profile->block_count,
                                               FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                               FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        }

        /* Make sure RAID marks the LBA for Error verify because PVD would have invalidated */
        ugly_duckling_wait_for_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);
        
        /* Get the edge chunks that is not covered by IO. These should have been invalidated */
        ugly_duckling_get_expected_invalid_lba_block_count(io_profile->start_lba,
                                                           io_profile->block_count,
                                                           &pre_start_lba, &pre_block_count,
                                                           &post_start_lba, &post_block_count);

        pre_start_lba += provision_drive_info.default_offset;
        post_start_lba += provision_drive_info.default_offset;

        /* Validate that the invalid pattern is really getting written to the region not covered by write IO
         */
        status = ugly_duckling_check_for_invalid_pattern(in_pvd_object_id,
                                                         pre_start_lba, pre_block_count,
                                                         post_start_lba, post_block_count);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        ugly_duckling_del_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);

        /* Wait for the error verify to complete*/
        status = ugly_duckling_wait_for_error_verify_checkpoint(raid_group_object_id, FBE_LBA_INVALID);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Wait for the verify invalidate to complete */
        status = ugly_duckling_wait_for_verify_invalidate_checkpoint(in_pvd_object_id, UGLY_DUCKLING_CHECKPOINT_WAIT_IN_SEC, FBE_LBA_INVALID);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        io_profile++;
        ugly_duckling_disable_error_injection(in_pvd_object_id);
    }
} // end ugly_duckling_metadata_error_write_operation_test()

/*!****************************************************************************
 *  ugly_duckling_metadata_error_read_operation_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate the handling of metadata error during read operation
 * when Zeroing is in progress. Since we cannot read the data, return an error
 * back to the client and kick off verify invalidate 
 * to check if there are other region that needs to be invalidated
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Verify Invalidate operation is kicked off to check the other regions
 *          2. RAID marks the region for Error Verify 
 * @author
 *    03/17/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_metadata_error_read_operation_test(fbe_object_id_t                                 in_pvd_object_id,
                                                 ugly_duckling_io_configuration_t *all_io_config,
                                               fbe_test_rg_configuration_t *rg_config_ptr,
                                               fbe_u16_t error_count)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t raid_group_object_id = FBE_OBJECT_ID_INVALID; 
    fbe_api_provision_drive_info_t provision_drive_info;
    ugly_duckling_io_profile_t *io_profile = NULL;
    ugly_duckling_io_configuration_t *io_config = NULL;
    fbe_lba_t                       rounded_lba;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Read Operation test.....\n");

    ugly_duckling_get_io_configuration(UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_READ_IO_TEST, all_io_config, &io_config);

    io_profile = &io_config->rdgen_io_config[0];
    
    /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(in_pvd_object_id, 
                                              &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* First disable all background operations  for this tests. */
    ugly_duckling_disable_background_operations(in_pvd_object_id);

    ugly_duckling_perform_error_injection(in_pvd_object_id, provision_drive_info.paged_metadata_lba,
                                          error_count,
                                          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                                          1);

    status = fbe_api_base_config_metadata_paged_clear_cache(in_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_set_zero_checkpoint(in_pvd_object_id, provision_drive_info.default_offset);

     fbe_test_get_rg_object_ids(rg_config_ptr, &rg_object_ids[0]);

     raid_group_object_id = rg_object_ids[0];

    ugly_duckling_add_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);
    
    /* Since the background processing is always aligned on the chunk, get the start LBA of the chunk this IO LBA is part of
     */
    rounded_lba = (io_profile->start_lba / UGLY_DUCKLING_CHUNK_SIZE) * UGLY_DUCKLING_CHUNK_SIZE;

    ugly_duckling_add_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);

    if (rg_config_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID1 || 
        rg_config_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* Set mirror perfered position to disable mirror read optimization */
        status = fbe_api_raid_group_set_mirror_prefered_position(raid_group_object_id, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    if((io_profile->rdgen_operation != FBE_RDGEN_OPERATION_INVALID) &&
       (io_profile->start_lba != FBE_LBA_INVALID))
    {
        // Perform the requested I/O and make sure there were no errors
        status = fbe_api_rdgen_send_one_io(&test_contexts[0],
                                           raid_group_object_id,
                                           FBE_CLASS_ID_INVALID,
                                           FBE_PACKAGE_ID_SEP_0,
                                           io_profile->rdgen_operation,
                                           io_profile->rdgen_pattern,
                                           io_profile->start_lba,
                                           io_profile->block_count,
                                           FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    }

    if (rg_config_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID1 || 
        rg_config_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* Set mirror perfered position to enable mirror read optimization */
        status = fbe_api_raid_group_set_mirror_prefered_position(raid_group_object_id, 0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Make sure RAID marks the LBA for Error verify because PVD would have invalidated */
    ugly_duckling_wait_for_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);

    ugly_duckling_wait_for_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);

    /* Now disable the error injection and let the verify invalidate algorithm run through */
    ugly_duckling_disable_error_injection(in_pvd_object_id);

    ugly_duckling_del_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);

    ugly_duckling_del_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);

    /* Wait for the error verify to complete*/
    status = ugly_duckling_wait_for_error_verify_checkpoint(raid_group_object_id, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = ugly_duckling_wait_for_verify_invalidate_checkpoint(in_pvd_object_id, UGLY_DUCKLING_CHECKPOINT_WAIT_IN_SEC, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

} // end ugly_duckling_metadata_error_read_operation_test()
/*!****************************************************************************
 *  ugly_duckling_read_corrupt_paged_test_case
 ******************************************************************************
 *
 * @brief
 *    This tests validate the handling of metadata error during read operation
 * when Zeroing is in progress. Since we cannot read the data, return an error
 * back to the client and kick off verify invalidate 
 * to check if there are other region that needs to be invalidated
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Verify Invalidate operation is kicked off to check the other regions
 *          2. RAID marks the region for Error Verify 
 * @author
 *  3/22/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static void 
ugly_duckling_read_corrupt_paged_test_case(fbe_object_id_t in_pvd_object_id,
                                           ugly_duckling_io_configuration_t *all_io_config,
                                           fbe_test_rg_configuration_t *rg_config_ptr,
                                           fbe_u16_t error_count,
                                           ugly_duckling_test_index_t io_test_index)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t raid_group_object_id = FBE_OBJECT_ID_INVALID; 
    fbe_api_provision_drive_info_t provision_drive_info;
    ugly_duckling_io_profile_t *io_profile = NULL;
    ugly_duckling_io_configuration_t *io_config = NULL;
    fbe_lba_t                       rounded_lba;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_api_trace_error_counters_t error_counters;

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Read Operation test.....\n");

    fbe_test_get_rg_object_ids(rg_config_ptr, &rg_object_ids[0]);

    raid_group_object_id = rg_object_ids[0];
    ugly_duckling_get_io_configuration(io_test_index, all_io_config, &io_config);

    io_profile = &io_config->rdgen_io_config[0];

    /* First seed the area with a good pattern.
     */
    status = fbe_api_rdgen_send_one_io(&test_contexts[0],
                                       raid_group_object_id,
                                       FBE_CLASS_ID_INVALID,
                                       FBE_PACKAGE_ID_SEP_0,
                                       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                       FBE_RDGEN_PATTERN_ZEROS,
                                       io_profile->start_lba,
                                       io_profile->block_count,
                                       FBE_RDGEN_OPTIONS_INVALID, 0, 0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(in_pvd_object_id, 
                                              &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* First disable all background operations  for this tests. */
    ugly_duckling_disable_background_operations(in_pvd_object_id);

    /* Make sure we don't panic on critical errors since we 
     * actually expect one due to the corrupt paged. 
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE,
                                           1,    /* Num errors. */
                                           FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    ugly_duckling_perform_corrupt_paged_injection(in_pvd_object_id, provision_drive_info.paged_metadata_lba,
                                                  error_count,
                                                  1);

    status = fbe_api_base_config_metadata_paged_clear_cache(in_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_set_zero_checkpoint(in_pvd_object_id, provision_drive_info.default_offset);

    ugly_duckling_add_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);

    /* Since the background processing is always aligned on the chunk, get the start LBA of the chunk this IO LBA is part of
     */
    rounded_lba = (io_profile->start_lba / UGLY_DUCKLING_CHUNK_SIZE) * UGLY_DUCKLING_CHUNK_SIZE;

    ugly_duckling_add_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);

    if (rg_config_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID1 || 
        rg_config_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* Set mirror perfered position to disable mirror read optimization */
        status = fbe_api_raid_group_set_mirror_prefered_position(raid_group_object_id, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    if((io_profile->rdgen_operation != FBE_RDGEN_OPERATION_INVALID) &&
       (io_profile->start_lba != FBE_LBA_INVALID))
    {
        // Perform the requested I/O and make sure there were no errors
        status = fbe_api_rdgen_send_one_io(&test_contexts[0],
                                           raid_group_object_id,
                                           FBE_CLASS_ID_INVALID,
                                           FBE_PACKAGE_ID_SEP_0,
                                           io_profile->rdgen_operation,
                                           io_profile->rdgen_pattern,
                                           io_profile->start_lba,
                                           io_profile->block_count,
                                           FBE_RDGEN_OPTIONS_INVALID, 0, 0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    }
    /* Make sure the I/O saw the media error and reconstructed successfully.
     */
    MUT_ASSERT_INT_EQUAL(test_contexts[0].start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(test_contexts[0].start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(test_contexts[0].start_io.status.status, FBE_STATUS_OK);

    if (io_test_index == UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_READ_IO_TEST)
    {
        MUT_ASSERT_INT_NOT_EQUAL(test_contexts[0].start_io.statistics.verify_errors.u_media_count, 0);
    }

    status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
    MUT_ASSERT_INT_NOT_EQUAL(error_counters.trace_critical_error_counter, 0);
    MUT_ASSERT_INT_EQUAL(error_counters.trace_error_counter, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "== validate error counters cerr: 0x%x err: 0x%x",
               error_counters.trace_critical_error_counter,
               error_counters.trace_error_counter);
    if (rg_config_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID1 || 
        rg_config_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* Set mirror perfered position to enable mirror read optimization */
        status = fbe_api_raid_group_set_mirror_prefered_position(raid_group_object_id, 0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Make sure RAID marks the LBA for Error verify because PVD would have invalidated */
    ugly_duckling_wait_for_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);

    ugly_duckling_wait_for_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);

    /* Now disable the error injection and let the verify invalidate algorithm run through */
    ugly_duckling_disable_error_injection(in_pvd_object_id);

    ugly_duckling_del_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);

    ugly_duckling_del_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);

    /* Wait for the error verify to complete*/
    status = ugly_duckling_wait_for_error_verify_checkpoint(raid_group_object_id, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = ugly_duckling_wait_for_verify_invalidate_checkpoint(in_pvd_object_id, UGLY_DUCKLING_CHECKPOINT_WAIT_IN_SEC, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We previously suppressed panic for critical errors since we expected it to occur.
     * Restore our trace level. 
     */
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID,    
                                           1,    /* Num errors. */
                                           FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

} // end ugly_duckling_read_corrupt_paged_test_case()
/*!****************************************************************************
 *  ugly_duckling_read_corrupt_paged_test
 ******************************************************************************
 *
 * @brief
 *    This tests the cases where paged is corrupted on a read, write or zero.
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Verify Invalidate operation is kicked off to check the other regions
 *          2. RAID marks the region for Error Verify 
 * @author
 *  3/22/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static void 
ugly_duckling_read_corrupt_paged_test(fbe_object_id_t in_pvd_object_id,
                                      ugly_duckling_io_configuration_t *all_io_config,
                                      fbe_test_rg_configuration_t *rg_config_ptr,
                                      fbe_u16_t error_count)
{
    ugly_duckling_read_corrupt_paged_test_case(in_pvd_object_id, all_io_config, rg_config_ptr, error_count, 
                                               UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_READ_IO_TEST);

    ugly_duckling_read_corrupt_paged_test_case(in_pvd_object_id, all_io_config, rg_config_ptr, error_count, 
                                               UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_WRITE_IO_TEST);
} // end ugly_duckling_read_corrupt_paged_test()

/*!****************************************************************************
 *  ugly_duckling_metadata_error_bgz_operation_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate the handling of metadata error during background 
 * zeroing operation. Since we cannot read the MD data, cannot determine the
 * state of the media so kick off verify invalidate operation
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Verify Invalidate operation is kicked off to check the other regions
 * @author
 *    03/17/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_metadata_error_bgz_operation_test(fbe_object_id_t in_pvd_object_id,
                                                ugly_duckling_io_configuration_t *all_io_config,
                                                fbe_test_rg_configuration_t *rg_config_ptr,
                                                fbe_u16_t error_count)
{
    mut_printf(MUT_LOG_TEST_STATUS, "Starting Background Zeroing Operation test...\n");

    ugly_duckling_inject_error_during_bgz(in_pvd_object_id,
                                          error_count, 
                                          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR);
    return;

} // end ugly_duckling_metadata_error_bgz_operation_test()

/*!****************************************************************************
 *  ugly_duckling_metadata_soft_media_bgz_operation_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate the handling of correctable Metadata Errors where the
 * data is received but need to perform a write verify on the next write so that
 * the drive performs remaps
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Write verify is performed on a correctable errors
 * @author
 *    03/17/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_metadata_soft_media_bgz_operation_test(fbe_object_id_t  in_pvd_object_id,
                                                     ugly_duckling_io_configuration_t *all_io_config,
                                               fbe_test_rg_configuration_t *rg_config_ptr,
                                               fbe_u16_t error_count)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_bool_t  is_enabled;
    fbe_lba_t  zero_checkpoint;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t provision_drive_info;

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Soft Media Operation test...\n");

    /* First disable all background operations  for this tests. */
    ugly_duckling_disable_background_operations(in_pvd_object_id);

    /* add a hook to make sure we get write verify on MD region */
    ugly_duckling_add_pvd_block_operation_hook(in_pvd_object_id, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY);

    /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(in_pvd_object_id, 
                                              &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    ugly_duckling_perform_error_injection(in_pvd_object_id, 
                                          provision_drive_info.paged_metadata_lba,
                                          error_count,
                                          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR,
                                          1);

    status = fbe_api_base_config_metadata_paged_clear_cache(in_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_lookup_lun_by_number(rg_config_ptr->logical_unit_configuration_list[0].lun_number, 
                                                   &lun_object_id);

    /* Initiate an operation that perform MD read which will get correctable errors */
    status = fbe_api_lun_initiate_zero(lun_object_id);
    status = fbe_api_provision_drive_set_zero_checkpoint(in_pvd_object_id, provision_drive_info.default_offset);
    status = fbe_test_sep_util_provision_drive_enable_zeroing(in_pvd_object_id);

    fbe_api_base_config_is_background_operation_enabled(in_pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING,
                                                        &is_enabled);
    MUT_ASSERT_INT_EQUAL(is_enabled, 1);

    /* make sure the operation is really progressing */
    status =fbe_api_provision_drive_get_zero_checkpoint(in_pvd_object_id, &zero_checkpoint);
    MUT_ASSERT_UINT64_NOT_EQUAL (zero_checkpoint, FBE_LBA_INVALID);
    MUT_ASSERT_UINT64_NOT_EQUAL (zero_checkpoint, 0);

    /* Check if our hook got hit by checking the counter */
    ugly_duckling_wait_for_pvd_block_operation_counter(in_pvd_object_id, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY, 1);
    
    ugly_duckling_del_pvd_block_operation_hook(in_pvd_object_id, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    ugly_duckling_disable_error_injection(in_pvd_object_id);
    return;
} // end ugly_duckling_metadata_soft_media_bgz_operation_test()

/*!****************************************************************************
 *  ugly_duckling_metadata_error_zero_opcode_operation_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate the handling of Metadata Errors when zero opcode is 
 * received 
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Verify Invalidate operation is kicked off to check the other regions
 * @author
 *    03/17/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_metadata_error_zero_opcode_operation_test(fbe_object_id_t  in_pvd_object_id,
                                               ugly_duckling_io_configuration_t *io_config,
                                               fbe_test_rg_configuration_t *rg_config_ptr,
                                               fbe_u16_t error_count)

{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_bool_t                      is_enabled;
    fbe_lba_t                       zero_checkpoint;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_object_id_t                 lun_object_id = FBE_OBJECT_ID_INVALID;

    /* Get the raid group object id so that we can check for the verify completion.
     * Get the raid group info so that we can write full stripes.
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_ptr->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Zero Operation test...\n");

    /* First disable all background operations  for this tests. */
    ugly_duckling_disable_background_operations(in_pvd_object_id);

     /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(in_pvd_object_id, 
                                              &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    ugly_duckling_add_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);

    ugly_duckling_perform_error_injection(in_pvd_object_id, 
                                          provision_drive_info.paged_metadata_lba,
                                          error_count,
                                          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                                          1);

    status = fbe_api_base_config_metadata_paged_clear_cache(in_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_lookup_lun_by_number(rg_config_ptr->logical_unit_configuration_list[0].lun_number, 
                                                               &lun_object_id);
    status = fbe_api_lun_initiate_zero(lun_object_id);
    status = fbe_api_provision_drive_set_zero_checkpoint(in_pvd_object_id, provision_drive_info.default_offset);
    status = fbe_test_sep_util_provision_drive_enable_zeroing(in_pvd_object_id);

    fbe_api_base_config_is_background_operation_enabled(in_pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING,
                                                        &is_enabled);
    MUT_ASSERT_INT_EQUAL(is_enabled, 1);

    status =fbe_api_provision_drive_get_zero_checkpoint(in_pvd_object_id, &zero_checkpoint);
    MUT_ASSERT_UINT64_NOT_EQUAL (zero_checkpoint, FBE_LBA_INVALID);
    MUT_ASSERT_UINT64_NOT_EQUAL (zero_checkpoint, 0);

    ugly_duckling_wait_for_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);

    /* Step 4b: Set a debug hook so that we don't miss the verify start.
     */
    status = fbe_test_add_debug_hook_active(rg_object_id, 
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    ugly_duckling_del_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);

    status = ugly_duckling_wait_for_verify_invalidate_checkpoint(in_pvd_object_id, UGLY_DUCKLING_CHECKPOINT_WAIT_IN_SEC, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    ugly_duckling_disable_error_injection(in_pvd_object_id);

    /* Step 5a: Wait for the raid group to start the verify.
     *          Due to the loss of the paged metadata.
     */ 
    status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                 FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START,
                                                 SCHEDULER_CHECK_STATES, 
                                                 SCHEDULER_DEBUG_ACTION_PAUSE, 
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Should not see any sectors traced.
     */
    status = fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_TRUE /* Stop the system on error*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 5c: Remove the raid group debug hook.
     */
    status = fbe_test_del_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);  
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 7: Wait for the R/W Verify to complete.
     */
    status = fbe_test_verify_wait_for_verify_completion(rg_object_id, FBE_TEST_WAIT_TIMEOUT_MS, FBE_LUN_VERIFY_TYPE_SYSTEM);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Other test may generate sector traces so clear stop on error.
     */
    status = fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_FALSE /* Don't stop on error*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    return;
} // end ugly_duckling_metadata_error_zero_opcode_operation_test()


/*!****************************************************************************
 *  ugly_duckling_metadata_error_verify_invalidate_operation_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate the handling of verify invalidate operation
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Verify Invaldiate code reads the invalid paged bit and writes 
 *             invalid pattern
 *          2. Invalid pattern is really getting written
 *          3. Remap Event is sent to RAID to reconstruct the region
 *          4. Degraded RG prevent invalidate operation from progressing
 *          5. Verify invalidate operation is complete
 * 
 * @author
 *    03/17/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_metadata_error_verify_invalidate_operation_test(fbe_object_id_t                                 in_pvd_object_id,
                                                              ugly_duckling_io_configuration_t *all_io_config,
                                               fbe_test_rg_configuration_t *rg_config_ptr,
                                               fbe_u16_t error_count)

{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t raid_group_object_id = FBE_OBJECT_ID_INVALID;        
    fbe_lba_t                       checkpoint;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_lba_t                       prev_checkpoint;
    ugly_duckling_io_profile_t *io_profile = NULL;
    fbe_u32_t chunk_index;
    fbe_lba_t   start_lba_for_checking;
    ugly_duckling_io_configuration_t *io_config = NULL;
    fbe_lba_t verify_invalidate_checkpoint;
    fbe_lba_t  current_checkpoint;
    fbe_lba_t                       rounded_lba;
    fbe_api_terminator_device_handle_t drive_info;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Verify Invaldiate Algorithm test...\n");

    ugly_duckling_get_io_configuration(UGLY_DUCKLING_TEST_VERIFY_INVALIDATE_IO_TEST, all_io_config, &io_config);

    io_profile = &io_config->rdgen_io_config[0];

    /* First disable all background operations  for this tests. */
    ugly_duckling_disable_background_operations(in_pvd_object_id);

     /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(in_pvd_object_id, 
                                              &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    ugly_duckling_perform_error_injection(in_pvd_object_id, 
                                          provision_drive_info.paged_metadata_lba,
                                          error_count,
                                          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                                          1);

    status = fbe_api_base_config_metadata_paged_clear_cache(in_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_set_zero_checkpoint(in_pvd_object_id, provision_drive_info.default_offset);

     fbe_test_get_rg_object_ids(rg_config_ptr, &rg_object_ids[0]);

     raid_group_object_id = rg_object_ids[0];

    /* As part of IO we would have already invalidate the chunk and so ignore that by moving the initiate checkpoint*/
    start_lba_for_checking = provision_drive_info.default_offset + UGLY_DUCKLING_CHUNK_SIZE;
    ugly_duckling_add_verify_invalidate_checkpoint_hook(in_pvd_object_id, start_lba_for_checking);

    /* Since the background processing is always aligned on the chunk, get the start LBA of the chunk this IO LBA is part of
     */
    rounded_lba = (io_profile->start_lba / UGLY_DUCKLING_CHUNK_SIZE) * UGLY_DUCKLING_CHUNK_SIZE;
    ugly_duckling_add_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);

    if ((io_profile->rdgen_operation != FBE_RDGEN_OPERATION_INVALID) &&
        (io_profile->start_lba != FBE_LBA_INVALID))
    {
        // Perform the requested I/O and make sure there were no errors
        status = fbe_api_rdgen_send_one_io(&test_contexts[0],
                                           raid_group_object_id,
                                           FBE_CLASS_ID_INVALID,
                                           FBE_PACKAGE_ID_SEP_0,
                                           io_profile->rdgen_operation,
                                           io_profile->rdgen_pattern,
                                           io_profile->start_lba,
                                           io_profile->block_count,
                                           FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    }

    ugly_duckling_wait_for_verify_invalidate_checkpoint_hook(in_pvd_object_id,start_lba_for_checking);
    ugly_duckling_wait_for_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);

    checkpoint = start_lba_for_checking;

    /* Just validate for some of the chunks to verify that invalidate pattern really got written and the
     * event is sent to RAID above to perform error verify
     */
    for(chunk_index = 1; chunk_index < 4; chunk_index ++)
    {
        prev_checkpoint = checkpoint;
        checkpoint += UGLY_DUCKLING_CHUNK_SIZE;

        ugly_duckling_add_verify_invalidate_checkpoint_hook(in_pvd_object_id, checkpoint);
        ugly_duckling_del_verify_invalidate_checkpoint_hook(in_pvd_object_id, prev_checkpoint);
        ugly_duckling_wait_for_verify_invalidate_checkpoint_hook(in_pvd_object_id,checkpoint);

        ugly_duckling_validate_pattern(in_pvd_object_id, prev_checkpoint, UGLY_DUCKLING_CHUNK_SIZE);

    }
    
    ugly_duckling_del_verify_invalidate_checkpoint_hook(in_pvd_object_id, checkpoint);

    
    /* Need to make sure on a degraded raid group, the metadata verify checkpoint does not move
     * until the RG has all the drives in ready state
     * Degrade the raid group 
     * Wait for the RG to go to degraded 
     * get the current verify invalidate checkpoint 
     * Wait for 15 seconds 
     * Make sure the verify invalidate checkpoint has not moved 
     * Reinsert the drive 
     * Wait for 15 seconds 
     * Make sure the verify invalidate checkpoint now is moving 
     */
    ugly_duckling_pull_drive(rg_config_ptr, 1, &drive_info);
    ugly_duckling_wait_for_rg_degraded(rg_config_ptr, 1);
    fbe_api_sleep(15000);
    status = fbe_api_provision_drive_get_verify_invalidate_checkpoint(in_pvd_object_id, &verify_invalidate_checkpoint);
    fbe_api_sleep(15000);
    status = fbe_api_provision_drive_get_verify_invalidate_checkpoint(in_pvd_object_id, &current_checkpoint);
    MUT_ASSERT_UINT64_EQUAL_MSG(verify_invalidate_checkpoint, current_checkpoint, "The checkpoint has moved which is unexpected\n");
    ugly_duckling_reinsert_drive(rg_config_ptr, 1, &drive_info);
    ugly_duckling_wait_for_rg_normal(rg_config_ptr,1 );

    /* Remove the hook and Wait for the error verify to complete*/
    ugly_duckling_del_error_verify_checkpoint_hook(raid_group_object_id, rounded_lba);

    mut_printf(MUT_LOG_TEST_STATUS, "Waiting for checkpoint to be invalid. This will take a while, please be patient....\n");
    status = ugly_duckling_wait_for_verify_invalidate_checkpoint(in_pvd_object_id, UGLY_DUCKLING_CHECKPOINT_WAIT_IN_SEC * 5, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = ugly_duckling_wait_for_error_verify_checkpoint(raid_group_object_id, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    ugly_duckling_disable_error_injection(in_pvd_object_id);

    return;
} // end ugly_duckling_metadata_error_verify_invalidate_operation_test()

/*!****************************************************************************
 *  ugly_duckling_metadata_error_permission_request_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate that on verify invalidate operation we get permission
 * from the upper objects and no operation is performed on the regions not 
 * controlled by MCR
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Non-MCR region are ignored which is known when permission request
 *             is sent
 * 
 * @author
 *    03/17/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_metadata_error_permission_request_test(fbe_object_id_t in_pvd_object_id,
                                                     ugly_duckling_io_configuration_t *io_config,
                                                     fbe_test_rg_configuration_t *rg_config_ptr,
                                                     fbe_u16_t error_count)

{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_object_id_t pvd_id = FBE_OBJECT_ID_INVALID;
    fbe_private_space_layout_region_id_t region_id; 
    fbe_private_space_layout_region_t region_info;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_lba_t                   curr_zero_checkpoint;
  //  fbe_api_base_config_metadata_paged_change_bits_t paged_change_bits;

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Permission Request test...\n");

    status = fbe_api_provision_drive_get_obj_id_by_location(0,0,0, &pvd_id);

    /* First disable all background operations  for this tests. */
    /* we already disable zeroing in the setup */
    status = fbe_test_sep_util_provision_drive_disable_verify (pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

     /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(pvd_id, 
                                              &provision_drive_info);
   
    ugly_duckling_perform_error_injection(pvd_id, 
                                          provision_drive_info.paged_metadata_lba,
                                          error_count,
                                          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                                          0x1000);


    /* We should never be issueing IO to region that are not controlled by MCR
     * Set hooks to validate that no verify invalidate operation is done to 
     * those regions. 
     */

    /* Add debug hooks to make sure we dont take action on non-mcr region 
     * Loop through the PSL to add debug hooks for the region not controlled
     * by MCR
     */
    for(region_id = 1; region_id < FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_LAST; region_id++)
    {
        fbe_private_space_layout_get_region_by_region_id(region_id,
                                                         &region_info);
        if(region_info.region_type != FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Adding debug hook for region ID:%d....\n", region_id);
            ugly_duckling_add_pvd_non_mcr_region_permission_request_hook(pvd_id, 
                                                                         region_info.starting_block_address);
        }
    }

#if 0 /* Disable for now since it takes a LOOOOOOOONG time and the private LUNs are not aligned for now*/
    paged_change_bits.metadata_offset = 0;
    paged_change_bits.metadata_record_data_size = 2;
    /* Corrupt the whole MD: i.e No. of bytes in a Block * chunk size * no. of paged chunks / size of provision drive metadata */
    paged_change_bits.metadata_repeat_count = 512 * UGLY_DUCKLING_CHUNK_SIZE * 2 / 2 ; 
    paged_change_bits.metadata_repeat_offset = 0;

    * (fbe_u32_t *)paged_change_bits.metadata_record_data = 0xFFFFFFFF;
    status = fbe_api_base_config_metadata_paged_clear_bits(pvd_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
#endif 
    status = fbe_api_base_config_metadata_paged_clear_cache(pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(pvd_id, 
                                              &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    ugly_duckling_add_verify_invalidate_checkpoint_hook(pvd_id, 0);

    status = fbe_api_provision_drive_get_zero_checkpoint(pvd_id, &curr_zero_checkpoint);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Permission Request test, background zero checkpoint 0x%llx, pvd 0x%x\n", curr_zero_checkpoint, pvd_id);
    
    status = fbe_test_sep_util_provision_drive_enable_zeroing(pvd_id);

    ugly_duckling_wait_for_verify_invalidate_checkpoint_hook(pvd_id, 0);

    ugly_duckling_del_verify_invalidate_checkpoint_hook(pvd_id, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "Waiting for checkpoint to be invalid. This will take a while, please be patient....\n");

    status = ugly_duckling_wait_for_verify_invalidate_checkpoint(pvd_id, UGLY_DUCKLING_CHECKPOINT_WAIT_IN_SEC * 5, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Validate that we have not done anything to the Non-MCR region
     */
    for(region_id = 1; region_id < FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_LAST; region_id++)
    {
        fbe_private_space_layout_get_region_by_region_id(region_id,
                                                         &region_info);
        if(region_info.region_type != FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Waiting for debug hook for region ID:%d....\n", region_id);
            ugly_duckling_wait_for_pvd_non_mcr_region_permission_request_counter(pvd_id, 
                                                                                 0,
                                                                                 region_info.starting_block_address);
            ugly_duckling_del_pvd_non_mcr_region_permission_request_hook(pvd_id, 
                                                                         region_info.starting_block_address);
        }
    }

    ugly_duckling_disable_error_injection(pvd_id);

    return;
} // end ugly_duckling_metadata_error_permission_request_test()

/*!****************************************************************************
 *  ugly_duckling_metadata_error_lba_checksum_error_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate the handling of metadata error during background 
 * zeroing operation. This test injects logical error to make sure they are handled
 * correctly
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Verify Invalidate operation is kicked off to for CRC and LBA stamp errors(Similar
 *          to hard media errors this is a data loss situation)
 *
 * @author
 *    05/30/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_metadata_error_lba_checksum_error_test(fbe_object_id_t in_pvd_object_id,
                                                     ugly_duckling_io_configuration_t *all_io_config,
                                                     fbe_test_rg_configuration_t *rg_config_ptr,
                                                     fbe_u16_t error_count)
{
    mut_printf(MUT_LOG_TEST_STATUS, "Starting LBA stamp Error test...\n");
    ugly_duckling_inject_error_during_bgz(in_pvd_object_id,
                                          error_count,
                                          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting CRC Error test...\n");

    ugly_duckling_inject_error_during_bgz(in_pvd_object_id,
                                          error_count,
                                          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_CRC);

    return;

} // end ugly_duckling_metadata_error_lba_checksum_error_test()

/*!****************************************************************************
 *  ugly_duckling_metadata_error_disk_zero_error_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate the handling of metadata error when disk zero command
 * is initiated which forces a write to the metadata region
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Verify Invalidate operation is kicked off and disk zero if marked as
 *     expected
 *
 * @author
 *    05/30/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_metadata_error_disk_zero_error_test(fbe_object_id_t in_pvd_object_id,
                                                  ugly_duckling_io_configuration_t *all_io_config,
                                                  fbe_test_rg_configuration_t *rg_config_ptr,
                                                  fbe_u16_t error_count)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_object_id_t unused_pvd_object_id = FBE_OBJECT_ID_INVALID;


    status = fbe_test_sep_util_get_any_unused_pvd_object_id(&unused_pvd_object_id);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to find unused PVD\n");

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Disk Zero Error test PVD %d\n", unused_pvd_object_id);


    /* First disable all background operations  for this tests. */
    ugly_duckling_disable_background_operations(unused_pvd_object_id);

    /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(unused_pvd_object_id, 
                                              &provision_drive_info);

    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Get Info for PVD failed\n");

    ugly_duckling_perform_error_injection(unused_pvd_object_id, 
                                          provision_drive_info.paged_metadata_lba,
                                          error_count,
                                          FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                                          1);

    /* Add hooks to make sure Verify invalidate is really kicked off */
    ugly_duckling_add_verify_invalidate_checkpoint_hook(unused_pvd_object_id, provision_drive_info.default_offset);

    status = fbe_api_base_config_metadata_paged_clear_cache(unused_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_initiate_disk_zeroing(unused_pvd_object_id);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to start disk zeroing on PVD\n");

    ugly_duckling_wait_for_verify_invalidate_checkpoint_hook(unused_pvd_object_id, provision_drive_info.default_offset);
    
    ugly_duckling_del_verify_invalidate_checkpoint_hook(unused_pvd_object_id, provision_drive_info.default_offset);

    /* Make sure verify invalidate operation completes */
    status = ugly_duckling_wait_for_verify_invalidate_checkpoint(unused_pvd_object_id, UGLY_DUCKLING_CHECKPOINT_WAIT_IN_SEC, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    ugly_duckling_disable_error_injection(unused_pvd_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "Done Disk Zero Error test PVD %d\n", unused_pvd_object_id);

    return;
}

/*!****************************************************************************
 *  ugly_duckling_metadata_retryable_error_disk_zero_error_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate the handling of retryable metadata error when disk zero command
 * is initiated which forces a write to the metadata region
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. when there is retryable error it is returned back to the caller
 *          2. Validate that the status is generic failure since this is a user
 *             initiated operation and which should be retried
 *
 * @author
 *    05/30/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_metadata_retryable_error_disk_zero_error_test(fbe_object_id_t in_pvd_object_id,
                                                            ugly_duckling_io_configuration_t *all_io_config,
                                                            fbe_test_rg_configuration_t *rg_config_ptr,
                                                            fbe_u16_t error_count)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_object_id_t unused_pvd_object_id = FBE_OBJECT_ID_INVALID;   
    fbe_object_id_t pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_protocol_error_injection_error_record_t     record;
    fbe_protocol_error_injection_record_handle_t record_handle;
	fbe_api_base_config_metadata_paged_change_bits_t paged_change_bits;

    status = fbe_test_sep_util_get_any_unused_pvd_object_id(&unused_pvd_object_id);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to find unused PVD\n");

    mut_printf(MUT_LOG_TEST_STATUS, "Starting retryable Disk Zero Error test PVD %d\n", unused_pvd_object_id);


    paged_change_bits.metadata_offset = 0;
    paged_change_bits.metadata_record_data_size = 2;
    paged_change_bits.metadata_repeat_count = 256;
    paged_change_bits.metadata_repeat_offset = 0;

    * (fbe_u16_t *)paged_change_bits.metadata_record_data = 0xFF;
    status = fbe_api_base_config_metadata_paged_set_bits(unused_pvd_object_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    fbe_api_protocol_error_injection_start();
    /* First disable all background operations  for this tests. */
    ugly_duckling_disable_background_operations(unused_pvd_object_id);

    /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(unused_pvd_object_id, 
                                              &provision_drive_info);

    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Get Info for PVD failed\n");

    /* Initialize the error injection record.
     */
    fbe_test_neit_utils_init_error_injection_record(&record);

    status = fbe_api_get_physical_drive_object_id_by_location(provision_drive_info.port_number, 
                                                              provision_drive_info.enclosure_number, 
                                                              provision_drive_info.slot_number, &pdo_object_id);

    /* Set up the error injection */
    record.object_id = pdo_object_id;
    record.lba_start = provision_drive_info.paged_metadata_lba;
    record.lba_end = record.lba_start + provision_drive_info.paged_metadata_capacity;
    record.num_of_times_to_insert = error_count;

    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 

    /* Since the disk zero does a write of Metadata to mark for zeroing, we need to inject errors
     * on the write
     */
    fbe_test_sep_util_get_scsi_command(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    fbe_test_sep_util_set_scsi_error_from_error_type(&record.protocol_error_injection_error.protocol_error_injection_scsi_error,
                                                     FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE);
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_base_config_metadata_paged_clear_cache(unused_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Initiate the disk zero and since this is user initiated operation
     * retryable errors are returned back to the user and so we expect
     * the API to get generic failure
     */
    status = fbe_api_provision_drive_initiate_disk_zeroing(unused_pvd_object_id);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_GENERIC_FAILURE, "Good status unexpected\n");

    ugly_duckling_disable_protocol_error_injection(pdo_object_id, record_handle);

    mut_printf(MUT_LOG_TEST_STATUS, "Done retryable Disk Zero Error test PVD %d\n", unused_pvd_object_id);

    return;
}

/*!****************************************************************************
 *  ugly_duckling_metadata_retryable_error_bgz_error_test
 ******************************************************************************
 *
 * @brief
 *    This tests validate the handling of retryable metadata error when background
 * zero is initiated which forces a read to the metadata region
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param all_io_config  - IO Profile for the test
 * @param rg_config_ptr - The RG configuration for this test   
 * @param error_count  - Number of errors to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. On Metadata retryable error it is returned back to caller to be retried
 *          2. Validate that the checkpoint is not moved when the errors are injected
 *
 * @author
 *    05/30/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_metadata_retryable_error_bgz_error_test(fbe_object_id_t in_pvd_object_id,
                                                      ugly_duckling_io_configuration_t *all_io_config,
                                                      fbe_test_rg_configuration_t *rg_config_ptr,
                                                      fbe_u16_t error_count)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_object_id_t pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_protocol_error_injection_error_record_t     record;
    fbe_protocol_error_injection_record_handle_t record_handle;

    mut_printf(MUT_LOG_TEST_STATUS, "Starting retryable Background Zero Error test...\n");

        /* First disable all background operations  for this tests. */
    ugly_duckling_disable_background_operations(in_pvd_object_id);

    fbe_api_protocol_error_injection_start();

    /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(in_pvd_object_id, 
                                              &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Initialize the error injection record.
     */
    fbe_test_neit_utils_init_error_injection_record(&record);

    status = fbe_api_get_physical_drive_object_id_by_location(provision_drive_info.port_number, 
                                                              provision_drive_info.enclosure_number, 
                                                              provision_drive_info.slot_number, &pdo_object_id);

    /* Set up the error injection */
    record.object_id = pdo_object_id;
    record.lba_start = provision_drive_info.paged_metadata_lba;
    record.lba_end = record.lba_start + provision_drive_info.paged_metadata_capacity;
    record.num_of_times_to_insert = error_count;

    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 

    /* The background zeroing operation does a read of Metadata to see if the chunk needs to be 
     * zeroed and so inject errors on read
     */
    fbe_test_sep_util_get_scsi_command(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    fbe_test_sep_util_set_scsi_error_from_error_type(&record.protocol_error_injection_error.protocol_error_injection_scsi_error,
                                                     FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE);
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Clear the cache so that read goes to the drive and the error can be injected */
    status = fbe_api_base_config_metadata_paged_clear_cache(in_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_set_zero_checkpoint(in_pvd_object_id, provision_drive_info.default_offset);

    /* Enable zeroing so that background zeroing operation is started */
    ugly_duckling_add_pvd_zero_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);
    status = fbe_test_sep_util_provision_drive_enable_zeroing(in_pvd_object_id);

    /* Make sure that checkpoint is not moved when the error is injected. In this case the
     * error count would be one more than the number of error getting injected 
     */
    ugly_duckling_wait_for_pvd_zero_checkpoint_counter(in_pvd_object_id, provision_drive_info.default_offset, error_count + 1);

    ugly_duckling_del_pvd_zero_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);
    ugly_duckling_disable_protocol_error_injection(pdo_object_id, record_handle);

    return;
}

/*!****************************************************************************
 *  ugly_duckling_metadata_error_lba_checksum_error_test
 ******************************************************************************
 *
 * @brief
 *    This tests injects the specified errors and validates that verify invalidate
 * algorithm is getting kicked off
 *
 * @param in_pvd_object_id - PVD object ID that is being targetted
 * @param error_count  - Number of errors to be injected
 * @param error_type - Type of error to be injected
 *
 * @return  void
 *
 * @notes: 
 *     This test validates that:
 *          1. Verify Invalidate operation is kicked off when the specified error is
 *     injected 
 *
 * @author
 *    05/30/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static void 
ugly_duckling_inject_error_during_bgz(fbe_object_id_t in_pvd_object_id,
                                      fbe_u16_t error_count,
                                      fbe_api_logical_error_injection_type_t error_type)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_api_provision_drive_info_t provision_drive_info;

    /* First disable all background operations  for this tests. */
    ugly_duckling_disable_background_operations(in_pvd_object_id);

    /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(in_pvd_object_id, 
                                              &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    ugly_duckling_perform_error_injection(in_pvd_object_id, 
                                          provision_drive_info.paged_metadata_lba,
                                          error_count,
                                          error_type,
                                          1);

    /* Add hooks to make sure Verify invalidate is really kicked off */
    ugly_duckling_add_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);

    status = fbe_api_base_config_metadata_paged_clear_cache(in_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_set_zero_checkpoint(in_pvd_object_id, provision_drive_info.default_offset);
    status = fbe_test_sep_util_provision_drive_enable_zeroing(in_pvd_object_id);

    ugly_duckling_wait_for_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);
    
    ugly_duckling_del_verify_invalidate_checkpoint_hook(in_pvd_object_id, provision_drive_info.default_offset);

    /* Make sure verify invalidate operation completes */
    status = ugly_duckling_wait_for_verify_invalidate_checkpoint(in_pvd_object_id, UGLY_DUCKLING_CHECKPOINT_WAIT_IN_SEC, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    ugly_duckling_disable_error_injection(in_pvd_object_id);
    return;
}
/*!****************************************************************************
 *  ugly_duckling_test
 ******************************************************************************
 *
 * @brief
 *    This is the main entry point for Ugly Duckling Scenario tests.   These tests
 *    validates the handling of metadata error for different user operation when
 *    zeroing is going on
 *
 *    Refer to individual media error case descriptions for more details.
 *
 * @param   void
 *
 * @return  void
 *
 * @author
 *    03/17/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/

void ugly_duckling_test(void)
{
    fbe_u32_t test_index = 0;
    while(ugly_duckling_test_config[test_index].name != NULL)
    {
        if (ugly_duckling_test_config[test_index + 1].name == NULL) {
            fbe_test_run_test_on_rg_config(&ugly_duckling_test_config[test_index].raid_group_config[0], 
                                           &ugly_duckling_test_config[test_index], ugly_duckling_run_tests,
                                           UGLY_DUCKLING_TEST_LUNS_PER_RAID_GROUP,
                                           UGLY_DUCKLING_TEST_CHUNKS_PER_LUN);
        }
        else {
            fbe_test_run_test_on_rg_config_with_time_save(&ugly_duckling_test_config[test_index].raid_group_config[0], 
                                           &ugly_duckling_test_config[test_index], ugly_duckling_run_tests,
                                           UGLY_DUCKLING_TEST_LUNS_PER_RAID_GROUP,
                                           UGLY_DUCKLING_TEST_CHUNKS_PER_LUN,
                                           FBE_FALSE);
        }
        test_index++;
    }
    return;
}

static void ugly_duckling_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_u32_t                       test_idx =0;          // test case index
    fbe_object_id_t                 pvd_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t                    status = FBE_STATUS_OK;            // status to validate
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_u32_t                       raid_group_count = 0;
    fbe_u32_t                       rg_idx = 0;             // RAID group index
    fbe_test_rg_configuration_t*    cur_rg_config_p = NULL;
    ugly_duckling_test_configuration_t *test_config = NULL;
    fbe_u32_t rg_disk_position;
    test_config = (ugly_duckling_test_configuration_t *)context_p;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_ptr);
    rg_disk_position = test_config->rg_position_to_inject_error;

    // test media error handling on each RAID group
    for ( rg_idx = 0; rg_idx < raid_group_count; rg_idx++ )
    {
        // get configuration info associated with this RAID group
        cur_rg_config_p = &rg_config_ptr[rg_idx];

        if (fbe_test_rg_config_is_enabled(cur_rg_config_p))
        {
            // get id of a PVD object associated with this RAID group
            status = fbe_api_provision_drive_get_obj_id_by_location( cur_rg_config_p->rg_disk_set[rg_disk_position].bus,
                                                                     cur_rg_config_p->rg_disk_set[rg_disk_position].enclosure,
                                                                     cur_rg_config_p->rg_disk_set[rg_disk_position].slot,
                                                                     &pvd_id
                                                                   );

            // insure that there were no errors
            MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

            // wait 20 seconds for this provision drive to become READY
            status = fbe_api_wait_for_object_lifecycle_state( pvd_id,
                                                              FBE_LIFECYCLE_STATE_READY,
                                                              (UGLY_DUCKLING_PVD_READY_WAIT_TIME * 1000),
                                                              FBE_PACKAGE_ID_SEP_0
                                                            );

            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
            status = fbe_api_provision_drive_get_info(pvd_id, 
                                                      &provision_drive_info);

            status = fbe_api_database_lookup_lun_by_number(cur_rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                           &lun_object_id);

            /* In all cases we dont want PVD to complete Zeroing. So add a hook and kick off zeroing */
            ugly_duckling_add_pvd_stop_zero_completion_hook(pvd_id);

            status = fbe_api_lun_initiate_zero(lun_object_id);
            status = fbe_api_provision_drive_set_zero_checkpoint(pvd_id, provision_drive_info.default_offset);

            mut_printf(MUT_LOG_TEST_STATUS, "Starting Test for RG TYPE: %s, PVD:0x%x....\n", test_config->name, pvd_id);
            while(test_config->error_config[test_idx].test_function != NULL)
            {
                test_config->error_config[test_idx].test_function(pvd_id, &test_config->io_config[0], cur_rg_config_p, test_config->error_config[test_idx].err_count);
                test_idx++;
            }   
            mut_printf(MUT_LOG_TEST_STATUS, "Completed Test for RG TYPE: %s, PVD:0x%x....\n", test_config->name, pvd_id);
            ugly_duckling_del_pvd_stop_zero_completion_hook(pvd_id);
            fbe_api_logical_error_injection_disable_object(pvd_id, FBE_PACKAGE_ID_SEP_0);
        }
    }
}


/*!****************************************************************************
 *  ugly_duckling_setup
 ******************************************************************************
 *
 * @brief
 *    This is the setup function for the ugly duckling test.  It configures all
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
 *    03/17/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/

void ugly_duckling_setup(void)
{

    fbe_status_t                    status = FBE_STATUS_OK;            // status to validate

    // Only load config in simulation.
    if (fbe_test_util_is_simulation())
    {
        fbe_test_pp_util_create_physical_config_for_disk_counts(UGLY_DUCKLING_520_DISK_COUNT,
                                                                UGLY_DUCKLING_4160_DISK_COUNT,
                                                                TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);

        elmo_load_sep_and_neit();

    }


    // disable error injection on all objects
    status = fbe_api_logical_error_injection_disable_object( FBE_OBJECT_ID_INVALID, FBE_PACKAGE_ID_SEP_0 );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    fbe_test_sep_util_disable_system_drive_zeroing();
    
    return;

}   // end ugly_duckling_setup()

/*!****************************************************************************
 *  ugly_duckling_cleanup
 ******************************************************************************
 *
 * @brief
 *    This is the clean-up function for the ugly duckling test.
 *
 * @param   void
 *
 * @return  void
 *
 * @author
 *    03/19/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/

void ugly_duckling_cleanup(void)
{
    // Only destroy config in simulation  
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;

}   // end ugly_duckling_cleanup()

/*!**************************************************************
 * ugly_duckling_wait_for_verify_invalidate_checkpoint()
 ****************************************************************
 * @brief
 *  Wait for the verify invalidate checkpoint going to certain value
 *
 * @param pvd_object_id - Object id to wait on
 * @param wait_seconds - max seconds to wait
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  03/17/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t ugly_duckling_wait_for_verify_invalidate_checkpoint(fbe_object_id_t pvd_object_id,
                                                                        fbe_u32_t wait_seconds,
                                                                        fbe_lba_t value)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t verify_invalidate_checkpoint;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec)
    {
        status = fbe_api_provision_drive_get_verify_invalidate_checkpoint(pvd_object_id, &verify_invalidate_checkpoint);
        
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        if (verify_invalidate_checkpoint == value)
        {
            break;
        }
        fbe_api_sleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= target_wait_msec)
    {
        MUT_ASSERT_UINT64_EQUAL_MSG(verify_invalidate_checkpoint, value, "Verify Invalidate checkpoint != value\n");
    }
    return status;
}
/******************************************
 * end ugly_duckling_wait_for_verify_invalidate_checkpoint()
 ******************************************/
/*!**************************************************************
 * ugly_duckling_wait_for_zero_checkpoint()
 ****************************************************************
 * @brief
 *  Wait for the zero checkpoint going to certain value
 *
 * @param pvd_object_id - Object id to wait on
 * @param wait_seconds - max seconds to wait
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  03/17/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t ugly_duckling_wait_for_zero_checkpoint(fbe_object_id_t pvd_object_id,
                                                           fbe_u32_t wait_seconds,
                                                           fbe_lba_t value)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t zero_checkpoint;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec)
    {
        status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &zero_checkpoint);
        
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        if (zero_checkpoint == value)
        {
            break;
        }
        fbe_api_sleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= target_wait_msec)
    {
        MUT_ASSERT_UINT64_EQUAL_MSG(zero_checkpoint, value, "Zero checkpoint != value\n");
    }
    return status;
}
/******************************************
 * end ugly_duckling_wait_for_zero_checkpoint()
 ******************************************/
/*!**************************************************************
 * ugly_duckling_disable_background_operations()
 ****************************************************************
 * @brief
 *  This functions stops all the background operations on the given PVD
 *
 * @param pvd_object_id - Object id to disable
 * 
 * @return fbe_status_t 
 *
 * @author
 *  03/17/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void ugly_duckling_disable_background_operations(fbe_object_id_t pvd_object_id)
{
    fbe_status_t status;
    /* Before starting the sniff related operation disable the disk zeroing.
     */
    status = fbe_api_provision_drive_set_zero_checkpoint(pvd_object_id, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not set PVD zero flag!");
    status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_test_sep_util_provision_drive_disable_verify (pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * ugly_duckling_perform_error_injection()
 ****************************************************************
 * @brief
 *  This function performs the error injection setup for the given object
 *
 * @param pvd_object_id - Object id to perform error injection
 * @param lba - LBA to start the error injection
 * @param err_count - Number of errors to be injected
 * @param err_type - Type of error to be injected
 * @param blocks - Number of blocks
 *
 * @return fbe_status_t 
 *
 * @author
 *  03/17/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void ugly_duckling_perform_error_injection(fbe_object_id_t pvd_object_id,
                                                  fbe_lba_t lba,
                                                  fbe_u16_t err_count,
                                                  fbe_api_logical_error_injection_type_t err_type,
                                                  fbe_block_count_t blocks)
                                                  
{
    fbe_status_t status;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_logical_error_injection_record_t error_record;

    ugly_duckling_init_error_record(&error_record, lba, err_count, err_type, blocks);

    // add new error injection record for selected error case
    status = fbe_api_logical_error_injection_create_record(&error_record );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
    // enable error injection on this provision drive
    status = fbe_api_logical_error_injection_enable_object( pvd_object_id, FBE_PACKAGE_ID_SEP_0 );
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
}

/*!**************************************************************
 * ugly_duckling_perform_corrupt_paged_injection()
 ****************************************************************
 * @brief
 *  This function performs the error injection setup for the given object
 *
 * @param pvd_object_id - Object id to perform error injection
 * @param lba - LBA to start the error injection
 * @param err_count - Number of errors to be injected
 * @param err_type - Type of error to be injected
 * @param blocks - Number of blocks
 *
 * @return fbe_status_t 
 *
 * @author
 *  3/13/2013 - Created. Rob Foley
 *
 ****************************************************************/
static void ugly_duckling_perform_corrupt_paged_injection(fbe_object_id_t pvd_object_id,
                                                          fbe_lba_t lba,
                                                          fbe_u16_t err_count,
                                                          fbe_block_count_t blocks)
                                                  
{
    fbe_status_t status;
    fbe_api_base_config_metadata_paged_change_bits_t paged_change_bits;
    fbe_provision_drive_paged_metadata_t paged_bits;
    fbe_u32_t repeat_count;
    fbe_api_provision_drive_info_t provision_drive_info;

    fbe_zero_memory(&paged_bits, sizeof(fbe_provision_drive_paged_metadata_t));
    paged_bits.consumed_user_data_bit = 1;
    paged_bits.need_zero_bit = 1;
    paged_bits.user_zero_bit = 1;

    /* Since we are going to read directly from the drive, we need to get the default offset of the PVD */
    status = fbe_api_provision_drive_get_info(pvd_object_id, 
                                              &provision_drive_info);

    paged_change_bits.metadata_offset = (provision_drive_info.default_offset/provision_drive_info.chunk_size) * sizeof(fbe_provision_drive_paged_metadata_t);
    paged_change_bits.metadata_record_data_size = sizeof(fbe_provision_drive_paged_metadata_t);

    /* Just corrupt one chunk of paged.
     */
    repeat_count = 1;
    paged_change_bits.metadata_repeat_count = repeat_count;
    paged_change_bits.metadata_repeat_offset = 0;

    fbe_copy_memory(&paged_change_bits.metadata_record_data[0],
                    &paged_bits, sizeof(fbe_provision_drive_paged_metadata_t));

    status = fbe_api_base_config_metadata_paged_clear_bits(pvd_object_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/**************************************
 * end ugly_duckling_perform_corrupt_paged_injection()
 **************************************/
/*!**************************************************************
 * ugly_duckling_wait_for_error_verify_checkpoint()
 ****************************************************************
 * @brief
 *  Wait for the error verify checkpoint going to certain value
 *
 * @param pvd_object_id - Object id to wait on
 * @
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  03/17/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t ugly_duckling_wait_for_error_verify_checkpoint(fbe_object_id_t raid_object_id,
                                                                   fbe_lba_t value)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = UGLY_DUCKLING_WAIT_SEC * FBE_TIME_MILLISECONDS_PER_SECOND;
    fbe_api_raid_group_get_info_t  raid_group_info;

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec)
    {
        status = fbe_api_raid_group_get_info(raid_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        if (raid_group_info.error_verify_checkpoint == value)
        {
            /* If we are waiting for a completion of verify make sure the event q is empty
             * to insure no verifies are incoming.
             */
            if ((value != FBE_LBA_INVALID) ||
                (raid_group_info.b_is_event_q_empty == FBE_TRUE))
            {
                break;
            }
        }
        fbe_api_sleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= target_wait_msec)
    {
        MUT_ASSERT_UINT64_EQUAL_MSG(raid_group_info.error_verify_checkpoint, value, "Error verify checkpoint != value\n");
    }
    return status;
}
/******************************************************
 * end ugly_duckling_wait_for_error_verify_checkpoint()
 ******************************************************/

/*!**************************************************************
 * ugly_duckling_get_expected_invalid_lba_block_count()
 ****************************************************************
 * @brief
 *  This functions returns the pre and post edges that are not part
 * of the IO that spans the chunks
 *
 * @param start_lba  - Start Lba of the IO
 * @param block_count - Block count for the IO
 * @param pre_start_lba - Returns the start of the LBA for that chunk
 * @param pre_block_count - Block count from pre_start_lba to start_lba
 * @param post_start_lba  - Start LBA for post the IO region
 * @param post_block_count - Count for the post IO region
 * 
 * @return fbe_status_t 
 *
 * @author
 *  03/17/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void 
ugly_duckling_get_expected_invalid_lba_block_count(fbe_lba_t start_lba, fbe_block_count_t block_count,
                                                   fbe_lba_t *pre_start_lba, fbe_block_count_t *pre_block_count,
                                                   fbe_lba_t *post_start_lba, fbe_block_count_t *post_block_count)
{
    fbe_chunk_size_t chunk_size;
    fbe_chunk_count_t chunk_count;
    fbe_chunk_index_t start_chunk_index;
    fbe_chunk_index_t end_chunk_index;

    chunk_size = UGLY_DUCKLING_CHUNK_SIZE;

    /* Get the chunk index for the given start lba and end lba. */
    start_chunk_index = (start_lba - (start_lba % chunk_size)) / chunk_size;

    /* get the end chunk index to find the stripe lock range. */
    if((start_lba + block_count) % chunk_size)
    {
        end_chunk_index = (start_lba + block_count + chunk_size - ((start_lba + block_count) % chunk_size)) / chunk_size;
    }
    else
    {
        end_chunk_index = (start_lba + block_count) / chunk_size;
    }

    /* get the chunk count. */
    chunk_count = (fbe_chunk_count_t) (end_chunk_index - start_chunk_index);

    *pre_start_lba = FBE_LBA_INVALID;
    *pre_block_count = 0;
    *post_start_lba = FBE_LBA_INVALID;
    *post_block_count = 0;

    /* if start lba is not aligned and (start + block count) is not aligned to chunk size 
     * then  increment the pre and post edge both if it covers more than one chunk and both
     * edges are set for zero or not valid else set only pre edge.
     */
    if((start_lba % chunk_size) && ((start_lba + block_count) % chunk_size))
    {
        *pre_start_lba = start_lba - (start_lba % chunk_size);
        *pre_block_count = start_lba - *pre_start_lba;
        *post_start_lba = start_lba + block_count;
        *post_block_count = (*pre_start_lba + (chunk_size * chunk_count)) - *post_start_lba;
    }

    /* if start lba is not aligned and (start + block count) is aligned to chunk size 
     * then  increment the edge (pre_edge) if it set for the zero on demand or page not valid.
     */
    else if(start_lba % chunk_size)
    {
        *pre_start_lba = start_lba - (start_lba % chunk_size);
        *pre_block_count = start_lba - *pre_start_lba; 
    }

    /* if start lba is aligned and (start + block count) is not aligned to chunk size 
     * then  increment the edge (post_edge) if it set for the zero on demand or page not valid
     */
    else if((start_lba + block_count) % chunk_size)
    {
        *post_start_lba = start_lba + block_count;
        *post_block_count = (start_lba + (chunk_size * chunk_count)) - *post_start_lba;;
    }
}
static fbe_status_t ugly_duckling_check_for_invalid_pattern(fbe_object_id_t in_pvd_object_id,
                                                            fbe_lba_t pre_start_lba, fbe_block_count_t pre_block_count,
                                                            fbe_lba_t post_start_lba, fbe_block_count_t post_block_count)
{
    if(pre_block_count != 0)
    {
        ugly_duckling_validate_pattern(in_pvd_object_id,
                                       pre_start_lba, 
                                       pre_block_count);
    }
    if(post_block_count != 0)
    {
        ugly_duckling_validate_pattern(in_pvd_object_id,
                                       post_start_lba, 
                                       post_block_count);
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * ugly_duckling_validate_pattern()
 ****************************************************************
 * @brief
 *  This functions performs the read to check for invalid pattern
 *
 * @param pvd_object_id - Object id to look for invalid pattern
 * @param start_lba - Start LBA to look for pattern
 * @param block_count - Block count
 * 
 * @return fbe_status_t 
 *
 * @author
 *  03/17/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t ugly_duckling_validate_pattern(fbe_object_id_t in_pvd_object_id,
                                                   fbe_lba_t start_lba, fbe_block_count_t block_count)
{
    fbe_raid_verify_error_counts_t verify_error_counts = {0};
    fbe_block_transport_block_operation_info_t  block_operation_info;
    fbe_sg_element_t * sg_list_p = NULL;
    fbe_payload_block_operation_t block_operation;
    fbe_u8_t * buffer = NULL;
    fbe_status_t status;
    ugly_duckling_invalid_sector_info_t *sector_info = NULL;
    fbe_block_count_t blocks;

        /* Allocate memory for a 2-element sg list.
     */
    buffer = fbe_api_allocate_memory((fbe_u32_t)(block_count * 520) + (2 * sizeof(fbe_sg_element_t)));
    sg_list_p = (fbe_sg_element_t *)buffer;

    /* Create sg list.
     */
    sg_list_p[0].address = buffer + 2*sizeof(fbe_sg_element_t);
    sg_list_p[0].count = (fbe_u32_t) block_count * 520;
    sg_list_p[1].address = NULL;
    sg_list_p[1].count = 0;

    /* Create appropriate payload.
     */
    fbe_payload_block_build_operation(&block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      start_lba,
                                      block_count,
                                      520,
                                      1, /* optimum block size */
                                      NULL);
    
    block_operation_info.block_operation = block_operation;
    block_operation_info.verify_error_counts = verify_error_counts;

    mut_printf(MUT_LOG_LOW, "=== manually read invalidated region - pvd obj: 0x%x lba: 0x%llx, blocks: %d\n", 
               in_pvd_object_id, (unsigned long long)start_lba, (int)block_count);

    status = fbe_api_block_transport_send_block_operation(in_pvd_object_id, 
                                                          FBE_PACKAGE_ID_SEP_0, 
                                                          &block_operation_info, 
                                                          sg_list_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    blocks = 0;
    while(blocks < block_count)
    {
        /* Check to make sure the sectors has the invalid pattern that we expect */
        sector_info = (ugly_duckling_invalid_sector_info_t *)(sg_list_p[0].address + (520 * blocks));
        MUT_ASSERT_INT_EQUAL_MSG(sector_info->first_reason, XORLIB_SECTOR_INVALID_REASON_PVD_METADATA_INVALID, "Invalidated reason unexpected"); 
        MUT_ASSERT_INT_EQUAL_MSG(sector_info->first_who, XORLIB_SECTOR_INVALID_WHO_PVD, "Invalided source unknown");
        MUT_ASSERT_INT_EQUAL_MSG(sector_info->last_reason, XORLIB_SECTOR_INVALID_REASON_PVD_METADATA_INVALID, "Invalidated reason unexpected"); 
        MUT_ASSERT_INT_EQUAL_MSG(sector_info->last_who, XORLIB_SECTOR_INVALID_WHO_PVD, "Invalided source unknown");
        MUT_ASSERT_INT_EQUAL_MSG(sector_info->reason, XORLIB_SECTOR_INVALID_REASON_PVD_METADATA_INVALID, "Invalidated reason unexpected"); 
        MUT_ASSERT_INT_EQUAL_MSG(sector_info->who, XORLIB_SECTOR_INVALID_WHO_PVD, "Invalided source unknown");
        blocks++;
    }

    fbe_api_free_memory(buffer);
    return FBE_STATUS_OK;
}
static void ugly_duckling_add_verify_invalidate_checkpoint_hook(fbe_object_id_t in_pvd_object_id, fbe_lba_t checkpoint)
{
    fbe_status_t status;
    status = fbe_api_scheduler_add_debug_hook(in_pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_VERIFY_INVALIDATE_CHECKPOINT, 
                                                  checkpoint,
                                                  NULL, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
static void ugly_duckling_del_verify_invalidate_checkpoint_hook(fbe_object_id_t in_pvd_object_id, fbe_lba_t checkpoint)
{
    fbe_status_t status;
    status = fbe_api_scheduler_del_debug_hook(in_pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE,
                                              FBE_PROVISION_DRIVE_SUBSTATE_VERIFY_INVALIDATE_CHECKPOINT, 
                                              checkpoint,
                                              NULL, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
static void ugly_duckling_wait_for_verify_invalidate_checkpoint_hook(fbe_object_id_t in_pvd_object_id, fbe_lba_t checkpoint)
{
    fbe_status_t status; 
    status = fbe_test_wait_for_debug_hook(in_pvd_object_id, 
                                         SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE, 
                                         FBE_PROVISION_DRIVE_SUBSTATE_VERIFY_INVALIDATE_CHECKPOINT,
                                         SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_PAUSE,
                                         checkpoint, NULL);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
}
static void ugly_duckling_add_error_verify_checkpoint_hook(fbe_object_id_t raid_group_object_id, fbe_lba_t checkpoint)
{
    fbe_status_t status;
    status = fbe_api_scheduler_add_debug_hook(raid_group_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT, 
                                                  checkpoint,
                                                  NULL, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
static void ugly_duckling_del_error_verify_checkpoint_hook(fbe_object_id_t raid_group_object_id, fbe_lba_t checkpoint)
{
    fbe_status_t status;
    status = fbe_api_scheduler_del_debug_hook(raid_group_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT, 
                                                  checkpoint,
                                                  NULL, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
static void ugly_duckling_wait_for_error_verify_checkpoint_hook(fbe_object_id_t raid_group_object_id, fbe_lba_t checkpoint)
{
    fbe_status_t status; 
    status = fbe_test_wait_for_debug_hook(raid_group_object_id, 
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY, 
                                         FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                         SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_PAUSE,
                                         checkpoint, NULL);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
}    

static void ugly_duckling_get_io_configuration(ugly_duckling_test_index_t index, 
                                               ugly_duckling_io_configuration_t *io_info,
                                               ugly_duckling_io_configuration_t **io_config)
{
    fbe_u32_t test_count = 0; 
    while(test_count < UGLY_DUCKLING_TEST_ZERO_ON_DEMAND_LAST)
    {
        if(io_info->test_index == index)
        {
            *io_config = io_info;
            return;
        }
        io_info++;
        test_count++;
    }
    MUT_ASSERT_INT_EQUAL_MSG(index, 0,"Test Index not found\n");
}
static void ugly_duckling_add_pvd_block_operation_hook(fbe_object_id_t in_pvd_object_id,
                                                       fbe_payload_block_operation_opcode_t opcode)
{
    fbe_status_t status;
    status = fbe_api_scheduler_add_debug_hook(in_pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION,
                                              FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK, 
                                              opcode,
                                              NULL, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
static void ugly_duckling_del_pvd_block_operation_hook(fbe_object_id_t in_pvd_object_id, 
                                                       fbe_payload_block_operation_opcode_t opcode)
{
    fbe_status_t status;
    status = fbe_api_scheduler_del_debug_hook(in_pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION,
                                              FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK, 
                                              opcode,
                                              NULL, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}
static void ugly_duckling_wait_for_pvd_block_operation_counter(fbe_object_id_t in_pvd_object_id, 
                                                               fbe_payload_block_operation_opcode_t opcode,
                                                               fbe_u64_t counter)
{
    fbe_status_t status; 
    status = fbe_test_wait_for_hook_counter(in_pvd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION, 
                                                 FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK,
                                                 SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_CONTINUE,
                                                 opcode, NULL, counter);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
}

static void ugly_duckling_pull_drive(fbe_test_rg_configuration_t*        in_rg_config_p,
                                     fbe_u32_t                           in_position,
                                     fbe_api_terminator_device_handle_t* out_drive_info_p)
{
    fbe_u32_t           				number_physical_objects = 0;
    fbe_status_t status;

    /* need to remove a drive here */
	status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
	sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, in_position,
                                              number_physical_objects-1, out_drive_info_p);
}

static void ugly_duckling_reinsert_drive(fbe_test_rg_configuration_t*        in_rg_config_p,
                                         fbe_u32_t                           in_position,
                                         fbe_api_terminator_device_handle_t* in_drive_info_p)
{
    fbe_u32_t           				number_physical_objects = 0;
    fbe_status_t status;

    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    sep_rebuild_utils_reinsert_drive_and_verify(in_rg_config_p, in_position,
                                                number_physical_objects+1, in_drive_info_p);
}
static void ugly_duckling_wait_for_rg_degraded(fbe_test_rg_configuration_t *current_rg_config_p, fbe_u32_t position)
{
    sep_rebuild_utils_verify_rb_logging(current_rg_config_p, position);
}

static void ugly_duckling_wait_for_rg_normal(fbe_test_rg_configuration_t *current_rg_config_p, fbe_u32_t position)
{
    sep_rebuild_utils_verify_not_rb_logging(current_rg_config_p, position);
}

static void ugly_duckling_add_pvd_non_mcr_region_permission_request_hook(fbe_object_id_t in_pvd_object_id,
                                                                         fbe_lba_t checkpoint)
{
    fbe_status_t status;
    status = fbe_api_scheduler_add_debug_hook(in_pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE,
                                              FBE_PROVISION_DRIVE_SUBSTATE_NON_MCR_REGION_CHECKPOINT, 
                                              checkpoint,
                                              NULL, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

static void ugly_duckling_del_pvd_non_mcr_region_permission_request_hook(fbe_object_id_t in_pvd_object_id,
                                                                         fbe_lba_t checkpoint)
{
    fbe_status_t status;
    status = fbe_api_scheduler_del_debug_hook(in_pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE,
                                              FBE_PROVISION_DRIVE_SUBSTATE_NON_MCR_REGION_CHECKPOINT, 
                                              checkpoint,
                                              NULL, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}
static void ugly_duckling_wait_for_pvd_non_mcr_region_permission_request_counter(fbe_object_id_t in_pvd_object_id, 
                                                                                 fbe_u64_t counter,
                                                                                 fbe_lba_t checkpoint)
{
    fbe_status_t status; 
    status = fbe_test_wait_for_hook_counter(in_pvd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE, 
                                                 FBE_PROVISION_DRIVE_SUBSTATE_NON_MCR_REGION_CHECKPOINT,
                                                 SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_CONTINUE,
                                                 checkpoint, NULL, counter);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
}
    
static void ugly_duckling_add_pvd_stop_zero_completion_hook(fbe_object_id_t in_pvd_object_id)
{
    fbe_status_t status;
    status = fbe_api_scheduler_add_debug_hook(in_pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                              FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION, 
                                              NULL,
                                              NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
static void ugly_duckling_del_pvd_stop_zero_completion_hook(fbe_object_id_t in_pvd_object_id)
{
    fbe_status_t status;
    status = fbe_api_scheduler_del_debug_hook(in_pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                              FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION, 
                                              NULL,
                                              NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}
static void ugly_duckling_wait_for_pvd_stop_zero_completion_hook(fbe_object_id_t in_pvd_object_id)
{
    fbe_status_t status; 
    status = fbe_test_wait_for_debug_hook(in_pvd_object_id, 
                                         SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO, 
                                         FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION,
                                         SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                         NULL, NULL);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
}
    
static void ugly_duckling_disable_error_injection(fbe_object_id_t in_pvd_object_id)
{
    fbe_status_t status;

    fbe_api_logical_error_injection_disable_object(in_pvd_object_id, FBE_PACKAGE_ID_SEP_0);

    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // disable any previous error injection records
    status = fbe_api_logical_error_injection_disable_records( 0, 255 );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
}

static void ugly_duckling_init_error_record(fbe_api_logical_error_injection_record_t *error_record,
                                            fbe_lba_t lba,
                                            fbe_u16_t err_to_be_injected,
                                            fbe_api_logical_error_injection_type_t err_type,
                                            fbe_block_count_t blocks)
{
    error_record->pos_bitmap = 0x4;
    error_record->width = 0x10;
    error_record->lba = lba;
    error_record->blocks = blocks;
    error_record->err_type = err_type;
    error_record->err_mode =FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT;
    error_record->err_count = 0;
    error_record->err_limit = err_to_be_injected;
    error_record->skip_count = 0;
    error_record->skip_limit= 0x15;
    error_record->err_adj = 1;
    error_record->start_bit = 0;
    error_record->num_bits = 0;
    error_record->bit_adj = 0;
    error_record->crc_det = 0;
    error_record->opcode = 0;
}   

static void ugly_duckling_disable_protocol_error_injection(fbe_object_id_t pdo_object_id,
                                                           fbe_protocol_error_injection_record_handle_t handle_p)
{
    fbe_status_t status;
    
    status = fbe_api_protocol_error_injection_remove_record(handle_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_protocol_error_injection_remove_object(pdo_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}

static void ugly_duckling_add_pvd_zero_checkpoint_hook(fbe_object_id_t in_pvd_object_id,
                                                       fbe_lba_t zero_checkpoint)
{
    fbe_status_t status;
    status = fbe_api_scheduler_add_debug_hook(in_pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                              FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY, 
                                              zero_checkpoint,
                                              NULL, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
static void ugly_duckling_del_pvd_zero_checkpoint_hook(fbe_object_id_t in_pvd_object_id, 
                                                       fbe_lba_t zero_checkpoint)
{
    fbe_status_t status;
    status = fbe_api_scheduler_del_debug_hook(in_pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                              FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY, 
                                              zero_checkpoint,
                                              NULL, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}
static void ugly_duckling_wait_for_pvd_zero_checkpoint_counter(fbe_object_id_t in_pvd_object_id, 
                                                               fbe_lba_t zero_checkpoint,
                                                               fbe_u64_t counter)
{
    fbe_status_t status; 
    status = fbe_test_wait_for_hook_counter(in_pvd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO, 
                                                 FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
                                                 SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_CONTINUE,
                                                 zero_checkpoint, NULL, counter);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
}

/************************************
 * end file ugly_duckling_test.c
 ***********************************/
