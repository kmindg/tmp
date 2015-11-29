/*******************************************************************************
 * Copyright (C) EMC Corporation, 2003
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargITNexus.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing I T Nexus IOCTLs to TCD.
 *
 ******************************************************************************/

#ifndef ScsiTargITNexus_h 
#define ScsiTargITNexus_h 1

#include "spid_types.h"
#include <ipexport.h>

typedef struct _I_T_NEXUS_KEY_FIBRE {

	SP_ID				SPId:8;		
	unsigned int		LogicalPortId:8;
	USHORT				Resv;
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t           Pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches -- padding to avoid changing on disk structure size */
	INITIATOR_KEY_FIBRE	fc;

} I_T_NEXUS_KEY_FIBRE, *PI_T_NEXUS_KEY_FIBRE;

typedef struct _I_T_NEXUS_KEY_ISCSI {

	PORTAL_GROUP_KEY	PortalGroupKey;
	INITIATOR_KEY_ISCSI	iscsi;

} I_T_NEXUS_KEY_ISCSI, *PI_T_NEXUS_KEY_ISCSI;

typedef struct _I_T_NEXUS_KEY_FCOE {

	SP_ID				SPId:8;		
	unsigned int		LogicalPortId:8;
	USHORT				Resv;
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t           Pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches -- padding to avoid changing on disk structure size */
	TI_OBJECT_ID		PortWWN;
	INITIATOR_KEY_FIBRE	fcoe;
	
} I_T_NEXUS_KEY_FCOE, *PI_T_NEXUS_KEY_FCOE;


typedef struct _I_T_NEXUS_KEY_SAS {

	SP_ID				SPId:8;		
	unsigned int		LogicalPortId:8;
	USHORT				Resv;
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t           Pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches -- padding to avoid changing on disk structure size */
	INITIATOR_KEY_FIBRE	sas;

} I_T_NEXUS_KEY_SAS, *PI_T_NEXUS_KEY_SAS;

// An I_T_NEXUS_KEY uniquely identifies an
// Initiator/Target nexus.  For Fibre Channel and SAS,
// this is an intitiator/port nexus.  For
// iSCSI, this is an initiator/portal group nexus.
//
// PortalGroupKey:
//    Identifies the portal group.  For Fibre
//    channel, the portal group is 1:1 with the
//    portal which is 1:1 with the port.
//
// fc - if PortalGroupKey.PortType is HOST_FIBRE_CHANNEL,
//    this contains a unique initiator identifier.
//
// iscsi - if PortalGroupKey.PortType is HOST_iSCSI,
//    this contains a unique initiator identifier.
//
// fcoe - if PortalGroupKey.PortType is HOST_FCOE,
//    this contains a unique initiator identifier.
//

typedef struct _I_T_NEXUS_KEY {

	HOST_PORT_TYPE	PortType:16;	
	USHORT			Resv;

#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t           Pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches -- padding to avoid changing on disk structure size */
	union {

		I_T_NEXUS_KEY_FIBRE	fc;
		I_T_NEXUS_KEY_ISCSI	iscsi;
		I_T_NEXUS_KEY_FCOE	fcoe;
		I_T_NEXUS_KEY_SAS	sas;
	};

} I_T_NEXUS_KEY, *PI_T_NEXUS_KEY;

// An I_T_NEXUS_KEY_LIST structure is used to contain a list of Initiator keys.

typedef struct _I_T_NEXUS_KEY_LIST {

	ULONG			Revision;
	ULONG			Count;
	I_T_NEXUS_KEY	Key[1];

} I_T_NEXUS_KEY_LIST, *PI_T_NEXUS_KEY_LIST;

#define CURRENT_I_T_NEXUS_KEY_LIST_REV	1

// Returns the number of bytes needed for an I_T_NEXUS_KEY_LIST
// containing KeyCount keys.

#define I_T_NEXUS_KEY_LSIZE(KeyCount)						\
	(sizeof(I_T_NEXUS_KEY_LIST) - sizeof(I_T_NEXUS_KEY) +	\
	(KeyCount) * sizeof(I_T_NEXUS_KEY))

#define I_T_NEXUS_KEY_FIBRE_EQUAL(a, b)				\
	((a).SPId == (b).SPId &&						\
	 (a).LogicalPortId == (b).LogicalPortId &&		\
	 (a).fc.wwn.u.IdHigh == (b).fc.wwn.u.IdHigh &&	\
	 (a).fc.wwn.u.IdLow == (b).fc.wwn.u.IdLow)

#define I_T_NEXUS_KEY_FCOE_EQUAL(a, b)			\
	((a).SPId == (b).SPId &&						\
	 (a).LogicalPortId == (b).LogicalPortId &&		\
	 (a).PortWWN.u.IdHigh == (b).PortWWN.u.IdHigh &&	\
	 (a).PortWWN.u.IdLow == (b).PortWWN.u.IdLow &&		\
	 (a).fcoe.wwn.u.IdHigh == (b).fcoe.wwn.u.IdHigh &&		\
	 (a).fcoe.wwn.u.IdLow == (b).fcoe.wwn.u.IdLow)


#define I_T_NEXUS_KEY_ISCSI_EQUAL(a, b)					\
	(a).PortalGroupKey == (b).PortalGroupKey &&			\
	 (a).iscsi.KeyLength == (b).iscsi.KeyLength &&		\
	 (_stricmp((a).iscsi.Key, (b).iscsi.Key) == 0)

#define I_T_NEXUS_KEY_SAS_EQUAL(a, b)					\
	((a).SPId == (b).SPId &&							\
	 (a).LogicalPortId == (b).LogicalPortId &&			\
	 (a).sas.wwn.u.IdHigh == (b).sas.wwn.u.IdHigh &&	\
	 (a).sas.wwn.u.IdLow == (b).sas.wwn.u.IdLow)

#define I_T_NEXUS_KEY_EQUAL(a, b)													\
	((a).PortType == (b).PortType &&												\
	  ((a).PortType == HOST_FIBRE_CHANNEL && I_T_NEXUS_KEY_FIBRE_EQUAL((a).fc,(b).fc)) ||	\
	  ((a).PortType == HOST_ISCSI && I_T_NEXUS_KEY_ISCSI_EQUAL((a).iscsi,(b).iscsi)) ||	\
	  ((a).PortType == HOST_FCOE && I_T_NEXUS_KEY_FCOE_EQUAL((a).fcoe,(b).fcoe)) ||  \
	  ((a).PortType == HOST_SAS && I_T_NEXUS_KEY_SAS_EQUAL((a).sas,(b).sas)))

///////////////////////////////////////////////////////////////////////////////
// An I_T_NEXUS_PARAM may be set to specify how an Initiator is to be handled.
//
// Revision:
//		Format of this stucture.
//
// Key:
//      Unique Key identifying initiator object and the portal group.
//
// Behavior: Defines the behavior initiators see.  The I_T nexus
//    behavior is only used if the session specifies that the default
//    should be used.
//
// InitiatorInfoLength:
//		Number of valid bytes in the InitiatorInfo[] field.
//
// InitiatorInfo:
//		Uninterpretted storage set and retrieved by the GUI.  It is intended
//		to be used for Initiator "information." (Changeable).
//
// CreatedByAAS:
//      This record was created by an Advanced Array Set (AAS) SCSI opertation,
//      this field is set to TRUE.  AAS operations that modify this record
//      will leave this field unchanged. IOCTLs that set this record have 
//      complete control over this value, but by convention they should set
//      this field to FALSE.
//
// Spare:
// Reserved:
//		Must be zeroes.
///////////////////////////////////////////////////////////////////////////////

#define I_T_NEXUS_UID_LEN	8
#define I_T_NEXUS_UID_SIZE	(I_T_NEXUS_UID_LEN * sizeof(CHAR))

typedef struct _I_T_NEXUS_PARAM {

	ULONG				Revision;
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t           Pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches -- padding to avoid changing on disk structure size */
	I_T_NEXUS_KEY		Key;
	INITIATOR_BEHAVIOR	Behavior;
	ULONG				InitiatorInfoLength;
	UCHAR				InitiatorInfo[K10_MAX_INITIATOR_INFO_LEN];
	BOOLEAN				CreatedByAAS;
	CHAR				Spare[3];
	CHAR				UID[I_T_NEXUS_UID_LEN];
	ULONG				Reserved[8];	// Must be 0

} I_T_NEXUS_PARAM, *PI_T_NEXUS_PARAM;

#define CURRENT_I_T_NEXUS_PARAM_REV	7

///////////////////////////////////////////////////////////////////////////////
//
// I_T_NEXUS_ENTRY may be read for previously persisted I_T_NEXUS_ENTRYs.
//
// Revision:
//		Revision of data structure.
//
// Params:
//      Persistent information about this IT_NEXUS.
//      Params.Key.PortalGroupKey.PortType specifies HOST_FIBRE_CHANNEL or HOST_ISCSI
//
// MgmtHackTemporary:
// MgmtHackOnline:
// MgmtHackSourceID:
//		Set by admin only.
//
// Reserved:
//		Must be zeroes.
//
///////////////////////////////////////////////////////////////////////////////


typedef struct _I_T_NEXUS_ENTRY {

    ULONG				Revision;
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t           Pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches -- padding to avoid changing on disk structure size */
    I_T_NEXUS_PARAM		Params;

    // The following fields are for private use of the admin code.  They are used
    // to preserve the myth that I_T_NEXUS and LOGIN_SESSION are 1:1, and in fact
    // are a single object.  They are not part of the Hostside interface.
    BOOLEAN				MgmtHackTemporary;
    UCHAR				MgmtHackOnline;
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u8_t           Pad2[6];
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches -- padding to avoid changing on disk structure size */
    ULONGLONG			MgmtHackSourceID;

    ULONG				Reserved[4];

} I_T_NEXUS_ENTRY, *PI_T_NEXUS_ENTRY;

// This enum defines the various revisions of the I_T_NEXUS structure

typedef enum _I_T_NEXUS_ENTRY_REV
{

	I_T_NEXUS_ENTRY_PRE_R16_REV = 6,
	I_T_NEXUS_ENTRY_R16_REV = 7,
	I_T_NEXUS_ENTRY_MAX_REV

} I_T_NEXUS_ENTRY_REV;

// This is the current version of the I_T_NEXUS Structure.
// The CURRENT_I_T_NEXUS_ENTRY_REV define should *NEVER* change, only
// the enumeration I_T_NEXUS_ENTRY_REV should change.
//
// If you want to add a new structure rev, then
// Add it to the above I_T_NEXUS_ENTRY_REV enumeration

#define CURRENT_I_T_NEXUS_ENTRY_REV	(I_T_NEXUS_ENTRY_MAX_REV - 1)

// An I_T_NEXUS_LIST structure is used to contain a list of I T Nexuses.

typedef struct _I_T_NEXUS_LIST {

	ULONG			Count;
	ULONG			Revision;
	I_T_NEXUS_ENTRY	Info[1];

} I_T_NEXUS_LIST, *PI_T_NEXUS_LIST;

// I T Nexus List size macro.

#define I_T_NEXUS_LSIZE(ITNexusCount)		\
	(sizeof(I_T_NEXUS_LIST) - sizeof(I_T_NEXUS_ENTRY) +	(ITNexusCount) * sizeof(I_T_NEXUS_ENTRY))

// An I_T_NEXUS_LIST structure is used to persist a list of I T Nexuses.

typedef struct _I_T_NEXUS_PARAM_LIST {

	ULONG			Count;
	ULONG			Revision;
	I_T_NEXUS_PARAM	Info[1];

} I_T_NEXUS_PARAM_LIST, *PI_T_NEXUS_PARAM_LIST;


// I T Nexus Param List size macro.

#define I_T_NEXUS_PARAM_LSIZE(ITNexusCount)		\
	(sizeof(I_T_NEXUS_PARAM_LIST) - sizeof(I_T_NEXUS_PARAM) + (ITNexusCount) * sizeof(I_T_NEXUS_PARAM))


// TPC_IBLOCK and TPC_INFO_BLOCKS used by the (private) IOCTL_SCSITARG_GET_TPC_LIST_IDS ioctl.

typedef struct _TPC_IBLOCK {

	I_T_NEXUS_KEY	Initiator;
	ULONG			ListIdentifier;
	USHORT			WhichList;
	BOOLEAN			ListIdZeroValid;
	UCHAR			ServiceAction;
	ULONGLONG		LogicalUnitNumber;
	ULONG			AgeInSeconds;

} TPC_IBLOCK, *PTPC_IBLOCK;

typedef struct _TPC_INFO_BLOCKS {

	USHORT		Count;		// Number of valid IBlocks.
	USHORT		Available;	// Number of IBlocks available.
	ULONG		Reserved;	// Reserved; always zero.
	TPC_IBLOCK	IBlock[1];

} TPC_INFO_BLOCKS, *PTPC_INFO_BLOCKS;

// Valid values for TPC_IBLOCK.WhichList

#define	TPC_LIST_AVAILABLE	((USHORT)0x0001)
#define	TPC_LIST_INUSE		((USHORT)0x0002)
#define	TPC_LIST_COMPLETED	((USHORT)0x0003)

// Macro to determine the size of a Third Party Copy Info Block list.

#define TPC_IBLOCK_LSIZE(IBlocks)		\
	(sizeof(TPC_INFO_BLOCKS) - sizeof(TPC_IBLOCK) +	(IBlocks) * sizeof(TPC_IBLOCK))



#if __cplusplus
// Define == operators for all of the types defined in this file.

//inline bool operator ==(const I_T_NEXUS_KEY & f, const I_T_NEXUS_KEY & s) 
//{
//	return  (f.PortalGroupKey == s.PortalGroupKey && f.fc.wwn == s.fc.wwn );
//}

#endif

#endif
