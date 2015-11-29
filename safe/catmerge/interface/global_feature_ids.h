#ifndef GLOBAL_FEATURE_ID_H
#define GLOBAL_FEATURE_ID_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                     global_feature_ids.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *      This header file contains defined feature IDs.
 *
 * NOTES:
 *
 * HISTORY:
 *
 *  9 Feb 00    C. Hopkins	EPR 3632 - New file containing all defined feature IDs
 * 24 Feb 04    B. Keesee       Added Mode Page 0x35 Featre Key
 *  2 May 01	H. Weiner	Add FEATURE_KEY_HOST_PORT_SPEEDS
 * 15 Dec 04	P. Caldwell	Add FEATURE_KEY_GROUP_VERIFY
 * 26 Jul 05	P. Caldwell	Add FEATURE_KEY_BE_LOOP_RECONFIGURATION
 * 16 Jan 06	P. Caldwell	Add FEATURE_KEY_PROACTIVE_SPARE
 * 24 May 06	P. Caldwell	Add FEATURE_KEY_RAID6
 * 15 May 07	M. Khokhar	Added FEATURE_KEY_WCA
 * 30 May 07	M. Khokhar	Add FEATURE_KEY_IPV6
 * 14 Aug 07	D. Hesi		Added FEATURE_KEY_FLEXPORT
 * 26 Nov 07	R. Hicks	Added FEATURE_KEY_DELTA_POLL
 * 27 Feb 08	G. Peterson	Aligned AX/CX Lists to each other
 *  4 Mar 08    G. Peterson     Added FEATURE_KEY_ALUA_SCSI2_RESV
 * 17 Mar 08	G. Peterson	Removed temporary FlexPorts value
 * 02 Sep 08	R. Hicks	Added FEATURE_KEY_VLAN
 * 24 Oct 08	D. Hesi		Added FEATURE_KEY_LUN_SHRINK
 * 10 Nov 08	R. Saravanan	Added FEATURE_KEY_POWERSAVINGS 
 * 01 Jan 09	R. Saravanan	Removed FEATURE_KEY_POWERSAVINGS 
 * 09 Jan 09	R. Saravanan	Included Back FEATURE_KEY_POWERSAVINGS 
 * 20 Mar 09    G. Peterson     Added FEATURE_KEY_POWERSAVINGS (corrected value)
 *                                    FEATURE_KEY_LUN_SHRINK
 *                                    FEATURE_KEY_VLAN
 * 25 Aug 09	D. Hesi		Added FEATURE_KEY_VIRTUAL_PROVISIONING,
 *								  FEATURE_KEY_COMPRESSION,
 *								  FEATURE_KEY_AUTO_TIERING, and
 *								  FEATURE_KEY_EFD_CACHE
 *
 ***************************************************************************/


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
 *  3 - Obtain a new value from Gary Peterson (Hopkinton) before
 *      adding it to this file.
 *
 ************************************************************************/

/*******************************
 * LITERAL DEFINITIONS
 *******************************/

/* FEATURE_ID */
/* WARNING - DO NOT REORDER/CHANGE OR INSERT FEATURE_IDs */
#define FEATURE_KEYS_FLARE_MASK             0x80000000
#define FEATURE_KEY_INVALID                 0x00000000
#define FEATURE_KEY_STORAGE_CENTRIC         0x80000001	/* Virtual arrays & Array and Lun IDs */
#define FEATURE_KEY_NUM_ENCLS               0x80000002
#define FEATURE_KEY_RAID_GROUPS             0x80000003
#define FEATURE_KEY_LUN_NAMES               0x80000004
#define FEATURE_KEY_PREZERO                 0x80000005
#define FEATURE_KEY_WARM_REBOOT             0x80000006
#define FEATURE_KEY_HI5_LUNS                0x80000007
#define FEATURE_KEY_VOD                     0x80000008	/* 80000008 Reserved for Video Flare */
#define FEATURE_KEY_AUTOMODE                0x80000009
#define FEATURE_KEY_OBITUARY                0x8000000A
#define FEATURE_KEY_PAGE_35                 0x8000000B
#define FEATURE_KEY_ENHANCED_BIND           0x8000000C
#define FEATURE_KEY_RAID3                   0x8000000D
#define FEATURE_KEY_HOST_PORT_SPEEDS        0x8000000E
#define FEATURE_KEY_HIDDEN_PSM              0x8000000F
#define FEATURE_KEY_PNR                     0x80000010
#define FEATURE_KEY_KATANA_DAE              0x80000011
#define FEATURE_KEY_LUNZ                    0x80000012
#define FEATURE_KEY_LUN_SN_IN_VPP80         0x80000013
#define FEATURE_KEY_AAQ_VA                  0x80000014
#define FEATURE_KEY_GROUP_VERIFY            0x80000015
#define FEATURE_KEY_BE_LOOP_RECONFIGURATION 0x80000016
#define FEATURE_KEY_PROACTIVE_SPARE         0x80000017
#define FEATURE_KEY_RAID6                   0x80000018
#define FEATURE_KEY_ALUA                    0x80000019
#define FEATURE_KEY_WCA                     0x8000001A
#define FEATURE_KEY_IPV6                    0x8000001B
#define FEATURE_KEY_TIERED_SERVICE_LEVELS   0x8000001C
#define FEATURE_KEY_DELTA_POLL              0x8000001D
#define FEATURE_KEY_FLEXPORT                0x8000001E
#define FEATURE_KEY_ALUA_SCSI2_RESV         0x8000001F
#define FEATURE_KEY_GREATER_THAN_256_LUNs   0x80000020
#define FEATURE_KEY_LUN_SHRINK              0x80000021
#define FEATURE_KEY_POWERSAVINGS            0x80000022
#define FEATURE_KEY_VLAN                    0x80000023
#define FEATURE_KEY_CAS                     0x80000024
#define FEATURE_KEY_VIRTUAL_PROVISIONING    0x80000025
#define FEATURE_KEY_COMPRESSION             0x80000026
#define FEATURE_KEY_AUTO_TIERING            0x80000027
#define FEATURE_KEY_EFD_CACHE               0x80000028
#define FEATURE_KEY_FULL_COPY_ACCELERATION  0x80000029
#define FEATURE_KEY_FULL_COPY_ACCELERATION_GT2TB	0x8000002A
#define FEATURE_KEY_OFFLOAD_DATA_XFER       0x8000002B
#define FEATURE_KEY_MCR                     0x8000002C
#define FEATURE_KEY_VVOL_BLOCK              0x8000002E


/***********************************************************************
 * IMPORTANT  !!  IMPORTANT  !!  IMPORTANT  !!  IMPORTANT  !!  IMPORTANT
 *
 * Obtain a new value from Gary Peterson (Hopkinton) before adding it to this file.
 *
 ************************************************************************/

#endif
