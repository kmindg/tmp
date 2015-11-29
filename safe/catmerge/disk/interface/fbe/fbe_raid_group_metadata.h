#ifndef FBE_RAID_GROUP_METADATA_H
#define FBE_RAID_GROUP_METADATA_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_raid_group_metadata.h
 ***************************************************************************
 *
 * @brief   The file contains the definitions for the the metadata format of
 *          a raid group.  All objects that inherit a raid group must use
 *          these definitions.
 *          Note: the non-paged metadata is defined in fbe_raid_group_object.h
 *          due to compilation issues when trying to define it in this file.
 *
 * @author
 *  02/04/2010  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/


/*!*******************************************************************
 * @enum fbe_raid_verify_flags_e
 * *********************************************************************
 * @brief
 *  This defines the individual verify bit flags that may be bitwise
 *  or'ed in the raid group chunk bitmap.
 *
 *********************************************************************/
enum fbe_raid_verify_flags_e
{
    FBE_RAID_BITMAP_VERIFY_NONE             = 0x00,
    FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE  = 0x01,
    FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY   = 0x02,
    FBE_RAID_BITMAP_VERIFY_ERROR            = 0x04,
    FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE = 0x08,
    FBE_RAID_BITMAP_VERIFY_SYSTEM           = 0x10,

    FBE_RAID_BITMAP_VERIFY_ALL              = (FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE |
                                               FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY |
                                               FBE_RAID_BITMAP_VERIFY_ERROR |
                                               FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE |
                                               FBE_RAID_BITMAP_VERIFY_SYSTEM)
};
typedef fbe_u8_t  fbe_raid_verify_flags_t;

/*!*******************************************************************
 * @struct fbe_raid_group_paged_metadata_s
 *********************************************************************
 * @brief
 *  This structure defines the layout of the paged metadata bitmap.
 *
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_raid_group_paged_metadata_s
{
    fbe_u8_t                        valid_bit:1;            // If TRUE, this paged metadata is valid. 
                                                            // If FALSE, this paged metadata encountered uncorrectable read error and was zeroed by MDS
    fbe_u8_t                        verify_bits:7;          // verify flag bits			(7 bits)
    fbe_raid_position_bitmask_t     needs_rebuild_bits;     // needs rebuild flag bits  (2 bytes)
    fbe_u8_t                        rekey:1;                // Rekey is needed.
    fbe_u8_t                        reserved_bits:7;        // reserved for future use    (1 byte) Peter Puhov (the alignment is VERY important)

} fbe_raid_group_paged_metadata_t;
#pragma pack()


/*!*******************************************************************
 * @enum fbe_raid_group_chunk_entry_size_t
 *********************************************************************
 * @brief
 *  This enum defines the size of each raid group chunk entry in bytes
 *
 *********************************************************************/
typedef enum fbe_raid_group_chunk_entry_size_s
{
    FBE_RAID_GROUP_CHUNK_ENTRY_SIZE = sizeof(fbe_raid_group_paged_metadata_t),
}
fbe_raid_group_chunk_entry_size_t;

/* Enum for different slot size versions for the parity write log
 */
/*!@todo This is currently unused, and must coordinate with the write log header version 
 *       read from disk after an upgrade, which may mean this value has changed.
 */
typedef enum fbe_raid_group_write_log_slot_size_version_e
{
    FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_VERSION_1 = 1,
    FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_CURRENT_VERSION = FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_VERSION_1
}fbe_raid_group_write_log_slot_size_version_t;


#endif /* FBE_RAID_GROUP_METADATA_H */

/*************************************
 * end file fbe_raid_group_metadata.h
 *************************************/
