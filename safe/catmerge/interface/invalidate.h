#ifndef INVALIDATE_H
#define INVALIDATE_H 0x00000001 /* from common dir */
#define FLARE__STAR__H

/***************************************************************************
 * Copyright (C) Data General Corporation 1989-1998
 * All rights reserved.
 * Licensed material -- property of Data General Corporation
 ***************************************************************************/

/************************************************************************
 * invalidate.h
 ************************************************************************
 *
 * Description:
 *   This is the header file for invalid sector handling.
 *
 * History:
 *   22-Sep-96 Ported from John Percy's changes to DH.  DGM
 *   01-Oct-96 Skip first 8 words in the sector.  DGM
 *   15-Oct-96 Added INV_XIS.  DGM
 *
 *
 ************************************************************************/

/*
 * Includes
 */

#include "generics.h"
#include "core_config.h"

/* Global function prototypes
 */
IMPORT void xor_fill_invalid_sector(void *, LBA_T, UINT_32, UINT_32);

/* Invalidate the sector if it was not already invalidated. Update the counts
 * and current date/time for the blocks that were previously invalidated.
 */ 
IMPORT void xor_fill_update_invalid_sector(void *, LBA_T, UINT_32, UINT_32);

/* Literals
 */
#define INV_MAGIC_NUMBER 0x46494E56     /* "FINV" */


/***************************************************
 * FL_INVALID_REASON_ENUM
 *
 * This enumeration identifies the values
 * for the 'reason' field in the invalid sector.
 *
 * In the enum below, there are "reserved"
 * unused values because this structure is historical
 * and these "unused" values were used for other
 * reasons in the past which are no longer in use.
 *
 * Reasons should never be re-used in the structure below.
 * but new reasons can be added at the end of the enum.
 *
 ***************************************************/

typedef enum FL_INVALID_REASON_ENUM_TAG
{
    /* INV_UNKNOWN is an invalid 'who' value.
     */
    INV_UNKNOWN = 0x00,

    /* INV_RAID_CORRUPT_CRC is used by either DAQ or possibly layered drivers
     * to purposefully mark a sector as corrupt (possibly lost).                                                                                              .
     */
    INV_RAID_CORRUPT_CRC = 0x01,

    /* INV_CLIENT_REQUESTED_INVALIDATE is used by the RAID
     * when a block is intentionally invalidated due to a cache `loosing' blocks.
     */
    INV_CLIENT_REQUESTED_INVALIDATE = 0x02,

    /* INV_DH_SOFT_MEDIA is used by the DH
     * to invalidate a sector after remap.
     */
    INV_DH_SOFT_MEDIA = 0x03,
  
    /* INV_RAID_DETECTED_BAD_CRC is used by RAID to invalidate a block when it
     * has been lost due to a CRC mismatch for a client write request.
     */
    INV_RAID_DETECTED_BAD_CRC = 0x04,

    INV_UNUSED_1 = 0x05,
    INV_UNUSED_2 = 0x06,

    /* INV_VERIFY is used by RAID to invalidate a block when it has been
     * lost due to errors on other drives.
     */
    INV_VERIFY = 0x07,
    
    /* INV_RAID_CORRUPT_DATA is used by RAID to invalidate just a data block
     * due to an IORB_OP_CORRUPT_DATA command.
     */
    INV_RAID_CORRUPT_DATA = 0x08,

    /* INV_RAID_COH is used to corrupt the data but generate correct CRC.
     *      This is only for debug and should not be observed in the field.
     */
    INV_RAID_COH = 0x09,

    /*******************************
     * Add new values here
     *******************************/
    INV_MAX_REASON_ENUM = 0x0A
}
FL_INVALID_REASON_ENUM;


/*****************************************************************************
 * FL_INVALID_WHO_ENUM
 * 
 *****************************************************************************
 * 
 * This enumeration identifies the values for the 'who' field in the invalid
 * sector.
 *
 * The `who' field is used for debugging purposes only.  That is there should
 * be no validation done on the `who' field;
 *
 *****************************************************************************/

typedef enum FL_INVALID_WHO_ENUM_TAG
{
    /* INV_WHO_UNKNOWN. Simply means no specific value
     */
    INV_WHO_UNKNOWN = -1,

    /* INV_WHO_SELF.
     */
    INV_WHO_SELF    = 0,

    /* INV_WHO_BAD_CRC. For debug purposes only
     */
    INV_WHO_BAD_CRC = 0xBADCBADC,   /* Bad CRC*/

    /* INV_WHO_CLIENT.  A client of RAID invalidated this sector.
     */
    INV_WHO_CLIENT  = 0xC11E4714,   /*CLIENT INvalidated*/

    /* INV_WHO_RAID. RAID invalidated this request.
     */
    INV_WHO_RAID    = 0x4A1D7E41 /* RAID VERIfy*/
}
FL_INVALID_WHO_ENUM;

/* Structures
 */
typedef struct invalidate_info
{
    /* NOTE:  The first 8 words in the sector are not
     * used (which means they will be left filled with
     * zero data).  This is to 'hide' the new information
     * we put here from the RAID-3 process (it computes
     * checksums only on the first and last 8 words in
     * a sector).
     */
    UINT_32 zeroed[8];

    UINT_32 magic_number;
    UINT_32 fudge;
    UINT_32 lba;
    UINT_32 timestamp;

    UINT_32 model_number;
    UINT_32 odd_controller;
    UINT_32 flare_rev;
    UINT_32 prom_rev;

    FL_INVALID_REASON_ENUM  reason;
    FL_INVALID_WHO_ENUM     who;
    
    /* hit_invalid_count: The times we have detected the invalid blocks.
     */
    UINT_32 hit_invalid_count;
    /* first_timestamp: The date/time the invalid block was first created.
     */
    UINT_32 first_timestamp;

    /* test_word: This word is used during rderr testing to
     * help invalidate a block.
     */
    UINT_32 test_word;

    /* Store the higher part of the lba if it is over 32 bit.
     */
    UINT_32 lba_hi;
}
INVALIDATE_INFO;

/*!***************************************************************************
 * @enum    FL_INVALID_METHOD_ENUM
 *****************************************************************************
 *
 * @brief   Enumeration of the different types of invalidation methods.
 *
 *****************************************************************************/
typedef enum FL_INVALID_METHOD_ENUM_TAG
{
    FL_INVALID_METHOD_INVALIDATE_DATA_AND_PARITY = 0, /*!< Invalidate any `data lost' data positions and the parity position */
    FL_INVALID_METHOD_INVALIDATE_DATA            = 1, /*!< Only invalidate the `data lost' positions */
    FL_INVALID_NUMBER_OF_INVALIDATE_METHODS
} FL_INVALID_METHOD_ENUM;

/*!***************************************************************************
 * @def     RG_GB_DEFAULT_INVALIDATE_METHOD
 *****************************************************************************
 *
 * @brief   The default invalidation method is to invalidate the `data lost'
 *          data position(s) and to invalidate the parity position.  On RAID-6
 *          we never invalidate parity positions.
 *
 *****************************************************************************/
#define FL_INVALID_METHOD_DEFAULT_INVALIDATE_METHOD (FL_INVALID_METHOD_INVALIDATE_DATA_AND_PARITY)


/*
 * end $Id: invalidate.h,v 1.1 1999/01/05 22:26:38 fladm Exp $
 */
#endif
