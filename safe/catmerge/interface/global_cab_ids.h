#ifndef GLOBAL_CAB_IDS_H
#define GLOBAL_CAB_IDS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * $Id:$
 ***************************************************************************
 *
 * DESCRIPTION:
 *   This header file contains cabinet IDs
 *
 * NOTES:
 *
 * HISTORY:
 *
 * $Log: global_cab_ids.h,v $
 * Revision 1.1  2000/02/09 18:05:28  fladm
 * User: hopkins
 * Reason: Trunk_EPR_Fixes
 * EPR 3632 - New file containing cabinet IDs
 *
 ***************************************************************************/

/*
 * LOCAL TEXT unique_file_id[] = "$Header: $"
 */

/***********************************************************************
 * IMPORTANT  !!  IMPORTANT  !!  IMPORTANT  !!  IMPORTANT  !!  IMPORTANT
 *
 * This file contains global data that is useful to other projects
 * outside of Flare.  The values defined here represent an interface
 * to the outside world and are shared by the other groups.  As a result
 * a few rules apply when modifying this file.
 *
 *  1 - Do not delete (ever) a value defined in this file
 *
 *  2 - Only #define's of new values are allowed in this file
 *
 *  3 - Only add values of like data (error code, id value, etc.)
 *      to this file.
 *
 *  4 - Try to understand how values and ranges are being allocated
 *      before adding new entries to ensure they are added in the 
 *      correct area.
 *
 *  5 - If you have any doubts consult project or group leaders
 *      before making a permanent addition.
 *
 ************************************************************************/
/*******************************
 * LITERAL DEFINITIONS
 *******************************/

/* The following Cabinet ID's are used in pages 25 and 26.
 */
#define AEP_CAB_EQUINOX          0  /* OBSOLETE */
#define AEP_CAB_FAT_ELMO_RACK    1
#define AEP_CAB_FAT_ELMO_TOWER   2
#define AEP_CAB_BABY_ELMO_RACK   3
#define AEP_CAB_BABY_ELMO_TOWER  4
#define AEP_CAB_NUC_TOWER        5
#define AEP_CAB_NUC_RACK         6

/*
 * Leave a gap for flare9 growth
 */
#define AEP_CAB_K5_HORNETS_NEST 0x20
#define AEP_CAB_K1              0x21

/* Agents need to be able to distinguish
 * a K10 chassis by referring to only the
 * chassis ID in mode page 26.  We define
 * that here even though the chassis is NOT
 * different from any other SP in a DPE.
 */
#define AEP_CAB_DPE_ONBOARD_AGENT                       0x22

/* 2GB Longbow DPE */
#define AEP_CAB_DPE_2GB                                 0x23

/* 2GB Drive enclosure ("DAE") 15-slot (used for X-1 SPs) */
#define AEP_CAB_DAE_2GB                                 0x24

/* XPE enclosure for Chameleon II XPs */
#define AEP_CAB_XPE                                     0x25

/* Piranha enclosure - 12 slot */
#define AEP_CAB_PIRANHA                                 0x26

/* Piranha2 enclosure - 12 slot SATA 2 */
#define AEP_CAB_PIRANHA2                                0x27

/* XPE2 enclosure for HammerHead */
#define AEP_CAB_ZEPHYR                                  0x28

/* XPE3 enclosure for Sledge/Jackhammer */
#define AEP_CAB_ZOMBIE                                  0x29


/* Bigfoot DAE enclosure - 12 slot SATAII/SAS */
#define AEP_CAB_BIGFOOT_DAE                             0x2B

/* Bigfoot DPE enclosure - 15 slot SATAII/SAS */
#define AEP_CAB_BIGFOOT_DPE                             0x2a

/* XPE4 enclosure for Dreadnought */
#define AEP_CAB_FOGBOW                                  0x2C

/* XPE5 enclosure for Trident/Ironclad */
#define AEP_CAB_BROADSIDE                               0x2D

/* DPE enclosure for Magnum */
#define AEP_CAB_HOLSTER                                 0x2E

/*15 drive DPE Enclosure for Sentry*/
#define AEP_CAB_BUNKER                                  0x2F

/*25 drive DPE Enclosure for Sentry*/
#define AEP_CAB_CITADEL                                 0x30

/*
 * New definitions for Transformer Platforms
 */
// Megatron xPE
#define AEP_CAB_RATCHET                                 0x31
// Jetfire 25 drive DPE
#define AEP_CAB_FALLBACK                                0x32
// Beachcomber 12 drive DPE
#define AEP_CAB_BOXWOOD                                 0x33
// Beachcomber 25 drive DPE
#define AEP_CAB_KNOT                                    0x34



#define AEP_CAB_FAT_AMDAS_RACK   0x81
#define AEP_CAB_FAT_AMDAS_TOWER  0x82
#define AEP_CAB_BABY_AMDAS_RACK  0x83
#define AEP_CAB_BABY_AMDAS_TOWER 0x84


/*
 * End $Id:$
 */

#endif
