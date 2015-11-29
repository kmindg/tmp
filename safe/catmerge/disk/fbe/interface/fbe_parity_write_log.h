#ifndef FBE_PARITY_WRITE_LOG_H
#define FBE_PARITY_WRITE_LOG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_parity_write_log.h
 ***************************************************************************
 *
 * @brief
 *  This file contains data structures for write logging.
 *
 * @version
 *   2/21/2012:  Created. Vamsi Vankamamidi
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_parity.h"

/*! @def FBE_PARITY_WRITE_LOG_ENABLE 
 *  @brief This temporary compile flag enables write-logging.
 */
#define FBE_PARITY_WRITE_LOG_ENABLE (FBE_TRUE)

/*! @def FBE_PARITY_WRITE_LOG_INVALID_SLOT 
 *  @brief This is a number indicating Invalid slot.
 */
#define FBE_PARITY_WRITE_LOG_INVALID_SLOT ((fbe_u32_t) 0xFFFFFFFF)

/*! @def FBE_PARITY_WRITE_LOG_HEADER_SIZE 
 *  @brief This has to be equal to logical block_size (without raid stamps).
 *  @TODO: This must be config param determined at during RG create 
 */
#define FBE_PARITY_WRITE_LOG_HEADER_SIZE (0x200)

/*! @def FBE_PARITY_WRITE_LOG_HEADER_BE_SIZE 
 *  @brief This has to be equal to drive's sector_size.
 *  @TODO: This must be config param determined at during RG create 
 */
#define FBE_PARITY_WRITE_LOG_HEADER_BE_SIZE (0x208)

/*! @def FBE_PARITY_WRITE_LOG_HEADER_CSUM_SEED 
 *  @brief Seed used when calculating checksum of all block checksums 
 */
#define FBE_PARITY_WRITE_LOG_HEADER_CSUM_SEED (0xC0FE)

/*! @def FBE_PARITY_WRITE_LOG_HEADER_VERSION_UNINITIALIZED 
 *  @brief This is a value indicating that write log slot header was never initialized, to avoid having an
 *         unpopulated buffer look like a valid header.
 */
#define FBE_PARITY_WRITE_LOG_HEADER_VERSION_UNINITIALIZED   (0xBAADF00DBAADFOOD)

/*! @def FBE_PARITY_WRITE_LOG_HEADER_VERSION_1 
 *  @brief This is a value indicating that write log slot header uses the version 1 definition for its fields.
 */
#define FBE_PARITY_WRITE_LOG_HEADER_VERSION_1   (0x600DF00D00010001)

/*! @def FBE_PARITY_WRITE_LOG_CURRENT_HEADER_VERSION 
 *  @brief This is a value indicating the current write log header in use by the current software revision.
 *         While the current software should be able to read in previous versions, after it does so
 *           it should reinitialize the on-disk headers to the current version.
 */
#define FBE_PARITY_WRITE_LOG_CURRENT_HEADER_VERSION   (FBE_PARITY_WRITE_LOG_HEADER_VERSION_1)

/*! @def FBE_PARITY_WRITE_LOG_HEADER_STATE_INVALID 
 *  @brief This is a value indicating that write log slot header is not valid.
 *           Uninitialized headers will also be invalid.
 */
#define FBE_PARITY_WRITE_LOG_HEADER_STATE_INVALID (0)

/*! @def FBE_PARITY_WRITE_LOG_HEADER_STATE_VALID 
 *  @brief This is a value indicating that write log slot header is initialized and valid.
 *         0 is uninitialized, and anything besides 0, invalid, or valid is garbage.
 */
#define FBE_PARITY_WRITE_LOG_HEADER_STATE_VALID   (1)

/*! @def FBE_PARITY_WRITE_LOG_HEADER_STATE_NEEDS_REMAP 
 *  @brief This is a value indicating that write log slot needs remap.
 */
#define FBE_PARITY_WRITE_LOG_HEADER_STATE_NEEDS_REMAP   (2)

typedef struct fbe_parity_write_log_slot_s {
    fbe_parity_write_log_slot_state_t state;
    fbe_parity_write_log_slot_invalidate_state_t invalidate_state;
}fbe_parity_write_log_slot_t;

/*!*******************************************************************
 * @enum fbe_parity_write_log_flags_e
 *********************************************************************
 * @brief Flags that indicate how we are using the write slot info.
 *
 *********************************************************************/
enum fbe_parity_write_log_flags_e
{
    FBE_PARITY_WRITE_LOG_FLAGS_NONE        = 0,
    FBE_PARITY_WRITE_LOG_FLAGS_QUIESCE     = 0x01,
    FBE_PARITY_WRITE_LOG_FLAGS_NEEDS_REMAP = 0x02,
    FBE_PARITY_WRITE_LOG_FLAGS_4K_SLOT     = 0x04,
    FBE_PARITY_WRITE_LOG_FLAGS_ENCRYPTED   = 0x08,
};

typedef fbe_u32_t fbe_parity_write_log_flags_t;

typedef struct fbe_parity_write_log_info_s{
    fbe_lba_t start_lba;
    fbe_u32_t slot_size;
    fbe_u32_t slot_count;
    fbe_queue_head_t request_queue_head;     /* siots waiting for slots queue */
    fbe_spinlock_t spinlock;    /* Lock to protect write_log struct. */
    fbe_parity_write_log_flags_t flags;
    fbe_parity_write_log_slot_t slot_array[FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_NORM]; /* use norm, it has most slots */
}fbe_parity_write_log_info_t;

/* This structure is written out as part of journal write, embedded in an array in the
 * fbe_parity_write_log_header_t structure below.  It must be packed since it is written 
 * to disk. 
 */
#pragma pack(1)
typedef struct fbe_parity_write_log_header_disk_info_s{

    /* Offset from the parity_start and block count */
    fbe_u16_t offset;
    fbe_u16_t block_cnt;

    /* Checksum of all blocks being written to a drive's write-log slot 
     * (calculated as checksum of checksums). */
    fbe_u16_t csum_of_csums;

}fbe_parity_write_log_header_disk_info_t;
#pragma pack()

/* This structure is written out as part of journal write.
 * It must be packed since it is written to disk. 
 */
#pragma pack(1)
typedef struct fbe_parity_write_log_header_timestamp_s{

    /* Precise secs and usecs since 1970, copied from a csx_precise_time_t struct */
    fbe_u64_t sec;
    fbe_u64_t usec;
}fbe_parity_write_log_header_timestamp_t;
#pragma pack()

/* This structure is written out as part of journal write. It occupies the first block in the journal slot
 * followed by user data. Both header and user data are written out as single write operation.
 * Slot headers of all drives participating in write op have same field values and thus any one can be used to
 * regenerate any other.  Individual data is contained in individual positions of the common disk_info array. 
 * It must be packed since it is written to disk, and must have large values first to avoid misalignment 
 * due to packing 
 */
#pragma pack(1)
typedef struct fbe_parity_write_log_header_s
{
    /* Unique number to validate that data in header is not garbage.
     * Can be invalid (0, garbage) or valid for a particular version 
     *   of the write logging on-disk definition.
     * Version defines how to interpret the rest of the fields when 
     *   reading the header from the disk.
     * Must be first value of header, and header of first slot in write 
     *   log area must always be initialized with proper version,
     *   so code can find it -- location of other headers may depend on version
     */
    fbe_u64_t header_version;

    /* Unique transaction number assigned to write log write (based on system time) */
    fbe_parity_write_log_header_timestamp_t header_timestamp;

    /* Logical block address where write operation starts on live stripe. */
    fbe_u64_t start_lba;

    /* Block address where parity starts on live stripe. */
    fbe_u64_t parity_start;

    /* Indicates whether the header is valid (contains data) or invalid */
    fbe_u16_t header_state;

    /* Count of data blocks involved in the write operation. */
    fbe_u16_t xfer_cnt;

    /* Count of parity blocks involved in the write operation. */
    fbe_u16_t parity_cnt;

    /* Bitmap of all the drives being written to */
    fbe_u16_t write_bitmap;

    /* Info about each disk in the write operation */
    fbe_parity_write_log_header_disk_info_t disk_info[FBE_PARITY_MAX_WIDTH];

}fbe_parity_write_log_header_t;
#pragma pack()


/* Accessors for write_logging */
static __forceinline fbe_parity_write_log_slot_state_t fbe_parity_write_log_get_slot_state(fbe_parity_write_log_info_t * write_log_info_p, fbe_u32_t slot_id)
{
    return write_log_info_p->slot_array[slot_id].state; 
}

static __forceinline void fbe_parity_write_log_set_slot_state(fbe_parity_write_log_info_t *write_log_info_p, fbe_u32_t slot_id, 
                                                       fbe_parity_write_log_slot_state_t state)
{
    write_log_info_p->slot_array[slot_id].state = state;
}

static __forceinline fbe_parity_write_log_slot_invalidate_state_t fbe_parity_write_log_get_slot_inv_state(fbe_parity_write_log_info_t * write_log_info_p, fbe_u32_t slot_id)
{
    return write_log_info_p->slot_array[slot_id].invalidate_state; 
}

static __forceinline void fbe_parity_write_log_set_slot_inv_state(fbe_parity_write_log_info_t *write_log_info_p, fbe_u32_t slot_id, 
                                                       fbe_parity_write_log_slot_invalidate_state_t inv_state)
{
    write_log_info_p->slot_array[slot_id].invalidate_state = inv_state;
}

static __forceinline fbe_u16_t fbe_parity_write_log_get_slot_hdr_csum_of_csums(fbe_parity_write_log_header_t * slot_header_p, fbe_u32_t pos)
{
    return slot_header_p->disk_info[pos].csum_of_csums; 
}

static __forceinline void fbe_parity_write_log_set_slot_hdr_csum_of_csums(fbe_parity_write_log_header_t * slot_header_p, fbe_u32_t pos, 
                                                       fbe_u16_t csum_of_csums)
{
    slot_header_p->disk_info[pos].csum_of_csums = csum_of_csums;
}

static __forceinline fbe_u16_t fbe_parity_write_log_get_slot_hdr_offset(fbe_parity_write_log_header_t * slot_header_p, fbe_u32_t pos)
{
    return slot_header_p->disk_info[pos].offset; 
}

static __forceinline void fbe_parity_write_log_set_slot_hdr_offset(fbe_parity_write_log_header_t * slot_header_p, fbe_u32_t pos, 
                                                       fbe_u16_t parity_offset)
{
    slot_header_p->disk_info[pos].offset = parity_offset;
}

static __forceinline fbe_u16_t fbe_parity_write_log_get_slot_hdr_blk_cnt(fbe_parity_write_log_header_t * slot_header_p, fbe_u32_t pos)
{
    return slot_header_p->disk_info[pos].block_cnt; 
}

static __forceinline void fbe_parity_write_log_set_slot_hdr_blk_cnt(fbe_parity_write_log_header_t * slot_header_p, fbe_u32_t pos, 
                                                       fbe_u16_t parity_cnt)
{
    slot_header_p->disk_info[pos].block_cnt = parity_cnt;
}
static __forceinline fbe_bool_t fbe_parity_write_log_is_flag_set(fbe_parity_write_log_info_t * write_log_info_p,
                                                                 fbe_parity_write_log_flags_t flag)
{
    return ((write_log_info_p->flags & flag) == flag);
}
static __forceinline void fbe_parity_write_log_init_flags(fbe_parity_write_log_info_t * write_log_info_p)
{
    write_log_info_p->flags = FBE_PARITY_WRITE_LOG_FLAGS_NONE;
    return;
}
static __forceinline void fbe_parity_write_log_get_flags(fbe_parity_write_log_info_t * write_log_info_p,
                                                         fbe_parity_write_log_flags_t *flags_p)
{
     *flags_p = write_log_info_p->flags;
}
static __forceinline void fbe_parity_write_log_set_flag(fbe_parity_write_log_info_t * write_log_info_p,
                                                             fbe_parity_write_log_flags_t flag)
{
     write_log_info_p->flags |= flag;
}
static __forceinline void fbe_parity_write_log_clear_flag(fbe_parity_write_log_info_t * write_log_info_p,
                                                               fbe_parity_write_log_flags_t flag)
{
    write_log_info_p->flags &= ~flag;
}
/**********************************
 * end file fbe_parity_write_log.h
 **********************************/

#endif 
