/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file guy_smiley_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test cases to validate event log messages reported 
 *  on detection of uncorrectable errors.
 *
 * @version
 *   10/12/2010:  Created - Jyoti Ranjan
 *   09/28/2012:  Added cases - parity error causes dead drive error - Dave Agans
 *
 * [TODO:]
 *
 * This section mention points because of which reporting of event log message
 * by RAID library is relatively less accurate. To make better reporting of 
 * event log messages, RAID library needs to be enhanced. List of improvement
 * area can be:
 *
 *      1. Reporting of media error
 *         [Description:] Currently, we do not have ability to look  inside
 *                        strip region (which is 64 blocks) while reporintg 
 *         media error. In realty,all 64 blocks may not have media errors. 
 *         Ideally, it will be great if we can report media errors at a 
 *         resolution of single block. We can also think of seperate error
 *         code to report media error.
 *
 *      2. Usage of xor error region in case of RAID 0
 *         [Description:] We do not create XOR error region in case of 
 *                        RAID 0 operation. As a result, we do use eboard to 
 *         report errors. Eboard does not empower us to report exact pba and
 *         number of errored blocks.
 *
 *      3. Reporting of messages for errors which does not appear after 
 *         retry (say retriable checksum error)
 *         [Description:] At this point of time, we do keep track of retried 
 *                        CRC/stamp errors in siots's vrts. We  XOR error 
 *         region. Also, we do report retriable do not create CRC errors only
 *         if same position is not having persistent CRC/stamp errors. For 
 *         example: if a disk position has partial CRC errors which does not 
 *         re-occur after retry and partial CRC errors which is persistent then 
 *         we do not report the former one. Also, we do not have ability to 
 *         report number of blocks as well as start pba correctly as xor region 
 *         does not exists for retriable errors.
 *             
 *      4. Fixed/small size of XOR error region
 *         [Description:] We do have maximum error region size as 16. So can 
 *                        report whatever maximum we can accomodate in 16 places
 *         Also, we do report errors wither using eboard or XOR error region at 
 *         any point of time.
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
char * guy_smiley_test_short_desc = "validates event log messages reported on detection of uncorrectable errors";
char * guy_smiley_test_long_desc = 
"guy_smiley_test injects error on a given raid type and issues specific rdgen I/O \n"
"to provoke occurence of error, which also causes RAID library to report event log messages. \n"
"This scenario validates correctness of reported even-log messages by RAID library. \n"
"-raid_test_level 0 and 1\n"
"     We test additional error cases and with single degraded and double degraded raid groups at -rtl 1.\n"
"Starting Config:  \n"
"        [PP] armada board \n"
"        [PP] SAS PMC port \n"
"        [PP] viper enclosure \n"
"        [PP] 3 SAS drives (PDO-[0..2]) \n"
"        [PP] 3 logical drives (LD-[0..2]) \n"
"        [SEP] 3 provision drives (PVD-[0..2]) \n"
"        [SEP] 3 virtual drives (VD-[0..2]) \n"
"        [SEP] raid object (RAID5 2+1) \n"
"        [SEP] LUN object (LUN-0) \n"
"\n"
"STEP 1: Create raid group configuration\n"
"STEP 2: Inject error using logical error injection service\n"
"STEP 3: Issue rdgen I/O\n"
"STEP 4: Validate rdgen I/O statistics as per predefined values\n"
"	[TODO] Improve validation of rdgen I/O\n"
"STEP 5: Validate the expected event log message correctness against a pre-defined expected parameters\n"
"Description last updated: 10/13/2011.\n";


/************************
 * LITERAL DECLARARION
 ***********************/

/*!*******************************************************************
 * @def GUY_SMILEY_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define GUY_SMILEY_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def GUY_SMILEY_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define GUY_SMILEY_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def GUY_SMILEY_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with.
 *
 *********************************************************************/
#define GUY_SMILEY_TEST_RAID_GROUP_COUNT 15

/*!*******************************************************************
 * @def GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR
 *********************************************************************
 * @brief Size of a region when mining for a hard error.
 *
 *********************************************************************/
#define GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR 0x40


/*!*******************************************************************
 * @var guy_smiley_validate_io_statistics_fn
 *********************************************************************
 * @brief The function is used to validate I/O statistics after 
 *        issuing I/O for given injected raid error.
 *
 *********************************************************************/
fbe_test_event_log_test_validate_rdgen_result_t guy_smiley_validate_rdgen_result_fn_p = NULL;



/**************************
 * FUNCTION DECLARATION(s)
 *************************/
void guy_smiley_test(void);
void guy_smiley_log_setup(void);
void guy_smiley_cleanup(void);

/***********************
 * FORWARD DECLARATION
 ***********************/
fbe_status_t guy_smiley_validate_rdgen_result(fbe_status_t rdgen_request_status,
                                              fbe_rdgen_operation_status_t rdgen_op_status,
                                              fbe_rdgen_statistics_t *rdgen_stats_p);

/*******************
 * GLOBAL VARIABLES
 ******************/


/*!*******************************************************************
 * @var guy_smiley_basic_crc_test_for_raid_5_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable CRC errors for 
 *        non-degraded RAID 3 and RAID 5
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_basic_crc_test_for_raid_5_non_degraded = 
{
    "Uncorrectable CRC test cases for non-degraded RAID 3 and RAID 5 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC errors at multiple -data position- of same strip, block(s) = 0x1, width = 0x5
         */
        {  
            "CRC errors at multiple -data positions- of -ONLY- one strip on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: CRC errors at -data and parity- position of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC errors at -data and parity- position of same strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            /*! @todo The algorithm is R5_468_VR which actually doesn't result in the
                             *        loss of either parity or data (since we have the write data to
                             *        correct with).  Seems like we should check if the data is actually
                             *        lost before reporting these errors.
                             */
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T002] Brief: CRC errors at -ALL data and parity- position of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC errors at -ALL data and parity- position(S) of same strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x4,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x84,  0x17C },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x5,
                        {
                            { 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0},
                            { 0x04, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x09,  /* expected number of event log mesages */
                        {
                            /*! @todo The algorithm is R5_468_VR which actually doesn't result in the
                             *        loss of either parity or data (since we have the write data to
                             *        correct with).  Seems like we should check if the data is actually
                             *        lost before reporting these errors.
                             */
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T003] Brief: CRC errors at multiple positions in mutliple disjoint set of strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC errors at multiple positions in mutliple disjoint set of strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x4,
                        {
                            { 0x10, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x10, 0x20, 0x13, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x20, 0x13, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20, /* expected lba */
                                0x13  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20, /* expected lba */
                                0x13  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20, /* expected lba */
                                0x13  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] Brief: Invalidated errors at multiple data positions of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "Invalidated errors at multiple data positions of same strip(s) on Write/Zero",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALIDATED, 0 },
                            { 0x08, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALIDATED, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        /*! @note Any time we `inject' invalidation errors, the parity will not
                         *        agree with the data position.  This results in a coherency error
                         *        logged against the parity position.
                         */
                        0x1,  /* expected number of event log messages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR, /* expected error code */
                                (VR_COH), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },

                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T005] Brief: RAID Crc error at one data position (and parity) while general CRC error at other data positions of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "RAID CRC error at one data position while general CRC error at other data positions of same strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x3,  0x77 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x3,  0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x3,  0x77 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x83,  0x177 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x01, 0x7,  0x9, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RAID_CRC, 0 },
                            { 0x08, 0x7,  0x9, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RAID_CRC, 0 },
                            { 0x10, 0x7,  0x9, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        /* Due to the way we inject `invalids' RAID detects a timestamp error for
                         * position 4 but blames parity.  This results in an uncorrectable error
                         * for position 0 (reconstructed) an uncorrectable error for position 3.
                         */
                        0x1,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                VR_RAID_CRC, /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },

                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T006] Brief: Single-bit CRC errors at multiple data positions of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "Single-bit CRC errors at multiple data positions of same strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x7E },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x7E },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x7E },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x17E },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x1,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC, 0 },
                            { 0x08, 0x1,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_SINGLE_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_SINGLE_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_SINGLE_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_SINGLE_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T007] Brief: Multi-bit CRC errors at multiple data positions of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "Multi-bit CRC errors at multiple data positions of same strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x79 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x79 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x79 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x70,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC, 0 },
                            { 0x08, 0x70,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x70, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x70, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x70, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x70, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x70, /* expected lba */
                                0x9  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },



        /* [T008] Brief: Multi-types of CRC errors at multiple data positions of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "Multi-types of CRC errors errors at multiple data positions of same strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x10, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC, 0  },
                            { 0x04, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC, 0  },
                        }
                    }, /* injected logical error detail */
                    {
                        0x7,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_SINGLE_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_SINGLE_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
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
 * @var guy_smiley_media_error_test_for_raid_5_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event 
 *        log messages reported against un-correctable media
 *        errors for non-degraded RAID 3 and RAID 5. The below 
 *        test cases also validates log because of occuerence 
 *        of media error along with other errors.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_media_error_test_for_raid_5_non_degraded = 
{
    "Uncorrectable media error test cases for non-degraded RAID 3 and RAID 5 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: Multiple hard-media errors (error blocks less than I/O blocks) causing uncorrectable strip(s),
         *        block(s) = -MANY-, width = 0x5
         */
        {   
            "Multiple hard-media errors (error blocks less than I/O blocks) causing uncorrectable strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0  },
                            { 0x08, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0  },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T001] 
         * Brief: Multiple hard-media errors (error blocks greater than I/O blocks) causing uncorrectable strip(s), 
                  block(s) = -MANY-, width = 0x5
         */
        {   
            "Multiple hard-media errors (error blocks greater than I/O blocks) causing uncorrectable strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x1,  0x10 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x1,  0x10 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x10 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x0,  0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x08, 0x0,  0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        
        /* [T002] Brief: CRC and media error causing uncorrectable strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC and media error causing uncorrectable strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x11 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x11 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x11 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x0,  0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x0,  0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x11  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x11  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x11  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x11  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x11  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: Single hard-media errors on 2 data disks causing uncorrectable strip(s), 
                  block(s) = -SINGLE-, width = 0x5
         */
        {   
            "Single hard-media errors on two data disks causing uncorrectable strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x3,  0x1 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x3,  0x1 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x3,  0x1 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x3,  0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x08, 0x3,  0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x3, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x3, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x3, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x3, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x3, /* expected lba */
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
 * @var guy_smiley_basic_stamp_test_for_raid_5_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event 
 *        log messages reported against un-correctable stamp 
 *        errors for non-degraded RAID 3 and RAID 5. The below 
 *        test cases inject -ONLY ONE TYPE- of stamp errors for
 *        a given test case. 
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_basic_stamp_test_for_raid_5_non_degraded = 
{
    "Uncorrectable stamp error test cases for non-degraded RAID 3 and RAID 5 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: LBA-stamp error(s) at two data positions of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "LBA-stamp error(s) at two data positions of same strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,            
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x08, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0,
                                0x10,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] 
         * Brief: LBA-stamp error(s) at two data positions of same strip, block(s) = -MANY-, width = 0x5
         * Note:  Injecting shed stamp error at data position is treated as lba-stamp error
         */
        {   
            "LBA-stamp error(s) at two data positions of same strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,            
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x10, 0x70, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x08, 0x70, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x70,
                                0x10,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x70,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x70,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x70,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x70,
                                0x10,
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
 * @var guy_smiley_misc_stamp_test_for_raid_5_non_degraded
 *********************************************************************
 * @brief The below mentioned test cases validate correctness of 
 *        event log message reported as a result of injection of 
 *        different combination of stamp errors only. All test cases 
 *        are meant for non-degraded RAID 3/5 only. 
 *         
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_misc_stamp_test_for_raid_5_non_degraded = 
{
    "Uncorrectable combination of stamp error test cases for non-degraded RAID 3 and RAID 5 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: Time-stamp and LBA-stamp error at two data positions of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "Time stamp and LBA-stamp error(s) at two data positions of same strip(s) on Write/Zero/Read",
            0x5, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,            
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 }, 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x100,0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x04, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
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
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                        }
                    },  /* expected result for above injected error */
                }
            }
        },

        /* [T001] 
         * Brief: time-stamp error at parity and lba-stamp error at data position, block = -MANY-, width = 0x5
         */
        {   
            "Time-stamp error at parity and lba-stamp error at data position of same strip(s) on Write/Zero/Read",
            0x5, /* array width */            
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x10, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x1,
                                0x10,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1,
                                0x10,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1,
                                0x10,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] 
         * Brief: Write-stamp and LBA-stamp error at two data positions of same strip, block(s) = -MANY-, width = 0x5
         */
        {   
            "Write stamp and LBA-stamp errors at two data positions of same strip(s) on Write/Zero/Read",
            0x5, /* array width */   
            0x1, /* Number of test record specification */         
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x100,0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x4, 0x7, 0x9, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                            { 0x2, 0x7, 0x9, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x7,
                                0x9,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x7,
                                0x9,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x7,
                                0x9,
                            },

                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: Write-stamp error at parity and lba-stamp error at data position, block = -MANY-, width = 0x5
         */
        {   
            "Write-stamp error at parity and lba-stamp error at data position of same strip(s) on Write/Zero/Read",
            0x5, /* array width */ 
            0x1, /* Number of test record specification */           
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x1,  0x7F },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x1,  0x7F },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x7F },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                            { 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x1,
                                0xF,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1,
                                0xF,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1,
                                0xF,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: Multiple stamp error(s) at different strip(s) but at data positions only, block(s) = -MANY-, width = 0x5
         */
        {   
            "Multiple stamp error(s) at different strip(s) but at data positions only on Write/Zero/Read",
            0x5, /* array width */ 
            0x1, /* Number of test record specification */           
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x4,
                        {
                            { 0x10, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x08, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x08, 0x60, 0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x04, 0x60, 0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x9,  /* expected number of event log mesages */
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
                                0x60,
                                0x20,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x60,
                                0x20,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x60,
                                0x20,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x60,
                                0x20,
                            },

                        },
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
 * @var guy_smiley_advanced_crc_and_stamp_test_for_raid_5_non_degraded
 *********************************************************************
 * @brief The below mentioned test cases validate correctness of 
 *        event log message reported as a result of injection of 
 *        different combination of CRC and stamp errors. All test 
 *        cases are meant for non-degraded RAID 3/5 only. 
 *         
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_advanced_crc_and_stamp_test_for_raid_5_non_degraded = 
{
    "Uncorrectable combination of CRC and stamp error test cases for non-degraded RAID 3 and RAID 5 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: CRC and LBA-stamp error at multiple but -ONLY-at data position of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC and LBA-stamp error(s) at multiple but -ONLY- at data position of same strip(s) on Write/Zero/Read",
            0x5, /* array width */  
            0x1, /* Number of test record specification */                      
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x100,0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x4, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0},
                            { 0x2, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 }
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0,
                                0x10,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] 
         * Brief: CRC and LBA stamp errors at parity and multiple data position of same strip(s), block = -MANY-, width = 0x5
         */
        {   
            "CRC and LBA stamp error(s) at parity and multiple data position of same strip(s) on Write/Zero/Read",
            0x5, /* array width */
            0x1, /* Number of test record specification */           
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x4,  0x7C },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x4,  0x7C },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x4,  0x7C },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0xC  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x4,
                                0xC,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x4,
                                0xC,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4,
                                0xC,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4,
                                0xC,
                            },

                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] 
         * Brief: CRC error at data and time-stamp error at parity positions of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC error at data and time-stamp error at parity position of stamp strip(s) on Write/Zero/Read",
            0x5, /* array width */ 
            0x1, /* Number of test record specification */           
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x4, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x10, 0x4, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x4,
                                0x10,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4,
                                0x10,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4,
                                0x10,
                            },

                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: CRC and time-stamp errors at data position only of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC and time-stamp error(s) at data position only of same strip(s) only on Write/Zero/Read",
            0x5, /* array width */
            0x1, /* Number of test record specification */            
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x9,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x9,  0x100 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x109,0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x4, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x4, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x2, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0,
                                0x10,
                            },
                        },
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: CRC, time-stamp and LBA errors at parity and data position of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC, time-stamp and LBA-stamp error(s) at parity and data position of same strip(s) only on Write/Zero/Read",
            0x5, /* array width */ 
            0x1, /* Number of test record specification */           
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                        },
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T005] 
         * Brief: CRC, time-stamp and LBA-stamp errors at parity and data position of multiple disjoint strip(s), block = -MANY-, width = 0x5
         */
        {   
            "CRC, time-stamp and LBA-stamp error(s) at parity and data position of multiple disjoint strip(s) on Write/Zero/Read",
            0x5, /* array width */
            0x1, /* Number of test record specification */            
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x6,
                        {
                            { 0x01, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x10, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },

                            { 0x08, 0x20, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x04, 0x20, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x20, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0xc,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20,
                                0x05,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20,
                                0x05,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20,
                                0x05,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20,
                                0x05,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x20,
                                0x05,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x20,
                                0x05,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x20,
                                0x05,
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
 * @var guy_smiley_basic_crc_test_for_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable CRC errors for 
 *        non-degraded RAID 6 (dual parity raid group)
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_basic_crc_test_for_raid_6_non_degraded = 
{
    "Uncorrectable CRC test cases for non-degraded RAID 6 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC errors at multiple -data position- of same strip, block(s) = 0x1, width = 0x8
         */
        {   
            "CRC errors at multiple -data positions- of -ONLY- one strip on Write/Zero/Read",
            0x8, /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x20, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T001] 
         * Brief: CRC errors at multiple -data- and -rp- position of same strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC errors at multiple -data- and -rp- position of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x7,  0x79 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x7,  0x79 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x7,  0x79 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0, 0x10,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x0, 0x10,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x0, 0x10,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T002] 
         * Brief: CRC errors at multiple -data- and -diagonal parity- position of same strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC errors at multiple -data- and -dp- position of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0, 0x12,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x0, 0x12,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0, 0x12, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x12  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x12  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x12  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x12  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x12  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x12  /* expected blocks */
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

                
        /* [T003] 
         * Brief: CRC errors at -data, rp and dp- position of same strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC errors at multiple -data, rp and dp- position of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x4,  0x7C },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x4,  0x7C },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x4,  0x7C },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x1, 0xF, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x1, 0xF, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x1, 0xF, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0xC  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0xC  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0xC  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0xC  /* expected blocks */
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: CRC errors at -all data and parity- position of same strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC errors at -ALL data and parity- position(S) of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x8,
                        {
                            { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x20, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x10, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x04, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0xe,  /* expected number of event log mesages */
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
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] 
         * Brief: CRC errors causing multiple disjoint set of uncorrectable strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC errors causing multiple disjoint set of uncorrectable strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x6,
                        {
                            { 0x80, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x80, 0x25, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x20, 0x25, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x25, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0xc,  /* expected number of event log mesages */
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
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x25, /* expected lba */
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
 * @var guy_smiley_media_error_test_for_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported because of 
 *        1. un-correctable media errors
 *        2. uncorrectable errors because of combination of media and 
 *           other errors     
 *
 * Test cases are meant only for non-degraded RAID 6 only.
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_media_error_test_for_raid_6_non_degraded = 
{
    "Uncorrectable media error test cases for non-degraded RAID 6 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: Media errors at multiple -data position- of same strip (error blocks < I/O blocks), 
         *        block(s) = 0x1, width = 0x8
         */
        {   
            "Media errors at multiple -data positions- of -ONLY- one strip on Write/Zero/Read, error blocks < I/O blocks",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x40, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x20, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T001] 
         * Brief: Media error -rp, dp and data- position of same strip(s) (error blocks > I/O blocks), 
         *        block(s) = -MANY-, width = 0x8
         */
        {   
            "Media error -rp, dp and data- position of same strip(s) on Write/Zero/Read, (error blocks > I/O blocks)",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0, 0x10,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x01, 0x0, 0x10,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x02, 0x0, 0x10,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x9, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR - 9  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x9, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR - 9  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x9, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR - 9  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x9, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR - 9 /* expected blocks */
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] 
         * Brief: Media error at -rp and dp-, CRC error at data- position of same strip(s) (error blocks > I/O blocks), 
         *        block(s) = -MANY-, width = 0x8
         */
        {   
            "Media error at -rp and dp-, CRC error at data- position of same strip(s) on Write/Zero/Read, (error blocks > I/O blocks)",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x1,  0x7E },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x1,  0x7E },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x7E },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0, 0x10,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x0, 0x10,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x02, 0x0, 0x10,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0xF  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0xF  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR-1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR-1  /* expected blocks */
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
 * @var guy_smiley_basic_stamp_test_for_raid_6_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable stamp errors (viz.
 *        time-stamp, lba-stamp and write-stamp) for non-degraded
 *        RAID 6 (dual parity raid group)
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_basic_stamp_test_for_raid_6_non_degraded = 
{
    "Uncorrectable stamp test cases for non-degraded RAID 6 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: LBA-stamp errors at multiple -data positions- of same strip(s), block(s) = 0x10, width = 0x8
         */
        {   
            "LBA-stamp errors at multiple -data positions- of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x77 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x77 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x70, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x40, 0x70, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x20, 0x70, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: LBA-stamp errors causing multiple uncorrectable strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "LBA-stamp error(s) causing multiple disjoint set of uncorrectable strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x6,
                        {
                            { 0x80, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x40, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x20, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x10, 0x25, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x08, 0x25, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x04, 0x25, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x10,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x25, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x25, /* expected lba */
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
 * @var guy_smiley_misc_stamp_test_for_raid_6_non_degraded
 *********************************************************************
 * @brief The below mentioned test cases validate correctness of 
 *        event log message reported as a result of injection of 
 *        different combination of different stamp error(s) only. 
 *        All test cases are meant for non-degraded RAID 6 only. 
 *         
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_misc_stamp_test_for_raid_6_non_degraded = 
{
    "Uncorrectable combination of stamp error test cases for non-degraded RAID 6 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: Time stamp error at one and LBA-stamp error at two data positions of same strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "Time stamp error at one and LBA-stamp error at  two data position respectively of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */     
            0x1, /* Number of test record specification */                   
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 }, 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x100,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x40, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x20, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x7,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x1,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1,
                                0x10,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] 
         * Brief: Time-stamp error at -rp-, LBA-stamp errors at -two data position-, block(s)= -MANY-, width = 0x8
         */
        {   
            "Time-stamp error at -rp- and LBA-stamp at two -data position- of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */    
            0x1, /* Number of test record specification */                    
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x7, 0x19, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x40, 0x7, 0x19, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x01, 0x7, 0x19, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x7,
                                0x19,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x7,
                                0x19,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x7,
                                0x19,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x7,
                                0x19,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x7,
                                0x19,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x7,
                                0x19,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] 
         * Brief: Time stamp error at -rp and data pos-, LBA stamp error at two data pos(s) of same strip, block(s) = 0x10, width = 0x8
         */
        {   
            "Time stamp error at -rp and data pos- and LBA stamp error at two data pos(s) of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */            
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x2,  0xFE },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x2,  0xFE },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x2,  0xFE },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x102,  0x1FE },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x4,
                        {
                            { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0  },
                            { 0x20, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0  },
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0xa,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
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
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: Write-stamp error at one data position and lba-stamp at two data positions of same strip(s), block = 0x10, width = 0x8
         */
        {   
            "Write-stamp error at one data position and lba-stamp at two data positions of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */ 
            0x1, /* Number of test record specification */                       
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                            { 0x40, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x20, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x9,
                                0x8,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x9,
                                0x8,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9,
                                0x8,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9,
                                0x8,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9,
                                0x8,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9,
                                0x8,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x9,
                                0x8,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x9,
                                0x8,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: Time-stamp error at -rp and dp- and LBA-stamp error at two data positions of same strip(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "Time-stamp error at -rp and dp- and LBA-stamp error at data positions of same strip(s) only on Write/Zero/Read",
            0x8,  /* array width */  
            0x1, /* Number of test record specification */          
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x100,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x4,
                        {
                            { 0x80, 0x0,  0x21, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0  },
                            { 0x40, 0x0,  0x21, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0  },
                            { 0x02, 0x0,  0x21, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x01, 0x0,  0x21, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0xa,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x21,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x21,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x21,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x21,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x21,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x21,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x21,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x21,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0,
                                0x21,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0,
                                0x21,
                            },
                        },
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
 * @var guy_smiley_misc_crc_and_stamp_test_for_raid_6_non_degraded
 *********************************************************************
 * @brief The below mentioned test cases validate correctness of 
 *        event log message reported as a result of injection of 
 *        different combination of CRC and stamp errors only. All 
 *        test cases are meant for non-degraded RAID 6 only. 
 *         
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_misc_crc_and_stamp_test_for_raid_6_non_degraded = 
{
    "Uncorrectable combination of CRC as well as stamp error test cases for non-degraded RAID 6 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: CRC and LBA-stamp error at one and two data position respectively of same strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC and LBA-stamp error at one and two data position respectively of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */  
            0x1, /* Number of test record specification */                      
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 }, 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x100,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0},
                            { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x20, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0,
                                0x10,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] 
         * Brief: CRC errors at both parity and LBA stamp error at data position of same strip(s), block(s)= -MANY-, width = 0x8
         */
        {   
            "CRC errors at both parity and LBA stamp error at data position of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,            
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x1,  0x7F },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x1,  0x7F },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x7F },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0, 0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x02, 0x0, 0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x0, 0x20, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1,
                                0x1F,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1,
                                0x1F,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1,
                                0x1F,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1,
                                0x1F,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] 
         * Brief: CRC, LBA stamp and time stamp error at data positions of same strip,
         *        block(s) = 0x10, width = 0x8
         */
        {   
            "CRC, LBA stamp and time stamp error at data positions of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */            
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x100,  0x1FF },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x4, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x40, 0x4, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0  },
                            { 0x20, 0x4, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x7,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x4,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x4,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x4,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x4,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x4,
                                0x10,
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: CRC error at diagonal parity, time stamp at row parity and LBA-stamp at data position 
         *        of same strip(s), block = 0x10, width = 0x8
         */
        {   
            "CRC error at dp, time stamp at rp and LBA-stamp at data position of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */ 
            0x1, /* Number of test record specification */           
            {
                {
                    guy_smiley_validate_rdgen_result,           
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x81,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0,
                                0x10,
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
 * @var guy_smiley_crc_and_stamp_test_for_raid_0
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable CRC as well as 
 *        stamp errors for RAID 0 only
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_crc_and_stamp_test_for_raid_0 = 
{
    "Uncorrectable CRC and stamp test cases for RAID 0 groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC error at only one position, block(s) = 0x1, width = 0x8
         */
        {   
            "CRC error at only one data position of one strip on Read",
            0x8,  /* array width */
            0x1,  /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] 
         * Brief: CRC errors at more than one data positions of same strip(s), 
         *        block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC errors at more than one data positions of same strip(s) on Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x3FF },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x6,
                        {
                            { 0x80, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x80, 0x20, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x40, 0x20, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x20, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x20, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T002] 
         * Brief: LBA-stamp errors at only one data position of strip(s),
         *        block(s) = 0x10, width = 0x8
         */
        {   
            "LBA-stamp errors at only one data position of strip(s) on Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x9,  0x577 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x1  /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: CRC and LBA stamps at multiple positions of same strip(s), 
         *        block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC and LBA stamps at alternate data positions of same strip(s) on Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x400 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x8,
                        {
                            { 0x01, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x04, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x10, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x20, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x40, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x80, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x10,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: Combination of CRC and LBA stamp errors at disjoint set of strip(s), 
         *        block(s) = -MANY-, width = 0x8
         */
        {   
            "Combination of CRC and LBA stamp errors at disjoint set of strip(s) on Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x479 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x6,
                        {
                            { 0x02, 0x0,   0xF, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x0,   0xF, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x08, 0x21,  0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x04, 0x21,  0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x21,  0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x01, 0x21,  0x60, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC | VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC | VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC | VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC | VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC ), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC ), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        
        /* [T005] 
         * Brief: CRC and LBA stamp error at same data position of a strip(s), 
         *        block(s) = 0x10, width = 0x8
         */
        {   
            "CRC and LBA stamp error at same data position of a strip(s) on Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x577 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x2,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                        },
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
 * @var guy_smiley_basic_crc_and_stamp_test_for_mirror_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable CRC as well as 
 *        stamp errors for RAID 1 (mirror) non-degraded only
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_basic_crc_and_stamp_test_for_mirror_non_degraded = 
{
    "Uncorrectable CRC and stamp test cases for RAID 1 (or mirror) groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC error at both disks of pair of same strip(s), block(s) = 0x1, width = 0x2
         */
        {   
            "CRC error at both disks of pair of same strip(s) on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] 
         * Brief: LBA-stamp error at both disks of pair of same strip(s), 
         *        block(s) = -MANY-, width = 0x2
         */
        {   
            "LBA-stamp error at both disks of pair of same strip(s) on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x400 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {   
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0xF  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0xF  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0xF  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0xF  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T002] 
         * Brief: CRC error at primary and LBA-stamp error at secondary of same strip(s), 
         *        block(s) = 0x10, width = 0x2
         */
        {   
            "CRC error at primary and LBA-stamp error at secondary of same strip(s) on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x9,  0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x9,  0x577 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: CRC and LBA stamp errors independently causing uncorrectable disjoint strips(s) , 
         *        block(s) = -MANY-, width = 0x2
         */
        {   
            "CRC and LBA stamp errors independently causing uncorrectable disjoint strips(s) on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x7,  0xF9 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x4,
                        {
                            { 0x01, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x80, 0x21, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x02, 0x80, 0x21, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x80, /* expected lba */
                                0x21  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x80, /* expected lba */
                                0x21  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x80, /* expected lba */
                                0x21  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x80, /* expected lba */
                                0x21  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] Brief: Multi and Single-bit CRC error at primary and secondary disk respectively of same strip(s), block(s) = 0x1, width = 0x2
         */
        {   
            "Multi and Single-bit CRC error at primary and secondary disk respectively of same strip(s) on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x7,  0x70 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x70, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC, 0 },
                            { 0x02, 0x70, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_SINGLE_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_SINGLE_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x70, /* expected lba */
                                0x7  /* expected blocks */
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
 * @var guy_smiley_media_error_for_mirror_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against:
 *           1. un-correctable media errrors alone
 *           2. combination of media errors with other errors (CRC 
 *              as well as stamp errors)
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_media_error_for_mirror_non_degraded = 
{
    "Uncorrectable media error test cases for RAID 1 (or mirror) groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: Media error at both pair of same strip(s) (number of error blocks < I/O blocks), 
         *        block(s) = -MANY-, width = 0x2
         */
        {   
            "Media error at both pair of same strip(s) on Read, number of error blocks < I/O blocks",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T001] 
         * Brief: Media error at both pair of same strip(s) (number of error blocks > I/O blocks), 
         *        block(s) = -MANY-, width = 0x2
         */
        {   
            "Media error at both pair of same strip(s) on Read, number of error blocks < I/O blocks",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x20 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x80, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x02, 0x0, 0x80, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x20  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] 
         * Brief: CRC error at primary and media error at secondary of same strip(s), 
         *        block(s) = -MANY-, width = 0x2
         */
        {   
            "CRC error at primary and media error at secondary of same strip(s) on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x10, /* expected lba */
                                0x30  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        
        /* [T003] 
         * Brief: MEDIA and CRC errors at FRIST FEW and LAST FEW blocks respecitvely in both disks(s), 
         *        block(s) = -MANY-, width = 0x2
         * Note: this test case is a negative test case and is intentionally written. It doee validate that
         *       we do report errors detected in first strip-region. Its because we do fail read 
         *       operation as soon as we detect that number of errors is greater than width of array.
         *       So, we will never report the errors in second strip region (i.e. above 0x64 blocks)
         */
        {   
            "MEDIA and CRC errors at FRIST FEW and LAST FEW blocks respecitvely  in both disks of same strip(s) on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x4,
                        {
                            { 0x01, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x02, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },                    
                            { 0x01, 0x75, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x75, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        
        /* [T004] 
         * Brief: CRC and MEDIA errors at FRIST FEW and LAST FEW blocks respecitvely in both disks(s), 
         *        block(s) = -MANY-, width = 0x2
         */
        {   
            "CRC and MEDIA errors at FRIST FEW and LAST FEW blocks respecitvely in both disks of same strip(s) on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x4,
                        {
                            { 0x01, 0x75, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x02, 0x75, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },                    
                            { 0x01, 0x0,  0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0,  0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x40, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x40, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x40, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x40, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x5  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x5  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x5  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x5  /* expected blocks */
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
 * @var guy_smiley_basic_crc_and_stamp_test_for_r10_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable CRC as well as 
 *        stamp errors for RAID 10 (stripped-mirror) non-degraded
 *        raid group only
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_basic_crc_and_stamp_test_for_r10_non_degraded = 
{
    "Uncorrectable CRC and stamp test cases for RAID 10 (stripped-mirror) non-degraded raid groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC error at both disks of mirror-pair, block(s) = 0x10, width = 0x6
         */
        {   
            "CRC error at both disks of mirror-pair on Read spanning -only one mirror pair-",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: CRC error at both disks of mirror-pair, block(s) = -MANY-, width = 0x6
         */
        {   
            "CRC error at both disks of mirror-pair on Read spanning -two mirror pair-",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x1, 0x17, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x1, 0x17, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x17  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x17  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x17  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x17  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x17  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x17  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x17  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x17  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] Brief: LBA-stamp error at both disks of mirror-pair, block(s) = -MANY-, width = 0x6
         */
        {   
            "LBA-stamp error at both disks of mirror-pair on Read spanning -only one mirror pair-",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x5,  0x7B },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x02, 0x0, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] Brief: LBA-stamp error at both disks of mirror-pair, block(s) = 0x10, width = 0x6
         */
        {   
            "LBA-stamp error at both disks of mirror-pair on Read spanning -three mirror pair-",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x5, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x02, 0x5, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0xc,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x5, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T004] Brief: CRC error at primary and LBA-stamp error at secondary strip(s), block(s) = -MANY-, width = 0x2
         */
        {   
            "CRC error at primary and LBA-stamp error at secondary strip(s) on Read spanning -all mirror pair-",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x9,  0x3F7 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x22, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0, 0x22, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0xc,  /* expected number of event log mesages */
                        {   
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                               
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                               
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x19  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] Brief: CRC errors causing multiple uncorrectable disjoint strip(s), block(s) = -MANY-, width = 0x2
         */
        {   
            "CRC errors causing multiple uncorrectable disjoint strip(s) on Read spanning only -one mirror pair-",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                            guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x6,
                        {
                            { 0x01, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x25, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x25, 0x05, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x60, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x60, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0xc,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x25, /* expected lba */
                                0x05  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x25, /* expected lba */
                                0x05  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x25, /* expected lba */
                                0x05  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x25, /* expected lba */
                                0x05  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x60, /* expected lba */
                                0x15  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x60, /* expected lba */
                                0x15  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x60, /* expected lba */
                                0x15  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x60, /* expected lba */
                                0x15  /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T006] Brief: Both CRC and LBA stamp error in each sector of strips(s) , block(s) = -MANY-, width = 0x2
         */
        {   
            "Both CRC and LBA stamp error in each sector of strips(s) on Read",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x4,
                        {
                            { 0x01, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x02, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
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
 * @var guy_smiley_media_error_test_for_r10_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable
 *              1. media error and
 *              2. combination of media as well as other errors (viz 
 *                 stamp and CRC errors)
 *
 *Test case is  valid only for non-degraded mirror raid groups.
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_media_error_test_for_r10_non_degraded = 
{
    "Uncorrectable media error test cases for RAID 10 (stripped-mirror) non-degraded raid groups only",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] 
         * Brief: Media error at both disks of pair, error blocks > I/O blocks, 
         *        block(s) = -MANY-, width = 0x6
         */
        {   
            "Media error at both disks of pair, error blocks > I/O blocks on Read spanning -only one mirror pair-",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x20 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x81,  0x20 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x101,  0x20 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x80, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x02, 0x0, 0x80, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x20  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T001] 
         * Brief: Media error at both disks of pair with error blocks > I/O blocks and I/O spanning, 
         *        two mirror-pair, block(s) = -MANY-, width = 0x6
         */
        {   
            "Media error at both disks of pair on Read spanning -only two mirror pair-",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0xa0 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x80, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x02, 0x0, 0x80, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                GUY_SMILEY_TEST_ERROR_MINE_REGION_SIZE_ON_MEDIA_ERROR  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x20  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

         /* [T002] 
         * Brief: Media error at both disks of pair, single error block, 
         *        block(s) = -SINGLE-, width = 0x6
         */
        {   
            "Media error at both disks of pair,  single error block Read spanning -only one mirror pair-",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x2 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x1, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                            { 0x02, 0x1, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MEDIA_CRC), /* expected error info */
                                0x1, /* expected lba */
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
 * @var guy_smiley_basic_crc_test_for_raid_5_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable CRC errors for 
 *        degraded RAID 3 and RAID 5 raid groups.
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_basic_crc_test_for_raid_5_degraded = 
{
    "Uncorrectable CRC test cases for -DEGRADED- RAID 3 and RAID 5 groups only, dead position = { 4 }",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC errors at one non-dead data position, block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC errors at one non-dead data position on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x80,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x100,   0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x08, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: CRC errors at -parity- position(s), block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC errors at -parity- position of same strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,   0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },                            
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] 
         * Brief: CRC errors at -parity and data- position of same strip(s), 
         *        block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC errors at -parity and data- position of same strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x77 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x80,  0xf7 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0xF0,   0x110 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x08, 0x0, 0x11, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x0, 0x11, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x11  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x11  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x11  /* expected blocks */
                            },                                                        
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x11  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x11  /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: CRC errors at multiple positions causing mutliple disjoint set of uncorrectable strip(s), 
         *        block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC errors causing disjoint set of uncorrectable strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x80,  0x100 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x4,
                        {
                            { 0x08, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x04, 0x0,  0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x20, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0xc,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x20, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x20, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x20, /* expected lba */
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
 * @var guy_smiley_misc_test_for_raid_5_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported on detection of multiple types of 
 *        stamp error(s) and/or CRC error(s).
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_misc_test_for_raid_5_degraded = 
{
    "Uncorrectable stamp test cases for -DEGRADED- RAID 3 and RAID 5 groups only, dead position(s) = { 4 }",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: LBA stamp error(s) at one non-dead data position, block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC errors at one non-dead data position on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x80,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x100,   0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x08, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T001] 
         * Brief: LBA stamp and CRC error(s) at data and -parity- position(s) respectively, 
         *        block(s) = -MANY-, width = 0x5
         */
        {   
            "LBA stamp and CRC errors at -data- and -parity- respectively of same strip(s) on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x2,   0x7E },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x2,   0x7E },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x2,   0x7E },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x100,   0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x08, 0x4, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x01, 0x4, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x4, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x4, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x4, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x4, /* expected lba */
                                0x10  /* expected blocks */
                            },

                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] 
         * Brief: CRC and LBA stamp errors at same sectors of -data- position, 
         *        block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC and LBA stamp error(s) at same sectors of -data- position on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x9,   0x70 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x9,   0x70 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x9,   0x70, FBE_TRUE },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x08, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        05,  /* expected number of event log mesages */
                        {
                            
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x9, /* expected lba */
                                0x7  /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: CRC and LBA stamp error at alternate sector of same data drive, 
         *        block(s) = -MANY-, width = 0x5
         */
        {   
            "CRC and LBA stamp error at alternate sector of same data drive on Write/Zero/Read",
            0x5,  /* array width */        
            0x1, /* Number of test record specification */    
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x80,  0x100 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x100,   0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x08, 0x0,  0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x1,  0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x2  /* expected blocks - both sectors combined in error region */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x2  /* expected blocks - both sectors combined in error region */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: CRC error at parity and time-stamp error at data-position, block(s) = -MANY-, width = 0x5
         * Note: Injecting time-stamp error does not have any affect in case of degraded R3/R5
         */
        {   
            "CRC error at parity and time-stamp error at data-position on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x180 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x80,  0x100 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x01, 0x0, 0x14, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x04, 0x0, 0x14, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x08, 0x0, 0x14, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x14  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x14,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x14  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* parity got reconstructed because of other data drives */
                                0x0, /* expected lba */
                                0x14  /* expected blocks */
                            },                                                                                    
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x14  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x14  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] 
         * Brief: CRC error at data and write-stamp error at parity position, block(s) = -MANY-, width = 0x5
         * Note: Injecting write-stamp error does not have any affect in case of degraded R3/R5
         */
        {   
            "CRC error at data and write-stamp error at parity position on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x77 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x80,  0x77 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x100, 0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x7, 0x19, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                            { 0x08, 0x7, 0x19, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x7, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x7, /* expected lba */
                                0x19  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x7, /* expected lba */
                                0x19  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T006] 
         * Brief: Write-stamp error at parity position, block(s) = various combinations, width = 0x5
         * Note: Write-stamp error on parity with dead drive means incomplete write, and
         *       thus dead drive rebuild data is lost and invalidated
         */
        {   
            "Write-stamp error at parity position on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x7,   0x70 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x87,  0x70 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x7,   0x70 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x7,   0x70, FBE_TRUE },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x0, 0x17, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* parity got reconstructed because of write stamp error */
                                0x7, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x7, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x7, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T007] 
         * Brief: Time-stamp error at parity position, block(s) = -MANY-, width = 0x5
         * Note: Time-stamp error on parity with dead drive means incomplete write, and
         *       thus dead drive rebuild data is lost and invalidated
         */
        {   
            "Time-stamp error at parity position on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,   0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x70, 0xF, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* parity got reconstructed because of time stamp error elsewhere */
                                0x70, /* expected lba */
                                0xF  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x70, /* expected lba */
                                0xF  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x70, /* expected lba */
                                0xF  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T008] 
         * Brief: Write-stamp error at data position, block(s) = various combinations, width = 0x5
         * Note: Write-stamp error on data with dead drive means incomplete write, and
         *       thus dead drive rebuild data is lost and invalidated
         */
        {   
            "Write-stamp error at data position on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x71 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x71 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x71 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x71, FBE_TRUE },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x08, 0x61, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x3,  /* expected number of event log messages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* parity got reconstructed due to write stamp error elsewhere */
                                0x61, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x61, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x61, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T009] 
         * Brief: Time-stamp error at data position, block(s) = -MANY-, width = 0x5
         * Note: Time-stamp error on data with dead drive means incomplete write, and
         *       thus dead drive rebuild data is lost and invalidated
         */
        {   
            "Time-stamp error at data position on Write/Zero/Read",
            0x5,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x100,   0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x08, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* parity got reconstructed because of time stamp error elsewhere */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
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
 * @var guy_smiley_basic_crc_and_stamp_test_for_mirror_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable CRC as well as 
 *        stamp errors for RAID 1 (mirror)degraded only
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_basic_crc_and_stamp_test_for_mirror_degraded = 
{
    "Uncorrectable CRC and stamp test cases for DEGRADED RAID 1 (or mirror) groups only, dead position = { 1 }",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC error at primary disk with secondary disk dead, block(s) = -MANY-, width = 0x2
         */
        {   
            "CRC error at primary disk with secondary disk dead on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x7E },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x0, 0x11, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: LBA-stamp error at primary disk with secondary disk dead, block(s) = -MANY-, width = 0x2
         */
        {   
            "LBA-stamp error at primary disk with secondary disk dead on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x400 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x2, 0x16, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {   
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x2, /* expected lba */
                                0x16  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x2, /* expected lba */
                                0x16  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x2, /* expected lba */
                                0x16  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x2, /* expected lba */
                                0x16  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T002] 
         * Brief: Both CRC as well as LBA stamp error at same sector of primary disk with secondary 
         *        disk dead, block(s) = 0x10, width = 0x2
         */
        {   
            "CRC and LBA stamp error at same sectorof primary disk with secondary disk dead on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x580 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        },
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: CRC and LBA stamp errors at alternate sector of primary disk with secondary 
         *        disk dead , block(s) = -MANY-, width = 0x2
         */
        {   
            "CRC and LBA stamp errors at alternate sector of primary disk with secondary disk dead on Read",
            0x2,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0,  0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x1,  0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
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
 * @var guy_smiley_basic_crc_and_stamp_test_for_r10_non_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable CRC as well as 
 *        stamp errors for RAID 10 (stripped-mirror) degraded
 *        raid group only
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_basic_crc_and_stamp_test_for_r10_degraded = 
{
    "Uncorrectable CRC and stamp test cases for RAID 10 degraded raid groups only, dead position(s) = {1, 3}",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC error at primary disk(s) on Read spanning only one mirror-pair, block(s) = 0x10, width = 0x6
         */
        {   
            "CRC error at primary disk(s)on Read spanning only first -mirror pair-",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x7,  0x79 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x0, 0x17, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x7, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x7, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] 
         * Brief: CRC error at primary disk(s)on Read spanning only first two mirror-pair, 
         *        block(s) = 0x10, width = 0x6
         */
        {   
            "CRC error at primary disk(s)on Read spanning only first two mirror pair",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x1, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },                    
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T003] 
         * Brief: CRC error at primary disk(s)on Read spanning all mirror-pair, block(s) = 0x10, width = 0x6
         * Note: We will get checksum error for 3rd mirror pair as only primary disk is having error and
         *       secondary disk is alive. Please see dead position(s) of raid group.
         */
        {   
            "CRC error at primary disk(s)on Read spanning all mirror pairs",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x9,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: LBA-stamp error at primary disk(s) of mirror-pair, block(s) = 0x10, width = 0x6
         * Note: please note that I/O has been issued only at second and third pair. So, 
         *       third pair will not encounter uncorrectable error as both disks are alive
         *       and error injected is at only primary disk.
         */
        {   
            "LBA-stamp error at primary disk(s) of mirror-pair on Read spanning -two mirror pair-",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x100 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] 
         * Brief: LBA-stamp error at both disks of mirror-pair, block(s) = 0x10, width = 0x6
         * Note: All mirror pair (including third pair) will encounter uncorrectable errors.
         *       3rd pair will get uncorrectable error even though no drive is dead for this
         *       pair. Its because of error injected at both primary as well as secondary.
         */
        {   
            "LBA-stamp error at both disks of mirror-pair on Read spanning all mirror pair",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0xc,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },                    
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },                    
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },                    
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T006] Brief: CRC and LBA stamp error at alternate block, block(s) = -MANY-, width = 0x6
         */
        {   
            "CRC and LBA stamp error at alternate block on performing Read spanning only one mirror pair",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x1, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T007] 
         * Brief: CRC and LBA stamp error at alternate block, block(s) = -MANY-, width = 0x6
         * Note: Please note that third pair will get only correctable error as both disks 
         *       are alive and only one disk is having error.
         */
        {   
            "CRC and LBA stamp error at alternate block on performing Read spanning all mirror pair",
            0x6,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x180 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x1, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x12,  /* expected number of event log mesages */
                        {   
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },

                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
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
 * @var guy_smiley_basic_crc_test_for_raid_6_single_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable CRC errors for 
 *       single-degraded RAID 6 (dual parity raid group)
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_basic_crc_test_for_raid_6_single_degraded = 
{
    "Uncorrectable CRC test cases for -SINGLE- degraded RAID 6 groups only, dead position(s) = { 7 }",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC errors at two non-dead -data position- of same strip, block(s) = 0x1, width = 0x8
         */
        {   
            "CRC errors at two non-dead -data position- of -ONLY- one strip on Write/Zero/Read for dead position",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x80, 0x10 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x40, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x20, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x0, /* expected lba */
                                0x1  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },


        /* [T001] 
         * Brief: CRC errors at multiple -data- and -rp- position of same strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC errors at multiple -data- and -rp- position of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x80,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x40, 0x4, 0x10,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x4, 0x10,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x4, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x4, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x4, /* expected lba */
                                0x10  /* expected blocks */
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        
        /* [T002] 
         * Brief: CRC errors at multiple -rp and dp- position of same strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC errors at multiple -rp and dp- position of same strip(s) on Write/Zero/Read spanning dead drive only",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x7,   0x79 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x7,   0x79 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x7,   0x79, FBE_TRUE },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x7, /* expected lba */
                                0x9  /* expected blocks */
                            }
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

                
        /* [T003]
         * Brief: CRC errors at -data position, RP and DP- of same strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC errors at -data position, RP and DP- of same strip(s) of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x9,  0x70 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x9,  0x70 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x9,  0x70, FBE_TRUE },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x40, 0x0, 0x19, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x01, 0x0, 0x19, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0, 0x19, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
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
 * @var guy_smiley_basic_misc_crc_and_stamp_test_for_raid_6_single_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable stamp errors 
 *        for -SINGLE DEGRADED- RAID 6 (dual parity raid group)
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_basic_misc_crc_and_stamp_test_for_raid_6_single_degraded = 
{
    "Uncorrectable combination of CRC as well stamp test cases for -SINGLE DEGRADED- RAID 6 groups, dead position(s) = { 7 }",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC and LBA-stamp error at two non-dead data positions of same strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC and LBA-stamp error at two non-dead data positions of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x40, 0x4, 0x4, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x20, 0x4, 0x4, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x4, /* expected lba */
                                0x4  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x4, /* expected lba */
                                0x4  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0x4  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0x4  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x4, /* expected lba */
                                0x4  /* expected blocks */
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x4, /* expected lba */
                                0x4  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x4, /* expected lba */
                                0x4  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* parity got reconstructed because of other data drives */
                                0x4, /* expected lba */
                                0x4  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] 
         * Brief: CRC errors at RP and DP, and LBA-stamp at non-dead data position of same strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC errors at RP and DP, and LBA-stamp at non-dead data position of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x8,  0x71 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x8,  0x71 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x8,  0x71 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x40, 0x0,  0x18, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x01, 0x0,  0x18, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                            { 0x02, 0x0,  0x18, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x8, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x8, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x8, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x8, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x8, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x8, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] 
         * Brief: Write stamp error at RP and DP - incomplete write causes invalidation of dead drive,
         * block(s) = -MANY-, width = 0x8
         * Since RP and DP agree, raid assumes data stamps are wrong
         */
        {   
            "Write stamp error at RP and DP on single-degraded Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x79 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x79 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x79, FBE_TRUE },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x70, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                            { 0x02, 0x70, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x9,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x70, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x70, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x70, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity invalidated expected error info */
                                0x70, /* expected lba */
                                0x9  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x70,
                                0x9,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x70,
                                0x9,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x70,
                                0x9,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x70,
                                0x9,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x70,
                                0x9,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T003] 
         * Brief: Time stamp error at RP and DP - incomplete write causes invalidation of dead drive,
         * block(s) = -MANY-, width = 0x8
         * Since RP and DP agree, raid assumes data stamps are wrong
         */
        {   
            "Time stamp error at RP and DP on single-degraded Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x9,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity invalidated expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: Write stamp error at two data positions - incomplete write causes invalidation of dead drives,
         * block(s) = -MANY-, width = 0x8
         */
        {   
            "Write stamp error at two data positions on single-degraded Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                            { 0x20, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x8,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity invalidated expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] 
         * Brief: Time stamp error at two data positions - incomplete write causes invalidation of dead drives,
         * block(s) = -MANY-, width = 0x8
         */
        {   
            "Time stamp error at two data positions on single-degraded Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x1,  0x7E },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x1,  0x7E },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x1,  0x7E, FBE_TRUE },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x40, 0x0, 0x11, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x20, 0x0, 0x11, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity invalidated expected error info */
                                0x1, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x1,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x1,
                                0x10,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T006] 
         * Brief: Write stamp error at RP and data positions - incomplete write causes invalidation of dead drives,
         * block(s) = -MANY-, width = 0x8
         */
        {   
            "Write stamp error at RP and data positions on single-degraded Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0xc,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* row parity invalidated expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* row parity invalidated expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x2, /* expected fru position */ 
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

        /* [T007] 
         * Brief: Time stamp error at RP and data positions - incomplete write causes invalidation of dead drives,
         * block(s) = -MANY-, width = 0x8
         */
        {   
            "Time stamp error at RP and data positions on single-degraded Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x2,
                        {
                            { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                            { 0x01, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x9,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* row parity invalidated expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x5, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x4, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x3, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
                            },
                            { 
                                0x2, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
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
    },
};




/*!*******************************************************************
 * @var guy_smiley_misc_crc_and_stamp_test_for_raid_6_double_degraded
 *********************************************************************
 * @brief These all test cases will be used to validate event log
 *        messages reported against un-correctable CRC errors for 
 *        double-degraded RAID 6 (dual parity raid group)
 *
 *********************************************************************/
static fbe_test_event_log_test_group_t guy_smiley_misc_crc_and_stamp_test_for_raid_6_double_degraded = 
{
    "Uncorrectable CRC test cases for -DOUBLE- degraded RAID 6 groups only, dead position(s) = { 0, 7 }",
    mumford_the_magician_run_single_test_case,
    {
        /* [T000] Brief: CRC errors at one non-dead -data position- of same strip, block(s) = 0x1, width = 0x8
         */
        {   
            "CRC errors at one non-dead data position of a strip on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x80, 0x10 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* diag parity got reconstructed because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity got invaliddated because of other data drives */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T001] Brief: CRC error(s) at DP, block(s) = -MANY-, width = 0x8
         */
        {   
            "CRC error(s) at DP on Write/Zero",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,   0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x80,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x02, 0x4, 0x14,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x4, /* expected lba */
                                0x14  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x4, /* expected lba */
                                0x14  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x4, /* expected lba */
                                0x14  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity invalidated expected error info */
                                0x4, /* expected lba */
                                0x14  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T002] Brief: LBA stamp error at non-dead data position of strip(s), block(s) = -MANY-, width = 0x8
         */
        {   
            "LBA stamp error at non-dead data position of strip(s) on Write/Zero/Read spanning dead drive only",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x9,   0x77 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x9,   0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x9,   0x77, FBE_TRUE },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x40, 0x0, 0x19,  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity invalidated expected error info */
                                0x9, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */
                }
            }
        },
                
        /* [T003] 
         * Brief: CRC error at DP and LBA-stamp at data position of same strip(s), block(s) = -MANY-, width = 0x8
         * Note: Please note that  below test case declares error injection at dead position, which should 
         *       not have any affect on behavior of test case.
         */
        {   
            "CRC errors at -data position, RP and DP- of same strip(s) of same strip(s) on Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x200 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x3,
                        {
                            { 0x80, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_LBA_STAMP), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR, /* expected error code */
                                (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity invalidated expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T004] 
         * Brief: Write stamp error at DP - incomplete write causes invalidation of dead drives,
         * block(s) = -MANY-, width = 0x8
         */
        {   
            "Write stamp error at DP on double-degraded Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x280 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x02, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity invalidated expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T005] 
         * Brief: Write stamp error at data position - incomplete write causes invalidation of dead drives,
         * block(s) = -MANY-, width = 0x8
         */
        {   
            "Write stamp error at data position on double-degraded Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x79 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x79 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x79, FBE_TRUE },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x40, 0x71, 0xF, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x6,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x71, /* expected lba */
                                0x8  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x71, /* expected lba */
                                0x8  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x71, /* expected lba */
                                0x8  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity invalidated expected error info */
                                0x71, /* expected lba */
                                0x8  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_WS), /* expected error info */
                                0x71,
                                0x8,
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x71,
                                0x8,
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T006] 
         * Brief: Time stamp error at DP - incomplete write causes invalidation of dead drives, block(s) = -MANY-, width = 0x8
         */
        {   
            "Time stamp error at DP on double-degraded Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x7,  0x77 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x7,  0x77 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x7,  0x77, FBE_TRUE },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x02, 0x70, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x4,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x70, /* expected lba */
                                0xE  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x70, /* expected lba */
                                0xE  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x70, /* expected lba */
                                0xE  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity invalidated expected error info */
                                0x70, /* expected lba */
                                0xE  /* expected blocks */
                            },
                        }
                    },  /* expected result for above injected error */ 
                }
            }
        },

        /* [T007] 
         * Brief: Time stamp error at data position - incomplete write causes invalidation of dead drives,
         * block(s) = -MANY-, width = 0x8
         */
        {   
            "Time stamp error at data position on double-degraded Write/Zero/Read",
            0x8,  /* array width */
            0x1, /* Number of test record specification */
            {
                {
                    guy_smiley_validate_rdgen_result,
                    { 
                        { FBE_RDGEN_OPERATION_WRITE_ONLY, 0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_ZERO_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_READ_ONLY,  0x0,  0x80 },
                        { FBE_RDGEN_OPERATION_INVALID }
                    }, /* rdgen specification used to produce the error */
                    {
                        0x1,
                        {
                            { 0x40, 0x0, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP, 0 },
                        }
                    }, /* injected logical error detail */
                    {
                        0x5,  /* expected number of event log mesages */
                        {
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x7, /* expected fru position */ 
                                SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x1, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x0, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_NO_ERROR), /* row parity invalidated expected error info */
                                0x0, /* expected lba */
                                0x10  /* expected blocks */
                            },
                            { 
                                0x6, /* expected fru position */ 
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED, /* expected error code */
                                (VR_TS), /* expected error info */
                                0x0,
                                0x10,
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
 * @var guy_smiley_raid_group_config_extended
 *********************************************************************
 * @brief This is the raid group configuration we will use to 
 *        valdiate the correctness of event log messages.for 
 *        extended version
 *********************************************************************/
static fbe_test_event_log_test_configuration_t guy_smiley_raid_group_config_extended_qual[] = 
{
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
            &guy_smiley_basic_crc_test_for_raid_5_non_degraded,
            &guy_smiley_media_error_test_for_raid_5_non_degraded,
            &guy_smiley_basic_stamp_test_for_raid_5_non_degraded,
            &guy_smiley_misc_stamp_test_for_raid_5_non_degraded,
            &guy_smiley_advanced_crc_and_stamp_test_for_raid_5_non_degraded,
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
            &guy_smiley_basic_crc_test_for_raid_6_non_degraded,
            &guy_smiley_media_error_test_for_raid_6_non_degraded,
            &guy_smiley_basic_stamp_test_for_raid_6_non_degraded,
            &guy_smiley_misc_stamp_test_for_raid_6_non_degraded,
            &guy_smiley_misc_crc_and_stamp_test_for_raid_6_non_degraded,
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
            &guy_smiley_crc_and_stamp_test_for_raid_0,
            NULL,
        }
    },

    /* RAID 1 configuration */
    {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {2,       0x8000,   FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,           8,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &guy_smiley_basic_crc_and_stamp_test_for_mirror_non_degraded,
            &guy_smiley_media_error_for_mirror_non_degraded,
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
            &guy_smiley_basic_crc_and_stamp_test_for_r10_non_degraded,
            &guy_smiley_media_error_test_for_r10_non_degraded,
            NULL,       
        }
    },

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
            &guy_smiley_basic_crc_test_for_raid_5_degraded,
            &guy_smiley_misc_test_for_raid_5_degraded,
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
            &guy_smiley_basic_crc_test_for_raid_6_single_degraded,
            &guy_smiley_basic_misc_crc_and_stamp_test_for_raid_6_single_degraded,
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
            &guy_smiley_misc_crc_and_stamp_test_for_raid_6_double_degraded,
            NULL,
        }
    },

    /* RAID 1 configuration */
    {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {2,       0xE000,   FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,           8,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  1, { 1, FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &guy_smiley_basic_crc_and_stamp_test_for_mirror_degraded,
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
            &guy_smiley_basic_crc_and_stamp_test_for_r10_degraded,
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
 * @var guy_smiley_raid_group_config_qual
 *********************************************************************
 * @brief This is the raid group configuration we will use to 
 *        valdiate the correctness of event log messages.for 
 *        qual version
 *********************************************************************/
static fbe_test_event_log_test_configuration_t guy_smiley_raid_group_config_qual[] = 
{
    /* RAID 3 and RAID 5 configuration (non-degraded) */
    {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,       2,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,       4,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &guy_smiley_basic_crc_test_for_raid_5_non_degraded,
            &guy_smiley_media_error_test_for_raid_5_non_degraded,
            &guy_smiley_basic_stamp_test_for_raid_5_non_degraded,
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
            &guy_smiley_basic_crc_test_for_raid_6_non_degraded,
            &guy_smiley_media_error_test_for_raid_6_non_degraded,
            &guy_smiley_basic_stamp_test_for_raid_6_non_degraded,
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
            &guy_smiley_crc_and_stamp_test_for_raid_0,
            NULL,
        }
   },

   /* RAID 1 configuration */
   {
        {
          /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
            {2,       0x8000,   FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,           8,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {  0, { FBE_U32_MAX },  0 }, /* Dead drives information */
        {
            &guy_smiley_basic_crc_and_stamp_test_for_mirror_non_degraded,
            &guy_smiley_media_error_for_mirror_non_degraded,
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
            &guy_smiley_basic_crc_and_stamp_test_for_r10_non_degraded,
            &guy_smiley_media_error_test_for_r10_non_degraded,
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
 * guy_smiley_validate_rdgen_result()
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
fbe_status_t guy_smiley_validate_rdgen_result(fbe_status_t rdgen_request_status,
                                              fbe_rdgen_operation_status_t rdgen_op_status,
                                              fbe_rdgen_statistics_t *rdgen_stats_p)
{
    MUT_ASSERT_INT_EQUAL(rdgen_stats_p->aborted_error_count, 0);
    return FBE_STATUS_OK;
}
/**********************************************
 * end guy_smiley_validate_io_statistics()
 **********************************************/





/*!**************************************************************
 * guy_smiley_test()
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
void guy_smiley_test(void)
{
    fbe_test_event_log_test_configuration_t * config_p = NULL;

    /* We will use same configuration for all test level. But we can change 
     * in future
     */
    if (fbe_sep_test_util_get_raid_testing_extended_level() > 0)
    {
        /* Use extended configuration.
         */
        config_p = guy_smiley_raid_group_config_extended_qual;
    }
    else
    {
        /* Use minimal configuration.
         */
        config_p = guy_smiley_raid_group_config_qual;
    }

    mumford_the_magician_run_tests(config_p);

    return;
}
/*************************
 * end guy_smiley_test()
 ************************/


/*!**************************************************************
 * guy_smiley_setup()
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
void guy_smiley_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    guy_smiley_validate_rdgen_result_fn_p = guy_smiley_validate_rdgen_result;

    mumford_the_magician_common_setup();

    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        mumford_simulation_setup(guy_smiley_raid_group_config_qual,
                                 guy_smiley_raid_group_config_extended_qual);
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
 * end guy_smiley_setup()
 *************************/


/*!**************************************************************
 * guy_smiley_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup for the guy_smiley_cleanup().
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  12/15/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
void guy_smiley_cleanup(void)
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
 * end guy_smiley_cleanup()
 ***************************/




/*****************************************
 * end file mumford_the_magician_test.c
 *****************************************/
