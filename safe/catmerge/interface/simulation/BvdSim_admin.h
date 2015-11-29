#ifndef BVDSIM_ADMIN_H
#define BVDSIM_ADMIN_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *      This file defines the functional interface to admin.
 *      This file contains APIs to change the state of the volume in volume data table.
 *      Volume data table (VirtualFlareLogicalTable) is a singleton class. These APIs
 *      use instance of this table to modify the state of particular volume in the table.
 *      When VirtualFlareLogicalTable instantiate, it marks all the volume as Ready in table.
 *      When any virtual flare volume opens first time then it reads the ready state of the 
 *      volume from the table and notifies upper leve driver accordingly.
 *   
 *
 *  HISTORY
 *     04/25/2011     Bhavesh Patel     Initial Version
 *
 *
 ***************************************************************************/

extern "C" {
#include "csx_ext.h"
};

#include "simulation/VirtualFlareLogicalTable.h"

// Hard Coded volume key for private volumes so we can track them by key.
#define CDR_KEY     "00000000000000000200000000000000"
#define VAULT_KEY   "01000000000000000200000000000000"
#define PSM_KEY     "02000000000000000200000000000000"
#define FDR_KEY     "03000000000000000200000000000000"

// Forward declaration.
class VirtualFlareLogicalTable;

CSX_MOD_EXPORT VirtualFlareLogicalTable * GetLogicalTableInstance();

VOID CSX_MOD_EXPORT ResetVolumeDataTable();
    
VOID CSX_MOD_EXPORT VolumeDataTable_MarkVolumeReady(UINT_32 volumeId, VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);
VOID CSX_MOD_EXPORT VolumeDataTable_MarkCDRReady(VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);
VOID CSX_MOD_EXPORT VolumeDataTable_MarkVaultReady(VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);

VOID CSX_MOD_EXPORT VolumeDataTable_MarkVolumeNotReady(UINT_32 volumeId, VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);
VOID CSX_MOD_EXPORT VolumeDataTable_MarkCDRNotReady(VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);
VOID CSX_MOD_EXPORT VolumeDataTable_MarkVaultNotReady(VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);

VOID CSX_MOD_EXPORT VolumeDataTable_MarkVolumeDegraded(UINT_32 volumeId, VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);
VOID CSX_MOD_EXPORT VolumeDataTable_MarkVolumePathNotPreferred(UINT_32 volumeId, VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);
VOID CSX_MOD_EXPORT VolumeDataTable_MarkCDRDegraded(VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);
VOID CSX_MOD_EXPORT VolumeDataTable_MarkVaultDegraded(VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);

VOID CSX_MOD_EXPORT VolumeDataTable_MarkVolumeNotDegraded(UINT_32 volumeId, VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);
VOID CSX_MOD_EXPORT VolumeDataTable_MarkVolumePathPreferred(UINT_32 volumeId, VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);
VOID CSX_MOD_EXPORT VolumeDataTable_MarkCDRNotDegraded(VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);
VOID CSX_MOD_EXPORT VolumeDataTable_MarkVaultNotDegraded(VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED);

K10_WWID ConvertStringToWWID(const char * stringWWID);

#endif // BVDSIM_ADMIN_H
