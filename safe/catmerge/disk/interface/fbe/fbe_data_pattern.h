#ifndef FBE_DATA_PATERN_H
#define FBE_DATA_PATERN_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_data_pattern.h
 ***************************************************************************
 *
 * @brief
 *   This header file contains structures and definitions needed by the
 *   data pattern library.
 *  This functionality is separated out from rdgen sglist . 
 *  implementation. Since now the user of this functionality are 
 *  FBECLI rdt, fbe_test along with rdgen service.
 *
 * NOTES:
 *
 * @revision
 *   08/30/10: Swati Fursule -- Created.
 *
 *
 ***************************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_sg_element.h"
#include "fbe/fbe_xor_api.h"
#include "xorlib_api.h"
#include "fbe/fbe_trace_interface.h"


/*********************************
 * EXTERNAL REFERENCE DEFINITIONS
 *********************************/

/*******************************
 * LITERAL DEFINITIONS
 *******************************/

/*!*******************************************************************
 * @def DATA_PATTERN_COND
 *********************************************************************
 * @brief We define this as the default condition handler.
 *        We overload this so that in test code we can
 *        test when these conditions are false.
 * 
 *        The naming is intentionally short (omitting FBE)
 *        so that we can keep some conditionals on one line.
 *
 *********************************************************************/
#define DATA_PATTERN_COND(m_cond) (m_cond)


/*!*******************************************************************
 * @def FBE_DATA_PATTERN_MAX_TRACE_CHARS
 *********************************************************************
 * @brief max number of chars we will put in a formatted trace string.
 *
 *********************************************************************/
#define FBE_DATA_PATTERN_MAX_TRACE_CHARS 128



/* These define some default params for our trace functions. 
 * This makes is much easier to maintain if we need to change 
 * these default params for any reason. 
 */
#define FBE_DATA_PATTERN_LIBRARY_TRACE_PARAMS_CRITICAL FBE_TRACE_LEVEL_CRITICAL_ERROR, __LINE__, __FUNCTION__
#define FBE_DATA_PATTERN_LIBRARY_TRACE_PARAMS_ERROR FBE_TRACE_LEVEL_ERROR, __LINE__, __FUNCTION__
#define FBE_DATA_PATTERN_LIBRARY_TRACE_PARAMS_DEBUG_HIGH FBE_TRACE_LEVEL_DEBUG_HIGH, __LINE__, __FUNCTION__
#define FBE_DATA_PATTERN_LIBRARY_TRACE_PARAMS_INFO FBE_TRACE_LEVEL_INFO, __LINE__, __FUNCTION__
#define FBE_DATA_PATTERN_LIBRARY_TRACE_PARAMS_WARNING FBE_TRACE_LEVEL_INFO, __LINE__, __FUNCTION__

/***************************************************
 * In some cases we may want to enable/disable
 * validation of the number of map-ins, map-outs
 * that the XOR driver does.
 *
 * if DATA_PATTERN_MAPPING_VALIDATION is defined as 1
 *   then we will count the numbers of map ins
 *   and fbe_assert when we fail to unmap appropriately.
 *
 * else if DATA_PATTERN_MAPPING_VALIDATION is not defined
 *        then we will not count or validate the
 *        numbers of map ins.
 *
 ***************************************************/

#if DBG || defined(LAB_DEBUG) || defined(UMODE_ENV) || defined(SIMMODE_ENV)

//#define DATA_PATTERN_MAPPING_VALIDATION 1

#endif

/*!*******************************************************************
 * @def FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG
 *********************************************************************
 * @brief
 *   This is the maximum blocks per SG.
 *   We use 128, since historically this is what flare uses.
 *********************************************************************/
#define FBE_DATA_PATTERN_MAX_BLOCKS_PER_SG 32

/*!*******************************************************************
 * @def FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS
 *********************************************************************
 * @brief
 *   This is the largest sg we will allocate.
 *   We use 128, since historically this is what flare uses.
 *********************************************************************/
#define FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS 128

/*!**************************************************
 * @def FBE_DATA_PATTERN_HEADER_PATTERN
 ***************************************************
 * @brief This pattern appears at the head of all
 * sectors generated.
 ***************************************************/
#define FBE_DATA_PATTERN_HEADER_PATTERN 0x3CC3

/*!**************************************************
 * @def FBE_DATA_PATTERN_MAX_HEADER_PATTERN
 ***************************************************
 * @brief This pattern appears at the maximum header 
 * entries in header array.
 ***************************************************/
#define FBE_DATA_PATTERN_MAX_HEADER_PATTERN 5

/*!**************************************************
 * @def FBE_DATA_PATTERN_MAP_MEMORY
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 ***************************************************/
#define FBE_DATA_PATTERN_MAP_MEMORY(memory_p, byte_count)((void*)0)

/*!**************************************************
 * @def FBE_DATA_PATTERN_UNMAP_MEMORY
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 ***************************************************/
#define FBE_DATA_PATTERN_UNMAP_MEMORY(memory_p, byte_count)((void)0)

/*!**************************************************
 * @def FBE_DATA_PATTERN_VIRTUAL_ADDRESS_AVAIL
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 *   For now everything is virtual.
 ***************************************************/
#define FBE_DATA_PATTERN_VIRTUAL_ADDRESS_AVAIL(sg_ptr)(FBE_TRUE)

/*!**************************************************
 * @def FBE_DATA_PATTERN_MAX_SEED_VALUE
 ***************************************************
 * @brief
 *   This is the maximum supported seed (lba) value.
 *   Currently the sequence id is 16-bits therefore
 *   the max is a 48-bit seed.
 ***************************************************/
#define FBE_DATA_PATTERN_MAX_SEED_VALUE ((fbe_lba_t)0x0000FFFFFFFFFFFF)

/*!*******************************************************************
 * @enum fbe_data_pattern_t
 *********************************************************************
 * @brief
 *  Pattern to be used when writing.
 *********************************************************************/
typedef enum fbe_data_pattern_e
{
    FBE_DATA_PATTERN_INVALID = 0, /*!< Always first. */
    FBE_DATA_PATTERN_ZEROS,       /*!< Zeros but with checksum set. */
    FBE_DATA_PATTERN_LBA_PASS,    /*!< Lba seed plus pass count. */
    FBE_DATA_PATTERN_CLEAR,       /*!< Zeros including meta-data. */
    FBE_DATA_PATTERN_INVALIDATED, /*!< Properly invalidated block.  */
    FBE_DATA_PATTERN_LAST         /*!< Always last. */
}
fbe_data_pattern_t;

/*!*******************************************************************
 * @enum    fbe_data_pattern_flags_e
 *********************************************************************
 * @brief
 *  Special flags that give additional information about the data
 *  being sent/checked.
 *********************************************************************/
typedef enum fbe_data_pattern_flags_e
{
    FBE_DATA_PATTERN_FLAGS_INVALID      = 0x00000000,   /*!< Always first. */
    FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC  = 0x00000001,   /*!< This request contains one or more `Corrupt CRC' sectors */
    FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA = 0x00000002,   /*!< This request contains one or more `Corrupt Data' sectors */
    FBE_DATA_PATTERN_FLAGS_RAID_VERIFY  = 0x00000004,   /*!< Expected invalidation reason is `RAID Verify'. */
    FBE_DATA_PATTERN_FLAGS_DATA_LOST    = 0x00000008,   /*!< Expected invalidation reason is `Data Lost - Invalidated'. */
    FBE_DATA_PATTERN_FLAGS_DO_NOT_CHECK_INVALIDATION_REASON = 0x00000010, /*!< Do not validate that the invalidation reason is valid. */
    FBE_DATA_PATTERN_FLAGS_VALID_AND_INVALID_DATA = 0x00000020,  /*!< We might have valid and invalid data.  So check block by block. */
    FBE_DATA_PATTERN_FLAGS_CHECK_CRC_ONLY         = 0x000000040, /*!< Only check crc. */
    FBE_DATA_PATTERN_FLAGS_CHECK_LBA_STAMP_ONLY   = 0x000000080, /*!< Only check lba stamp. */
    FBE_DATA_PATTERN_FLAGS_LAST         /*!< Always last. */
} fbe_data_pattern_flags_t;

/*!************************************************** 
 *  @struct fbe_data_pattern_info_t 
 *
 *  @brief This is the data pattern information structure
 *   which is used for the fill and check sg and sector.
 *
 ***************************************************/
typedef struct fbe_data_pattern_info_s
{
    /*! data pattern which is used for filling up the words in 
     *  sectors
     */
    fbe_data_pattern_t  pattern;
    /*! Data pattern flags that describe data pattern request 
     */
    fbe_data_pattern_flags_t pattern_flags;
    /*! LBA value
     */
    fbe_lba_t           seed;
    /*! `sequence identifier' used for debugging purpose. Fixed portion of seed.
     */
    fbe_u32_t           sequence_id;
    fbe_u32_t           pass_count;
    /*! Number of header words
     */
    fbe_u32_t           num_header_words;
    fbe_block_count_t num_sg_blocks; /*! Number of blocks in sg to consider. Ignore if 0. */
    /*!< Start of the transfer being checked. different from seed since seed remains 0 for some patterns.  */
    fbe_lba_t           start_lba; 
    /*! Header array
     */
    fbe_u64_t           header_array[FBE_DATA_PATTERN_MAX_HEADER_PATTERN];
}fbe_data_pattern_info_t;

/*!*******************************************************************
 * @enum fbe_data_pattern_sp_id_t
 *********************************************************************
 * @brief  Private enum of SP identifiers (hides a specific abstractions)
 *
 *********************************************************************/
typedef enum fbe_data_pattern_sp_id_e
{
    FBE_DATA_PATTERN_SP_ID_INVALID = 0x0,
    FBE_DATA_PATTERN_SP_ID_A       = 0xA,
    FBE_DATA_PATTERN_SP_ID_B       = 0xB,
    FBE_DATA_PATTERN_SP_ID_LAST
} 
fbe_data_pattern_sp_id_t;

/*!*******************************************************************
 * @struct fbe_data_pattern_region_list_t
 *********************************************************************
 * @brief This is a list of regions that have a set pattern.
 *
 *********************************************************************/
typedef struct fbe_data_pattern_region_list_s
{
    fbe_lba_t lba; /*!< Logical block address for start of region. */
    fbe_block_count_t blocks; /*!< Number of blocks in region. */
    fbe_data_pattern_t pattern; /*!< Pattern of region */
    fbe_u32_t pass_count; /*!< Expected pass count */
    fbe_data_pattern_sp_id_t sp_id; /*!< SP that wrote this. */ 
}
fbe_data_pattern_region_list_t;
fbe_status_t fbe_data_pattern_region_list_validate(fbe_data_pattern_region_list_t *list_p,
                                                   fbe_u32_t list_length);
fbe_status_t fbe_data_pattern_region_list_validate_pair(fbe_data_pattern_region_list_t *src_list_p,
                                                        fbe_u32_t src_list_length,
                                                        fbe_data_pattern_region_list_t *dst_list_p,
                                                        fbe_u32_t dst_list_length);
fbe_status_t fbe_data_pattern_region_list_copy_sp_id(fbe_data_pattern_region_list_t *src_list_p,
                                                     fbe_u32_t src_list_length,
                                                     fbe_data_pattern_region_list_t *dst_list_p,
                                                     fbe_u32_t dst_list_length);
fbe_status_t fbe_data_pattern_region_list_copy_pattern(fbe_data_pattern_region_list_t *src_list_p,
                                                       fbe_u32_t src_list_length,
                                                       fbe_data_pattern_region_list_t *dst_list_p,
                                                       fbe_u32_t *dst_list_length_p,
                                                       fbe_data_pattern_t pattern,
                                                       fbe_bool_t b_pass_count_exclude,
                                                       fbe_u32_t pass_count);
fbe_status_t fbe_data_pattern_create_region_list(fbe_data_pattern_region_list_t *list_p,
                                                 fbe_u32_t *list_length_p,
                                                 fbe_u32_t max_list_length,
                                                 fbe_lba_t start_lba,
                                                 fbe_block_count_t max_blocks,
                                                 fbe_lba_t capacity,
                                                 fbe_data_pattern_t pattern);
fbe_status_t fbe_data_pattern_region_list_set_random_pattern(fbe_data_pattern_region_list_t *list_p,
                                                             fbe_u32_t max_list_length,
                                                             fbe_data_pattern_t pattern,
                                                             fbe_data_pattern_t background_pattern,
                                                             fbe_u32_t background_pass_count,
                                                             fbe_data_pattern_sp_id_t background_sp_id);
/*!**************************************************
 *  fbe_data_pattern_update_seed()
 *
 *  @brief This is used to build data pattern information 
 *  structure which is used for the fill and check sg and sector.
 *
 ***************************************************/
static __forceinline fbe_status_t 
fbe_data_pattern_update_seed(fbe_data_pattern_info_t *data_pattern_info_p,
                             fbe_lba_t new_seed)
{
    data_pattern_info_p->seed = new_seed;
    return FBE_STATUS_OK;
}

/*!**************************************************
 *  fbe_data_pattern_build_info() 
 *
 *  @brief This is used to build data pattern information 
 *  structure which is used for the fill and check sg and sector.
 *
 ***************************************************/
static __forceinline fbe_status_t 
fbe_data_pattern_build_info(fbe_data_pattern_info_t *data_pattern_info_p,
                                               fbe_data_pattern_t pattern,
                                               fbe_data_pattern_flags_t pattern_flags,
                                               fbe_lba_t seed,
                                               fbe_u32_t sequence_id,
                                               fbe_u32_t num_header_words,
                                               fbe_u64_t *header_array_p)
{
    fbe_u8_t header_index;
    data_pattern_info_p->pattern = pattern;
    data_pattern_info_p->pattern_flags = pattern_flags;
    data_pattern_info_p->seed = seed;
    data_pattern_info_p->sequence_id = sequence_id;
    data_pattern_info_p->num_header_words = num_header_words;
    data_pattern_info_p->num_sg_blocks = 0;  /* By default we ignore if 0. */
    data_pattern_info_p->start_lba = seed;

    for(header_index=0; header_index<data_pattern_info_p->num_header_words;
        header_index++)
    {
        data_pattern_info_p->header_array[header_index] = header_array_p[header_index];
    }
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t 
fbe_data_pattern_set_sg_blocks(fbe_data_pattern_info_t *data_pattern_info_p,
                               fbe_block_count_t blocks)
{
    data_pattern_info_p->num_sg_blocks = blocks;
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t 
fbe_data_pattern_set_current_lba(fbe_data_pattern_info_t *data_pattern_info_p,
                                 fbe_lba_t lba)
{
    data_pattern_info_p->start_lba = lba;
    return FBE_STATUS_OK;
}

/* fbe_data_pattern_print.c*/
void fbe_data_pattern_trace(fbe_trace_level_t trace_level,
                            fbe_trace_message_id_t message_id,
                            const char * fmt, ...)
                            __attribute__((format(__printf_func__,3,4)));

/* fbe_data_pattern_sg.c*/
fbe_status_t fbe_data_pattern_sg_fill_with_memory(fbe_sg_element_t *sg_p,
                                                  fbe_u8_t *memory_ptr,
                                                  fbe_block_count_t blocks,
                                                  fbe_block_size_t block_size,
                                                  fbe_u32_t max_sg_data_elements,
                                                  fbe_u32_t max_blocks_per_sg);

fbe_status_t fbe_data_pattern_fill_sg_list(fbe_sg_element_t * sg_ptr,
                                           fbe_data_pattern_info_t* data_pattern_info_p,
                                           fbe_block_size_t block_size,
                                           fbe_u64_t corrupt_bitmap,
                                           fbe_u32_t max_sg_data_elements);

fbe_status_t fbe_data_pattern_check_sg_list(fbe_sg_element_t * sg_ptr,
                                            fbe_data_pattern_info_t* data_pattern_info_p,
                                            fbe_block_size_t block_size,
                                            fbe_u64_t corrupt_bitmap,
                                            fbe_object_id_t object_id,
                                            fbe_u32_t max_sg_data_elements,
                                            fbe_bool_t b_panic);


/* fbe_data_pattern_sector.c*/
fbe_bool_t fbe_data_pattern_fill_sector(fbe_u32_t sectors,
                                        fbe_data_pattern_info_t* data_pattern_info_p,
                                        fbe_u32_t * memory_ptr,
                                        fbe_bool_t b_append_checksum,
                                        fbe_block_size_t block_size,
                                        const fbe_bool_t b_is_virtual);

fbe_status_t fbe_data_pattern_trace_sector(const fbe_sector_t *const sector_p);

fbe_status_t fbe_data_pattern_check_sector(fbe_u32_t sectors,
                                           fbe_data_pattern_info_t* data_pattern_info_p,
                                           fbe_u32_t * memory_ptr,
                                           const fbe_bool_t b_is_virtual,
                                           fbe_block_size_t block_size,
                                           fbe_object_id_t object_id,
                                           fbe_bool_t b_panic);

fbe_status_t fbe_data_pattern_check_for_invalidated_sector(fbe_u32_t sectors,
                                                           fbe_data_pattern_info_t* data_pattern_info_p,
                                                           fbe_u32_t * memory_ptr,
                                                           fbe_u64_t corrupt_bitmap,
                                                           const fbe_bool_t b_is_virtual,
                                                           fbe_object_id_t object_id,
                                                           fbe_bool_t b_panic);

fbe_status_t fbe_data_pattern_fill_invalidated_sector(fbe_u32_t sectors,
                                                      fbe_data_pattern_info_t* data_pattern_info_p,
                                                      fbe_u32_t * memory_ptr,
                                                      const fbe_bool_t b_is_virtual,
                                                      fbe_u64_t corrupt_bitmap,
                                                      xorlib_sector_invalid_reason_t reason);

/*************************
 * end file fbe_data_pattern.h
 *************************/
#endif /* FBE_DATA_PATERN_H */
