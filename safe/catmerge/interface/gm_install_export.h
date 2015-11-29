/***************************************************************************
 * Copyright (C) Data General Corporation 1989-1998
 * All rights reserved.
 * Licensed material -- property of Data General Corporation
 ***************************************************************************/

/****************************************************************
 * gm_install_export.h
 *
 * DESCRIPTION:
 *   This header files contains structures used by both the install process
 *   and Disk Firmware Download. Exported from gm_install.h, except where noted.
 *
 *    Revision History
 *    ----------------
 *    11 Aug 00   D. Zeryck   Initial version.
 *    15 Aug 00   D. Zeryck   Add HEADER_MARKER
 *    26 Sep 00   B. Yang           
 *    26 Apr 05   P. Tolvanen Added FDF_CFLAGS_NO_REBOOT flag
 *  02 Feb 06   P. Tolvanen Added FDF_CFLAGS_TRAILER_PRESENT flag.  Added 'trailer'
 *                record to header space for TLA and comparison revision string.
 *    27 Feb 06   P. Caldwell Replaced reserved field with install_header_size and 
 *                put trailer_offset after install_header_size.
 *****************************************************************/

#define FDF_FILENAME_LEN        80   // Also set in adm_api.h as ADM_OFD_FILENAME_LENGTH

#if !defined (GM_INSTALL_EXPORT_H)
#define GM_INSTALL_EXPORT_H

/* From aep_scsi_install.h                                     */
/* The size of the header info used for Download Drive Microcode
 * is fixed at 8192 bytes.  That is, we prepend a vendor's ucode
 * image with an 8KB header.  It really has nothing to do with how
 * large a sector is.
 */
#define INSTALL_HEADER_SIZE   8192


/***************************************************************************/
/* This is a special marker used to identify a firmware image header block */
/***************************************************************************/

#define HEADER_MARKER                   0x76543210

/***************************************************************************/
/* These are the sizes of the inquiry vendor/product id                    */
/***************************************************************************/

#define INQ_VENDOR_ID_SIZE             8
#define INQ_PRODUCT_ID_SIZE           16
#define INQ_FIRMWARE_REV_SIZE          4

/*
 * While the defined header format only allows for 32 bytes for the drive
 * selection bitmap, the following 8000-odd bytes are unused, so we'll just
 * track FRU_COUNT for now.
 */
#define FDF_MAX_SELECTED_BYTES         (((FRU_COUNT)/(BITS_PER_BYTE))+1)

#if FDF_MAX_SELECTED_BYTES > 8000
#error FDF header does not have room for device selection bitmap
#endif

#define FDF_MAX_SELECTED_DRIVES         1000

#define FDF_CFLAGS_RESERVED            0x01
#define FDF_CFLAGS_SELECT_DRIVE        0x02  /* check drive bitmap for targeted drives              */
#define FDF_CFLAGS_CHECK_REVISION      0x04  /* Compare revision on Drive vs File revision          */
#define FDF_CFLAGS_NO_REBOOT           0x08  /* Don't reboot SP after download (NorthStar)          */
#define FDF_CFLAGS_TRAILER_PRESENT     0x10  /* Indicates if TLA trailer is present in header       */
#define FDF_CFLAGS_DELAY_DRIVE_RESET   0x20  /* Manual Reset of drive required for firmware load    */
#define FDF_CFLAGS_ONLINE              0x40  /* Online firmware download                            */
#define FDF_CFLAGS_FAIL_NONQUALIFIED   0x80  /* Fail if there are non-qualified drives              */

#define FDF_CFLAGS2_CHECK_TLA          0x01  /* TLA # compared between the FDF and the drive when set */
#define FDF_CFLAGS2_FAST_DOWNLOAD      0x02  /* FAST download */
#define FDF_CFLAGS2_TRIAL_RUN          0x10  /* Trial run */

/* Note: "no-reboot" option relevant for NorthStar and Hammer-series. This flag may be obsolete
 *       after 'rolling-update/transaction logging" goes in.
 */   

/*
 * Drive Select Element
 *
 * This structure defines a drive that is selected
 * for drive firmware download.
 */
typedef struct drive_selected_element_s
{
    unsigned char  bus;
    unsigned char  enclosure;
    unsigned char  slot;
    unsigned char  reserved;
}drive_selected_element_t;

/*
 * FDF File Header
 *
 * This structure defines some of the data which we get
 * in the first block of a drive installation.
 */
#pragma pack(4)
typedef struct header_block_struct
{
    csx_u32_t header_marker;
    csx_u32_t image_size;
    unsigned char vendor_id[INQ_VENDOR_ID_SIZE];
    unsigned char product_id[INQ_PRODUCT_ID_SIZE];
    unsigned char fw_revision[INQ_FIRMWARE_REV_SIZE];
    csx_u32_t install_header_size;
    csx_u16_t trailer_offset;
    unsigned char cflags;
    unsigned char cflags2;
    unsigned char cflagsReserved[2];
    csx_u16_t max_dl_drives;
    unsigned char ofd_filename[FDF_FILENAME_LEN];
    csx_u16_t num_drives_selected;
    drive_selected_element_t first_drive;
}
HEADER_BLOCK_STRUCT;
#pragma pack()


/* The size of the Serviceability Meta-block info used
 * for Download Drive Microcode 
 */

/*****************************************************************/
/* This is a special marker used to identify a firmware image    */
/* trailer block embedded within the overall header              */
/*****************************************************************/

#define FDF_TRAILER_MARKER                   0x4E617669


/*****************************************************************/
/* This is a revision number to allow for conversion detection   */ /* should the trailer layout change.                             */
/*****************************************************************/

#define FDF_TRAILER_REV_NUM                  1

/*****************************************************************/
/* These are the sizes for the trailer fields                    */
/*****************************************************************/

#define FDF_TRAILER_TLA_SIZE                 12

/* Note that the current flare revision size is 9 (01/06) */
/* 24 seems a reasonable expansion size (“dddd/bbbb”)     */
#define FDF_TRAILER_REVISION_SIZE            24 

/* Calculated size of trailer pad. Intent is to allow modest expansion of trailer before
 * users of trailer information need to do conversions over checking a Tflags value for 
 * new field addition
*/
#define PAD_SIZE (128-FDF_TRAILER_TLA_SIZE-FDF_TRAILER_REVISION_SIZE-sizeof(long)-sizeof(long)-sizeof(long))
/*
 * FDF File Serviceability Meta-data block
 *
 * This structure defines the information we get
 * in the LAST bytes of a drive installation header.
 */
typedef struct fdf_serviceability_block
{
    unsigned long trailer_marker;
    unsigned long revisionNumber;
    unsigned long Tflags;                                 /* Trailer Flags              */
    unsigned char TLA_number[FDF_TRAILER_TLA_SIZE];       /* left justified             */
    unsigned char fw_revision[FDF_TRAILER_REVISION_SIZE]; /*    "    "                  */
    unsigned char pad[PAD_SIZE];                          /* reserved expansion space   */
}
FDF_SERVICEABILITY_BLOCK;

#define FDF_EXPANSION_PAD  (INSTALL_HEADER_SIZE - \
                            sizeof(HEADER_BLOCK_STRUCT) - \
                            sizeof(FDF_SERVICEABILITY_BLOCK))

/* NOTE: This value should be used ONLY by the make_fdf tool when creating the 
 * 'template' for the fdf download header.  All parsers of the header block should use
 * the 'trailer_offset' value found in the HEADER_BLOCK_STRUCT to locate the actual
 * start of the trailer for the fdf file being accessed.
 */
#define FDF_TRAILER_OFFSET (INSTALL_HEADER_SIZE - sizeof(FDF_SERVICEABILITY_BLOCK))

/*
 * FDF Overlay Header
 *
 * This structure defines the overall layout of the first block of 
 * a drive installation.
 */

typedef struct fdf_header_overlay
{
    HEADER_BLOCK_STRUCT       legacyHeader;
    unsigned char             reservedDriveExpansion[FDF_EXPANSION_PAD];
    FDF_SERVICEABILITY_BLOCK  ServiceabilityBlock; 
}
FDF_HEADER_OVERLAY;

#endif /* GM_INSTALL_EXPORT_H  */






