/*******************************************************************************
 * Copyright (C) EMC Corporation, 2003-2015
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargStorageGroups.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing Storage Group IOCTLs to TCD.
 *
 * "Access Logix" was formerly known as "Storage Centric."
 * "Storage Group" was formerly known as a "Virtual Array."
 *
 ******************************************************************************/

#ifndef ScsiTargStorageGroups_h
#define ScsiTargStorageGroups_h 1


#pragma pack(4)

// An SG_LUN defines a single LUN map for a Storage Group.

typedef struct _SG_LUN
{

	ULONGLONG	Lun;			// The LUN requested by the initiator.
	XLU_WWN		Wwn;			// The XLU it maps to.
	BOOLEAN		WriteProtect;	// Is this LUN write-protected?

} SG_LUN, *PSG_LUN;


///////////////////////////////////////////////////////////////////////////////
// A STORAGE_GROUP_PARAM defines the persistent attributes of a Storage Group. 
//
// Revision:
//		Format of this structure.
//
// Key:	The WWN of this Storage Group.
//
// StorageGroupNameLength:
//		Number of valid bytes in the StorageGroupName[] field.
//
// StorageGroupName:
//		Uninterpretted storage set and retrieved by the GUI.  It is intended
//		to be used for a "friendly" name for the user. (Changeable).
//
// GuiParameter:
//		A field stored by Tcd, but owned by the GUI.  (Changeable).
//
// NumLuns:
//		The number of Lun to XLU_WWN mappings. (Changeable).
//
// Shared:
//		Specifies if this VA is shared?
//
// VAMiscFlags:
//		Misc. attributes.
//
// SecondaryID;
//		Last Secondary ID of the VVol, this field is not used for traditional storage groups.
//
// PEMap[MAX_NUMBER_OF_PE_PER_SG];
//		This defines the mapping of PE host LUN to PE XLU WWN
// 
// Reserved:
//		Must be zero.
//
// LunMap[]:
//		Map a 64 bit Lun number to the 128 bit XLU_WWN. (Changeable).
//
///////////////////////////////////////////////////////////////////////////////

#define MAX_NUMBER_OF_PE_PER_SG			2

// STORAGE_GROUP_PE0 and STORAGE_GROUP_PE1 definitions must not be changed. 

typedef enum _STORAGE_GROUP_TYPE {
	
	STORAGE_GROUP_PE0	= 0,
	STORAGE_GROUP_PE1,
	STORAGE_GROUP_USER,
	STORAGE_GROUP_MAX,
	
} STORAGE_GROUP_TYPE;

typedef struct _STORAGE_GROUP_PARAM {

	ULONG		Revision;
	SG_WWN		Key;
	ULONG		StorageGroupNameLength;
	UCHAR		StorageGroupName[K10_MAX_VARRAY_NAME_LEN];
	ULONGLONG	GuiParameter;
	ULONG		NumLuns;
	BOOLEAN		Shared;
	UCHAR		Resv[3];
	ULONG		VAMiscFlags;
	ULONGLONG	SecondaryID;
	ULONG		Reserved[6];
	SG_LUN		PEMap[MAX_NUMBER_OF_PE_PER_SG];     // This defines the mapping of PE host LUN to PE XLU WWN.
	SG_LUN		LunMap[1];	                        // This defines the mapping of host LUN to XLU WWN.

} STORAGE_GROUP_PARAM, *PSTORAGE_GROUP_PARAM;

#define VIRTUAL_ARRAY_PARAM		STORAGE_GROUP_PARAM
#define PVIRTUAL_ARRAY_PARAM	PSTORAGE_GROUP_PARAM

#define CURRENT_STORAGE_GROUP_PARAM_REV	7

// Storage Group Param size macro.

#define SG_PARAM_SIZE(VALunCount)					\
	(sizeof(STORAGE_GROUP_PARAM) - sizeof(SG_LUN) +	\
	(VALunCount) * sizeof(SG_LUN))

// K10_STORECENTRIC_TARGET_PTR_LIST allows higher-level software to construct a 
// list using pointers rather than using a strictly serialized structure.

typedef struct _K10_STORECENTRIC_TARGET_PTR_LIST {

	ULONG				Count;		// number of target blocks in pList
	STORAGE_GROUP_PARAM	**ppList;	// List of STORAGE_GROUP_PARAM blocks.

} K10_STORECENTRIC_TARGET_PTR_LIST, *PK10_STORECENTRIC_TARGET_PTR_LIST;

///////////////////////////////////////////////////////////////////////////////
// STORAGE_GROUP_ENTRY may be read for previously configured STORAGE_GROUPs.
///////////////////////////////////////////////////////////////////////////////

typedef struct _STORAGE_GROUP_ENTRY {

	ULONG					Revision;
	ULONG					Reserved[4];
    STORAGE_GROUP_PARAM		Params;

} STORAGE_GROUP_ENTRY, *PSTORAGE_GROUP_ENTRY;

// Storage Group Entry size macro.

#define SG_ENTRY_SIZE(VALunCount)									\
	((sizeof(STORAGE_GROUP_ENTRY) - sizeof(STORAGE_GROUP_PARAM)) +	\
	SG_PARAM_SIZE(VALunCount))

#define CURRENT_STORAGE_GROUP_ENTRY_REV	6

// A STORAGE_GROUP_LIST structure is used to contain an entire list of Storage Groups entries in memory.

typedef struct _STORAGE_GROUP_LIST
{

	ULONG				Count;
	ULONG				Revision;
	STORAGE_GROUP_ENTRY	Info[1];

} STORAGE_GROUP_LIST, *PSTORAGE_GROUP_LIST;


// Count of traditional SGs (as opposed to PE storage groups).
// Note that the count of PEs is stored in the XLU list and not in the SG list. 

#define SG_LIST_TRADITIONAL_SG_COUNT                (GlobalStorageGroupList->Count - GlobalXLUList->NumOfPEs)


// Maximum number of traditional SGs allowed.

#define MAX_TRADITIONAL_STORAGE_GROUPS_PER_ARRAY    (MAX_USER_DEFINED_SGS_PER_ARRAY + ARRAY_DEFINED_SG_COUNT)


// An SGROUP_PARAM_LIST structure is used to to persist a list of Storage Group parameters.

typedef struct _SGROUP_PARAM_LIST
{

	ULONG				Count;
	ULONG				Revision;
	STORAGE_GROUP_PARAM	Info[1];

} SGROUP_PARAM_LIST, *PSGROUP_PARAM_LIST;

// Storage Group List header size macro.

#define SGL_HEADER_SIZE	\
	(sizeof(STORAGE_GROUP_LIST) - sizeof(STORAGE_GROUP_ENTRY))

// A SG_KEY_LIST structure is used to contain 
// an entire list of Storage Group keys.

typedef struct _SG_KEY_LIST {

	ULONG		Revision;
	ULONG		Count;
	SG_WWN		Key[1];

} SG_KEY_LIST, *PSG_KEY_LIST;

// Storage Group Key List size macro.

#define SG_KEY_LSIZE(KeyCount)					\
	(sizeof(SG_KEY_LIST) - sizeof(SG_WWN) +		\
	(KeyCount) * sizeof(SG_WWN))

#define CURRENT_STORAGE_GROUP_KEY_LIST_REV	1

#pragma pack()

#endif
