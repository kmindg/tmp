#ifndef __FBE_RAID_POOL_ENTRIES_H__
#define __FBE_RAID_POOL_ENTRIES_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_pool_entries.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the structures that are allocated
 *  from memory pools for raid.
 *
 * @version
 *   8/5/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_memory_api.h"
#include "fbe_raid_library_private.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @struct fbe_raid_fruts_pool_entry_t
 *********************************************************************
 * @brief This is the type of structure we allocate from the fruts pool.
 *
 *********************************************************************/
typedef struct fbe_raid_fruts_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_raid_fruts_t fruts;
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_fruts_pool_entry_t;

/*!*******************************************************************
 * @struct fbe_raid_siots_pool_entry_t
 *********************************************************************
 * @brief This is the type of structure we allocate from the siots pool.
 *
 *********************************************************************/
typedef struct fbe_raid_siots_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_raid_siots_t siots;
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_siots_pool_entry_t;

/*!*******************************************************************
 * @struct fbe_raid_vcts_pool_entry_t
 *********************************************************************
 * @brief This is the type of structure we allocate from the vcts pool.
 *
 *********************************************************************/
typedef struct fbe_raid_vcts_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_raid_vcts_t vcts;
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_vcts_pool_entry_t;

/*!*******************************************************************
 * @struct fbe_raid_buffer_pool_entry_t
 *********************************************************************
 * @brief This is the type of structure we allocate from the buffer pool.
 *
 *********************************************************************/
typedef struct fbe_raid_buffer_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_u8_t buffer[FBE_RAID_PAGE_SIZE_STD * FBE_BE_BYTES_PER_BLOCK];
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_buffer_pool_entry_t;

/*!*******************************************************************
 * @struct fbe_raid_buffer_2kb_pool_entry_t
 *********************************************************************
 * @brief This is the type of structure we allocate from the buffer pool.
 *
 *********************************************************************/
typedef struct fbe_raid_buffer_2kb_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_u8_t buffer[FBE_RAID_PAGE_SIZE_MIN * FBE_BE_BYTES_PER_BLOCK];
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_buffer_2kb_pool_entry_t;

/*!*******************************************************************
 * @struct fbe_raid_buffer_16kb_pool_entry_t
 *********************************************************************
 * @brief This is the type of structure we allocate from the buffer pool.
 *
 *********************************************************************/
typedef struct fbe_raid_buffer_16kb_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_u8_t buffer[FBE_RAID_PAGE_SIZE_STD * FBE_BE_BYTES_PER_BLOCK];
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_buffer_16kb_pool_entry_t;

/*!*******************************************************************
 * @struct fbe_raid_buffer_32kb_pool_entry_t
 *********************************************************************
 * @brief This is the type of 32kb structure we allocate from the buffer pool.
 *
 *********************************************************************/
typedef struct fbe_raid_buffer_32kb_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_u8_t buffer[FBE_RAID_PAGE_SIZE_MAX * FBE_BE_BYTES_PER_BLOCK];
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_buffer_32kb_pool_entry_t;

/*!*******************************************************************
 *  @def FBE_RAID_CALCULATE_TOTAL_SG_ENTRIES
 *********************************************************************
 * @brief Calculates the total number of sg entries needed for
 *        an sg list that has the input number of data entries.
 *        We simply add one for the terminator.
 *
 *********************************************************************/
#define FBE_RAID_CALCULATE_TOTAL_SG_ENTRIES(m_data_entries)\
(m_data_entries + 1)

/*!*******************************************************************
 * @def FBE_RAID_SG_1_DATA_ENTRIES
 *********************************************************************
 * @brief This is the number of data sg elements in an sg 1.
 *
 *********************************************************************/
#define FBE_RAID_SG_1_DATA_ENTRIES 1

/*!*******************************************************************
 * @def FBE_RAID_SG_1_ENTRIES
 *********************************************************************
 * @brief This is the number of total sg elements in an sg 1.
 *
 *********************************************************************/
#define FBE_RAID_SG_1_ENTRIES FBE_RAID_CALCULATE_TOTAL_SG_ENTRIES(FBE_RAID_SG_1_DATA_ENTRIES)

/*!*******************************************************************
 * @struct fbe_raid_sg_1_pool_entry_t
 *********************************************************************
 * @brief This is the type of structure we allocate for an sg 1.
 *
 *********************************************************************/
typedef struct fbe_raid_sg_1_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_sg_element_t sg[FBE_RAID_SG_1_ENTRIES];
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_sg_1_pool_entry_t;

/*!*******************************************************************
 * @def FBE_RAID_SG_8_DATA_ENTRIES
 *********************************************************************
 * @brief This is the number of data sg elements in an sg 8.
 *
 *********************************************************************/
#define FBE_RAID_SG_8_DATA_ENTRIES 8

/*!*******************************************************************
 * @def FBE_RAID_SG_8_ENTRIES
 *********************************************************************
 * @brief This is the number of total sg elements in an sg 8.
 *
 *********************************************************************/
#define FBE_RAID_SG_8_ENTRIES FBE_RAID_CALCULATE_TOTAL_SG_ENTRIES(FBE_RAID_SG_8_DATA_ENTRIES)

/*!*******************************************************************
 * @struct fbe_raid_sg_8_pool_entry_t
 *********************************************************************
 * @brief This is the type of structure we allocate for an sg 8.
 *
 *********************************************************************/
typedef struct fbe_raid_sg_8_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_sg_element_t sg[FBE_RAID_SG_8_ENTRIES];
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_sg_8_pool_entry_t;

/*!*******************************************************************
 * @def FBE_RAID_SG_32_DATA_ENTRIES
 *********************************************************************
 * @brief This is the number of data sg elements in an sg 32.
 *
 *********************************************************************/
#define FBE_RAID_SG_32_DATA_ENTRIES 32

/*!*******************************************************************
 * @def FBE_RAID_SG_32_ENTRIES
 *********************************************************************
 * @brief This is the number of total sg elements in an sg 32.
 *
 *********************************************************************/
#define FBE_RAID_SG_32_ENTRIES FBE_RAID_CALCULATE_TOTAL_SG_ENTRIES(FBE_RAID_SG_32_DATA_ENTRIES)

/*!*******************************************************************
 * @struct fbe_raid_sg_32_pool_entry_t
 *********************************************************************
 * @brief This is the type of structure we allocate for an sg 32.
 *
 *********************************************************************/
typedef struct fbe_raid_sg_32_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_sg_element_t sg[FBE_RAID_SG_32_ENTRIES];
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_sg_32_pool_entry_t;

/*!*******************************************************************
 * @def FBE_RAID_SG_128_DATA_ENTRIES
 *********************************************************************
 * @brief This is the number of data sg elements in an sg 128.
 *
 *********************************************************************/
#define FBE_RAID_SG_128_DATA_ENTRIES 128

/*!*******************************************************************
 * @def FBE_RAID_SG_128_ENTRIES
 *********************************************************************
 * @brief This is the number of total sg elements in an sg 128.
 *
 *********************************************************************/
#define FBE_RAID_SG_128_ENTRIES FBE_RAID_CALCULATE_TOTAL_SG_ENTRIES(FBE_RAID_SG_128_DATA_ENTRIES)

/*!*******************************************************************
 * @struct fbe_raid_sg_128_pool_entry_t
 *********************************************************************
 * @brief This is the type of structure we allocate for an sg 128.
 *
 *********************************************************************/
typedef struct fbe_raid_sg_128_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_sg_element_t sg[FBE_RAID_SG_128_ENTRIES];
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_sg_128_pool_entry_t;

/*!*******************************************************************
 * @def FBE_RAID_SG_HEADER_OVERHEAD
 *********************************************************************
 * @brief The overhead in the sg header is equivalent to
 *        an sg 1 minus the extra sg.
 *
 *********************************************************************/
#define FBE_RAID_SG_HEADER_OVERHEAD (sizeof(fbe_raid_sg_1_pool_entry_t) - sizeof(fbe_sg_element_t))

/*!*******************************************************************
 * @def FBE_RAID_SG_MAX_DATA_ENTRIES
 *********************************************************************
 * @brief This is the number of data sg elements in an sg MAX.
 *        We take a full page, see how many sg elements fit and
 *        subtract one to account for a terminator.
 *
 * @notes Currently we need to use the minimum page size so that we
 *        don't break.  In the future we need an method that dynamically
 *        calculates the max data entries based on page size.
 *
 *********************************************************************/
#define FBE_RAID_SG_MAX_DATA_ENTRIES \
(( ((FBE_RAID_PAGE_SIZE_STD * FBE_BE_BYTES_PER_BLOCK)- FBE_RAID_SG_HEADER_OVERHEAD) /sizeof(fbe_sg_element_t)))

/*!*******************************************************************
 * @def FBE_RAID_SG_MAX_ENTRIES
 *********************************************************************
 * @brief This is the number of total sg elements in an sg MAX.
 *
 *********************************************************************/
#define FBE_RAID_SG_MAX_ENTRIES FBE_RAID_CALCULATE_TOTAL_SG_ENTRIES(FBE_RAID_SG_MAX_DATA_ENTRIES)

/*!*******************************************************************
 * @struct fbe_raid_sg_max_pool_entry_t
 *********************************************************************
 * @brief This is the type of structure we allocate for an sg 128.
 *
 *********************************************************************/
typedef struct fbe_raid_sg_max_pool_entry_s
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t header;
#endif
    fbe_sg_element_t sg[FBE_RAID_SG_MAX_ENTRIES];
#if RAID_DBG_MEMORY_ENABLED
    fbe_u32_t magic;
#endif
}
fbe_raid_sg_max_pool_entry_t;


/************************ 
 *fbe_raid_memory_api.c
 ************************/
extern fbe_status_t fbe_raid_memory_page_element_to_buffer_pool_entry(fbe_raid_memory_element_t *page_element_p,
                                                                               fbe_raid_buffer_pool_entry_t **pool_entry_pp);

/***********************************
 * end file fbe_raid_pool_entries.h
 ***********************************/

#endif /* end __FBE_RAID_POOL_ENTRIES_H__ */

