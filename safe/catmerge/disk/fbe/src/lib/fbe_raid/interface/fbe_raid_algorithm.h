#ifndef FBE_RAID_ALGORITHM_H
#define FBE_RAID_ALGORITHM_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_algorithm.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of raid algorithms.
 *
 * @version
 *   5/19/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*****************************************************************
 *
 * @enum fbe_raid_siots_algorithm_t
 * 
 * @brief fbe_raid_siots_t Algorithm
 *
 * This algorithm is used in the siots to identify the
 * type of operation that is being performed.
 *
 * This algorithm is used in the state machines which may be used
 * for several operations; ie. REBUILD/SHED.
 *
 * *** I M P O R T A N T ***
 * The alogorithm value is logged into the event log.
 * To prevent confusion we need to be VERY CAREFUL about how we add new algorithms.
 * 
 * o New algorithms are always added at the end.
 * o Existing algorithms are never removed.
 * o IMPORTANT:  When a new algorithm is added, please add the
 *               corresponding string to
 *               fbe_raid_debug_siots_algorithm_trace_info
 *
 ******************************************************************/
typedef enum fbe_raid_siots_algorithm_e
{
    RG_NO_ALG,                      /* 0x0 */
    R5_SM_RD,                       /* 0x1 */
    R5_RD,                          /* 0x2 */
    R5_468,                         /* 0x3 */
    R5_MR3,                         /* 0x4 */
    OBSOLETE_R5_ALIGNED_MR3_WR,     /* 0x5 */
    OBSOLETE_ALG_WAS_R5_DEG_WR,     /* 0x6 */
    OBSOLETE_R5_DWR_BKFILL,         /* 0x7 */
    OBSOLETE_R5_DWRBK_468_RECOVERY, /* 0x8 */
    OBSOLETE_R5_DWRBK_MR3_RECOVERY, /* 0x9 */
    R5_DEG_RD,                      /* 0xA */
    OBSOLETE_R5_EXP_WR,             /* 0xB */
    OBSOLETE_R5_SDRD,               /* 0xC */
    R5_VR,                          /* 0xD */
    R5_RB,                          /* 0xE straight rebuild */
    OBSOLETE_R5_RB_READ,            /* 0xF rebuild for read */
    OBSOLETE_ALG_WAS_R5_RB_SHED,    /* 0x10 */
    OBSOLETE_ALG_WAS_R5_RB_BUFFER,  /* 0x11 */
    R5_RD_VR,                       /* 0x12 */
    R5_468_VR,                      /* 0x13 */
    R5_MR3_VR,                      /* 0x14 */
    R5_CORRUPT_DATA,                /* 0x15 */
    RAID0_RD,                       /* 0x16 */
    RAID0_WR,                       /* 0x17 */
    RAID0_VR,                       /* 0x18 */
    OBSOLETE_RAID0_CORRUPT_CRC,     /* 0x19 */
    MIRROR_RD,                      /* 0x1A */
    MIRROR_WR,                      /* 0x1B */
    MIRROR_VR,                      /* 0x1C */
    MIRROR_CORRUPT_DATA,            /* 0x1D */
    MIRROR_RB,                      /* 0x1E */
    MIRROR_RD_VR,                   /* 0x1F */
    OBSOLETE_RG_PASS_THRU,          /* 0x20 */
    OBSOLETE_ALG_WAS_RG_EQUALIZE,   /* 0x21 */
    OBSOLETE_ALG_WAS_R5_EQZ_VR,     /* 0x22 */
    MIRROR_COPY,                    /* 0x23 */
    MIRROR_COPY_VR,                 /* 0x24 */
    OBSOLETE_ALG_WAS_R5_RD_REMAP,   /* 0x25 */
    OBSOLETE_ALG_WAS_MIRROR_RD_REMAP,/* 0x26 */
    OBSOLETE_ALG_WAS_R0_REMAP_RVR,  /* 0x27 */
    OBSOLETE_R5_REMAP_RVR,          /* 0x28 */
    MIRROR_REMAP_RVR,      /* 0x29 */
    OBSOLETE_MIRROR_VR_BLOCKS,      /* 0x2A */
    RG_ZERO,                        /* 0x2B */
    OBSOLETE_RG_SNIFF_VR,           /* 0x2C */
    R5_DEG_MR3,                     /* 0x2D */
    R5_DEG_468,                     /* 0x2E */
    R5_DEG_VR,                      /* 0x2F */
    R5_DRD_VR,                      /* 0x30 */
    MIRROR_VR_WR,                   /* 0x31 */
    RAID0_BVA_VR,                   /* 0x32 */
    OBSOLETE_R5_EXP_RB,             /* 0x33 */
    RAID0_RD_VR,                    /* 0x34 */
    R5_DEG_RVR,                     /* 0x35 */
    MIRROR_PACO,                    /* 0x36 */
    MIRROR_VR_BUF,                  /* 0x37 */
    RG_CHECK_ZEROED,                /* 0x38 */
    RG_FLUSH_JOURNAL,               /* 0x39 */
    R5_SMALL_468,                   /* 0x3A */
    RG_FLUSH_JOURNAL_VR,            /* 0x3B */
    RG_WRITE_LOG_HDR_RD,            /* 0x3C */
    R5_REKEY,                       /* 0x3D */
    MIRROR_REKEY,                   /* 0x3E */
    R5_RCW,                         /* 0x3F */
    R5_DEG_RCW,                     /* 0x40 */
    R5_RCW_VR,                      /* 0x41 */

    /********************************** 
     * ADD NEW ALGORITHMS HERE. 
     * IMPORTANT !!  
     *  When you add or change algorithms, you need to update fbe_raid_debug_siots_algorithm_trace_info 
     **********************************/
    FBE_RAID_SIOTS_ALGORITHM_LAST,   /* 0x42 */
}
fbe_raid_siots_algorithm_e;

/*************************
 * end file fbe_raid_algorithm.h
 *************************/

#endif /* FBE_RAID_ALGORITHM_H */
