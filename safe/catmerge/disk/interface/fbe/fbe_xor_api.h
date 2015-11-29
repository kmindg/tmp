#ifndef FBE_XOR_API_H
#define FBE_XOR_API_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_xor_api.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interface to the FBE XOR Library.
 *
 * @version
 *   5/14/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_sg_element.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_winddk.h"
#include "xorlib_api.h"

/* Return Status for XOR operations.
 */
/*!*******************************************************************
 * @enum fbe_xor_status_t
 *********************************************************************
 * @brief These are the possible xor command status values.
 *
 *********************************************************************/
typedef enum fbe_xor_command_status_e
{
    FBE_XOR_STATUS_INVALID           =      0,
    FBE_XOR_STATUS_NO_ERROR          = 0x0001,
    FBE_XOR_STATUS_CHECKSUM_ERROR    = 0x0002,
    FBE_XOR_STATUS_CONSISTENCY_ERROR = 0x0004,
    FBE_XOR_STATUS_BAD_MEMORY        = 0x0008,
    FBE_XOR_STATUS_BAD_METADATA      = 0x0010,
    FBE_XOR_STATUS_BAD_SHEDSTAMP     = 0x0020,
    FBE_XOR_STATUS_UNEXPECTED_ERROR  = 0x0040, /*!< Unhandled condition encountered. */
}
fbe_xor_status_t;

#define FBE_XOR_INV_DRV_POS              0xFFFF

/* This is our known bad invalid sector checksum.
 * The way one calculates this is essentially the default checksum,
 * 5EED ^ FFFF.
 */
#define FBE_XOR_INVALID_SECTOR_CHECKSUM  0xA112
/* Number of frus we support.
 */
#define FBE_XOR_MAX_FRUS 16

/* This is the size of the rebuild and parity
 * vectors that are part of the interfaces.
 */
#define FBE_XOR_NUM_REBUILD_POS 2
#define FBE_XOR_NUM_PARITY_POS 2

/* This is the minimum number of data disk for a MR3 write
 */
#define FBE_XOR_MR3_WRITE_MIN_DATA_DISKS    2

/*!**************************************************
 * @enum fbe_xor_option_t
 * 
 * @brief Enum for checksum and lba stamp setting and gneration options.
 * These are bit values so multiple options can be set at once.
 ***************************************************/
typedef enum fbe_xor_option_e
{
    FBE_XOR_OPTION_NONE                  = 0x00000000,  /*!< No options selected. */
    FBE_XOR_OPTION_CHK_CRC               = 0x00000001,  /*!< Check checksums. */
    FBE_XOR_OPTION_GEN_CRC               = 0x00000002,  /*!< Generate checksums. */
    FBE_XOR_OPTION_CHK_LBA_STAMPS        = 0x00000004,  /*!< Check lba stamps. */
    FBE_XOR_OPTION_GEN_LBA_STAMPS        = 0x00000008,  /*!< Generate lba stamps. */
    FBE_XOR_OPTION_SET_STAMPS            = 0x00000010,  /*!< Set stamps. */
    FBE_XOR_OPTION_DEBUG                 = 0x00000020,  /*!< Enable additional XOR debug options */
    FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM  = 0x00000040,  /*!< If XOR debug is enabled, generate an error trace for checksum errors */
    FBE_XOR_OPTION_INVALIDATE_BAD_BLOCKS = 0x00000080,  /*!< Invalidate any bad blocks found. */
    FBE_XOR_OPTION_CORRUPT_DATA          = 0x00000100,  /*!< Buffer contains invalidated blocks for a corrupt DATA. */
    FBE_XOR_OPTION_CORRUPT_CRC           = 0x00000200,  /*!< Buffer contains invalidated blocks for a corrupt crc. */
    FBE_XOR_OPTION_GEN_R6PS_STAMP        = 0x00000400,  /*!< Generate the RAID-6 parity shedding stamp */
    FBE_XOR_OPTION_ALLOW_INVALIDS        = 0x00000800,  /*!< Don't report errors for invalidated data for pre-reads */
    FBE_XOR_OPTION_WRITE_REQUEST         = 0x00001000,  /*!< This is a write request (determines how to handle bad crc errors). */
    FBE_XOR_OPTION_VALIDATE_DATA         = 0x00002000,  /*!< If enabled, validate that we don't return invalidated data with good crc */
    FBE_XOR_OPTION_CHECK_DATA_PATTERN    = 0x00004000,  /*!< If `validate data' is set and data pattern checking is enabled check data pattern */
    FBE_XOR_OPTION_LOGICAL_REQUEST       = 0x00008000,  /*!< This is a `logical' request and therefore don't validate `seed' on invalids */
    FBE_XOR_OPTION_IGNORE_INVALIDS       = 0x00010000,  /*!< This indicates that no action should be take when invalidated sectors are encountered */
    FBE_XOR_OPTION_BVA                   = 0x00020000,  /*!<This is a flag to hint XOR lib this is a BVA case and to not panic on Expected COH errors*/
    /* Insert next FBE_XOR_OPTION_xxxx   = 0x00040000, */
}
fbe_xor_option_t;
 

/*********************************
 * STRUCTURE DEFINITIONS
 *********************************/

/*!*******************************************************************
 * @struct fbe_xor_zero_buffer_t
 *********************************************************************
 * @brief This is the structure passed to the xor driver for
 *        zero buffer operation.
 *
 *********************************************************************/
typedef struct fbe_xor_zero_buffer_s
{
    fbe_u32_t disks;
    fbe_xor_status_t status;
    fbe_lba_t offset;

    struct fbe_xor_zero_buffer_vectors_s
    {
        fbe_sg_element_t *sg_p;
        fbe_block_count_t offset;
        fbe_block_count_t count;
        fbe_lba_t seed;
    }
    fru[FBE_XOR_MAX_FRUS];
}
fbe_xor_zero_buffer_t;

/*!*******************************************************************
 * @enum fbe_xor_error_state_t
 *********************************************************************
 * @brief These are the possible xor error (a.k.a. eboard) states
 *
 *********************************************************************/
typedef enum fbe_xor_error_state_e
{
    FBE_XOR_ERROR_STATE_INVALID         =           0,
    FBE_XOR_ERROR_STATE_INITIALIZED     =  0xfbe0e0e1,
    FBE_XOR_ERROR_STATE_VALID           =  0xfbe0e0e7, 
    FBE_XOR_ERROR_STATE_DESTROYED       =  0xfbe0e0ed,
}
fbe_xor_error_state_t;

/*!***************************************************************************
 * @def     fbe_xor_error_t
 *****************************************************************************
 * @brief   This structure keeps track of the types and positions of error
 *     taken while executing specific XOR operations. 
 *
 * xor_error_state - The `state' of the eboard.  The eboard must be in at least
 *                   the created state before it can be updated.
 *
 *   u_crc_bitmap       - Uncorrectable crc errors 
 *   c_crc_bitmap       - Correctable crc errors 
 *   u_coh_bitmap       - Uncorrectable coherency errors
 *   c_coh_bitmap       - Correctable coherency errors
 *   u_ts_bitmap        - Uncorrectable time stamp errors
 *   c_ts_bitmap        - Correctable time stamp errors 
 *   u_ws_bitmap        - Uncorrectable write stamp errors 
 *   c_ws_bitmap        - Correctable write stamp errors 
 *   u_ss_bitmap        - Uncorrectable shed stamp errors
 *   c_ss_bitmap        - Correctable shed stamp errors 
 *   u_n_poc_coh_bitmap - Uncorrectable parity of checksum errors (multiple)
 *   c_n_poc_coh_bitmap - Correctable parity of checksum errors (multiple)
 *   u_poc_coh_bitmap   - Uncorrectable parity of checksum errors
 *   c_poc_coh_bitmap   - Correctable parity of checksum errors
 *   u_coh_unk_bitmap   - Uncorrectable unkown coherency error (RAID 6)
 *   c_coh_unk_bitmap   - Correctable unkown coherency error (RAID 6)
 *   u_rm_magic_bitmap  - Uncorrectable magic number (raw mirror)
 *   c_rm_magic_bitmap  - Correctable magic number (raw mirror)
 *   c_rm_seq_bitmap    - Correctable sequence number (raw mirror)
 *   w_bitmap           - The bitmap of write positions
 *   u_bitmap           - The bitmap of uncorrectable positions 
 *   m_bitmap           - Modified 
 *   hard_media_err_bitmap    - The bitmap of media errors.
 *                        This is used as input to xor driver.
 *                        So the XOR driver knows to invalidate blocks.
 *  retry_err_bitmap    - Bitmap of positions that encountered a retryable
 *                        error
 *
 *  If any of the below are set, then we either have the same
 *  positional bits set in u_crc_bitmap or c_crc_bitmap.
 *
 *   crc_raid_bitmap      - The bitmap of raid invalid crc errors.
 *   crc_dh_bitmap        - The bitmap of dh invalid crc errors.
 *   crc_klondike_bitmap  - The bitmap of klondike induced errors.
 *   crc_lba_stamp_bitmap - The bitmap of lba stamp error.
 *   hard_media_err_bitmap - The bitmap of errors caused
 *                          by media errors on this drive.
 *   corrupt_crc_bitmap   - The bitmap of sectors intentionally invalidated
 *                          by FLARE with the corrupt crc format.
 *   corrupt_data_bitmap  - The bitmap of sectors intentionally invalidated
 *                          by FLARE with the corrupt crc format but the
 *                          parity has been updated yet.
 *   crc_unknown_bitmap   - The bitmap of sectors detected to be invalid,
 *                          but for which the invalid format was unknown.
 *   crc_single_bitmap   -  The bitmap of sectors detected to be invalid,
 *                          but for which the invalid format was unknown.
 *                          Also, the CRC computed from the data differed from
 *                          the CRC in the metadata by only 1 bit.
 *   crc_multi_bitmap     - The bitmap of sectors detected to be invalid,
 *                          but for which the invalid format was unknown.
 *                          Also, the CRC computed from the data differed from
 *                          the CRC in the metadata by more than 1 bit.
 *   zeroed_bitmap        - The bitmap of sectors that should be zeroed,
 *                          but were detected to not be zeroed.
 *   crc_copy_bitmap      - The bitmap of copy invalid crc errors.
 *   
 * crc_pvd_metadata_bitmap- Bitmap of PVD Metadata invalid CRC Errors.   
 *
 * @note    `Fixed Values' - Those values that are set during creation and are
 *                           note updated when an error occurs.
 *
 *  raid_group_object_id - The raid group object identifier associated with
 *                          the I/O request.
 *  raid_group_offset    - The offset of the raid group associated with the 
 *                         request.
 *
 * @note    Need to update fbe_xor_eboard_set_persistent_values() if additional
 *          `fixed' values are added to the eboard.
 *
 *******************************************************************************/
typedef struct fbe_xor_error_s
{
    /*! Since the eboard contains the offset used to generate the LBA stamp
     *  it must be valid.  This means it must be properly created.  We use
     *  special states to validate the eboard.
     */
    fbe_xor_error_state_t   xor_error_state;
       
    /*! We currently save the raid group object id and raid group
     *  offset for this request.
     *
     *  @note Add any new `persistent' values here!!
     *  @note Need to update fbe_xor_eboard_set_persistent_values().
     */
    fbe_object_id_t         raid_group_object_id;
    fbe_lba_t               raid_group_offset;
    fbe_u16_t               hard_media_err_bitmap;
    fbe_u16_t               retry_err_bitmap;
    fbe_u16_t               no_data_err_bitmap; /*!< should be reconstructed, but not due to physical errors. */

    /*! Error bitmaps - Per position bitmaps for each type of error
     */
    fbe_u16_t u_crc_bitmap;
    fbe_u16_t c_crc_bitmap;
    fbe_u16_t u_coh_bitmap;
    fbe_u16_t c_coh_bitmap;
    fbe_u16_t u_ts_bitmap;
    fbe_u16_t c_ts_bitmap;
    fbe_u16_t u_ws_bitmap;
    fbe_u16_t c_ws_bitmap;
    fbe_u16_t u_ss_bitmap;
    fbe_u16_t c_ss_bitmap;
    fbe_u16_t u_n_poc_coh_bitmap;
    fbe_u16_t c_n_poc_coh_bitmap;
    fbe_u16_t u_poc_coh_bitmap;
    fbe_u16_t c_poc_coh_bitmap;
    fbe_u16_t u_coh_unk_bitmap;
    fbe_u16_t c_coh_unk_bitmap;
    fbe_u16_t u_rm_magic_bitmap;
    fbe_u16_t c_rm_magic_bitmap;
    fbe_u16_t c_rm_seq_bitmap;
    fbe_u16_t w_bitmap;
    fbe_u16_t u_bitmap;
    fbe_u16_t m_bitmap;
    fbe_u16_t crc_invalid_bitmap;
    fbe_u16_t crc_raid_bitmap;
    fbe_u16_t crc_bad_bitmap;
    fbe_u16_t crc_dh_bitmap;
    fbe_u16_t crc_klondike_bitmap;
    fbe_u16_t crc_lba_stamp_bitmap;
    fbe_u16_t media_err_bitmap;
    fbe_u16_t corrupt_crc_bitmap;
    fbe_u16_t corrupt_data_bitmap;
    fbe_u16_t crc_unknown_bitmap;
    fbe_u16_t crc_single_bitmap;
    fbe_u16_t crc_multi_bitmap;
    fbe_u16_t crc_multi_and_lba_bitmap;
    fbe_u16_t zeroed_bitmap;
    fbe_u16_t crc_copy_bitmap;
    fbe_u16_t crc_pvd_metadata_bitmap;
}
fbe_xor_error_t;

/* Accessors for state
 */
static __forceinline void fbe_xor_error_set_state(fbe_xor_error_t *eboard_p, fbe_xor_error_state_t new_state)
{
    eboard_p->xor_error_state = new_state;
    return;
}
static __forceinline fbe_xor_error_state_t fbe_xor_error_get_state(fbe_xor_error_t *eboard_p)
{
    return(eboard_p->xor_error_state);
}
static __forceinline fbe_bool_t fbe_xor_is_valid_eboard(fbe_xor_error_t *eboard_p)
{
    if ((eboard_p != NULL)                                                   &&
        ((eboard_p->xor_error_state == FBE_XOR_ERROR_STATE_INITIALIZED) ||
         (eboard_p->xor_error_state == FBE_XOR_ERROR_STATE_VALID)          )    )
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
static __forceinline fbe_bool_t fbe_xor_is_eboard_setup(fbe_xor_error_t *eboard_p)
{
    if ((eboard_p != NULL)                                       &&
        (eboard_p->xor_error_state == FBE_XOR_ERROR_STATE_VALID)          )
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
static __forceinline void fbe_xor_reset_eboard(fbe_xor_error_t *eboard_p)
{
    eboard_p->xor_error_state = FBE_XOR_ERROR_STATE_INITIALIZED;
    return;
}

/* Accessor for offset
 */
static __forceinline fbe_lba_t fbe_xor_error_get_offset(fbe_xor_error_t *eboard_p)
{
    return(eboard_p->raid_group_offset);
}

/* Accessor for raid group id
 */
static __forceinline fbe_object_id_t fbe_xor_error_get_raid_group_id(fbe_xor_error_t *eboard_p)
{
    return(eboard_p->raid_group_object_id);
}

/***********************************************************************
 * fbe_xor_error_type_t
 *  This enum has one entry for each type of error
 *  that the RG Driver can insert.
 *
 *  FBE_XOR_ERR_TYPE_NONE - This says to not inject any errors.
 *                     Used for disabling error table entries.
 *
 *  FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR - Tell DH to inject a soft media error.
 *
 *  FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR - Tell DH to inject a hard media error.
 *
 *  FBE_XOR_ERR_TYPE_RND_MEDIA_ERR - Tell DH to randomly inject soft/hard error.
 *
 *  FBE_XOR_ERR_TYPE_CRC - Inject an unknown and unexpected crc error.
 *
 *  FBE_XOR_ERR_TYPE_KLOND_CRC - Inject a klondike formatted crc error.
 *
 *  FBE_XOR_ERR_TYPE_DH_CRC - Inject a DH formatted crc error.
 *                       This is an invalid sector with
 *                       the "who" field set to INV_DH_SOFT_MEDIA.
 *
 *  FBE_XOR_ERR_TYPE_RAID_CRC - Inject a RAID formatted crc error.
 *                         This is an invalid sector with
 *                         the "who" field set to INV_VERIFY.
 *
 *  FBE_XOR_ERR_TYPE_CORRUPT_CRC - Inject a CORRUPT_CRC formatted crc error.
 *                            This is an invalid sector with
 *                            the "who" field set to INV_DH_WR_BAD_CRC.
 *
 *  FBE_XOR_ERR_TYPE_LBA_STAMP - A LBA stamp error. Not used in injection since
 *                           Shed stamps accomplish the same thing. Used in reporting
 *                           the error type.
 *
 *  FBE_XOR_ERR_TYPE_WS - Inject a write stamp error. (yes, on all raid types)
 *
 *  FBE_XOR_ERR_TYPE_TS - Inject a time stamp error. (yes, on all raid types)
 *
 *  FBE_XOR_ERR_TYPE_SS - Inject a shed stamp error. (yes, on all raid types)
 *
 *  FBE_XOR_ERR_TYPE_BOGUS_WS - Inject a bogus write stamp error. (yes, on all raid types)
 *                         Bogus in this context means that it is invalid for the
 *                         width of array being tested or is bogus because we're setting
 *                         parity drive position bit.
 *
 *  FBE_XOR_ERR_TYPE_BOGUS_TS - Inject a bogus time stamp error. (yes, on all raid types)
 *
 *  FBE_XOR_ERR_TYPE_BOGUS_SS - Inject a bogus shed stamp error. (yes, on all raid types)
 *
 *  FBE_XOR_ERR_TYPE_1NS  - Inject an error to one of the non-S data symbols.
 *
 *  FBE_XOR_ERR_TYPE_1S   - Inject an error to one of the     S data symbols.
 *
 *  FBE_XOR_ERR_TYPE_1R   - Inject an error to one of the row      parity symbols.
 *
 *  FBE_XOR_ERR_TYPE_1D   - Inject an error to one of the diagonal parity symbols.
 *
 *  FBE_XOR_ERR_TYPE_1COD - Inject an error to one of the data   checksums.
 *
 *  FBE_XOR_ERR_TYPE_1COP - Inject an error to one of the parity checksums.
 *
 *  FBE_XOR_ERR_TYPE_1POC - Inject an error to one of the parity of checksums. 
 *
 *  FBE_XOR_ERR_TYPE_COH  - ???
 *
 *  FBE_XOR_ERR_TYPE_CORRUPT_DATA - Inject a CORRUPT_DATA formatted crc error.
 *                            This is an invalid sector with
 *                            the "who" field set to INV_RAID_CORRUPT_DATA.
 *
 *  FBE_XOR_ERR_TYPE_N_POC_COH - We detected multiple POC coherency errors.
 *
 *  FBE_XOR_ERR_TYPE_POC_COH - We detected a single POC coherency error.
 *
 *  FBE_XOR_ERR_TYPE_COH_UNKNOWN - We detected some unknown coherency error.
 *
 *  FBE_XOR_ERR_TYPE_RB_FAILED - We detected an uncorrectable error on a RB pos.
 *
 *  FBE_XOR_ERR_TYPE_UNKNOWN - This is essentially an "invalid error type".
 *                        This can be used for initializing values.
 *
 *  Below are the flags.
 * 
 *  FBE_XOR_ERR_TYPE_UNMATCHED - This is an unmatched/unexpected error by
 *                               the error injection service.
 *
 *  FBE_XOR_ERR_TYPE_RB_INV_DATA - We took a crc error on invalidated data
 *                             and then rebuilt invalidated data.
 *
 *  FBE_XOR_ERR_TYPE_INITIAL_TS - We took a bogus TS error because of an
 *                            "initial" time stamp.
 *
 *  FBE_XOR_ERR_TYPE_ZEROED - We took this correctable error because of a
 *                        zeroed condition.
 *
 ***********************************************************************/
typedef enum fbe_xor_error_type_s
{
    FBE_XOR_ERR_TYPE_NONE = 0,                  /* 0x00 */
    FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR,            /* 0x01 */
    FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR,            /* 0x02 */
    FBE_XOR_ERR_TYPE_RND_MEDIA_ERR,             /* 0x03 */
    FBE_XOR_ERR_TYPE_CRC,                       /* 0x04 */
    FBE_XOR_ERR_TYPE_KLOND_CRC,                 /* 0x05 */
    FBE_XOR_ERR_TYPE_DH_CRC,                    /* 0x06 */
    FBE_XOR_ERR_TYPE_RAID_CRC,                  /* 0x07 */
    FBE_XOR_ERR_TYPE_CORRUPT_CRC,               /* 0x08 */
    FBE_XOR_ERR_TYPE_WS,                        /* 0x09 */
    FBE_XOR_ERR_TYPE_TS,                        /* 0x0A */
    FBE_XOR_ERR_TYPE_SS,                        /* 0x0B */
    FBE_XOR_ERR_TYPE_BOGUS_WS,                  /* 0x0C */
    FBE_XOR_ERR_TYPE_BOGUS_TS,                  /* 0x0D */
    FBE_XOR_ERR_TYPE_BOGUS_SS,                  /* 0x0E */
    FBE_XOR_ERR_TYPE_1NS,                       /* 0x0F */
    FBE_XOR_ERR_TYPE_1S,                        /* 0x10 */
    FBE_XOR_ERR_TYPE_1R,                        /* 0x11 */
    FBE_XOR_ERR_TYPE_1D,                        /* 0x12 */
    FBE_XOR_ERR_TYPE_1COD,                      /* 0x13 */
    FBE_XOR_ERR_TYPE_1COP,                      /* 0x14 */
    FBE_XOR_ERR_TYPE_1POC,                      /* 0x15 */
    FBE_XOR_ERR_TYPE_COH,                       /* 0x16 */
    FBE_XOR_ERR_TYPE_CORRUPT_DATA,              /* 0x17 */
    FBE_XOR_ERR_TYPE_N_POC_COH,                 /* 0x18 */
    FBE_XOR_ERR_TYPE_POC_COH,                   /* 0x19 */
    FBE_XOR_ERR_TYPE_COH_UNKNOWN,               /* 0x1A */
    FBE_XOR_ERR_TYPE_RB_FAILED,                 /* 0x1B */
    FBE_XOR_ERR_TYPE_LBA_STAMP,                 /* 0x1C */
    FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC,            /* 0x1D */
    FBE_XOR_ERR_TYPE_MULTI_BIT_CRC,             /* 0x1E */
    FBE_XOR_ERR_TYPE_TIMEOUT_ERR,               /* 0x1F */
    FBE_XOR_ERR_TYPE_CORRUPT_CRC_INJECTED,      /* 0x20 */
    FBE_XOR_ERR_TYPE_CORRUPT_DATA_INJECTED,     /* 0x21 */
    FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM,  /* 0x22 */
    FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM,    /* 0x23 */
    FBE_XOR_ERR_TYPE_SILENT_DROP,               /* 0x24 */
    FBE_XOR_ERR_TYPE_INVALIDATED,               /* 0x25 */
    FBE_XOR_ERR_TYPE_BAD_CRC,                   /* 0x26 */
    FBE_XOR_ERR_TYPE_DELAY_DOWN,                /* 0x27 */
    FBE_XOR_ERR_TYPE_DELAY_UP,                  /* 0x28 */
    FBE_XOR_ERR_TYPE_COPY_CRC,                  /* 0x29 */
    FBE_XOR_ERR_TYPE_PVD_METADATA,              /* 0x2A */
    FBE_XOR_ERR_TYPE_IO_UNEXPECTED,             /* 0x2B*/
    FBE_XOR_ERR_TYPE_INCOMPLETE_WRITE,          /* 0x2C*/
    FBE_XOR_ERR_TYPE_KEY_ERROR,                 /* 0x2D*/
    FBE_XOR_ERR_TYPE_KEY_NOT_FOUND,             /* 0x2E*/
    FBE_XOR_ERR_TYPE_ENCRYPTION_NOT_ENABLED,    /* 0x2F*/
    FBE_XOR_ERR_TYPE_MULTI_BIT_WITH_LBA_STAMP,  /* 0x30*/
    /* Add any new error types here.  We cannot change these
     * defines since we have many error tables created with these numbers.
     * IMPORTANT:
     *            When adding new error types, please update the
     *            string definitions in XOR_ERROR_TYPE_STRINGS below.
     */
    FBE_XOR_ERR_TYPE_UNKNOWN,                   /* 0x30 */
    FBE_XOR_ERR_TYPE_MAX,                       /* 0x31 */
    FBE_XOR_ERR_TYPE_MASK = 0xFF,               /* 0xFF */
    
    /* The below are treated like flags.
     */
    FBE_XOR_ERR_TYPE_UNMATCHED = 0x0200,
    FBE_XOR_ERR_TYPE_OTHERS_INVALIDATED = 0x100,
    FBE_XOR_ERR_TYPE_RB_INV_DATA = 0x1000,
    FBE_XOR_ERR_TYPE_INITIAL_TS = 0x2000,
    FBE_XOR_ERR_TYPE_ZEROED = 0x4000,
    FBE_XOR_ERR_TYPE_UNCORRECTABLE = 0x8000
}
fbe_xor_error_type_t;

/* Mask of flags within the fbe_xor_error_type_t enumeration.
 */ 
#define FBE_XOR_ERR_TYPE_FLAG_MASK (FBE_XOR_ERR_TYPE_OTHERS_INVALIDATED | \
                                    FBE_XOR_ERR_TYPE_ZEROED | \
                                    FBE_XOR_ERR_TYPE_INITIAL_TS | \
                                    FBE_XOR_ERR_TYPE_UNCORRECTABLE |\
                                    FBE_XOR_ERR_TYPE_RB_INV_DATA)
/*!*******************************************************************
 * @def FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC
 *********************************************************************
 * @brief Determine if this is an unexepcted crc error.
 *
 *********************************************************************/
#define FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC(x) \
    (((x & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_CRC)  ||\
     ((x & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC) ||\
     ((x & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_MULTI_BIT_CRC))

/*****************************************
 * XOR_ERROR_TYPE_STRINGS
 *
 * Strings to represent the fields for
 * displaying types of errors.
 *****************************************/
#define FBE_XOR_ERROR_TYPE_STRINGS\
    "FBE_XOR_ERR_TYPE_NONE",\
    "FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR",\
    "FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR",\
    "FBE_XOR_ERR_TYPE_RND_MEDIA_ERR",\
    "FBE_XOR_ERR_TYPE_CRC",\
    "FBE_XOR_ERR_TYPE_KLOND_CRC",\
    "FBE_XOR_ERR_TYPE_DH_CRC",\
    "FBE_XOR_ERR_TYPE_RAID_CRC",\
    "FBE_XOR_ERR_TYPE_CORRUPT_CRC",\
    "FBE_XOR_ERR_TYPE_WS",\
    "FBE_XOR_ERR_TYPE_TS",\
    "FBE_XOR_ERR_TYPE_SS",\
    "FBE_XOR_ERR_TYPE_BOGUS_WS",\
    "FBE_XOR_ERR_TYPE_BOGUS_TS",\
    "FBE_XOR_ERR_TYPE_BOGUS_SS",\
    "FBE_XOR_ERR_TYPE_1NS",\
    "FBE_XOR_ERR_TYPE_1S",\
    "FBE_XOR_ERR_TYPE_1R",\
    "FBE_XOR_ERR_TYPE_1D",\
    "FBE_XOR_ERR_TYPE_1COD",\
    "FBE_XOR_ERR_TYPE_1COP",\
    "FBE_XOR_ERR_TYPE_1POC",\
    "FBE_XOR_ERR_TYPE_COH",\
    "FBE_XOR_ERR_TYPE_CORRUPT_DATA",\
    "FBE_XOR_ERR_TYPE_N_POC_COH",\
    "FBE_XOR_ERR_TYPE_POC_COH",\
    "FBE_XOR_ERR_TYPE_COH_UNKNOWN",\
    "FBE_XOR_ERR_TYPE_RB_FAILED",\
    "FBE_XOR_ERR_TYPE_LBA_STAMP",\
    "FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC",\
    "FBE_XOR_ERR_TYPE_MULTI_BIT_CRC",\
    "FBE_XOR_ERR_TYPE_TIMEOUT_ERR",\
    "FBE_XOR_ERR_TYPE_CORRUPT_CRC_INJECTED",\
    "FBE_XOR_ERR_TYPE_CORRUPT_DATA_INJECTED",\
    "FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM",\
    "FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM",\
    "FBE_XOR_ERR_TYPE_SILENT_DROP",\
    "FBE_XOR_ERR_TYPE_INVALIDATED",\
    "FBE_XOR_ERR_TYPE_BAD_CRC",\
    "FBE_XOR_ERR_TYPE_DELAY_DOWN",\
    "FBE_XOR_ERR_TYPE_DELAY_UP",\
    "FBE_XOR_ERR_TYPE_COPY_CRC",\
    "FBE_XOR_ERR_TYPE_PVD_METADATA",\
    "FBE_XOR_ERR_TYPE_IO_UNEXPECTED",\
    "FBE_XOR_ERR_TYPE_INCOMPLETE_WRITE",\
    "FBE_XOR_ERR_TYPE_KEY_ERROR",\
    "FBE_XOR_ERR_TYPE_KEY_NOT_FOUND",\
    "FBE_XOR_ERR_TYPE_ENCRYPTION_NOT_ENABLED",\
    "FBE_XOR_ERR_TYPE_UNKNOWN",\
    "FBE_XOR_ERR_TYPE_MAX"
/*****************************************
 * end XOR_ERROR_TYPE_STRINGS
 *****************************************/

/*!*************************************************
 * @struct fbe_xor_error_region_t XOR_ERROR_REGION
 **************************************************/
typedef struct fbe_xor_error_region_s
{
    fbe_lba_t    lba;          /* Disk lba of the error */
    fbe_u16_t blocks;       /* Number of adjacent blocks in error. */
    fbe_u16_t pos_bitmask;  /* Bitmask of positions in error. */

    /* XOR_ERROR_TYPE. (CRC,TS,WS,SS, etc.)
     * This is defined as a fbe_u32_t and not an XOR_ERROR_TYPE,
     * since it is easier to view fbe_u32_t in the debugger since
     * One can see the flags and the error type separately.
     */
    fbe_u32_t error;
}
fbe_xor_error_region_t;

/***************************************************
 * FBE_XOR_MAX_ERROR_REGIONS
 *
 * This is the maximum number of error regions we allow.
 * This value was picked considering that we will have 
 * at least one error region per disk. We have restricted 
 * to this number considering the memory consumption
 * as Raid driver will allocate many of this.
 ***************************************************/

#define FBE_XOR_MAX_ERROR_REGIONS FBE_XOR_MAX_FRUS 

/***************************************************
 * XOR_ERROR_REGIONS
 *
 * This structure defines a set of XOR_ERROR_REGIONS
 * structures.
 ***************************************************/
typedef struct fbe_xor_error_regions_s
{
    /* This index always indicates the next available error region
     * in the below array of regions.
     */
    fbe_u8_t next_region_index;

    /* This is an array of regions.
     */
    fbe_xor_error_region_t regions_array[FBE_XOR_MAX_ERROR_REGIONS];
}
fbe_xor_error_regions_t;

/*!**********************************************************
 * @def FBE_XOR_ERROR_REGION_IS_EMPTY
 * The error regions struct is empty if the next available
 * region slot is the first slot.
 ***********************************************************/
#define FBE_XOR_ERROR_REGION_IS_EMPTY( m_region_p )\
(m_region_p->next_region_index == 0)

/*********************************
 * MACROS
 *********************************/

/*!**************************************************
 * fbe_xor_eboard_clear_mask
 * 
 * @brief
 * Clears a certain set of bits from an error board.
 *
 ***************************************************/
static __inline void fbe_xor_eboard_clear_mask( fbe_xor_error_t *pEboard, fbe_u16_t ClearMask )
{
    /* This macro clears only the bits that we wish to
     * track across multiple calls to XOR.
     *
     * At this time the following bits do not
     * need to be cleared because they are
     * not bits that need to be tabulated
     * across multiple calls to XOR:
     *  m_bitmap
     *  hard_media_err_bitmap
     */
    pEboard->u_crc_bitmap &= ~ClearMask;
    pEboard->c_crc_bitmap &= ~ClearMask;
    pEboard->u_coh_bitmap &= ~ClearMask;
    pEboard->c_coh_bitmap &= ~ClearMask;
    pEboard->u_ts_bitmap &= ~ClearMask;
    pEboard->c_ts_bitmap &= ~ClearMask;
    pEboard->u_ws_bitmap &= ~ClearMask;
    pEboard->c_ws_bitmap &= ~ClearMask;
    pEboard->u_ss_bitmap  &= ~ClearMask;
    pEboard->c_ss_bitmap &= ~ClearMask;
    pEboard->u_n_poc_coh_bitmap &= ~ClearMask;
    pEboard->c_n_poc_coh_bitmap &= ~ClearMask;
    pEboard->u_poc_coh_bitmap &= ~ClearMask;
    pEboard->c_poc_coh_bitmap &= ~ClearMask;
    pEboard->u_coh_unk_bitmap &= ~ClearMask;
    pEboard->c_coh_unk_bitmap &= ~ClearMask;
    pEboard->crc_raid_bitmap &= ~ClearMask;
    pEboard->crc_invalid_bitmap &= ~ClearMask;
    pEboard->crc_bad_bitmap &= ~ClearMask;
    pEboard->crc_dh_bitmap &= ~ClearMask;
    pEboard->crc_klondike_bitmap &= ~ClearMask;
    pEboard->crc_lba_stamp_bitmap &= ~ClearMask;
    pEboard->media_err_bitmap &= ~ClearMask;
    pEboard->corrupt_crc_bitmap &= ~ClearMask;
    pEboard->corrupt_data_bitmap &= ~ClearMask;
    pEboard->crc_unknown_bitmap &= ~ClearMask;
    pEboard->crc_single_bitmap &= ~ClearMask;
    pEboard->crc_multi_bitmap &= ~ClearMask;
    pEboard->crc_multi_and_lba_bitmap &= ~ClearMask;
    pEboard->zeroed_bitmap &= ~ClearMask;
    pEboard->w_bitmap &= ~ClearMask;
    pEboard->u_bitmap &= ~ClearMask;
    pEboard->u_rm_magic_bitmap &= ~ClearMask;
    pEboard->c_rm_magic_bitmap &= ~ClearMask;
    pEboard->c_rm_seq_bitmap &= ~ClearMask;
    pEboard->crc_copy_bitmap &= ~ClearMask;
    pEboard->crc_pvd_metadata_bitmap &= ~ClearMask;

    return;
}
/*****************************************
 * fbe_xor_eboard_clear_mask()
 *****************************************/

/*!**************************************************
 * fbe_xor_update_global_eboard
 * 
 * @brief
 * Updates a global eboard with results of a
 * local eboard.
 *
 * This macro would be used to keep track of
 * a running total of errors while we are
 * looping over some number of calls to fill out
 * a local eboard.
 ***************************************************/
static __inline void fbe_xor_update_global_eboard( fbe_xor_error_t *m_EBoard_global,fbe_xor_error_t *m_EBoard_local)
{
    /* This nacro clears only the bits that we wish to
     * track across multiple calls to XOR.
     *
     * At this time the following bits do not
     * need to be cleared because they are
     * not bits that need to be tabulated
     * across multiple calls to XOR:
     *  m_bitmap
     *  hard_media_err_bitmap
     */
    m_EBoard_global->u_crc_bitmap |= m_EBoard_local->u_crc_bitmap;
    m_EBoard_global->c_crc_bitmap |= m_EBoard_local->c_crc_bitmap;
    m_EBoard_global->u_coh_bitmap |= m_EBoard_local->u_coh_bitmap;
    m_EBoard_global->c_coh_bitmap |= m_EBoard_local->c_coh_bitmap;
    m_EBoard_global->u_ts_bitmap |= m_EBoard_local->u_ts_bitmap;
    m_EBoard_global->c_ts_bitmap |= m_EBoard_local->c_ts_bitmap;
    m_EBoard_global->u_ws_bitmap |= m_EBoard_local->u_ws_bitmap;
    m_EBoard_global->c_ws_bitmap |= m_EBoard_local->c_ws_bitmap;
    m_EBoard_global->u_ss_bitmap |= m_EBoard_local->u_ss_bitmap;
    m_EBoard_global->c_ss_bitmap |= m_EBoard_local->c_ss_bitmap;
    m_EBoard_global->u_n_poc_coh_bitmap |= m_EBoard_local->u_n_poc_coh_bitmap;
    m_EBoard_global->c_n_poc_coh_bitmap |= m_EBoard_local->c_n_poc_coh_bitmap;
    m_EBoard_global->u_poc_coh_bitmap |= m_EBoard_local->u_poc_coh_bitmap;
    m_EBoard_global->c_poc_coh_bitmap |= m_EBoard_local->c_poc_coh_bitmap;
    m_EBoard_global->u_coh_unk_bitmap |= m_EBoard_local->u_coh_unk_bitmap;
    m_EBoard_global->c_coh_unk_bitmap |= m_EBoard_local->c_coh_unk_bitmap;
    m_EBoard_global->crc_raid_bitmap |= m_EBoard_local->crc_raid_bitmap;
    m_EBoard_global->crc_invalid_bitmap |= m_EBoard_local->crc_invalid_bitmap;
    m_EBoard_global->crc_bad_bitmap |= m_EBoard_local->crc_bad_bitmap;
    m_EBoard_global->crc_dh_bitmap |= m_EBoard_local->crc_dh_bitmap;
    m_EBoard_global->crc_klondike_bitmap |= m_EBoard_local->crc_klondike_bitmap;
    m_EBoard_global->crc_lba_stamp_bitmap |= m_EBoard_local->crc_lba_stamp_bitmap;
    m_EBoard_global->media_err_bitmap |= m_EBoard_local->media_err_bitmap;
    m_EBoard_global->corrupt_crc_bitmap |= m_EBoard_local->corrupt_crc_bitmap;
    m_EBoard_global->corrupt_data_bitmap |= m_EBoard_local->corrupt_data_bitmap;
    m_EBoard_global->crc_unknown_bitmap |= m_EBoard_local->crc_unknown_bitmap;
    m_EBoard_global->crc_single_bitmap |= m_EBoard_local->crc_single_bitmap;
    m_EBoard_global->crc_multi_bitmap |= m_EBoard_local->crc_multi_bitmap;
    m_EBoard_global->crc_multi_and_lba_bitmap |= m_EBoard_local->crc_multi_and_lba_bitmap;
    m_EBoard_global->zeroed_bitmap |= m_EBoard_local->zeroed_bitmap;
    m_EBoard_global->w_bitmap |= m_EBoard_local->w_bitmap;
    m_EBoard_global->u_bitmap |= m_EBoard_local->u_bitmap;
    m_EBoard_global->u_rm_magic_bitmap |= m_EBoard_local->u_rm_magic_bitmap;
    m_EBoard_global->c_rm_magic_bitmap |= m_EBoard_local->c_rm_magic_bitmap;
    m_EBoard_global->c_rm_seq_bitmap |= m_EBoard_local->c_rm_seq_bitmap;
    m_EBoard_global->crc_copy_bitmap |= m_EBoard_local->crc_copy_bitmap;
    m_EBoard_global->crc_pvd_metadata_bitmap |= m_EBoard_local->crc_pvd_metadata_bitmap;

    return;
}
/****************************************
 * fbe_xor_update_global_eboard()
 ****************************************/

/*!***************************************************************************
 *          fbe_xor_eboard_set_persistent_values() 
 *****************************************************************************
 * 
 * @brief   This method populates the `fixed' (i.e. values that don't change 
 *          when the eboard is updated) values of the XOR eboard.
 *
 * @param   raid_group_object_id_p - Pointer eboard raid group object id to set
 * @param   raid_group_offset_p - Pointer eboard raid group offset to set
 * @param   hard_media_err_bitmap_p - Pointer to hard error bitmap (this is 
 *                                    preserved for the life of the request)
 * @param   retry_err_bitmap_p - Pointer to the retry bitmask to populate
 * @param   raid_group_object_id - Raid group object id to use to populate eboard
 * @param   raid_group_offset - Raid group offset to use to populate eboard
 * @param   hard_media_err_bitmap - Hard error value to populate eboard with
 * @param   retry_err_bitmap - Bitmask of positions that encountered retryable 
 *                             errors
 *
 * @note    Need to update this API if additional fixed values
 *          are added to the eboard.
 *
 * @note    The eboard state is not considered a fixed value.
 *
 * @return  None
 *   
 *****************************************************************************/
static __forceinline void fbe_xor_eboard_set_persistent_values(fbe_object_id_t *const raid_group_object_id_p,
                                                   fbe_lba_t *const raid_group_offset_p,
                                                   fbe_u16_t *const hard_media_err_bitmap_p,
                                                   fbe_u16_t *const retry_err_bitmap_p,
                                                   const fbe_object_id_t raid_group_object_id,
                                                   const fbe_lba_t raid_group_offset,
                                                   const fbe_u16_t hard_media_err_bitmap,
                                                   const fbe_u16_t retry_err_bitmap)

{
    *raid_group_object_id_p = raid_group_object_id;
    *raid_group_offset_p = raid_group_offset;
    *hard_media_err_bitmap_p = hard_media_err_bitmap;
    *retry_err_bitmap_p = retry_err_bitmap;
}
/***********************************************
 * end of fbe_xor_eboard_set_persistent_values()
 ***********************************************/

/*!***************************************************************************
 *          fbe_xor_init_eboard() 
 *****************************************************************************
 * 
 * @brief   Initializing the eboard populates the `persistent' values and sets
 *          the state to `initialized'.
 *
 * @param   eboard_p - Pointer to eboard to `create.
 * @param   raid_group_object_id - Raid group object id to populate eboard with
 * @param   raid_group_offset - Raid group offset to populate eboard with
 * 
 * @note    Need to add any new `fixed' values to the parameter list
 * 
 * @return  None
 *   
 *****************************************************************************/
static __forceinline void fbe_xor_init_eboard(fbe_xor_error_t *const eboard_p, 
                                       const fbe_object_id_t raid_group_object_id,
                                       const fbe_lba_t raid_group_offset)
{
    /* Set the fixed fields then set the state to `initialized'
     */
    fbe_xor_eboard_set_persistent_values(&eboard_p->raid_group_object_id,
                                    &eboard_p->raid_group_offset,
                                    &eboard_p->hard_media_err_bitmap,
                                    &eboard_p->retry_err_bitmap,
                                    raid_group_object_id,
                                    raid_group_offset,
                                    0, /* Initialize with 0 hard media errors */
                                    0  /* Initialize with 0 retryable errors */);

    fbe_xor_error_set_state(eboard_p, FBE_XOR_ERROR_STATE_INITIALIZED);

    return;
}
/* end fbe_xor_init_eboard */

/*!********************************************************
 *          fbe_xor_setup_eboard() 
 **********************************************************
 * 
 * @brief   Setup the fields of the eboard.  Validates
 *          that the eboard has been properly initialized.
 *
 * @param   eboard_p - Pointer to ebaord to initialize
 * @param   u_crc_bitmap - Initial value to set UCRC errors to
 *
 * @return  status - FBE_STATUS_T
 *   
 **********************************************************/
static __forceinline fbe_status_t fbe_xor_setup_eboard(fbe_xor_error_t *const eboard_p,
                                               const fbe_u16_t u_crc_bitmap)
{
    /*! Local allocation for all `persistent' values */
    fbe_object_id_t raid_group_object_id;
    fbe_lba_t       raid_group_offset;
    fbe_u16_t       hard_media_err_bitmap;
    fbe_u16_t       retry_err_bitmap;

    /* Validate that the ebaord has been created properly.
     */
    if (fbe_xor_is_valid_eboard(eboard_p) == FBE_FALSE)
    {
        return FBE_STATUS_NOT_INITIALIZED;
    }

    /* Initialize locals with values that were setup during the creation of
     * the eboard.
     */
    fbe_xor_eboard_set_persistent_values(&raid_group_object_id,
                                         &raid_group_offset,
                                         &hard_media_err_bitmap,
                                         &retry_err_bitmap,
                                         eboard_p->raid_group_object_id,
                                         eboard_p->raid_group_offset,
                                         eboard_p->hard_media_err_bitmap,
                                         eboard_p->retry_err_bitmap);

    /* Now zero the entire eboard
     */
    fbe_zero_memory(eboard_p, sizeof(*eboard_p));

    /* Now restore the `persistent' values
     */
    fbe_xor_eboard_set_persistent_values(&eboard_p->raid_group_object_id,
                                         &eboard_p->raid_group_offset,
                                         &eboard_p->hard_media_err_bitmap,
                                         &eboard_p->retry_err_bitmap,
                                         raid_group_object_id,
                                         raid_group_offset,
                                         hard_media_err_bitmap,
                                         retry_err_bitmap);

    /* Now set the uncorrectable bitmap to the value passed
     */
    eboard_p->u_crc_bitmap = u_crc_bitmap;

    /* Now set the eboard state
     */
    fbe_xor_error_set_state(eboard_p, FBE_XOR_ERROR_STATE_VALID);

    return FBE_STATUS_OK;
}
/* end fbe_xor_setup_eboard() */


/*!************************************************** 
 * fbe_xor_destroy_eboard 
 *   Clear the magic number and set state to destroyed
 ***************************************************/
static __forceinline void fbe_xor_destroy_eboard(fbe_xor_error_t *const eboard_p) 
{
    fbe_xor_error_set_state(eboard_p, FBE_XOR_ERROR_STATE_DESTROYED);
    eboard_p->raid_group_object_id = 0;
    eboard_p->raid_group_offset = FBE_LBA_INVALID;
    return;
}
/* end fbe_xor_create_eboard */

/*!*******************************************************************
 * @struct fbe_raid_verify_error_counts_t
 *********************************************************************
 * @brief This structure keeps track of error counts.
 *
 *********************************************************************/
typedef struct fbe_raid_verify_error_counts_s
{
    fbe_u32_t u_crc_count;        /* Uncorrectable crc errors */
    fbe_u32_t c_crc_count;        /* Correctable crc errors */
    fbe_u32_t u_crc_multi_count;        /* Uncorrectable crc multi-bit errors */
    fbe_u32_t c_crc_multi_count;        /* Correctable crc multi-bit errors */
    fbe_u32_t u_crc_single_count;        /* Uncorrectable crc single-bit errors */
    fbe_u32_t c_crc_single_count;        /* Correctable crc single-bit errors */
    fbe_u32_t u_coh_count;        /* Uncorrectable coherency errors */
    fbe_u32_t c_coh_count;        /* Correctable coherency errors */
    fbe_u32_t u_ts_count;         /* Uncorrectable time stamp errors */
    fbe_u32_t c_ts_count;         /* Correctable time stamp errors */
    fbe_u32_t u_ws_count;         /* Uncorrectable write stamp errors */
    fbe_u32_t c_ws_count;         /* Correctable write stamp errors */
    fbe_u32_t u_ss_count;         /* Uncorrectable shed stamp errors */
    fbe_u32_t c_ss_count;         /* Correctable shed stamp errors */
    fbe_u32_t u_ls_count;         /* Uncorrectable lba stamp errors */
    fbe_u32_t c_ls_count;         /* Correctable lba stamp errors */
    fbe_u32_t u_media_count;         /* Uncorrectable media errors */
    fbe_u32_t c_media_count;         /* Correctable media errors */
    fbe_u32_t c_soft_media_count;    /* Soft media errors*/
    fbe_u32_t retryable_errors;      /* Errors we could retry. */
    fbe_u32_t non_retryable_errors;  /* Errors we could not retry */
    fbe_u32_t shutdown_errors;       /* Error where we went shutdown. */
    fbe_u32_t u_rm_magic_count;      /* Uncorrectable raw mirror magic num errors. */
    fbe_u32_t c_rm_magic_count;      /* Correctable raw mirror magic num errors. */
    fbe_u32_t c_rm_seq_count;        /* Correctable raw mirror sequence num errors. */
    fbe_u32_t invalidate_count;      /* # of detected invalidated blocks plus # of blocks we invadidated during the verify due to uncorrectable errors*/
}
fbe_raid_verify_error_counts_t;

/*!*******************************************************************
 * @struct fbe_raid_flush_error_counts_t
 *********************************************************************
 * @brief This structure keeps track of error count
 *        during write log flush operations.
 *
 *********************************************************************/
typedef struct fbe_raid_flush_error_counts_s
{
    fbe_raid_verify_error_counts_t verify_error_counts; /* Standard verify counters */
    fbe_u32_t u_hdr_count;        /* Uncorrectable header read errors */
    fbe_u32_t c_hdr_count;        /* Correctable header read errors */
}
fbe_raid_flush_error_counts_t;

/*!*******************************************************************
 * @struct fbe_raid_verify_raw_mirror_errors_t
 *********************************************************************
 * @brief This structure keeps track of error counts and error bitmaps for raw mirrors.
 *
 *********************************************************************/
typedef struct fbe_raid_verify_raw_mirror_errors_s
{
    fbe_raid_verify_error_counts_t  verify_errors_counts;   /* Raw mirror verify error counts. */
    fbe_u16_t raw_mirror_magic_bitmap;   /* Bitmask of raw mirror positions with magic num errors.*/
    fbe_u16_t raw_mirror_seq_bitmap;     /* Bitmask of raw mirror positions with seq num errors. */
}
fbe_raid_verify_raw_mirror_errors_t;

/*!**************************************************
 * @enum fbe_xor_seed_t
 * 
 * @brief Enum for possible seed values
 ***************************************************/
typedef enum fbe_xor_seed_e
{
    FBE_XOR_SEED_UNREFERENCED_VALUE = -1,   /*!< Seed is ignored */
}
fbe_xor_seed_t;

/* Exported from xor_local.h */ 

/*!**************************************************
 * @enum fbe_xor_move_command_t
 * 
 * @brief Enum for checksum and lba stamp setting and gneration options.
 * These are bit values so multiple options can be set at once.
 ***************************************************/
typedef enum fbe_xor_move_command_e
{
    FBE_XOR_MOVE_COMMAND_INVALID            = 0,
    FBE_XOR_MOVE_COMMAND_MEM_TO_MEM512      = 0x07,
    FBE_XOR_MOVE_COMMAND_MEM512_TO_MEM520   = 0x1D,
    FBE_XOR_MOVE_COMMAND_MEM_TO_MEM520      = 0x1E,
    FBE_XOR_MOVE_COMMAND_MEM520_TO_MEM512   = 0x1F,
    FBE_XOR_MOVE_COMMAND_MEM512_8_TO_MEM520 = 0x24,
    FBE_XOR_MOVE_COMMAND_MEM520_TO_MEM520   = 0x26,
    FBE_XOR_MOVE_COMMAND_LAST               = 0x27,
}
fbe_xor_move_command_t;


/*!*******************************************************************
 * @struct fbe_xor_recovery_verify_t
 *********************************************************************
 * @brief This is the structure passed to the xor driver for a
 *        recovery verify xor operation.
 *
 *********************************************************************/
typedef struct fbe_xor_recovery_verify_s
{
    fbe_u16_t parity_pos[FBE_XOR_NUM_PARITY_POS];
    fbe_u16_t rebuild_pos[FBE_XOR_NUM_REBUILD_POS];
    fbe_xor_status_t status;
    fbe_block_count_t count; /*!< Blocks to xor on every drive. */
    fbe_lba_t seed;  /*!< Lba seed */
    fbe_lba_t offset; /*! raid group offset */

    struct fbe_xor_recovery_verify_vectors_s
    {
        fbe_sg_element_t *sg_p;
        fbe_u16_t offset;
        fbe_u16_t bitkey_p;
        fbe_u32_t data_position;
    }
    fru[FBE_XOR_MAX_FRUS];

    /* Ptr to error board used for logging errors.
     */
    fbe_xor_error_t *eboard_p;

    /* Option bits for checking or generating checksums/stamps.
     */
    fbe_xor_option_t option;

    /* Number of data disks in this stripe.
     */
    fbe_u16_t data_disks;

    /* Number of total disks in this stripe.
     */
    fbe_u16_t array_width;

    /* Error regions structure.
     */
    fbe_xor_error_regions_t *eregions_p;

}
fbe_xor_recovery_verify_t;

/*!*******************************************************************
 * @struct fbe_xor_parity_reconstruct_t
 *********************************************************************
 * @brief
 *   Defines the structure needed to do an xor parity reconstruct.
 *
 *********************************************************************/
typedef struct fbe_xor_parity_reconstruct_s
{
    fbe_sg_element_t *sg_p[FBE_XOR_MAX_FRUS];
    fbe_lba_t seed;
    fbe_block_count_t count;
    fbe_u16_t bitkey[FBE_XOR_MAX_FRUS];
    fbe_u16_t offset[FBE_XOR_MAX_FRUS];
    fbe_u16_t data_position[FBE_XOR_MAX_FRUS];
    fbe_u16_t parity_pos[FBE_XOR_NUM_PARITY_POS];
    fbe_u16_t rebuild_pos[FBE_XOR_NUM_REBUILD_POS];
    fbe_u16_t data_disks;
    fbe_xor_status_t status;
    fbe_xor_option_t option;
    fbe_bool_t b_final_recovery_attempt;
    fbe_xor_error_regions_t *eregions_p;
    fbe_xor_error_t *eboard_p;
    fbe_bool_t b_preserve_shed_data;
    fbe_raid_verify_error_counts_t * error_counts_p;
}
fbe_xor_parity_reconstruct_t;
/*!**************************************************
 * @enum fbe_xor_468_flag_t
 * 
 * @brief Enum for flags related to 468.
 ***************************************************/
typedef enum fbe_xor_468_flag_e
{
    FBE_XOR_468_FLAG_NONE                  = 0x00000000,  /*!< No options selected. */
    FBE_XOR_468_FLAG_BVA_WRITE             = 0x00000001,  /*!< This is a BVA write. */
}
fbe_xor_468_flag_t;
/*!*******************************************************************
 * @struct fbe_xor_468_write_t
 *********************************************************************
 * @brief This is the structure passed to the xor library to perform
 *        a 468 write operation.
 *
 *********************************************************************/
typedef struct fbe_xor_468_s
{
    /*! Status of the operation.
     */
    fbe_xor_status_t status;

    /* This is the member for small writes aka 468.
     * This is the execute stamps array.
     * We use an array of structures, so that as we access
     * each index in the array we prefetch the rest of the structure.
     */
    struct xor_command_small_write_fru_s
    {
        fbe_sg_element_t *read_sg;
        fbe_sg_element_t *write_sg;
        fbe_lba_t seed;
        fbe_block_count_t count;
        /* Difference between the read start and the write start. 
         * Read start is always <= write start.  It is only less 
         * than write start when we have an unaligned write with pre-read. 
         */
        fbe_u32_t read_offset;

        /* Extra offset to where the write data starts.
         */
        fbe_u32_t write_offset;
        /* Number of read blocks.
         * This might be bigger than the write count if we have an 
         * unaligned write with pre-read. 
         */
        fbe_block_count_t read_count;

        fbe_u16_t position_mask;
        fbe_u32_t data_position;
    }
    fru[FBE_XOR_MAX_FRUS];

    /* Ptr to error board used for logging errors.
     */
    fbe_xor_error_t *eboard_p;

    /* Option bits for checking or generating checksums/stamps.
     */
    fbe_xor_option_t option;

    fbe_xor_468_flag_t flags; /*!< flags related to this operation. */

    /* Get number of parity that are supported. R5=1, R3=1, R6=2 
     */
    fbe_u32_t parity_disks;

    /* Number of data disks in this operation.
     */
    fbe_u16_t data_disks;

    /* Number of total disks in this stripe.
     */
    fbe_u16_t array_width;

    /* Error regions structure.
     */
    fbe_xor_error_regions_t *eregions_p;

    /* The raid group offset.  The edge now contains the raid group offset
     * (when there is more than (1) raid group bound on a set of disks the 
     * drive lba used by raid will not contain the edge offset).  Since the 
     * LBA stamp must uniquely identify a drive logical block, the offset 
     * must be taken into account when generating and checking the LBA stamp. 
     */
    fbe_lba_t   offset;
} 
fbe_xor_468_write_t;

/*!*******************************************************************
 * @struct fbe_xor_mr3_write_t
 *********************************************************************
 * @brief This is the structure passed to the xor library to perform
 *        a MR3 write operation.
 *
 *********************************************************************/
typedef struct fbe_xor_mr3_write_s
{
    /*! Status of this operation. 
     */
    fbe_xor_status_t status;

    /* This is the member for mr3.
     * This is the execute stamps array.
     * We use an array of structures, so that as we access
     * each index in the array we prefetch the rest of the structure.
     */
    struct mr3_fru_s
    {
        fbe_sg_element_t *sg;
        fbe_lba_t start_lda;
        fbe_lba_t end_lda;
        fbe_block_count_t count;
        fbe_u16_t bitkey;
        fbe_u32_t data_position;
    }
    fru[FBE_XOR_MAX_FRUS];

    /* Ptr to error board used for logging errors.
     */
    fbe_xor_error_t *eboard_p;

    /* Option bits for checking or generating checksums/stamps.
     */
    fbe_xor_option_t option;

    /* Get number of parity that are supported. R5=1, R6=2 
     */
    fbe_u32_t parity_disks;

    /* Number of data disks in this stripe.
     */
    fbe_u16_t data_disks;

    /* Number of total disks in this stripe.
     */
    fbe_u16_t array_width;

    /* Error regions structure.
     */
    fbe_xor_error_regions_t *eregions_p;

    /* The raid group offset.  The edge now contains the raid group offset
     * (when there is more than (1) raid group bound on a set of disks the 
     * drive lba used by raid will not contain the edge offset).  Since the 
     * LBA stamp must uniquely identify a drive logical block, the offset 
     * must be taken into account when generating and checking the LBA stamp. 
     */
    fbe_lba_t   offset;
} 
fbe_xor_mr3_write_t;

/*!*******************************************************************
 * @struct fbe_xor_468_write_t
 *********************************************************************
 * @brief This is the structure passed to the xor library to perform
 *        a 468 write operation.
 *
 *********************************************************************/
typedef struct fbe_xor_rcw_s
{
    /*! Status of the operation.
     */
    fbe_xor_status_t status;

    /* This is the member for small writes aka 468.
     * This is the execute stamps array.
     * We use an array of structures, so that as we access
     * each index in the array we prefetch the rest of the structure.
     */
    struct xor_command_write_fru_s
    {
        fbe_sg_element_t *read_sg;
        fbe_sg_element_t *write_sg;
        fbe_sg_element_t *read2_sg;
        fbe_lba_t seed;
        fbe_block_count_t count;
        /* Offsets for unaligned writes
         */
        fbe_u32_t write_offset;
        fbe_u32_t write_end_offset;

        /* Number of read blocks.
         * This might be bigger than the write count if we have an 
         * unaligned write with pre-read. 
         */
        fbe_block_count_t read_count;
        fbe_block_count_t write_count;
        fbe_block_count_t read2_count;

        fbe_u16_t position_mask;
        fbe_u32_t data_position;
    }
    fru[FBE_XOR_MAX_FRUS];

    /* Ptr to error board used for logging errors.
     */
    fbe_xor_error_t *eboard_p;

    /* Option bits for checking or generating checksums/stamps.
     */
    fbe_xor_option_t option;

    fbe_xor_468_flag_t flags; /*!< flags related to this operation. */

    /* Get number of parity that are supported. R5=1, R3=1, R6=2 
     */
    fbe_u32_t parity_disks;

    /* Number of data disks in this operation.
     */
    fbe_u16_t data_disks;

    /* Number of total disks in this stripe.
     */
    fbe_u16_t array_width;

    /* Error regions structure.
     */
    fbe_xor_error_regions_t *eregions_p;

    /* The raid group offset.  The edge now contains the raid group offset
     * (when there is more than (1) raid group bound on a set of disks the 
     * drive lba used by raid will not contain the edge offset).  Since the 
     * LBA stamp must uniquely identify a drive logical block, the offset 
     * must be taken into account when generating and checking the LBA stamp. 
     */
    fbe_lba_t   offset;
} 
fbe_xor_rcw_write_t;

/*!*******************************************************************
 * @struct fbe_xor_execute_stamps_t
 *********************************************************************
 * @brief This is the structure passed to the xor library to perform
 *        a check/set stamps operation.
 *
 *********************************************************************/
typedef struct fbe_xor_execute_stamps_s
{
    /*! Status of this operation. 
     */
    fbe_xor_status_t status;

    /* This is the execute stamps array.
     * We use an array of structures, so that as we access
     * each index in the array we prefetch the rest of the structure.
     */
    struct execute_stamps_fru_s
    {
        fbe_sg_element_t *sg;
        fbe_lba_t seed;
        fbe_block_count_t count;
        fbe_u32_t offset;
        fbe_u16_t position_mask;
    }
    fru[FBE_XOR_MAX_FRUS];

    /* Ptr to error board used for logging errors.
     */
    fbe_xor_error_t * eboard_p;
    /* Option bits for checking or generating checksums/stamps.
     */
    fbe_xor_option_t option;

    /* Number of disks to check/set for.
     */
    fbe_u16_t data_disks;

    /* Number of total disks in this stripe.
     */
    fbe_u16_t array_width;

    fbe_block_count_t total_blocks; /*! Used for determining when to prefetch. */

    /* Programmed values for rest of stamps.
     */
    fbe_u16_t ts;
    fbe_u16_t ss;
    fbe_u16_t ws;

    /* Error regions structure.
     */
    fbe_xor_error_regions_t *eregions_p;
} 
fbe_xor_execute_stamps_t;

/*!*******************************************************************
 * @struct fbe_xor_striper_verify_t
 *********************************************************************
 * @brief This is the structure passed to the xor library to perform
 *        a striper verify operation.
 *
 *********************************************************************/
typedef struct fbe_xor_striper_verify_s
{
    /*! Status of the operation.
     */
    fbe_xor_status_t status;

    fbe_lba_t seed;     /*! Lba of this strip. */
    fbe_block_count_t count;    /*! Total blocks to verify. */

    /* We use an array of structures, so that as we access
     * each index in the array we prefetch the rest of the structure.
     */
    struct fbe_xor_striper_verify_fru_s
    {
        fbe_sg_element_t *sg_p;
    }
    fru[FBE_XOR_MAX_FRUS];

    /*! Bit for each fru to indicate which drive it is on.
     */
    fbe_u16_t position_mask[FBE_XOR_MAX_FRUS];

    /*! Number of total disks in this stripe.
     */
    fbe_u16_t array_width;

    fbe_xor_error_regions_t *eregions_p;
    fbe_xor_error_t *eboard_p;
    fbe_raid_verify_error_counts_t * error_counts_p;
    
    fbe_bool_t b_retrying; /*! TRUE if we are doing a retry of an error. */
} 
fbe_xor_striper_verify_t;

/*!*******************************************************************
 * @struct fbe_xor_invalidate_sector_t
 *********************************************************************
 * @brief
 *   Defines the structure needed to do an xor invalidate sector.
 *
 *********************************************************************/
typedef struct fbe_xor_invalidate_sector_s
{
    /*! Status of the operation.
     */
    fbe_xor_status_t status;

    /* We use an array of structures, so that as we access
     * each index in the array we prefetch the rest of the structure.
     */
    struct fbe_xor_invalidate_sector_fru_s
    {
        fbe_sg_element_t *sg_p;  /*! The SG for this fru. */
        fbe_u32_t offset;        /*! Offset into SG. */
        fbe_lba_t seed;          /*! Lba of this strip. */
        fbe_block_count_t count; /*! Total blocks to invalidate. */
    }
    fru[FBE_XOR_MAX_FRUS];

    /*! Number of total disks to invalidate
     */
    fbe_u16_t data_disks;

    /*! Invalidate reason and who 
     */
    xorlib_sector_invalid_reason_t  invalidated_reason;
    xorlib_sector_invalid_who_t     invalidated_by;
}
fbe_xor_invalidate_sector_t;

/*!*******************************************************************
 * @struct fbe_xor_mirror_reconstruct_t
 *********************************************************************
 * @brief
 *   Defines the structure needed to do an xor mirror reconstruct.
 *   Mirror rebuilds and mirror verifys use this structure as does raw mirror I/O.
 *
 *********************************************************************/
typedef struct fbe_xor_mirror_reconstruct_s
{
    fbe_sg_element_t   *sg_p[FBE_XOR_MAX_FRUS];     /*!< Array of sgls for reconstruct request. */
    fbe_lba_t           start_lba;                  /*!< Starting lba for reconstruct request. */
    fbe_block_count_t  blocks;                     /*!< Number of blocks to be reconstructed. */
    fbe_xor_option_t    options;                    /*!< XOR options flags. */
    fbe_u16_t           valid_bitmask;              /*!< Bitmask of position(s) with good data (i.e. data that has been read w/o error) */
    fbe_u16_t           needs_rebuild_bitmask;      /*!< Bitmask of position(s) that either need to be rebuild or failed. */
    fbe_u16_t           width;                      /*!< Number of positions (i.e. raid group width) involved in this request. */
    fbe_xor_error_regions_t *eregions_p;            /*!< Array of error regions generated by request. */
    fbe_xor_error_t    *eboard_p;                   /*!< Pointer to previously allocated eboard. */
    fbe_raid_verify_error_counts_t *error_counts_p; /*!< Pointer to previously allocated error counters. */
    fbe_bool_t          raw_mirror_io_b;            /*!< Used to differentiate between object and raw mirror I/O. */
}
fbe_xor_mirror_reconstruct_t;

/*!*******************************************************************
 * @struct fbe_xor_mem_move_t
 *********************************************************************
 * @brief This is the structure passed to the xor driver for a memory move operation.
 *
 *********************************************************************/
typedef struct fbe_xor_mem_move_s
{

    fbe_xor_status_t    status;
    fbe_xor_option_t    option;
    fbe_u32_t           disks;
    fbe_lba_t           offset;
    fbe_u16_t           parity_pos[FBE_XOR_NUM_PARITY_POS];
    fbe_u16_t           rebuild_pos[FBE_XOR_NUM_REBUILD_POS];

    struct fbe_xor_mem_move_vectors_s
    {
        fbe_sg_element_t *source_sg_p;
        fbe_sg_element_t *dest_sg_p;
        fbe_u32_t source_offset;
        fbe_u32_t dest_offset;
        fbe_block_count_t count;
        fbe_lba_t seed;
    }
    fru[FBE_XOR_MAX_FRUS];

    fbe_xor_error_t *eboard_p;
}
fbe_xor_mem_move_t;

static __inline void fbe_xor_lib_execute_stamps_init(fbe_xor_execute_stamps_t *execute_stamps_p)
{
    execute_stamps_p->status = FBE_XOR_STATUS_INVALID;
    execute_stamps_p->total_blocks = 0;
    execute_stamps_p->ts = 0;
    execute_stamps_p->ws = 0;
    execute_stamps_p->ss = 1;
    return;
}
/*************************************************** 
 * FBE_XOR_LBA_STAMP_PREFETCH_BLOCKS 
 *   Blocks to prefetch for lba stamp checking. 
 ***************************************************/
enum {FBE_XOR_LBA_STAMP_PREFETCH_BLOCKS = 8};

/*************************************************** 
 * XOR_PREFETCH_LBA_STAMP
 *  Perform a prefetch on the lba stamp.
 ***************************************************/
static __forceinline void FBE_XOR_PREFETCH_LBA_STAMP(fbe_sector_t *const sector_p)
{
    fbe_u64_t addr = (fbe_u64_t)(ULONG_PTR)&sector_p->lba_stamp;

    /* If the address is not aligned, then align it.
     */
    if ((addr % 16) != 0)
    {
        addr -= (addr % 16);
    }
#ifdef _AMD64_   
    csx_p_io_prefetch((const char *)addr, CSX_P_IO_PREFETCH_HINT_NTA);
#endif
    return;
}
/*************************************************** 
 * XOR_PREFETCH_LBA_STAMP_PAUSE
 *  Perform a prefetch on the lba stamp and pause.
 *  There is a difference under AMD64 since this
 *  set of instructions we are using is only valid there.
 ***************************************************/
#ifdef _AMD64_
#define FBE_XOR_PREFETCH_LBA_STAMP_PAUSE(m_sector)\
( FBE_XOR_PREFETCH_LBA_STAMP(m_sector), csx_p_atomic_crude_pause() )
#else   
/* In other environments this is not defined.
 */
#define FBE_XOR_PREFETCH_LBA_STAMP_PAUSE(m_sector)\
(FBE_XOR_PREFETCH_LBA_STAMP(m_sector))
#endif

/************************************************************
 * Global Functions
 ************************************************************/

fbe_status_t fbe_xor_library_init(void);
fbe_status_t fbe_xor_library_destroy(void);

/* fbe_xor_interface.c
 */
void fbe_xor_lib_eboard_init(fbe_xor_error_t *const eboard_p);
fbe_object_id_t fbe_xor_lib_eboard_get_raid_group_id(fbe_xor_error_t *const eboard_p);
fbe_status_t fbe_xor_lib_fill_invalid_sector(void *data_p,
                                             fbe_lba_t seed,
                                             xorlib_sector_invalid_reason_t reason,
                                             xorlib_sector_invalid_who_t who);
fbe_status_t fbe_xor_lib_fill_update_invalid_sector(void *data_p,
                                                fbe_lba_t seed,
                                                xorlib_sector_invalid_reason_t reason,
                                                xorlib_sector_invalid_who_t who);
fbe_status_t fbe_xor_lib_fill_invalid_sectors(fbe_sg_element_t * sg_ptr,
                                          fbe_lba_t seed,
                                          fbe_block_count_t blocks,
                                          fbe_u32_t offset,
                                          xorlib_sector_invalid_reason_t reason,
                                          xorlib_sector_invalid_who_t who);
fbe_status_t fbe_xor_lib_execute_invalidate_sector(fbe_xor_invalidate_sector_t *xor_invalidate_p);
fbe_status_t fbe_xor_lib_execute_stamps(fbe_xor_execute_stamps_t * const execute_stamps_p);
fbe_status_t fbe_xor_lib_check_lba_stamp(fbe_xor_execute_stamps_t * const execute_stamps_p);
fbe_u16_t fbe_xor_lib_calculate_checksum(const fbe_u32_t * srcptr);
void fbe_xor_lib_calculate_and_set_checksum(fbe_u32_t * sector_p);
fbe_status_t fbe_xor_lib_validate_data(const fbe_sector_t *const sector_p,
                                       const fbe_lba_t seed,
                                       const fbe_xor_option_t option);
fbe_bool_t fbe_xor_lib_is_lba_stamp_valid(fbe_sector_t * const sector_p, fbe_lba_t lba, fbe_lba_t offset);
fbe_bool_t fbe_xor_lib_is_sector_invalidated(const fbe_sector_t *const sector_p,
                                             xorlib_sector_invalid_reason_t *const reason_p);
fbe_bool_t fbe_xor_lib_is_lba_stamp_valid(fbe_sector_t * const sector_p, fbe_lba_t lba, fbe_lba_t offset);
fbe_status_t fbe_xor_lib_mem_move(fbe_xor_mem_move_t *mem_move_p,
                              fbe_xor_move_command_t move_cmd,
                              fbe_xor_option_t option,
                              fbe_u16_t src_dst_pairs, 
                              fbe_u16_t ts,
                              fbe_u16_t ss,
                              fbe_u16_t ws);
fbe_u16_t fbe_xor_lib_error_get_total_bitmask(fbe_xor_error_t * const eboard_p);
fbe_u16_t fbe_xor_lib_error_get_crc_bitmask(fbe_xor_error_t * const eboard_p);
fbe_u16_t fbe_xor_lib_error_get_wsts_bitmask(fbe_xor_error_t * const eboard_p);
fbe_u16_t fbe_xor_lib_error_get_coh_bitmask(fbe_xor_error_t * const eboard_p);
fbe_status_t fbe_xor_lib_zero_buffer(fbe_xor_zero_buffer_t *zero_buffer_p);
fbe_status_t fbe_xor_lib_zero_buffer_raw_mirror(fbe_xor_zero_buffer_t *zero_buffer_p);
fbe_status_t fbe_xor_lib_validate_mirror_stamps(fbe_sector_t *sector_p, fbe_lba_t lba, fbe_lba_t offset);
fbe_status_t fbe_xor_lib_check_lba_stamp_prefetch(fbe_sg_element_t *sg_ptr, fbe_block_count_t blocks);
fbe_status_t fbe_xor_convert_retryable_reason_to_unretryable(fbe_sg_element_t *sg_p, 
                                                             fbe_block_count_t total_blocks);

/* fbe_xor_interface_striper.c
 */
fbe_status_t fbe_xor_lib_raid0_verify(fbe_xor_striper_verify_t *xor_verify_p);
fbe_status_t fbe_xor_lib_validate_raid0_stamps(fbe_sector_t *sector_p, fbe_lba_t lba, fbe_lba_t offset);

/* fbe_xor_interface_parity.c
 */
fbe_status_t fbe_xor_lib_parity_rebuild(fbe_xor_parity_reconstruct_t *rebuild_p);
fbe_status_t fbe_xor_lib_parity_reconstruct(fbe_xor_parity_reconstruct_t *rc_p);
fbe_status_t fbe_xor_lib_parity_verify(fbe_xor_parity_reconstruct_t *verify_p);
fbe_status_t fbe_xor_lib_parity_468_write(fbe_xor_468_write_t *const write_p);
fbe_status_t fbe_xor_lib_parity_mr3_write(fbe_xor_mr3_write_t *const write_p);
fbe_status_t fbe_xor_lib_parity_rcw_write(fbe_xor_rcw_write_t *const write_p);
fbe_status_t fbe_xor_lib_parity_verify_copy_user_data(fbe_xor_mem_move_t *mem_move_p);
fbe_status_t fbe_xor_lib_parity_recovery_verify_const_parity(fbe_xor_recovery_verify_t *xor_recovery_verify_p);

/* fbe_xor_interface_mirror.c
 */
fbe_status_t fbe_xor_lib_mirror_add_error_region(const fbe_xor_error_t *const eboard_p, 
                                             fbe_xor_error_regions_t *const regions_p, 
                                             const fbe_lba_t lba,
                                             const fbe_block_count_t blocks,
                                             const fbe_u16_t width);
fbe_status_t fbe_xor_lib_mirror_verify(fbe_xor_mirror_reconstruct_t *rebuild_p);
fbe_status_t fbe_xor_lib_mirror_rebuild(fbe_xor_mirror_reconstruct_t *rebuild_p);
fbe_status_t fbe_xor_lib_raw_mirror_sector_set_seq_num(fbe_sector_t *sector_p, 
                                                       fbe_u64_t sequence_num);
fbe_u64_t fbe_xor_lib_raw_mirror_sector_get_seq_num(fbe_sector_t *sector_p);
fbe_status_t fbe_xor_lib_validate_mirror_stamps(fbe_sector_t *sector_p, fbe_lba_t lba, fbe_lba_t offset);
fbe_status_t fbe_xor_lib_validate_raw_mirror_stamps(fbe_sector_t *sector_p, fbe_lba_t lba, fbe_lba_t offset);

/* fbe_xor_sector_trace.c
 */
fbe_status_t fbe_xor_lib_sector_trace_raid_sector(const fbe_sector_t *const sector_p);

/*************************
 * end file fbe_xor_api.h
 *************************/

#endif 
