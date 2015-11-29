#ifndef FBE_SECTOR_H
#define	FBE_SECTOR_H 0x00000001
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sector.h
 ***************************************************************************
 *
 * @brief
 *  This file contains sector definitions..
 *
 * @version
 *   6/9/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*******************************
 * LITERAL DEFINITIONS
 *******************************/

/* This is the mask used to test/set/clear the
 * bit in the timestamp field which indicates
 * that corresponding sectors on each FRU in the
 * logical unit should share the same timestamp.
 */
#define FBE_SECTOR_ALL_TSTAMPS		0x8000u

/* This is the value of the timestamp field when
 * writestamps or shedstamps are used.
 */
#define FBE_SECTOR_INVALID_TSTAMP	0x7FFFu

/* For Raid 6, we use the FL_INVALID_TSTAMP for the initial
 * timestamp, but we set FL_RAID6_PARITY_INVALID_TSTAMP
 * instead of FL_INVALID_TSTAMP when updating parity.
 *
 * This is used since on RAID 6 we need to know if the
 * initial pattern exists on parity.  By using this
 * time stamp instead of the invalid timestamp, we know
 * that there is data in this stripe.
 */
#define FBE_SECTOR_R6_INVALID_TSTAMP	0x0u

/* This is the value of the timestamp field when
 * a logical unit is first bound. SCSI-products
 * bind with a zero timestamp, but Fibre-products
 * bind with the invalid timestamp
 */
#define FBE_SECTOR_INITIAL_TSTAMP	FBE_SECTOR_INVALID_TSTAMP
#define FBE_520_BLOCKS_PER_4K 8
#define FBE_4K_BYTES_PER_BLOCK (520 * FBE_520_BLOCKS_PER_4K)
#define FBE_BE_BYTES_PER_BLOCK 520
#define FBE_BYTES_PER_MEGABYTE (1024 * 1024)
#define FBE_BLOCKS_PER_MEGABYTE (FBE_BYTES_PER_MEGABYTE / FBE_BYTES_PER_BLOCK) 
#define FBE_BLOCKS_PER_GIGABYTE (FBE_BLOCKS_PER_MEGABYTE * 1024)
#define FBE_BLOCKS_PER_TERRABYTE (FBE_BLOCKS_PER_GIGABYTE * 1024)
#define FBE_BYTES_PER_BLOCK 512

/* Number of bytes per exported block.
 */
#define FBE_FE_BYTES_PER_BLOCK 512

#define FBE_WORDS_PER_BLOCK (FBE_BYTES_PER_BLOCK/sizeof(fbe_u32_t))

/* Number of bytes per block for a raw mirror.
 *
 * It is set to the following:                                                                                                                                                                 .
 *      (sector data - (sequence number + magic number)).                                                                                       .
 */
#define FBE_RAW_MIRROR_DATA_SIZE   (FBE_BYTES_PER_BLOCK - (sizeof(fbe_u64_t) + sizeof(fbe_u64_t)))

/* Number of words per block for raw mirror
 */
#define FBE_RAW_MIRROR_WORDS_PER_BLOCK  (FBE_RAW_MIRROR_DATA_SIZE/sizeof(fbe_u32_t))

/* This is the constant seed passed into the checksum
 * routines to be xor'd against the data in the sector.
 * It is only used when the FIXED_SEED_CHECKSUMMING
 * define is set.
 * NOTE: for debugging purposes, testproc generates sectors
 * that contain all the same data (ie. 128 words of 0x0d231400).
 * If all the data is the same, it all Xor's to 0, and then is
 * xor'd against the seed, so the result is the seed defined
 * below... but wait, we also shift that result left one bit and
 * fold it into a 16 bit value, so the seed below would become
 * 0x00015EEC (shift), and then 0x00005EED (fold).  Therefore,
 * with fixed seed checksumming, all of our sectors will have
 * the value 0x5EED ("SEED") in the lower 16 bits after
 * we finish checksumming.      - MJH
 *
 * NOTE: The use of this constant directly is being discouraged;
 *       use the functions from fl_csum_util.h instead!
 */
#define FBE_SECTOR_CHECKSUM_SEED_CONST  0x0000AF76

/*!*******************************************************************
 * @def FBE_SECTOR_DEFAULT_CHECKSUM
 *********************************************************************
 * @brief This is the default checksum value.
 *
 *********************************************************************/
 #define FBE_SECTOR_DEFAULT_CHECKSUM 0x5eed

/*********************************
 * STRUCTURE DEFINITIONS
 *********************************/

/*!*******************************************************************
 * @struct fbe_raw_mirror_sector_data_t
 *********************************************************************
 * @brief
 *   This is the definition of the data portion of a sector for a raw mirror.
 *
 *********************************************************************/
typedef struct fbe_raw_mirror_sector_data_s
{
    fbe_u32_t data_word[FBE_RAW_MIRROR_WORDS_PER_BLOCK];
    fbe_u64_t sequence_number;
    fbe_u64_t magic_number;
}
fbe_raw_mirror_sector_data_t;

/*!*******************************************************************
 * @struct fbe_sector_t
 *********************************************************************
 * @brief
 *   This is the FBE definition of a sector.
 *
 *********************************************************************/
typedef struct fbe_sector_s
{
    fbe_u32_t data_word[FBE_WORDS_PER_BLOCK];
    fbe_u16_t crc;
    fbe_u16_t time_stamp;
    fbe_u16_t write_stamp;
    fbe_u16_t lba_stamp;
}
fbe_sector_t;

/* Literals
 */


/*********************************
 * EXTERNAL REFERENCE DEFINITIONS
 *********************************/

/************************
 * PROTOTYPE DEFINITIONS
 ************************/

#endif /* def FBE_SECTOR_H */
