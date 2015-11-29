/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file mumford_the_magician_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test cases to validate event log messages reported 
 *  on detection of correctable errors.
 *
 * @version
 *   8/21/2010:  Created - Pradnya Patankar
 *   2/10/2011:  Rewrote - Jyoti Ranjan
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
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_raid_error.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe_test_common_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
 
/*************************
 *   TEST DESCRIPTION
 *************************/

char * mumford_the_magician_test_short_desc = "Validate event log messages reported on detection of correctable errors.";
char * mumford_the_magician_test_long_desc = "\n\
The Mumford the Magician scenario injects errors on a given raid type and issues specific rdgen I/O.\n"
"Also, it validates event log messages reported by the RAID library on detection of error. \n"
"Examples of injected test cases are time stamp error, write stamp error, shed stamp error,\n"
"single bit checksum error, multibit checksum error, combination of errors, etc. \n"
"\n\
STEP 1: Configure all raid groups and LUNs.\n\
        - Tests cover 520 block sizes.\n\
        - Tests cover r10, r1, r5, r3, r6 raid types.\n\
        - All raid groups have 1 LUN each.\n\
STEP 2: Write a background pattern to all LUNs for first test in a series of tests for a give RG type.\n\
STEP 3: Inject predefined error using logical error injection service.\n\
STEP 4: Issue rdgen I/O.\n\
STEP 5: Validate rdgen I/O statistics as per predefined values.\n\
STEP 6: Validate the expected event log message correctness against predefined expected parameters.\n\
STEP 7: Repeat steps 3-6 for each test in series.\n\
STEP 8: Repeat steps 2-7 for each RG type.\n\
STEP 9: Destroy raid groups and LUNs.\n\
\n\
Outstanding Issues:\n\
The following tests have been commented out for R6 testing:\n\
			- Write-stamp error in -many- blocks at -row parity- drive on Write/Zero\n\
			- Write-stamp error in -many- blocks at -digonal parity- drive on Write/Zero\n\
			- Write-stamp error in -many- blocks at -row and digonal parity- drives on Write/Zero\n\
			- Write-stamp error in -many- blocks at -row parity and data- drives on Write/Zero\n\
			- Write-stamp error in -many- blocks at -row and digonal parity- drives on Write/Zero\n\
\n\
Description last updated: 10/13/2011.\n";


/************************
 * LITERAL DECLARARION
 ***********************/

/*!*******************************************************************
 * @def MUMFORD_THE_MAGICIAN_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define MUMFORD_THE_MAGICIAN_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def MUMFORD_THE_MAGICIAN_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define MUMFORD_THE_MAGICIAN_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def MUMFORD_THE_MAGICIAN_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with.
 *
 *********************************************************************/
#define MUMFORD_THE_MAGICIAN_TEST_RAID_GROUP_COUNT 15


/*!*******************************************************************
 * @def MUMFORD_THE_MAGICIAN_TEST_VERIFY_WAIT_SECONDS
 *********************************************************************
 * @brief Number of seconds we should wait for the verify to finish.
 *
 *********************************************************************/
#define MUMFORD_THE_MAGICIAN_TEST_VERIFY_WAIT_SECONDS 120


/*!*******************************************************************
 * @def MUMFORD_THE_MAGICIAN_WAIT_PERIOD_FOR_REMOVAL
 *********************************************************************
 * @brief Maximum time-period (in msec) to wait for lifecycle  
 *        state to reflect removal of drive.
 *
 *********************************************************************/
#define MUMFORD_THE_MAGICIAN_WAIT_PERIOD_FOR_REMOVAL 50000



/*!*******************************************************************
 * @var mumford_the_magician_validate_rdgen_result_fn
 *********************************************************************
 * @brief The function is used to validate rdgen related parameters
 *        after issuing a single I/O request
 *
 *********************************************************************/
fbe_test_event_log_test_validate_rdgen_result_t mumford_the_magician_validate_rdgen_result_fn_p = NULL;


/***********************
 * FORWARD DECLARARION 
 ***********************/
fbe_status_t mumford_the_magician_validate_rdgen_result(fbe_status_t rdgen_request_status,
                                                        fbe_rdgen_operation_status_t rdgen_op_status,
                                                        fbe_rdgen_statistics_t *rdgen_stats_p);

/***********************
 * GLOBAL VARIABLES
 ***********************/
static fbe_event_log_get_event_log_info_t * events_array = NULL;

/*!*******************************************************************
 * @var mumford_the_magician_basic_crc_test_for_raid_5_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against correctable CRC errors for 
 *        non-degraded RAID 3 and RAID 5
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_basic_crc_test_for_raid_5_non_degraded = 
{
    "Correctable CRC test cases for non-degraded RAID 3 and RAID 5 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC @ data pos, block(s) = 0x1, width = 0x5
         */
        {   
            "CRC error in -single- block at data drive on Write/Zero/Read",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 
                            { 0x10, 0x1, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 }
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x4, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                            (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                            0x1, /* expected lba */
                            0x1  /* expected blocks */
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

           
        /* [T001] Brief: CRC @ data pos, block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC error in -multiple- blocks at data drive on Write/Zero/Read",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x19,  0x7F },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x19,  0x7F },                
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x19,  0x7F },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x10, 0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x4, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                            (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                            0x19, /* expected lba */
                            0x17  /* expected blocks */
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] Brief: CRC @ parity pos, block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC error in -single- block at parity drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x7F },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x7F },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x7, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x0, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                            (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                            0x7, /* expected lba */
                            0x1 /* expected blocks */
                        }
                    },  /* expected result for above injected error */ 
                },
            },
        },


        /* [T003] Brief: CRC @ parity pos, block(s) = -MANY-, width = 0x5
         */
        {  
            "CRC error in -multiple- blocks at parity drive on Write/Zero",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x27,  0x142 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x27,  0x142 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x21, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x0, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                            (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                            0x21, /* expected lba */
                            0x10 /* expected blocks */
                        }
                    },  /* expected result for above injected error */ 
                },
            },
        },

        {
            "Terminator",
            FBE_U32_MAX, /* Terminator */
        }
    }
};



/*!*******************************************************************
 * @var mumford_the_magician_crc_retry_test_for_raid_5_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against CRC errors which were 
 *        reported as a result of retried CRC/stamp errors.
 *
 *********************************************************************/

/*!@todo Following two test cases for R3 and R5 split out because there is change in the direct IO path 
  *   where R3 type is not supported if the block size is equal or less than 0x80 but R5 does. 
  *   For CRC error cases, we have to increase "err_limit" +1 for the IO which took the direct IO path. We have test 
  *   parameters common for R3 and R5 so until we do not have direct IO support for R3 for small size, R3 and R5 non  
  *   degraded cases will execute separately. Once the pending work done, we need to combine the following both 
  *   test tables.
  * 1. mumford_the_magician_crc_retry_test_for_raid_5_non_degraded
  * 2. mumford_the_magician_crc_retry_test_for_raid_3_non_degraded
  */

static fbe_test_event_log_test_group_t mumford_the_magician_crc_retry_test_for_raid_5_non_degraded = 
{
    "Retry-correctable test cases for non-degraded RAID 5 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC @ data pos, block(s) = -MANY-, width = 0x5
         */
        {   
            "Retriable CRC error at data drive on Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x3 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x4, /* expected fru position */ 
                            SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                            (VR_CRC_RETRY), /* expected error info */
                            0x0, /* expected lba */
                            0x80  /* expected blocks */
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },

        /* [T001] 
         * Brief: CRC @ data pos, block(s) = -MANY-, width = 0x5
         * Note: In this test case we do inject both retriable as well normal CRC 
         *       at same disk. Current test case validates that we do not see 
         *       retriable CRC. Ideally, it is limitation of raid library not being
         *       able to report different portion of same drive differently,
         *       if some part of drive(s) are infected with CRC errors which vanishes
         *       on retry and some do not.
         */
        {   
            "Retriable as well as non-retriable CRC error at same data drive on Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x2 },
                            { 0x10, 0x21, 0xF, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x10 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x4, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                            (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                            0x21, /* expected lba */
                            0xF  /* expected blocks */
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },

  
        /* [T002] 
         * Brief: Non retriable CRC error as well as retriable CRC error at two different drive(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "Non retriable CRC error as well as retriable CRC error at two different data drive(s) on Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x7F },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x10, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x0 },
                            { 0x08, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x1 },
                            { 0x01, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x1 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CRC_RETRY), /* expected error info */
                                0x1, /* expected lba */
                                0x7F  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CRC_RETRY), /* expected error info */
                                0x1, /* expected lba */
                                0x7F  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
            },
        },

        {
            "Terminator",
            FBE_U32_MAX, /* Terminator */
        }
    }
};

static fbe_test_event_log_test_group_t mumford_the_magician_crc_retry_test_for_raid_3_non_degraded = 
{
    "Retry-correctable test cases for non-degraded RAID 3 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC @ data pos, block(s) = -MANY-, width = 0x5
         */


        {   
            "Retriable CRC error at data drive on Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x3 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x4, /* expected fru position */ 
                            SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                            (VR_CRC_RETRY), /* expected error info */
                            0x0, /* expected lba */
                            0x80  /* expected blocks */
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },

        /* [T001] 
         * Brief: CRC @ data pos, block(s) = -MANY-, width = 0x5
         * Note: In this test case we do inject both retriable as well normal CRC 
         *       at same disk. Current test case validates that we do not see 
         *       retriable CRC. Ideally, it is limitation of raid library not being
         *       able to report different portion of same drive differently,
         *       if some part of drive(s) are infected with CRC errors which vanishes
         *       on retry and some do not.
         */
        {   
            "Retriable as well as non-retriable CRC error at same data drive on Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x2 },
                            { 0x10, 0x20, 0x11, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x10 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x4, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                            (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                            0x20, /* expected lba */
                            0x11  /* expected blocks */
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },

  
        /* [T002] 
         * Brief: Non retriable CRC error as well as retriable CRC error at two different drive(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "Non retriable CRC error as well as retriable CRC error at two different data drive(s) on Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x71 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x10, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x0 },
                            { 0x08, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x1 },
                            { 0x01, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x1 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CRC_RETRY), /* expected error info */
                                0x0, /* expected lba */
                                0x71  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CRC_RETRY), /* expected error info */
                                0x0, /* expected lba */
                                0x71  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
            },
        },


        {
            "Terminator",
            FBE_U32_MAX, /* Terminator */
        }
    }
};


/*!*******************************************************************
 * @var mumford_the_magician_basic_crc_test_for_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against correctable CRC errors for 
 *        non-degraded RAID 6
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_basic_crc_test_for_raid_6_non_degraded = 
{
    "Correctable CRC test cases for non-degraded RAID 6 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC @ data pos, block(s) = 0x1, width = 0x8
         */
        {   
            "CRC error in -single- block at -data- drive on Write/Zero/Read",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },                
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x80, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0, /* expected lba */
                                0x1 /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },

       
        /* [T001] Brief: CRC @ data pos, block(s) = -MANY-, width = 0x6
         */
        {   
            "CRC error in -multiple- blocks at -data- drive on Write/Zero/Read",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },                
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x80, 0x21, 0x17, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x21, /* expected lba */
                                0x17 /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },

        /* [T002] Brief: CRC @ -row- parity pos, block(s) = 0x1, width = 0x8
         */
        {   
            "CRC error in -single- block at -row parity- drive on Write/Zero",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x281 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x281 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1 /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },

        /* [T003] Brief: CRC @ -row- parity pos, block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC error in -many- blocks at -row parity- drive on Write/Zero",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x1,  0x79 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x21, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x21, /* expected lba */
                                0x10 /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
            },
        },
    

        /* [T004] Brief: CRC @ -diagonal- parity pos, block(s) = 0x1, width = 0x8
         */
        {   
            "CRC error in -single- block at -diagonal parity- drive on Write/Zero",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x7F },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x02, 0x7, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            {
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x1 /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                },
            }
        },

        /* [T005] Brief: CRC @ -diagonal- parity pos, block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC error in -many- blocks at -diagonal parity- drive on Write/Zero",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x23,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x23,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x02, 0x20, 0x17, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */ 
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20, /* expected lba */
                                0x17 /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },

        /* [T006] Brief: CRC @ two data position (of same strip), block(s) = -many-, width = 0x8
         */
        {   
            "CRC error in -multiple- block at -two data- drives on Write/Zero/Read",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x4,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x4,  0x100 },                
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x4,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10, /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10 /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },

        /* [T007] Brief: CRC @ -row- parity position and one data position (of same strip),
         *        block(s) = 0x1, width = 0x8
         */
        {   
            "CRC error in -single- block at -row parity- and -data- drives on Write/Zero/Read",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },                
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x80, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1, /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1 /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },

        /* [T008] Brief: CRC @ -diagonal- parity position and one data position (of same strip), 
         *        block(s) = 0x1, width = 0x8
         */
        {   
            "CRC error in -single- block at -diagonal parity- and -data- drives on Write/Zero/Read",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x2,  0x79 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x2,  0x78 },                
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x2,  0x79 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x02, 0x2, 0x01, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x80, 0x2, 0x01, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x2, /* expected lba */
                                0x1, /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                2, /* expected lba */
                                0x1 /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },

        
        /* [T009] Brief: CRC @ -row- and -diagonal- parity position (of same strip), 
         *        block(s) = 0x10, width = 0x8
         */
        {   
            "CRC error in -many- blocks at -row and diagonal parity- drives on Write/Zero",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x20, 0x11, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x20, 0x11, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20, /* expected lba */
                                0x11, /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20, /* expected lba */
                                0x11 /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                },
            },
        },


        {
            "Terminator",
            FBE_U32_MAX, /* Terminator */
        }
    }
};



/*!*******************************************************************
 * @var mumford_the_magician_basic_test_for_mirror_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against correctable errors for 
 *        non-degraded RAID 1 and RAID 10
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_basic_test_for_mirror_non_degraded = 
{
    "Correctable test cases for RAID 1 (mirror) raid groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC @ primary-data pos, block(s) = 0x1, width = 0x2
         */
        {   
            "CRC error in -single- block at -primary- position on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0, 0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0, /* expected lba */
                                0x1 /* expected blocks */
                            }
                        },
                    },  /* expected result for above injected error */ 
                },
            }
        },

        /* [T001] Brief: CRC @ primary pos, block(s) = -MANY-, width = 0x2
         */
        {   
            "CRC error in -many- blocks at -primary- position on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x1,  0xFF },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x05, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                               0x0, /* expected fru position */ 
                               SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                               (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                               0x5, /* expected lba */
                               0x10 /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                 },
             },
        },

        
        /* [T002] Brief: LBA-stamp @ primary pos, block(s) = 0x1, width = 0x2
         */
        {   
            "LBA-stamp error in -single- block at -primary- position on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x17F },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x1, 0x01, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            {
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, 
                                0x1,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T003] Brief: LBA stamp @ primary pos , block(s) = -MANY-, width = 0x2
         */
        {   
            "LBA-stamp error in -many- blocks at -primary- position on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x1,  0x79 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x05, 0x13, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x0, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                            (VR_LBA_STAMP), /* expected error info */
                            0x5, 
                            0x13,
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: CRC @ primary pos but strip not having more than one error, block(s) = 0x1, width = 0x2
         * Note : As errors have injected at two non-overlapping set of pba. So reported
         *        messages will be many.
         */
        {   
            "Multiple CRC error but no strip having more than one error at -primary- position on Read",
            0x2, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x00, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x20, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, 
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20,
                                0x5,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] 
         * Brief: lba stamp @ primary pos but no two errors having more than one error, block(s) = 0x1, width = 0x2
         * Note : As errors have injected at two non-overlapping set of pba. So reported
         *        messages will be many.
         */
        {   
            "Multiple LBA-stamp error but no strip having more than one error at -primary- position on Read",
            0x2, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x1,  0x70 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x00, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x01, 0x20, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, 
                                0xF
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x20, 
                                0x5,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        {
            "Terminator",
            FBE_U32_MAX, /* Terminator */
        }
    }
};



/*!*******************************************************************
 * @var mumford_the_magician_crc_retry_test_for_mirror_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against correctable errors for 
 *        non-degraded RAID 1 and RAID 10
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_crc_retry_test_for_mirror_non_degraded = 
{
    "Retriable error test cases for RAID 1 (mirror) raid groups only",
    mumford_the_magician_run_single_test_case,
    {
#if 0    
        /* [T000] Brief: Retriable CRC error at -primary- position, block(s) = -MANY-, width = 0x2
         */
        {   
            "Retriable CRC error at -primary- position on Read",
            0x2, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0, 0x20 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    /*! @note Must inject (3) times to include direct I/O. */
                    {
                        0x1,
                        {
                            { 0x01, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x3 },
                        },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CRC_RETRY), /* expected error info */
                                0, /* expected lba */
                                0x20 /* expected blocks */
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },
     

         /* [T001] 
          * Brief: Retriable CRC error at -secondary- and non-retriable CRC error at -primary- position,
          *        block(s) = -MANY-, width = 0x2
          */
        {   
            "Retriable CRC error at -secondary-  and non-retriable CRC at primary position on Read",
            0x2, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0, 0x20 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x0 },
                            { 0x02, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0x2 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0, /* expected lba */
                                0x10 /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CRC_RETRY), /* expected error info */
                                0, /* expected lba */
                                0x20 /* expected blocks */
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },
#endif   
        {
            "Terminator",
            FBE_U32_MAX, /* Terminator */
        }
    }
};

/*!*******************************************************************
 * @var mumford_the_magician_basic_test_for_r10_only_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against correctable errors for 
 *        non-degraded RAID 10 only
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_basic_test_for_r10_only_non_degraded = 
{
    "Correctable test cases for RAID 10 (stripped-mirror) raid groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC @ primary-data pos, block(s) = 0x1, width = 0x2
         */
        {   
            "CRC error in -single- block at -only one primary- position on Read",
            0x6, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0, 0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0, /* expected lba */
                                0x1 /* expected blocks */
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: CRC @ primary pos, block(s) = -MANY-, width = 0x2
         */
        {   
            "CRC error in -many- blocks at -multiple primary- position on Read",
            0x6, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x1,  0xFF },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x05, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x5, /* expected lba */
                                0x10 /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x5, /* expected lba */
                                0x10 /* expected blocks */
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T002] Brief: LBA-stamp @ primary pos, block(s) = 0x1, width = 0x2
         */
        {   
            "LBA-stamp error in -single- block at -multiple primary- position on Read",
            0x6, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x1,  0x1F9 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x1, 0x07, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, 
                                0x7,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, 
                                0x7,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, 
                                0x7,
                            }
                        }
                    },  /* expected result for above injected error */
                }
            }
        },


        /* [T003] Brief: LBA stamp @ primary pos , block(s) = -MANY-, width = 0x2
         */
        {   
            "LBA-stamp error in -many- blocks but at -only one primary- position on Read",
            0x6, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x05, 0x13, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x0, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                            (VR_LBA_STAMP), /* expected error info */
                            0x5, 
                            0x13,
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: CRC @ primary pos but no strip having more than one error, block(s) = 0x1, width = 0x2
         * Note : As errors have injected at two non-overlapping set of pba. So reported
         *        messages will be many.
         */
        {   
            "Multiple CRC errors at -only one primary- position but no strip having more than one error on Read",
            0x6, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x00, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x20, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, 
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20, 
                                0x5,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T005] 
         * Brief: lba stamp @ primary pos but no two errors having more than one error, block(s) = 0x1, width = 0x2
         * Note : As errors have injected at two non-overlapping set of pba. So reported
         *        messages will be many.
         */
        {   
            "Multiple LBA-stamp errors at -only one primary- position but no two strip having more than one error on Read",
            0x6, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x01, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x01, 0x20, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x01, 
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x20, 
                                0x5,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        {
            "Terminator",
            FBE_U32_MAX, /* Terminator */
        }
    }
};



/*!*******************************************************************
 * @var mumford_the_magician_basic_stamp_test_for_raid_5_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against correctable stamp errors for 
 *        non-degraded RAID 3 and RAID 5
 *
 * @ note:
 *   The following stamps are validated:
 *          1. time-stamp
 *          2. write-stamp
 *          3. shed-stamp
 *          4. lba-stamp
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_basic_stamp_test_for_raid_5_non_degraded = 
{
    "Correctable stamp error test cases for non-degraded RAID 3 and RAID 5 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: time-stamp error (at data pos), block(s) = 0x1, width = 0x5
         */
        {   
            "Time-stamp error in -single- block at -data- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x1,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x1,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x1,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x1,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: time-stamp error (at data pos), block(s) = 0x10, width = 0x5
         */
        {   
            "Time-stamp error in -many- blocks at -data- drive on Write/Zero",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {   
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x179 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x79 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x27, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x27,
                                0x10
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x27,
                                0x10
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x27,
                                0x10
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x27,
                                0x10
                            },
                        }
                    },  /* expected result for above injected error */
                }
            }
        },

                
        /* [T002] Brief: time-stamp error (at parity pos), block(s) = 0x1, width = 0x5
         */
        {   
            "Time-stamp error in -single- block at -parity- drive on Write/Zero",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x1,  0x7F },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x1,  0x7F },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x7, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x0, /* expected fru position */ 
                            SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                            (VR_TS), /* expected error info */
                            0x7,
                            0x1
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T003] Brief: time-stamp error (at parity pos), block(s) = 0x10, width = 0x5
         */
        {   
            "Time-stamp error in -many- blocks at -parity- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x180 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x21, 0x17, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x0, /* expected fru position */ 
                            SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                            (VR_TS), /* expected error info */
                            0x21,
                            0x17
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: write-stamp error (at data pos), block(s) = 0x1, width = 0x5
         */
        {   
            "Write-stamp error in -single- block at -data- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x180 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x5, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            /*! @note We now properly flag the position in error since parity is treated
                             *        just like any other data position.  In addition when a write stamp
                             *        error is injected we clear the `all valid' in the time stamp to
                             *        guarantee a write stamp error.
                             */
                            { 
                                0x4, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x5,
                                0x1
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] 
         * Brief: write-stamp error (at data pos), block(s) = 0x10, width = 0x5
         */
        {   
            "Write-stamp error in -many- blocks at -data- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x29,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x29,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x21, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            /*! @note We now properly flag the position in error since parity is treated
                             *        just like any other data position.  In addition when a write stamp
                             *        error is injected we clear the `all valid' in the time stamp to
                             *        guarantee a write stamp error.
                             */
                            { 
                                0x4, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x21,
                                0x10
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T006] 
         * Brief: write-stamp error (at parity pos), block(s) = 0x1, width = 0x5
         */
        {   
            "Write-stamp error in -single- block at -parity- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x79 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        1,
                        { 0x01, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x1
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x1
                            },
                        }            
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T007] 
         * Brief: write-stamp error (at parity pos), block(s) = 0x10, width = 0x5
         */
        {   
            "Write-stamp error in -many- blocks at -parity- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x1,  0x79 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x21, 0x17, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x21,
                                0x17
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x21,
                                0x17
                             }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T008] Brief: shed-stamp error (at data pos), block(s) = 0x1, width = 0x5
         */
        {   
            "Shed-stamp error in -single- block at -data- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SHED_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x4, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                            (VR_LBA_STAMP), /* expected error info */
                            0x0,
                            0x1
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T009] Brief: shed-stamp error (at data pos), block(s) = 0x10, width = 0x5
         */
        {   
            "Shed-stamp error in -many- blocks at -data- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x180 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x21, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SHED_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x4, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                            (VR_LBA_STAMP), /* expected error info */
                            0x21,
                            0x10
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T010] Brief: shed-stamp error (at parity pos), block(s) = 0x1, width = 0x5
         *        Please note that shed stamp error in case of non-degraded raid type 
         *        is reported as LBA stamp always.
         */
        {   
            "Shed-stamp error in -single- block at -parity- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x79 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x1, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SHED_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x0, /* expected fru position */ 
                            SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                            (VR_SS), /* expected error info */
                            0x1,
                            0x1
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T011] Brief: shed-stamp error (at parity pos), block(s) = 0x10, width = 0x5
         *        Please note that shed stamp error in case of non-degraded raid type
         *         is reported as LBA stamp always.
         */
        {  
            "Shed-stamp error in -many- blocks at -parity- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x1,  0x7F },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x21, 0x17, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SHED_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x0, /* expected fru position */ 
                            SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                            (VR_SS), /* expected error info */
                            0x21,
                            0x17
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T012] Brief: LBA-stamp error (at data pos), block(s) = 0x1, width = 0x5
         */
        {   
            "LBA-stamp error in -single- block at -data- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0},
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x4, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                            (VR_LBA_STAMP), /* expected error info */
                            0x0,
                            0x1
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T013] Brief: LBA-stamp error (at data pos), block(s) = -MANY-, width = 0x5
         */
        {   
            "LBA-stamp error in -many- blocks at -data- drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x180 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x21, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x4, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                            (VR_LBA_STAMP), /* expected error info */
                            0x21,
                            0x10,
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T013] Brief: LBA-stamp error (at data pos), block(s) = -MANY-, width = 0x5
         */
        {   
            "LBA-stamp error in -many- blocks at -data- drive on small Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x43,  0x7 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x43,  0x7 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x10, 0x41, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        { 
                            0x4, /* expected fru position */ 
                            SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                            (VR_LBA_STAMP), /* expected error info */
                            0x43,
                            0x7,
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        {
            "Terminator",
            FBE_U32_MAX /* Terminator */
        }
    }
};





/*!*******************************************************************
 * @var mumford_the_magician_misc_error_test_for_raid_5_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log 
 *        messages reported as a result of injected error of 
 *        -different error type-. For example: validation of event
 *        log messages if time stamp as well as CRC error are injected 
 *        at different lba.
 *        
 * Note:
 * Error combination will be choosen such that generated error provoked
 * by rdgen I/O are correctable in nature.
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_misc_error_test_for_raid_5_non_degraded = 
{
    "Correctable error test cases (injecting multiple errors) for non-degraded RAID 3 and RAID 5 only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: CRC and time-stamp error (at different data pos), block(s) = 0x1, width = 0x5
         * Note : Error have been injected in non-overlapping blocks to avoid generation of 
         *        uncorrectable errors.
         */
        {   
            "CRC and time-stamp error at non-overlapping strips but (but at different data drives) on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x04, 0x00, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x02, 0x20, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {

                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20,
                                0x05,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T001] 
         * Brief: multiple-CRC error at disjoint blocks (at data pos), width = 0x5
         * Note : Error have been injected to avoid uncorrectable errors.
         */
        {   
            "Multiple-CRC errors of -many- blocks at same -data- drive on Write/Zero",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x10, 0x01, 0x06, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x10, 0x10, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x10, 0x20, 0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x01,
                                0x06
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x10,
                                0x05
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20,
                                0x20
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T002] 
         * Brief: time-stamp error (at parity pos), block(s) = 0x10, width = 0x5
         * Note : Although we injected mutliple errors but CRC will be detected 
         *        first and we will report the event-log messages for CRC 
         *        error only.
         */
        {   
            "Multiple error (CRC, lba-stamp and time-stamp) in -same- block at -data- drive on Write/Zero",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x180 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x1
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: multiple errors (at parity pos), block(s) = 0x10, width = 0x5
         */
        {   
            "Multiple errors (time-stamp, lba-stamp and CRC) but no strip having more than one error at same drive on Write/Zero",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x10, 0x05, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x10, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x10, 0x40, 0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x05,
                                0x05
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x20,
                                0x10
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x40,
                                0x20
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x40,
                                0x20
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x40,
                                0x20
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x40,
                                0x20
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        {
            "Terminator",
            FBE_U32_MAX /* Terminator */
        }
    }
};



/*!*******************************************************************
 * @var mumford_the_magician_basic_ts_test_for_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against correctable time-stamp errors for 
 *        non-degraded RAID 6
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_basic_ts_test_for_raid_6_non_degraded = 
{
    "Correctable time stamp test cases for non-degraded RAID 6 only",
    mumford_the_magician_run_single_test_case,
    {    
        /* [T000] Brief: time-stamp error (at data pos), block(s) = 1, width = 0x8
         */
        {
            "Time-stamp error in -single- block at -data- drive on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x80, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x1,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: time-stamp error (at data pos), block(s) = -MANY-, width = 0x8
         */
        {
            "Time-stamp error in -many- blocks at -data- drive on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x80, 0x21, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x21,
                                0x10,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] Brief: time-stamp error (at -row parity- pos), block(s) = -MANY-, width = 0x8
         */
        {
            "Time-stamp error in -many- blocks at -row parity- drive on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x7F },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x21, 0x17, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x21,
                                0x17,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] Brief: time-stamp error (at -diagonal parity- pos), block(s) = -MANY-, width = 0x8
         */
        {
            "Time-stamp error in -many- blocks at -diagonal parity- drive on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x11,  0x70 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x01, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x20,
                                0x10,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T004] Brief: time-stamp error (at -two data- pos), block(s) = -MANY-, width = 0x8
         */
        {
            "Time-stamp error in -many- blocks at -two data- drives on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x80, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x40, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x20,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x20,
                                0x10,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T005] 
         * Brief: time-stamp error (at -data- pos and -row parity- pos), block(s) = -MANY-, width = 0x8 
         * Note : The current test case uses two different size of blocks to inject errors. As a result, 
         *        number of event log messages change.
         */
        {
            "Time-stamp error in -many- blocks at -data and row parity- drives on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x01, 0x0, 0x01, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x1,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x1,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x1,
                                0xf,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T006]
         * Brief: time-stamp error (at -data- pos and -diagonal parity- pos), block(s) = -MANY-, width = 0x8 
         * Note : The current test case uses two different size of blocks to inject errors. As a result, 
         *        number of event log messages change.
         */
        {
            "Time-stamp error in -many- blocks at -data and diagonal parity- drives on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x02, 0x0, 0x40, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x10,
                                0x30,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


                
        /* [T007]
         * Brief: time-stamp error (at -row- pos and -diagonal parity- pos), block(s) = -MANY-, width = 0x8 
         * Note : The current test case uses two different size of blocks to inject errors. As a result, 
         *        number of event log messages change.
         */
        {
            "Time-stamp error in -many- blocks at -row and diagonal parity- drives on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x02, 0x10, 0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x10,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x10,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x20,
                                0x50,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        {
            "Terminator",
            FBE_U32_MAX /* Terminator */
        }
    }
};




/*!*******************************************************************
 * @var mumford_the_magician_basic_ws_and_ls_test_for_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against correctable write and lba stamp 
 *        errors for non-degraded RAID 6
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_basic_ws_and_ls_test_for_raid_6_non_degraded = 
{
    "Correctable write and lba stamp test cases for non-degraded RAID 6 only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: lba-stamp error (at data pos), block(s) = 1, width = 0x8
         */
        {
            "LBA-stamp error in -single- block at -data- drive on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x80, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x1,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: lba-stamp error (at data pos), block(s) = -MANY-, width = 0x8
         */
        {
            "LBA-stamp error in -many- blocks at -data- drive on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x80,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        1,
                        { 0x40, 0x27, 0x11, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x27,
                                0x11,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] 
         * Brief: lba-stamp error (at -two data- pos), block(s) = -MANY-, width = 0x8
         * Note : The current test case uses two different size of blocks to inject errors. As
         *        a result, number of event log messages change.
         */
        {
            "LBA-stamp error in -many- blocks at -two data- drives on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x80, 0x0,  0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x40, 0x10, 0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x10,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x10,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x20,
                                0x50,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: write-stamp error (at -data- pos), block(s) = -MANY-, width = 0x8
         */
        {
            "Write-stamp error in -many- blocks at -data- drives on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 0x80, 0x1, 0x11, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x1,
                                0x11,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x1,
                                0x11,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x1,
                                0x11,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x1,
                                0x11,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T004]
         * Brief: write-stamp error (at -two data- pos), block(s) = -MANY-, width = 0x8
         */
        {
            "Write-stamp error in -many- blocks at -two data- drives on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x1,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x80, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                            { 0x40, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x20,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x20,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x20,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x20,
                                0x10,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

#if 0
        /* [T005] 
         * Brief: write-stamp error (at -row parity- pos), block(s) = -MANY-, width = 0x8 
         */
        {
            "Write-stamp error in -many- blocks at -row parity- drive on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 }
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x1,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x1,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T006] 
         * Brief: write-stamp error (at -diagonal parity- pos), block(s) = -MANY-, width = 0x8 
         */
        {
            "Write-stamp error in -many- blocks at -digonal parity- drive on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 }
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x10,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T007] 
         * Brief: write-stamp error (at -row and diagonal parity- pos), block(s) = -MANY-, width = 0x8 
         */
        {
            "Write-stamp error in -many- blocks at -row and digonal parity- drives on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 }
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x10,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T008] 
         * Brief: write-stamp error (at -row parity and data- pos), block(s) = -MANY-, width = 0x8 
         */
        {
            "Write-stamp error in -many- blocks at -row parity and data- drives on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 }
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x1,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T009] 
         * Brief: write-stamp error (at -row and diagonal parity- pos), block(s) = -MANY-, width = 0x8 
         */
        {
            "Write-stamp error in -many- blocks at -row and digonal parity- drives on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 }
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x10,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },
#endif

        {
            "Terminator",
            FBE_U32_MAX /* Terminator */
        }
    }
};



/*!*******************************************************************
 * @var mumford_the_magician_advanced_crc_test_for_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event 
 *        log messages reported against complex scenarios of 
 *        correctable CRC errors.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_advanced_crc_test_for_raid_6_non_degraded = 
{
    "Correctable error test cases (injecting multiple errors) for non-degraded RAID 6 only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: Multiple CRC errors error at data pos, block(s) = -many-, width = 0x8
         * Note: Injected multiple-error at only one data drives only. 
         */
        {
            "Multiple CRC errors but at same -data- drive on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x80, 0x10, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x80, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x1,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x10,
                                0x05,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20,
                                0x10,
                            }
                        }
                    },  /* expected result for above injected error */
                }
            }
        },

        /* [T001] 
         * Brief: Multiple CRC errors at more than one drive, block(s) = -MANY-, width = 0x8
         * Note: Injected two CRC errors of same strip. Also, injecting errors with different
         *       combination of drive (viz. row parity + data drive , two data drive and 
         *       row-parity and diagonal parity.
         */
        {
            "Multiple CRC errors in -many- blocks at various drives on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x80,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x6,
                        {
                            { 0x80, 0x00, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x00, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x20, 0x20, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x20, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0},
                            { 0x01, 0x40, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x40, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20,
                                0x05,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20,
                                0x05,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x40,
                                0x15,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x40,
                                0x15,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        {
            "Terminator",
            FBE_U32_MAX /* Terminator */
        }
    }
};





/*!*******************************************************************
 * @var mumford_the_magician_misc_test_for_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against correctable errors. Injected 
 *        errors are combination of various errors.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_misc_test_for_raid_6_non_degraded = 
{
    "Correctable write and lba stamp test cases for non-degraded RAID 6 only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: CRC and LBA stamp errors in same strip of data drives, block(s) = -MANY-, width = 0x8
         */
        {
            "CRC and LBA stamp errors in same strip of -data drives- on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x80, 0x0,  0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x10, 0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x20,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x10,
                                0x60,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] 
         * Brief: CRC and time-stamp errors in same strip of data drives, block(s) = -MANY-, width = 0x8
         */
        {
            "CRC and time stamp errors in same strip of -data drives- on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x80, 0x0,  0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x10, 0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x20,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x10,
                                0x60,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] 
         * Brief: LBA-stamp and time-stamp errors in same strip of data drives, block(s) = -MANY-, width = 0x8
         */
        {
            "LBA-stamp and time stamp errors in same strip of -data drives- on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x100,  0x200 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x80, 0x0,  0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x40, 0x10, 0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x20,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x10,
                                0x60,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: CRC and LBA-stamp errors in same strip but at data and row parity drive, block(s) = -MANY-, width = 0x8
         */
        {
            "CRC and LBA-stamp errors in same strip but at -data and row parity drive- on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0,  0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x80, 0x10, 0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x20,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x10,
                                0x60,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T004] 
         * Brief: CRC and time-stamp errors in same strip but at data and row parity drive, block(s) = -MANY-, width = 0x8
         */
        {
            "CRC and time stamp errors in same strip but at -data and row parity drive- on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0,  0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x10, 0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x20,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x10,
                                0x60,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] 
         * Brief: LBA-stamp and time-stamp errors in same strip but at data and row parity drive, block(s) = -MANY-, width = 0x8
         */
        {
            "LBA-stamp and time stamp errors in same strip but at -data and row parity drive- on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x80, 0x0,  0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x02, 0x10, 0x61, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x20,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x10,
                                0x61,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T006] 
         * Brief: CRC and time-stamp errors in same strip of both parity dirves, block(s) = -MANY-, width = 0x8
         */
        {
            "CRC and time stamp errors in same strip of -row and diagonal parity dirves- on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0,  0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x10, 0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x20,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x10,
                                0x60,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T007] 
         * Brief: CRC, time-stamp and LBA-stamp errors but no strip with more than two error, block(s) = -MANY-, width = 0x8
         */
        {
            "CRC, time-stamp and LBA-stamp errors but -no strip- infected with more than two errors on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0,  0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x10, 0x40, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x20, 0x50, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 }
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x20,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x10,
                                0x40,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x50,
                                0x10,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

                
        /* [T008] 
         * Brief: CRC, time-stamp and LBA-stamp errors but no strip with more than two error, block(s) = -MANY-, width = 0x8
         */
        {
            "CRC, time-stamp and LBA-stamp errors at same data drive on Write/Zero",
            0x8, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0,  0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x80, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x80, 0x50, 0x25, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 }
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x05,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x20,
                                0x10,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x50,
                                0x25,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        {
            "Terminator",
            FBE_U32_MAX /* Terminator */
        }
    }
};



/*!*******************************************************************
 * @var mumford_the_magician_misc_test_for_zeroed_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against correctable errors. Injected 
 *        errors are only on strips which has never seen any I/O.
 *        Objective is to write test cases which will re-produce 
 *        VR_ZEROED extended status bits in event log.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t mumford_the_magician_misc_test_for_zeroed_raid_6_non_degraded = 
{
    "Correctable test cases for freshly bound (i.e. all strip zeroed) for non-degraded RAID 6 only",
    mumford_the_magician_run_single_test_case_on_zeroed_lun,
    {
        /* [T000] 
         * Brief: CRC error(s) at data position of two consecutive strips, block(s) = 0x2, width = 0xa
         */
        {
            "CRC error(s) at data position of first strip on Write",
            0xa, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x2 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x200, 0x0,  0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 }
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x9, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_ZEROED | VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x2,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] 
         * Brief: CRC and LBA stamp error(s) at two different data position strips, block(s) = 0x2, width = 0xa
         * Note: I/O used in this test case should not overlap with any I/O issued in any 
         *       of  above test case of current test group. Otherwise, we will not be able to 
         *       generated VR_ZEROED in extended bit.
         */
        {
            "CRC and LBA stamp error(s) at two different data position strips on Write",
            0xa, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    mumford_the_magician_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x10,  0x5 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x200, 0x10,  0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC, 0 },
                            { 0x100, 0x10,  0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x9, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_ZEROED | VR_SINGLE_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x10,
                                0x5,
                            },
                            { 
                                0x8, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_ZEROED | VR_LBA_STAMP), /* expected error info */
                                0x10,
                                0x5,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        {
            "Terminator",
            FBE_U32_MAX /* Terminator */
        }
    }
};



/*!*******************************************************************
 * @var mumford_the_magician_raid_group_config_qual
 *********************************************************************
 * @brief This is the raid group configuration we will use to 
 *        valdiate the correctness of event log messages.
 *
 *********************************************************************/
static fbe_test_event_log_test_configuration_t mumford_the_magician_raid_group_config_qual[] = 
{
    /* RAID 3 and RAID 5 configuration */
    {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,       2,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,       4,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &mumford_the_magician_basic_crc_test_for_raid_5_non_degraded,
            &mumford_the_magician_basic_stamp_test_for_raid_5_non_degraded,
            &mumford_the_magician_misc_error_test_for_raid_5_non_degraded,
            NULL,
        }
    },


    /* Only RAID 5 configuration */
    {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,       2,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &mumford_the_magician_crc_retry_test_for_raid_5_non_degraded,
            NULL,
        }
    },

    /* Only RAID 3 configuration */
    {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,       4,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &mumford_the_magician_crc_retry_test_for_raid_3_non_degraded,
            NULL,
        }
    },


   /* RAID 6 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {8,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,    FBE_CLASS_ID_PARITY,    520,      6,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &mumford_the_magician_basic_crc_test_for_raid_6_non_degraded,
            &mumford_the_magician_basic_ts_test_for_raid_6_non_degraded,
            &mumford_the_magician_basic_ws_and_ls_test_for_raid_6_non_degraded,
            &mumford_the_magician_advanced_crc_test_for_raid_6_non_degraded,
            &mumford_the_magician_misc_test_for_raid_6_non_degraded,
        }
   },

   /* RAID 6 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {10,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,    FBE_CLASS_ID_PARITY,    520,   10,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &mumford_the_magician_misc_test_for_zeroed_raid_6_non_degraded,
        }
   },


   /* RAID 1 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {2,       0x8000,   FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,        12,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &mumford_the_magician_basic_test_for_mirror_non_degraded,
            &mumford_the_magician_crc_retry_test_for_mirror_non_degraded,
            NULL,
        }
   },

   /* RAID 10 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,            block size  RAID-id.  bandwidth.*/
            {6,       0xE000,   FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,        16,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {   
            &mumford_the_magician_basic_test_for_r10_only_non_degraded,
            NULL,
        }
   },

   { 
        {   
            { FBE_U32_MAX, /* Terminator. */},
        }
   }
};


/**************************
 * FUNCTION DECLARATION(s)
 *************************/
fbe_u32_t mumford_the_magician_get_test_count(fbe_test_event_log_test_case_t * test_cases_p);
fbe_u32_t mumford_the_magician_get_test_group_count(fbe_test_event_log_test_group_table_t * test_group_array_p);
fbe_u32_t mumford_the_magician_get_config_count(fbe_test_event_log_test_configuration_t * config_p);
fbe_status_t mumford_the_magician_validate_rdgen_result(fbe_status_t rdgen_request_status,
                                                        fbe_rdgen_operation_status_t rdgen_op_status,
                                                        fbe_rdgen_statistics_t *rdgen_stats_p);
fbe_status_t mumford_the_magician_run_test_group(fbe_test_rg_configuration_t * raid_group_p,
                                                 fbe_test_event_log_test_group_t * test_group_p);
fbe_u32_t mumford_the_magician_get_io_spec_count(fbe_test_event_log_test_io_spec_t * io_specification_p);
void mumford_the_magician_test(void);
void mumford_the_magician_setup(void);
fbe_status_t mumford_the_magician_init_logical_error_rec(fbe_api_logical_error_injection_record_t * error_rec_p,
                                                         fbe_u16_t pos_bitmap,
                                                         fbe_lba_t lba,
                                                         fbe_u32_t blocks,
                                                         fbe_api_logical_error_injection_type_t err_type,
                                                         fbe_api_logical_error_injection_mode_t err_mode,
                                                         fbe_u32_t err_limit);
fbe_bool_t mumford_the_magician_is_message_present(fbe_package_id_t package_id,
                                                   fbe_u32_t error_code,
                                                   fbe_object_id_t rg_object_id,
                                                   fbe_u16_t pos,
                                                   fbe_lba_t lba,
                                                   fbe_u32_t blocks,
                                                   fbe_u32_t err_info,
                                                   fbe_u32_t extra_info);
fbe_status_t mumford_the_magician_enable_error_injection_for_raid_group(fbe_raid_group_type_t raid_type,
                                                                        fbe_object_id_t rg_object_id);
fbe_status_t mumford_the_magician_remove_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_u32_t raid_group_count,
                                                fbe_test_drive_removed_info_t *remove_info_p,
                                                fbe_bool_t b_transition_pvd_fail,
                                                fbe_bool_t b_pull_drive);
fbe_status_t mumford_the_magician_insert_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_u32_t raid_group_count,
                                                fbe_test_drive_removed_info_t *remove_info_p,
                                                fbe_bool_t b_transition_pvd,
                                                fbe_bool_t b_reinsert_drive,
                                                fbe_bool_t b_use_sata);
fbe_status_t mumford_the_magician_run_single_io(fbe_test_rg_configuration_t * raid_group_p, 
                                                fbe_test_event_log_test_io_spec_t * io_spec_p, 
                                                fbe_test_event_log_test_injected_error_info_t * injected_errors_p,
                                                fbe_test_event_log_test_result_t * log_result_p,
                                                fbe_test_event_log_test_validate_rdgen_result_t rdgen_result_check_fn_p,
                                                fbe_bool_t b_write_pattern);
fbe_status_t mumford_the_magician_run_single_test_case_on_zeroed_lun(fbe_test_rg_configuration_t * raid_group_p,
                                                                     fbe_test_event_log_test_case_t * test_case_p);

void mumford_the_magician_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void mumford_adjust_expected_log_message_for_4k(fbe_test_rg_configuration_t * raid_group_p, 
                                           fbe_test_event_log_test_io_spec_t * io_spec_p, 
                                           fbe_test_event_log_test_injected_error_info_t * injected_errors_p,
                                           fbe_test_event_log_test_result_t * log_result_p,
                                           fbe_test_event_log_test_result_t * adjusted_log_result_p); 
fbe_bool_t mumford_has_diff_overlapping_error_ranges(fbe_test_event_log_test_injected_error_info_t * injected_errors_p);
fbe_bool_t mumford_is_4k_raid_group(fbe_test_rg_configuration_t * raid_group_p);

/*************************
 * FUNCTION DEFINITION(s)
 *************************/



/*!**************************************************************
 * mumford_the_magician_get_test_count()
 ****************************************************************
 * @brief
 * Count all test cases of given test group. Group list is having
 * terminator at end.
 *
 * @param - test_cases_p - pointer to array of test cases 
 *
 * @return number of test cases
 *
 * @author
 *  8/23/2010 - Created - Pradnya Patankar
 *
 ****************************************************************/
fbe_u32_t mumford_the_magician_get_test_count(fbe_test_event_log_test_case_t * test_cases_p)
{
    fbe_u32_t count = 0;

    /* Count each entry till we hit terminator.
     */
    while (test_cases_p->width != FBE_U32_MAX)
    {
        count++;
        test_cases_p++;
    }
    return count;
}
/******************************************
 * end mumford_the_magician_get_test_count()
 ******************************************/



/*!**************************************************************
 * mumford_the_magician_get_io_specification_count()
 ****************************************************************
 * @brief
 * Count all I/O specification of given list
 *
 * @param - io_specification_p - pointer to array of io specification 
 *
 * @return number of io specification
 *
 * @author
 *  11/23/2010 - Created - Jyoti Ranjan
 *
 ****************************************************************/
fbe_u32_t mumford_the_magician_get_io_spec_count(fbe_test_event_log_test_io_spec_t * io_specification_p)
{
    fbe_u32_t count = 0;

    /* Count each entry till we hit terminator.
     */
    while (io_specification_p->opcode != FBE_RDGEN_OPERATION_INVALID)
    {
        count++;
        io_specification_p++;
    }
    return count;
}
/******************************************
 * end mumford_the_magician_get_io_specification_count()
 ******************************************/




/*!**************************************************************
 *  mumford_the_magician_init_logical_error_rec()
 ****************************************************************
 * @brief
 * Initializes logical error record
 *
 * @param - error_rec_p - pointer to array of io specification 
 * @param - pos_bitmap  - bitmap for each position to error 
 * @param - lba         - physical address to begin errs
 * @param - blocks;     - blocks to extend the errs for
 * @param - err_type    - type of errors
 * @param - err_mode    - mode of error injection
 * @param - err_limit   - maximum number of times a give error needs to be injected
 *
 * @return fbe_status_t
 *
 * @author
 *  11/23/2010 - Created - Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t mumford_the_magician_init_logical_error_rec(fbe_api_logical_error_injection_record_t * error_rec_p,
                                                         fbe_u16_t pos_bitmap,
                                                         fbe_lba_t lba,
                                                         fbe_u32_t blocks,
                                                         fbe_api_logical_error_injection_type_t err_type,
                                                         fbe_api_logical_error_injection_mode_t err_mode,
                                                         fbe_u32_t err_limit)
{
    error_rec_p->pos_bitmap = pos_bitmap; /* Position at which error needs to be injected */    
    error_rec_p->width = 0x10;            /* Width of array */            
    error_rec_p->lba = lba;               /* PBA at which error injection has to start */            
    error_rec_p->blocks = blocks;         /* Number of consecutive blocks at which error(s) have to be injected */           
    error_rec_p->err_type = err_type;     /* Type of error to be injected */
    error_rec_p->err_mode = err_mode;     /* Mode of error injection, viz. always, once etc */
    error_rec_p->err_count = 0x0;         /* Indicates number of times error injected so far */      
    error_rec_p->err_limit = err_limit;   /* Indicates maximum number of times errors have to be injected. */
    error_rec_p->skip_count = 0x0;        /* Indicates number of times error injection has been skipped so far */    
    error_rec_p->skip_limit = 0x15;       /* Indicates amximum number of time errors injection have to be skipped */
    error_rec_p->err_adj = 0x0 ;          /* Error adjacency bitmask */
    error_rec_p->start_bit = 0x0 ;        /* Starting bit of an error */
    error_rec_p->num_bits = 0x0;          /* Number of bits of an error*/
    error_rec_p->bit_adj = 0x0;           /* Indicates if erroneous bits adjacent */
    error_rec_p->crc_det = 0x0;           /* Indicates if CRC detectable */
    error_rec_p->opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;

    return FBE_STATUS_OK;
}
/******************************************
 * end mumford_the_magician_init_logical_error_rec()
 ******************************************/


/*!**************************************************************
 * mumford_the_magician_get_test_group_count()
 ****************************************************************
 * @brief
 *  Get the number of test groups we will be testing.
 *
 * @param test_group_table_p - pointer to table of test groups
 *
 * @return number of test groups.
 *
 * @author
 *  10/20/2010 - Created - Pradnya Patankar
 *
 ****************************************************************/
fbe_u32_t mumford_the_magician_get_test_group_count(fbe_test_event_log_test_group_table_t * test_group_table_p)
{
    fbe_u32_t test_group_count = 0;

    /* Loop until we hit the terminator.
     */
    while (test_group_table_p->table[test_group_count] != NULL)
    {
        test_group_count++;
    }

    return test_group_count;
}
/**********************************************
 * end mumford_the_magician_get_test_group_count()
 *********************************************/


/*!**************************************************************
 * mumford_the_magician_get_config_count()
 ****************************************************************
 * @brief
 *  Get the number of configs we will be working with.
 *
 * @param config_p - array of configs. 
 *
 * @return fbe_u32_t number of configs.   
 *
 * @author
 *  10/20/2010 - Created - Pradnya Patankar
 *
 ****************************************************************/
fbe_u32_t mumford_the_magician_get_config_count(fbe_test_event_log_test_configuration_t *config_p)
{
    fbe_u32_t config_count = 0;
    /* Loop until we find the table number or we hit the terminator.
     */
    while (config_p->raid_group_config[0].width != FBE_U32_MAX)
    {
        config_count++;
        config_p++;
    }
    return config_count;
}
/**********************************************
 * end mumford_the_magician_get_config_count()
 **********************************************/



/*!**************************************************************
 * mumford_the_magician_validate_rdgen_result()
 ****************************************************************
 * @brief
 * This function validates the IO statistics of the generated IO.
 *
 * @param rdgen_request_status - status of reqquest sent using rdgen
 * @param rdgen_op_status      - rdgen operation status
 * @param rdge_stats_p         - pointer to rdgen I/O statistics
 *
 * @return None.
 *
 * @author
 *  10/19/2010 - Created - Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t mumford_the_magician_validate_rdgen_result(fbe_status_t rdgen_request_status,
                                                        fbe_rdgen_operation_status_t rdgen_op_status,
                                                        fbe_rdgen_statistics_t *rdgen_stats_p)
{
    /* In case of correctable error we do not expect any error.
     * So valdiation is very simple.
     */
    MUT_ASSERT_INT_EQUAL(rdgen_stats_p->error_count, 0);
    MUT_ASSERT_INT_EQUAL(rdgen_stats_p->aborted_error_count, 0);
    MUT_ASSERT_INT_EQUAL(rdgen_stats_p->io_failure_error_count, 0);
    MUT_ASSERT_INT_EQUAL(rdgen_stats_p->media_error_count, 0);

    return FBE_STATUS_OK;
}
/**********************************************
 * end mumford_the_magician_validate_io_statistics()
 **********************************************/




/*!**************************************************************
 * mumford_the_magician_is_message_present()
 ****************************************************************
 * @brief
 *  Checks event log to figure out whether an event log message 
 *  with given attributes is present or not.
 *
 * @param package_id   - pacakge id
 * @param error_code   - error code of expected message
 * @param rg_object_id - expected raid group object id 
 * @param pos          - expected error position
 * @param lba          - expected start error lba
 * @param blocks       - expected number of errored blocks
 * @param err_info     - expected error info 
 * @param extra_info   - expected extra info (containing algorithm etc)
 * 
 * @return FBE_TRUE if message is present else FBE_FALSE
 *
 * @author
 *  10/19/2010 - Created - Jyoti Ranjan
 *
 ****************************************************************/
fbe_bool_t mumford_the_magician_is_message_present(fbe_package_id_t package_id,
                                                   fbe_u32_t error_code,
                                                   fbe_object_id_t rg_object_id,
                                                   fbe_u16_t pos,
                                                   fbe_lba_t lba,
                                                   fbe_u32_t blocks,
                                                   fbe_u32_t err_info,
                                                   fbe_u32_t extra_info)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t msg_lba;
    fbe_object_id_t msg_object_id;
    fbe_u32_t msg_fru_pos, msg_blocks, msg_err_info, msg_extra_info;
    fbe_char_t * substr_p = NULL;
    fbe_u32_t event_log_index;
    fbe_u32_t total_events_copied = 0;


    /* Get all event log. 
     * events_array is allocated in common_setup, and checked at the beginning of the test 
     */
    status = fbe_api_get_all_events_logs(events_array, &total_events_copied, FBE_EVENT_LOG_MESSAGE_COUNT,
                                                                 package_id);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "== Error: failed to get all event logs  == \n");
    
    /* Match the expected message with message present in event log
     */
    for(event_log_index = 0; event_log_index < total_events_copied; event_log_index++)
    {
        if (events_array[event_log_index].event_log_info.msg_id != error_code)
        {
            continue;
        }

        /* Found a string with matching error code. Extract relevant 
         * fields from the order in which we do report event log 
         * message. Currently, event log message reports the information
         * in following order:
         *      1. Object id
         *      2. position at which error occured
         *      3. LBA at which error occured
         *      4. Number of errored blocks
         *      5. Error info
         *      6. Extra info (viz algorithm)
         */

         substr_p = strtok(events_array[event_log_index].event_log_info.msg_string,":");
         substr_p = strtok (NULL, ":");
         sscanf(substr_p, "%x", &msg_object_id);
         if ((rg_object_id != MUMFORD_THE_MAGICIAN_TEST_WILD_CARD) && (msg_object_id != rg_object_id))  
         { 
             continue; 
         }
         substr_p = strtok (NULL, ":");
         sscanf(substr_p, "%x", &msg_fru_pos);
         if ((pos != (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD) && (msg_fru_pos != pos)) 
         { 
             continue; 
         }
         substr_p = strtok (NULL, ":");
         sscanf(substr_p, "%llx", &msg_lba);
         if ((lba != MUMFORD_THE_MAGICIAN_TEST_WILD_CARD) && (msg_lba != lba)) 
         { 
             continue; 
         }
         substr_p = strtok (NULL, ":");
         sscanf(substr_p, "%x", &msg_blocks);
         if ((blocks != MUMFORD_THE_MAGICIAN_TEST_WILD_CARD) && (msg_blocks != blocks)) 
         { 
             continue; 
         }
         substr_p = strtok (NULL, ":");
         sscanf(substr_p, "%x", &msg_err_info);
         if ((err_info != MUMFORD_THE_MAGICIAN_TEST_WILD_CARD) && (msg_err_info != err_info)) 
         { 
             continue; 
         }
         substr_p = strtok (NULL, ":");
         sscanf(substr_p, "%x", &msg_extra_info);
         if ((extra_info != MUMFORD_THE_MAGICIAN_TEST_WILD_CARD) && (msg_extra_info != extra_info)) 
         { 
             continue; 
         }

         /* All parameters matched. We found the expected message
          * in event log. 
          */
         return (FBE_TRUE);
    }

    return FBE_FALSE;
}
/**********************************************
 * end mumford_the_magician_is_message_present()
 **********************************************/

void fbe_test_disable_raid_group_background_operation(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_base_config_background_operation_t background_op)
{
    fbe_status_t status;
    fbe_bool_t b_is_enabled;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
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
            status = fbe_api_base_config_disable_background_operation(downstream_object_list.downstream_object_list[index], background_op);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            fbe_api_base_config_is_background_operation_enabled(downstream_object_list.downstream_object_list[index], 
                                                                background_op, &b_is_enabled);
            MUT_ASSERT_INT_EQUAL(FBE_FALSE, b_is_enabled);
        }
    }
    else
    {
        status = fbe_api_base_config_disable_background_operation(rg_object_id, background_op);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_api_base_config_is_background_operation_enabled(rg_object_id, background_op, &b_is_enabled);
        MUT_ASSERT_INT_EQUAL(FBE_FALSE, b_is_enabled);
    }
}

void fbe_test_enable_raid_group_background_operation(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_base_config_background_operation_t background_op)
{
    fbe_status_t status;
    fbe_bool_t b_is_enabled;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
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
            status = fbe_api_base_config_enable_background_operation(downstream_object_list.downstream_object_list[index], background_op);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            fbe_api_base_config_is_background_operation_enabled(downstream_object_list.downstream_object_list[index], 
                                                                background_op, &b_is_enabled);
            MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_is_enabled);
        }
    }
    else
    {
        status = fbe_api_base_config_enable_background_operation(rg_object_id, background_op);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_api_base_config_is_background_operation_enabled(rg_object_id, background_op, &b_is_enabled);
        MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_is_enabled);
    }
}

/*!**************************************************************
 * mumford_the_magician_run_single_io()
 ****************************************************************
 * @brief
 * Run a single I/O and validates event log message(s) reported
 * whilpe performing I/O.
 *
 * @param raid_group_p - pointer raid group configuration
 * @param io_spec_p    - pointer to io specificatio detail
 * @param injected_errors_p - pointer to injected error records details
 * @param log_result_p - pointer to expected result detail
 * @param rdgen_result_check_fn_p - pointer to function to validate rdgen result
 * @param b_write_pattern - write a known pattern before using I/O
 * 
 * @return fbe_status_t.
 *
 * @author
 *  02/10/2011 - Modified - Jyoti Ranjan
 *
 * @Note
 *
 * Caller of this function is expected to create logical error 
 * records which will be used to inject specific errors.
 ****************************************************************/
fbe_status_t mumford_the_magician_run_single_io(fbe_test_rg_configuration_t * raid_group_p, 
                                                fbe_test_event_log_test_io_spec_t * io_spec_p, 
                                                fbe_test_event_log_test_injected_error_info_t * injected_errors_p,
                                                fbe_test_event_log_test_result_t * log_result_p,
                                                fbe_test_event_log_test_validate_rdgen_result_t rdgen_result_check_fn_p,
                                                fbe_bool_t b_write_pattern)
{
    fbe_status_t status = FBE_STATUS_OK; 
    fbe_status_t io_request_status = FBE_STATUS_OK;

    fbe_object_id_t rg_object_id, logged_object_id, lun_object_id;    
    fbe_api_rdgen_context_t context;

    fbe_bool_t b_poc_injection = FBE_FALSE;
    fbe_bool_t b_error_injected = FBE_TRUE; 
    fbe_api_logical_error_injection_mode_t injection_mode;
    fbe_api_logical_error_injection_get_stats_t injected_error_stats;
    fbe_api_logical_error_injection_record_t error_rec;// = {0};
    fbe_rdgen_options_t rdgen_option;
    fbe_u32_t error_index;
    fbe_test_event_log_test_result_t adjusted_log_result = {0};
    
    /* Check whether error needs to be injected or not. In some cases (for example
     * validation of event log message corrupt CRC operation -ONLY-), we do not 
     * inject error but expect event log message(s).
     */
    b_error_injected = (injected_errors_p->injected_error_count != 0) ? FBE_TRUE : FBE_FALSE;

    
    /* Disable all the records, if there are some already enabled.
     * And, create new set of error records.
     */
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_database_lookup_raid_group_by_number(raid_group_p->raid_group_id, &rg_object_id);
    fbe_api_database_lookup_lun_by_number(raid_group_p->logical_unit_configuration_list->lun_number, 
                                               &lun_object_id);

    /* Write background pattern on all luns to erase old data with known pattern.
     */
    if (b_write_pattern)
    {
        big_bird_write_background_pattern();
    }

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

    if (b_error_injected)
    {
        for(error_index = 0; 
            error_index < injected_errors_p->injected_error_count; 
            error_index++)
        {
            injection_mode = (injected_errors_p->error_list[error_index].err_limit != 0) 
                                 ? (FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT)
                                 : (FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS);

            mumford_the_magician_init_logical_error_rec(&error_rec,
                                                        injected_errors_p->error_list[error_index].pos_bitmap,
                                                        injected_errors_p->error_list[error_index].lba,
                                                        injected_errors_p->error_list[error_index].blocks,
                                                        injected_errors_p->error_list[error_index].err_type,
                                                        injection_mode,
                                                        injected_errors_p->error_list[error_index].err_limit);
            status = fbe_api_logical_error_injection_create_record(&error_rec);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        /* Enable error injection for the current raid group object.
         * Also, enable poc injection if asked for.
         */
        status = mumford_the_magician_enable_error_injection_for_raid_group(raid_group_p->raid_type, rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if (b_poc_injection)
        {
            status = fbe_api_logical_error_injection_enable_poc();
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        status = fbe_api_logical_error_injection_enable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Validate the injected errors are as per our configuration.
         */
        status = fbe_api_logical_error_injection_get_stats(&injected_error_stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(injected_error_stats.b_enabled, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(injected_error_stats.num_records, injected_errors_p->injected_error_count);

    } /* end if (b_error_injected) */

    /* If operation is corrupt CRC or corrupt data then ask rdgen not 
     * to randomize corrupt operation. We do achieve so by setting rdgen
     * oprion accordingly.
     */
    if ((io_spec_p->opcode != FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK) &&
        (io_spec_p->opcode != FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY) &&
        (io_spec_p->opcode != FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK))
    {
        rdgen_option = FBE_RDGEN_OPTIONS_INVALID;
    }
    else
    {
        rdgen_option = FBE_RDGEN_OPTIONS_DO_NOT_RANDOMIZE_CORRUPT_OPERATION;        
    }

    rdgen_option |= FBE_RDGEN_OPTIONS_DISABLE_RANDOM_SG_ELEMENT;

    /* Send I/O
     */
    io_request_status = fbe_api_rdgen_send_one_io(&context,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_PACKAGE_ID_SEP_0,
                                                  io_spec_p->opcode,
                                                  FBE_RDGEN_LBA_SPEC_FIXED,
                                                  io_spec_p->start_lba,
                                                  io_spec_p->blocks,
                                                  rdgen_option,
                                                  0, 
                                                  0, /* no expiration or abort time */
                                                  FBE_API_RDGEN_PEER_OPTIONS_INVALID);


    if (b_error_injected)
    {
        /* Raid 10s only enable the mirrors, so just disable the mirror class.
         */
        if (raid_group_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            status = fbe_api_logical_error_injection_disable_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0);
        }
        else
        {
            status = fbe_api_logical_error_injection_disable_class(raid_group_p->class_id, FBE_PACKAGE_ID_SEP_0);
        }
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Disable error injection. Also, disable POC injection if 
         * we have enabled it earlier.
         */
        status = fbe_api_logical_error_injection_disable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if (b_poc_injection)
        {
            status = fbe_api_logical_error_injection_disable_poc();
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        
        /* Destroy all the objects the logical error injection service is 
         * tracking. This will allow us to start fresh when we start the 
         * next test with number of errors and number of objects reported 
         * by lei.
         */
        status = fbe_api_logical_error_injection_destroy_objects();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    } /* end if (injected_errors_p != NULL) */

    /* Validate rdgen statastics.
     */
    status = rdgen_result_check_fn_p(io_request_status, 
                                     context.start_io.status.rdgen_status, 
                                     &context.start_io.statistics);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

   
    /* In case of stripped-mirror raid group, do not validate object id 
     * against which error is reported. So, we will pass an wild card
     * to function validating event log result.
     */ 
    logged_object_id = (raid_group_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10) 
                            ? (rg_object_id)
                            : (MUMFORD_THE_MAGICIAN_TEST_WILD_CARD);


    mumford_adjust_expected_log_message_for_4k(raid_group_p, io_spec_p, 
                                               injected_errors_p, log_result_p, &adjusted_log_result); 

    /* Validate reported messages with expected one.
     */
    mumford_the_magician_validates_event_log_message(&adjusted_log_result, logged_object_id, FBE_TRUE); /* TRUE = verify message present */

    return FBE_STATUS_OK;
}
/**********************************************
 * end mumford_the_magician_run_single_io()
 **********************************************/

/*!**************************************************************
 * mumford_adjust_expected_log_message_for_4k()
 ****************************************************************
 * @brief
 * Adjust expected error messages according for 4k drives
 *
 * @param raid_group_p - the raid group configuration against which test will be run
 * @param io_spec_p    - pointer to io specificatio detail
 * @param injected_errors_p - pointer to injected error records details
 * @param log_result_p - pointer to expected result detail
 * @param adjusted_log_result_p - pointer to expected event log message 
 * 
 * @return None
 *
 * @author
 *  06/10/2014 - Deanna Heng
 *
 ****************************************************************/
void mumford_adjust_expected_log_message_for_4k(fbe_test_rg_configuration_t * raid_group_p, 
                                           fbe_test_event_log_test_io_spec_t * io_spec_p, 
                                           fbe_test_event_log_test_injected_error_info_t * injected_errors_p,
                                           fbe_test_event_log_test_result_t * log_result_p,
                                           fbe_test_event_log_test_result_t * adjusted_log_result_p) 
{
    fbe_u32_t error_index = 0;
    fbe_u32_t msg_index = 0;
    fbe_u32_t fru_pos;       /*!< the fru position (against reported error) in event log */
    fbe_u32_t error_code;    /*!< the expected error code in event log */
    fbe_u32_t error_info;    /*!< the expected error type in event log */
    fbe_lba_t error_lba;     /*!< the expected lba in event log message */
    fbe_u32_t error_blocks;  /*!< the expected lba in event log message */
    fbe_lba_t aligned_error_lba;
    fbe_lba_t aligned_end_error_lba;
    fbe_lba_t end_error_lba;

    if (raid_group_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        /* RAID0 logs 1 error block, do not align
         */
        adjusted_log_result_p = log_result_p;
        return;
    }
    else if (io_spec_p->opcode == FBE_RDGEN_OPERATION_READ_ONLY && 
             io_spec_p->do_not_align_errors == FBE_TRUE)
    {
        /* If the flag is set to not align the errors, continue.
         * This is required for degraded read operations where the 
         * alignment does not expand for degraded read recovery verify 
         */
        adjusted_log_result_p = log_result_p;
        return;
    }
    else if (!mumford_is_4k_raid_group(raid_group_p))
    {
        adjusted_log_result_p = log_result_p;
        return;
    } 

    adjusted_log_result_p->num_msgs = log_result_p->num_msgs;

    for(msg_index = 0; msg_index < log_result_p->num_msgs; msg_index++)
    {
        
        fru_pos = log_result_p->event_validation_record[msg_index].fru_pos;
        error_code = log_result_p->event_validation_record[msg_index].error_code;
        error_info = log_result_p->event_validation_record[msg_index].error_info;
        error_lba = log_result_p->event_validation_record[msg_index].error_lba;
        error_blocks = log_result_p->event_validation_record[msg_index].error_blocks;
        adjusted_log_result_p->event_validation_record[msg_index].fru_pos = fru_pos;
        adjusted_log_result_p->event_validation_record[msg_index].error_code = error_code;
        adjusted_log_result_p->event_validation_record[msg_index].error_info = error_info;
        adjusted_log_result_p->event_validation_record[msg_index].error_lba = error_lba;
        adjusted_log_result_p->event_validation_record[msg_index].error_blocks = error_blocks;


        end_error_lba = error_lba + error_blocks;
        aligned_error_lba = (error_lba / FBE_520_BLOCKS_PER_4K) * FBE_520_BLOCKS_PER_4K;
        aligned_end_error_lba = ((end_error_lba + FBE_520_BLOCKS_PER_4K - 1) / FBE_520_BLOCKS_PER_4K) * FBE_520_BLOCKS_PER_4K;
        error_blocks = (fbe_u32_t)(aligned_end_error_lba - aligned_error_lba); 

        if ((error_code == SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED ||
            error_code == SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED) &&
            (error_info == VR_CRC_RETRY))
        {
            adjusted_log_result_p->event_validation_record[msg_index].error_lba = aligned_error_lba; 
            adjusted_log_result_p->event_validation_record[msg_index].error_blocks = error_blocks;
                
        }
        else if (mumford_has_diff_overlapping_error_ranges(injected_errors_p)) 
        {
            continue;
        }
        else 
        {
            /* Check the injection range
             * Align the error lbas to 4k range for the aligned prereads
             */
            for (error_index = 0; 
                 error_index < injected_errors_p->injected_error_count; 
                 error_index++)
            {
                fbe_lba_t injected_start = injected_errors_p->error_list[error_index].lba;
                fbe_lba_t injected_end = injected_start + injected_errors_p->error_list[error_index].blocks;
                /* Adjust if the result lba falls within the injected record
                 */
                if (injected_start < end_error_lba && injected_end > error_lba)
                {
                    /* align all error reporting caused by media errors */
                    if (injected_errors_p->error_list[error_index].err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR)
                    {
                        adjusted_log_result_p->event_validation_record[msg_index].error_lba = aligned_error_lba; 
                        adjusted_log_result_p->event_validation_record[msg_index].error_blocks = error_blocks;
                        continue;
                    }

                    /* expected log result is reported based on 520, so we just need to determine
                     * whether the error range needs to be aligned to the beginning
                     * of the error injection or to the aligned write/error report region
                     */
                    if (injected_start >= aligned_error_lba && injected_start < error_lba )
                    {
                            /* ****** <--- aligned io
                               *    *
                               *====* <--- start injection
                               *    *
                               ****** <--- io
                               *    * 
                               ******
                             */
                        adjusted_log_result_p->event_validation_record[msg_index].error_lba = injected_start;

                    }
                    else if (injected_start <= aligned_error_lba) 
                    {
                            /* *====* <--- start injection
                               *    *
                               ****** <--- aligned_io
                               *    * 
                               ****** <--- io
                               *    * 
                               ******
                             */
                        adjusted_log_result_p->event_validation_record[msg_index].error_lba = aligned_error_lba; 
                    } 

                    if (injected_end > aligned_end_error_lba)
                    {
                        error_blocks = (fbe_u32_t)(aligned_end_error_lba - adjusted_log_result_p->event_validation_record[msg_index].error_lba);
                        adjusted_log_result_p->event_validation_record[msg_index].error_blocks = error_blocks; 
                    } 
                    else if (injected_end > end_error_lba)
                    {
                        error_blocks = (fbe_u32_t)(injected_end - adjusted_log_result_p->event_validation_record[msg_index].error_lba);
                        adjusted_log_result_p->event_validation_record[msg_index].error_blocks = error_blocks; 
                    }
                    else 
                    {
                        error_blocks = (fbe_u32_t)(end_error_lba - adjusted_log_result_p->event_validation_record[msg_index].error_lba);
                        adjusted_log_result_p->event_validation_record[msg_index].error_blocks = error_blocks; 
                    }
                }     
            }
        }
    }
}
/**********************************************
 * end mumford_adjust_expected_log_message_for_4k()
 **********************************************/

/*!**************************************************************
 * mumford_has_diff_overlapping_error_ranges()
 ****************************************************************
 * @brief
 *  Determine if the logical error injection has overlapping
 *  but different error ranges. 
 *  i.e. crc error from 0-10, media error 5-15
 *
 * @param injected_errors_p - pointer to injected error records details
 * 
 * @return fbe_bool_t
 *
 * @author
 *  06/11/2014 - Deanna Heng
 *
 ****************************************************************/
fbe_bool_t mumford_has_diff_overlapping_error_ranges(fbe_test_event_log_test_injected_error_info_t * injected_errors_p) 
{
    fbe_u32_t i = 0;
    fbe_u32_t j = 0;


    for(i = 0; i < injected_errors_p->injected_error_count; i++)
    {
        for (j = 0; j< injected_errors_p->injected_error_count; j++)
        {
            if (i == j)
            {
                continue;
            }

            /* There is an uneven overlap if the end of one error range falls inbetween some other error range*/
            if ((injected_errors_p->error_list[i].lba + injected_errors_p->error_list[i].blocks > 
                 injected_errors_p->error_list[j].lba) &&
                (injected_errors_p->error_list[i].lba + injected_errors_p->error_list[i].blocks < 
                 injected_errors_p->error_list[j].lba + injected_errors_p->error_list[j].blocks))
            {
                return FBE_TRUE;
            }
            
        }
    }

    return FBE_FALSE;
}
/**********************************************
 * end mumford_has_diff_overlapping_error_ranges()
 **********************************************/

/*!**************************************************************
 * mumford_is_4k_raid_group()
 ****************************************************************
 * @brief
 *  Determine whether the raid group is 4k
 *
 * @param raid_group_p - pointer to raid group config in question
 * 
 * @return fbe_bool_t
 *
 * @author
 *  06/11/2014 - Deanna Heng
 *
 ****************************************************************/
fbe_bool_t mumford_is_4k_raid_group(fbe_test_rg_configuration_t * raid_group_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t raid_group_info = {0};
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_base_config_downstream_object_list_t downstream_object_list = {0};
    fbe_u32_t index = 0;


    /* Get the raid group object id
     */
    status = fbe_api_database_lookup_raid_group_by_number(raid_group_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (raid_group_info.optimal_block_size == FBE_520_BLOCKS_PER_4K ||
        raid_group_p->block_size == FBE_4K_BYTES_PER_BLOCK)
    {
        return FBE_TRUE;
    }

    if (raid_group_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* For each mirrored pair check if the raid group is 4k
         */
        for (index = 0; 
            index < downstream_object_list.number_of_downstream_objects; 
            index++)
        {
            status = fbe_api_raid_group_get_info(downstream_object_list.downstream_object_list[index], &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if (raid_group_info.optimal_block_size == FBE_520_BLOCKS_PER_4K ||
                raid_group_p->block_size == FBE_4K_BYTES_PER_BLOCK)
            {
                return FBE_TRUE;
            }
        } /* end for each mirrored pair. */
    } 

    return FBE_FALSE;
}
/**********************************************
 * end mumford_is_4k_raid_group()
 **********************************************/


/*!**************************************************************
 * mumford_the_magician_run_single_test_case_on_zeroed_lun()
 ****************************************************************
 * @brief
 * Run a test case on a lun which is freshly bound and no I/O 
 * has occured so far. It injects error and validates the reported
 * result in event log. Primarily, objective is to validate
 * reporting of VR_ZEROED flag. At this point of only RAID 6 has 
 * the ability to detect whether I/O has been issued on 
 * freshly bound lun.
 *
 * @param raid_group_p - the raid group configuration against which test will be run
 * @param test_case_p  - error we want to inject
 * 
 * @return fbe_status_t.
 *
 * @author
 *  02/10/2011 - Modified - Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t mumford_the_magician_run_single_test_case_on_zeroed_lun(fbe_test_rg_configuration_t * raid_group_p,
                                                                     fbe_test_event_log_test_case_t * test_case_p)
{
    fbe_u32_t io_spec_index, io_spec_count;
    fbe_u32_t record_index;

    /* Since we are injecting errors purposefully , we will trace those 
     * sectors that result `critical' error.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
     
    
    mut_printf(MUT_LOG_TEST_STATUS, test_case_p->description);


    /* Iterate through all test records and exercise it.
     */
    for(record_index = 0; record_index < test_case_p->record_count; record_index++)
    {
        /* Validate event log result for each I/O.
         */
        io_spec_count = mumford_the_magician_get_io_spec_count(test_case_p->test_records[record_index].io_spec_list);
        for(io_spec_index = 0; io_spec_index < io_spec_count; io_spec_index++)
        {
            /* Issue I/O without writing any known pattern.
             */
            mumford_the_magician_run_single_io(raid_group_p, 
                                               &test_case_p->test_records[record_index].io_spec_list[io_spec_index],
                                               &test_case_p->test_records[record_index].injected_errors,
                                               &test_case_p->test_records[record_index].event_log_result,
                                               test_case_p->test_records[record_index].validate_rdgen_result_fn_p,
                                               FBE_FALSE);
            

            mut_printf(MUT_LOG_TEST_STATUS, 
                       "test record [%d]: validated event log messages reported by rdgen I/O 0x%x [of 0x%x] - "
                       "opcode=0x%x, start_pba=0x%llx, blocks=0x%llx",
                       record_index,
                       io_spec_index, 
                       io_spec_count,
                       test_case_p->test_records[record_index].io_spec_list[io_spec_index].opcode,
                       (unsigned long long)test_case_p->test_records[record_index].io_spec_list[io_spec_index].start_lba,
                       (unsigned long long)test_case_p->test_records[record_index].io_spec_list[io_spec_index].blocks);
        } 
    } /* end  for(record_index = 0; record_index < test_case_p->record_count; record_index++) */
 

    /* Restore the sector trace level to it's default.
     */
    fbe_test_sep_util_restore_sector_trace_level();


    return FBE_STATUS_OK;
}
/****************************************************************
 * end mumford_the_magician_run_single_test_case_on_zeroed_lun()
 ***************************************************************/


/*!**************************************************************
 * mumford_the_magician_run_single_test_case()
 ****************************************************************
 * @brief
 * Run a test case which injects error and validates the reported
 * result in event log. 
 *
 * @param raid_group_p - the raid group configuration against which test will be run
 * @param test_case_p  - error we want to inject
 * 
 * @return fbe_status_t.
 *
 * @author
 *  10/19/2010 - Created -  Pradnya Patankar
 *  10/28/2010 - Modified - Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t mumford_the_magician_run_single_test_case(fbe_test_rg_configuration_t * raid_group_p,
                                                       fbe_test_event_log_test_case_t * test_case_p)
{
    fbe_u32_t io_spec_index, io_spec_count;
    fbe_bool_t b_write_pattern;
    fbe_u32_t record_index;
      

    /* Since we are injecting errors purposefully , we will trace those 
     * sectors that result `critical' error.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
     
    mut_printf(MUT_LOG_TEST_STATUS, test_case_p->description);

    /* Iterate through all test records and exercise it.
     */
    for(record_index = 0; record_index < test_case_p->record_count; record_index++)
    {
        /* Set flag appropriately which is used to indicate whether we do want 
         * to write a known pattern before issuing rdgen I/O or not. If test case 
         * has more than one records then we will not be writing a know pattern 
         * after exercising first record.
         */
        b_write_pattern = (record_index != 0) ? FBE_FALSE : FBE_TRUE;


        /* Validate event log result for each I/O.
         */
        io_spec_count = mumford_the_magician_get_io_spec_count(test_case_p->test_records[record_index].io_spec_list);
        for(io_spec_index = 0; io_spec_index < io_spec_count; io_spec_index++)
        {
            mumford_the_magician_run_single_io(raid_group_p, 
                                               &test_case_p->test_records[record_index].io_spec_list[io_spec_index],
                                               &test_case_p->test_records[record_index].injected_errors,
                                               &test_case_p->test_records[record_index].event_log_result,
                                               test_case_p->test_records[record_index].validate_rdgen_result_fn_p,
                                               b_write_pattern);
            
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "test record [%d] : validated event log messages reported by rdgen I/O 0x%x [of 0x%x] - "
                       "opcode=0x%x, start_pba=0x%llx, blocks=0x%llx",
                       record_index,
                       io_spec_index, 
                       io_spec_count,
                       test_case_p->test_records[record_index].io_spec_list[io_spec_index].opcode,
                       (unsigned long long)test_case_p->test_records[record_index].io_spec_list[io_spec_index].start_lba,
                       (unsigned long long)test_case_p->test_records[record_index].io_spec_list[io_spec_index].blocks);
        } 
    } /* end for(record_index = 0; record_index < test_case_p->record_count; record_index++) */
 

    /* Restore the sector trace level to it's default.
     */
    fbe_test_sep_util_restore_sector_trace_level();

    return FBE_STATUS_OK;
}
/******************************************
 * end mumford_the_magician_run_single_test_case()
 ******************************************/

#if 0 /*!@note -- Useful temp variable (enable with count code below) to verify no unexpected ELOG msgs */
      /* Final count displayed in app log should equal number of lines in SP log containing "Extra info: 0x" */
static fbe_u32_t total_expected_ELOG_messages = 0;
#endif

/*!**************************************************************
 * mumford_the_magician_validates_event_log_message()
 ****************************************************************
 * @brief
 * It validates whether expected event log message(s) is/are 
 * present in event log or not.
 *
 * @param result_p - pointer to expected event log message 
 * @param logged_object_id - object ID against which messages were logged.
 *
 * @return fbe_status_t.
 *
 * @author
 *  02/10/2011 - Created -  Jyoti Ranjan
 *
 * @Note:
 *
 * Caller of this fucntion is expected to exrecise a sceanrio 
 * which will result in logging of expected messages 
 * to be present in event log.
 **************************************************************/
fbe_status_t mumford_the_magician_validates_event_log_message(fbe_test_event_log_test_result_t * result_p,
                                                              fbe_object_id_t logged_object_id,
                                                              fbe_bool_t b_verify_present)
{
    fbe_bool_t b_present = FBE_FALSE;
    fbe_u32_t msg_index;

    for(msg_index = 0; msg_index < result_p->num_msgs; msg_index++)
    {
        b_present = mumford_the_magician_is_message_present(FBE_PACKAGE_ID_SEP_0,
                                                            result_p->event_validation_record[msg_index].error_code,
                                                            logged_object_id, 
                                                            result_p->event_validation_record[msg_index].fru_pos,
                                                            result_p->event_validation_record[msg_index].error_lba,
                                                            result_p->event_validation_record[msg_index].error_blocks,
                                                            result_p->event_validation_record[msg_index].error_info,
                                                            MUMFORD_THE_MAGICIAN_TEST_WILD_CARD /* ignore algorithm */);

        /* Check the status of event log
         */
        if ((b_present != FBE_TRUE) && (b_verify_present == FBE_TRUE))
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "Expected message[%d]: msg_id=0x%x, rg_object_id=0x%x, fru_pos=0x%x, "
                       "error_lba=0x%llx, error_blocks=0x%x, error_info=0x%x, algorithm=%s",
                       msg_index,
                       result_p->event_validation_record[msg_index].error_code, 
                       logged_object_id, 
                       result_p->event_validation_record[msg_index].fru_pos,
                       (unsigned long long)result_p->event_validation_record[msg_index].error_lba,
                       result_p->event_validation_record[msg_index].error_blocks,
                       result_p->event_validation_record[msg_index].error_info,
                       "WILD_CARD" /* wild card */);

            MUT_FAIL_MSG("== Either message is not present or is not as expected! ==");
        }
        else if ((b_present == FBE_TRUE) && (b_verify_present != FBE_TRUE))
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "Unexpected message[%d]: msg_id=0x%x, rg_object_id=0x%x, fru_pos=0x%x, "
                       "error_lba=0x%llx, error_blocks=0x%x, error_info=0x%x, algorithm=%s",
                       msg_index,
                       result_p->event_validation_record[msg_index].error_code, 
                       logged_object_id, 
                       result_p->event_validation_record[msg_index].fru_pos,
                       result_p->event_validation_record[msg_index].error_lba,
                       result_p->event_validation_record[msg_index].error_blocks,
                       result_p->event_validation_record[msg_index].error_info,
                       "WILD_CARD" /* wild card */);

            MUT_FAIL_MSG("== Message is present but should not be! ==");
        }
#if 0 /*!@note -- Useful temp code (enable with static variable above) to verify no unexpected ELOG msgs */
        if (b_present == FBE_TRUE)
        {
            total_xpected_ELOG_messages++;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "MSG COUNT: expected message count now: %d ",
                       total_expected_ELOG_messages);
        }
#endif
    } /* end for(msg_index = 0; msg_index < test_case_p->event_log_result.num_msgs; msg_index++) */

    return FBE_STATUS_OK;
}
/*********************************************************
 * end mumford_the_magician_validates_event_log_message()
 ********************************************************/

/*!**************************************************************
 * mumford_the_magician_run_test_group()
 ****************************************************************
 * @brief
 * Run all test cases of given test group and validate event log 
 * messages reported (if any) on detection of error.
 *
 * @param raid_group_p      - raid group configuration against which test will be run
 * @param test_group_p      - ponter to test group
 * 
 * @return fbe_status_t.
 *
 * @author
 *  10/25/2010 - Created - Pradnya Patankar
 *  10/28/2010 - Modified - Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t mumford_the_magician_run_test_group(fbe_test_rg_configuration_t * raid_group_p,
                                                 fbe_test_event_log_test_group_t * test_group_p)
{
    fbe_u32_t test_index = 0;
    fbe_u32_t test_case_count = mumford_the_magician_get_test_count(&test_group_p->test_case_list[0]);
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status;

    fbe_api_database_lookup_raid_group_by_number(raid_group_p->raid_group_id, &rg_object_id);
    if (raid_group_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1 || 
        raid_group_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* Set mirror perfered position to disable mirror read optimization */
        status = fbe_api_raid_group_set_mirror_prefered_position(rg_object_id, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    for(test_index = 0; test_index < test_case_count; test_index++)
    {
        /* Just do a sanity check before running test case 
         * whether a given test case is valid for current raid 
         * group configuration or not.
         */
        if (test_group_p->test_case_list[test_index].width != raid_group_p->width)
        {
            continue;
        }

        mut_printf(MUT_LOG_LOW, "running test no = %d (of total 0x%x) for raid type = 0x%x", 
                   test_index, test_case_count, raid_group_p->raid_type);
        fbe_test_disable_raid_group_background_operation(raid_group_p, FBE_RAID_GROUP_BACKGROUND_OP_VERIFY_ALL);

        /* Now execute the test cases
         */
        test_group_p->execute_single_test_case_fn_p(raid_group_p,
                                                    &test_group_p->test_case_list[test_index]);

        fbe_test_enable_raid_group_background_operation(raid_group_p, FBE_RAID_GROUP_BACKGROUND_OP_VERIFY_ALL);

        /* Only mirrors might see different behavior if a verify is in progress (we only read from the first part of the 
         * mirror). 
         */
        if (((raid_group_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
            (raid_group_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)) &&
            !fbe_test_rg_is_degraded(raid_group_p))
        {
            abby_cadabby_wait_for_verifies(raid_group_p, 1);
        }

        mut_printf(MUT_LOG_LOW, "test no = %d (of total 0x%x) passed for raid type = 0x%x", 
                   test_index, test_case_count, raid_group_p->raid_type);
    }

    if (raid_group_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1 || 
        raid_group_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* Set mirror perfered position to enable mirror read optimization */
        status = fbe_api_raid_group_set_mirror_prefered_position(rg_object_id, 0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end mumford_the_magician_run_test_group()
 ******************************************/


/*!**************************************************************
 * mumford_the_magician_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param config_p - Current config.
 *
 * @return None.
 *
 * @author
 *  10/21/2010 - Created - Pradnya Patankar
 *
 ****************************************************************/
void mumford_the_magician_load_physical_config(fbe_test_event_log_test_configuration_t * config_array_p)
{
    fbe_u32_t                   config_index;
    fbe_u32_t                   CSX_MAYBE_UNUSED extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t raid_group_count;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;
    fbe_u32_t current_num_520_drives = 0;
    fbe_u32_t current_num_4160_drives = 0;
    fbe_u32_t config_count = mumford_the_magician_get_config_count(config_array_p);

    for (config_index = 0; config_index < config_count; config_index++)
    {
        rg_config_p = &config_array_p[config_index].raid_group_config[0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }

        raid_group_count = fbe_test_get_rg_array_length(&config_array_p[config_index].raid_group_config[0]);

        if (raid_group_count == 0)
        {
            continue;
        }

        /* First get the count of drives.
         */
        fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, 
                                                           raid_group_count,
                                                           &current_num_520_drives,
                                                           &current_num_4160_drives);
        num_520_drives = FBE_MAX(num_520_drives, current_num_520_drives);
        num_4160_drives = FBE_MAX(num_4160_drives, current_num_4160_drives);

        mut_printf(MUT_LOG_TEST_STATUS, "%s: %s %d 520 drives and %d 4160 drives", 
                   __FUNCTION__, raid_type_string_p, current_num_520_drives, current_num_4160_drives);
    }


    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 512 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives);

    /* Add a configuration to each raid group config.
     */
    for (config_index = 0; config_index < config_count; config_index++)
    {
        rg_config_p = &config_array_p[config_index].raid_group_config[0];
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        
        raid_group_count = fbe_test_get_rg_array_length(&config_array_p[config_index].raid_group_config[0]);
        if (raid_group_count == 0)
        {
            continue;
        }
        fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, raid_group_count, 
                                            num_520_drives, num_4160_drives);
    }

    fbe_test_pp_util_create_physical_config_for_disk_counts(num_520_drives,
                                                            num_4160_drives,
                                                            TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
    return;
}
/*************************************************
 * end mumford_the_magician_load_physical_config()
 *************************************************/

/*!**************************************************************
 * mumford_the_magician_enable_error_injection_for_raid_groups()
 ****************************************************************
 * @brief
 *  For a given raid group of given raid type, enables error injection.
 *
 * @param raid_type    - raid group type
 * @param rg_object_id - Object id to enable error injection
 *
 * @return fbe_status_t 
 *
 * @author
 *  12/8/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t mumford_the_magician_enable_error_injection_for_raid_group(fbe_raid_group_type_t raid_type,
                                                                        fbe_object_id_t rg_object_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

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

    return status;
}
/********************************************************************
 * end mumford_the_magician_enable_error_injection_for_raid_groups()
 *******************************************************************/



/*!**************************************************************
 * mumford_the_magician_remove_drives()
 ****************************************************************
 * @brief 
 * Remove drives from the given raid group configuration
 *
 * @param rg_config_p           - raid group test config
 * @param raid_group_count      - number of raid group
 * @param remove_info_p         - the list of drives to remove
 * @param b_transition_pvd_fail - true to not using terminator to remove drive
 * @param b_pull_drive          - true to remove and later reinsert the same drive.                   
 *
 * @return fbe_status_t
 *
 * @author
 *  01/18/2011 - Jyoti Ranjan Created
 *
 ****************************************************************/
fbe_status_t mumford_the_magician_remove_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_u32_t raid_group_count,
                                                fbe_test_drive_removed_info_t *remove_info_p,
                                                fbe_bool_t b_transition_pvd_fail,
                                                fbe_bool_t b_pull_drive)
{
    fbe_u32_t drive_index;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_status_t status;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t num_removed = remove_info_p->num_removed;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;

    for(rg_index =0; rg_index < raid_group_count; rg_index++)
    {
        if(!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p ++;
            continue;
        }
        /* Remove all the drives specified by the preset array.
         */
        for (drive_index = 0; drive_index < num_removed; drive_index++)
        {
            fbe_u32_t position_to_remove = remove_info_p->drives_removed_array[drive_index];
            if (position_to_remove == FBE_U32_MAX)
            {
                num_removed = drive_index;
                break;
            }

            drive_to_remove_p = &current_rg_config_p->rg_disk_set[position_to_remove];
            mut_printf(MUT_LOG_TEST_STATUS,
                "%s: Removing drive raid group: %d position: 0x%x (%d_%d_%d)",
                __FUNCTION__, rg_config_p->raid_group_id, position_to_remove,
                drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot);

            if (b_transition_pvd_fail)
            {
                fbe_object_id_t pvd_id;
                status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus,
                                                                        drive_to_remove_p->enclosure,
                                                                        drive_to_remove_p->slot,
                                                                        &pvd_id);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                status = fbe_api_set_object_to_state(pvd_id, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_SEP_0);
            }
            else if (b_pull_drive)
            {
                /* We are going to pull and reinsert the drive (with original data in place).
                 */
                status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                     drive_to_remove_p->enclosure, 
                                                     drive_to_remove_p->slot,
                                                     &current_rg_config_p->drive_handle[position_to_remove]);
            }
            else
            {
                /* We are going to remove and reinsert.
                 */
                if (rg_config_p->block_size == 520)
                {
                    status = fbe_test_pp_util_remove_sas_drive(drive_to_remove_p->bus, 
                                                               drive_to_remove_p->enclosure, 
                                                               drive_to_remove_p->slot);
                }
                else
                {
                    /* Remove a drive from the 512 raid group.
                     */
                    status = fbe_test_pp_util_remove_sata_drive(drive_to_remove_p->bus, 
                                                                drive_to_remove_p->enclosure, 
                                                                drive_to_remove_p->slot);
                }
            }
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        } /* end for (drive_index = 0; drive_index < num_removed; drive_index++) */
    
        /* Now wait for the drives to be destroyed.
         * We intentionally separate out the waiting from the actions to allow all the 
         * transitioning of drives to occur in parallel. 
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        for (drive_index = 0; drive_index < num_removed; drive_index++)
        {
            fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id,
                                                                               remove_info_p->drives_removed_array[drive_index],
                                                                               &vd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);
            status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                             MUMFORD_THE_MAGICIAN_WAIT_PERIOD_FOR_REMOVAL, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        } /* end for (drive_index = 0; drive_index < num_removed; drive_index++) */

        current_rg_config_p++;
    } /* end for(rg_index =0; rg_index < raid_group_count; rg_index++) */

    return FBE_STATUS_OK;
}
/******************************************
 * end mumford_the_magician_remove_drives()
 ******************************************/


                                                
/*!**************************************************************
 * mumford_the_magician_insert_drives()
 ****************************************************************
 * @brief 
 * Insert drives for the given raid group configuration
 *
 * @param rg_config_p      - raid group test config
 * @param raid_group_count - number of raid group
 * @param remove_info_p    - the list of drives to remove
 * @param b_transition_pvd - True to not use terminator to reinsert.
 * @param b_reinsert_drive - True if we used the pull and now
 *                           need to use the reinsert.
 * @param b_use_sata       - true to insert a sata drive for 512.
 *
 *
 * @return fbe_status_t
 *
 * @author
 *  01/18/2011 - Jyoti Ranjan Created
 *
 ****************************************************************/
fbe_status_t mumford_the_magician_insert_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_u32_t raid_group_count,
                                                fbe_test_drive_removed_info_t *remove_info_p,
                                                fbe_bool_t b_transition_pvd,
                                                fbe_bool_t b_reinsert_drive,
                                                fbe_bool_t b_use_sata)
{
    fbe_u32_t drive_index;
    fbe_u32_t rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_status_t status;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t num_removed = remove_info_p->num_removed;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;

    for(rg_index =0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t position_to_insert;

        if(!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p ++;
            continue;
        }

        /* Remove all the drives specified by the preset array.
         */
        for (drive_index = 0; drive_index < num_removed; drive_index++)
        {
            position_to_insert = remove_info_p->drives_removed_array[drive_index];
            drive_to_insert_p = &current_rg_config_p->rg_disk_set[position_to_insert];
            
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "== %s inserting rg %d position %d (%d_%d_%d). ==", 
                       __FUNCTION__, rg_index, position_to_insert, 
                       drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);
            
            if (b_transition_pvd)
            {
                status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_insert_p->bus,
                                                                        drive_to_insert_p->enclosure,
                                                                        drive_to_insert_p->slot,
                                                                        &pvd_object_id);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                status = fbe_api_set_object_to_state(pvd_object_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
            else if (b_reinsert_drive)
            {
                status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                         drive_to_insert_p->enclosure, 
                                                         drive_to_insert_p->slot,
                                                         current_rg_config_p->drive_handle[position_to_insert]);
            }
            else if (b_use_sata == FBE_FALSE)
            {
                status = fbe_test_pp_util_insert_unique_sas_drive(drive_to_insert_p->bus, 
                                                                  drive_to_insert_p->enclosure, 
                                                                  drive_to_insert_p->slot,
                                                                  current_rg_config_p->block_size,
                                                                  TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
                                                                  &current_rg_config_p->drive_handle[position_to_insert]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
            else
            {
                status = fbe_test_pp_util_insert_sata_drive(drive_to_insert_p->bus, 
                                                            drive_to_insert_p->enclosure, 
                                                            drive_to_insert_p->slot,
                                                            512,
                                                            TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
                                                            &current_rg_config_p->drive_handle[position_to_insert]);
            }
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        } /* end for (drive_index = 0; drive_index < num_removed; drive_index++) */

        current_rg_config_p++;
    } /* end for(rg_index =0; rg_index < raid_group_count; rg_index++) */

    return FBE_STATUS_OK;
}
/******************************************
 * end mumford_the_magician_insert_drives()
 ******************************************/
/*!**************************************************************
 * mumford_the_magician_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a raid I/O test for given configuration to validate the reported
 *  result in event log.
 *
 * @param   rg_config_p  - points to raid group configuration table
 * @param   context_p  - points to event log test configuration on which test is 
 *                                    currently running
 *
 * @return None.   
 * 
 *
 * @author
 *  05/10/2011 - Created. Amit D
 *
 ****************************************************************/
void mumford_the_magician_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t raid_group_count, test_group_count;
    fbe_u32_t rg_index;
    fbe_u32_t test_group_index = 0;
    fbe_test_event_log_test_configuration_t * test_config_p = NULL;
    fbe_test_event_log_test_group_table_t * test_group_table_p = NULL;

    /* Wait for zeroing and initial verify to finish. 
     * Otherwise we will see our tests hit unexpected results when the initial verify gets kicked off. 
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    /* Context has pointer to log test configuration on which test is currently running
     */
    test_config_p = (fbe_test_event_log_test_configuration_t *) context_p;

    /* Get total number of raid group count
     */

    raid_group_count = fbe_test_get_rg_array_length(test_config_p->raid_group_config);

    /* Get total number of test cases
     */
    test_group_table_p = &test_config_p->test_group_table;
    test_group_count = mumford_the_magician_get_test_group_count(test_group_table_p);


    /* Power down drives of all raid groups,if we have to 
     * do so as per configuration.
     */
    if (test_config_p->drives_to_remove.num_removed != 0)
    {
        mumford_the_magician_remove_drives(test_config_p->raid_group_config,
                                           raid_group_count,
                                           &test_config_p->drives_to_remove,
                                           FBE_FALSE,
                                           FBE_TRUE);
        fbe_api_sleep(200);
    }

    /* Run all test cases for a given raid group and loop untill we cover 
     * all raid types. If test case is not applicable to given raid type 
     * then skip it.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            continue;
        }
        /* Run all possible test cases against raid types of the test group
         */
        for (test_group_index = 0; test_group_index < test_group_count; test_group_index++)
        {

            /* Execute the test cases of the test-group
             */
            mut_printf(MUT_LOG_LOW, "--------------------------------------------------");
            mut_printf(MUT_LOG_LOW, 
                       "Running test group[%d] (of total %d) against rg_index = %d",
                       test_group_index, test_group_count, rg_index);
            mut_printf(MUT_LOG_LOW, 
                       "Description of test group[%d]: %s",
                       test_group_index,
                       test_group_table_p->table[test_group_index]->description);
            mumford_the_magician_run_test_group(&rg_config_p[rg_index],
                                                test_group_table_p->table[test_group_index]);
            mut_printf(MUT_LOG_LOW, "--------------------------------------------------");
        }    /* for (test_group_index = 0; test_group_index < test_count; test_group_index++) */
    }    /* for (rg_index = 0; rg_index < raid_group_count; rg_index++) */

    /* Re-insert removed drives
     */
    if (test_config_p->drives_to_remove.num_removed != 0)
    {
        mumford_the_magician_insert_drives(test_config_p->raid_group_config,
                                           raid_group_count,
                                           &test_config_p->drives_to_remove,
                                           FBE_FALSE,
                                           FBE_TRUE,
                                           FBE_FALSE);
        fbe_api_sleep(1000);
    }
}
/**************************************
 * end mumford_the_magician_test_rg_config()
 **************************************/


/*!**************************************************************
 * mumford_the_magician_run_tests()
 ****************************************************************
 * @brief
 * Run all test cases which injects error and validates the reported
 * result in event log. 
 *
 * @param config_p                   - array of configuration against which test needs to be run
 * 
 * @return None
 *
 * @author
 *  10/19/2010 - Created - Pradnya Patankar
 *  05/10/2011    Modified - Amit D
 *
 ****************************************************************/
void mumford_the_magician_run_tests(fbe_test_event_log_test_configuration_t *config_array_p)
{
    fbe_u32_t config_count, config_index;
    fbe_test_event_log_test_configuration_t * test_config_p = NULL;
    const fbe_char_t *raid_type_string_p = NULL;

    /* we need to make sure the test is setup properly */
    MUT_ASSERT_TRUE_MSG(events_array != NULL, "== Error: failed to setup event log memory == \n");

    /* Get the total number of configurations
     */
    config_count = mumford_the_magician_get_config_count(config_array_p);

    /* For each set of configuration run all test cases.
     */
    for (config_index = 0; config_index < config_count; config_index++)
    {
        mut_printf(MUT_LOG_LOW, "running tests for configuration = %d", config_index );
        test_config_p = &config_array_p[config_index];
        fbe_test_sep_util_get_raid_type_string(test_config_p->raid_group_config->raid_type, &raid_type_string_p);

        if (!fbe_sep_test_util_raid_type_enabled(test_config_p->raid_group_config->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled", 
                       raid_type_string_p, test_config_p->raid_group_config->raid_type);
            continue;
        }
        
        if (config_index + 1 >= config_count) {
            /* Now create the raid groups and run the tests
             */
            fbe_test_run_test_on_rg_config(test_config_p->raid_group_config ,test_config_p , 
                                                            mumford_the_magician_test_rg_config,
                                                            MUMFORD_THE_MAGICIAN_TEST_LUNS_PER_RAID_GROUP,
                                                            MUMFORD_THE_MAGICIAN_TEST_CHUNKS_PER_LUN);
        }
        else{
            fbe_test_run_test_on_rg_config_with_time_save(test_config_p->raid_group_config ,test_config_p , 
                                                            mumford_the_magician_test_rg_config,
                                                            MUMFORD_THE_MAGICIAN_TEST_LUNS_PER_RAID_GROUP,
                                                            MUMFORD_THE_MAGICIAN_TEST_CHUNKS_PER_LUN,FBE_FALSE);
        }
    } /* for (config_index; config_index < config_count; config_count++) */
}
/********************************
 * end mumford_the_magician_run_tests()
 *******************************/


/*!**************************************************************
 * mumford_the_magician_test()
 ****************************************************************
 * @brief
 *  Run a event logging for correctable errors test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  8/25/2010 - Created - Pradnya Patankar
 *
 ****************************************************************/
void mumford_the_magician_test(void)
{
    /* We will use same configuration for all test level. But it can change 
     * in future.
     */
    mumford_the_magician_run_tests(mumford_the_magician_raid_group_config_qual);
    return;
}
/***********************************
 * end mumford_the_magician_test()
 **********************************/


/*!**************************************************************
 * mumford_the_magician_setup()
 ****************************************************************
 * @brief
 *  Setup for a event log test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/19/2010 - Created - Pradnya Patankar
 *
 ****************************************************************/
void mumford_the_magician_setup(void)
{
    mumford_the_magician_common_setup();

    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        /* We will use same configuration for all test level. But it can change 
         * in future.
         */
        mumford_simulation_setup(mumford_the_magician_raid_group_config_qual,
                                 mumford_the_magician_raid_group_config_qual);
    }

    return;
}
/**************************************
 * end mumford_the_magician_setup()
 **************************************/


/*!**************************************************************
 * mumford_simulation_setup()
 ****************************************************************
 * @brief
 *  Setup the simulation configuration.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  3/24/2011 - Created. Rob Foley
 *
 ****************************************************************/
void mumford_simulation_setup(fbe_test_event_log_test_configuration_t *config_p_qual,
                              fbe_test_event_log_test_configuration_t *config_p_extended)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_event_log_test_configuration_t *config_p = NULL;
        fbe_u32_t test_index;
        fbe_u32_t config_count;
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        if (test_level == 0)
        {
            /* Qual.
             */
            config_p = config_p_qual;
        }
        else
        {
            /* Extended. 
             */
            config_p = config_p_extended;
        }
        config_count = mumford_the_magician_get_config_count(config_p);
        for (test_index = 0; test_index < config_count; test_index++)
        {
            rg_config_p = &config_p[test_index].raid_group_config[0];
            fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
            fbe_test_rg_setup_random_block_sizes(rg_config_p);
            fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        }

        /* We will use same configuration for all test level. But it can change 
         * in future.
         */
        mumford_the_magician_load_physical_config(config_p);
        elmo_load_sep_and_neit();
    }
    return;
}
/**************************************
 * end mumford_simulation_setup()
 **************************************/


/*!**************************************************************
 * mumford_the_magician_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup for the mumford_the_magician_test().
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/19/2010 - Created. Pradnya Patankar
 *
 ****************************************************************/
void mumford_the_magician_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    mumford_the_magician_common_cleanup();

    return;

}
/******************************************
 * end mumford_the_magician_cleanup()
 ******************************************/

/*!**************************************************************
 * mumford_the_magician_common_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup memory for eventlogs.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *
 ****************************************************************/
void mumford_the_magician_common_cleanup(void)
{
    if (events_array!=NULL) 
    {
        free (events_array);
    }

    return;
}

/*!**************************************************************
 * mumford_the_magician_common_setup()
 ****************************************************************
 * @brief
 *  Setup to collect all eventlogs.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
void mumford_the_magician_common_setup(void)
{
    /* allocate memory for eventlog messages, if we haven't done so */
    if (events_array==NULL) 
    {
        events_array = (fbe_event_log_get_event_log_info_t *)malloc(sizeof(fbe_event_log_get_event_log_info_t) * FBE_EVENT_LOG_MESSAGE_COUNT);
    }
    return;
}
/*****************************************
 * end file mumford_the_magician_test.c
 *****************************************/
