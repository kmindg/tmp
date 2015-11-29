#ifndef RAID_ERROR_H
#define RAID_ERROR_H
/*******************************************************************
 * Copyright (C) EMC Corporation 2000-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*******************************************************************
 *  raid_error.h
 *******************************************************************
 *
 * @brief
 *  This file contains definitions of RAID errors.  The error type 
 *  determines what will traced and logged to the event log.
 *
 *
 * @author
 *  04/16/2008: RDP - Moved RAID error definitions to `raid_error.h'
 *
 *
 *******************************************************************/

/*************************
 * INCLUDE FILES
 *************************/

/***************************************************
 * RAID EXTENDED STATUS Definitions.
 *
 * 
 * |--------------|---------------|--------|-------|---------------|
 * | Algorithm    | unused        | Reason |    Error Bits         |
 * |--------------|---------------|--------|-------|---------------|
 * 31            23               15     11       7               0
 *
 * These are the flags that we log to the event log
 * as part of the extended status.
 *
 * Basically we take the RAID_VR_TS eboard values
 * and for each drive extract a single bit
 * for each different type of error encountered.
 *
 * Reason codes are used to specify the kind of checksum error.
 * We use the upper nibble of byte 2 for backwards compatibility.
 *
 ***************************************************/
#define VR_NO_ERROR          0x000   /* No problems detected */
#define VR_UNEXPECTED_CRC    0x001   /* CRC Error where we don't recognize pattern */
#define VR_COH               0x002   /* consistency error */
#define VR_TS                0x004   /* timestamp error */
#define VR_WS                0x008   /* writestamp error */
#define VR_SS                0x010   /* shedstamp error */
#define VR_POC               0x020   /* Parity of checksums error. (R6)           */
#define VR_N_POC             0x040   /* Multiple Parity of checksums error. (R6)  */
#define VR_UNKNOWN_COH       0x080   /* An unknown coherency error occurred. (R6) */
#define VR_ZEROED            0x100   /* Should have been zeroed and was not. (R6) */
#define VR_LBA               0x200   /* An LBA stamp error (w/o CRC) has occurred */
#define VR_CRC_RETRY         0x400   /* Retried crc error. */
#define VR_UNUSED_1          0x800   /* Reserved. */
#define VR_RAW_MIRROR_MAGIC_NUM 0x1000  /* Magic number error (raw mirror) */
#define VR_RAW_MIRROR_SEQ_NUM   0x2000  /* Sequence number mismatch (raw mirror) */
#define VR_BITFIELD_COUNT    14      /* Number of possible bits above. */
/***************************************************
 * Reason codes are for
 * determining the kind of checksum error.
 * Typically more than one different error from the
 * below set will not occur in the same stripe on the
 * same drive at the same time.  If more than one
 * of these occurs we will only log one of them since
 * they use the same bits.
 ***************************************************/

#define VR_RAID_CRC          0x10000000   /* Raid checksum error */
#define VR_KLONDIKE_CRC      0x20000000   /* Klondike induced Checksum Error */
#define VR_DH_CRC            0x30000000   /* DH induced Checksum Error*/
#define VR_MEDIA_CRC         0x40000000   /* Media Error induced Checksum Error */
#define VR_CORRUPT_CRC       0x50000000   /* Intentionally invalidated corrupt crc Error. */
#define VR_CORRUPT_DATA      0x60000000   /* Intentionally invalidated corrupt data Error. */
#define VR_LBA_STAMP         0x70000000   /* LBA Stamp Error */
#define VR_SINGLE_BIT_CRC    0x80000000   /* A single bit CRC error */
#define VR_MULTI_BIT_CRC     0x90000000   /* A multi bit CRC error */
#define VR_INVALID_CRC       0xA0000000   /* Sector was invalidated due to data lost */
#define VR_BAD_CRC           0xB0000000   /* Unexpected CRC error on write data */
#define VR_COPY_CRC          0xC0000000   /* COPY induced Checksum Error*/
#define VR_PVD_METADATA_CRC  0xD0000000   /* PVD Metadata read related CRC Error */
#define VR_MAX_FLAG          0xE0000000   /* Next available reason */
#define VR_REASON_MASK       0xF0000000   /* Mask of reasons. */


/*!*******************************************************************
 * @def FBE_RAID_EXTRA_INFO_FLAGS_OFFSET
 *********************************************************************
 * @brief Offset to left shift the below flags in the extra info field
 *        of the event log.
 *
 *********************************************************************/
#define FBE_RAID_EXTRA_INFO_FLAGS_OFFSET 8  

/*!*******************************************************************
 * @enum fbe_raid_extra_info_flags_e
 *********************************************************************
 * @brief Set of flags we log in the extra info field to give more
 *        information about the command.
 *
 *********************************************************************/
enum fbe_raid_extra_info_flags_e
{
    FBE_RAID_EXTRA_INFO_FLAGS_INCOMPLETE_WR_VR = 0x0001,
    FBE_RAID_EXTRA_INFO_FLAGS_ERROR_VR         = 0x0002,
    FBE_RAID_EXTRA_INFO_FLAGS_SYSTEM_VR        = 0x0004,
    FBE_RAID_EXTRA_INFO_FLAGS_USER_RD_VR       = 0x0008,
    FBE_RAID_EXTRA_INFO_FLAGS_USER_VR          = 0x0010,
    FBE_RAID_EXTRA_INFO_FLAGS_VERIFY_WRITE     = 0x0020,
    FBE_RAID_EXTRA_INFO_FLAGS_NEXT             = 0x0040,
};
/*************************************************************
 * Name: VR_TS_Flag_Strings
 *
 * Description:  Each string represents a different RAID_VR_TS 
 *               flag from the above set of #defines.
 *               
 *               
 *************************************************************/
#define RAID_VR_TS_Flag_Strings\
    "VR_UNEXPECTED_CRC",\
    "VR_COH",\
    "VR_TS",\
    "VR_WS",\
    "VR_SS",\
    "VR_RAID_CRC",\
    "VR_KLONDIKE_CRC",\
    "VR_DH_CRC",\
    "VR_MEDIA_CRC",\
    "VR_CORRUPT_CRC",\
    "VR_LBA STAMP",\
    "VR_SINGLE_BIT_CRC",\
    "VR_MULTI_BIT_CRC",\
    "VR_INVALID_CRC",\
    "VR_BAD_CRC",\
    "VR_COPY_CRC",\
    "VR_PVD_METADATA_CRC",\
    "VR_MAX_FLAG"

/*****************************************
 * end of raid_error.h
 *****************************************/

#endif /* RAID_ERROR_H */
