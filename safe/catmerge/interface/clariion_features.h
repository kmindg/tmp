#ifndef FEATURES_H
#define FEATURES_H

/*******************************************************************************
 * Copyright (C) EMC Corporation 1999-2011
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * features.h
 *
 * This is the header file defining supported features.
 *
 * BECAUSE THIS DECLARES THE STATIC FEATURE TABLE IT SHOULD ONlY
 * BE #INCLUDE'd IN ONE SOURCE FILE.  RIGHT NOW THAT FILE IS ArrayFeatures.c
 *
 * Revision History
 * ----------------
 * 15 Dec 04    P. Caldwell Added FEATURE_KEY_GROUP_VERIFY item, also
 *                        changed FEATURE_KEY_STORAGE_CENTRIC size to 15
 * 26 Jul 05    P. Caldwell Added FEATURE_KEY_BE_LOOP_RECONFIGURATION
 * 16 Jan 06    P. Caldwell Added FEATURE_KEY_PROACTIVE_SPARE
 * 24 May 06    P. Caldwell Added FEATURE_KEY_RAID6
 * 15 May 07    M. Khokhar Added FEATURE_KEY_WCA
 * 30 May 07	M. Khokhar	Add FEATURE_KEY_IPV6
 * 14 Aug 07	D. Hesi		Added FEATURE_KEY_FLEXPORT
 * 26 Nov 07	R. Hicks	Added FEATURE_KEY_DELTA_POLL
 * 27 Feb 08    G. Peterson	Added FEATURE_KEY_FLEXPORT_LEGACY
 *				(temporary value, see DIMS 191946).
 *  4 Mar 08    G. Peterson     Added FEATURE_KEY_ALUA_SCSI2_RESV
 *                              (commented out, feature coming soon)
 * 17 Mar 08	G. Peterson	Removed temporary FlexPorts entry
 * 02 Sep 08	R. Hicks	Added FEATURE_KEY_VLAN.
 * 24 Oct 08	D. Hesi		Added FEATURE_KEY_LUN_SHRINK
 * 10 Nov 08	R. Saravanan	Added New Feature Key for R29 Disk Spin Down Feature
 * 01 Jan 09	R. Saravanan	Removed the support for feature Key for FEATURE_KEY_POWERSAVINGS
 * 09 Jan 09	R. Saravanan	Added back the support for feature Key for FEATURE_KEY_POWERSAVINGS
 * 25 Aug 09	D. Hesi		Added FEATURE_KEY_VIRTUAL_PROVISIONING,
 *								  FEATURE_KEY_COMPRESSION,
 *								  FEATURE_KEY_AUTO_TIERING, and
 *								  FEATURE_KEY_EFD_CACHE
 ******************************************************************************/

#include "scsitarg.h"
#include "global_feature_ids.h"

/*******************************************************************************
 * Feature list table
 ******************************************************************************/



#define FKEY_BASE_STATUS	(FEATURE_KEY_STATUS_SUPPORTED | FEATURE_KEY_STATUS_INCLUDED)

FEATURE_ENTRY feature_tbl[] =
{
    {FEATURE_KEY_STORAGE_CENTRIC, "Storage Centric", 15, FKEY_BASE_STATUS},
    {FEATURE_KEY_RAID_GROUPS, "Raid Groups", 11, FKEY_BASE_STATUS},
    {FEATURE_KEY_LUN_NAMES, "Lun Names", 9, FKEY_BASE_STATUS},
    {FEATURE_KEY_PREZERO, "Pre-Zero", 8, FKEY_BASE_STATUS},
    {FEATURE_KEY_ENHANCED_BIND, "Enhanced Bind", 13, FKEY_BASE_STATUS},
    {FEATURE_KEY_RAID3, "Raid-3", 6, FKEY_BASE_STATUS},
    {FEATURE_KEY_HOST_PORT_SPEEDS, "Host Port Speed", 15, FKEY_BASE_STATUS},
    {FEATURE_KEY_HIDDEN_PSM, "Hidden PSM", 10, FKEY_BASE_STATUS},
    {FEATURE_KEY_PNR, "Passive not Ready", 17, FKEY_BASE_STATUS},
    {FEATURE_KEY_KATANA_DAE, "Katana Enclosure", 16, FKEY_BASE_STATUS},
    {FEATURE_KEY_LUNZ, "LUN_Z", 5, FKEY_BASE_STATUS},
    {FEATURE_KEY_LUN_SN_IN_VPP80, "LUN_SN_IN_VPP80", 15, FKEY_BASE_STATUS},
    {FEATURE_KEY_AAQ_VA, "AAQ Virtual Array Report", 24, FKEY_BASE_STATUS},
    {FEATURE_KEY_GROUP_VERIFY, "Group Verify", 12, FKEY_BASE_STATUS},
    {FEATURE_KEY_BE_LOOP_RECONFIGURATION, "Back End Loop Reconfiguration", 29, FKEY_BASE_STATUS},
    {FEATURE_KEY_PROACTIVE_SPARE, "Proactive Spare", 15, FKEY_BASE_STATUS},
    {FEATURE_KEY_RAID6, "Raid-6", 6, FKEY_BASE_STATUS},
    {FEATURE_KEY_ALUA, "ALUA Active/Active support", 26, FKEY_BASE_STATUS},
    {FEATURE_KEY_WCA, "Write Cache Availability", 24, FKEY_BASE_STATUS},
    {FEATURE_KEY_IPV6, "IPv6", 4, FKEY_BASE_STATUS},
    {FEATURE_KEY_DELTA_POLL, "Delta Poll support", 18, FKEY_BASE_STATUS},
    {FEATURE_KEY_FLEXPORT, "Flexport support", 16, FKEY_BASE_STATUS},
    {FEATURE_KEY_ALUA_SCSI2_RESV, "SCSI-2 Reservations for ALUA", 28, FKEY_BASE_STATUS},
    {FEATURE_KEY_GREATER_THAN_256_LUNs, "> 256 LUNs support", 18, FKEY_BASE_STATUS},
    {FEATURE_KEY_LUN_SHRINK, "LUN Shrink support", 18, FKEY_BASE_STATUS},
    {FEATURE_KEY_POWERSAVINGS, "Disk Spin Down Support", 22, FKEY_BASE_STATUS},
    {FEATURE_KEY_VLAN, "VLAN support", 12, FKEY_BASE_STATUS},
    {FEATURE_KEY_CAS, "Compare And Swap Support", 24, FKEY_BASE_STATUS},
    {FEATURE_KEY_VIRTUAL_PROVISIONING, "Virtual Provisioning support", 28, FKEY_BASE_STATUS},
    {FEATURE_KEY_COMPRESSION, "Compression support", 19, FKEY_BASE_STATUS},
    {FEATURE_KEY_AUTO_TIERING, "Auto Tiering support", 20, FKEY_BASE_STATUS},
    {FEATURE_KEY_EFD_CACHE, "EFD Cache support", 17, FKEY_BASE_STATUS},
    {FEATURE_KEY_FULL_COPY_ACCELERATION, "Full Copy Acceleration", 22, FKEY_BASE_STATUS},
    {FEATURE_KEY_FULL_COPY_ACCELERATION_GT2TB, "Full Copy Acceleration >2TB", 27, FKEY_BASE_STATUS},
	{FEATURE_KEY_OFFLOAD_DATA_XFER, "Offload Data Xfer", 17, FKEY_BASE_STATUS},
    {FEATURE_KEY_MCR, "Multi-Core Raid", 15, FKEY_BASE_STATUS},
    {FEATURE_KEY_VVOL_BLOCK, "VVol Block", 10, FKEY_BASE_STATUS},
    {FEATURE_KEY_INVALID, "", 0, 0},	/* END of TABLE */
};

#endif
