/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file minion_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test to inject errors on the paged during encryption.
 *
 * @author
 *  5/1/2014 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_dps_memory_interface.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe_database.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_encryption_interface.h"
#include "sep_hook.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "sep_zeroing_utils.h"
#include "fbe_test.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_trace_interface.h"
#include "pp_utils.h"
#include "sep_rebuild_utils.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "neit_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * minion_short_desc = "This scenario tests uncorrectable errors on paged metadata during encryption.";
char * minion_long_desc ="\
The scenario corrupts paged metadata during encryption and tests.";

enum minion_defines_e {
    MINION_TEST_LUNS_PER_RAID_GROUP = 1,

    /* A few extra chunks beyond what we absoltely need for the LUNs. */
    MINION_EXTRA_CHUNKS = 32,
    MINION_MAX_IO_SIZE_BLOCKS = (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE, //4*0x400*6
    MINION_MAX_ERRORS = 5,

    /* Blocks needed for our test cases.  To test all the transition cases we need at least 4
     * blocks worth of paged. 
     */  
    MINION_PAGED_BLOCKS = 3,
    MINION_CHUNKS_PER_PAGED_BLOCK = (FBE_BYTES_PER_BLOCK / FBE_RAID_GROUP_CHUNK_ENTRY_SIZE),
    MINION_CHUNKS_PER_LUN = (MINION_CHUNKS_PER_PAGED_BLOCK * MINION_PAGED_BLOCKS),
    
    MINION_BLOCKS_PER_RG = ((MINION_CHUNKS_PER_LUN + MINION_EXTRA_CHUNKS) * 0x800) + FBE_TEST_EXTRA_CAPACITY_FOR_METADATA,
    MINION_MAX_TABLES_PER_RAID_TYPE = 10,
};

/*!*************************************************
 * @enum minion_raid_type_t
 ***************************************************
 * @brief Set of raid types we will test.
 *
 * @note  The raid type value must match the order in
 *        the error case and raid group configuration
 *        tables.
 ***************************************************/
typedef enum minion_raid_type_e
{
    MINION_RAID_TYPE_RAID0 = 0,
    MINION_RAID_TYPE_RAID1 = 1,
    MINION_RAID_TYPE_RAID5 = 2,
    MINION_RAID_TYPE_RAID3 = 3,
    MINION_RAID_TYPE_RAID6 = 4,
    MINION_RAID_TYPE_RAID10 = 5,
    MINION_RAID_TYPE_LAST,
}
minion_raid_type_t;
typedef struct minion_error_record_s
{
    fbe_chunk_index_t chunk_index;
    fbe_chunk_count_t chunk_count;
    fbe_api_logical_error_injection_type_t err_type; /*!< Type of errors, see above      */
}
minion_error_record_t;

typedef void (*minion_test_function_t)(fbe_test_rg_configuration_t *rg_config_p,
                                               void *error_case_p);
/*!*************************************************
 * @typedef minion_error_case_t
 ***************************************************
 * @brief Single test case with the error and
 *        the blocks to read for.
 *
 ***************************************************/
typedef struct minion_error_case_s
{
    fbe_u8_t *description_p;            /*!< Description of test case. */
    fbe_u32_t line;                     /*!< Line number of test case. */
    minion_test_function_t function_p;
    minion_error_record_t errors[MINION_MAX_ERRORS];
    fbe_chunk_index_t first_rekey_chunk;
    fbe_chunk_index_t last_rekey_chunk;
    fbe_chunk_index_t chunk_to_pause_rekey;

    /*! Determines if we should run this test case during the reboot test.
     */
    fbe_bool_t b_run_reboot; 
}
minion_error_case_t;

/* Because the LUNs are so large, we optimize to write the pattern once.
 */
static fbe_bool_t minion_wrote_pattern = FBE_FALSE;

/* We pick a random raid type in setup and run our tests on that.
 */
static minion_raid_type_t minion_current_raid_type = MINION_RAID_TYPE_LAST;

/* Determines if this is a reboot test or not.  The test function sets this before we start.
 */
static fbe_bool_t minion_should_reboot = FBE_FALSE;

/* Determines if we should run even or odd test cases.
 */
static fbe_u32_t minion_run_even_test_cases = FBE_FALSE;

/*!*******************************************************************
 * @var minion_current_config_array
 *********************************************************************
 * @brief Pointer to the config array we are testing with.
 *
 *********************************************************************/
static fbe_test_rg_configuration_array_t *minion_current_config_array = NULL;

/*!*******************************************************************
 * @var minion_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_array_t minion_raid_group_config[] = 
{
    {
        /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/

        {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},
        {2,       MINION_BLOCKS_PER_RG,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0},
        {4,       MINION_BLOCKS_PER_RG * 2,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,          1,          0},
        {3,       MINION_BLOCKS_PER_RG * 3,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            5,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};

/***************************************** 
 * FORWARD FUNCTION DECLARATIONS
 *****************************************/

static void minion_corrupt_paged_during_encryption(fbe_test_rg_configuration_t * rg_config_p,
                                                   void *err_case_p);
static void minion_corrupt_paged_after_encryption(fbe_test_rg_configuration_t * rg_config_p,
                                                   void *err_case_p);
/*!*******************************************************************
 * @var minion_non_ambiguous_error_cases
 *********************************************************************
 * @brief
 *  These are cases which are very simple in the sense that
 *  just looking at the paged metadata allows us to know where the
 *  checkpoint should be.
 *
 *********************************************************************/
minion_error_case_t minion_simple_error_cases[] =
{
    {
        /*                              | -- Range of paged loss --- |
         *                             \|/                          \|/
         *  ... | Rekeyed | Not Rekeyed | Loss | Loss | Loss | ...   | Not Rekeyed | Not Rekeyed |
         *               /|\
         *                | Rekey Checkpoint
         */
        "Loss in clearly not rekeyed area.  Checkpoint before loss.  start -> Rekeyed -> Not Rekeyed -> Loss ", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                MINION_CHUNKS_PER_PAGED_BLOCK, /* Chunk index. */
                1, /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        /* There are no valid chunks prior to not rekeyed.
         */
        0, /* First rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK - 2, /* Last rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK - 1, /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {
        /*                          | -- Range of paged loss --- |
         *                         \|/                          \|/
         *  ... | Rekeyed | Rekeyed | Loss | Loss | Loss | ...   | Rekeyed | Not Rekeyed |
         *                                                                /|\
         *                                                                 | Rekey Checkpoint
         */
        "Loss in clearly rekeyed area.  Checkpoint after loss.  start -> Rekeyed -> Loss -> Rekeyed -> Not Rekeyed", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                MINION_CHUNKS_PER_PAGED_BLOCK, /* Chunk index. */
                1, /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        /* There are no valid chunks prior to not rekeyed.
         */
        0, /* First rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK * 2, /* Last rekeyed chunk. */
        (MINION_CHUNKS_PER_PAGED_BLOCK * 2) + 1, /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {
        /*   | -- Range of paged loss --- |
         *  \|/                          \|/
         *   | Loss | Loss | Loss | ...   | Rekeyed | Not Rekeyed |
         *                                         /|\
         *                                          | Rekey Checkpoint
         */  
        "Loss in clearly rekeyed area.  Checkpoint after loss.  start -> Loss -> Rekeyed -> Not Rekeyed", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                0, /* Chunk index. */
                1, /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        /* There are no valid chunks prior to not rekeyed.
         */
        0, /* First rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK, /* Last rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK + 1, /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {NULL, FBE_U32_MAX, NULL, FBE_U32_MAX}, /* Terminator */
};

/*!*******************************************************************
 * @var minion_ambiguous_error_cases
 *********************************************************************
 * @brief
 *  These are cases which require us to not only scan the paged
 *  but also verify the live stripe to determine the state of the
 *  lost chunks.
 *
 *********************************************************************/
minion_error_case_t minion_ambiguous_error_cases[] =
{
    {
        /*   | -- Range of paged loss --- |
         *  \|/                          \|/
         *   | Loss | Loss | Loss | ...   | Not Rekeyed | Not Rekeyed |
         *  /|\
         *   |
         *   Rekey Checkpoint
         */
        "Loss in ambiguous area.  Checkpoint before loss.  start -> Loss -> Not Rekeyed", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                0, /* Chunk index. */
                1, /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        /* There are no valid chunks prior to not rekeyed.
         */
        FBE_U32_MAX, /* First rekeyed chunk. */
        FBE_U32_MAX, /* Last rekeyed chunk. */
        0, /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {   /*   | ----- Range of paged loss --- |
         *  \|/                             \|/
         *   | Loss | Loss | Loss |    ...   | Not Rekeyed | Not Rekeyed | ...
         *         /|\ 
         *          | 
         *           Rekey Checkpoint
         */
        "Loss in ambiguous area.  Checkpoint within loss.  start -> Loss -> Not Rekeyed", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                1, /* Chunk index. */
                1, /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        0, /* First rekeyed chunk. */
        0, /* Last rekeyed chunk. */
        1, /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {   /*   | ----- Range of paged loss --- |
         *  \|/                             \|/
         *   | Loss | Loss | Loss |    ...   | Not Rekeyed | Not Rekeyed | ...
         *                                  /|\ 
         *                                   | 
         *                                   Rekey Checkpoint
         */
        "Loss in ambiguous area.  Checkpoint after loss.  start -> Loss -> Not Rekeyed", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                MINION_CHUNKS_PER_PAGED_BLOCK / 2, /* Chunk index. */
                1, /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        0, /* First rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK - 1, /* Last rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK, /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {
        /*                          | -- Range of paged loss --- |
         *                         \|/                          \|/
         *  ... | Rekeyed | Rekeyed | Loss | Loss | Loss | ...   | Not Rekeyed | Not Rekeyed |
         *                         /|\
         *                          | Rekey Checkpoint
         */
        "Loss in ambiguous area.  Checkpoint before loss.  start -> Rekeyed -> Loss -> Not Rekeyed", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                MINION_CHUNKS_PER_PAGED_BLOCK, /* Chunk index. */
                1, /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        /* There are no valid chunks prior to not rekeyed.
         */
        0, /* First rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK - 1, /* Last rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK, /* Chunk to pause rekey */
        FBE_TRUE, /* Run for reboot. */
    },
    {   /*                          | -- Range of paged loss --- |
         *                         \|/                          \|/
         *  ... | Rekeyed | Rekeyed | Loss | Loss | Loss | ...   | Not Rekeyed | Not Rekeyed |
         *                                       /|\
         *                                        | Rekey Checkpoint
         */
        "Loss in ambiguous area.  Checkpoint within loss.  start -> Rekeyed -> Loss -> Not Rekeyed", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                MINION_CHUNKS_PER_PAGED_BLOCK + 2, /* Chunk index. */
                1, /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        0, /* First rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK + 1, /* Last rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK + 2, /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {  /*                           | -- Range of paged loss --- |
         *                         \|/                          \|/
         *  ... | Rekeyed | Rekeyed | Loss | Loss | Loss | ...   | Not Rekeyed | Not Rekeyed |
         *                                                      /|\
         *                                                       | Rekey Checkpoint
         */
        "Loss in ambiguous area.  Checkpoint after loss.  start -> Rekeyed -> Loss -> Not Rekeyed", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                MINION_CHUNKS_PER_PAGED_BLOCK + (MINION_CHUNKS_PER_PAGED_BLOCK / 2), /* Chunk index. */
                1, /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        0, /* First rekeyed chunk. */
        (MINION_CHUNKS_PER_PAGED_BLOCK * 2) - 1, /* Last rekeyed chunk. */
        MINION_CHUNKS_PER_PAGED_BLOCK * 2, /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {
        "Cause error on paged after encryption is done.", __LINE__,
        minion_corrupt_paged_after_encryption,
        {
            {
                MINION_CHUNKS_PER_PAGED_BLOCK, /* Chunk index. */
                1, /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        FBE_U32_MAX, /* First rekeyed chunk. */
        FBE_U32_MAX, /* Last rekeyed chunk. */
        FBE_LBA_INVALID, /* Chunk to pause rekey */
        FBE_TRUE, /* Run for reboot. */
    },
    {NULL, FBE_U32_MAX, NULL, FBE_U32_MAX}, /* Terminator */
};

/*!*******************************************************************
 * @var minion_everything_rekeyed_error_cases
 *********************************************************************
 * @brief
 *  These are cases where we have already finished rekeying
 *  and simply need to reconstruct this rekeyed state of the paged.
 *
 *********************************************************************/
minion_error_case_t minion_everything_rekeyed_error_cases[] =
{
    {
        /*   | -- Range of paged loss --- |
         *  \|/                          \|/
         *   | Loss | Loss | Loss | Loss  |
         *                               /|\
         *                                | Rekey Checkpoint
         */
        "Everything rekeyed (all lost)  start -> Loss -> Loss -> Loss -> Loss", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                0, /* Chunk index. */
                (MINION_CHUNKS_PER_PAGED_BLOCK * 3) , /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        /* There are no valid chunks prior to not rekeyed.
         */
        0, /* First rekeyed chunk. */
        (MINION_CHUNKS_PER_PAGED_BLOCK * 3) - 1, /* Last rekeyed chunk. */
        (MINION_CHUNKS_PER_PAGED_BLOCK * 3), /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {
        /*   | -- Range of paged loss --- |
         *  \|/                          \|/
         *   | Loss | Loss | Loss | Loss  |
         *                               /|\
         *                                | Rekey Checkpoint
         */
        "Everything rekeyed (First lost)  start -> Loss -> Loss -> Rekeyed -> Rekeyed", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                0, /* Chunk index. */
                (MINION_CHUNKS_PER_PAGED_BLOCK * 2) , /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        /* There are no valid chunks prior to not rekeyed.
         */
        0, /* First rekeyed chunk. */
        (MINION_CHUNKS_PER_PAGED_BLOCK * 3) - 1, /* Last rekeyed chunk. */
        (MINION_CHUNKS_PER_PAGED_BLOCK * 3), /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {
        /*   | -- Range of paged loss --- |
         *  \|/                          \|/
         *   | Loss | Loss | Loss | Loss  |
         *                               /|\
         *                                | Rekey Checkpoint
         */
        "Everything rekeyed (Last lost)  start -> Rekeyed -> Rekeyed-> Loss -> Loss ", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                0, /* Chunk index. */
                (MINION_CHUNKS_PER_PAGED_BLOCK * 2) , /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        /* There are no valid chunks prior to not rekeyed.
         */
        0, /* First rekeyed chunk. */
        (MINION_CHUNKS_PER_PAGED_BLOCK * 3) - 1, /* Last rekeyed chunk. */
        (MINION_CHUNKS_PER_PAGED_BLOCK * 3), /* Chunk to pause rekey */
        FBE_TRUE, /* Run for reboot. */
    },
    {NULL, FBE_U32_MAX, NULL, FBE_U32_MAX}, /* Terminator */
};

/*!*******************************************************************
 * @var minion_everything_rekeyed_error_cases
 *********************************************************************
 * @brief
 *  These are cases where rekey has not even started yet,
 *  so we need to reconstruct this non-rekeyed state of the paged.
 *
 *********************************************************************/
minion_error_case_t minion_nothing_rekeyed_error_cases[] =
{
    {
        /*   | -- Range of paged loss --- |
         *  \|/                          \|/
         *   | Loss | Loss | Loss | Loss  | 
         *  /|\
         *   |
         *   Rekey Checkpoint
         */
        "Nothing rekeyed (all lost).  start -> Loss -> Loss -> Loss -> Loss", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                0, /* Chunk index. */
                (MINION_CHUNKS_PER_PAGED_BLOCK * 3), /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        /* There are no valid chunks prior to not rekeyed.
         */
        FBE_U32_MAX, /* First rekeyed chunk. */
        FBE_U32_MAX, /* Last rekeyed chunk. */
        0, /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {
        /*   | --------- Range of paged loss ---------- |
         *  \|/                                        \|/
         *   | Loss | Loss | Not Rekeyed | Not Rekeyed  | 
         *  /|\
         *   |
         *   Rekey Checkpoint
         */
        "Nothing rekeyed (First lost).  start -> Loss -> Loss -> Not Rekeyed -> Not Rekeyed", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                0, /* Chunk index. */
                (MINION_CHUNKS_PER_PAGED_BLOCK * 2) , /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        /* There are no valid chunks prior to not rekeyed.
         */
        FBE_U32_MAX, /* First rekeyed chunk. */
        FBE_U32_MAX, /* Last rekeyed chunk. */
        0, /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {
        /*   | --------- Range of paged loss ---------- |
         *  \|/                                        \|/
         *   | Loss | Loss | Not Rekeyed | Not Rekeyed  | 
         *  /|\
         *   |
         *   Rekey Checkpoint
         */
        "Nothing rekeyed (Last lost).  start -> Not Rekeyed -> Not Rekeyed-> Loss -> Loss ", __LINE__,
        minion_corrupt_paged_during_encryption,
        {
            {
                (MINION_CHUNKS_PER_PAGED_BLOCK * 2), /* Chunk index. */
                (MINION_CHUNKS_PER_PAGED_BLOCK * 2) , /* chunk count */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP,
            },
            {FBE_U32_MAX, FBE_U32_MAX} /* Terminator */
        },
        /* There are no valid chunks prior to not rekeyed.
         */
        FBE_U32_MAX, /* First rekeyed chunk. */
        FBE_U32_MAX, /* Last rekeyed chunk. */
        0, /* Chunk to pause rekey */
        FBE_FALSE, /* Run for reboot. */
    },
    {NULL, FBE_U32_MAX, NULL, FBE_U32_MAX}, /* Terminator */
};

/*!*******************************************************************
 * @var minion_error_cases
 *********************************************************************
 * @brief
 *  This is our test set of error cases.
 *
 *********************************************************************/
minion_error_case_t *minion_error_cases[MINION_RAID_TYPE_LAST][MINION_MAX_TABLES_PER_RAID_TYPE] =
{
    {
        &minion_simple_error_cases[0],
        &minion_everything_rekeyed_error_cases[0],
        &minion_nothing_rekeyed_error_cases[0],
        &minion_ambiguous_error_cases[0],
        NULL /* Terminator */
    },
};

/*!**************************************************************
 * minion_set_options()
 ****************************************************************
 * @brief
 *  This function sets up some raid group params such as the
 *  vault wait time so that our vault encrypts quickly.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/13/2014 - Created. Rob Foley
 *
 ****************************************************************/

static void minion_set_options(void)
{
    #define VAULT_ENCRYPT_WAIT_TIME_MS 1000
    fbe_api_raid_group_class_set_options_t set_options;
    fbe_api_raid_group_class_init_options(&set_options);
    set_options.encrypt_vault_wait_time = VAULT_ENCRYPT_WAIT_TIME_MS;
    set_options.paged_metadata_io_expiration_time = 30000;
    set_options.user_io_expiration_time = 30000;
    fbe_api_raid_group_class_set_options(&set_options, FBE_CLASS_ID_PARITY);
    return;
}
/******************************************
 * end minion_set_options()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_validate_chunks_for_rekey()
 ****************************************************************
 * @brief
 *  This is a rebuild logging test.
 *  We pull out drives run a bit of I/O,
 *  and then re-insert the same drives.
 *
 * @param rg_config_p - Raid group config under test.
 * @param raid_group_count - Number of raid groups total in the list.
 * @param expected_num_nr_chunks - number of chunks we expect to
 *                                 be marked NR on each RG.
 * 
 *
 * @return None.
 *
 * @author
 *  3/29/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_validate_chunks_for_rekey(fbe_test_rg_configuration_t * const rg_config_p,
                                                 minion_error_case_t *error_case_p)
{
    fbe_status_t status;
    fbe_api_raid_group_get_paged_info_t paged_info;
    fbe_object_id_t rg_object_id;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
        fbe_api_base_config_downstream_object_list_t downstream_object_list;
        fbe_u32_t mirror_index;
        fbe_api_raid_group_get_paged_info_t mirror_paged_info;
        fbe_zero_memory(&paged_info, sizeof(paged_info));

        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
        /* For a raid 10 the downstream object list has the IDs of the mirrors. 
         * We need to map the position to specific mirror (mirror index) and a position in the mirror (position_in_mirror).
         */
        for (mirror_index = 0; 
            mirror_index < downstream_object_list.number_of_downstream_objects; 
            mirror_index++) {
            status = fbe_api_raid_group_get_paged_bits(downstream_object_list.downstream_object_list[mirror_index], 
                                                       &mirror_paged_info, FBE_TRUE    /* Get data from disk*/);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_TEST_STATUS, "== object: 0x%x first_rekey_chunk: 0x%llx last: 0x%llx num_rekey_chunks: 0x%llx",
                       downstream_object_list.downstream_object_list[mirror_index],
                       mirror_paged_info.first_rekey_chunk, mirror_paged_info.last_rekey_chunk, mirror_paged_info.num_rekey_chunks);

            if (mirror_paged_info.first_rekey_chunk != error_case_p->first_rekey_chunk){
                MUT_FAIL_MSG("First rekey chunk unexpected");
            }

            if (mirror_paged_info.last_rekey_chunk != error_case_p->last_rekey_chunk){
                MUT_FAIL_MSG("last rekey chunk unexpected");
            }
        }
    } else {
        status = fbe_api_raid_group_get_paged_bits(rg_object_id, &paged_info, FBE_TRUE    /* Get data from disk*/);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== object: 0x%x first_rekey_chunk: 0x%llx last: 0x%llx num_rekey_chunks: 0x%llx",
                   rg_object_id,
                   paged_info.first_rekey_chunk, paged_info.last_rekey_chunk, paged_info.num_rekey_chunks);

        if (paged_info.first_rekey_chunk != error_case_p->first_rekey_chunk) {
            MUT_FAIL_MSG("First rekey chunk unexpected");
        }

        if (paged_info.last_rekey_chunk != error_case_p->last_rekey_chunk) {
            MUT_FAIL_MSG("last rekey chunk unexpected");
        }
    }

   
    return;
}
/**************************************
 * end fbe_test_sep_util_validate_chunks_for_rekey()
 **************************************/
/*!*******************************************************************
 * @var minion_error_template
 *********************************************************************
 * @brief This is the basis for our error record.
 *        All the error records we create start with this template.
 *
 *********************************************************************/
fbe_api_logical_error_injection_record_t minion_error_template =
{ FBE_RAID_MAX_DISK_ARRAY_WIDTH,    /* pos to inject error on */
    0x10,    /* width */
    FBE_U32_MAX,    /* lba */
    FBE_U32_MAX,    /* max number of blocks */
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID,    /* error type */
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
 * minion_enable_logical_error_injection()
 ****************************************************************
 * @brief
 *  Enable injection across all RGs.
 *  Different RGs have different error records.
 *
 * @param record_p
 * @param rg_config_p
 * @param inject_rg_object_id_p
 *
 * @author
 *  5/13/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t minion_enable_logical_error_injection(minion_error_case_t * record_p, 
                                                          fbe_test_rg_configuration_t *rg_config_p,
                                                          fbe_object_id_t *inject_rg_object_id_p)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_record_t record;
    fbe_api_raid_group_get_info_t info;
    fbe_raid_group_map_info_t map_info;
    fbe_u32_t error_index = 0;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    /* Disable all the records.
     */
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    fbe_copy_memory(&record, &minion_error_template, sizeof(fbe_api_logical_error_injection_record_t));

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        /* Determine first paged lba.
         */
        status = fbe_api_raid_group_get_info(inject_rg_object_id_p[rg_index], &info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_zero_memory(&map_info, sizeof(fbe_raid_group_map_info_t));
        map_info.lba = (record_p->errors[error_index].chunk_index / MINION_CHUNKS_PER_PAGED_BLOCK); 

        map_info.lba += info.paged_metadata_start_lba;
        status = fbe_api_raid_group_map_lba(inject_rg_object_id_p[rg_index], &map_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        record.pos_bitmap = 1 << map_info.data_pos;
        record.lba = map_info.pba - map_info.offset;    /* Take off the addr offset since we're injecting just below RAID.*/
        record.blocks = record_p->errors[error_index].chunk_count / MINION_CHUNKS_PER_PAGED_BLOCK;
        if (record.blocks == 0) {
            record.blocks = 1;
        }
        record.err_type = record_p->errors[error_index].err_type;
        record.err_adj = record.pos_bitmap;
        if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0) {
            record.err_adj |= 1 << map_info.parity_pos;    /* Parity or mirror*/
        }
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
            record.err_adj |= 1 << (map_info.parity_pos + 1);    /* Diagonal parity*/
        }
        /* Add our own records.
         */
        status = fbe_api_logical_error_injection_create_object_record(&record, inject_rg_object_id_p[rg_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "== create record for rg_object_id: 0x%x pos_bitmap: 0x%x lba: 0x%llx blocks: 0x%llx etype: 0x%x",
                   inject_rg_object_id_p[rg_index], record.pos_bitmap, record.lba, record.blocks, record.err_type);

        if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0) {
            record.pos_bitmap = 1 << map_info.parity_pos;    /* Parity or mirror*/

            status = fbe_api_logical_error_injection_create_record(&record);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            mut_printf(MUT_LOG_TEST_STATUS, "== create parity record for rg_object_id: 0x%x pos_bitmap: 0x%x lba: 0x%llx blocks: 0x%llx etype: 0x%x",
                       inject_rg_object_id_p[rg_index], record.pos_bitmap, record.lba, record.blocks, record.err_type);
        }

        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
            record.pos_bitmap = 1 << (map_info.parity_pos + 1);    /* Diagonal parity*/

            status = fbe_api_logical_error_injection_create_record(&record);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_TEST_STATUS, "== create dparity record for rg_object_id: 0x%x pos_bitmap: 0x%x lba: 0x%llx blocks: 0x%llx etype: 0x%x",
                       inject_rg_object_id_p[rg_index], record.pos_bitmap, record.lba, record.blocks, record.err_type);
        }

        /* Enable injection on the RG.
         */
        status = fbe_api_logical_error_injection_enable_object(inject_rg_object_id_p[rg_index], FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }
    /* Enable injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Enable error injection ==");
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
/******************************************
 * end minion_enable_logical_error_injection()
 ******************************************/
/*!**************************************************************
 * minion_disable_error_injection()
 ****************************************************************
 * @brief
 *  Stop injecting errors across all RGs.
 *
 * @param rg_config_p
 * @param inject_rg_object_id_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/13/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t minion_disable_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_object_id_t *inject_rg_object_id_p)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_get_stats_t stats, *stats_p = &stats;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index;

    /* Stop logical error injection.
     */
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_logical_error_injection_disable_object(inject_rg_object_id_p[rg_index], FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    status = fbe_api_logical_error_injection_get_stats(stats_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats_p->b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats_p->num_objects_enabled, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "%s stats_p->num_objects %d, stats_p->num_objects_enabled %d", 
               __FUNCTION__, stats_p->num_objects, stats_p->num_objects_enabled);

    return status;
}
/******************************************
 * end minion_disable_error_injection()
 ******************************************/
/*!**************************************************************
 * minion_reboot_sp()
 ****************************************************************
 * @brief
 *  Reboot the SP and bring it up with appropriate scheduler hooks set.
 *
 * @param rg_config_p
 * @param error_case_p
 * @param inject_object_id_p
 *
 * @return None.
 *
 * @author
 *  5/13/2014 - Created. Rob Foley
 *
 ****************************************************************/
static void minion_reboot_sp(fbe_test_rg_configuration_t * rg_config_p,
                             minion_error_case_t *error_case_p,
                             fbe_object_id_t *inject_object_id_p)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_sep_package_load_params_t sep_params;
    fbe_neit_package_load_params_t neit_params;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u16_t data_disks;
    fbe_lba_t capacity;

    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);
    /* Setup to boot active again.
     */
    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on startup to detect that paged verify started. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        
        sep_params.scheduler_hooks[rg_index].object_id = inject_object_id_p[rg_index];
        sep_params.scheduler_hooks[rg_index].monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE;
        sep_params.scheduler_hooks[rg_index].monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START;
        sep_params.scheduler_hooks[rg_index].check_type = SCHEDULER_CHECK_STATES;
        sep_params.scheduler_hooks[rg_index].action = SCHEDULER_DEBUG_ACTION_PAUSE;
        sep_params.scheduler_hooks[rg_index].val1 = NULL;
        sep_params.scheduler_hooks[rg_index].val2 = NULL;
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_sep_util_get_raid_class_info(rg_config_p->raid_type,
                                                       rg_config_p->class_id,
                                                       rg_config_p->width,
                                                       1,    /* not used yet. */
                                                       &data_disks,
                                                       &capacity);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p->capacity = MINION_BLOCKS_PER_RG * data_disks;
        current_rg_config_p++;
    }

   /* Crash the active SP.
    */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Now we will bring up the original SP.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(minion_current_config_array, active_sp, &sep_params, &neit_params, FBE_FALSE);

    sep_config_load_kms(NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on rebooted SP for encryption complete ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for RGs to start paged verify.  Validate checkpoint.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_info(inject_object_id_p[rg_index], &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== RAID Group %d object 0x%x encryption checkpoint is at: 0x%llx",
                   current_rg_config_p->raid_group_id, inject_object_id_p[rg_index], rg_info.rekey_checkpoint);
        /* We never persisted the chkpt, so we start from 0.
         */
        if (rg_info.rekey_checkpoint != 0) {
            MUT_FAIL_MSG("Failed to find expected checkpoint");
        }
        current_rg_config_p++;
    }

    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE,
                                           1,    /* Num errors. */
                                           FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end minion_reboot_sp()
 ******************************************/
/*!**************************************************************
 * minion_corrupt_paged_during_encryption()
 ****************************************************************
 * @brief
 *  Seed a pattern.
 *  Run read only I/O during encryption.
 *  Check pattern after encryption finishes.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

static void minion_corrupt_paged_during_encryption(fbe_test_rg_configuration_t * rg_config_p,
                                                   void *err_case_p)
{
    minion_error_case_t *error_case_p = (minion_error_case_t*)err_case_p;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_lba_t first_chkpt;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t num_raid_groups;
    fbe_u32_t rg_index;
    fbe_object_id_t rg_object_id;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_u32_t substate;
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_object_id_t inject_rg_object_id[MINION_RAID_TYPE_LAST];

    /* No point to test cases where the checkpoint is at zero since 
     * the non-reboot test case is as good as the reboot test case where 
     * the checkpoint gets set back to 0. 
     */
    if (minion_should_reboot && (error_case_p->chunk_to_pause_rekey == 0)){
        return;
    }

    fbe_test_sep_get_active_target_id(&active_sp);
    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            if (downstream_object_list.number_of_downstream_objects == 0) {
                MUT_FAIL_MSG("number of ds objects is 0");
            }
            inject_rg_object_id[rg_index] = downstream_object_list.downstream_object_list[0];
        } else {
            inject_rg_object_id[rg_index] = rg_object_id;
        }
        current_rg_config_p++;
    }

    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE,
                                           1,    /* Num errors. */
                                           FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (!minion_wrote_pattern) {
        mut_printf(MUT_LOG_TEST_STATUS, "== Write and check pattern ==");
        big_bird_write_background_pattern();
        minion_wrote_pattern = FBE_TRUE;
    }
    first_chkpt = (error_case_p->chunk_to_pause_rekey) * FBE_RAID_DEFAULT_CHUNK_SIZE;
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for first pause at 0x%llx==", first_chkpt);

    /* If we are pausing at the start, make sure we pause after reconstructing paged.
     */
    if (first_chkpt == 0) {
        substate = FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START;
    } else {
        /* Otherwise just pause before we check for quiesce, since when we 
         * pause at the end before completing we need to catch it here. 
         */
        substate = FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_ENTRY;
    }
    fbe_test_sep_use_chkpt_hooks(rg_config_p, first_chkpt, 
                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                 substate,
                                 FBE_TEST_HOOK_ACTION_ADD_CURRENT);
    /* kick off a rekey.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for first pause at 0x%llx==", first_chkpt);
    fbe_test_sep_use_chkpt_hooks(rg_config_p, first_chkpt, 
                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                 substate,
                                 FBE_TEST_HOOK_ACTION_WAIT_CURRENT);

    if (minion_should_reboot) {
        /* Reboot SP. 
         * Also start it up with a hook at the beginning of paged verify. 
         */
        minion_reboot_sp(rg_config_p, error_case_p, &inject_rg_object_id[0]);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Add error injection for paged metadata.");
    minion_enable_logical_error_injection(error_case_p, rg_config_p, &inject_rg_object_id[0]);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Clear paged cache so next paged read goes to disk. ");
        fbe_api_base_config_metadata_paged_clear_cache(inject_rg_object_id[rg_index]);

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_PAGED_COMPLETE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    if (minion_should_reboot) {
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
                current_rg_config_p++;
                continue;
            }
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1,    /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                           FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            current_rg_config_p++;
        }
    } else {
        /* If our error is behind the rekey, we need to force a read of this paged metadata behind the rk checkpoint.
         */
        if ((error_case_p->errors[0].chunk_index / MINION_CHUNKS_PER_PAGED_BLOCK) !=
            (error_case_p->chunk_to_pause_rekey / MINION_CHUNKS_PER_PAGED_BLOCK)) {
            mut_printf(MUT_LOG_TEST_STATUS, "== Kick off verify to see metadata error below rekey chkpt. ");
            current_rg_config_p = rg_config_p;
            for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
                if (!fbe_test_rg_config_is_enabled(current_rg_config_p)){
                    current_rg_config_p++;
                    continue;
                }
                status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_api_raid_group_initiate_verify(rg_object_id, FBE_PACKET_FLAG_NO_ATTRIB, FBE_LUN_VERIFY_TYPE_USER_RW);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                current_rg_config_p++;
            }
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Delete hook to allow encryption to proceed eventually ==");
        fbe_test_sep_use_chkpt_hooks(rg_config_p, first_chkpt, 
                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                     substate,
                                     FBE_TEST_HOOK_ACTION_DELETE);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for hit of metadata error.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_PAGED_COMPLETE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    /* Leave error injection enabled during paged verify so the errors are detected and we are 
     * forced to reconstruct the paged. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Disable error injection now that verify is complete.");
    minion_disable_error_injection(rg_config_p, &inject_rg_object_id[0]);

    mut_printf(MUT_LOG_TEST_STATUS, "== Let paged verify continue. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_PAGED_COMPLETE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for paged verify to finish.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate paged rekey bits");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)){
            current_rg_config_p++;
            continue;
        }
        fbe_test_sep_util_validate_chunks_for_rekey(current_rg_config_p, error_case_p);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");

    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey completed successfully.");


    mut_printf(MUT_LOG_TEST_STATUS, "== check pattern ==");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    status = fbe_api_trace_clear_error_counter(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/******************************************
 * end minion_corrupt_paged_during_encryption()
 ******************************************/

/*!**************************************************************
 * minion_corrupt_paged_after_encryption()
 ****************************************************************
 * @brief
 *  Corrupt paged after encryption.
 *  Ensure that the error gets handled and object comes back to ready.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  5/13/2014 - Created. Rob Foley
 *
 ****************************************************************/

static void minion_corrupt_paged_after_encryption(fbe_test_rg_configuration_t * rg_config_p,
                                                   void *err_case_p)
{
    minion_error_case_t *error_case_p = (minion_error_case_t*)err_case_p;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t num_raid_groups;
    fbe_u32_t rg_index;
    fbe_object_id_t rg_object_id;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_object_id_t inject_rg_object_id[MINION_RAID_TYPE_LAST];

    fbe_test_sep_get_active_target_id(&active_sp);
    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            if (downstream_object_list.number_of_downstream_objects == 0) {
                MUT_FAIL_MSG("number of ds objects is 0");
            }
            inject_rg_object_id[rg_index] = downstream_object_list.downstream_object_list[0];
        } else {
            inject_rg_object_id[rg_index] = rg_object_id;
        }
        current_rg_config_p++;
    }

    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE,
                                           1,    /* Num errors. */
                                           FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add error injection for paged metadata.");
    minion_enable_logical_error_injection(error_case_p, rg_config_p, &inject_rg_object_id[0]);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Clear paged cache so next paged read goes to disk. ");
        fbe_api_base_config_metadata_paged_clear_cache(inject_rg_object_id[rg_index]);

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_PAGED_COMPLETE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

   
    /* If our error is behind the rekey, we need to force a read of this paged metadata behind the rk checkpoint.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Kick off verify to see metadata error. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_raid_group_initiate_verify(rg_object_id, FBE_PACKET_FLAG_NO_ATTRIB, FBE_LUN_VERIFY_TYPE_USER_RW);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for hit of metadata error.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_PAGED_COMPLETE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    /* Leave error injection enabled during paged verify so the errors are detected and we are 
     * forced to reconstruct the paged. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Disable error injection now that verify is complete.");
    minion_disable_error_injection(rg_config_p, &inject_rg_object_id[0]);

    mut_printf(MUT_LOG_TEST_STATUS, "== Let paged verify continue. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_PAGED_COMPLETE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for paged verify to finish.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                       FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for object: 0x%x to become ready. ", inject_rg_object_id[rg_index]);
        status = fbe_api_wait_for_object_lifecycle_state(inject_rg_object_id[rg_index], 
                                                         FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    status = fbe_api_trace_clear_error_counter(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/******************************************
 * end minion_corrupt_paged_after_encryption()
 ******************************************/

fbe_u32_t minion_get_table_case_count(minion_error_case_t *error_case_p)
{
    fbe_u32_t test_case_index = 0;

    /* Loop over all the cases in this table until we hit a terminator.
     */
    while (error_case_p[test_case_index].description_p != NULL)
    {
        test_case_index++;
    }
    return test_case_index;
}
fbe_u32_t minion_get_test_case_count(minion_error_case_t *error_case_p[])
{
    fbe_u32_t test_case_count = 0;
    fbe_u32_t table_index = 0;
    fbe_u32_t test_case_index;

    /* Loop over all the tables until we hit a terminator.
     */
    while (error_case_p[table_index] != NULL)
    {
        test_case_index = 0;
        /* Add the count for this table.
         */
        test_case_count += minion_get_table_case_count(&error_case_p[table_index][0]);
        table_index++;
    }
    return test_case_count;
}
/*!**************************************************************
 * minion_test_rg_config()
 ****************************************************************
 * @brief
 *  Test paged metadata errors during encryption.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void minion_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    minion_error_case_t **error_case_p = NULL;
    fbe_u32_t test_index = 0;
    fbe_u32_t table_test_index;
    fbe_u32_t total_test_case_count;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t start_table_index = 0;
    fbe_u32_t table_index = 0;
    fbe_u32_t start_test_index = 0;
    fbe_u32_t table_test_count;
    fbe_u32_t max_case_count = FBE_U32_MAX;
    fbe_u32_t test_case_executed_count = 0;
    fbe_u32_t repeat_count = 1;
    fbe_u32_t current_iteration = 0;
    fbe_bool_t b_start_index;
    fbe_u32_t extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* The user can optionally choose a table index or test_index to start at.
     * By default we continue from this point to the end. 
     */
    fbe_test_sep_util_get_cmd_option_int("-start_table", &start_table_index);
    b_start_index = fbe_test_sep_util_get_cmd_option_int("-start_index", &start_test_index);
    fbe_test_sep_util_get_cmd_option_int("-total_cases", &max_case_count);

    error_case_p = (minion_error_case_t**)context_p;
    total_test_case_count = minion_get_test_case_count(error_case_p);

    mut_printf(MUT_LOG_TEST_STATUS, "minion start_table: %d start_index: %d total_cases: %d",start_table_index, start_test_index, max_case_count);

    fbe_test_sep_util_get_raid_type_string(rg_config_p[0].raid_type, &raid_type_string_p);

    /* Loop over all the tests for this raid type.
     */
    while (error_case_p[table_index] != NULL) {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Starting table %d for raid type %s", 
                   __FUNCTION__, table_index, raid_type_string_p);

        /* If we have not yet reached the start index, get the next table.
         */
        if (table_index < start_table_index) {
            table_index++;
            continue;
        }
        table_test_index = 0;
        table_test_count = minion_get_table_case_count(&error_case_p[table_index][0]);

        /* Loop until we hit a terminator.
         */ 
        while (error_case_p[table_index][table_test_index].description_p != NULL) {
            fbe_time_t start_time = fbe_get_time();
            fbe_time_t time;
            fbe_time_t msec_duration;
            minion_error_case_t *current_error_case_p = &error_case_p[table_index][table_test_index];

            
            if (start_test_index == current_error_case_p->line) {
                /* Found a match.  We allow the start_test_index to also match
                 *                 a line number in a test case. 
                 */
                start_test_index = 0;
            } else if (table_test_index < start_test_index) {
                /* If we have not yet reached the start index, get the next test case.
                 */ 
                table_test_index++;
                test_index++;
                continue;
            }
            /* If this test case should not run during this test, then skip it.
             */
            if ((minion_should_reboot && !current_error_case_p->b_run_reboot) || 
                (!minion_should_reboot && current_error_case_p->b_run_reboot)) {

                /* If we have not yet reached the start index, get the next test case.
                 */ 
                table_test_index++;
                test_index++;
                continue;
            }

            /* If the start index was not provided,
             * and we're not the reboot test, then we will 
             * use either even or odd test cases to reduce the test time. 
             */
            if (extended_testing_level < 2 &&
                !minion_should_reboot && !b_start_index) {
                if (extended_testing_level) {
                    if ( (minion_run_even_test_cases && ((test_index % 2) != 0)) ||
                         (!minion_run_even_test_cases && ((test_index % 2) == 0)) ) {
                        mut_printf(MUT_LOG_TEST_STATUS, "== Skip test case %u only running %s cases",
                                   test_index, (minion_run_even_test_cases) ? "even" : "odd");
                        table_test_index++;
                        test_index++;
                        continue;
                    }
                }
            }
            current_iteration = 0;
            repeat_count = 1;
            fbe_test_sep_util_get_cmd_option_int("-repeat_case_count", &repeat_count);
            while (current_iteration < repeat_count) {
                const fbe_char_t *raid_type_string_p = NULL;
                mut_printf(MUT_LOG_TEST_STATUS, "\n\n");
                mut_printf(MUT_LOG_TEST_STATUS, "test_case: %s", current_error_case_p->description_p); 
                fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
                mut_printf(MUT_LOG_TEST_STATUS, "starting table case: (%u of %u) overall: (%u of %u) line: %u for raid type %s (0x%x)",
                           table_test_index, table_test_count,
                           test_index, total_test_case_count,  
                           current_error_case_p->line, raid_type_string_p, rg_config_p->raid_type);

                /* Run this test case for all raid groups.
                 */
                (current_error_case_p->function_p)(rg_config_p, current_error_case_p);

                mut_printf(MUT_LOG_TEST_STATUS, "== Delete encryption completing hooks ==");

                current_iteration++;
                if (repeat_count > 1) {
                    mut_printf(MUT_LOG_TEST_STATUS, "minion completed test case iteration [%d of %d]", current_iteration, repeat_count);
                }
            }
            time = fbe_get_time();
            msec_duration = (time - start_time);
            mut_printf(MUT_LOG_TEST_STATUS,
                       "table %d test %d took %llu seconds (%llu msec)",
                       table_index, table_test_index,
                       (unsigned long long)(msec_duration / FBE_TIME_MILLISECONDS_PER_SECOND),
                       (unsigned long long)msec_duration);
            table_test_index++;
            test_index++;
            test_case_executed_count++;

            /* If the user chose a limit on the max test cases and we are at the limit, just return. 
             */  
            if (test_case_executed_count >= max_case_count) {
                return;
            }
        }    /* End loop over all tests in this table. */

        /* After the first test case reset start_test_index, so for all the remaining tables we will 
         * start at the beginning. 
         */
        start_test_index = 0;
        table_index++;
    }    /* End loop over all tables. */

    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end minion_test_rg_config()
 ******************************************/

/*!**************************************************************
 * minion_run_test()
 ****************************************************************
 * @brief
 *  Run the minion test.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void minion_run_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &minion_raid_group_config[0][0];
    minion_current_config_array = &minion_raid_group_config[0];

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         minion_error_cases[0],
                                                         minion_test_rg_config,
                                                         MINION_TEST_LUNS_PER_RAID_GROUP,
                                                         MINION_CHUNKS_PER_LUN,
                                                         FBE_TRUE /* don't destroy config */);
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    return;
}
/******************************************
 * end minion_run_test()
 ******************************************/

/*!**************************************************************
 * minion_setup_rg_config()
 ****************************************************************
 * @brief
 *  Initialize our rg config for this large rg encryption test.
 *
 * @param  rg_config_p - our config to test with.               
 *
 * @return None.
 *
 * @author
 *  5/13/2014 - Created. Rob Foley
 *
 ****************************************************************/

void minion_setup_rg_config(fbe_test_rg_configuration_t *rg_config_p)
{
    /* We no longer create the raid groups during the setup
     */
    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    fbe_test_rg_setup_random_block_sizes(rg_config_p);
    fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

    switch (rg_config_p[0].raid_type) {
        case FBE_RAID_GROUP_TYPE_RAID3:
            rg_config_p[0].width = 5;
            rg_config_p[0].capacity = 4 * MINION_BLOCKS_PER_RG;
            break;
        case FBE_RAID_GROUP_TYPE_RAID5:
            rg_config_p[0].width = 3;
            rg_config_p[0].capacity = 2 * MINION_BLOCKS_PER_RG;
            break;
        case FBE_RAID_GROUP_TYPE_RAID6:
            rg_config_p[0].width = 4;
            rg_config_p[0].capacity = 2 * MINION_BLOCKS_PER_RG;
            break;
        default:
            /* Just allow this case to pass.  No need to change it.
             */
            break;
    };

    fbe_test_sep_util_update_extra_chunk_size(rg_config_p, MINION_EXTRA_CHUNKS);
}
/******************************************
 * end minion_setup_rg_config()
 ******************************************/

/*!**************************************************************
 * minion_setup()
 ****************************************************************
 * @brief
 *  Setup for an encryption test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/
void minion_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    /* This will automatically setup the seed if it has not already been set.
     */
    fbe_test_init_random_seed();

    /* Avoid the mod.  Half the time we will be even.
     */
    minion_run_even_test_cases = fbe_random() > (FBE_U32_MAX / 2);
    mut_printf(MUT_LOG_TEST_STATUS, "== Only run %s test cases", (minion_run_even_test_cases) ? "even" : "odd");

    /* We will randomize the first entry, a parity raid group and pick one of RAID-5, RAID-3, RAID-6.
     */
    switch (fbe_random() % 3) {
        case 0:
            minion_current_raid_type = FBE_RAID_GROUP_TYPE_RAID3;
            break;
        case 1:
            minion_current_raid_type = FBE_RAID_GROUP_TYPE_RAID6;
            break;
        default:
            minion_current_raid_type = FBE_RAID_GROUP_TYPE_RAID5;
            break;
    };
    minion_raid_group_config[0]->raid_type = minion_current_raid_type;
    mut_printf(MUT_LOG_TEST_STATUS, "starting %s using raid type: %d", __FUNCTION__, minion_current_raid_type);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();
    
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &minion_raid_group_config[0][0]; 
        array_p = &minion_raid_group_config[0];

        minion_setup_rg_config(rg_config_p);

        /* Use simulator PSL so that it does not take forever and a day to zero the system drives.
         */
        fbe_test_set_use_fbe_sim_psl(FBE_TRUE);

        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
        elmo_load_sep_and_neit();
    }
    minion_set_options();
    /* We inject errors on purpose, so we do not want to stop on any sector trace errors.
     */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_test_disable_background_ops_system_drives();
    
    /* load KMS */
    sep_config_load_kms(NULL);
    fbe_test_set_rg_vault_encrypt_wait_time(1000);
    /* It can take a few minutes for the vault to encrypt itself. 
     * So waits for hooks, etc might need to be longer. 
     */
    fbe_test_set_timeout_ms(10 * 60000);

    return;
}
/**************************************
 * end minion_setup()
 **************************************/

void minion_test(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "starting %s", __FUNCTION__);
    /* We pick a random type in setup and run the test on this type.
     */
    minion_should_reboot = FBE_FALSE;

    mut_printf(MUT_LOG_TEST_STATUS, "== Only run %s test cases", (minion_run_even_test_cases) ? "even" : "odd");
    minion_run_test();
}
void minion_reboot_test(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "starting %s", __FUNCTION__);
    minion_should_reboot = FBE_TRUE;
    /* We pick a random type in setup and run the test on this type.
     */
    minion_run_test();
}

/*!**************************************************************
 * minion_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the minion test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void minion_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {		
        fbe_test_sep_util_destroy_kms();
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end minion_cleanup()
 ******************************************/


/*************************
 * end file minion_test.c
 *************************/


