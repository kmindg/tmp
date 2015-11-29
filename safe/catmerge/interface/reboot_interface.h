//***************************************************************************
// Copyright (C) EMC Corporation 2010
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// File Name:
//      reboot_interface
//
// Contents:
//      Data structures for accessing Reboot Section of NvRam.
//
// Exported:
//      K10_REBOOT_DATA
//
// Revision History:
//      09/08/10    DM Created to make interface exposed to other drivers
//
//--

#ifndef _REBOOT_INTERFACE_H_
#define _REBOOT_INTERFACE_H_

#ifdef _REBOOT_EXPORTS_
#define REBOOTAPI CSX_CDECLS_BEGIN CSX_MOD_EXPORT
#else
#define REBOOTAPI CSX_CDECLS_BEGIN CSX_MOD_IMPORT
#endif

REBOOTAPI EMCPAL_STATUS rebootIsSystemDegraded(BOOLEAN *pSystemState);

CSX_CDECLS_END
#endif // #ifndef _REBOOT_INTERFACE_H_

// End reboot_interface.h
