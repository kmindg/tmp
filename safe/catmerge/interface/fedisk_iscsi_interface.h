//**********************************************************************
//.++
//  COMPANY CONFIDENTIAL:
//     Copyright (C) EMC Corporation, 2006
//     All rights reserved.
//     Licensed material - Property of EMC Corporation.
//.--
//**********************************************************************

//**********************************************************************
//.++
// FILE NAME:
//      fedisk_iscsi_interface.h
//
// DESCRIPTION:
//      This module contains structure definitions for IOCTLs
//      supported by the FEDisk driver that are called by the
//      iSCSI admin dll.
//.--
//**********************************************************************

#ifndef _FEDISKISCSIINTERFACEH_
#define _FEDISKISCSIINTERFACEH_

#include "cpd_user.h"
#include "iSCSIZonesInterface.h"

//**********************************************************************
//.++
// TYPE:
//      FEDISK_VERIFY_ZONE_PATH_IN_BUF
//
// DESCRIPTION:
//      Input buffer from FE_DISK_IOCTL_VERIFY_ZONE_PATH
//
// MEMBERS:
//      SpId                Intended SP of operation
//      PhysPortIndex       Index of front end port to use for login attempt
//      VirtPortIndex       Index of virtual port within the physical port
//      ZoneId              ID of Zone within ZoneDB
//      PathNumber          ID of Path within ZoneDB
//
// REVISION HISTORY:
//      26-Jul-04   MHolt       Created.
//.--
//**********************************************************************

typedef struct _FEDISK_VERIFY_ZONE_PATH_IN_BUF
{
    SP_ID           SpId;
    ULONG           PhysPortIndex;
    USHORT          VirtPortIndex;
    ULONG           ZoneId;
    ULONG           PathNumber;

} FEDISK_VERIFY_ZONE_PATH_IN_BUF, *PFEDISK_VERIFY_ZONE_PATH_IN_BUF;

#endif
