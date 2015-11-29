/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file lefty_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains tests to validate unexpected error path
 *
 * @version
 *   07/10/2010:  Created. Jyoti Ranjan
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
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe_private_space_layout_generated.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * lefty_short_desc = "Validates handling of unexpected error path for raid library";
char * lefty_long_desc = "\n\
The lefty scenario validates handling of unexpected error path in raid library.\n\
This includes:\n\
        - striper(r0), mirror (r10), parity unit (r5, r3, r6) testing for unexpected error path.\n\
        - Support for 520 byte per block SAS and 512 byte per block SATA.\n\
\n\
\n\
-raid_test_level 0 and 1\n\
   - We test additional combinations of raid group widths and 512 block size at -rtl 1.\n\
\n\
 STEP 1:  Load SEP, NEIT and Physical package before starting of each test.\n\
 STEP 2:  Create Raid group and LUN configuration.\n\
 STEP 3:  Enable unexpected error-testing path for RAID Library.\n\
 STEP 4:  Power down the drives if count is set in config table.\n\
 STEP 5:  Generate rdgen IO from config table parameters and start it for the given duration.\n\
 STEP 6:  Monitor the error testing stats periodically.\n\
 STEP 7:  Stop rdgen.\n\
 STEP 8:  Validate rdgen error counter with following - \n\
          - none of I/O should have been aborted\n\
          - none of the blocks should have detected CRC while performing read\n\
 STEP 8:  Insert all drives if removed during the test and wait for the rebuild to complete.\n\
 STEP 9:  Reset error-testing stats and disable unexpected error-testing path for RAID Library.\n\
 STEP 10: Destroy SEP, NEIT and Physical package.\n\
 STEP 11: Continue for the next test from config table.\n\
\n\
Test Specific Switches:\n\
          - none.\n\
\n\
Outstanding Issues:\n\
    - Raid 6 single degraded test is disabled in extended test mode.\n\
    - Raid 6 double degraded test is disabled in extended test mode.\n\
    - Need to remove usage of SATA drives.\n\
    - Need code cleanup to remove unused function.\n\
\n\
Description last updated: 10/13/2011.\n";


/************************
 * LITERAL DECLARARION
 ***********************/

/*!*******************************************************************
 * @def LEFTY_TEST_ELEMENT_SIZES
 *********************************************************************
 * @brief number of element sizes to test.
 *
 *********************************************************************/
#define LEFTY_TEST_ELEMENT_SIZES 1



/*!*******************************************************************
 * @def LEFTY_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief number of LUNs in our raid group.
 *
 *********************************************************************/
#define LEFTY_TEST_LUNS_PER_RAID_GROUP  1


/*!*******************************************************************
 * @def LEFTY_CHUNKS_PER_LUN
 *********************************************************************
 * @brief number of chunks in our lun.
 *
 *********************************************************************/
#define LEFTY_CHUNKS_PER_LUN  3


/*!*******************************************************************
 * @def LEFTY_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief maximum number of raid groups we will test with.
 *
 *********************************************************************/
#define LEFTY_TEST_RAID_GROUP_COUNT  FBE_TEST_MAX_RAID_GROUP_COUNT



/*!*******************************************************************
 * @def LEFTY_TEST_MAX_TEST_CASE
 *********************************************************************
 * @brief maximum number of different test cases to use against a 
 *        given cofiguration
 *
 *********************************************************************/
#define LEFTY_TEST_MAX_TEST_CASE  15



/*!*******************************************************************
 * @def LEFTY_MAX_CONFIGS
 *********************************************************************
 * @brief maximum number of configurations we will test in any 
 *        mode (qual or extended)
 *
 *********************************************************************/
#define LEFTY_MAX_CONFIGS  15

/*!*******************************************************************
 * @def LEFTY_MAX_CONFIGS_FOR_EXTENDED_MODE
 *********************************************************************
 * @brief The number of configurations we will test in extended mode.
 *
 *********************************************************************/
#define LEFTY_MAX_CONFIGS_FOR_EXTENDED_MODE  LEFTY_MAX_CONFIGS

/*!*******************************************************************
 * @def LEFTY_TEST_MAX_RAID_LIB_ERROR_TESTING_LEVEL
 *********************************************************************
 * @brief Max number of unexpected error-testing level 
 *
 *********************************************************************/
#define LEFTY_TEST_MAX_RAID_LIB_ERROR_TESTING_LEVEL 4



 /*********************************************************************
  * @brief literal declaration for timeout period (in sec) for different 
  *  test levels
  *
  *********************************************************************/
#define LEFTY_TEST_ERROR_TESTING_PERIOD_FOR_LEVEL_0  60
#define LEFTY_TEST_ERROR_TESTING_PERIOD_FOR_LEVEL_1  300
#define LEFTY_TEST_ERROR_TESTING_PERIOD_FOR_LEVEL_2  600
#define LEFTY_TEST_ERROR_TESTING_PERIOD_FOR_LEVEL_3  1500



/*!*******************************************************************
 * @def LEFTY_TEST_RAID_LIB_ERROR_TESTING_POLLING_INTERVAL
 *********************************************************************
 * @brief polling interval to query the error testing stats
 *
 *********************************************************************/
#define LEFTY_TEST_RAID_LIB_ERROR_TESTING_POLLING_INTERVAL  1000



 /*!*******************************************************************
 * @def LEFTY_TEST_RAID_LIB_ERROR_TESTING_ERROR_STATS_SATURATION_COUNT
 **********************************************************************
 * @brief maximum consecutive poll count to wait before timeout if 
 *        error stats do not change
 *
 *********************************************************************/
#define LEFTY_TEST_RAID_LIB_ERROR_TESTING_ERROR_STATS_SATURATION_COUNT  2





/*!*******************************************************************
 * @struct fbe_lefty_io_test_case_t
 *********************************************************************
 * @brief Defines a set of test parameters for a test case
 *
 *********************************************************************/
typedef struct fbe_lefty_io_test_case_s
{
    fbe_rdgen_operation_t rdgen_opcode;          /*!< operation code */
    fbe_u32_t max_blocks;                        /*!< maximum blocks */
    fbe_u32_t duration_secs;                     /*!< maximum duration for which test case should be run */
    fbe_bool_t b_non_cached;
}
fbe_lefty_io_test_case_t;





/*!*******************************************************************
 * @struct fbe_lefty_test_configuration_t
 *********************************************************************
 * @brief Defines configuration attributes against which test case
 *        will be executed
 *
 *********************************************************************/
typedef struct fbe_lefty_test_configuration_s
{
    fbe_test_rg_configuration_t raid_group_config[FBE_TEST_MAX_RAID_GROUP_COUNT]; /*!< raid group configuration */
    fbe_u32_t num_drives_to_power_down;  /*!< maximum number of drives to power off */
}
fbe_lefty_test_configuration_t;

/************************
 * DATA DEFINITION
 ***********************/


/*!*******************************************************************
 * @var lefty_rdgen_contexts
 *********************************************************************
 * @brief rdgen context to be used
 *
 ********************************************************************/
static fbe_api_rdgen_context_t lefty_rdgen_contexts[LEFTY_TEST_LUNS_PER_RAID_GROUP];

/*!*******************************************************************
 * @var lefty_raid_lib_error_testing_period
 *********************************************************************
 * @brief Defines timeout period (in msec) used for various test levels
 *
 *********************************************************************/
fbe_u32_t lefty_raid_lib_error_testing_period[LEFTY_TEST_MAX_RAID_LIB_ERROR_TESTING_LEVEL]  = 
{ 
    LEFTY_TEST_ERROR_TESTING_PERIOD_FOR_LEVEL_0,
    LEFTY_TEST_ERROR_TESTING_PERIOD_FOR_LEVEL_1,
    LEFTY_TEST_ERROR_TESTING_PERIOD_FOR_LEVEL_2,
    LEFTY_TEST_ERROR_TESTING_PERIOD_FOR_LEVEL_3
};

/*!*******************************************************************
 * @var lefty_rdgen_io_test_case_qual
 *********************************************************************
 * @brief This is the array of test cases for qual (level 0)
 *
 *********************************************************************/
fbe_lefty_io_test_case_t  lefty_rdgen_io_test_case_qual[LEFTY_TEST_MAX_TEST_CASE] = 
{
    /*  rdgen opcode,                               max blocks, duration (sec) non_cached?  */
    {   FBE_RDGEN_OPERATION_ZERO_READ_CHECK,            1024,        5,      FBE_FALSE},
    {   FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK,      1024,        5,      FBE_FALSE},
    {   FBE_RDGEN_OPERATION_WRITE_READ_CHECK,           1024,        5,      FBE_FALSE},
    {   FBE_RDGEN_OPERATION_WRITE_READ_CHECK,           1024,        5,      FBE_FALSE},
    {   FBE_RDGEN_OPERATION_WRITE_READ_CHECK,           1024,        5,      FBE_TRUE},
    {   FBE_RDGEN_OPERATION_VERIFY_WRITE,               1024,        5,      FBE_FALSE}, 
    {   FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK,    1024,        5,      FBE_FALSE},
    {   FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK,     1024,        5,      FBE_FALSE},
    {   FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY,          1024,        5,      FBE_FALSE}, 
    {   FBE_RDGEN_OPERATION_INVALID,   FBE_U32_MAX},
};

/*!*******************************************************************
 * @var lefty_rdgen_io_test_case_exended
 *********************************************************************
 * @brief This is the array of test cases for extended level 
 *        (level 1 and above)
 *
 *********************************************************************/
fbe_lefty_io_test_case_t  lefty_rdgen_io_test_case_extended[LEFTY_TEST_MAX_TEST_CASE] = 
{
    /*  rdgen opcode,                               max blocks, duration (sec)*/
    {   FBE_RDGEN_OPERATION_ZERO_READ_CHECK,            1024,        15,      FBE_FALSE},
    {   FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK,      1024,        15,      FBE_FALSE},
    {   FBE_RDGEN_OPERATION_WRITE_READ_CHECK,           1024,        15,      FBE_FALSE},
    {   FBE_RDGEN_OPERATION_WRITE_READ_CHECK,           1024,        15,      FBE_TRUE},
    {   FBE_RDGEN_OPERATION_VERIFY_WRITE,               1024,        15,      FBE_FALSE}, 
    {   FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK,    1024,        15,      FBE_FALSE},
    {   FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK,     1024,        15,      FBE_FALSE},
    {   FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY,          1024,        15,      FBE_FALSE}, 
    {   FBE_RDGEN_OPERATION_INVALID,   FBE_U32_MAX},
};

/*!*******************************************************************
 * @var lefty_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations for qual (level 0)
 *
 *********************************************************************/
fbe_test_logical_error_table_test_configuration_t lefty_test_config_for_qual_mode[] = 
{
    {
        {     /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
              {1,       0x8000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,             2,          0},
              {5,       0xF000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,             4,          0},
              {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {0},
    },
    {
        {     /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
              {2,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,             6,          0},
              {2,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,             8,          0},
              {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {0},
    },
    {
        {     /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
              {4,       0x8000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,             2,          0},
              {16,       0xF000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,             4,          0},
              {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {0},
    },
    {
        {     /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
              {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            10,          0},
              {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            12,          0},
              {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {0},
    },
    {
        {     /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
              {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            14,          0},
              {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            16,          0},
              {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {0},
    },
    {
        {     /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
              {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            18,          0},
              {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            20,          0},
              {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {0},
    },
    { 
        {   
            { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        }
    }
};


/*!*******************************************************************
 * @var lefty_test_config_for_extended_mode
 *********************************************************************
 * @brief This is the array of configurations we will loop over for 
 *        extended mode (level 1 and above)
 *
 *********************************************************************/
fbe_test_logical_error_table_test_configuration_t lefty_test_config_for_extended_mode[] = 
{
    /* RAID 0 : block size = 520 and 512 bytes only */
    {
        {
            /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
            {1,       0x8000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            4,          0},
            {3,       0x9000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            5,          0},
            {5,       0xF000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            6,          0},
            {16,      0xF000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            7,          0},
        
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {0},
    },

    {
        /* RAID 1 : non degraded, block size = 520 and 512 bytes only */
        {
            /* width,   capacity    raid type,                  class,               block size      RAID-id.    bandwidth.*/
            {2,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            9,          0},
            {3,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,           10,          0},
            {2,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,           11,          1},
            {3,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,           12,          1},

            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
         },
        {0},
        {0},
    },
    {
        {     /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
              {4,       0x8000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,             2,          0},
              {16,       0xF000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,             4,          0},
              {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {0},
    },

    {
        /* RAID 3, RAID 5 and RAID 6: non-degraded, block size = 520 and 512 bytes only */
        {
            /* width,   capacity    raid type,                  class,               block size      RAID-id.    bandwidth.*/
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            17,          0},
            {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            18,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {0},
    },

    {
        /* RAID 3, RAID 5 and RAID 6: non-degraded, block size = 520 and 512 bytes only */
        {
            /* width,   capacity    raid type,                  class,               block size      RAID-id.    bandwidth.*/
            {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            21,          0},
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            22,          0},
            {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            23,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            24,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {0},
    },
    {
        /* RAID 3, RAID 5 and RAID 6: non-degraded, block size = 520 and 512 bytes only */
        {
            /* width,   capacity    raid type,                  class,               block size      RAID-id.    bandwidth.*/
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            25,          0},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            26,          0},
            {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            27,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            28,          0},

            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {0},
    },
    {
        /* RAID 1 : single-degraded, block size = 520 bytes only */
        {
            /* width,   capacity    raid type,                  class,               block size      RAID-id.    bandwidth.*/
            {2,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            41,          0},
            {3,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            42,          0},
            {2,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            43,          0},
            {3,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            44,          0},

            {2,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            45,          0},
            {3,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            46,          0},
            {2,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            47,          0},
            {3,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            48,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
         },
         
       // 1, - DE452(track to enable this test code)
        {0},
        {0},
    },


    {
        /* RAID 3 and RAID 5 : single-degraded, block size = 520 bytes only */
        {
            /* width,   capacity    raid type,                  class,              block size      RAID-id.    bandwidth.*/
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            49,          0},
            {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            50,          0},
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            51,          0},
            {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            52,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
       // 1, - DE452(track to enable this test code)
        {0},
        {0},
    },


    {
        /* RAID 3 and RAID 5 : single-degraded, block size = 520 bytes only */
        {
            /* width,   capacity    raid type,                  class,              block size      RAID-id.    bandwidth.*/
            {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            53,          0},
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            54,          0},
            {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            55,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            56,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
       // 1, - DE452(track to enable this test code)
        {0},
        {0},
    },

#if 0
    {
        /* RAID 6 : single-degraded, block size = 520 bytes only */
        {
           /* width,   capacity    raid type,                  class,                block size      RAID-id.    bandwidth.*/
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            65,          0},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            66,          0},
            {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            67,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            68,          0},
        
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            69,          0},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            70,          0},
            {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            71,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            72,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {1},
    },

    {
        /* RAID 6 : double-degraded, block size = 520 bytes only */
        {
            /* width,   capacity    raid type,                  class,               block size      RAID-id.    bandwidth.*/
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            73,          0},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            74,          0},
            {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            75,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            76,          0},
        
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            78,          0},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            79,          0},
            {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            80,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            81,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0},
        {2},
    },

#endif

    { 
        {   
            { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        }
    }
};


/************************
 * FUNCTION DECLARATION
 ***********************/
void lefty_setup(void);
fbe_bool_t lefty_get_cmd_option_int(char *option_name_p, fbe_u32_t *value_p);
fbe_u32_t lefty_get_config_count(fbe_test_logical_error_table_test_configuration_t * config_array_p);
fbe_u32_t lefty_get_test_case_count(fbe_lefty_io_test_case_t *test_case_p);
fbe_u32_t lefty_get_raid_group_count(fbe_test_logical_error_table_test_configuration_t * config_p);
fbe_status_t lefty_test_setup_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                 fbe_object_id_t object_id,
                                                 fbe_class_id_t class_id,
                                                 fbe_rdgen_operation_t rdgen_operation,
                                                 fbe_lba_t capacity,
                                                 fbe_u32_t max_passes,
                                                 fbe_u32_t threads,
                                                 fbe_u32_t io_size_blocks);
fbe_status_t lefty_wait_for_test_case_completion(fbe_u32_t target_wait_msec,
                                                 fbe_u32_t max_iteration,
                                                 fbe_u32_t *actual_wait_msec_p);
void lefty_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
fbe_status_t lefty_run_single_test_case(fbe_lefty_io_test_case_t * test_case_p,
                                        fbe_u32_t * traced_paths_p,
                                        fbe_u32_t * test_duration_p);
void lefty_validate_rdgen_io_statistics(fbe_rdgen_statistics_t * rdgen_statistics_p);
void lefty_cleanup(void);

/*!**************************************************************
 * lefty_get_cmd_option_int()
 ****************************************************************
 * @brief
 *  Return an option value that is an integer.
 *
 * @param option_name_p - name of option to get
 * @param value_p - pointer to value to return.
 *
 * @return fbe_bool_t - FBE_TRUE if value found FBE_FALSE otherwise.
 *
 ****************************************************************/
fbe_bool_t lefty_get_cmd_option_int(char *option_name_p, fbe_u32_t *value_p)
{
    char *value = mut_get_user_option_value(option_name_p);
    if (value != NULL)
    {
        *value_p = atoi(value);
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/******************************************
 * end lefty_get_cmd_option_int()
 ******************************************/


/*!**************************************************************
 * lefty_get_config_count()
 ****************************************************************
 * @brief
 *  Get the number of configs we will be working with.
 *  Loops until it hits the terminator.
 *
 * @param config_p - array of configs
 *
 * @return fbe_u32_t  - number of configs
 *
 * @author
 *  7/21/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_u32_t lefty_get_config_count(fbe_test_logical_error_table_test_configuration_t * config_array_p)
{
    fbe_u32_t config_count = 0;

    /* Loop until we find the table number or we hit the terminator.
     */
    while (config_array_p->raid_group_config[0].width != FBE_U32_MAX)
    {
        config_count++;
        config_array_p++;
    }

    return config_count;
}
/******************************************
 * end lefty_get_config_count()
 ******************************************/
/*!**************************************************************
 * lefty_get_test_case_count()
 ****************************************************************
 * @brief
 *  Get the number of test cases we will be working with.
 *  Loops until it hits the terminator.
 *
 * @param  test_case_p - array of test cases.
 *
 * @return fbe_u32_t  - number of test cases.
 *
 * @author
 *  7/21/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_u32_t lefty_get_test_case_count(fbe_lefty_io_test_case_t *test_case_p)
{
    fbe_u32_t test_case_count = 0;

    /* Loop until we find the table number or we hit the terminator.
     */
    while (test_case_p->rdgen_opcode != FBE_RDGEN_OPERATION_INVALID)
    {
        test_case_count++;
        test_case_p++;
    }

    return test_case_count;
}
/******************************************
 * end lefty_get_test_case_count()
 ******************************************/

/*!**************************************************************
 * lefty_get_raid_group_count()
 ****************************************************************
 * @brief
 *  Get the number of raid groups in a given configuration
 *  Loops until it hits the terminator.
 *
 * @param  config_p - configuration containing raid group details
 *
 * @return fbe_u32_t 0 number of raid groups for given configuration
 *
 * @author
 *  7/21/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_u32_t lefty_get_raid_group_count(fbe_test_logical_error_table_test_configuration_t * config_p)
{
    fbe_test_rg_configuration_t *raid_group_config_p = NULL;
    fbe_u32_t raid_group_count = 0;

    /* Loop until we hit the terminator.
     */
    raid_group_config_p = &config_p->raid_group_config[0];
    while (raid_group_config_p->width != FBE_U32_MAX)
    {
        raid_group_count++;
        raid_group_config_p++;
    }

    return raid_group_count;
}
/*********************************************
 * end lefty_get_raid_group_count()
 ********************************************/
/*!**************************************************************
 * lefty_test_setup_rdgen_test_context()
 ****************************************************************
 * @brief
 *  Setup the context structure to run a random I/O test.
 *
 * @param context_p       - context of the operation.
 * @param object_id       - object id to run io against.
 * @param class_id        - class id to test.
 * @param rdgen_operation - operation to start.
 * @param capacity        - capacity in blocks.
 * @param max_passes      - number of passes to execute.
 * @param io_size_blocks  - I/O size to fill with in blocks.
 * 
 * @return fbe_status_t
 *
 * @author
 *  07/10/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t lefty_test_setup_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                 fbe_object_id_t object_id,
                                                 fbe_class_id_t class_id,
                                                 fbe_rdgen_operation_t rdgen_operation,
                                                 fbe_lba_t capacity,
                                                 fbe_u32_t max_passes,
                                                 fbe_u32_t threads,
                                                 fbe_u32_t io_size_blocks)
{
    fbe_status_t status;
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             object_id, 
                                             class_id,
                                             FBE_PACKAGE_ID_SEP_0,
                                             rdgen_operation,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             max_passes,
                                             0, /* io count not used */
                                             0, /* time not used*/
                                             threads,
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             capacity, /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             1,    /* Min blocks */
                                             io_size_blocks  /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************
 * end lefty_test_setup_rdgen_test_context()
 ******************************************/
/******************************************************
 * lefty_wait_for_test_case_completion()
 ****************************************************************
 * @brief
 *  Run a wait loop which run untill one of the following condition
 *  is satisifed:
 *      1. wait loop has been run already for specified period by caller and/or
 *      2. there is no change in error statistics collected over next
 *         consecutive iterations (as specified by caller)
 *
 * @param target_wait_msec   - maximum duration to wait for
 * @param max_iteration      - maximum poll-iteration for which error stats can be constant.
 * @param actual_wait_msec_p - param to return period for which wait loop was run
 *
 * @return fbe_status_t
 *
 * @author
 *  07/10/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t lefty_wait_for_test_case_completion(fbe_u32_t target_wait_msec,
                                                 fbe_u32_t max_iteration,
                                                 fbe_u32_t * actual_wait_msec_p)
{
    fbe_u32_t elapsed_msec = 0;
    fbe_api_raid_library_error_testing_stats_t old_error_stats;
    fbe_api_raid_library_error_testing_stats_t curr_error_stats;
    fbe_u32_t unchanged_error_stats_iteration_count;


    /* Get the error-testing stats before waiting for a specified duration.
     */
    fbe_api_raid_group_get_raid_library_error_testing_stats(&old_error_stats);
    unchanged_error_stats_iteration_count = 0;

    /* We will timeout if one of the following occurs:
     *      1. loop has been run for a period equal to or more than specified 
     *         period by caller
     *      2. number of unexpected error-paths is no longer increasing for 
     *         a number (as per max_iteration) of consecutive polls 
     */
    while ((elapsed_msec < target_wait_msec) && (unchanged_error_stats_iteration_count < max_iteration))
    {
        fbe_api_sleep(LEFTY_TEST_RAID_LIB_ERROR_TESTING_POLLING_INTERVAL);
        elapsed_msec += LEFTY_TEST_RAID_LIB_ERROR_TESTING_POLLING_INTERVAL;

        fbe_api_raid_group_get_raid_library_error_testing_stats(&curr_error_stats);

        if (old_error_stats.error_count == curr_error_stats.error_count)
        {
            unchanged_error_stats_iteration_count += 1;
        }
        else
        {
            old_error_stats = curr_error_stats;
            unchanged_error_stats_iteration_count = 0;
        }
    }

    /* Return the period for which we ran tests as we may timeout prematurely if 
     * no new error paths are being detected anymore.
     */
    *actual_wait_msec_p = elapsed_msec;
    return FBE_STATUS_OK;
}
/*********************************************
 * end lefty_wait_for_test_case_completion()
 ********************************************/


 
/******************************************************
 * lefty_validate_rdgen_io_statistics()
 ****************************************************************
 * @brief
 *   validates and traces results of rdgen I/O statistics 
 *
 * @param rdgen_statistics_p  - pointer to rdgen io statistics
 *
 * @return None
 *
 * @author
 *  09/02/2010 - Created. Jyoti Ranjan
 *
 * @note
 *  This function asserts if it gets rdgen status as well as 
 *  statistics which is unexpected.
 ****************************************************************/
void lefty_validate_rdgen_io_statistics(fbe_rdgen_statistics_t * rdgen_statistics_p)
{
    /* We neither injected uncorrectable errors nor aborted I/O. So, we do expect
     * the following:
     *      1. none of I/O should have been aborted
     *      2. none of the blocks should have detected CRC while performing read 
     */
    MUT_ASSERT_INT_EQUAL(rdgen_statistics_p->aborted_error_count, 0);
    MUT_ASSERT_INT_EQUAL(rdgen_statistics_p->bad_crc_blocks_count, 0);
}
/*******************************************
 * end lefty_validate_rdgen_io_statistics()
 ******************************************/




/******************************************************
 * lefty_run_single_test_case()
 ****************************************************************
 * @brief
 *   runs a single test case
 *
 * @param test_case_p     - pointer to test case to be executed
 * @param traced_paths_p  - pointer to return number of new unexpected paths traced
 * @param test_duration_p - pointer to return druation (in msec) for which test was run
 *
 * @return number of traced unexptected paths while executing the given test case.
 *
 * @author
 *  07/10/2010 - Created. Jyoti Ranjan
 *
 * @Note
 *  The function assumes that caller has already enabled error testing path 
 *  prior to calling it. Also, caller has setup the correct configuration
 *  for the given test case.
 ****************************************************************/
fbe_status_t lefty_run_single_test_case(fbe_lefty_io_test_case_t * test_case_p,
                                        fbe_u32_t * traced_paths_p,
                                        fbe_u32_t * test_duration_p)
{
    fbe_status_t status;
    fbe_u32_t test_duration_msec;
    fbe_api_raid_library_error_testing_stats_t pre_test_stats;
    fbe_api_raid_library_error_testing_stats_t post_test_stats;
    fbe_u32_t wait_seconds;

    fbe_api_rdgen_context_t * context_p = &lefty_rdgen_contexts[0];

    /* Get the error-testing statistics before starting test
     */
    fbe_api_raid_group_get_raid_library_error_testing_stats(&pre_test_stats);

    /* Setup the rdgen context
     */
    status = lefty_test_setup_rdgen_test_context(context_p,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_CLASS_ID_LUN,
                                                 test_case_p->rdgen_opcode,
                                                 FBE_LBA_INVALID, /* use capacity */
                                                 0, /* run forever */
                                                 8, /* threads */
                                                 test_case_p->max_blocks);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (test_case_p->b_non_cached)
    {
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, 
                                                            FBE_RDGEN_OPTIONS_NON_CACHED | FBE_RDGEN_OPTIONS_CONTINUE_ON_ALL_ERRORS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, 
                                                            FBE_RDGEN_OPTIONS_CONTINUE_ON_ALL_ERRORS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Start rdgen operation(s)
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s rdgen operation  started : opcode = 0x%x, max blocks = 0x%x ==\n",
               __FUNCTION__,
               test_case_p->rdgen_opcode,
               test_case_p->max_blocks);
    /* We will use the longer of either the user specified for the test case specified
     * test time. 
     * Also if user specified is 0, we will use the default test case wait time. 
     */
    wait_seconds = fbe_test_sep_io_get_io_seconds();
    if (wait_seconds > test_case_p->duration_secs)
    {
        wait_seconds = test_case_p->duration_secs;
    }
    /* We let rdgen run until we either hit max time, or until we see there are no more new errors seen for some number
     * of polls. 
     */
    lefty_wait_for_test_case_completion(wait_seconds * FBE_TIME_MILLISECONDS_PER_SECOND,
                                        LEFTY_TEST_RAID_LIB_ERROR_TESTING_ERROR_STATS_SATURATION_COUNT,
                                        &test_duration_msec);

    
    /* Stop rdgen operation(s). Stopping rdgen destroys the context 
     * structure and clean-up all resources. Also
     */
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Validate rdgen result.
     */
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.rdgen_status, FBE_RDGEN_OPERATION_STATUS_MISCOMPARE);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.rdgen_status, FBE_RDGEN_OPERATION_STATUS_BAD_CRC_MISMATCH);
    lefty_validate_rdgen_io_statistics(&context_p->start_io.statistics);


    /* Get the error-testing after completion of I/O
     */
    fbe_api_raid_group_get_raid_library_error_testing_stats(&post_test_stats);

    /* Return the count of new error-paths discovered as well as period
     * for which we did ran test actually.
     */
    *traced_paths_p = post_test_stats.error_count - pre_test_stats.error_count;
    *test_duration_p = test_duration_msec;


    return FBE_STATUS_OK;
}
/***********************************
 * end lefty_run_single_test_case()
 **********************************/
void lefty_disable_lun_unexpected_error_handling(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_object_id_t lun_object_id;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            break;
        }
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        for ( lun_index = 0; lun_index < rg_config_p->number_of_luns_per_rg; lun_index++)
        {
            fbe_test_sep_util_wait_for_initial_verify(current_rg_config_p);
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_lun_disable_unexpected_errors(lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
}
/*!**************************************************************
 * lefty_test_rg_config()
 ****************************************************************
 * @brief
 * 
 *
 * @param   rg_config_p
 * @param   context_p points to config on which test is currently running               
 *
 * @return None.   
 *
 ****************************************************************/
void lefty_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t status;
	fbe_u32_t raid_group_count;
    fbe_u32_t config_duration_msec = 0;  /* tracks how long we did run tests for the given configuration  */
	fbe_api_raid_library_error_testing_stats_t pre_config_stats;
    fbe_api_raid_library_error_testing_stats_t post_config_stats;
	fbe_u32_t test_duration_msec;
    fbe_u32_t traced_paths;
	fbe_test_logical_error_table_test_configuration_t * lefty_config_p = NULL;
	fbe_u32_t test_case_index;
	fbe_u32_t test_case_count = 0;
	fbe_u32_t test_level;
    fbe_u32_t duration_sec;
    fbe_lefty_io_test_case_t * test_array_p = NULL;
    fbe_test_rg_configuration_t * current_rg_config_p;
	fbe_u32_t raid_group_index;
  
    test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        duration_sec = lefty_raid_lib_error_testing_period[test_level];
        test_array_p = &lefty_rdgen_io_test_case_extended[0];
    }
    else
    {
        duration_sec = lefty_raid_lib_error_testing_period[LEFTY_TEST_MAX_RAID_LIB_ERROR_TESTING_LEVEL - 1];
        test_array_p = &lefty_rdgen_io_test_case_qual[0];
    }

	/* Context has pointer to config on which test is currently running
	 */
	lefty_config_p = (fbe_test_logical_error_table_test_configuration_t *) context_p;
    lefty_disable_lun_unexpected_error_handling(lefty_config_p->raid_group_config);
	elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
	fbe_test_sep_util_disable_trace_limits();

    fbe_api_raid_group_get_raid_library_error_testing_stats(&pre_config_stats);
    MUT_ASSERT_INT_EQUAL(pre_config_stats.error_count, 0);
	
    config_duration_msec = 0;
    raid_group_count = lefty_get_raid_group_count(lefty_config_p);

    /* Power down drives if we have to.
     */
    if (lefty_config_p->num_drives_to_power_down[0] != 0)
    {
        big_bird_remove_all_drives(lefty_config_p->raid_group_config,
                                   raid_group_count,
                                   lefty_config_p->num_drives_to_power_down[0], 
                                   FBE_TRUE, /* We do not plan on re-inserting this drive later. */ 
                                   3000, /* msecs between removals */
                                   FBE_DRIVE_REMOVAL_MODE_RANDOM);
        fbe_api_sleep(3000);
        config_duration_msec += 3000;
    }

    /* Run I/O to cover the code path of raid library. We will iterate over
     * staticaly defined test cases to issue variety of I/O.
     */
    test_case_count = lefty_get_test_case_count(test_array_p);
    for(test_case_index = 0; test_case_index < test_case_count; test_case_index++)
    {
        /* Enable error-testing path and get error-testing stats prior to
         * running test cases against a new configuration. 
         */
        status = fbe_api_raid_group_set_raid_lib_error_testing();
        //status = fbe_api_raid_group_enable_raid_lib_random_errors(100);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        lefty_run_single_test_case(&test_array_p[test_case_index], &traced_paths, &test_duration_msec);
        config_duration_msec += test_duration_msec;

        mut_printf(MUT_LOG_TEST_STATUS, 
                   "== %s Traced paths=%03d for TEST ID=%d, DRIVES POWERED OFF=%02d : "
                   "allocated time=%d secs, actual test duration=%d secs\n",
                   __FUNCTION__, traced_paths,
                   test_case_index, 
                   lefty_config_p->num_drives_to_power_down[0],
                   test_array_p[test_case_index].duration_secs,
                   test_duration_msec/FBE_TIME_MILLISECONDS_PER_SECOND);
        /* Reset error-testing stats which will clean all logs as well as 
         * disable error-testing.
         */
        status = fbe_api_raid_group_reset_raid_lib_error_testing_stats();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    } /* end for(test_case_index = 0; test_case_index < test_case_count; test_case_index++) */
    

    /* Power up drives if we had powerd off. It makes us discover new set of 
     * unexpected error paths during rebuild operation.
     */
    if (lefty_config_p->num_drives_to_power_down[0] != 0)
    {
        big_bird_insert_all_drives(lefty_config_p->raid_group_config, raid_group_count, lefty_config_p->num_drives_to_power_down[0],
                                   FBE_TRUE    /* We are pulling and reinserting same drive */);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting drives successful. ==", __FUNCTION__);

        fbe_api_sleep(5000);
        config_duration_msec += 5000;

        /* Wait for all objects to become ready including raid groups.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);

        current_rg_config_p = lefty_config_p->raid_group_config;
        for ( raid_group_index = 0; raid_group_index < raid_group_count; raid_group_index++)
        {
			if (fbe_test_rg_config_is_enabled(current_rg_config_p))
			{
				fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
			}
            current_rg_config_p++;
        }
    
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
        big_bird_wait_for_rebuilds(lefty_config_p->raid_group_config, raid_group_count, lefty_config_p->num_drives_to_power_down[0]);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);
    } /* end if (lefty_config_p->num_drives_to_power_down[0] != 0) */

    /* Log the error statistics collected over a period of time against the 
     * given configuration.
     */
    fbe_api_raid_group_get_raid_library_error_testing_stats(&post_config_stats);
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Traced paths=%03d in duration=%d secs\n",
               __FUNCTION__,
               post_config_stats.error_count - pre_config_stats.error_count,
               config_duration_msec/FBE_TIME_MILLISECONDS_PER_SECOND);

    /* Reset error-testing stats which will clean all logs as well as 
     * disable error-testing.
     */
    fbe_api_raid_group_reset_raid_lib_error_testing_stats();

    fbe_api_sleep(10000);
}


/******************************************************
 * lefty_run_tests()
 ****************************************************************
 * @brief
 *   runs various test cases against various lun configuraion
 *
 * @param config_array_p - pointer to two dimensional array containing raid group config
 *
 * @return fbe_status_t 
 *
 * @author
 *  07/10/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t lefty_run_tests(fbe_test_logical_error_table_test_configuration_t * config_array_p,
                             fbe_raid_group_type_t raid_type)
{
    fbe_u32_t config_count;
    fbe_u32_t config_index;
    fbe_test_logical_error_table_test_configuration_t * lefty_config_p = NULL;
	fbe_u32_t raid_group_count;
    fbe_bool_t b_tested = FBE_FALSE;
    fbe_status_t status;

    /* Get total number of configuration to test for. And loop over all 
     * configurations and run tests against it
     */
    config_count = lefty_get_config_count(config_array_p);
    for(config_index = 0; config_index < config_count; config_index++)
    {
        lefty_config_p = &config_array_p[config_index];
		raid_group_count = lefty_get_raid_group_count(lefty_config_p);

        if (lefty_config_p->raid_group_config[0].raid_type != raid_type)
        {
            continue;
        }
        b_tested = FBE_TRUE;

        /* Create the setup for a given configuration. Setup operation involves the 
         * following activities:
         *      - Load the physical package and create the physical configuration. 
         *      - Load the logical packages
         *      - Fill up the disk and LU information to raid group configuration.
         *      - Kick off the creation of all the raid group with Logical unit configuration.
         */


        if (config_index + 1 >= config_count) {
            fbe_test_run_test_on_rg_config(&lefty_config_p->raid_group_config[0], lefty_config_p, 
                                           lefty_test_rg_config,
                                           LEFTY_TEST_LUNS_PER_RAID_GROUP,
                                           LEFTY_CHUNKS_PER_LUN); 
        }
        else {
            fbe_test_run_test_on_rg_config_with_time_save(&lefty_config_p->raid_group_config[0], lefty_config_p, 
                                           lefty_test_rg_config,
                                           LEFTY_TEST_LUNS_PER_RAID_GROUP,
                                           LEFTY_CHUNKS_PER_LUN,
                                           FBE_FALSE); 
        }

    } /*  for(config_index = 0; config_index < config_count; config_index++) */
    MUT_ASSERT_INT_EQUAL(b_tested, FBE_TRUE);

    /* we want to make sure system raid group /luns recovered from injected errors so that we 
     * won't the destroy job.
     */
    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE,
                                              FBE_LIFECYCLE_STATE_READY, 30000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
/***************************
 * end lefty_run_tests()
 **************************/

/*!**************************************************************
 * lefty_test()
 ****************************************************************
 * @brief
 *  Run tests to validate the unexpected error path
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  07/10/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
void lefty_test(fbe_raid_group_type_t raid_type)
{
    fbe_test_logical_error_table_test_configuration_t * config_array_p = NULL; 
    fbe_u32_t test_level;
    

    test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        config_array_p = &lefty_test_config_for_extended_mode[0];
    }
    else
    {
        config_array_p = &lefty_test_config_for_qual_mode[0];
    }

    /* Run the test.
     */
    lefty_run_tests(config_array_p, raid_type);

    return;
}
/******************************************
 * end lefty_test()
 ******************************************/
void lefty_raid0_test(void)
{
    lefty_test(FBE_RAID_GROUP_TYPE_RAID0);
}
void lefty_raid1_test(void)
{
    lefty_test(FBE_RAID_GROUP_TYPE_RAID1);
}
void lefty_raid10_test(void)
{
    lefty_test(FBE_RAID_GROUP_TYPE_RAID10);
}
void lefty_raid5_test(void)
{
    lefty_test(FBE_RAID_GROUP_TYPE_RAID5);
}
void lefty_raid3_test(void)
{
    lefty_test(FBE_RAID_GROUP_TYPE_RAID3);
}
void lefty_raid6_test(void)
{
    lefty_test(FBE_RAID_GROUP_TYPE_RAID6);
}
/*!**************************************************************
 * lefty_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the lefty test.
 *
 * @param  None.
 *
 * @return None.
 *
 * @author
 *  07/10/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
void lefty_cleanup(void)
{
    fbe_status_t status;
    status = fbe_api_trace_enable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_enable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Restore the sector trace level to the default.
     */
    fbe_test_sep_util_restore_sector_trace_level();

    /* Ensure we always unload the packages if we get some error and need to cleanup.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_api_raid_group_reset_raid_lib_error_testing_stats();
        fbe_sep_util_destroy_neit_sep_phy_expect_errs();
    }
    return;
}
/******************************************
 * end lefty_cleanup()
 ******************************************/

/*!**************************************************************
 * lefty_setup()
 ****************************************************************
 * @brief
 *  Setup for a lefty test
 *
 * @param  None.
 *
 * @return None.
 *
 * @author
 *  07/10/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
void lefty_setup(void)
{
    fbe_status_t status;
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            config_p = &lefty_test_config_for_extended_mode[0];
        }
        else
        {
            config_p = &lefty_test_config_for_qual_mode[0];
        }
        abby_cadabby_simulation_setup(config_p, 
                                      FBE_FALSE  /*! @todo Add support to use random drive block size */);
        abby_cadabby_load_physical_config(config_p);
        elmo_load_sep_and_neit();
    }
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* This test will pull a drive out and insert a new drive
     * We configure this drive as a spare and we want this drive to swap in immediately. 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    fbe_test_common_util_test_setup_init();
     return;
}
/******************************************
 * end lefty_setup()
 ******************************************/


/*************************
 * end file lefty_test.c
 *************************/


