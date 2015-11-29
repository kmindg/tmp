#ifndef XOR_SECTOR_HISTORY_H
#define XOR_SECTOR_HISTORY_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2005 - 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_xor_sector_history.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions to help save copies of sectors for 
 *  debugging purposes.
 *
 * @author
 *  07/25/2005 - Created Rob Foley.
 *
 ***************************************************************************/

/*************************
 **   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_winddk.h"

/*************************************************************
 * For debugging purposes we save the contents of blocks
 * with checksum errors that do not match any of the
 * known invalid patterns.
 *
 * We save the first FBE_XOR_FIRST_SECTOR_HISTORY_COUNT blocks and
 * we save the last FBE_XOR_LAST_SECTOR_HISTORY_COUNT blocks.
 *************************************************************/
#define FBE_XOR_FIRST_SECTOR_HISTORY_COUNT 6
#define FBE_XOR_LAST_SECTOR_HISTORY_COUNT 4

/* Max number of chars we allow in an error string displayed to ktrace.
 * This is 30 so that we will have space to display other chars
 * in same ktrace line.
 */
#define FBE_XOR_SECTOR_HISTORY_ERROR_MAX_CHARS 30

/*********************************************************************
 * fbe_xor_sector_history_record_t
 *
 * Simple debug structure with information for each block
 * that we decide to save.
 *
 *********************************************************************/
typedef struct fbe_xor_sector_history_record_s
{

    /*! Block where raid detected error (doesn't include edge offset) 
     */
    fbe_lba_t lba;

    /*! Block where raid detected error including edge offset 
     */
    fbe_lba_t pba;
    
    /*! The raid group object id that error occurred on 
     */
    fbe_object_id_t raid_group_object_id;

    /*! raid group position where error occurred
     */
    fbe_u32_t position;
        
    /*! The actual sector data for the block in error 
     */
    fbe_sector_t sector;

    /*! Time Stamp of error.  This is the first time we hit this error.
     */
    fbe_time_t time_stamp; 

    /*! Save the virtual drive object that error occurred on.
     */
    fbe_object_id_t drive_object_id;

    /*! Number of times we hit this error. 
     */
    fbe_u32_t hit_count;

    /*! The CRC computed from the data differs from the CRC 
     * in the metadata by this many bits. 
     */
    fbe_u32_t num_diff_bits_in_crc;  

    /*! This is the most recent time we hit this error.
     */
    fbe_time_t last_time_stamp; 
    
} fbe_xor_sector_history_record_t;

/*********************************************************************
 * fbe_xor_sector_history_gb_t
 *
 * We save the first FBE_XOR_FIRST_SECTOR_HISTORY_COUNT blocks and
 * we save the last FBE_XOR_LAST_SECTOR_HISTORY_COUNT blocks.
 *
 * This allows us to see the first N blocks that took a checksum error
 * and to see the last N blocks that took a checksum error.
 *
 *********************************************************************/
typedef struct fbe_xor_sector_history_gb_s
{
    /* When this flag is set, a reset is required.
     * This flag is most useful from ktcons or the debugger to
     * easily reset the history to zero.
     * This should always be first, so that it is easy to
     * get the address of this variable from ktcons
     * (since it is the same as the address of the structure).
     */
    fbe_bool_t reset_pending;
    
    /* This is a count of the blocks saved in
     * our first_saved_sector_array[].
     * Once this counter hits FBE_XOR_FIRST_SECTOR_HISTORY_COUNT,
     * we do not save any more blocks in first_saved_sector_array[],
     * but instead start saving in last_saved_sector_array[].
     */
    fbe_u32_t first_sectors_count;

    /* This tells us next index we may use to save a block in
     * the last_saved_sectors_array[].
     * The last_saved_sectors_array is a ring, so this resets
     * after it goes beyond FBE_XOR_LAST_SECTOR_HISTORY_COUNT.
     */
    fbe_u32_t last_sectors_next_index;

    /* This is the total number of sectors saved since last boot.
     * Even though the number of sectors saved is limited,
     * due to the fact that the "last" array wraps around,
     * this counter tells us how many times sectors were saved.
     */
    fbe_u32_t total_saved_sectors;

    /* This is the total number of CRC errors saved since last boot.
     * This includes both retryable and uncorrectable CRC errors.
     */
    fbe_u32_t total_crc_errors;

    /* This is the first N sectors saved.
     * Once this array is full we start saving sectors
     * in the last_saved_sectors_array.
     */
    fbe_xor_sector_history_record_t first_saved_sectors_array[FBE_XOR_FIRST_SECTOR_HISTORY_COUNT];

    /* This is the last N sectors saved.
     * We only save sectors here once first_saved_sectors_array is full.
     * We treat this array as a ring, overwriting the oldest entry.
     */
    fbe_xor_sector_history_record_t last_saved_sectors_array[FBE_XOR_LAST_SECTOR_HISTORY_COUNT];

    /* This spin lock is used to coordinate access to the buffer. 
     * It is needed since the offload might access the buffer also. 
     */
    fbe_spinlock_t spin_lock;
}
fbe_xor_sector_history_gb_t;
/****************************************
 * END fbe_xor_sector_history.h
 ****************************************/

#endif /* XOR_SECTOR_HISTORY_H */
