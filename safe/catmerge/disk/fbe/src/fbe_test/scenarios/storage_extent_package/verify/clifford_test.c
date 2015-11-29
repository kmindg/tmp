/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file clifford_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a user background verify test.
 *
 * @version
 *   8/18/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_database.h"
#include "sep_dll.h"
#include "physical_package_dll.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe_test_configurations.h"
#include "clifford_test.h"
#include "fbe_test_common_utils.h"
#include "fbe_private_space_layout.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * clifford_crc_error_test_short_desc = "Test single and multi bit CRC errors and shooting of drives as a result.";

char * clifford_short_desc = "test user initiated LU verify operations";
char * clifford_long_desc =
    "\n"
    "\n"
    "The Clifford scenario tests the user initiated write verify and read-only\n"
    "verify operations.\n"
    "\n"
    "It covers the following cases:\n"
    "    - It validates that a verify operation is setup and performed on paged metadata region\n"
    "    - It verifies that the lun and raid objects can setup and perform a verify operation.\n"
    "    - It validates the accumulated errors in the LUN verify report.\n"
    "\n"
    "Dependencies:\n"
    "    - The raid object must support verify type I/Os and any required verify meta-data.\n"
    "    - The lun object must support the user verify command set.\n"
    "    - Persistent meta-data storage.\n"
    "\n"
    "Starting Config:\n"
    "    [PP]  4 fully populated enclosures configured\n"
    "    [SEP] 3 LUNs on a raid-0 object with 6 drives\n"
    "    [SEP] 3 LUNs on a raid-5 object with 5 drives\n"
    "    [SEP] 3 LUNs on a raid-5 object with 2 drives\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create and verify the initial physical package config.\n"
    "    - Create all provision drives (PD) with an I/O edge attched to a logical drive (LD).\n"
    "    - Create all virtual drives (VD) with an I/O edge attached to a PD.\n"
    "    - Create all raid objects with I/O edges attached to all VDs.\n"
    "    - Create all lun objects with an I/O edge attached to its RAID.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"
    "STEP 2: Write initial data pattern\n"
    "    - Use rdgen to write an initial data pattern to all LUN objects.\n"
    "\n"
    "STEP 3: Inject RAID stripe errors.\n"
    "    - Use rdgen to inject errors one LUN of each RAID object.\n"
    "\n"
    "STEP 4: Initiate read-only verify operation.\n"
    "    - Validate that the LU verify report properly reflects the completed\n"
    "      verify operation.\n"
    "\n"
    "STEP 5: Initiate a write verify operation.\n"
    "    - Validate that the LU verify report properly reflects the completed\n"
    "      verify operation.\n"
    "    - Verify that all correctable errors from the prior read-only verify\n"
    "      operation were not corrected.\n"
    "\n"
    "STEP 6: Initiate a second write verify operation.\n"
    "    - Validate that the LU verify report properly reflects the completed\n"
    "      verify operation.\n"
    "    - Verify that all correctable errors from the prior write verify\n"
    "      operation were corrected.\n"
    "\n"
    "Description last updated: 9/27/2011.\n";





/*!*******************************************************************
 * @def CLIFFORD_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Number of raid groups we will test with.
 *
 *********************************************************************/
#define CLIFFORD_TEST_RAID_GROUP_COUNT   4

/*!*******************************************************************
 * @def CLIFFORD_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in our raid group.
 *********************************************************************/
#define CLIFFORD_TEST_LUNS_PER_RAID_GROUP    3

/*!*******************************************************************
 * @def CLIFFORD_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of Chunks in Lun.
 *********************************************************************/
#define CLIFFORD_TEST_CHUNKS_PER_LUN    3

/*!*******************************************************************
 * @def CLIFFORD_TEST_ERROR_RECORD_COUNT
 *********************************************************************
 * @brief Number of logical error injections.
 *********************************************************************/
#define CLIFFORD_TEST_ERROR_RECORD_COUNT    12

/*!*******************************************************************
 * @def CLIFFORD_BASE_OFFSET
 *********************************************************************
 * @brief All drives have this base offset, that we need to add on.
 *
 *********************************************************************/
#define CLIFFORD_BASE_OFFSET 0x10000

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

static void clifford_raid5_verify_test(fbe_u32_t index, fbe_test_rg_configuration_t * in_current_rg_config_p);
static void clifford_raid5_group_verify_test(fbe_test_rg_configuration_t* in_current_rg_config_p);
static void clifford_raid1_verify_test(fbe_u32_t index, fbe_test_rg_configuration_t * in_current_rg_config_p);
static void clifford_raid10_verify_test(fbe_u32_t index, fbe_test_rg_configuration_t * in_current_rg_config_p);
static void clifford_verify_with_error_injection_test(fbe_test_rg_configuration_t *in_rg_config_p);
static void clifford_verify_with_error_injection_test_group(fbe_u32_t in_index, fbe_test_rg_configuration_t * in_current_rg_config_p);
static void clifford_verify_with_error_injection_and_send_crc_errs_test(fbe_test_rg_configuration_t *in_rg_config_p);
static void clifford_crc_errors_test(fbe_test_rg_configuration_t * current_rg_config_p);
static void clifford_validate_verify_counts(fbe_lun_verify_counts_t *expected_counts, fbe_lun_verify_counts_t *verify_counts);
static void clifford_newly_zeroed_verify_test(fbe_test_rg_configuration_t *rg_config_p);
void clifford_set_new_library_debug_flag_for_raid_group(fbe_object_id_t rg_object_id,
                                                    fbe_raid_library_debug_flags_t new_debug_flag);
void clifford_clear_new_library_debug_flag_for_raid_group(fbe_object_id_t rg_object_id,
                                                    fbe_raid_library_debug_flags_t new_debug_flag);


/*****************************************
 * EXTERNAL FUNCTION DEFINITIONS
 *****************************************/
extern fbe_status_t mumford_the_magician_enable_error_injection_for_raid_group(fbe_raid_group_type_t raid_type,
                                                                        fbe_object_id_t rg_object_id);

/*****************************************
 * GLOBAL DATA
 *****************************************/

/*!*******************************************************************
 * @var clifford_raid_groups
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t clifford_raid_group_config[] = 
{
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {6,         0x32000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,  520,            0,          0},
    {5,         0x32000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            1,          0},
    {2,         0x32000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            2,          0},
    {4,         0x32000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,            3,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};

/*!*******************************************************************
 * @var fbe_clifford_test_context_g
 *********************************************************************
 * @brief This contains the context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t fbe_clifford_test_context_g[CLIFFORD_TEST_LUNS_PER_RAID_GROUP * CLIFFORD_TEST_RAID_GROUP_COUNT];

/*!*******************************************************************
 * @def clifford_debug_enable_raid_group_types
 *********************************************************************
 * @brief Global array that determines which raid group types should
 *        have debug flags enabled.
 *
 *********************************************************************/
/*! @todo Remove `FBE_RAID_GROUP_TYPE_RAID1' from the line below*/
fbe_raid_group_type_t clifford_debug_enable_raid_group_types[CLIFFORD_TEST_RAID_GROUP_COUNT] = {0/*FBE_RAID_GROUP_TYPE_RAID1*/};
fbe_raid_library_debug_flags_t clifford_raid_library_debug_flags = {FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING      | 
                                                                    FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING   };
fbe_bool_t  clifford_enable_pvd_tracing = FBE_FALSE; /* Default is FBE_FALSE */
fbe_provision_drive_debug_flags_t   clifford_pvd_debug_flags = (FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING       |
                                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING         );

/*!*******************************************************************
 * @struct clifford_logical_error_case_t
 *********************************************************************
 * @brief Definition of logical error insertion case.
 *
 *********************************************************************/
typedef struct clifford_logical_error_case_s
{
    char*                                     description;           // error case description
    fbe_raid_group_number_t                        raid_group_id;
    fbe_api_logical_error_injection_record_t  record;                // logical error injection record
    fbe_lun_verify_counts_t                   report;
} clifford_logical_error_case_t;

/*!*******************************************************************
 * @def clifford_error_record_array
 *********************************************************************
 * @brief Global array that determines which raid group should
 *        inject logical errors.
 *
 *********************************************************************/
clifford_logical_error_case_t clifford_error_record_array[CLIFFORD_TEST_ERROR_RECORD_COUNT] =
{
    {
        " Test logical error injection on RG id 0 error type CRC",
        0,
        {                                           // logical error injection record
            0x4, 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        " Test logical error injection on RG id 0 error type SOFT MEDIA ERROR",
        0,
        {                                           // logical error injection record
            0x4, 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x1, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    },
    {
        " Test logical error injection on RG id 1 error type CRC",
        1,
        {                                           // logical error injection record
            0x2, 0x5, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        " Test logical error injection on RG id 1 error type HARD MEDIA ERROR",
        1,
        {                                           // logical error injection record
            0x2, 0x5, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x1, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    },
    {
        " Test logical error injection on RG id 1 error type SOFT MEDIA ERROR",
        1,
        {                                           // logical error injection record
            0x2, 0x5, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x1, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    },
    {
        " Test logical error injection on RG id 1 error type TIME STAMP ERROR",
        1,
        {                                           // logical error injection record
            0x2, 0x5, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        " Test logical error injection on RG id 2 error type CRC",
        2,
        {                                           // logical error injection record
            0x1, 0x2, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        " Test logical error injection on RG id 2 error type SOFT MEDIA ERROR",
        2,
        {                                           // logical error injection record
            0x1, 0x2, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x1, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    },
    {
        " Test logical error injection on RG id 3 error type CRC",
        3,
        {                                           // logical error injection record
            0x2, 0x4, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x1, 0x0, 0x15, 0x2, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        " Test logical error injection on RG id 3 error type SOFT MEDIA ERROR",
        3,
        {                                           // logical error injection record
            0x2, 0x4, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x1, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    },
    {
        " Test logical error injection on RG id 0 error type lba stamp",
        0,
        {                                           // logical error injection record
            0x4, 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
    },
    {
        " Test logical error injection on RG id 0 error type lba stamp",
        1,
        {                                           // logical error injection record
            0x2, 0x5, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    },
};
enum clifford_defines {
    /* Drive position to fail in crc error test. */
    CLIFFORD_POS_TO_FAIL = 1
};
/*!*******************************************************************
 * @def clifford_crc_error_record_array
 *********************************************************************
 * @brief Global array that determines which raid group should
 *        inject logical errors.
 *
 *********************************************************************/
clifford_logical_error_case_t clifford_crc_error_record_array[] =
{
    {
        "RAID-0 single bit crc error does not result in drive being shot.",
        0,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        "RAID-0 multi-bit crc error.",
        0,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        "RAID-0 multi-bit crc with lba stamp error when encrypted.",
        0,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        "RAID-5 single bit crc error does not result in drive being shot.",
        1,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        "RAID-5 multi-bit crc error.",
        1,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        "RAID-5 multi-bit crc with lba stamp error when encrypted.",
        1,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        "RAID-1 single bit crc error does not result in drive being shot.",
        2,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        "RAID-1 multi-bit crc error.",
        2,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        "RAID-1 multi-bit crc with lba stamp error when encrypted.",
        2,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        "RAID-10 single bit crc error does not result in drive being shot.",
        3,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2 * 2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        "RAID-10 multi-bit crc error.",
        3,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2 * 2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        "RAID-10 multi-bit crc with lba stamp error when encrypted.",
        3,
        {                                           // logical error injection record
            (1 << CLIFFORD_POS_TO_FAIL), 0x6, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x2 * 2, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        },
        {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {NULL, FBE_U32_MAX,}, /* Terminator */
};


/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/



/*!**************************************************************
 * clifford_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the Clifford test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void clifford_setup(void)
{
    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_init_rg_configuration_array(&clifford_raid_group_config[0]);

#if 0 // Uncomment this to use a drive that is exactly the same size as the private space.
        // Load the physical package and create the physical configuration. 
        elmo_physical_config();
        elmo_load_sep_and_neit();
#endif
        elmo_load_config(&clifford_raid_group_config[0],
                         CLIFFORD_TEST_LUNS_PER_RAID_GROUP,
                         CLIFFORD_TEST_CHUNKS_PER_LUN);
    
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            sep_config_load_kms(NULL);
        }
    }

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    return;
}   // End clifford_setup()

void clifford_encryption_setup(void)
{
    /* Mark that encryption is enabled before running the test.
     */
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);

    clifford_setup();
}
/*!**************************************************************
 * clifford_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the clifford test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/12/2011 - Created. Vamsi V
 *
 ****************************************************************/

void clifford_cleanup(void)
{

    fbe_test_sep_util_restore_sector_trace_level();
 
    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);
   
    if (fbe_test_util_is_simulation())
    {
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            fbe_test_sep_util_destroy_kms();
        }
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!***************************************************************************
 * clifford_enable_debug_flags_for_rg()
 *****************************************************************************
 * @brief
 *  This function sets the appropriate debug flags based on:
 *      - pvd debug enabled
 * 
 * @param rg_config_p - Pointer test configuration raid group data table
 *
 * @return void
 *
 ***************************************************************************/
static void clifford_enable_debug_flags_for_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       index;
    fbe_object_id_t pvd_object_id;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags = 0;
    fbe_object_id_t rg_object_id;

    /* Populate the raid library debug flags to the value desired
     * (from the -debug_flags command line parameter)
     */
    raid_library_debug_flags = fbe_test_sep_util_get_raid_library_debug_flags(clifford_raid_library_debug_flags);
    if (raid_library_debug_flags != 0)
    {
        /* Set the debug flags for raid groups that we are interested in.
         * Currently is the is the the 3-way non-optimal raid group.
         */
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    
        /* Set the raid library debug flags for this raid group
         */
        status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                            raid_library_debug_flags);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s set raid library debug flags to 0x%08x for rg_object_id: %d == ", 
                   __FUNCTION__, raid_library_debug_flags, rg_object_id);
    }
    /* If PVD tracing is enabled, reset the debug flags
     */
    if (clifford_enable_pvd_tracing == FBE_TRUE)
    {
        for (index = 0; index < rg_config_p->width; index++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[index].bus,
                                                                    rg_config_p->rg_disk_set[index].enclosure,
                                                                    rg_config_p->rg_disk_set[index].slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Set the PVD debug flags to the default
             */
            fbe_test_sep_util_provision_drive_set_debug_flag(pvd_object_id, clifford_pvd_debug_flags);
        }
    }

    return;
}
/* end of clifford_enable_debug_flags_for_rg() */

/*!***************************************************************************
 * clifford_clear_debug_flags_for_rg()
 *****************************************************************************
 * @brief
 *  This function clears the appropriate debug flags based on:
 *      - pvd debug enabled
 * 
 * @param rg_config_p - Pointer test configuration raid group data table
 *
 * @return void
 *
 ***************************************************************************/
static void clifford_clear_debug_flags_for_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       index;
    fbe_object_id_t pvd_object_id;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags = 0;
    fbe_object_id_t rg_object_id;

    /* Clear the raid library debug flags as required
     */
    raid_library_debug_flags = fbe_test_sep_util_get_raid_library_debug_flags(clifford_raid_library_debug_flags);
    if (raid_library_debug_flags != 0)
    {
        /* Clear the raid library debug flags
         */
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    
        /* Clear the raid library debug flags for this raid group
         */
        status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                            0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* If PVD tracing is enabled, reset the debug flags
     */
    if (clifford_enable_pvd_tracing == FBE_TRUE)
    {
        for (index = 0; index < rg_config_p->width; index++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[index].bus,
                                                                    rg_config_p->rg_disk_set[index].enclosure,
                                                                    rg_config_p->rg_disk_set[index].slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Set the PVD debug flags to the default
             */
            fbe_test_sep_util_provision_drive_set_debug_flag(pvd_object_id, 0);
        }
    }

    return;
}
/* end of clifford_clear_debug_flags_for_rg() */


/*!***************************************************************************
 * clifford_set_debug_flags()
 *****************************************************************************
 * @brief
 *  This function sets the appropriate debug flags based on:
 *      - raid group type - For instance only enable the flags for mirror groups
 * 
 * @param in_rg_config_p - Pointer to test configuration data table
 * @param b_set_flags - FBE_TRUE - Enable the debug flags
 *                      FBE_FALSE - Clear the debug flags
 *
 * @return void
 *
 ***************************************************************************/
void clifford_set_debug_flags(fbe_test_rg_configuration_t* in_rg_config_p,
                                     fbe_bool_t b_set_flags)
{
    fbe_u32_t                       rg_index;
    fbe_test_rg_configuration_t    *rg_config_p = in_rg_config_p;
    fbe_u32_t                       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    // Check and enabled debug flags as required
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t   temp_rg_index;

        if (!fbe_test_rg_config_is_enabled(rg_config_p))
        {
            rg_config_p ++;
            continue;
        }
        /* Determine if debug is allowed for the raid group type
         */
        for (temp_rg_index = 0; temp_rg_index < raid_group_count; temp_rg_index++)
        {
            /* Check if debug is enabled for this raid grpup type.
             */
            if (rg_config_p->raid_type == clifford_debug_enable_raid_group_types[temp_rg_index])
            {
                if (b_set_flags == FBE_TRUE)
                {
                    clifford_enable_debug_flags_for_rg(rg_config_p);
                }
                else
                {
                    clifford_clear_debug_flags_for_rg(rg_config_p);
                }
            }
        }

        /* Goto next raid group config
         */
        rg_config_p++;
    }

    return;
}
/* end of clifford_set_debug_flags() */

/*!**************************************************************
 * clifford_verify_system_luns()
 ****************************************************************
 * @brief
 *  Kick off system LUN verifies and make sure they complete without errors.
 *  We assume the system LUNs are relatively small.
 * 
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

void clifford_verify_system_luns(void)
{
    fbe_status_t status;
    fbe_private_space_layout_lun_info_t lun_info_0;
    fbe_private_space_layout_lun_info_t lun_info_1;
#ifndef __SAFE__
    fbe_private_space_layout_lun_info_t lun_info_2;
#endif
    fbe_private_space_layout_lun_info_t lun_info_raw_mirror_1;
    fbe_private_space_layout_lun_info_t lun_info_raw_mirror_2;
    fbe_lun_verify_report_t verify_report;
    fbe_object_id_t         rg_object_id;

    mut_printf(MUT_LOG_TEST_STATUS,"%s starting", __FUNCTION__);
    /* Get three small private space LUNs to kick verify off on.
     * There could be more and could be fewer LUNs here. 
     * The idea is to just verify that we do not get unexpected errors on these LUNs. 
     */
    status = fbe_private_space_layout_get_lun_by_lun_number(FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_DDBS, &lun_info_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_private_space_layout_get_lun_by_lun_number(FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_EFD_CACHE, &lun_info_1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#ifndef __SAFE__
    status = fbe_private_space_layout_get_lun_by_lun_number(FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_WIL_A, &lun_info_2);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#endif
    status = fbe_private_space_layout_get_lun_by_lun_number(FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_NP, &lun_info_raw_mirror_1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_private_space_layout_get_lun_by_lun_number(FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_SYSTEM_CONFIG, &lun_info_raw_mirror_2);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Disable system verify so that our verify report indicates pass count only for our intended verify operation.
    status = fbe_api_database_lookup_raid_group_by_number(lun_info_0.raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status =  fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_SYSTEM_VERIFY);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(lun_info_1.raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status =  fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_SYSTEM_VERIFY);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

#ifndef __SAFE__
    status = fbe_api_database_lookup_raid_group_by_number(lun_info_2.raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#endif
    status =  fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_SYSTEM_VERIFY);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(lun_info_raw_mirror_1.raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status =  fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_SYSTEM_VERIFY);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(lun_info_raw_mirror_2.raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status =  fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_SYSTEM_VERIFY);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* We use Incomplete write verify since it is higher priority than a zero, which is going on in sim on the system
     * drives.  Incomplete write verify also increments the verify report unlike error verify.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== starting Error verifies == ");
    fbe_test_sep_util_lun_clear_verify_report(lun_info_0.object_id);
    status = fbe_api_lun_initiate_verify(lun_info_0.object_id, 
                                         FBE_PACKET_FLAG_NO_ATTRIB, 
                                         FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE,
                                         FBE_TRUE, /* entire LUN */
                                         0, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_util_lun_clear_verify_report(lun_info_1.object_id);
    status = fbe_api_lun_initiate_verify(lun_info_1.object_id, 
                                         FBE_PACKET_FLAG_NO_ATTRIB, 
                                         FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE,
                                         FBE_TRUE, /* entire LUN */
                                         0, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

#ifndef __SAFE__
    fbe_test_sep_util_lun_clear_verify_report(lun_info_2.object_id);
    status = fbe_api_lun_initiate_verify(lun_info_2.object_id, 
                                         FBE_PACKET_FLAG_NO_ATTRIB, 
                                         FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE,
                                         FBE_TRUE, /* entire LUN */
                                         0, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#endif

    fbe_test_sep_util_lun_clear_verify_report(lun_info_raw_mirror_1.object_id);
    status = fbe_api_lun_initiate_verify(lun_info_raw_mirror_1.object_id, 
                                         FBE_PACKET_FLAG_NO_ATTRIB, 
                                         FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE,
                                         FBE_TRUE, /* entire LUN */
                                         0, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_util_lun_clear_verify_report(lun_info_raw_mirror_2.object_id);
    status = fbe_api_lun_initiate_verify(lun_info_raw_mirror_2.object_id, 
                                         FBE_PACKET_FLAG_NO_ATTRIB, 
                                         FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE,
                                         FBE_TRUE, /* entire LUN */
                                         0, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== waiting verify for lun %d obj: %d", FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_DDBS, lun_info_0.object_id);
    clifford_wait_for_lun_verify_completion(lun_info_0.object_id, &verify_report, 1);
    clifford_validate_verify_results(verify_report, 1, 0, 0, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "== verify complete for lun %d obj: %d", FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_DDBS, lun_info_0.object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "== waiting verify for lun %d obj: %d", FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_EFD_CACHE, lun_info_1.object_id);
    clifford_wait_for_lun_verify_completion(lun_info_1.object_id, &verify_report, 1);
    clifford_validate_verify_results(verify_report, 1, 0, 0, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "== verify complete for lun %d obj: %d", FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_EFD_CACHE, lun_info_1.object_id);

#ifndef __SAFE__
    mut_printf(MUT_LOG_TEST_STATUS, "== waiting verify for lun %d obj: %d", FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_WIL_A, lun_info_2.object_id);
    clifford_wait_for_lun_verify_completion(lun_info_2.object_id, &verify_report, 1);
    clifford_validate_verify_results(verify_report, 1, 0, 0, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "== verify complete for lun %d obj: %d", FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_WIL_A, lun_info_2.object_id);
#endif

    mut_printf(MUT_LOG_TEST_STATUS, "== waiting verify for lun %d obj: %d", 
               FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_NP, lun_info_raw_mirror_1.object_id);
    clifford_wait_for_lun_verify_completion(lun_info_raw_mirror_1.object_id, &verify_report, 1);
    clifford_validate_verify_results(verify_report, 1, 0, 0, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "== verify complete for lun %d obj: %d", 
               FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_NP, lun_info_raw_mirror_1.object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "== waiting verify for lun %d obj: %d", 
               FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_SYSTEM_CONFIG, lun_info_raw_mirror_2.object_id);
    clifford_wait_for_lun_verify_completion(lun_info_raw_mirror_2.object_id, &verify_report, 1);
    clifford_validate_verify_results(verify_report, 1, 0, 0, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "== verify complete for lun %d obj: %d", 
               FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_SYSTEM_CONFIG, lun_info_raw_mirror_2.object_id);

    mut_printf(MUT_LOG_TEST_STATUS,"%s complete", __FUNCTION__);
}
/******************************************
 * end clifford_verify_system_luns()
 ******************************************/
/*!**************************************************************
 * clifford_test_rg_config()
 ****************************************************************
 * @brief
 *  This is the entry point into the Clifford test scenario.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void clifford_test_rg_config(fbe_test_rg_configuration_t *in_rg_config_p, void * context_p)
{
    fbe_u32_t       index;
    fbe_test_rg_configuration_t    *rg_config_p = in_rg_config_p;
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Clifford Test ===");
    // Set the trace level
    fbe_test_sep_util_set_default_trace_level(FBE_TRACE_LEVEL_INFO);      

    // Set the debug flags as required
    clifford_set_debug_flags(rg_config_p, FBE_TRUE);

    // Write initial data pattern to all configured LUNs.
    // This is necessary because bind is not working yet.
    clifford_write_background_pattern();

    /* Verify a few system LUNs.
     */
    clifford_verify_system_luns();

    // Perform the write verify test on all raid groups.
    for (index = 0; index < raid_group_count; index++)
    {
        if ( fbe_test_rg_config_is_enabled(rg_config_p))
        {
            fbe_u32_t       lun_index;
            fbe_object_id_t rg_object_id;            
            fbe_test_logical_unit_configuration_t * logical_unit_configuration_p = NULL;
        
            mut_printf(MUT_LOG_TEST_STATUS, "clifford verify test RAID GROUP %d of %d.", 
                        index+1, raid_group_count);

            // Wait for all LUN's to reach background operation state.
            logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
            for (lun_index = 0; lun_index < CLIFFORD_TEST_LUNS_PER_RAID_GROUP; lun_index++)
            {
                fbe_object_id_t lun_object_id;
                fbe_lun_verify_report_t verify_report_p;
            
                fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
                MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
            
                fbe_test_sep_util_wait_for_lun_verify_completion(lun_object_id, &verify_report_p, 1);
            }

            /* Let system verify complete before we proceed. */
            fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);    
            fbe_test_verify_wait_for_verify_completion(rg_object_id, FBE_TEST_WAIT_TIMEOUT_MS, FBE_LUN_VERIFY_TYPE_SYSTEM);            
    
            switch(rg_config_p->raid_type)
            {
                case FBE_RAID_GROUP_TYPE_RAID0:
                    clifford_raid0_verify_test(index, rg_config_p);
                    clifford_newly_zeroed_verify_test(rg_config_p);
                    break;
    
                case FBE_RAID_GROUP_TYPE_RAID5:
                    clifford_raid5_verify_test(index, rg_config_p);
                    clifford_raid5_group_verify_test(rg_config_p);
                    clifford_newly_zeroed_verify_test(rg_config_p);
                    break;
                case FBE_RAID_GROUP_TYPE_RAID1:
                    clifford_raid1_verify_test(index, rg_config_p);
                    clifford_newly_zeroed_verify_test(rg_config_p);
                    break;
                case FBE_RAID_GROUP_TYPE_RAID10:
                    clifford_raid10_verify_test(index, rg_config_p);
                    clifford_newly_zeroed_verify_test(rg_config_p);
                    break;
                default:
                    MUT_ASSERT_TRUE(0);
                    break; 
            }
        }

        rg_config_p++;
    }

    // Perform error injection test on all raid groups.
    clifford_verify_with_error_injection_test(in_rg_config_p);

    // Clear the debug flags as required
    clifford_set_debug_flags(in_rg_config_p, FBE_FALSE);

}  // End clifford_test()

/*!**************************************************************
 * clifford_test()
 ****************************************************************
 * @brief
 *  This is the entry point into the Clifford test scenario.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void clifford_test(void)
{
    fbe_test_rg_configuration_t    *rg_config_p = &clifford_raid_group_config[0];

   /* Now create the raid groups and run the tests
    */
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, 
                                       clifford_test_rg_config,
                                       CLIFFORD_TEST_LUNS_PER_RAID_GROUP,
                                       CLIFFORD_TEST_CHUNKS_PER_LUN);
   
   return;
}  // End clifford_test()

/*!**************************************************************
 * clifford_crc_error_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a test where verify discovers CRC errors.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void clifford_crc_error_test_rg_config(fbe_test_rg_configuration_t *in_rg_config_p, void * context_p)
{
    fbe_test_rg_configuration_t    *rg_config_p = in_rg_config_p;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Clifford Test ===");
    // Set the trace level
    fbe_test_sep_util_set_default_trace_level(FBE_TRACE_LEVEL_INFO);      

    // Set the debug flags as required
    clifford_set_debug_flags(rg_config_p, FBE_TRUE);

    // Write initial data pattern to all configured LUNs.
    // This is necessary because bind is not working yet.
    clifford_write_background_pattern();

    // Perform error injection test on all raid groups and also check crc errors sent to PDO.
    clifford_verify_with_error_injection_and_send_crc_errs_test(in_rg_config_p);

    // Clear the debug flags as required
    clifford_set_debug_flags(in_rg_config_p, FBE_FALSE);

}
/**************************************
 * end clifford_crc_error_test_rg_config()
 **************************************/

/*!**************************************************************
 * clifford_crc_error_test()
 ****************************************************************
 * @brief
 *  Run a test where verify discovers CRC errors.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void clifford_crc_error_test(void)
{
    fbe_test_rg_configuration_t    *rg_config_p = &clifford_raid_group_config[0];

   /* Now create the raid groups and run the tests
    */
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, 
                                   clifford_crc_error_test_rg_config,
                                   CLIFFORD_TEST_LUNS_PER_RAID_GROUP,
                                   CLIFFORD_TEST_CHUNKS_PER_LUN);
   
   return;
}
/**************************************
 * end clifford_crc_error_test()
 **************************************/
/*!**************************************************************
 * clifford_crc_error_encrypted_test()
 ****************************************************************
 * @brief
 *  Run a test where verify discovers CRC errors.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void clifford_crc_error_encrypted_test(void)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t    *rg_config_p = &clifford_raid_group_config[0];

    mut_printf(MUT_LOG_TEST_STATUS, "== %s enabling encryption before creating RGs.", __FUNCTION__);
    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

   /* Now create the raid groups and run the tests
    */
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, 
                                   clifford_crc_error_test_rg_config,
                                   CLIFFORD_TEST_LUNS_PER_RAID_GROUP,
                                   CLIFFORD_TEST_CHUNKS_PER_LUN);
   
   return;
}
/**************************************
 * end clifford_crc_error_encrypted_test()
 **************************************/

void clifford_disable_sending_pdo_usurper(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_object_id_t rg_object_id;
    fbe_u32_t rg_index;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t object_id;

    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    for (rg_index = 0; rg_index < rg_config_p->width;rg_index++)
    {
        status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[rg_index].bus,
                                                                  rg_config_p->rg_disk_set[rg_index].enclosure,
                                                                  rg_config_p->rg_disk_set[rg_index].slot,
                                                                  &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%d_%d_%d obj id: 0x%x clearing pdo crc action",
                   rg_config_p->rg_disk_set[rg_index].bus,
                   rg_config_p->rg_disk_set[rg_index].enclosure,
                   rg_config_p->rg_disk_set[rg_index].slot,
                   object_id);

        status = fbe_api_physical_drive_set_crc_action(object_id, 0x0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}
/*!**************************************************************************
 * clifford_raid0_verify_test
 ***************************************************************************
 * @brief
 *  This function performs a RAID-0 verify test. 
 * 
 * @param in_index     - Index into the test configuration data table
 *
 * @return void
 *
 ***************************************************************************/
void clifford_raid0_verify_test(fbe_u32_t in_index,
                                fbe_test_rg_configuration_t * in_current_rg_config_p)
{
    fbe_object_id_t                                 lun_object_id;              // LUN object to test
    fbe_object_id_t                                 rg_object_id;               // RAID object to test
    fbe_object_id_t                                 first_vd_object_id;         // First VD object to test
    fbe_lba_t                                       start_lba;                  // where to write bad data
    fbe_u32_t                                       u_crc_count;                // number of uncorrectable CRC blocks
    fbe_u32_t                                       reported_error_count = 0;   // count of verify errors
    fbe_u32_t                                       pass_count = 0;             // number of verify passes
    fbe_u32_t                                       running_total_errors = 0;   // running total error count
    fbe_u32_t                                       index;
    fbe_u16_t                                       data_disks = 0;
    fbe_lba_t                                       imported_capacity = 0;
    fbe_test_logical_unit_configuration_t *         logical_unit_configuration_p = NULL;
    fbe_lun_verify_report_t                         verify_report[CLIFFORD_TEST_LUNS_PER_RAID_GROUP]; // the verify report data
    fbe_status_t                                    status;
    fbe_api_lun_calculate_capacity_info_t           calculate_capacity;
    fbe_api_raid_group_get_info_t                   raid_group_info;    //  raid group information from "get info" command
    fbe_u32_t                                       md_verify_pass_count;
    fbe_u32_t                                       paged_md_verify_pass_count;


    /* find number of data disks for this RG. */
    
    status = fbe_test_sep_util_get_raid_class_info(in_current_rg_config_p->raid_type,
                                                   in_current_rg_config_p->class_id,
                                                   in_current_rg_config_p->width,
                                                   in_current_rg_config_p->capacity,
                                                   &data_disks,
                                                   &imported_capacity);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get first virtual drive object-id of the RAID group. */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS,"rg_id: 0x%x  rg_obj_id: 0x%x", in_current_rg_config_p->raid_group_id, rg_object_id);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 0, &first_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, first_vd_object_id);

    /* Make sure the raid group does not shoot drives when it sees the errors since it will prevent the verifies 
     * from making progress.
     */
    clifford_disable_sending_pdo_usurper(in_current_rg_config_p);

    /* get raid group info for calculate the lun imported capacity.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &md_verify_pass_count);
    
    mut_printf(MUT_LOG_TEST_STATUS,"Entering RAID 0 Verify ");

    for (index =0; index < CLIFFORD_TEST_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
       
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);

        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        //mut_printf(MUT_LOG_TEST_STATUS, "Current lun number is %d", logical_unit_configuration_p->lun_number);

        pass_count =0;
        running_total_errors = 0;
        reported_error_count = 0;
        if (index == 0)
        {
            start_lba = 0;
        }
        else
        {
            calculate_capacity.imported_capacity = FBE_LBA_INVALID;
            calculate_capacity.exported_capacity = logical_unit_configuration_p->capacity;
            calculate_capacity.lun_align_size = raid_group_info.lun_align_size;
            status = fbe_api_lun_calculate_imported_capacity(&calculate_capacity);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INTEGER_NOT_EQUAL(FBE_LBA_INVALID, calculate_capacity.imported_capacity);
            start_lba = (calculate_capacity.imported_capacity / data_disks) * index;
        }

        // STEP 3: Inject RAID stripe errors.
        //! @todo Inject errors at the beginning and end of the LUN extent.
        u_crc_count = 4;

        clifford_write_error_pattern(first_vd_object_id, start_lba, u_crc_count);

        // Initialize the verify report data.
        fbe_test_sep_util_lun_clear_verify_report(lun_object_id);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 4: Initiate read-only verify operation.
        // - Validate that the LU verify report properly reflects the completed
        //   verify operation.
        ///////////////////////////////////////////////////////////////////////////
        
        pass_count++;
        md_verify_pass_count++;
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RO verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RO, &verify_report[index], pass_count);
        running_total_errors = u_crc_count*pass_count;

        // Print counters in the verify report
        mut_printf(MUT_LOG_TEST_STATUS,"verify report pass count: %d", verify_report[index].pass_count);
        mut_printf(MUT_LOG_TEST_STATUS,"verify report crc error count: %d", verify_report[index].current.uncorrectable_multi_bit_crc);

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);
        

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == u_crc_count);

        fbe_test_sep_util_sum_verify_results_data(&verify_report[index].current, &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == u_crc_count);

        // Validate previous result counters. They should be zero since this is the first pass.
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == 0);

        fbe_test_sep_util_sum_verify_results_data(&verify_report[index].previous, &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == 0);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == running_total_errors);

        fbe_test_sep_util_sum_verify_results_data(&verify_report[index].totals, &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 5: Initiate a write verify operation.
        //    - Validate that the LU verify report properly reflects the completed
        //      verify operation.
        //    - Verify that all correctable errors from the prior read-only verify 
        //      operation were not corrected. Since there are no correctbale errors
        //      for RAID-0, there should be no change in the error counts.
        ///////////////////////////////////////////////////////////////////////////   
        pass_count++;
        md_verify_pass_count++;
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RW verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RW, &verify_report[index],pass_count);

        // Update expected report values
        running_total_errors += u_crc_count;

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        ////MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == u_crc_count);

        fbe_test_sep_util_sum_verify_results_data(&verify_report[index].current, &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == u_crc_count);

        // Validate previous result counters
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == u_crc_count);

        fbe_test_sep_util_sum_verify_results_data(&verify_report[index].previous, &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == u_crc_count);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == running_total_errors);

        fbe_test_sep_util_sum_verify_results_data(&verify_report[index].totals, &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 6: Initiate a second write verify operation.
        //    - Validate that the LU verify report properly reflects the completed
        //      verify operation. For RAID-0 all errors should be reported as 
        //      uncorrectable CRC errors.
        //
        //    - Verify that all correctable errors from the prior write verify
        //      operation were corrected. Since there are no correctbale errors
        //      for RAID-0, there should be no change in the error counts.
        ///////////////////////////////////////////////////////////////////////////  
        pass_count++;
        md_verify_pass_count++;
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RW verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RW, &verify_report[index],pass_count);

        // Update expected report values
        running_total_errors += u_crc_count;

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        ////MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == u_crc_count);

        fbe_test_sep_util_sum_verify_results_data(&verify_report[index].current, &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == u_crc_count);

        // Validate previous result counters
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == u_crc_count);

        fbe_test_sep_util_sum_verify_results_data(&verify_report[index].previous, &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == u_crc_count);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == running_total_errors);

        fbe_test_sep_util_sum_verify_results_data(&verify_report[index].totals, &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);
        logical_unit_configuration_p++;
    }
    
    return;
}   // End clifford_raid0_verify_test()


/*!**************************************************************************
 * clifford_raid5_verify_test
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
 * @param in_index     - Index into the test configuration data table
 *
 * @return void
 *
 ***************************************************************************/
static void clifford_raid5_verify_test(fbe_u32_t in_index,
                                       fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_object_id_t                                 lun_object_id;              // LUN object to test
    fbe_object_id_t                                 rg_object_id;               // RAID object to test
    fbe_object_id_t                                 first_vd_object_id;         // First VD object to test
    fbe_object_id_t                                 second_vd_object_id;        // First VD object to test
    fbe_lba_t                                       start_lba;                  // where to write bad data
    fbe_u32_t                                       reported_error_count = 0;   // count of verify errors
    fbe_u32_t                                       expected_error_count = 0;   // number of expected errors
    fbe_u32_t                                       correctable_count = 0;      // number of correctable data blocks
    fbe_u32_t                                       uncorrectable_count =0;     // number of uncorrectable data blocks 
    fbe_u32_t                                       injected_on_first = 0;      /* number of errors injected on first position */
    fbe_u32_t                                       injected_on_second = 0;     /* number of errors injected on second position */                                                                       
    fbe_u32_t                                       pass_count = 0;             // number of verify passes
    fbe_u32_t                                       running_total_errors = 0;   // running total error count
    fbe_u32_t                                       index;
    fbe_u16_t                                       data_disks = 0;
    fbe_lba_t                                       imported_capacity = 0;
    fbe_test_logical_unit_configuration_t *         logical_unit_configuration_p = NULL;
    fbe_lun_verify_report_t                         verify_report[CLIFFORD_TEST_LUNS_PER_RAID_GROUP]; // the verify report data
    fbe_status_t                                    status;
    fbe_api_lun_calculate_capacity_info_t           calculate_capacity;
    fbe_api_raid_group_get_info_t                   raid_group_info;    //  raid group information from "get info" command
    fbe_u32_t                                       md_verify_pass_count;
    fbe_u32_t                                       paged_md_verify_pass_count;
    
    /* find number of data disks for this RG. */
    status = fbe_test_sep_util_get_raid_class_info(in_current_rg_config_p->raid_type,
                                                   in_current_rg_config_p->class_id,
                                                   in_current_rg_config_p->width,
                                                   in_current_rg_config_p->capacity,
                                                   &data_disks,
                                                   &imported_capacity);

    /* Get first virtual drive object-id of the RAID group. */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 0, &first_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(first_vd_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 1, &second_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(second_vd_object_id, FBE_OBJECT_ID_INVALID);

    /* Make sure the raid group does not shoot drives when it sees the errors since it will prevent the verifies 
     * from making progress.
     */
    clifford_disable_sending_pdo_usurper(in_current_rg_config_p);

    /* get raid group info for calculate the lun imported capacity.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &md_verify_pass_count);
        
    mut_printf(MUT_LOG_TEST_STATUS,"ENtering RAID 5 lun verify");
    for (index =0; index < CLIFFORD_TEST_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);

        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        pass_count =0;
        running_total_errors = 0;
        reported_error_count = 0;

        if (index == 0)
        {
            start_lba = 0;
        }
        else
        {
            calculate_capacity.imported_capacity = FBE_LBA_INVALID;
            calculate_capacity.exported_capacity = logical_unit_configuration_p->capacity;
            calculate_capacity.lun_align_size = raid_group_info.lun_align_size;
            status = fbe_api_lun_calculate_imported_capacity(&calculate_capacity);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INTEGER_NOT_EQUAL(FBE_LBA_INVALID, calculate_capacity.imported_capacity);
            start_lba = (calculate_capacity.imported_capacity / data_disks) * index;
        }

        /*! @note Inject the following errors on a X + 1 RAID-5 raid group:
         *          o Inject (40) single block errors into the first data position
         *          o Inject ( 5) single block errors into the second data position
         *        The result varies based on whether any errors are injected to the
         *        parity position or not.  This is due to the fact that parity is never
         *        invalidated and therefore always correctable.
         *          o Parity is the first position:
         *              + (40) Correctable errors on the first data position
         *              + ( 5) Uncorrectable errors on the second data position
         *          o No errors injected to parity ((5) blocks invalidated on both data positions)
         *              + ( 5) Uncorrectable errors on both data positions
         *              + (35) Correctable errors on first data position
         *
         *  @todo Inject errors at the beginning and end of the LUN extent.
		 */
        injected_on_first = 40;
        injected_on_second = 5;
        status = fbe_test_sep_util_get_expected_verify_error_counts(rg_object_id,
                                                                    start_lba,
                                                                    injected_on_first,
                                                                    injected_on_second,
                                                                    &correctable_count,
                                                                    &uncorrectable_count);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        expected_error_count = correctable_count + uncorrectable_count;            

        /* Inject correctable errors on VD1 */
        clifford_write_error_pattern(first_vd_object_id, start_lba, injected_on_first);

        /* Inject uncorrectable CRC errors on VD2 */
        clifford_write_error_pattern(second_vd_object_id, start_lba, injected_on_second);

        // Initialize the verify report data.
        fbe_test_sep_util_lun_clear_verify_report(lun_object_id);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 4: Initiate read-only verify operation.
        // - Validate that the LU verify report properly reflects the completed
        //   verify operation.
        ///////////////////////////////////////////////////////////////////////////
        pass_count++;
        md_verify_pass_count++;

        mut_printf(MUT_LOG_TEST_STATUS,"Initiating R0 verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);

        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RO, &(verify_report[index]), pass_count);
        running_total_errors = expected_error_count*pass_count;

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        ////MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].current), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate previous result counters. They should be zero since this is the first pass.
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == 0);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].previous), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == 0);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].totals), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 5: Initiate a write verify operation.
        //    - Validate that the LU verify report properly reflects the completed
        //      verify operation.
        //    - Verify that all correctable errors from the prior read-only verify 
        //      operation were not corrected. 
        ///////////////////////////////////////////////////////////////////////////   
        pass_count++;
        md_verify_pass_count++;
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RW verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RW, &(verify_report[index]),pass_count);

        // Update expected report values
        // Total count here should be the same as last pass since read-only verify should
        // not have fixed any errors.
        running_total_errors += expected_error_count;   

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        ////MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].current), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate previous result counters
        MUT_ASSERT_TRUE(verify_report[index].previous.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].previous), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.correctable_multi_bit_crc == correctable_count*pass_count);
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == uncorrectable_count*pass_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].totals), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 6: Initiate a second write verify operation.
        //    - Validate that the LU verify report properly reflects the completed
        //      verify operation. For RAID-0 all errors should be reported as 
        //      uncorrectable CRC errors.
        //
        //    - Verify that all correctable errors from the prior write verify
        //      operation were corrected. 
        ///////////////////////////////////////////////////////////////////////////  
        // Update expected report values
        pass_count++;
        md_verify_pass_count++;
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RW verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RW, &(verify_report[index]), pass_count);

        // Total count here should add only uncorrectables since prior write verify should
        // have fixed any correctable errors.
        running_total_errors += uncorrectable_count;

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        ////MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.correctable_multi_bit_crc == 0);
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].current), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == uncorrectable_count);

        // Validate previous result counters
        MUT_ASSERT_TRUE(verify_report[index].previous.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].previous), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.correctable_multi_bit_crc == correctable_count*(pass_count-1));
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == uncorrectable_count*pass_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].totals), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);
        logical_unit_configuration_p++;
    }

    return;

}   // End clifford_raid5_verify_test()


/*!**************************************************************
 * clifford_newly_zeroed_verify_test()
 ****************************************************************
 * @brief
 *  * Disable background zeroing so that the drive will remain zeroed.
 *  * Zero the LUN.
 *  * Inject errors across the raid group.
 *  * Start a verify.  The verify should check for zeros
 *   and realize that no verify is needed..
 *  * Make sure the verify hits no errors
 *
 * @param  rg_config_p - Current group config.               
 *
 * @return None.
 *
 * @author
 *  12/2/2011 - Created. Rob Foley
 *
 ****************************************************************/

static void clifford_newly_zeroed_verify_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t  raid_group_count = fbe_test_get_rg_count(rg_config_p);
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_test_logical_unit_configuration_t *logical_unit_configuration_p = NULL;
    fbe_u32_t index;
    fbe_lun_verify_report_t verify_report;
    fbe_u32_t reported_error_count;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Disable background zeroing for all PVDs attached to this raid group. 
     * This prevents background zeroing from occuring when we zero the LUNs. 
     * This allows us to have LUNs in a zeroed state when we kick off the verify. 
     */
    fbe_test_sep_util_rg_config_disable_zeroing(rg_config_p);

    /* Zero entire rg so that verify will have nothing to do when it runs.
     * This is needed since LUNs have a reserved area that will get background zeroed when 
     * the LUN is bound.  We don't want verify to have anything to do for this area so 
     * instead of zeroing the LUN we zero the rg. 
     */
    fbe_test_sep_util_zero_object_capacity(rg_object_id,
                                           0/* start at 0 since we want to zero entire capacity. */);

    /* Disable all the records.
     */
    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable the table that injects across all positions.
     */
    status = fbe_api_logical_error_injection_load_table(5, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = abby_cadabby_enable_error_injection_for_raid_groups(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_NOT_EQUAL(stats.num_records, 0);

    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;

    for (index = 0; index < CLIFFORD_TEST_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);

        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        /* Kick off the verify and wait for it to finish.
         */
        fbe_test_sep_util_lun_clear_verify_report(lun_object_id);
        fbe_test_sep_util_lun_config_clear_total_error_count(logical_unit_configuration_p);
        logical_unit_configuration_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Initiating verify on zeroed LUN");

    /* Start a verify on this raid group.
     */
    fbe_test_sep_util_raid_group_initiate_verify(rg_object_id, FBE_LUN_VERIFY_TYPE_USER_RW);

    /* Wait for the verifies to finish.
     */
    abby_cadabby_wait_for_verifies(rg_config_p, raid_group_count);

    /* Validate no errors were encountered.
     */
    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;
    for (index = 0; index < CLIFFORD_TEST_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);

        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        /* Sum all the errors to be sure that there are no errors.
         */
        fbe_test_sep_util_lun_get_verify_report(lun_object_id, &verify_report);
        reported_error_count = 0;
        fbe_test_sep_util_sum_verify_results_data(&verify_report.totals, &reported_error_count);
        MUT_ASSERT_INT_EQUAL(reported_error_count, 0);

        logical_unit_configuration_p++;
    }
    /* disable injection.
     */
    abby_cadabby_disable_error_injection(rg_config_p);

    /* Clear out all the records so that the table is blank.
     */
    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end clifford_newly_zeroed_verify_test()
 ******************************************/

/*!**************************************************************************
 * clifford_raid5_group_verify_test
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
 * @param in_index     - Index into the test configuration data table
 *
 * @return void
 *
 ***************************************************************************/
static void clifford_raid5_group_verify_test(fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_object_id_t                                 lun_object_id;              // LUN object to test
    fbe_object_id_t                                 rg_object_id;               // RAID object to test
    fbe_object_id_t                                 first_vd_object_id;         // First VD object to test
    fbe_object_id_t                                 second_vd_object_id;        // First VD object to test
    fbe_lba_t                                       start_lba;                  // where to write bad data
    fbe_u32_t                                       reported_error_count = 0;   // count of verify errors
    fbe_u32_t                                       expected_error_count = 0;   // number of expected errors
    fbe_u32_t                                       correctable_count = 0;      // number of correctable data blocks
    fbe_u32_t                                       uncorrectable_count =0;     // number of uncorrectable data blocks                                                                        
    fbe_u32_t                                       pass_count = 0;             // number of verify passes
    fbe_u32_t                                       index;
    fbe_u16_t                                       data_disks = 0;
    fbe_lba_t                                       imported_capacity = 0;
    fbe_test_logical_unit_configuration_t *         logical_unit_configuration_p = NULL;
    fbe_status_t                                    status;
    fbe_api_lun_calculate_capacity_info_t           calculate_capacity;
    fbe_api_raid_group_get_info_t                   raid_group_info;    //  raid group information from "get info" command
    
    /* find number of data disks for this RG. */
    status = fbe_test_sep_util_get_raid_class_info(in_current_rg_config_p->raid_type,
                                                   in_current_rg_config_p->class_id,
                                                   in_current_rg_config_p->width,
                                                   in_current_rg_config_p->capacity,
                                                   &data_disks,
                                                   &imported_capacity);

    /* Get first virtual drive object-id of the RAID group. */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 0, &first_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(first_vd_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 1, &second_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(second_vd_object_id, FBE_OBJECT_ID_INVALID);

    /* Make sure the raid group does not shoot drives when it sees the errors since it will prevent the verifies 
     * from making progress.
     */
    clifford_disable_sending_pdo_usurper(in_current_rg_config_p);

    /* get raid group info for calculate the lun imported capacity.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;

    mut_printf(MUT_LOG_TEST_STATUS, "Entering RAID 5 group based verify");

    for (index =0; index < CLIFFORD_TEST_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);

        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        pass_count =0;
        reported_error_count = 0;

        if (index == 0)
        {
            start_lba = 0;
        }
        else
        {
            calculate_capacity.imported_capacity = FBE_LBA_INVALID;
            calculate_capacity.exported_capacity = logical_unit_configuration_p->capacity;
            calculate_capacity.lun_align_size = raid_group_info.lun_align_size;
            status = fbe_api_lun_calculate_imported_capacity(&calculate_capacity);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INTEGER_NOT_EQUAL(FBE_LBA_INVALID, calculate_capacity.imported_capacity);
            start_lba = (calculate_capacity.imported_capacity / data_disks) * index;
        }

            
        // STEP 3: Inject RAID stripe errors on the first VD.
        expected_error_count = 40;
        uncorrectable_count = 5;
        correctable_count = expected_error_count - uncorrectable_count;

        // Inject errors on VD1
        clifford_write_error_pattern(first_vd_object_id, start_lba, expected_error_count);

        // Inject uncorrectable CRC errors on VD2
        clifford_write_error_pattern(second_vd_object_id, start_lba, uncorrectable_count);

        // Initialize the verify report data.
        fbe_test_sep_util_lun_clear_verify_report(lun_object_id);
        fbe_test_sep_util_lun_config_clear_total_error_count(logical_unit_configuration_p);

        logical_unit_configuration_p++;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Initiate read-only verify operation.
    // - Validate that the LU verify report properly reflects the completed
    //   verify operation.
    ///////////////////////////////////////////////////////////////////////////
    pass_count++;

    mut_printf(MUT_LOG_TEST_STATUS, "Initiating read only verify with pass count %d", pass_count);

    clifford_initiate_group_verify_cmd(in_current_rg_config_p,
                                       rg_object_id, FBE_LUN_VERIFY_TYPE_USER_RO,
                                       pass_count,
                                       expected_error_count,
                                       uncorrectable_count);

    ///////////////////////////////////////////////////////////////////////////
    // Initiate a write verify operation.
    //    - Validate that the LU verify report properly reflects the completed
    //      verify operation.
    //    - Verify that all correctable errors from the prior read-only verify 
    //      operation were not corrected. 
    ///////////////////////////////////////////////////////////////////////////   
    pass_count++;

    mut_printf(MUT_LOG_TEST_STATUS, "Initiating read write verify with pass count %d", pass_count); 

    clifford_initiate_group_verify_cmd(in_current_rg_config_p,
                                       rg_object_id, FBE_LUN_VERIFY_TYPE_USER_RW,
                                       pass_count,
                                       expected_error_count,
                                       uncorrectable_count);

    ///////////////////////////////////////////////////////////////////////////
    // Initiate a second write verify operation.
    //    - Validate that the LU verify report properly reflects the completed
    //      verify operation. For RAID-0 all errors should be reported as 
    //      uncorrectable CRC errors.
    //
    //    - Verify that all correctable errors from the prior write verify
    //      operation were corrected. 
    ///////////////////////////////////////////////////////////////////////////  
    pass_count++;

    mut_printf(MUT_LOG_TEST_STATUS, "Initiating read write verify with pass count %d", pass_count); 

    clifford_initiate_group_verify_cmd(in_current_rg_config_p,
                                       rg_object_id, FBE_LUN_VERIFY_TYPE_USER_RW,
                                       pass_count,
                                       expected_error_count,
                                       uncorrectable_count);

    mut_printf(MUT_LOG_TEST_STATUS, "Ending raid 5 group based verify");
    return;

}   // End clifford_raid5_group_verify_test()


/*!**************************************************************************
 * clifford_raid1_verify_test
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
 * @param in_index     - Index into the test configuration data table
 *
 * @return void
 *
 ***************************************************************************/
static void clifford_raid1_verify_test(fbe_u32_t in_index,
                                       fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_object_id_t                                 lun_object_id;              // LUN object to test
    fbe_object_id_t                                 rg_object_id;               // RAID object to test
    fbe_object_id_t                                 first_vd_object_id;         // First VD object to test
    fbe_object_id_t                                 second_vd_object_id;        // Second VD object to test
    fbe_lba_t                                       start_lba;                  // where to write bad data
    fbe_u32_t                                       reported_error_count = 0;   // count of verify errors
    fbe_u32_t                                       expected_error_count = 0;   // number of expected errors
    fbe_u32_t                                       correctable_count = 0;      // number of correctable data blocks
    fbe_u32_t                                       uncorrectable_count =0;     // number of uncorrectable data blocks 
    fbe_u32_t                                       injected_on_first = 0;      /* number of errors injected on first position */
    fbe_u32_t                                       injected_on_second = 0;     /* number of errors injected on second position */ 
    fbe_u32_t                                       pass_count = 0;             // number of verify passes
    fbe_u32_t                                       running_total_errors = 0;   // running total error count
    fbe_u32_t                                       index;
    fbe_u16_t                                       data_disks = 0;
    fbe_lba_t                                       imported_capacity = 0;
    fbe_test_logical_unit_configuration_t *         logical_unit_configuration_p = NULL;
    fbe_lun_verify_report_t                         verify_report[CLIFFORD_TEST_LUNS_PER_RAID_GROUP]; // the verify report data
    fbe_status_t                                    status;
    fbe_api_lun_calculate_capacity_info_t           calculate_capacity;
    fbe_api_raid_group_get_info_t                   raid_group_info;    //  raid group information from "get info" command
    fbe_u32_t                                       md_verify_pass_count;
    fbe_u32_t                                       paged_md_verify_pass_count;

    /* find number of data disks for this RG. */
    status = fbe_test_sep_util_get_raid_class_info(in_current_rg_config_p->raid_type,
                                                   in_current_rg_config_p->class_id,
                                                   in_current_rg_config_p->width,
                                                   in_current_rg_config_p->capacity,
                                                   &data_disks,
                                                   &imported_capacity);

    /* Get first virtual drive object-id of the RAID group. */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 0, &first_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(first_vd_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 1, &second_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(second_vd_object_id, FBE_OBJECT_ID_INVALID);

    /* Make sure the raid group does not shoot drives when it sees the errors since it will prevent the verifies 
     * from making progress.
     */
    clifford_disable_sending_pdo_usurper(in_current_rg_config_p);

    /* get raid group info for calculate the lun imported capacity.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &md_verify_pass_count);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Entering RAID 1 Lun verify");

    for (index =0; index < CLIFFORD_TEST_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);

        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        pass_count =0;
        running_total_errors = 0;
        reported_error_count = 0;

        if (index == 0)
        {
            start_lba = 0;
        }
        else
        {
            calculate_capacity.imported_capacity = FBE_LBA_INVALID;
            calculate_capacity.exported_capacity = logical_unit_configuration_p->capacity;
            calculate_capacity.lun_align_size = raid_group_info.lun_align_size;
            status = fbe_api_lun_calculate_imported_capacity(&calculate_capacity);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INTEGER_NOT_EQUAL(FBE_LBA_INVALID, calculate_capacity.imported_capacity);
            start_lba = (calculate_capacity.imported_capacity / data_disks) * index;
        }

        /*! @note Inject the following errors on a 1 + 1 RAID-1 raid group:
         *          o Inject (40) single block errors into the first data position
         *          o Inject ( 5) single block errors into the second data position
         *        The result is:
         *          o (35) Correctable errors on the first data position
         *          o ( 5) Uncorrectable errors on the second data position
         *
         *  @todo Inject errors at the beginning and end of the LUN extent.
         */
        injected_on_first = 40;
        injected_on_second = 5;
        status = fbe_test_sep_util_get_expected_verify_error_counts(rg_object_id,
                                                                    start_lba,
                                                                    injected_on_first,
                                                                    injected_on_second,
                                                                    &correctable_count,
                                                                    &uncorrectable_count);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        expected_error_count = correctable_count + uncorrectable_count;            

        /* Inject correctable errors on VD1 */
        clifford_write_error_pattern(first_vd_object_id, start_lba, injected_on_first);

        /* Inject uncorrectable CRC errors on VD2 */
        clifford_write_error_pattern(second_vd_object_id, start_lba, injected_on_second);

        // Initialize the verify report data.
        fbe_test_sep_util_lun_clear_verify_report(lun_object_id);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 4: Initiate read-only verify operation.
        // - Validate that the LU verify report properly reflects the completed
        //   verify operation.
        ///////////////////////////////////////////////////////////////////////////
        // Update expected report values
        pass_count++;
        md_verify_pass_count++;
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RO verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RO, &(verify_report[index]), pass_count);
        
        running_total_errors = expected_error_count*pass_count;

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        //MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].current), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate previous result counters. They should be zero since this is the first pass.
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == 0);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].previous), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == 0);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].totals), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 5: Initiate a write verify operation.
        //    - Validate that the LU verify report properly reflects the completed
        //      verify operation.
        //    - Verify that all correctable errors from the prior read-only verify 
        //      operation were not corrected. 
        ///////////////////////////////////////////////////////////////////////////   
        // Update expected report values
        pass_count++;
        md_verify_pass_count++;
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RW verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RW, &(verify_report[index]), pass_count);

        // Total count here should be the same as last pass since read-only verify should
        // not have fixed any errors.
        running_total_errors += expected_error_count;   

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        //MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].current), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate previous result counters
        MUT_ASSERT_TRUE(verify_report[index].previous.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].previous), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.correctable_multi_bit_crc == correctable_count*pass_count);
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == uncorrectable_count*pass_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].totals), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 6: Initiate a second write verify operation.
        //    - Validate that the LU verify report properly reflects the completed
        //      verify operation. For RAID-0 all errors should be reported as 
        //      uncorrectable CRC errors.
        //
        //    - Verify that all correctable errors from the prior write verify
        //      operation were corrected. 
        ///////////////////////////////////////////////////////////////////////////  
        // Update expected report values
        pass_count++;
        md_verify_pass_count++;
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RW verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RW, &(verify_report[index]), pass_count);

        // Total count here should add only uncorrectables since prior write verify should
        // have fixed any correctable errors.
        running_total_errors += uncorrectable_count;

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        //MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.correctable_multi_bit_crc == 0);
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].current), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == uncorrectable_count);

        // Validate previous result counters
        MUT_ASSERT_TRUE(verify_report[index].previous.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].previous), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.correctable_multi_bit_crc == correctable_count*(pass_count-1));
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == uncorrectable_count*pass_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].totals), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);
        logical_unit_configuration_p++;
    }

    return;

}   // End clifford_raid1_verify_test()




/*!**************************************************************************
 * clifford_raid10_verify_test
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
 * @param in_index     - Index into the test configuration data table
 *
 * @return void
 *
 ***************************************************************************/
static void clifford_raid10_verify_test(fbe_u32_t in_index,
                                       fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_object_id_t                                 lun_object_id;              // LUN object to test
    fbe_object_id_t                                 rg_object_id;               // RAID object to test
    fbe_object_id_t                                 first_vd_object_id;         // First VD object to test
    fbe_object_id_t                                 second_vd_object_id;        // Second VD object to test
    fbe_lba_t                                       start_lba;                  // where to write bad data
    fbe_u32_t                                       reported_error_count = 0;   // count of verify errors
    fbe_u32_t                                       expected_error_count = 0;   // number of expected errors
    fbe_u32_t                                       correctable_count = 0;      // number of correctable data blocks
    fbe_u32_t                                       uncorrectable_count =0;     // number of uncorrectable data blocks 
    fbe_u32_t                                       injected_on_first = 0;      /* number of errors injected on first position */
    fbe_u32_t                                       injected_on_second = 0;     /* number of errors injected on second position */ 
    fbe_u32_t                                       pass_count = 0;             // number of verify passes
    fbe_u32_t                                       running_total_errors = 0;   // running total error count
    fbe_u32_t                                       index;
    fbe_u16_t                                       data_disks = 0;
    fbe_lba_t                                       imported_capacity = 0;
    fbe_test_logical_unit_configuration_t *         logical_unit_configuration_p = NULL;
    fbe_lun_verify_report_t                         verify_report[CLIFFORD_TEST_LUNS_PER_RAID_GROUP]; // the verify report data
    fbe_status_t                                    status;
    fbe_api_lun_calculate_capacity_info_t           calculate_capacity;
    fbe_api_raid_group_get_info_t                   raid_group_info;    //  raid group information from "get info" command
    fbe_u32_t                                       md_verify_pass_count;
    fbe_u32_t                                       paged_md_verify_pass_count;

    /* find number of data disks for this RG. */
    status = fbe_test_sep_util_get_raid_class_info(in_current_rg_config_p->raid_type,
                                                   in_current_rg_config_p->class_id,
                                                   in_current_rg_config_p->width,
                                                   in_current_rg_config_p->capacity,
                                                   &data_disks,
                                                   &imported_capacity);

    /* Get first virtual drive object-id of the RAID group. */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 0, &first_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(first_vd_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 1, &second_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(second_vd_object_id, FBE_OBJECT_ID_INVALID);

    /* Make sure the raid group does not shoot drives when it sees the errors since it will prevent the verifies 
     * from making progress.
     */
    clifford_disable_sending_pdo_usurper(in_current_rg_config_p);

    /* get raid group info for calculate the lun imported capacity.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &md_verify_pass_count);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Entering RAID 10 Lun verify");

    for (index =0; index < CLIFFORD_TEST_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);

        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        pass_count =0;
        running_total_errors = 0;
        reported_error_count = 0;

        if (index == 0)
        {
            start_lba = 0;
        }
        else
        {
            calculate_capacity.imported_capacity = FBE_LBA_INVALID;
            calculate_capacity.exported_capacity = logical_unit_configuration_p->capacity;
            calculate_capacity.lun_align_size = raid_group_info.lun_align_size;
            status = fbe_api_lun_calculate_imported_capacity(&calculate_capacity);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INTEGER_NOT_EQUAL(FBE_LBA_INVALID, calculate_capacity.imported_capacity);
            start_lba = (calculate_capacity.imported_capacity / data_disks) * index;
        }

        /*! @note Inject the following errors on a X * (1 + 1) RAID-10 raid group: 
         *        Since we injected into the first (2) positions, we can treat it
         *        like a 2-way mirror.
         *          o Inject (40) single block errors into the first data position
         *          o Inject ( 5) single block errors into the second data position
         *        The result is:
         *          o (40) Correctable errors on the first data position
         *          o ( 5) Uncorrectable errors on the second data position
         *
         *  @todo Inject errors at the beginning and end of the LUN extent.
         */
        injected_on_first = 40;
        injected_on_second = 5;
        status = fbe_test_sep_util_get_expected_verify_error_counts(rg_object_id,
                                                                    start_lba,
                                                                    injected_on_first,
                                                                    injected_on_second,
                                                                    &correctable_count,
                                                                    &uncorrectable_count);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        expected_error_count = correctable_count + uncorrectable_count;            

        /* Inject correctable errors on VD1 */
        clifford_write_error_pattern(first_vd_object_id, start_lba, injected_on_first);

        /* Inject uncorrectable CRC errors on VD2 */
        clifford_write_error_pattern(second_vd_object_id, start_lba, injected_on_second);

        // Initialize the verify report data.
        fbe_test_sep_util_lun_clear_verify_report(lun_object_id);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 4: Initiate read-only verify operation.
        // - Validate that the LU verify report properly reflects the completed
        //   verify operation.
        ///////////////////////////////////////////////////////////////////////////
        // Update expected report values
        pass_count++;
        md_verify_pass_count++;

        //pass_count += 2;
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RO verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RO, &(verify_report[index]), pass_count);
        
        running_total_errors = expected_error_count*pass_count;

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        //MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].current), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate previous result counters. They should be zero since this is the first pass.
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == 0);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].previous), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == 0);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].totals), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 5: Initiate a write verify operation.
        //    - Validate that the LU verify report properly reflects the completed
        //      verify operation.
        //    - Verify that all correctable errors from the prior read-only verify 
        //      operation were not corrected. 
        ///////////////////////////////////////////////////////////////////////////   
        // Update expected report values
        pass_count++;
        md_verify_pass_count++;
        //pass_count += 2;
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RW verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RW, &(verify_report[index]), pass_count);

        // Total count here should be the same as last pass since read-only verify should
        // not have fixed any errors.
        running_total_errors += expected_error_count;   

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        //MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].current), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate previous result counters
        MUT_ASSERT_TRUE(verify_report[index].previous.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].previous), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.correctable_multi_bit_crc == correctable_count*pass_count);
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == uncorrectable_count*pass_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].totals), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);

        ///////////////////////////////////////////////////////////////////////////
        // STEP 6: Initiate a second write verify operation.
        //    - Validate that the LU verify report properly reflects the completed
        //      verify operation. For RAID-0 all errors should be reported as 
        //      uncorrectable CRC errors.
        //
        //    - Verify that all correctable errors from the prior write verify
        //      operation were corrected. 
        ///////////////////////////////////////////////////////////////////////////  
        // Update expected report values
        pass_count++;
        md_verify_pass_count++;
        //pass_count += 2;
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RW verify for lun %d with pass count %d", logical_unit_configuration_p->lun_number, pass_count);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RW, &(verify_report[index]), pass_count);

        // Total count here should add only uncorrectables since prior write verify should
        // have fixed any correctable errors.
        running_total_errors += uncorrectable_count;

        // Verify that LU verify results data properly reflects the completed verify operation.
        status = fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &paged_md_verify_pass_count);
        //MUT_ASSERT_TRUE(paged_md_verify_pass_count == md_verify_pass_count);
        MUT_ASSERT_TRUE(verify_report[index].pass_count == pass_count);

        // Validate current result counters
        MUT_ASSERT_TRUE(verify_report[index].current.correctable_multi_bit_crc == 0);
        MUT_ASSERT_TRUE(verify_report[index].current.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].current), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == uncorrectable_count);

        // Validate previous result counters
        MUT_ASSERT_TRUE(verify_report[index].previous.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(verify_report[index].previous.uncorrectable_multi_bit_crc == uncorrectable_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].previous), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        // Validate total result counters
        MUT_ASSERT_TRUE(verify_report[index].totals.correctable_multi_bit_crc == correctable_count*(pass_count-1));
        MUT_ASSERT_TRUE(verify_report[index].totals.uncorrectable_multi_bit_crc == uncorrectable_count*pass_count);

        fbe_test_sep_util_sum_verify_results_data(&(verify_report[index].totals), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == running_total_errors);
        logical_unit_configuration_p++;
    }

    return;

}   // End clifford_raid10_verify_test()

/*!**************************************************************************
 * clifford_wait_for_verify_percentage_completion
 ***************************************************************************
 * @brief
 *  This function initiates a verify operation on the specified LUN
 *  and waits for it to complete.
 * 
 * @param in_lun_object_id   - The lun to verify
 * @param in_verify_type     - type of verify operation to perform
 *
 * @return void
 *
 ***************************************************************************/
void clifford_wait_for_verify_percentage_completion(fbe_object_id_t            in_lun_object_id,
                                                    fbe_lun_verify_type_t      in_verify_type)
{
    fbe_u32_t                   sleep_time;
    fbe_api_lun_get_status_t    lun_bgverify_status;
    fbe_status_t                status;    

    for (sleep_time = 0; sleep_time < 30; sleep_time++)
    {
        status = fbe_api_lun_get_bgverify_status(in_lun_object_id,&lun_bgverify_status,in_verify_type);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if(lun_bgverify_status.percentage_completed == 100)
        {
            return;
        }

        // Sleep 100 miliseconds
        fbe_api_sleep(100);
    }

    //  The verify operation took too long and timed out.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Timeout. percentage: %d", __FUNCTION__, lun_bgverify_status.percentage_completed);
    MUT_ASSERT_TRUE(0)
    return;

}   // End clifford_wait_for_verify_percentage_completion()

/*!**************************************************************************
 * clifford_initiate_verify_cmd
 ***************************************************************************
 * @brief
 *  This function initiates a verify operation on the specified LUN
 *  and waits for it to complete.
 * 
 * @param in_lun_object_id   - The lun to verify
 * @param in_verify_type     - type of verify operation to perform
 * @param in_verify_report_p - Pointer to the verify report
 *
 * @return void
 *
 ***************************************************************************/
void clifford_initiate_verify_cmd(fbe_object_id_t            in_lun_object_id,
                                         fbe_lun_verify_type_t      in_verify_type,
                                         fbe_lun_verify_report_t*   in_verify_report_p,
                                         fbe_u32_t                  in_pass_count)
{
    fbe_api_lun_get_status_t lun_bgverify_status;
    fbe_status_t status;    
        
    // Send a user initiated verify control op to the LUN.
    fbe_test_sep_util_lun_initiate_verify(in_lun_object_id, 
                                          in_verify_type,
                                          FBE_TRUE, /* Verify the entire lun */   
                                          FBE_LUN_VERIFY_START_LBA_LUN_START,     
                                          FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN); 
         
    status = fbe_api_lun_get_bgverify_status(in_lun_object_id,&lun_bgverify_status,in_verify_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    // Not sure why we would expect percentage to be zero if the verify is in progres...
    //MUT_ASSERT_INT_EQUAL(lun_bgverify_status.percentage_completed, 0);

    // Wait for the verify to finish on this LUN
    clifford_wait_for_lun_verify_completion(in_lun_object_id, in_verify_report_p, in_pass_count);

    clifford_wait_for_verify_percentage_completion(in_lun_object_id, in_verify_type);
    return;

}   // End clifford_initiate_verify_cmd()


/*!**************************************************************************
 * clifford_initiate_group_verify_cmd
 ***************************************************************************
 * @brief
 *  This function initiates a verify operation on the specified LUN
 *  and waits for it to complete.
 * 
 * @param   in_current_rg_config_p - Pointer to raid group configuration info
 * @param   pass_count - expected pass count
 * @param   injected_on_first - number of errors injected on first position
 * @param   injected_on_second - number of errors injected on second position
 *
 * @return void
 *
 ***************************************************************************/
void clifford_initiate_group_verify_cmd(fbe_test_rg_configuration_t *in_current_rg_config_p,
                                        fbe_object_id_t              in_raid_object_id,
                                        fbe_lun_verify_type_t        in_verify_type,
                                        fbe_u32_t                    pass_count,
                                        fbe_u32_t                    injected_on_first,
                                        fbe_u32_t                    injected_on_second)
{
    fbe_api_rg_get_status_t bgverify_status;
    fbe_status_t            status; 

    // Send a user initiated verify control op to the LUN.
    fbe_test_sep_util_raid_group_initiate_verify(in_raid_object_id, in_verify_type);

    status = fbe_api_raid_group_get_bgverify_status(in_raid_object_id,&bgverify_status,in_verify_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Since verify is in progress the percentage complete could be anything 0..100. 
     * Just make sure the percentage is within the allowable range. 
     */
    if (bgverify_status.percentage_completed > 100)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "percent complete is: 0x%x", bgverify_status.percentage_completed);
        MUT_FAIL();
    }

    /*Test the API for BG Verify checkpoint percentage complete for RG*/
    clifford_wait_for_verify_and_validate_results(in_current_rg_config_p,
                                                  pass_count, injected_on_first,
                                                  injected_on_second);

    /*Test the API for BG Verify checkpoint percentage complete for RG*/
    status = fbe_api_raid_group_get_bgverify_status(in_raid_object_id,&bgverify_status,in_verify_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(bgverify_status.percentage_completed, 100);

    return;

}   // End clifford_initiate_group_verify_cmd()


/*!**************************************************************************
 * clifford_wait_for_verify_and_validate_results
 ***************************************************************************
 * @brief
 *  This function initiates a verify operation on the specified LUN
 *  and waits for it to complete.
 * 
 * @param   in_current_rg_config_p - Pointer to raid group configuration info
 * @param   pass_count - expected pass count
 * @param   injected_on_first - number of errors injected on first position
 * @param   injected_on_second - number of errors injected on second position
 *
 * @return void
 *
 ***************************************************************************/
void clifford_wait_for_verify_and_validate_results(fbe_test_rg_configuration_t* in_current_rg_config_p,                                                          
                                                   fbe_u32_t            pass_count,
                                                   fbe_u32_t            injected_on_first,
                                                   fbe_u32_t            injected_on_second)
                                                                                
{
    fbe_status_t            status;
    fbe_u32_t               index;
    fbe_lun_verify_report_t verify_report[CLIFFORD_TEST_LUNS_PER_RAID_GROUP];
    fbe_object_id_t         rg_object_id;
    fbe_object_id_t         lun_object_id;
    fbe_test_logical_unit_configuration_t *logical_unit_configuration_p = NULL;
    fbe_u32_t               correctable_count;
    fbe_u32_t               uncorrectable_count;
    fbe_u32_t               expected_error_count;
    fbe_u32_t               total_errors_for_this_pass;
    fbe_lba_t               start_lba;
    fbe_api_lun_calculate_capacity_info_t  calculate_capacity;  
    fbe_api_raid_group_get_info_t          raid_group_info;    /*  raid group information from "get info" command */       

    /* Need the raid group id.
     */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);

    /* get raid group info for calculate the lun imported capacity.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    for (index =0; index < CLIFFORD_TEST_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);

        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        mut_printf(MUT_LOG_TEST_STATUS, "Validing verify results for lun %d", 
                   logical_unit_configuration_p->lun_number);

        /* Get start physical lba of lun
         */
        if (index == 0)
        {
            start_lba = 0;
        }
        else
        {
            calculate_capacity.imported_capacity = FBE_LBA_INVALID;
            calculate_capacity.exported_capacity = logical_unit_configuration_p->capacity;
            calculate_capacity.lun_align_size = raid_group_info.lun_align_size;
            status = fbe_api_lun_calculate_imported_capacity(&calculate_capacity);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INTEGER_NOT_EQUAL(FBE_LBA_INVALID, calculate_capacity.imported_capacity);
            start_lba = (calculate_capacity.imported_capacity / raid_group_info.num_data_disk) * index;
        }

        // Wait for the verify to finish on this LUN
        clifford_wait_for_lun_verify_completion(lun_object_id, &(verify_report[index]), pass_count);

        /*! @note Inject the following errors on a X + 1 RAID-5 raid group:
         *          o Inject (40) single block errors into the first data position
         *          o Inject ( 5) single block errors into the second data position
         *        The result varies based on whether any errors are injected to the
         *        parity position or not.  This is due to the fact that parity is never
         *        invalidated and therefore always correctable.
         *          o Parity is the first position:
         *              + (40) Correctable errors on the first data position
         *              + ( 5) Uncorrectable errors on the second data position
         *          o No errors injected to parity ((5) blocks invalidated on both data positions)
         *              + ( 5) Uncorrectable errors on both data positions
         *              + (35) Correctable errors on first data position
         *
         *  @todo Inject errors at the beginning and end of the LUN extent.
         */
        status = fbe_test_sep_util_get_expected_verify_error_counts(rg_object_id,
                                                                    start_lba,
                                                                    injected_on_first,
                                                                    injected_on_second,
                                                                    &correctable_count,
                                                                    &uncorrectable_count);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        expected_error_count = correctable_count + uncorrectable_count;


        /* Update the total errors expected with expected error count.
         * For pass_count 3, we have removed the correctable errors so 
         * don't include those in the total. 
         */
        if (pass_count == 3)
        {
            total_errors_for_this_pass = uncorrectable_count;
        }
        else
        {
            total_errors_for_this_pass = expected_error_count;
        }
        status = fbe_test_sep_util_lun_config_increment_total_error_count(logical_unit_configuration_p,
                                                                          pass_count,
                                                                          total_errors_for_this_pass);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now validate all the counters
         */
        clifford_validate_verify_results(verify_report[index], pass_count,
                                         expected_error_count, uncorrectable_count, 
                                         logical_unit_configuration_p->total_error_count);

        logical_unit_configuration_p++;
    }
    
    return;

}   // End clifford_wait_for_verify_and_validate_results()


/*!**************************************************************************
 * clifford_validate_verify_results
 ***************************************************************************
 * @brief
 *  This function initiates a verify operation on the specified LUN
 *  and waits for it to complete.
 * 
 * @param in_lun_object_id   - The lun to verify
 * @param in_verify_type     - type of verify operation to perform
 *
 * @return void
 *
 ***************************************************************************/
void clifford_validate_verify_results(fbe_lun_verify_report_t in_verify_report,
                                             fbe_u32_t           pass_count,
                                             fbe_u32_t           expected_error_count,
                                             fbe_u32_t           uncorrectable_count,
                                             fbe_u32_t           running_total_errors)
{
    fbe_u32_t correctable_count =0;
    fbe_u32_t reported_error_count=0;


    correctable_count = expected_error_count - uncorrectable_count;
    
    // Verify that LU verify results data properly reflects the completed verify operation.
    MUT_ASSERT_TRUE(in_verify_report.pass_count == pass_count);


    if(pass_count == 1)
    {
        // Validate previous result counters. They should be zero since this is the first pass.
        MUT_ASSERT_TRUE(in_verify_report.previous.uncorrectable_multi_bit_crc == 0);
        fbe_test_sep_util_sum_verify_results_data(&(in_verify_report.previous), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == 0);
    }

    else
    {
        // Validate previous result counters
        MUT_ASSERT_TRUE(in_verify_report.previous.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(in_verify_report.previous.uncorrectable_multi_bit_crc == uncorrectable_count);
    
        fbe_test_sep_util_sum_verify_results_data(&(in_verify_report.previous), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);
    }


    if(pass_count == 3)
    {
        // Validate current result counters
        MUT_ASSERT_TRUE(in_verify_report.current.correctable_multi_bit_crc == 0);
        MUT_ASSERT_TRUE(in_verify_report.current.uncorrectable_multi_bit_crc == uncorrectable_count);
    
        fbe_test_sep_util_sum_verify_results_data(&(in_verify_report.current), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == uncorrectable_count);

        MUT_ASSERT_TRUE(in_verify_report.totals.correctable_multi_bit_crc == correctable_count*(pass_count - 1));

    }

    else
    {
        // Validate current result counters
        MUT_ASSERT_TRUE(in_verify_report.current.correctable_multi_bit_crc == correctable_count);
        MUT_ASSERT_TRUE(in_verify_report.current.uncorrectable_multi_bit_crc == uncorrectable_count);
    
        fbe_test_sep_util_sum_verify_results_data(&(in_verify_report.current), &reported_error_count);
        MUT_ASSERT_TRUE(reported_error_count == expected_error_count);

        MUT_ASSERT_TRUE(in_verify_report.totals.correctable_multi_bit_crc == correctable_count*pass_count);

    }

    // Validate total result counters
    MUT_ASSERT_TRUE(in_verify_report.totals.uncorrectable_multi_bit_crc == uncorrectable_count*pass_count);

    fbe_test_sep_util_sum_verify_results_data(&(in_verify_report.totals), &reported_error_count);
    MUT_ASSERT_TRUE(reported_error_count == running_total_errors);


}// End clifford_validate_verify_results



/*!**************************************************************************
 * clifford_write_background_pattern
 ***************************************************************************
 * @brief
 *  This function writes a data pattern to all the LUNs in the RAID group.
 * 
 * @param void
 *
 * @return void
 *
 ***************************************************************************/
void clifford_write_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &fbe_clifford_test_context_g[0];


    /* First fill the raid group with a pattern.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            CLIFFORD_ELEMENT_SIZE);
    /* Run our I/O test.
     */
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "clifford background pattern write for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x",
               context_p->object_id,
               context_p->start_io.statistics.pass_count,
               context_p->start_io.statistics.io_count,
               context_p->start_io.statistics.error_count);
    return;

}   // End clifford_write_background_pattern()


/*!**************************************************************************
 * clifford_write_error_pattern
 ***************************************************************************
 * @brief
 *  This function writes a data pattern to one VD in the RAID group.
 * 
 * @param in_object_id - ID of the VD object to write bad data on.
 * @param in_start_lba - start lba where bad data will be written.
 * @param in_size      - number of blocks to write.
 *
 * @return void
 *
 * @author
 *  10/22/2009 - Created. Moe Valois
 *
 ***************************************************************************/
void clifford_write_error_pattern(fbe_object_id_t in_object_id, fbe_lba_t in_start_lba, fbe_u32_t in_size)
{
    fbe_api_rdgen_context_t *context_p = &fbe_clifford_test_context_g[0];

    in_start_lba += CLIFFORD_BASE_OFFSET;

    /* Write a clear pattern to the first VD object to generate CRC errors.
     */
    clifford_setup_fill_range_rdgen_test_context(context_p,
                                            in_object_id,
                                            FBE_CLASS_ID_INVALID,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_RDGEN_PATTERN_CLEAR,
                                            in_start_lba,
                                            in_start_lba + in_size - 1,
                                            1);
    /* Run our I/O test.
     */
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "clifford write error pattern for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x",
               context_p->object_id,
               context_p->start_io.statistics.pass_count,
               context_p->start_io.statistics.io_count,
               context_p->start_io.statistics.error_count);
    return;

}   // End clifford_write_error_pattern()


/*!**************************************************************************
 * clifford_set_trace_level
 ***************************************************************************
 * @brief
 *  This function sets the FBE trace level.
 * 
 * @param in_trace_type - The type of tracing 
 * @param in_id         - Trace identifier
 * @param in_level      - Trace level
 *
 * @return void
 *
 ***************************************************************************/
void clifford_set_trace_level(fbe_trace_type_t in_trace_type, fbe_u32_t in_id, fbe_trace_level_t in_level)
{
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    level_control.trace_type = in_trace_type;
    level_control.fbe_id = in_id;
    level_control.trace_level = in_level;
    status = fbe_api_trace_set_level(&level_control, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

}   // End clifford_set_trace_level()


/*!**************************************************************
 * clifford_wait_for_lun_verify_completion()
 ****************************************************************
 * @brief
 *  This function waits for the user verify operation to complete
 *  on the specified LUN.
 *
 * @param in_object_id      - pointer to LUN object
 *
 * @return void
 *
 ****************************************************************/
void clifford_wait_for_lun_verify_completion(fbe_object_id_t            in_object_id,
                                                    fbe_lun_verify_report_t*   in_verify_report_p,
                                                    fbe_u32_t                  in_pass_count)
{
    fbe_u32_t                   sleep_time;


    for (sleep_time = 0; sleep_time < (CLIFFORD_MAX_VERIFY_WAIT_TIME_SECS*10); sleep_time++)
    {
        // Get the verify report for this LUN.
        fbe_test_sep_util_lun_get_verify_report(in_object_id, in_verify_report_p);

        if(in_verify_report_p->pass_count == in_pass_count)
        {
            return;
        }

        // Sleep 100 miliseconds
        EmcutilSleep(100);
    }

    //  The verify operation took too long and timed out.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Timeout. Exp: %d, Act: %d", __FUNCTION__, in_pass_count, in_verify_report_p->pass_count);
    MUT_ASSERT_TRUE(0)
    return;

}   // End clifford_wait_for_lun_verify_completion()


/*!**************************************************************
 * clifford_setup_fill_range_rdgen_test_context()
 ****************************************************************
 * @brief
 *  Setup the context for a fill operation.  This operation will
 *  sweep the specified area using i/os of the same size.
 *
 * @param context_p - Context structure to setup.  
 * @param object_id - object id to run io against.
 * @param class_id - class id to test.
 * @param rdgen_operation - operation to start.
 * @param pattern - data pattern
 * @param start_lba - start lba to test in blocks.
 * @param end_lba - end lba to test in blocks.
 * @param io_size_blocks - io size to fill with in blocks.             
 *
 * @return None.
 *
 * @author
 *  10/22/2009 - Created. Moe Valois
 *
 ****************************************************************/

void clifford_setup_fill_range_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                             fbe_object_id_t object_id,
                                             fbe_class_id_t class_id,
                                             fbe_rdgen_operation_t rdgen_operation,
                                             fbe_rdgen_pattern_t pattern,
                                             fbe_lba_t start_lba,
                                             fbe_lba_t end_lba,
                                             fbe_u32_t io_size_blocks)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(context_p, 
                                         object_id,
                                         class_id, 
                                         FBE_PACKAGE_ID_SEP_0,
                                         rdgen_operation,
                                         pattern,
                                         1, /* We do one full sequential pass. */
                                         0, /* num ios not used */
                                         0,    /* time not used*/
                                         1,    /* threads */
                                         FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                         start_lba,    /* Start lba*/
                                         start_lba,    /* Min lba */
                                         end_lba,      /* Max lba */
                                         FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                         io_size_blocks,    /* Min blocks */
                                         io_size_blocks     /* Max blocks */ );

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;

}   // End clifford_setup_fill_range_rdgen_test_context()


/*!**************************************************************
 * clifford_verify_with_error_injection_test()
 ****************************************************************
 * @brief
 *  Background test with error injection.
 *
 * @param None.  
 *
 * @return None.
 *
 * @author
 *  2/18/2011 - Created. chenl6
 *
 ****************************************************************/

void clifford_verify_with_error_injection_test(fbe_test_rg_configuration_t *in_rg_config_p)
{
    fbe_u32_t                       rg_index;
    fbe_test_rg_configuration_t    *rg_config_p = in_rg_config_p;
    fbe_u32_t                       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Write initial data pattern to all configured LUNs.
     */
    clifford_write_background_pattern();

    /* Perform the verify test on all raid groups. 
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(rg_config_p))
        {
    
            mut_printf(MUT_LOG_TEST_STATUS, "Clifford verify test (error injection) iteration %d of %d.", 
                      rg_index+1, raid_group_count);
            clifford_verify_with_error_injection_test_group(rg_index, rg_config_p);
        }
        rg_config_p++;
    }

    return;
}  // End clifford_verify_with_error_injection_test()

/*!**************************************************************
 * clifford_verify_with_error_injection_and_send_crc_errs_test()
 ****************************************************************
 * @brief
 *  Test verify with error injection 
 *  This function is also checking whether on crc errors injected 
 *  by logical service, CRC usurper is sent to PDO, once we set
 *  corresponding raid library debug flag.
 *
 * @param in_rg_config_p - RG Configuration.  
 *
 * @return None.
 *
 * @author
 *  2/29/2012 - Created. Swati Fursule
 *
 ****************************************************************/

void clifford_verify_with_error_injection_and_send_crc_errs_test(fbe_test_rg_configuration_t *in_rg_config_p)
{
    fbe_u32_t                       rg_index;
    fbe_test_rg_configuration_t    *rg_config_p = in_rg_config_p;
    fbe_u32_t                       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Write initial data pattern to all configured LUNs.
     */
    clifford_write_background_pattern();

    /* Perform the verify test on all raid groups. 
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(rg_config_p)) {

            mut_printf(MUT_LOG_TEST_STATUS, "Clifford test to check enabling crc usurper to PDO (with error injection) iteration %d of %d.", 
                       rg_index+1, raid_group_count);
            clifford_crc_errors_test(rg_config_p);
        }

        rg_config_p++;
    }

    return;
}
/**************************************
 * end clifford_verify_with_error_injection_and_send_crc_errs_test()
 **************************************/
/*!**************************************************************
 * clifford_verify_with_error_injection_test_group()
 ****************************************************************
 * @brief
 *  Background verify test with error injection for each RG.
 *
 * @param in_index - Index into the test configuration data table.
 * @param in_current_rg_config_p - Raid Group config.
 *
 * @return None.
 *
 * @author
 *  2/18/2011 - Created. chenl6
 *
 ****************************************************************/

void clifford_verify_with_error_injection_test_group(fbe_u32_t in_index,
                                                     fbe_test_rg_configuration_t * in_current_rg_config_p)
{
    fbe_object_id_t                                 lun_object_id;              // LUN object to test
    fbe_object_id_t                                 rg_object_id;               // RAID object to test
    fbe_u32_t                                       index = 0;
    fbe_test_logical_unit_configuration_t *         logical_unit_configuration_p = NULL;
    fbe_lun_verify_report_t                         verify_report; // the verify report data
    fbe_status_t                                    status;
    fbe_api_logical_error_injection_get_stats_t     injected_error_stats;
    clifford_logical_error_case_t *                 error_case_p;
    fbe_api_logical_error_injection_record_t        rest_of_capacity_record; 

    /* Get object-id of the RAID group. */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS,"rg_id: 0x%x  rg_obj_id: 0x%x", in_current_rg_config_p->raid_group_id, rg_object_id);

    /* Get Lun object-id. */
    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    for (index =0; index < CLIFFORD_TEST_ERROR_RECORD_COUNT; index++)
    {
        error_case_p = &clifford_error_record_array[index];

        if (error_case_p->raid_group_id != in_current_rg_config_p->raid_group_id)
        {
            continue;
        }

        mut_printf( MUT_LOG_TEST_STATUS, "Clifford: %s", error_case_p->description );

        /* Initialize the verify report data. 
         */
        fbe_test_sep_util_lun_clear_verify_report(lun_object_id);

        /* Disable all the records, if there are some already enabled.
         */
        status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Create our own error-record(s).
         */
        status = fbe_api_logical_error_injection_create_record(&error_case_p->record);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Create another record for the rest of the capacity to not inject any errors.
         * This record has a limit of 0.  This explictily says don't inject errors.
         */
        rest_of_capacity_record = error_case_p->record;
        rest_of_capacity_record.lba ++;
        rest_of_capacity_record.blocks = FBE_U32_MAX;
        rest_of_capacity_record.err_limit = 0;
        status = fbe_api_logical_error_injection_create_record(&rest_of_capacity_record);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Enable error injection for the current raid group object
         */
        status = mumford_the_magician_enable_error_injection_for_raid_group(in_current_rg_config_p->raid_type, rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_logical_error_injection_enable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Validate the injected errors are as per our configuration.
         */
        status = fbe_api_logical_error_injection_get_stats(&injected_error_stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(injected_error_stats.b_enabled, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(injected_error_stats.num_records, 2 );

        /* Initiate read-only verify operation.
         */
        mut_printf(MUT_LOG_TEST_STATUS,"Initiating RO verify for lun %d", logical_unit_configuration_p->lun_number);
        clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RO, &verify_report, 1);

        /* Verify.
         */
        clifford_validate_verify_counts(&error_case_p->report, &verify_report.current);
        clifford_validate_verify_counts(&error_case_p->report, &verify_report.totals);
        status = fbe_api_logical_error_injection_get_stats( &injected_error_stats );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
        MUT_ASSERT_TRUE( injected_error_stats.num_errors_injected == (fbe_u64_t)(error_case_p->record.err_limit) );

        /* Raid 10s only enable the mirrors, so just disable the mirror class.
         */
        if (in_current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            status = fbe_api_logical_error_injection_disable_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0);
        }
        else
        {
            status = fbe_api_logical_error_injection_disable_class(in_current_rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
        }
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_logical_error_injection_disable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}  // End clifford_verify_with_error_injection_test_group()

/*!**************************************************************
 * fbe_test_sep_util_disable_send_pdo_error()
 ****************************************************************
 * @brief
 *  Refresh all the drive slot information for a raid group configuration.
 *  This info can change over time as permanent spares get swapped in.
 *
 * @param rg_config_p - Our configuration.
 *
 * @return None.
 *
 * @author
 *  4/29/2014 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_sep_util_disable_send_pdo_error(fbe_test_rg_configuration_t *rg_config_p,
                                              fbe_raid_library_debug_flags_t new_debug_flag)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_raid_library_debug_flags_t raid_library_debug_flags = FBE_RAID_LIBRARY_DEBUG_FLAG_NONE;
    
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_api_base_config_downstream_object_list_t mirror_downstream_object_list;

        /* Get the downstream object list for this mirrored pair.  
         */ 
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &mirror_downstream_object_list);

        status = fbe_api_raid_group_get_library_debug_flags(mirror_downstream_object_list.downstream_object_list[0], 
                                                            &raid_library_debug_flags);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        raid_library_debug_flags |= new_debug_flag;
        /* Set the raid library debug flags for this raid group
          */
        status = fbe_api_raid_group_set_library_debug_flags(mirror_downstream_object_list.downstream_object_list[0], 
                                                            raid_library_debug_flags);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        status = fbe_api_raid_group_get_library_debug_flags(rg_object_id, &raid_library_debug_flags);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        raid_library_debug_flags |= new_debug_flag;
        /* Set the raid library debug flags for this raid group
          */
        status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, raid_library_debug_flags);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return;
}
/**************************************
 * end fbe_test_sep_util_disable_send_pdo_error()
 **************************************/
/*!**************************************************************
 * clifford_confirm_drives_alive()
 ****************************************************************
 * @brief
 *  Confirm that the drives in this RG are alive
 *
 * @param current_rg_config_p -
 * @param skip_position - Position to skip or FBE_U32_MAX if unused.               
 *
 * @return void. 
 *
 * @author
 *  4/30/2014 - Created. Rob Foley
 *
 ****************************************************************/

void clifford_confirm_drives_alive(fbe_test_rg_configuration_t * current_rg_config_p, fbe_u32_t skip_position)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t pvd_object_id;

    for (index = 0; index < current_rg_config_p->width; index++) {
        if (index != skip_position) {
            fbe_lifecycle_state_t lifecycle_state;
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[index].bus,
                                                                    current_rg_config_p->rg_disk_set[index].enclosure,
                                                                    current_rg_config_p->rg_disk_set[index].slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            
            status = fbe_api_get_object_lifecycle_state(pvd_object_id, &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if (lifecycle_state != FBE_LIFECYCLE_STATE_READY) {
                mut_printf(MUT_LOG_TEST_STATUS, "lifecycle state is %d != ready", lifecycle_state);
                MUT_FAIL();
            }
        }
    }
}
/******************************************
 * end clifford_confirm_drives_alive()
 ******************************************/
/*!**************************************************************
 * clifford_crc_errors_test()
 ****************************************************************
 * @brief
 *  Inject checksum errors and validate the correct behavior for the particular
 *  type of error injected and whether or not encryption is enabled.
 *
 * @param in_current_rg_config_p - Raid Group config.
 *
 * @return None.
 *
 * @author
 *  4/29/2014 - Created from from clifford_verify_with_error_injection_test_group.
 *              Rob Foley
 *
 ****************************************************************/

static void clifford_crc_errors_test(fbe_test_rg_configuration_t * current_rg_config_p)
{
    fbe_object_id_t                                 lun_object_id;    // LUN object to test
    fbe_object_id_t                                 rg_object_id;    // RAID object to test
    fbe_u32_t                                       index = 0;
    fbe_test_logical_unit_configuration_t *         logical_unit_configuration_p = NULL;
    fbe_lun_verify_report_t                         verify_report;    // the verify report data
    fbe_status_t                                    status;
    fbe_api_logical_error_injection_get_stats_t     injected_error_stats;
    clifford_logical_error_case_t *                 error_case_p;
    fbe_api_logical_error_injection_record_t        rest_of_capacity_record; 
    fbe_object_id_t                                 pvd_object_id;
    fbe_api_trace_error_counters_t                  error_counters;

    /* Get object-id of the RAID group. */
    fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS,"rg_id: 0x%x  rg_obj_id: 0x%x", current_rg_config_p->raid_group_id, rg_object_id);

    /* Get Lun object-id. */
    logical_unit_configuration_p = current_rg_config_p->logical_unit_configuration_list;
    MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    /* Set new library debug flag FBE_RAID_LIBRARY_DEBUG_FLAG_SEND_CRC_ERR_TO_PDO for raid group
     * it is set, although we inject errors, we need to send crc errors to pdo. */
    //clifford_set_new_library_debug_flag_for_raid_group(rg_object_id, FBE_RAID_LIBRARY_DEBUG_FLAG_SEND_CRC_ERR_TO_PDO);
    fbe_test_sep_util_disable_send_pdo_error(current_rg_config_p, FBE_RAID_LIBRARY_DEBUG_FLAG_SEND_CRC_ERR_TO_PDO);
    index = 0;
    while (clifford_crc_error_record_array[index].description != NULL) {
        error_case_p = &clifford_crc_error_record_array[index];

        if (error_case_p->raid_group_id != current_rg_config_p->raid_group_id) {
            index++;
            continue;
        }
        /* No sense in testing multibit with lba stamp error if we are not encrypted since it 
         * has the same result as a multibit crc error. 
         */
        if ((error_case_p->record.err_type == FBE_XOR_ERR_TYPE_MULTI_BIT_WITH_LBA_STAMP) &&
            !fbe_test_sep_util_get_encryption_test_mode()) {
            index++;
            continue;
        }
        mut_printf( MUT_LOG_TEST_STATUS, "Clifford: %s", error_case_p->description );

        /* Initialize the verify report data. 
         */
        fbe_test_sep_util_lun_clear_verify_report(lun_object_id);

        /* Disable all the records, if there are some already enabled.
         */
        status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Create our own error-record(s).
         */
        status = fbe_api_logical_error_injection_create_record(&error_case_p->record);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Create another record for the rest of the capacity to not inject any errors.
         * This record has a limit of 0.  This explictily says don't inject errors.
         */
        rest_of_capacity_record = error_case_p->record;
        rest_of_capacity_record.lba ++;
        rest_of_capacity_record.blocks = FBE_U32_MAX;
        rest_of_capacity_record.err_limit = 0;
        status = fbe_api_logical_error_injection_create_record(&rest_of_capacity_record);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Enable error injection for the current raid group object
         */
        status = mumford_the_magician_enable_error_injection_for_raid_group(current_rg_config_p->raid_type, rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_logical_error_injection_enable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Validate the injected errors are as per our configuration.
         */
        status = fbe_api_logical_error_injection_get_stats(&injected_error_stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(injected_error_stats.b_enabled, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(injected_error_stats.num_records, 2 );

        /* Initiate read-only verify operation.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "Initiating Read Only verify for lun %d", 
                   logical_unit_configuration_p->lun_number);

        /* Multi-bit crc errors cause drives to get shot when we are not encrypted.
         */
        if (error_case_p->record.err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC) {
            /* Since the drive is shot, we will not complete the verify.
             */
            fbe_test_sep_util_lun_initiate_verify(lun_object_id, 
                                                  FBE_LUN_VERIFY_TYPE_USER_RO,
                                                  FBE_TRUE,    /* Verify the entire lun */   
                                                  FBE_LUN_VERIFY_START_LBA_LUN_START,     
                                                  FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[CLIFFORD_POS_TO_FAIL].bus,
                                                                    current_rg_config_p->rg_disk_set[CLIFFORD_POS_TO_FAIL].enclosure,
                                                                    current_rg_config_p->rg_disk_set[CLIFFORD_POS_TO_FAIL].slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_TEST_STATUS, "== Confirm that pvd object: 0x%x is FAIL", pvd_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL, 
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_TEST_STATUS, "== Confirm all other drives are alive.");
             clifford_confirm_drives_alive(current_rg_config_p, CLIFFORD_POS_TO_FAIL);

            mut_printf(MUT_LOG_TEST_STATUS, "== Clear drive fault on pvd object: 0x%x", pvd_object_id);
            status = fbe_api_provision_drive_clear_drive_fault(pvd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            mut_printf(MUT_LOG_TEST_STATUS, "== Wait for pvd object: 0x%x to be READY", pvd_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        } else {

            /* We know that this particular error type will cause a trace error.
             */
            if (error_case_p->record.err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP){
                status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
                MUT_ASSERT_INT_EQUAL(error_counters.trace_error_counter, 0);
            }
            /* Single bit and multi bit with lba stamp error. */
            clifford_initiate_verify_cmd(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RO, &verify_report, 1);

            if ((error_case_p->record.err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP) &&
                (current_rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0)){
                /* Validate we received trace errors and clear out the counters.
                 * RAID-0 always does a call home for any uncorrectable error. 
                 */
                status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
                mut_printf(MUT_LOG_TEST_STATUS, "== Found %d trace errors as expected", error_counters.trace_error_counter);
                MUT_ASSERT_INT_NOT_EQUAL(error_counters.trace_error_counter, 0);
                status = fbe_api_trace_clear_error_counter(FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            mut_printf(MUT_LOG_TEST_STATUS, "== Confirm all drives are alive.");
            clifford_confirm_drives_alive(current_rg_config_p, FBE_U32_MAX);
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for the raid group to become ready rg: %d object: 0x%x",
                   current_rg_config_p->raid_group_id, rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Verifies to finish on rg: %d object: 0x%x",
                   current_rg_config_p->raid_group_id, rg_object_id);
        abby_cadabby_wait_for_verifies(current_rg_config_p, 1);

        /* Validate that the correct errors were injected/detected.
         */
        clifford_validate_verify_counts(&error_case_p->report, &verify_report.current);
        clifford_validate_verify_counts(&error_case_p->report, &verify_report.totals);
        status = fbe_api_logical_error_injection_get_stats( &injected_error_stats );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
        MUT_ASSERT_TRUE( injected_error_stats.num_errors_injected == (fbe_u64_t)(error_case_p->record.err_limit) );

        /* Raid 10s only enable the mirrors, so just disable the mirror class.
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            status = fbe_api_logical_error_injection_disable_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0);
        } else {
            status = fbe_api_logical_error_injection_disable_class(current_rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
        }
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_logical_error_injection_disable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        index++;
    }

    return;
}
/**************************************
 * end clifford_crc_errors_test()
 **************************************/

/*!**************************************************************
 * clifford_validate_verify_counts()
 ****************************************************************
 * @brief
 *  Background verify test with error injection for each RG.
 *
 * @param expected_counts - expected error counts.
 * @param verify_counts - error counts to be verified.
 *
 * @return None.
 *
 * @author
 *  2/22/2011 - Created. chenl6
 *
 ****************************************************************/
void clifford_validate_verify_counts(fbe_lun_verify_counts_t *expected_counts,
                                   fbe_lun_verify_counts_t *verify_counts)
{
    MUT_ASSERT_INT_EQUAL(verify_counts->correctable_single_bit_crc, expected_counts->correctable_single_bit_crc);
    MUT_ASSERT_INT_EQUAL(verify_counts->uncorrectable_single_bit_crc, expected_counts->uncorrectable_single_bit_crc);
    MUT_ASSERT_INT_EQUAL(verify_counts->correctable_multi_bit_crc, expected_counts->correctable_multi_bit_crc);
    MUT_ASSERT_INT_EQUAL(verify_counts->uncorrectable_multi_bit_crc, expected_counts->uncorrectable_multi_bit_crc);
    MUT_ASSERT_INT_EQUAL(verify_counts->correctable_write_stamp, expected_counts->correctable_write_stamp);
    MUT_ASSERT_INT_EQUAL(verify_counts->uncorrectable_write_stamp, expected_counts->uncorrectable_write_stamp);
    MUT_ASSERT_INT_EQUAL(verify_counts->correctable_time_stamp, expected_counts->correctable_time_stamp);
    MUT_ASSERT_INT_EQUAL(verify_counts->uncorrectable_time_stamp, expected_counts->uncorrectable_time_stamp);
    MUT_ASSERT_INT_EQUAL(verify_counts->correctable_shed_stamp, expected_counts->correctable_shed_stamp);
    MUT_ASSERT_INT_EQUAL(verify_counts->uncorrectable_shed_stamp, expected_counts->uncorrectable_shed_stamp);
    MUT_ASSERT_INT_EQUAL(verify_counts->correctable_lba_stamp, expected_counts->correctable_lba_stamp);
    MUT_ASSERT_INT_EQUAL(verify_counts->uncorrectable_lba_stamp, expected_counts->uncorrectable_lba_stamp);
    MUT_ASSERT_INT_EQUAL(verify_counts->correctable_coherency, expected_counts->correctable_coherency);
    MUT_ASSERT_INT_EQUAL(verify_counts->uncorrectable_coherency, expected_counts->uncorrectable_coherency);
    MUT_ASSERT_INT_EQUAL(verify_counts->correctable_media, expected_counts->correctable_media);
    MUT_ASSERT_INT_EQUAL(verify_counts->uncorrectable_media, expected_counts->uncorrectable_media);
    MUT_ASSERT_INT_EQUAL(verify_counts->correctable_soft_media, expected_counts->correctable_soft_media);

    return;
}  // End clifford_validate_verify_counts()

/*!**************************************************************
 * clifford_set_new_library_debug_flag_for_raid_group()
 ****************************************************************
 * @brief
 *  For a given raid group, set new library debug flag.
 *
 * @param new_debug_flag    -new library debug flag
 * @param rg_object_id - Object id to which flag needs to set
 *
 * @return fbe_status_t 
 *
 * @author
 *  3/1/2012 - Created. Swati Fursule
 *
 ****************************************************************/
void clifford_set_new_library_debug_flag_for_raid_group(fbe_object_id_t rg_object_id,
                                                    fbe_raid_library_debug_flags_t new_debug_flag)
{
    fbe_status_t status;
    fbe_raid_library_debug_flags_t raid_library_debug_flags = FBE_RAID_LIBRARY_DEBUG_FLAG_NONE;

    status = fbe_api_raid_group_get_library_debug_flags(rg_object_id, 
                                               &raid_library_debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    raid_library_debug_flags |= new_debug_flag;
   /* Set the raid library debug flags for this raid group
     */
    status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                        raid_library_debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/********************************************************************
 * end clifford_set_new_library_debug_flag_for_raid_group()
 *******************************************************************/
/*!**************************************************************
 * clifford_clear_new_library_debug_flag_for_raid_group()
 ****************************************************************
 * @brief
 *  For a given raid group, set new library debug flag.
 *
 * @param new_debug_flag    -new library debug flag
 * @param rg_object_id - Object id to which flag needs to set
 *
 * @return fbe_status_t 
 *
 * @author
 *  3/1/2012 - Created. Swati Fursule
 *
 ****************************************************************/
void clifford_clear_new_library_debug_flag_for_raid_group(fbe_object_id_t rg_object_id,
                                                    fbe_raid_library_debug_flags_t new_debug_flag)
{
    fbe_status_t status;
    fbe_raid_library_debug_flags_t raid_library_debug_flags = FBE_RAID_LIBRARY_DEBUG_FLAG_NONE;

    status = fbe_api_raid_group_get_library_debug_flags(rg_object_id, 
                                               &raid_library_debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    raid_library_debug_flags &= ~new_debug_flag;
   /* Set the raid library debug flags for this raid group
     */
    status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                        raid_library_debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/********************************************************************
 * end clifford_clear_new_library_debug_flag_for_raid_group()
 *******************************************************************/
/*************************
 * end file clifford_test.c
 *************************/


