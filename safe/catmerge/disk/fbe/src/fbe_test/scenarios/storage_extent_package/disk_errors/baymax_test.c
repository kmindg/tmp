/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
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
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_raid_error.h"
#include "fbe_test_common_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe/fbe_api_scheduler_interface.h"

/*************************
 *   TEST DESCRIPTION
 *************************/
char * baymax_short_desc = "validates event log messages are not reported for known errors";
char * baymax_long_desc = 
"baymax_test injects an invalid pattern on a given raid type and issues specific rdgen I/O \n"
"to provoke occurence of a error, which also causes RAID library to detect the known error \n"
"but not to report event log messages. \n"
"This scenario validates correctness of reported event-log messages by RAID library. \n"
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
#define BAYMAX_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def GUY_SMILEY_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define BAYMAX_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def baymax_error_test_case_t
 *********************************************************************
 * @brief Test sequence struct to pass in to tests
 *
 *********************************************************************/
typedef struct baymax_error_test_case_s
{
    char*               description;  /* description of test*/
    fbe_data_pattern_t  invalid_pattern; /* pattern to use for io */
    fbe_lba_t           error_lba; /* lba to write invalid pattern to */
    fbe_block_count_t   error_blocks; /* number of block to write invalid pattern to */
    fbe_lba_t           read_lba; /* lba to read from */
    fbe_block_count_t   read_blocks; /* number of blocks to read for */
    fbe_bool_t          b_is_error_verify_run; /* check for error verify and events*/
    fbe_test_event_log_test_injected_error_info_t lei_records; /* lei records */
    fbe_test_event_log_test_result_t event_log_result; /* expected or unexpected messages */
} baymax_error_test_case_t;

/***********************
 * FORWARD DECLARATION
 ***********************/
void baymax_invalids_test(fbe_test_rg_configuration_t * raid_group_p, baymax_error_test_case_t * test_case_p);
void baymax_run_single_test_case(fbe_test_rg_configuration_t * rg_config_p,
                                 baymax_error_test_case_t * test_case_p);
void baymax_clean_error_injection(fbe_test_rg_configuration_t * rg_config_p);
void baymax_set_up_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                   baymax_error_test_case_t * test_case_p);

void baymax_free_resources(fbe_u8_t **memory_ptr);
void baymax_io_to_lun(fbe_object_id_t lun_object_id,
                                 fbe_payload_block_operation_opcode_t block_opcode,
                                 fbe_lba_t lba,
                                 fbe_block_count_t block_count,
                                 fbe_u32_t block_size, 
                                 fbe_data_pattern_flags_t data_pattern_flags,
                                 fbe_bool_t b_check_checksum);
void baymax_pretest_cleanup(fbe_test_rg_configuration_t *rg_config_p);
void baymax_set_error_verify_hook(fbe_test_rg_configuration_t *rg_config_p); 
void baymax_clear_error_verify_hook(fbe_test_rg_configuration_t *rg_config_p); 
void baymax_wait_for_error_verify_hook(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_bool_t is_expected_to_hit);

/*****************************************
 * EXTERNAL FUNCTION DEFINITIONS
 *****************************************/
extern fbe_status_t mumford_the_magician_enable_error_injection_for_raid_group(fbe_raid_group_type_t raid_type,
                                                                               fbe_object_id_t rg_object_id);


/*******************
 * GLOBAL VARIABLES
 ******************/

/*!*******************************************************************
 * @var baymax_raid_group_config_qual
 *********************************************************************
 * @brief This is the raid group configuration we will use to 
 *        valdiate the correctness of event log messages.for 
 *        qual version
 *********************************************************************/
static fbe_test_rg_configuration_t baymax_raid_group_config[] = 
{
     /* width,   capacity    raid type,                   class,             block size  RAID-id.  bandwidth.*/
     {5,       0xE000,   FBE_RAID_GROUP_TYPE_RAID5,    FBE_CLASS_ID_PARITY,    520,       0,          0},
     {5,       0xE000,   FBE_RAID_GROUP_TYPE_RAID3,    FBE_CLASS_ID_PARITY,    520,       1,          0},
     {8,       0xE000,   FBE_RAID_GROUP_TYPE_RAID6,    FBE_CLASS_ID_PARITY,    520,       2,          0},
     {2,       0xE000,   FBE_RAID_GROUP_TYPE_RAID1,    FBE_CLASS_ID_MIRROR,    520,       3,          0},
     {6,       0xE000,   FBE_RAID_GROUP_TYPE_RAID10,   FBE_CLASS_ID_STRIPER,   520,       4,          0},
     {8,       0x32000,  FBE_RAID_GROUP_TYPE_RAID0,    FBE_CLASS_ID_STRIPER,   520,       5,          0},
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var baymax_invalid_error_tests 
 *********************************************************************
 * @brief Test cases to run against
 *********************************************************************/
static baymax_error_test_case_t baymax_invalid_error_tests[] =
{
    {
        "Corrupt CRC with no error injection",
        FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC,
        0x0, 0x1, 0x0, 0x10,
        FBE_FALSE,
        {
            0x0,
            {0}
        }, /* injected logical error detail */
        {
            0x2,  /* number of expected event log messages */
            {
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                    VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                    VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
             }
        }
    },
    {
        "Raid invalidated with no error injection",
        FBE_DATA_PATTERN_FLAGS_RAID_VERIFY,
        0x0, 0x1, 0x0, 0x10,
        FBE_FALSE,
        {
            0x0,
            {0}
        }, /* injected logical error detail */
        {
            0x2,  /* number of expected event log messages */
            {
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                    VR_RAID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                    VR_RAID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
             }
        }
    },
    {
        "Data lost with no error injection",
        FBE_DATA_PATTERN_FLAGS_DATA_LOST,
        0x0, 0x1, 0x0, 0x10,
        FBE_FALSE,
        {
            0x0,
            {0}
        }, /* injected logical error detail */
        {
            0x2,  /* number of expected event log messages */
            {
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                    MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, //VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                    MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, //VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
             }
        }
    },
    {
        "Corrupt CRC with lba stamp in error region",
        FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC,
        0x0, 0x1, 0x0, 0x10,
        FBE_TRUE,
        {
            0x1,
            {
                { 0x10, 0x01, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
            }
        }, /* injected logical error detail */
        {
            0x3,  /* number of expected event log messages */
            {
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                    VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                    VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                    VR_LBA_STAMP,  /* expected error info */
                    0x1, /* expected lba */
                    0x1  /* expected blocks */
                },
             }
        }
    },
    {
        "Raid invalidated with lba stamp error in error region",
        FBE_DATA_PATTERN_FLAGS_RAID_VERIFY,
        0x0, 0x1, 0x0, 0x10,
        FBE_TRUE,
        {
            0x1,
            {
                { 0x10, 0x01, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
            }
        }, /* injected logical error detail */
        {
            0x3,  /* number of expected event log messages */
            {
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                    VR_RAID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                    VR_RAID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                    VR_LBA_STAMP,  /* expected error info */
                    0x1, /* expected lba */
                    0x1  /* expected blocks */
                },
             }
        }
    },
    {
        "Data lost with lba stamp error in error region",
        FBE_DATA_PATTERN_FLAGS_DATA_LOST,
        0x0, 0x1, 0x0, 0x10,
        FBE_TRUE,
        {
            0x1,
            {
                { 0x10, 0x01, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, 0 },
            }
        }, /* injected logical error detail */
        {
            0x3,  /* number of expected event log messages */
            {
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                    MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, //VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                    MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, //VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR, /* expected error code */
                    VR_LBA_STAMP,  /* expected error info */
                    0x1, /* expected lba */
                    0x1  /* expected blocks */
                },
             }
        }
    },
    {
        "Corrupt CRC with lba stamp in error region",
        FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC,
        0x0, 0x1, 0x0, 0x10,
        FBE_TRUE,
        {
            0x1,
            {
                { 0x10, 0x01, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
            }
        }, /* injected logical error detail */
        {
            0x3,  /* number of expected event log messages */
            {
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                    VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                    VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                    (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                    0x1, /* expected lba */
                    0x1  /* expected blocks */
                },
             }
        }
    },
    {
        "Raid invalidated with lba stamp error in error region",
        FBE_DATA_PATTERN_FLAGS_RAID_VERIFY,
        0x0, 0x1, 0x0, 0x10,
        FBE_TRUE,
        {
            0x1,
            {
                { 0x10, 0x01, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
            }
        }, /* injected logical error detail */
        {
            0x3,  /* number of expected event log messages */
            {
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                    VR_RAID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                    VR_RAID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                    (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                    0x1, /* expected lba */
                    0x1  /* expected blocks */
                },
             }
        }
    },
    {
        "Data lost with lba stamp error in error region",
        FBE_DATA_PATTERN_FLAGS_DATA_LOST,
        0x0, 0x1, 0x0, 0x10,
        FBE_TRUE,
        {
            0x1,
            {
                { 0x10, 0x01, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, 0 },
            }
        }, /* injected logical error detail */
        {
            0x3,  /* number of expected event log messages */
            {
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, /* expected error code */
                    MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, //VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED, /* expected error code */
                    MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, //VR_INVALID_CRC, /* expected error info */
                    0x0, /* expected lba */
                    0x1  /* expected blocks */
                },
                {   
                    (fbe_u16_t)MUMFORD_THE_MAGICIAN_TEST_WILD_CARD, /* expected fru position */ 
                    SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, /* expected error code */
                    (VR_MULTI_BIT_CRC | VR_UNEXPECTED_CRC), /* expected error info */
                    0x1, /* expected lba */
                    0x1  /* expected blocks */
                },
             }
        }
    },
    
    {0,0,0,0,0, NULL, NULL} // terminator writes and read block counts are 0

};

fbe_api_logical_error_injection_record_t baymax_error_template =
{   0x10,    /* pos to inject error on */
    0x10,    /* width */
    0x1, //FBE_U32_MAX,    /* lba */
    0x1, //FBE_U32_MAX,    /* max number of blocks */
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP,    /* error type */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,    /* error mode */
    0x0,    /* error count */
    0x15,    /* error limit */
    0x0,    /* skip count */
    0x15,    /* skip limit */
    0x0,    /* error adjacency */
    0x0,    /* start bit */
    0x0,    /* number of bits */
    0x0,    /* erroneous bits */
    0x0,    /* crc detectable */
    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode */
};



/*!**************************************************************
 * baymax_free_resources()
 ****************************************************************
 * @brief
 *  release the memory reserved
 *
 * @param memory_ptr - pointer to memory        
 *
 * @return None.   
 *
 * @author
 *  12/09/2014 - Created. Deanna Heng
 *
 ****************************************************************/
void baymax_free_resources(fbe_u8_t **memory_ptr)
{
    if(*memory_ptr != NULL)
    {
        fbe_api_free_memory(*memory_ptr);
        *memory_ptr = NULL;
    }
    return;
}
/******************************************
 * end baymax_free_resources()
 ******************************************/

/*!**************************************************************
 * baymax_io_to_lun()
 ****************************************************************
 * @brief
 *  Send the specified IO to the lun
 *
 * @param lun_object_id - object id of the lun to send the io to
 * @param block_opcode - opcode of operation read/write
 * @param lba - lba to send io to 
 * @param block_count - number of blocks
 * @param block_size - size of blocks
 * @param data_pattern_flags - flags to indicate corrupt crc, raid, lost
 * @param b_check_checksum - whether or not to check the checksum      
 *
 * @return None.   
 *
 * @author
 *  10/09/2014 - Created. Deanna Heng
 *
 ****************************************************************/
void baymax_io_to_lun(fbe_object_id_t lun_object_id,
                                 fbe_payload_block_operation_opcode_t block_opcode,
                                 fbe_lba_t lba,
                                 fbe_block_count_t block_count,
                                 fbe_u32_t block_size, 
                                 fbe_data_pattern_flags_t data_pattern_flags,
                                 fbe_bool_t b_check_checksum)
{

    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u8_t                       *write_memory_ptr=NULL;
    fbe_sg_element_t                write_sg_list[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {0};
    fbe_sg_element_t               *sg_p;
    fbe_u64_t                       header_array[FBE_DATA_PATTERN_MAX_HEADER_PATTERN];
    fbe_u32_t                       bytes_to_allocate = 0;
    fbe_data_pattern_info_t         data_pattern_info = {0};
    fbe_data_pattern_t              pattern = FBE_DATA_PATTERN_LBA_PASS;
    fbe_payload_block_operation_t   block_operation;
    fbe_block_transport_block_operation_info_t block_operation_info;
    fbe_package_id_t                package_id = FBE_PACKAGE_ID_SEP_0;
    fbe_raid_verify_error_counts_t  verify_err_counts={0};
    fbe_payload_block_operation_status_t expected_read_with_check_checksum_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR; //FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
    fbe_payload_block_operation_qualifier_t expected_read_with_check_checksum_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST; //FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;

    bytes_to_allocate = (fbe_u32_t)(block_count * block_size);

    /*
     * sg list for write operation
     */
    write_memory_ptr = (fbe_u8_t*)fbe_api_allocate_memory(bytes_to_allocate);
    MUT_ASSERT_NOT_NULL(write_memory_ptr);
    sg_p = (fbe_sg_element_t*)write_sg_list;

    /* First plant the sgls
     */
    status = fbe_data_pattern_sg_fill_with_memory(sg_p,
                                                  write_memory_ptr,
                                                  block_count,
                                                  block_size,
                                                  FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS,
                                                  FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG);
    if( status != FBE_STATUS_OK)
    {
        baymax_free_resources(&write_memory_ptr);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, " Failed to fill write memory ");

    /* Fill data pattern information structure
     */
    header_array[0] = FBE_TEST_HEADER_PATTERN;
    header_array[1] = lun_object_id;
    status = fbe_data_pattern_build_info(&data_pattern_info,
                                         pattern,
                                         data_pattern_flags,
                                         lba,
                                         0x55, /*sequence_id*/
                                         2, /*num_header_words*/
                                         header_array);
    if( status != FBE_STATUS_OK)
    {
        baymax_free_resources(&write_memory_ptr);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status," Failed to build write pattern info ");

    status = fbe_data_pattern_fill_sg_list(sg_p,
                                           &data_pattern_info,
                                           block_size,
                                           FBE_U64_MAX,
                                           FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS);
    if( status != FBE_STATUS_OK)
    {
        baymax_free_resources(&write_memory_ptr);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, " Error while Filling sg list operation");

    /* Build the block operation for the original request.
     */
    status = fbe_payload_block_build_operation(&block_operation,
                                               block_opcode,
                                               lba,
                                               block_count,
                                               block_size,
                                               1,
                                               NULL);
    if( status != FBE_STATUS_OK)
    {
        baymax_free_resources(&write_memory_ptr);
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, " Error while building write block operation");


    if (b_check_checksum == FBE_TRUE)
    {
        /* Set the `check checksum' flag.
         */
        fbe_payload_block_set_flag(&block_operation,
                                   FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);
    }
    else
    {
        /* Else clear the `check checksum' flag.
         */
        fbe_payload_block_clear_flag(&block_operation,
                                     FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);
    }

    /* send the block operation
     */
    block_operation_info.block_operation = block_operation;
    block_operation_info.verify_error_counts = verify_err_counts;
    status = fbe_api_block_transport_send_block_operation(lun_object_id, 
                                                          package_id,
                                                          &block_operation_info, 
                                                          sg_p);

    if( status != FBE_STATUS_OK)
    {
        baymax_free_resources(&write_memory_ptr);
    }

       /* Validate that the request was sent successfully and that read
         * block operations status and qualifier are as expected.
         */
    if (block_operation_info.block_operation.block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
    {
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,
                                 " Error while sending read block operation");
        MUT_ASSERT_INT_EQUAL_MSG(expected_read_with_check_checksum_status, 
                                 block_operation_info.block_operation.status,
                                 " Error found in read block operation");
        MUT_ASSERT_INT_EQUAL_MSG(expected_read_with_check_checksum_qualifier, 
                                 block_operation_info.block_operation.status_qualifier,
                                 " Error found in read block operation");
    }


    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status," Error while sending block operation");
}
/******************************************
 * end baymax_io_to_lun()
 ******************************************/

/*!**************************************************************
 * baymax_set_error_verify_hook()
 ****************************************************************
 * @brief
 *  Set the error verify hook. Adjust accordingly for raid10
 *  since we want to check the on the mirror
 *
 * @param rg_config_p - the raid group configuration to test         
 *
 * @return None.   
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void baymax_set_error_verify_hook(fbe_test_rg_configuration_t *rg_config_p) 
{
    fbe_u32_t               index = 0;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_object_id_t         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t         mirror_rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;

    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    { 
        //  Get the downstream object list; this is the list of mirror objects for this R10 RG
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
        for (index = 0; index < downstream_object_list.number_of_downstream_objects; index++)
        {
            //  Get the object ID of the corresponding mirror object
            mirror_rg_object_id = downstream_object_list.downstream_object_list[index];
            
            //mut_printf(MUT_LOG_LOW, "Adding Debug Hook for rgid %d object %d ds_mirror_id: %d", 
            //           rg_config_p->raid_group_id, rg_object_id, mirror_rg_object_id);
            
            status = fbe_api_scheduler_add_debug_hook(mirror_rg_object_id,
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                      FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                      0,0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_LOG);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        
    } else {
        status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, 
                                                  SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}
/******************************************
 * end baymax_set_error_verify_hook()
 ******************************************/

/*!**************************************************************
 * baymax_clear_error_verify_hook()
 ****************************************************************
 * @brief
 *  Clear the error verify hook
 *
 * @param rg_config_p - the pointer to the raid group     
 *
 * @return None.
 *
 * @author
 *  12/09/2014 - Created - Deanna Heng
 *
 ****************************************************************/
void baymax_clear_error_verify_hook(fbe_test_rg_configuration_t *rg_config_p) 
{
    fbe_u32_t               index = 0;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_object_id_t         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t         mirror_rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;

    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    { 
        //  Get the downstream object list; this is the list of mirror objects for this R10 RG
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
        for (index = 0; index < downstream_object_list.number_of_downstream_objects; index++)
        {
            //  Get the object ID of the corresponding mirror object
            mirror_rg_object_id = downstream_object_list.downstream_object_list[index];
            
            //mut_printf(MUT_LOG_LOW, "Clearing Debug Hook for rgid %d object %d ds_mirror_id: %d", 
            //               rg_config_p->raid_group_id, rg_object_id, mirror_rg_object_id);
            
            status = fbe_api_scheduler_del_debug_hook(mirror_rg_object_id,
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                      FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                      0,0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_LOG);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        
    } else {
        status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, 
                                                  SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}
/*************************
 * end baymax_clear_error_verify_hook()
 ************************/

/*!**************************************************************
 * baymax_wait_for_error_verify_hook()
 ****************************************************************
 * @brief
 *  Verify error verify was run or not run
 *
 * @param rg_config_p - the pointer to the raid group 
 *        is_expected_to_hit - whether or not error verify is expected to run        
 *
 * @return None.
 *
 * @author
 *  12/09/2014 - Created - Deanna Heng
 *
 ****************************************************************/
void baymax_wait_for_error_verify_hook(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_bool_t is_expected_to_hit) 
{
    fbe_u32_t               index = 0;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_object_id_t         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t         mirror_rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_scheduler_debug_hook_t      hook;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = 2000;


    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY;
    hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT;
    hook.object_id = rg_object_id;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_LOG;
    hook.val1 = 0;
    hook.val2 = 0;
    hook.counter = 0;

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    { 
        //  Get the downstream object list; this is the list of mirror objects for this R10 RG
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
    }

    while (current_time < timeout_ms) {
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        { 
            for (index = 0; index < downstream_object_list.number_of_downstream_objects; index++)
            {
                //  Get the object ID of the corresponding mirror object
                mirror_rg_object_id = downstream_object_list.downstream_object_list[index];
                hook.object_id = mirror_rg_object_id;
                
                //mut_printf(MUT_LOG_HIGH, "Getting Debug Hook for rgid %d object %d ds_mirror_id: %d", 
                //               rg_config_p->raid_group_id, rg_object_id, mirror_rg_object_id);
                
                status = fbe_api_scheduler_get_debug_hook(&hook);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                if (hook.counter != 0) {
                    mut_printf(MUT_LOG_TEST_STATUS, "We have hit the debug hook %d times", (int)hook.counter);
                    if (is_expected_to_hit)
                    {
                        return;
                    }
                    else
                    {
                        MUT_ASSERT_UINT64_EQUAL_MSG(0, hook.counter, "Hook is not expected to hit");
                    }
                }
            }
            
        } else {
            status = fbe_api_scheduler_get_debug_hook(&hook);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if (hook.counter != 0) {
                mut_printf(MUT_LOG_TEST_STATUS, "We have hit the debug hook %d times", (int)hook.counter);
                if (is_expected_to_hit)
                {
                    return;
                }
                else
                {
                    MUT_ASSERT_UINT64_EQUAL_MSG(0, hook.counter, "Hook is not expected to hit");
                }
            }
        }

        current_time = current_time + 200;
        fbe_api_sleep (200);
    }

    if (is_expected_to_hit)
    {
        MUT_ASSERT_UINT64_NOT_EQUAL(0, hook.counter);
    }

}
/*************************
 * end baymax_wait_for_error_verify_hook()
 ************************/

/*!**************************************************************
 * baymax_pretest_cleanup()
 ****************************************************************
 * @brief
 *  Erase errors from disk and from logs
 *
 * @param rg_config_p - the pointer to the raid group          
 *
 * @return None.
 *
 * @author
 *  12/09/2014 - Created - Deanna Heng
 *
 ****************************************************************/
void baymax_pretest_cleanup(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t lun_object_id;

    /* Write background pattern on all luns to erase old data with known pattern.
     */
    //big_bird_write_background_pattern();
    fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, 
                                          &lun_object_id);

    baymax_io_to_lun(lun_object_id, 
                     FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                     0,
                     100,
                     rg_config_p->block_size, 
                     FBE_DATA_PATTERN_FLAGS_INVALID,
                     FBE_FALSE);
    /* Clear following old statistics:
     *  1. sep event logs
     *  2. verify report
     *  3. sep event logs statistics
     */
    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "== %s Failed to clear SEP event log! == ");
    //status = fbe_api_lun_clear_verify_report(lun_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_clear_event_log_statistics(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "== %s Failed to clear SEP event log statistics! == ");

}
/*************************
 * end baymax_pretest_cleanup()
 ************************/

/*!**************************************************************
 * baymax_set_up_error_injection()
 ****************************************************************
 * @brief
 *  Wait for verify to complete on the raid group
 *
 * @param rg_config_p - the pointer to the raid group          
 *        verify_type - the type of verify to wait for  
 *
 * @return None.
 *
 * @author
 *  12/09/2014 - Created - Deanna Heng
 *
 ****************************************************************/
void baymax_wait_for_verify_complete(fbe_test_rg_configuration_t * rg_config_p,
                                     fbe_lun_verify_type_t verify_type)
{
    fbe_object_id_t rg_object_id;
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    mut_printf(MUT_LOG_LOW, "Wait for verify to complete 0x%x\n", rg_object_id);
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
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
           fbe_test_verify_wait_for_verify_completion(downstream_object_list.downstream_object_list[index],
                                                   10000,
                                                   FBE_LUN_VERIFY_TYPE_ERROR);
        }
    } 
    else
    {
        fbe_test_verify_wait_for_verify_completion(rg_object_id,
                                                   10000,
                                                   verify_type);
    }
}
/*************************
 * end baymax_wait_for_verify_complete()
 ************************/

/*!**************************************************************
 * baymax_set_up_error_injection()
 ****************************************************************
 * @brief
 *  Set up the records for the raid group
 *
 * @param rg_config_p - the pointer to the raid group          
 *        test_case_p - the test case to run  
 *
 * @return None.
 *
 * @author
 *  12/09/2014 - Created - Deanna Heng
 *
 ****************************************************************/
void baymax_set_up_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                   baymax_error_test_case_t * test_case_p)
{
    fbe_u32_t index;
    fbe_object_id_t rg_object_id;//, lun_object_id;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_logical_error_injection_record_t  record;
    fbe_api_logical_error_injection_get_stats_t lei_stats;
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    for (index = 0; index < test_case_p->lei_records.injected_error_count; index++)
    {
        fbe_copy_memory(&record, &baymax_error_template, sizeof(fbe_api_logical_error_injection_record_t));
    
        record.err_type = test_case_p->lei_records.error_list[index].err_type;
        record.lba = test_case_p->lei_records.error_list[index].lba;
        record.blocks = test_case_p->lei_records.error_list[index].blocks;
        record.pos_bitmap = test_case_p->lei_records.error_list[index].pos_bitmap;
        record.err_limit = test_case_p->lei_records.error_list[index].err_limit;

        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10 || rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1)
        {
            record.pos_bitmap = 0x1;
        } 

        /* Disable all the records, if there are some already enabled.
         */
        status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Create our own error-record(s).
         */
        status = fbe_api_logical_error_injection_create_record(&record);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Enable error injection for the current raid group object
     */
    status = mumford_the_magician_enable_error_injection_for_raid_group(rg_config_p->raid_type, rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Validate the injected errors are as per our configuration.
     */
    status = fbe_api_logical_error_injection_get_stats(&lei_stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(lei_stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(lei_stats.num_records, test_case_p->lei_records.injected_error_count );
}
/*************************
 * end baymax_set_up_error_injection()
 ************************/

/*!**************************************************************
 * baymax_clean_error_injection()
 ****************************************************************
 * @brief
 *  Remove the records and disable lei
 *
 * @param rg_config_p - the pointer to the raid group           
 *
 * @return None.
 *
 * @author
 *  12/09/2014 - Created - Deanna Heng
 *
 ****************************************************************/
void baymax_clean_error_injection(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Raid 10s only enable the mirrors, so just disable the mirror class.
         */
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            status = fbe_api_logical_error_injection_disable_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        } else {
            status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        status = fbe_api_logical_error_injection_disable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/*************************
 * end baymax_clean_error_injection()
 ************************/

/*!**************************************************************
 * baymax_run_single_test_case()
 ****************************************************************
 * @brief
 *  Run one single test case on the raid group
 *
 * @param rg_config_p - the pointer to the raid group
 *        test_case_p - the test case to run             
 *
 * @return None.
 *
 * @author
 *  12/09/2014 - Created - Deanna Heng
 *
 ****************************************************************/
void baymax_run_single_test_case(fbe_test_rg_configuration_t * rg_config_p,
                                 baymax_error_test_case_t * test_case_p)
{
    fbe_object_id_t rg_object_id, lun_object_id;  
    fbe_object_id_t logged_object_id;
    fbe_bool_t b_is_error_verify_run = test_case_p->b_is_error_verify_run;

    mut_printf(MUT_LOG_TEST_STATUS, test_case_p->description);

    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, 
                                          &lun_object_id);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        b_is_error_verify_run = FBE_FALSE;
    }

    baymax_pretest_cleanup(rg_config_p);
    baymax_set_error_verify_hook(rg_config_p);
    baymax_set_up_error_injection(rg_config_p, test_case_p);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Writing pattern 0x%x to lun %d\n", test_case_p->invalid_pattern, rg_config_p->logical_unit_configuration_list->lun_number);
    baymax_io_to_lun(lun_object_id, 
                     FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                     test_case_p->error_lba,
                     test_case_p->error_blocks,
                     rg_config_p->block_size, 
                     test_case_p->invalid_pattern,
                     FBE_FALSE);
    mut_printf(MUT_LOG_TEST_STATUS, "Read from lun %d\n", rg_config_p->logical_unit_configuration_list->lun_number);
    baymax_io_to_lun(lun_object_id,
                     FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                     test_case_p->read_lba,
                     test_case_p->read_blocks,
                     rg_config_p->block_size, 
                     NULL,
                     FBE_TRUE);

    fbe_api_sleep(1000);
    baymax_wait_for_verify_complete(rg_config_p, FBE_LUN_VERIFY_TYPE_ERROR);
    baymax_wait_for_error_verify_hook(rg_config_p, b_is_error_verify_run);
    /* In case of stripped-mirror raid group, do not validate object id 
     * against which error is reported. So, we will pass an wild card
     * to function validating event log result.
     */ 
    logged_object_id = (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10) 
                            ? (rg_object_id)
                            : (MUMFORD_THE_MAGICIAN_TEST_WILD_CARD);

    /* Validate reported messages with expected one.
     */
    mumford_the_magician_validates_event_log_message(&test_case_p->event_log_result, logged_object_id, b_is_error_verify_run); /* TRUE = verify message present */
    baymax_clear_error_verify_hook(rg_config_p);
    baymax_clean_error_injection(rg_config_p);
}
/*************************
 * end baymax_run_single_test_case()
 ************************/


/*!**************************************************************
 * baymax_invalids_test()
 ****************************************************************
 * @brief
 *  Run all the test cases specified for each raid group
 *
 * @param rg_config_p - the pointer to the raid groups
 *        test_case_p - the test cases to run               
 *
 * @return None.
 *
 * @author
 *  12/09/2014 - Created - Deanna Heng
 *
 ****************************************************************/
void baymax_invalids_test(fbe_test_rg_configuration_t * rg_config_p, baymax_error_test_case_t * test_case_p)
{
    fbe_u32_t                   raid_group_count = 0;
    fbe_u32_t                   index = 0;
    fbe_u32_t drives_to_remove = 1;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    while (test_case_p->error_blocks !=0 && test_case_p->read_blocks !=0)
    {
        if (test_case_p->lei_records.injected_error_count > 0)
        {
            fbe_api_logical_error_injection_enable_ignore_corrupt_crc_data_errors();
        } 
        else
        {
            fbe_api_logical_error_injection_disable_ignore_corrupt_crc_data_errors();
        }

        for (index = 0; index < raid_group_count; index++) 
        {


            baymax_run_single_test_case(&rg_config_p[index], test_case_p); 


            // Now run the degraded test
            /* Skip Raid 0 
             */
            if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID0 ||
                test_case_p->lei_records.injected_error_count > 0)
            {
                continue;
            }

            big_bird_remove_all_drives(&rg_config_p[index], 1, drives_to_remove, 
                                       FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                                       0, /* do not wait between removals */
                                       FBE_DRIVE_REMOVAL_MODE_RANDOM);
            baymax_run_single_test_case(&rg_config_p[index], test_case_p); 

            big_bird_insert_all_drives(&rg_config_p[index], 1, drives_to_remove, 
                                       FBE_TRUE /* We do not plan on re-inserting this drive later. */ );
            /* Wait for the rebuilds to finish.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
            big_bird_wait_for_rebuilds(&rg_config_p[index], 1, drives_to_remove );
            mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);
        }

        test_case_p++;
    }
}
/*************************
 * end baymax_invalids_test()
 ************************/

/*!**************************************************************
 * baymax_run_tests()
 ****************************************************************
 * @brief
 *  Run invalids test              
 *
 * @return None.
 *
 * @author
 *  12/09/2014 - Created - Deanna Heng
 *
 ****************************************************************/
void baymax_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    baymax_error_test_case_t * error_cases_p = (baymax_error_test_case_t *) context_p; 
    /* Before starting the test verify that all system verifies are complete
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    baymax_invalids_test(rg_config_p, error_cases_p);

    return;
}
/*************************
 * end baymax_run_tests()
 ************************/

/*!**************************************************************
 * baymax_test()
 ****************************************************************
 * @brief
 *  Run invalids test
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  12/09/2014 - Created - Deanna Heng
 *
 ****************************************************************/
void baymax_test(void)
{
    fbe_test_run_test_on_rg_config(&baymax_raid_group_config[0], 
                                   &baymax_invalid_error_tests[0], 
                                   baymax_run_tests,
                                   BAYMAX_LUNS_PER_RAID_GROUP,
                                   BAYMAX_CHUNKS_PER_LUN);

    return;
}
/*************************
 * end baymax_test()
 ************************/


/*!**************************************************************
 * baymax_setup()
 ****************************************************************
 * @brief
 *  Setup for a event log test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  12/09/2014 - Created - Deanna Heng
 *
 ****************************************************************/
void baymax_setup(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t                   raid_group_count = 0;

    mumford_the_magician_common_setup();

    /* Set error injection test mode*/
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    if (fbe_test_util_is_simulation()) 
    {
        rg_config_p = &baymax_raid_group_config[0];
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        /* Initialize the raid group configuration 
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        /* Setup the physical config for the raid groups on both SPs
         */
        elmo_create_physical_config_for_rg(rg_config_p, raid_group_count);
        elmo_load_sep_and_neit();
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/**************************
 * end baymax_setup()
 *************************/


/*!**************************************************************
 * baymax_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup for the baymax_cleanup().
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  12/09/2014 - Created. Deanna Heng
 *
 ****************************************************************/
void baymax_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    if (fbe_test_util_is_simulation()) 
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    mumford_the_magician_common_cleanup();
    return;
}
/***************************
 * end baymax_cleanup()
 ***************************/




/*****************************************
 * end file baymax_test.c
 *****************************************/
