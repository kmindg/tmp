/*******************************************************************
 * Copyright (C) EMC Corporation 2006
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/
/***************************************************************************
 *  SharedSizes.h
 ***************************************************************************
 *
 *
 * DESCRIPTION
 *  This file contains global definitions of stripe size related constants
 *  to facilitate sharing them between kernel and user space modules
 *
 * HISTORY:
 *  01/12/2005 - CHH.  Created.
 *  09/09/2008 - RDP.  Update to support Block Size Virtualization requirements.
 *  10/10/2008 - RDP.  Scatter gather list with offset is no longer required
 *                     for Block Size Virtualization.  Set the offsets to 0.
 ***************************************************************************/

/* This constant defines that maximum number of data SGs in a single
 * SGL.  This is choosen for buffer space and alignment restrictions.
 */
#define BM_MAX_SG_DATA_ELEMENTS         128

/* These are the `extra' pre and post SG entries that are consumed by the
 * logical drive object based on physical block size.
 */
#define LOGICAL_DRIVE_MAX_512_PRE_SGS   1
#define LOGICAL_DRIVE_MAX_512_POST_SGS  1
#define LOGICAL_DRIVE_MAX_520_PRE_SGS   0
#define LOGICAL_DRIVE_MAX_520_POST_SGS  0
#define LOGICAL_DRIVE_MAX_4K_PRE_SGS    8
#define LOGICAL_DRIVE_MAX_4K_POST_SGS   8

/* We no longer require an offset to support the logical drive. Although
 * the ability to add offset is still supported, the default offset is 0.
 */
#define BM_MAX_PRE_SGS                  (0)
#define BM_MAX_POST_SGS                 (0)

/* This is the count for the terminator element.
 */
#define BM_SG_TERMINATOR_COUNT          1

/* Most consumers will not use the pre and post SGs.  Therefore define a
 * a constant that does not include them.
 */
#define BM_MAX_SG_DATA_AND_TERM_ELEMENTS (BM_MAX_SG_DATA_ELEMENTS + BM_SG_TERMINATOR_COUNT)

/* This is the maximum number of elements a scatter gather list can have.
 * This was originally in bm_types.h
 */
#define BM_MAX_SG_ELEMENTS              (BM_MAX_PRE_SGS + BM_MAX_SG_DATA_ELEMENTS + BM_MAX_POST_SGS + BM_SG_TERMINATOR_COUNT)

/* Define the basic RAID stripe sizes.  These originally came from
	rg_local.h
*/
#define RG_MAX_ELEMENT_SIZE          (BM_MAX_SG_DATA_ELEMENTS)
#define RG_STRIPER_MAX_ELEMENT_SIZE  (RG_MAX_ELEMENT_SIZE * 2)

// define this as there is really no explicit definition
// to handle RAID3 searches in the disk block cleaner.
// This ABSOLUTELY needs to be kept in sync with the max
// size that can be calculated for a bandwidth unit in r5_gen_wr().
#define RAID3_MAX_SEARCH_SIZE 1024

// Block Virtualization Services (BVS) requires the following defines:
#define RAID_MIN_520_BLOCKS_FOR_520_BPS_DRIVE	   1
#define RAID_MIN_520_BLOCKS_FOR_512_BPS_DRIVE	 64
#define RAID_MIN_520_BLOCKS_FOR_4096_BPS_DRIVE	512
#define RAID_MIN_520_BLOCKS_FOR_MEGABYTE_ALIGNMENT	2048
#define LOGICAL_MIN_520_BLOCKS_FOR_DRIVES_SUPPORTED	(RAID_MIN_520_BLOCKS_FOR_4096_BPS_DRIVE)

//Max transfer size Write Same command
#define FBE_WRITE_SAME_MAX_BLOCKS			0x800	//this many blocks = 1 MB


