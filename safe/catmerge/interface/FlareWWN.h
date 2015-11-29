// FlareWWN.h: Flare WWN construction and decode routines
//
// Copyright (C) 2001	EMC Corporation
//
//
// These routines construct and decode the WWN names that are used for flare components.
// The WWN values are only unique within the array!
//
//	Revision History
//	----------------
//	21 Sep 01	H. Weiner	Initial version.
//	15 Oct 01	H. Weiner	Make exportable
//
//////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------Defines

#ifndef FLARE_WWN_H
#define FLARE_WWN_H 1
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

// false = bad WWN
CSX_MOD_EXPORT bool FlareWWNSanityCheck( K10_WWID wwn );

CSX_MOD_EXPORT DWORD GetWwnBus( K10_WWID wwn );

CSX_MOD_EXPORT DWORD GetWwnEnclosure( K10_WWID wwn );

CSX_MOD_EXPORT DWORD GetWwnSlot( K10_WWID wwn );

// In order that WWNs be consistent, this field is set to 0 for hardware components
// that are SP independent
CSX_MOD_EXPORT DWORD GetWwnSP( K10_WWID wwn );

CSX_MOD_EXPORT DWORD GetWwnType( K10_WWID wwn );

// For disks, this is the FRU #, for RAID groups, it is the RG ID, etc.
CSX_MOD_EXPORT DWORD GetWwnIdNum( K10_WWID wwn );

CSX_MOD_EXPORT K10_WWID EncodeFlareWWN(DWORD dwBus, DWORD dwEnclosure, DWORD dwSlot,
							  DWORD dwSP, DWORD dwNumber, DWORD dwType);

#endif
		
