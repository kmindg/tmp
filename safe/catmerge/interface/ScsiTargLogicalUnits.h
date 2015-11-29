/*******************************************************************************
 * Copyright (C) EMC Corporation, 2003-2015 
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargLogicalUnits.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing XLU IOCTLs to TCD.
 *
 ******************************************************************************/

#ifndef ScsiTargLogicalUnits_h
#define ScsiTargLogicalUnits_h 1

#include "VolumeAttributes.h"


// Enumeration of assign modes

typedef enum _ASSIGN_MODE
{

	AUTO_ASSIGN_DISABLE = 0,
	AUTO_ASSIGN_ENABLE = 1

} ASSIGN_MODE;

// Enumeration of tresspass modes.

typedef enum _TRESPASS_MODE
{

	AUTO_TRESPASS_DISABLE = 0,
	AUTO_TRESPASS_ENABLE = 1

} TRESPASS_MODE;

///////////////////////////////////////////////////////////////////////////////
// An XLU_PARAM structure will exist for each Logical Unit (LU) that
// is to be made visible to attached hosts.  This structure defines how a TxD
// will address and access a diskside device.
//
// Revision:
//		Format of this structure.
//
// Key:	The WWN of the XLU, which is how it is identified.
//
// Type:
//		This is a case-sensitive string.  It can be used by application-level
//		code for descriptive purposes.  It is used by the TCD to determine
//		the type of TxD to look for when mapping requests to the XLU.  ("Disk"
//		means use the TDD;  "Generic" means use a driver whose type is yet to 
//		be defined, but can become linked with the TCD and operate perfectly 
//		well...unless or until it needs driver-type-specific support from the
//		TCD). (Not changeable).
//
// BindTime:
//      The bind time to report in the inquiry page.  0 indicates that the 
//      value returned from the last GET_RAID_INFO hsould be used.
//
// RaidType:
//      The Raid Type to use when generating the inquiry page.  
//      RAID_TYPE_UNKNOWN (i.e. 0) indicates the value returned from the last
//      GET_RAID_INFO should be used.
//
// Spare:
//
// DeviceName:
//		This is the NT-Device name of the storage device for the XLU.  The TxD
//		uses this name to gain access to the device.  
//
// RequestedSize:
//		In 512-byte sectors, this defines the number of blocks to make available
//		to the host.  A value of 0 means all blocks from the device shall be
//		available.  
//		(Changeable while quiesced only).
//
// Offset:
//		In 512-byte sectors, this defines the offset from the beginning of the
//		device where the XLU's storage is to begin.  Typically this will be 0.
//		(Changeable while quiesced only).
//
// XluMiscFlags:
//		Misc. attributes.
//		
// DefaultAssignment:
//      Specifies which SP this XLU is assigned to by default. (Changeable).
//
// AutoTrespass:
//      Specifies if auto trespass is enabled. (Changeable).
//
// AutoAssign:
//      Specifies if auto assign is enabled. (Changeable).
//
// GangId:
//		An ID of a "gang" that this XLU belongs to, or 0.
//
// UserDefineNameo[]:
//		LUN Nice Name.
//
// Reserved*:
//		Must be zero.
///////////////////////////////////////////////////////////////////////////////

typedef struct _XLU_PARAM {

	ULONG			Revision;
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t       Pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
	XLU_WWN			Key;
	CHAR			Type[12];
	ULONG			BindTime;
	UCHAR			RaidType;
	UCHAR			XluMiscFlags;
	CHAR 			Spare[2];
	CHAR			DeviceName[K10_DEVICE_ID_LEN];	// "\Device\CLARiiONdisk0"
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t       Pad2;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
	ULONGLONG		RequestedSize;
	ULONGLONG		Offset;
	SP_ASSIGNMENT	DefaultAssignment;
	TRESPASS_MODE	AutoTrespass;
	ASSIGN_MODE		AutoAssign;
	ULONG			Resv;
	XLU_WWN 		GangId;
	CHAR			UserDefinedName[sizeof(K10_LOGICAL_ID)];  
	ULONG			Reserved[8];

} XLU_PARAM, *PXLU_PARAM;

#define CURRENT_XLU_PARAM_REV	5

///////////////////////////////////////////////////////////////////////////////
// XLU_ENTRY may be read for previously configured XLUs
//
// Revision:
//		Revision of structure.
//
// Params:
//      Persistent information about this XLU.  Params.Key is unique across XLUs.
//
// ActualSize:
//		Actual size of LUN, in 512-byte sectors.
//
// Attached:
//		When the XLU is enabled, Attached becomes TRUE on its very first access.
//		It is then only reset again when the TCD starts up or when the XLU is
//		disabled and all outstanding IO has completed.
//
// Quiesced:
//		Indication if XLU is currently quiesced.
//
// OwningSP:
//		SPA : 0, SPB : 1, UNOWNED : -1
//
// Ready
//		True if volume can accept I/O
//
// VolumeAttributes
//		Bitmap as received from driver stack in response to GET_VOLUME_STATE IOCTL
//
// Reserved:
//		Must be zero.
///////////////////////////////////////////////////////////////////////////////

typedef struct _XLU_ENTRY {

	ULONG					Revision;
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t               Pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
	XLU_PARAM				Params;			  // Unique key identifying XLU.
	ULONGLONG				ActualSize;		  // Actual size of LUN, in 512-byte sectors.
	BOOLEAN 				Attached;		  // Set on first access.
	UCHAR					Quiesced;
	UCHAR					Ready;
	BOOLEAN				    Initialized;		// Initialized and Connected to upper redirector	  
	SP_ASSIGNMENT			OwningSp;	  	
	VOLUME_ATTRIBUTE_BITS	VolumeAttributes; // attributes as reported by GET_VOLUME_STATE
	ULONG					Reserved[3];	  // Must be 0.

} XLU_ENTRY, *PXLU_ENTRY;

#define CURRENT_XLU_ENTRY_REV	7

// An XLU_ENTRY_LIST structure is used to contain a list of XLUs, used by IOCTL_SCSITARG_GET_ALL_XLU_ENTRIES.

typedef struct _XLU_ENTRY_LIST {
	
	ULONG		Count;
	ULONG		Revision;
	XLU_ENTRY	Info[1];
	
} XLU_ENTRY_LIST, *PXLU_ENTRY_LIST;

// XLU entry List size.

#define XLU_ENTRY_LSIZE(XluCount)					\
	(sizeof(XLU_ENTRY_LIST) - sizeof(XLU_ENTRY) +	\
	(XluCount) * sizeof(XLU_ENTRY))

// An XLU_LIST structure is used to contain a list of XLUs in memory.

typedef struct _XLU_LIST {

	ULONG		Count;		// Traditional XLUs + PE XLUs + VVOL XLUs.
	ULONG		NumOfPEs;	// Number of PE XLUs.
	ULONG		NumOfVVols;	// Number of VVOL XLUs.
	ULONG		Revision;
	BOOLEAN     Modified;
	XLU_ENTRY	Info[1];

} XLU_LIST, *PXLU_LIST;

// XLU List size macro.

#define XLU_LSIZE(XluCount)		\
	(sizeof(XLU_LIST) - sizeof(XLU_ENTRY) +	(XluCount) * sizeof(XLU_ENTRY))


// Count of XLUs for traditional LUNs.

#define XLU_LIST_TRADITIONAL_LUN_COUNT      (GlobalXLUList->Count - GlobalXLUList->NumOfVVols - GlobalXLUList->NumOfPEs)


// Maximum number of traditional LUNs allowed.

#define MAX_TRADITIONAL_LUNS                (MAX_POOL_LUS + MAX_ADVANCED_SNAPSHOT_LUNS)


// An XLU_PARAM_LIST structure is used to persist a list of XLU parameters.

typedef struct _XLU_PARAM_LIST {

	ULONG		Count;		// Traditional XLUs + PE XLUs + VVOL XLUs.
	ULONG		NumOfPEs;	// Number of PE XLUs.
	ULONG		NumOfVVols;	// Number of VVOL XLUs.
	ULONG		Revision;
	XLU_PARAM	Info[1];

} XLU_PARAM_LIST, *PXLU_PARAM_LIST;

#define XLU_PARAM_LSIZE(XluCount)	\
	(sizeof(XLU_PARAM_LIST) - sizeof(XLU_PARAM) + (XluCount) * sizeof(XLU_PARAM))


// An XLU_KEY_LIST is used to contain a list of XLU Keys.

#pragma pack(4)

typedef struct _XLU_KEY_LIST {

	ULONG		Revision;
    ULONG		Count;
	XLU_WWN		Key[1];

} XLU_KEY_LIST, *PXLU_KEY_LIST;

#define CURRENT_XLU_KEY_LIST_REV	1

// XLU Key List size.

#define XLU_KEY_LSIZE(KeyCount)					\
	(sizeof(XLU_KEY_LIST) - sizeof(XLU_WWN) +	\
	(KeyCount) * sizeof(XLU_WWN))
		
#pragma pack()

#endif
