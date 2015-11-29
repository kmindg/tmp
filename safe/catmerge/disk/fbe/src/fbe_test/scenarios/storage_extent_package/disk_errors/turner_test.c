/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file turner_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test cases to validate event log messages 
 *  related to corrupt CRC and corrupt Data operation.
 *
 * @version
 *   02/15/2011:  Created - Jyoti Ranjan
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
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_raid_error.h"
#include "fbe_test_common_utils.h"





 
/*************************
 *   TEST DESCRIPTION
 *************************/

char * turner_test_short_desc = "Validate event log messages because of sectors having corrupt CRC and corrupt data.";
char * turner_test_long_desc = "\n\
The Turner scenario validates the correctness of reported event log messages on detection of sectors \n"
"having corrupt CRC and corrupt Data. \n"
"This test injects errors on a given raid type and issues specific rdgen I/O \n"
"to provoke occurence of error, which also causes RAID library to report event log messages. \n"
"\n\
    -raid_test_level 0 and 1\n\
        - Additional combinations of raid group capacities and number of drives pulled are tested with -rtl 1.\n\
\n\
STEP 1: Configure all raid groups and LUNs.\n\
        - Tests cover 520 block sizes and 4k block sizes.\n\
        - Tests cover r10, r1, r0 raid types.\n\
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
The following RAID types have been commented out for both qual and extended testing:\n\
			- R3, R5, R6 \n\
\n\
Desription last updated: 10/13/2011.\n";

/************************
 * LITERAL DECLARARION
 ***********************/

/*!*******************************************************************
 * @def TURNER_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define TURNER_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def TURNER_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define TURNER_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def TURNER_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with.
 *
 *********************************************************************/
#define TURNER_TEST_RAID_GROUP_COUNT 15

/*!*******************************************************************
 * @def TURNER_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR
 *********************************************************************
 * @brief Size of a region when mining for a hard error.
 *
 *********************************************************************/
#define TURNER_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR 0x40




/*!*******************************************************************
 * @def TURNER_TEST_INVALID_FRU_POSITION
 *********************************************************************
 * @brief Invalid disk position. Used in referring a disk position
 *        wherever we do not expect a valid fru position.
 *
 *********************************************************************/
#define TURNER_TEST_INVALID_FRU_POSITION ((fbe_u32_t) -1)


/*!*******************************************************************
 * @def TURNER_TEST_INVALID_BITMASK
 *********************************************************************
 * @brief Invalid bitmask. 
 *
 *********************************************************************/
#define TURNER_TEST_INVALID_BITMASK (0x0)




/**************************
 * FUNCTION DECLARATION(s)
 *************************/
void turner_test(void);
void turner_test_setup(void);
void turner_cleanup(void);
fbe_status_t turner_validate_rdgen_result(fbe_status_t rdgen_request_status,
                                          fbe_rdgen_operation_status_t rdgen_op_status,
                                          fbe_rdgen_statistics_t *rdgen_stats_p);




/*!*******************************************************************
 * @var b_packages_loaded
 *********************************************************************
 * @brief set to TRUE if the packages have been loaded.
 *
 *********************************************************************/
static fbe_bool_t CSX_MAYBE_UNUSED b_packages_loaded = FBE_FALSE;


/*!*******************************************************************
 * @var turner_validate_user_io_result_fn_p
 *********************************************************************
 * @brief The function is used to validate I/O statistics after 
 *        user I/O oepration
 *
 *********************************************************************/
fbe_test_event_log_test_validate_rdgen_result_t turner_validate_user_io_result_fn_p = NULL;



/*******************
 * GLOBAL VARIABLES
 ******************/


/*!*******************************************************************
 * @var turner_config_array_p
 *********************************************************************
 * @brief Pointer to configuration used to run test cases. It is 
 *        initialized at the time of setup.
 *
 *********************************************************************/
static fbe_test_event_log_test_configuration_t * CSX_MAYBE_UNUSED turner_config_array_p = NULL;





/*!*******************************************************************
 * @var turner_corrupt_crc_test_for_raid_5_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of corrupt CRC and corrupt 
 *        data operation.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t CSX_MAYBE_UNUSED turner_corrupt_crc_operation_test_for_raid_5_non_degraded = 
{
    "Test cases for corrupt CRC and corrupt data operation for non-degraded RAID 3 and RAID 5 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Corrupt CRC operations for many blocks of one disk
         */
        {   
            "Corrupt CRC operations for many blocks of one disk",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Corrupt CRC operations for many blocks of two disks
         */
        {   
            "Corrupt CRC operations for many blocks of two disks",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] Brief: Corrupt CRC operations for many blocks of all disks except one
         */
        {   
            "Corrupt CRC operations for many blocks of of all disks except one",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] Brief: Corrupt CRC operations for many blocks of all disks
         */
        {   
            "Corrupt CRC operations for many blocks of many disk",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] Brief: Corrupt DATA operations for many blocks but at only one disk
         */
        {   
            "Corrupt DATA operations for many blocks but at only one disk",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x80,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },
            
        
        /* [T005] Brief: Corrupt DATA operations spanning two data disks
         */
        {   
            "Corrupt DATA operations spanning two data disks",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x90 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },                            
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x10, /* expected lba */
                                0x70  /* expected blocks */
                            },                            
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

            
        /* [T006] Brief: Corrupt DATA operations spanning all data disks except one
         */
        {   
            "Corrupt DATA operations spanning all data disks except one",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },                            
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            }, 
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T007] Brief: Corrupt DATA operations spanning all data disks
         */
        {   
            "Corrupt DATA operations spanning all data disks",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },                            
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },                            
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },                            
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
 * @var turner_corrupt_crc_pattern_and_misc_error_for_raid_5_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of invalidated sector because 
 *        of previous corrupt operation seen while performing user I/O
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t CSX_MAYBE_UNUSED turner_corrupt_crc_pattern_and_misc_error_for_raid_5_non_degraded = 
{
    "Test cases for validating existence of corrupted CRC sector alongwith other errors for non-degraded RAID 3/5 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Detected invalidated sector because of corrupt CRC operation during pre-read of write
         */
        {   
            "Detected invalidated sector because of corrupt CRC operation during pre-read of write",
            0x5, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Detected invalidated sector because of corrupt CRC operation during read
         */
        {   
            "Detected invalidated sector because of corrupt CRC operation during read",
            0x5, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x0, 
                        { 
                            /* During read operation, we do not expect any message to 
                             * be reported event if we get strips having invalidated sectors 
                             * -ONLY- because of past corrupt operation 
                             */
                            { TURNER_TEST_INVALID_FRU_POSITION }, 
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },
            
        /* [T002] Brief: Detected corrupt CRC invalidated sector and check sum error during pre-read of write
         */
        {   
            "Detected corrupt CRC invalidated sector and check sum error during pre-read of write",
            0x5, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 
                            { 0x08, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            {
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


            
        /* [T003] Brief: Detected corrupt CRC invalidated sector and LBA stamp error during pre-read of write
         */
        {   
            "Detected corrupt CRC invalidated sector and LBA stamp error during pre-read of write",
            0x5, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 
                            { 0x08, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            {
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

            
        /* [T004] Brief: Detected corrupt CRC invalidated sector and LBA stamp error on read
         */
        {   
            "Detected corrupt CRC invalidated sector, CRC and LBA stamp error on read",
            0x5, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        { 
                            { 0x08, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x04, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            {
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            {
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
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
 * @var turner_corrupt_data_pattern_and_misc_error_for_raid_5_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of invalidated sector because 
 *        of previous corrupt data operation seen while performing 
 *        user I/O
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t CSX_MAYBE_UNUSED turner_corrupt_data_pattern_and_misc_error_for_raid_5_non_degraded = 
{
    "Test cases for validating existence of corrupted DATA sector alongwith other errors for non-degraded RAID 3/5 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Detected invalidated sector because of corrupt DATA operation during pre-read of write
         */
        {   
            "Detected invalidated sector because of corrupt DATA operation during pre-read of write",
            0x5, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            }, /* expected blocks */
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Detected invalidated sector because of corrupt DATA operation during read
         */
        {   
            "Detected invalidated sector because of corrupt DATA operation during read",
            0x5, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1, 
                        { 
                            { 
                                0x4, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },
        
        /* [T002] Brief: Detected multiple-invalidated sector in same strip because of corrupt DATA operation during pre-read of write
         */
        {   
            "Detected multiple-invalidated sector in a strip due to corrupt DATA operation during pre-read of write",
            0x5, /* array width */
            0x3, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            }, /* expected blocks */
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x80,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            }, /* expected blocks */
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Third test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T003] Brief: Detected multiple-invalidated sector in same strip because of corrupt DATA operation during read
         */
        {   
            "Detected multiple-invalidated sector in a strip due to corrupt DATA operation during read",
            0x5, /* array width */
            0x3, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            }, /* expected blocks */
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x80,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            }, /* expected blocks */
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Third test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] Brief: Detected corrupt DATA invalidated sector and check sum error during pre-read of write
         */
        {   
            "Detected corrupt DATA invalidated sector and check sum error during pre-read of write",
            0x5, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 
                            { 0x08, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            {
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

            
        /* [T005] Brief: Detected corrupt DATA invalidated sector, strip having LBA stamp and checksum error on read
         */
        {   
            "Detected corrupt DATA invalidated sector, strip having LBA stamp and checksum error on read",
            0x5, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        { 
                            { 0x04, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x02, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x9,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            {
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            {
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
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
 * @var turner_corrupt_crc_operation_test_for_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of corrupt CRC and corrupt 
 *        data operation.for non-degraded RAID 6
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t CSX_MAYBE_UNUSED turner_corrupt_crc_operation_test_for_raid_6_non_degraded = 
{
    "Test cases for corrupt CRC and corrupt data operation for non-degraded RAID 6 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Corrupt CRC operations for many blocks of one disk
         */
        {   
            "Corrupt CRC operations for many blocks of one disk",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Corrupt CRC operations for many blocks of two disks
         */
        {   
            "Corrupt CRC operations for many blocks of two disks",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T002] Brief: Corrupt CRC operations for many blocks of all disks
         */
        {   
            "Corrupt CRC operations for many blocks of all disks",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x300 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] Brief: Corrupt DATA operations for many blocks but at only one disk
         */
        {   
            "Corrupt DATA operations for many blocks but at only one disk",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },
            
        
        /* [T004] Brief: Corrupt DATA operations spanning two data disks
         */
        {   
            "Corrupt DATA operations spanning two data disks",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x90 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },                            
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x10, /* expected lba */
                                0x70  /* expected blocks */
                            },                            
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] Brief: Corrupt DATA operations spanning all data disks
         */
        {   
            "Corrupt DATA operations spanning all data disks",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x300 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,
                        {
                             { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },                            
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },                            
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },                            
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },                            
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
 * @var turner_corrupt_crc_pattern_and_misc_error_for_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of invalidated sector in 
 *        past because of previous corrupt operation seen while 
 *        performing user I/O
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t CSX_MAYBE_UNUSED turner_corrupt_crc_pattern_and_misc_error_for_raid_6_non_degraded = 
{
    "Test cases for validating existence of corrupted CRC sector alongwith other errors for non-degraded RAID 6 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Detected invalidated sector because of corrupt CRC operation during pre-read of write
         */
        {   
            "Detected invalidated sector because of corrupt CRC operation during pre-read of write",
            0x8, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x6 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x6  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x6  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x6 /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x6  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Detected invalidated sector because of previous corrupt CRC operation on full strip write
         */
        {   
            "Detected invalidated sector because of previous corrupt CRC operation on full strip write",
            0x8, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x7,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0xc,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x80 /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] Brief: Detected invalidated sector because of corrupt CRC operation during read
         */
        {   
            "Detected invalidated sector because of corrupt CRC operation during read",
            0x8, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x0, 
                        { 
                            /* During read operation, we do not expect any message to 
                             * be reported event if we get strips having invalidated sectors 
                             * -ONLY- because of past corrupt operation 
                             */
                            { TURNER_TEST_INVALID_FRU_POSITION }, 
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },
 
        /* [T003] Brief: Detected previously invalidated sectors because of corrupt CRC operation and CRC error on read
         */
        {   
            "Detected previously invalidated sectors because of corrupt CRC operation and CRC error on read",
            0x8, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x80,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 
                             { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 10 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T004] Brief: Detected CRC error, LBA stamp error and  corrupt CRC sector because of past ivnalidation on same strip during read
         */
        {   
            "Detected CRC error, LBA stamp error and  corrupt CRC sector because of past ivnalidation on same strip during read",
            0x8, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x100,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        { 
                             { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 10 },
                             { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 10 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
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
 * @var turner_corrupt_data_pattern_and_misc_error_for_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of invalidated sector in 
 *        past because of previous corrupt DATA operation seen while 
 *        performing user I/O
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t CSX_MAYBE_UNUSED turner_corrupt_data_pattern_and_misc_error_for_raid_6_non_degraded = 
{
    "Test cases for validating existence of corrupted DATA sector alongwith other errors for non-degraded RAID 6 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Detected invalidated sector because of corrupt Data operation during pre-read of small write
         */
        {   
            "Detected invalidated sector because of corrupt DATA operation during pre-read of small write",
            0x8, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x6 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x6  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

            
        /* [T001] Brief: Detected invalidated sector because of previous corrupt DATA operation on full strip write
         */
        {   
            "Detected invalidated sector because of previous corrupt DATA operation on full strip write",
            0x8, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x300 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0xe,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x80 /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

            
        /* [T002] Brief: Detected previously invalidated sectors because of corrupt DATA operation and CRC error on read
         */
        {   
            "Detected previously invalidated sectors because of corrupt DATA operation and CRC error on read",
            0x8, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x80,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 
                             { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 10 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] Brief: Detected multiple invalidated corrupt data sector (in past) and CRC error in same strip on read
         */
        {   
            "Detected multiple invalidated corrupt data sector (in past) and CRC error in same strip on read",
            0x8, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x80,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 
                             { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 10 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
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
 * @var turner_corrupt_crc_operation_test_for_raid_1_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of corrupt CRC and corrupt 
 *        data operation.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t turner_corrupt_crc_operation_test_for_raid_1_non_degraded = 
{
    "Test cases for corrupt CRC and corrupt data operation for non-degraded RAID 1 (mirror) groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Corrupt CRC operations for many blocks 
         */
        {   
            "Corrupt CRC operations for many blocks",
            0x2, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x1,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Corrupt DATA operations for many blocks
         */
        {   
            "Corrupt Data operations for many blocks",
            0x2, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x13 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x13  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

            
        /* [T002] Brief: Detected invalidated sector because of previous corrupt CRC operation on Read
         */
        {   
            "Detected invalidated sector because of previous corrupt CRC operation on Read",
            0x2, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x20 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        /* During read operation, we do not expect any message to 
                         * be reported event if we get strips having invalidated sectors 
                         * -ONLY- because of past corrupt operation 
                         */
                        0x0,
                        { TURNER_TEST_INVALID_FRU_POSITION }, 
                    },  /* expected result for above injected error */ 
                }
            }
        },


            
        /* [T003] Brief: Detected invalidated sector because of previous corrupt DATA operation on Read
         */
        {   
            "Detected invalidated sector because of previous corrupt DATA operation on Read",
            0x2, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x1,  0x1E },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x1, /* expected lba */
                                0x1E  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x1,
                                0x1E,
                            },
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
 * @var turner_misc_error_and_corrupted_sectors_for_raid_1_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of detection of corrupt 
 *        sectors (invalidated in past because of corrupt CRC and 
 *        corrupt data operation) alongwith other errors.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t turner_misc_error_and_corrupted_sectors_for_raid_1_non_degraded = 
{
    "Test cases for corrupt CRC and corrupt data operation for non-degraded RAID 1 (mirror) groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T001] Brief: Detected invalidated sector because of previous corrupt CRC operation and CRC error on Read
         */
        {   
            "Detected invalidated sector because of previous corrupt CRC operation and CRC error on Read",
            0x2, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x20 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x40,  0x20 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 
                            { 0x01, 0x40, 0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x40, /* expected lba */
                                0x20  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T001] Brief: Detected invalidated sector because of previous corrupt CRC operation and LBA stamp error on Read
         */
        {   
            "Detected invalidated sector because of previous corrupt CRC operation and LBA stamp error on Read",
            0x2, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x1,  0x20 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x20  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x40,  0x20 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        { 
                            { 0x01, 0x40, 0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x40, /* expected lba */
                                0x20  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T002] Brief: Detected CRC error, LBA stamp error and invalidated sector because of previous corrupt operation on Read
         */
        {   
            "Detected invalidated sector because of previous corrupt DATA operation and CRC error on Read",
            0x2, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x17 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x17  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        { 
                            { 0x01, 0x25, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x01, 0x55, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x17  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x25, /* expected lba */
                                0x5 /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x55, /* expected lba */
                                0x10  /* expected blocks */
                            },
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
 * @var turner_corrupt_crc_operation_test_for_raid_10_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of corrupt CRC and corrupt 
 *        data operation.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t turner_corrupt_crc_operation_test_for_raid_10_non_degraded = 
{
    "Test cases for corrupt CRC and corrupt data operation for non-degraded stripped mirror (RAID 10) groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Corrupt CRC operations for many blocks 
         */
        {   
            "Corrupt CRC operations for many blocks",
            0x6, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x99 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x19  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Corrupt DATA operations for many blocks
         */
        {   
            "Corrupt Data operations for many blocks",
            0x6, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x1,  0x120 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x1, /* expected lba */
                                0x7F  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x21  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },
            
        
            
        /* [T002] Brief: Detected invalidated sector because of previous corrupt DATA operation on Read
         */
        {   
            "Detected invalidated sector because of previous corrupt DATA operation on Read",
            0x6, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x9,  0x117 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x9, /* expected lba */
                                0x77  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x9,
                                0x77,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0,
                                0x80,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0,
                                0x20,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] Brief: Detected invalidated sector because of previous corrupt CRC operation on Read
         */
        {   
            "Detected invalidated sector because of previous corrupt CRC operation on Read",
            0x6, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0xFF },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x7F  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        /* During read operation, we do not expect any message to 
                         * be reported event if we get strips having invalidated sectors 
                         * -ONLY- because of past corrupt operation 
                         */
                        0x0,
                        { TURNER_TEST_INVALID_FRU_POSITION }, 
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
 * @var turner_corrupt_crc_operation_test_for_raid_0
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of corrupt CRC and corrupt 
 *        data operation.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t turner_corrupt_crc_operation_test_for_raid_0 = 
{
    "Test cases for corrupt CRC and corrupt data operation for non-degraded RAID 0 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Corrupt CRC operations for many blocks 
         */
        {   
            "Corrupt CRC operations for many blocks",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x13 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x13  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Corrupt CRC operations for full stripe
         */
        {   
            "Corrupt CRC operations for full stripe",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x400 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },                            
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T002] Brief: Corrupt DATA operations for many blocks
         */
        {   
            "Corrupt Data operations for many blocks",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x1,  0x13 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x1, /* expected lba */
                                0x13  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] Brief: Corrupt DATA operations for full stripe 
         */
        {   
            "Corrupt DATA operations for full stripe",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x7,  0x389 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },                            
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x79  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

            
        /* [T004] Brief: Detected invalidated sector because of previous corrupt CRC operation on Read
         */
        {   
            "Detected invalidated sector because of previous corrupt CRC operation on Read",
            0x8, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x25 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x25  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        /* During read operation, we do not expect any message to 
                         * be reported event if we get strips having invalidated sectors 
                         * -ONLY- because of past corrupt operation 
                         */
                        0x0,
                        { TURNER_TEST_INVALID_FRU_POSITION }, 
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T005] Brief: Detected invalidated sector because of previous corrupt DATA operation on Read
         */
        {   
            "Detected invalidated sector because of previous corrupt DATA operation on Read",
            0x8, /* array width */
            0x2, /* Number of test record specification */
            {
                /* First test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x25 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x25  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                },
                /* Second test record */
                { 
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
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
 * @var turner_corrupt_crc_operation_test_for_raid_5_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of corrupt CRC and corrupt 
 *        data operation for degraded RAID 3/5.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t CSX_MAYBE_UNUSED turner_corrupt_crc_operation_test_for_raid_5_degraded = 
{
    "Test cases for corrupt CRC and corrupt data operation for -DEGRADED- RAID 3/5 groups only, dead position = { 4 }",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Corrupt CRC operation of many blocks on NON-DEAD drive
         */
        {   
            "Corrupt CRC operation of many blocks on NON-DEAD drive",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x80,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Corrupt CRC operation of many blocks on DEAD drive
         */
        {   
            "Corrupt CRC operation of many blocks on DEAD drive",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] Brief: Corrupt DATA operations of many blocks only on DEAD DRIVE
         */
        {   
            "Corrupt DATA operations of many blocks only on DEAD DRIVE",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        /* During corrupt DATA operation, we do not expect any message  
                         * to be reported event if corrupt DATA operation is only on 
                         * dead drive
                         */
                        0x0,
                        { TURNER_TEST_INVALID_FRU_POSITION }, 
                    },  /* expected result for above injected error */ 
                }
            }
        },
  
        /* [T003] Brief: Corrupt DATA operations of many blocks but on NON-DEAD drive
         */
        {   
            "Corrupt DATA operations of many blocks but on NON-DEAD drive",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x80,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,  
                        {
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

            
        /* [T004] Brief: Corrupt DATA operations spanning dead as well as non-dead disk
         */
        {   
            "Corrupt DATA operations spanning dead as well as non-dead disk",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        /* During corrupt DATA operation, we do not expect any 
                         * message to be reported. if corrupt DATA operation span 
                         * to dead drive.
                         */
                        0x0,
                        { TURNER_TEST_INVALID_FRU_POSITION }, 
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
 * @var turner_corrupt_crc_operation_test_for_raid_6_single_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of corrupt CRC and corrupt 
 *        data operation.for single degraded RAID 6 only
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t CSX_MAYBE_UNUSED turner_corrupt_crc_operation_test_for_raid_6_single_degraded = 
{
    "Test cases for corrupt CRC and corrupt data operation for -SINGLE- degraded RAID 6 groups only, dead position(s) = { 7 }",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Corrupt CRC operations of many blocks on -DEAD- disk
         */
        {   
            "Corrupt CRC operations of many blocks on -DEAD- disk",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Corrupt CRC operations of many blocks on NON-DEAD disks
         */
        {   
            "Corrupt CRC operations for many blocks of two disks",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x80,  0x20 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T002] Brief: Corrupt CRC operations of many blocks spanning DEAD as well as NON-DEAD disk
         */
        {   
            "Corrupt CRC operations of many blocks spanning DEAD as well as NON-DEAD disk",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x300 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] Brief: Corrupt DATA operations of many blocks on DEAD DISK
         */
        {   
            "Corrupt DATA operations of many blocks on DEAD DISK",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        /* During corrupt DATA operation, we do not expect any 
                         * message to be reported. if corrupt DATA operation span 
                         * to dead drive.
                         */
                        0x0,
                        { TURNER_TEST_INVALID_FRU_POSITION }, 
                    },  /* expected result for above injected error */ 
                }
            }
        },
            
        
        /* [T004] Brief: Corrupt DATA operations of many blocks on NON-DEAD disk
         */
        {   
            "Corrupt DATA operations spanning two data disks",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x80,  0x90 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },                            
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x10, /* expected lba */
                                0x70  /* expected blocks */
                            },                            
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] Brief: Corrupt DATA operations spanning DEAD as well as NON-DEAD disk
         */
        {   
            "Corrupt DATA operations spanning DEAD as well as NON-DEAD disk",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x300 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        /* During corrupt DATA operation, we do not expect any 
                         * message to be reported. if corrupt DATA operation span 
                         * to dead drive.
                         */
                        0x0,
                        { TURNER_TEST_INVALID_FRU_POSITION }, 
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
 * @var turner_corrupt_crc_operation_test_for_raid_6_double_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of corrupt CRC and corrupt 
 *        data operation.for DOUBLE degraded RAID 6 only
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t CSX_MAYBE_UNUSED turner_corrupt_crc_operation_test_for_raid_6_double_degraded = 
{
    "Test cases for corrupt CRC and corrupt data operation for -DOUBLE- degraded RAID 6 groups only, dead position(s) = { 0, 7 }",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Corrupt CRC operations of many blocks on -DEAD- disk
         */
        {   
            "Corrupt CRC operations of many blocks on -DEAD- disk",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Corrupt CRC operations of many blocks on NON-DEAD disks
         */
        {   
            "Corrupt CRC operations for many blocks of two disks",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x80,  0x20 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T002] Brief: Corrupt CRC operations of many blocks spanning DEAD as well as NON-DEAD disk
         */
        {   
            "Corrupt CRC operations of many blocks spanning DEAD as well as NON-DEAD disk",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x0,  0x300 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_CORRUPT_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] Brief: Corrupt DATA operations of many blocks on DEAD DISK
         */
        {   
            "Corrupt DATA operations of many blocks on DEAD DISK",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        /* During corrupt DATA operation, we do not expect any 
                         * message to be reported. if corrupt DATA operation span 
                         * to dead drive.
                         */
                        0x0,
                        { TURNER_TEST_INVALID_FRU_POSITION }, 
                    },  /* expected result for above injected error */ 
                }
            }
        },
            
        
        /* [T004] Brief: Corrupt DATA operations of many blocks on NON-DEAD disk
         */
        {   
            "Corrupt DATA operations spanning two data disks",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x80,  0x90 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },                            
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x10, /* expected lba */
                                0x70  /* expected blocks */
                            },                            
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] Brief: Corrupt DATA operations spanning DEAD as well as NON-DEAD disk
         */
        {   
            "Corrupt DATA operations spanning DEAD as well as NON-DEAD disk",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x0,  0x300 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        /* During corrupt DATA operation, we do not expect any 
                         * message to be reported. if corrupt DATA operation span 
                         * to dead drive.
                         */
                        0x0,
                        { TURNER_TEST_INVALID_FRU_POSITION }, 
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
 * @var turner_corrupt_crc_operation_test_mirror_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of corrupt CRC and corrupt 
 *        data operation. for DEGRADED mirror raid group.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t turner_corrupt_crc_operation_test_mirror_degraded = 
{
    "Test cases for corrupt CRC and corrupt data operation for non-degraded RAID 1 (mirror) groups only, dead position = { 1 }",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: Corrupt CRC operations for many blocks 
         */
        {   
            "Corrupt CRC operations for many blocks",
            0x2, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x1,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Corrupt DATA operations for many blocks
         */
        {   
            "Corrupt Data operations for many blocks",
            0x2, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x9,  0x17 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x1,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x9, /* expected lba */
                                0x17  /* expected blocks */
                            },
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
 * @var turner_corrupt_crc_operation_test_for_raid_10_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported as a result of corrupt CRC and corrupt 
 *        data operation for degraded RAID 10 group only
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t turner_corrupt_crc_operation_test_for_raid_10_degraded = 
{
    "Test cases for corrupt CRC and corrupt data operation for DEGRADED RAID 10 groups only, dead position = { 1, 3 }",
    mumford_the_magician_run_single_test_case,
    { 
        /* [T000] Brief: Corrupt CRC operations for many blocks 
         */
        {   
            "Corrupt CRC operations for many blocks",
            0x6, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, 0x7,  0x89 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x7, /* expected lba */
                                0x79  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_CRC), /* parity got invalidated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: Corrupt DATA operations for many blocks
         */
        {   
            "Corrupt Data operations for many blocks",
            0x6, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    turner_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, 0x9,  0x120 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x0,
                        { 
                            { TURNER_TEST_INVALID_BITMASK }  /* No error injected */
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x9, /* expected lba */
                                0x77  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x80  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_CORRUPT_DATA), /* expected error info */
                                0x0, /* expected lba */
                                0x29  /* expected blocks */
                            },
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
 * @var turner_raid_group_config_extended
 *********************************************************************
 * @brief This is the raid group configuration we will use to 
 *        valdiate the correctness of event log messages.for 
 *        extended version
 *********************************************************************/
static fbe_test_event_log_test_configuration_t turner_raid_group_config_extended_qual[] = 
{ 
#if 0 
    /* RAID 3 and RAID 5 configuration (non-degraded) */
    {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,       12,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,       20,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &turner_corrupt_crc_operation_test_for_raid_5_non_degraded,
            &turner_corrupt_crc_pattern_and_misc_error_for_raid_5_non_degraded,
            &turner_corrupt_data_pattern_and_misc_error_for_raid_5_non_degraded,
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
            &turner_corrupt_crc_operation_test_for_raid_6_non_degraded,
            &turner_corrupt_crc_pattern_and_misc_error_for_raid_6_non_degraded,
            &turner_corrupt_data_pattern_and_misc_error_for_raid_6_non_degraded,
            NULL,
        }
   },
#endif 

   /* RAID 1 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {2,       0x8000,   FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,           8,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &turner_corrupt_crc_operation_test_for_raid_1_non_degraded,
            &turner_misc_error_and_corrupted_sectors_for_raid_1_non_degraded,
            NULL,
        }
   },

   /* RAID 10 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,            block size  RAID-id.  bandwidth.*/
            {6,       0xE000,   FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,        10,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {   
            &turner_corrupt_crc_operation_test_for_raid_10_non_degraded,
            NULL,       
        }
   },

   /* RAID 0 configuration */
   {
        {
            /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {8,         0x32000,  FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,        0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &turner_corrupt_crc_operation_test_for_raid_0,
            NULL,
        }
   },
#if 0 
    /* RAID 3 and RAID 5 configuration (degraded) */
    {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,       2,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,       4,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 1, { 4, FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &turner_corrupt_crc_operation_test_for_raid_5_degraded,
            NULL,
        }
    },

   /* RAID 6 configuration - single degraded */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {8,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,    FBE_CLASS_ID_PARITY,    520,      6,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  1, {7, FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &turner_corrupt_crc_operation_test_for_raid_6_single_degraded,
            NULL,
        }
   },

   /* RAID 6 configuration - double degraded */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {8,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,    FBE_CLASS_ID_PARITY,    520,      6,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  2, {0, 7, FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &turner_corrupt_crc_operation_test_for_raid_6_double_degraded,
            NULL,
        }
   },
#endif 

   /* RAID 1 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {2,       0xE000,   FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,           8,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  1, { 1, FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &turner_corrupt_crc_operation_test_mirror_degraded,
            NULL,
        }
   },

   /* RAID 10 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,            block size  RAID-id.  bandwidth.*/
            {6,       0xE000,   FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,        10,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  2, {1, 3, FBE_U32_MAX },  0 }, /* Dead drives information */
        {   
            &turner_corrupt_crc_operation_test_for_raid_10_degraded,
            NULL,       
        }
   },

   { 
        {   
            { FBE_U32_MAX, /* Terminator. */},
        }
   }
};


/*!*******************************************************************
 * @var turner_raid_group_config_qual
 *********************************************************************
 * @brief This is the raid group configuration we will use to 
 *        valdiate the correctness of event log messages.for 
 *        qual version
 *********************************************************************/
static fbe_test_event_log_test_configuration_t turner_raid_group_config_qual[] = 
{
#if 0 
    /* RAID 3 and RAID 5 configuration (non-degraded) */
    {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,       12,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,       20,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &turner_corrupt_crc_operation_test_for_raid_5_non_degraded,
            &turner_corrupt_crc_pattern_and_misc_error_for_raid_5_non_degraded,
            &turner_corrupt_data_pattern_and_misc_error_for_raid_5_non_degraded,
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
            &turner_corrupt_crc_operation_test_for_raid_6_non_degraded,
            &turner_corrupt_crc_pattern_and_misc_error_for_raid_6_non_degraded,
            &turner_corrupt_data_pattern_and_misc_error_for_raid_6_non_degraded,
            NULL,
        }
   },
#endif 

   /* RAID 1 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {2,       0x8000,   FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,           8,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &turner_corrupt_crc_operation_test_for_raid_1_non_degraded,
            &turner_misc_error_and_corrupted_sectors_for_raid_1_non_degraded,
            NULL,
        }
   },

   /* RAID 10 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,            block size  RAID-id.  bandwidth.*/
            {6,       0xE000,   FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,        10,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {   
            &turner_corrupt_crc_operation_test_for_raid_10_non_degraded,
            NULL,       
        }
   },

   /* RAID 0 configuration */
   {
        {
            /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {8,         0x32000,  FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,        0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &turner_corrupt_crc_operation_test_for_raid_0,
            NULL,
        }
   },

   { 
        {   
            { FBE_U32_MAX, /* Terminator. */},
        }
   }
};




/*************************
 * FUNCTION DEFINITION(s)
 *************************/


/*!**************************************************************
 * turner_validate_rdgen_result()
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
 *  12/15/2010 - Created - Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t turner_validate_rdgen_result(fbe_status_t rdgen_request_status,
                                              fbe_rdgen_operation_status_t rdgen_op_status,
                                              fbe_rdgen_statistics_t *rdgen_stats_p)
{
    MUT_ASSERT_INT_EQUAL(rdgen_stats_p->aborted_error_count, 0);
    return FBE_STATUS_OK;
}
/**********************************************
 * end turner_validate_io_statistics()
 **********************************************/





/*!**************************************************************
 * turner_test()
 ****************************************************************
 * @brief
 *  Run a event logging for correctable errors test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  12/15/2010 - Created - Jyoti Ranjan
 *
 ****************************************************************/
void turner_test(void)
{
    fbe_test_event_log_test_configuration_t * turner_config_array_p = NULL;

    /* We will use same configuration for all test level. But we can change 
     * in future
     */
    if (fbe_sep_test_util_get_raid_testing_extended_level() > 0)
    {
        /* Use extended configuration.
         */
        turner_config_array_p = turner_raid_group_config_extended_qual;
    }
    else
    {
        /* Use minimal configuration.
         */
        turner_config_array_p = turner_raid_group_config_qual;
    }

    fbe_api_logical_error_injection_enable_ignore_corrupt_crc_data_errors();

    mumford_the_magician_run_tests(turner_config_array_p);

    fbe_api_logical_error_injection_disable_ignore_corrupt_crc_data_errors(); 

    return;
}
/*************************
 * end turner_test()
 ************************/


/*!**************************************************************
 * turner_setup()
 ****************************************************************
 * @brief
 *  Setup for a event log test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  12/15/2010 - Created - Jyoti Ranjan
 *
 ****************************************************************/
void turner_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    mumford_the_magician_common_setup();

    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        mumford_simulation_setup(turner_raid_group_config_qual,
                                 turner_raid_group_config_extended_qual);
    }

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************
 * end turner_setup()
 *************************/

/*!**************************************************************
 * turner_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup for the turner_cleanup().
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  12/15/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
void turner_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    fbe_test_sep_util_restore_sector_trace_level();

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    mumford_the_magician_common_cleanup();

    return;
}
/***************************
 * end turner_cleanup()
 ***************************/




/*****************************************
 * end file mumford_the_magician_test.c
 *****************************************/
