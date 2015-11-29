/*******************************************************************************
 * Copyright (C) EMC Corporation, 2003-2015
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargGlobals.h
 *
 * This header file contains structures and constants which are needed by multiple
 * Scsitarg header files.
 *
 ******************************************************************************/

#ifndef ScsiTargGlobals
#define ScsiTargGlobals 1

#include "EmcPAL.h"

// Enumeration of Initiator Types.

typedef enum _INITIATOR_TYPE
{

	CLIENT_FIBRE_OPEN = 0,			// No deviations, MUST BE ZERO!
	CLIENT_HP_WITH_AUTOTRESPASS,
	CLIENT_HP_WITHOUT_AUTOTRESPASS,
	CLIENT_SGI,
	CLIENT_DELL,
	CLIENT_SCSI2_OPEN,
	CLIENT_FUJITSU_SIEMENS,
	CLIENT_CLARIION_ARRAY,
	CLIENT_CLARIION_ARRAY_CMI,
	CLIENT_CLARIION_ARRAY_NOT_CMI,
	CLIENT_HP_SAUNA_WITH_AUTOTRESPASS,
	CLIENT_COMPAQ_TRU64,
	CLIENT_HP_ALT_WITH_AUTOTRESPASS,
	CLIENT_HP_ALT_WITHOUT_AUTOTRESPASS,
	CLIENT_CELERRA,				// Behaves same as FIBRE_OPEN
	CLIENT_LAST,
	CLIENT_FIBRE_OPEN_RP,		// used by admin as temporary storage for Recoverpoint initiator types
	                     		// admin will convert initiator to CLIENT_FIBRE_OPEN for hostside IOCTLs
	                     		// added after CLIENT_LAST so all valid initiator types are still < CLIENT_LAST
	USE_DEFAULT_INITIATOR_TYPE = 0x80000000 // This is a negative number!

} INITIATOR_TYPE;

// Enumeration of initial assignment of a lun (traditional LUN, PE or VVol).

typedef enum _SP_ASSIGNMENT
{

	ASSIGN_SP_NONE = -1,
	ASSIGN_SP_A = 0,
	ASSIGN_SP_B = 1

} SP_ASSIGNMENT;


///////////////////////////////////////////////////////////////////////////////
// The TI_OBJECT_ID structure contains information about a Parallel SCSI or
// Fibre Channel initiator/target.
//
// NOTE: For Fibre Channel, administrative programs must NOT byteswap the
// elements.  But the integer values used for parallel SCSI must be byteswapped 
// when passing through the administrative interface.
///////////////////////////////////////////////////////////////////////////////

typedef struct _TI_OBJECT_ID
{

	union {

		struct {

			ULONGLONG	IdHigh;	// PS: 0, FC: Node World Wide Name.
			ULONGLONG	IdLow;	// PS: Init/Target ID, FC: Port World Wide Name.
		};

		K10_WWID	WWNs;

	} u;

} TI_OBJECT_ID, *PTI_OBJECT_ID;

// A Virtual Array is identified by its 128 bit World Wide Name

typedef TI_OBJECT_ID	SG_WWN, *PSG_WWN;

// An XLU is identified by its 128 bit World Wide Name

typedef TI_OBJECT_ID	XLU_WWN, *PXLU_WWN;

// A VVOL is identified by its 128 bit World Wide Name

typedef TI_OBJECT_ID	VVOL_WWN, *PVVOL_WWN;

// A PE is identified by its 128 bit World Wide Name

typedef TI_OBJECT_ID	PE_WWN, *PPE_WWN;

#define WWNS_EQUAL(a,b)		\
	(((a)->u.IdHigh == (b)->u.IdHigh) && ((a)->u.IdLow == (b)->u.IdLow))

#define WWN_EQUAL_ZERO(a)	\
	((a)->u.IdHigh == 0 && (a)->u.IdLow == 0)

#define K10_MAX_HOST_KEY_BYTES 256

//////////////////////////////////////////////////////////////////////////////
// A HOST_KEY uniquely identifies a host.
//
// KeyLength: the number of bytes in Key.
//    0 indicates there is no host.
// Key: 0..KeyLength-1 bytes are the host identifier.

typedef struct _HOST_KEY {

	ULONG	KeyLength;
	CHAR	Key[K10_MAX_HOST_KEY_BYTES];

} HOST_KEY, *PHOST_KEY;

#if __cplusplus
extern "C++" inline bool operator ==(const TI_OBJECT_ID & first, const TI_OBJECT_ID & second) 
{
	return (((first).u.IdHigh == (second).u.IdHigh) && ((first).u.IdLow == (second).u.IdLow));
}
#endif

// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.  (Quoted from "/ddk/inc/devioctl.h".)

#define FILE_DEVICE_SCSITARG			0x80CA	// unique in devioctl.h
#define FILE_DEVICE_DISKTARG			0x80CB	// unique in devioctl.h
#define FILE_DEVICE_TAPETARG			0x80CC	// unique in devioctl.h

// Wildcard constants

#define WC8		0xFF
#define WC64	CSX_CONST_U64(0xFFFFFFFFFFFFFFFF)

// The various CMI Lun definitions

#define CMI_LUN         0x3FFF  
#define CMI_LUN_ACCESS  0x0   //GSP00 Future: AMODE_VOLUME_SET_ADDRESSING

// XLU_STATISTICS keeps track of DLU-level statistics.

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

typedef struct _XLU_STATISTICS {

	ULONG Revision;
#ifdef ALAMOSA_WINDOWS_ENV		
	ULONG ReadRequests;
	ULONG WriteRequests;
#else
	ULONGLONG ReadRequests;
	ULONGLONG WriteRequests;
#endif	
	ULONG WriteSameRequests;
	ULONG CasRequests;
	ULONG CawRequests;
	ULONG UnmapRequests;
	ULONG ExtendedCopySources;
	ULONG ExtendedCopyDestinations;
	ULONG PopulateTokenRequests;
	ULONG WriteUsingTokenRequests;
	ULONGLONG BlocksRead;
	ULONGLONG BlocksWritten;
	ULONG MaxRequestCount;
	ULONG NonZeroQueueArrivals;
	ULONGLONG SumQueueLengths;
	ULONG NumberOfExplicitTrespasses;
	ULONG NumberOfImplicitTrespasses;
	ULONG NonZeroReqCntArrivals;
	EMCPAL_LARGE_INTEGER SumOutstandingRequests;
	EMCPAL_LARGE_INTEGER BusyTicks;		// Kept in TDD in resoultion returned by NT, but
	EMCPAL_LARGE_INTEGER IdleTicks;		// value is converted to 100ms ticks when retrieved
	EMCPAL_LARGE_INTEGER TotalIoTicks;
	EMCPAL_LARGE_INTEGER LastLunOpsOutChangeTime;	// Timestamp corresponding to last change in LunOpsOut

	union 
	{
		EMCPAL_LARGE_INTEGER TimeStamp;	// Keeps TimeStamp of last idle/busy transition
		ULONG SPMaxRequestCount;	// Used to return Sp level Max Outstanding Requests
	};								// NOTE: This value is returned in all GetStat reqeusts
	
	ULONG QFullBusy;
	ULONG CurrentIOCount;			// Required for OBS

} XLU_STATISTICS, *PXLU_STATISTICS;

// Modified Data Mover Request statistics

typedef struct _MDMR_STATISTICS {

	// S_* This volume was the Source of an Extended Copy where the ...
	LONGLONG	S_DegradedAtBothEnds;		// Source and Destination volumes not owned by the local SP.
	LONGLONG	S_DegradedAtSourceOnly;		// Source volume not owned by the local SP.
	LONGLONG	S_DegradedAtDestinationOnly;// Destination volume not owned by the local SP.
	LONGLONG	S_NoDcaReadDueToDDN;		// Source volume advertised DCA Read, but something in driver stack (RP Splitter) changed its mind.
	LONGLONG	S_NoDcaWriteDueToDDN;		// Destination volume advertised DCA Write, but something in driver stack (RP Splitter) changed its mind.

	// D_* This volume was the Destination of an Extended Copy where the ...
	LONGLONG	D_DegradedAtBothEnds;		// Source and Destination volumes not owned by the local SP.
	LONGLONG	D_DegradedAtSourceOnly;		// Source volume not owned by the local SP.
	LONGLONG	D_DegradedAtDestinationOnly;// Destination volume not owned by the local SP.
	LONGLONG	D_NoDcaReadDueToDDN;		// Source volume advertised DCA Read, but something in driver stack (RP Splitter) changed its mind.
	LONGLONG	D_NoDcaWriteDueToDDN;		// Destination volume advertised DCA Write, but something in driver stack (RP Splitter) changed its mind.

} MDMR_STATISTICS, *PMDMR_STATISTICS;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

#define BASELINE_XLU_STATISTICS_REV		3
#define VAAI_ODX__XLU_STATISTICS_REV	4

#define CURRENT_XLU_STATISTICS_REV		VAAI_ODX__XLU_STATISTICS_REV


// TCD_IO_ACTIVITY to return the IO activity info

typedef struct _TCD_IO_ACTIVITY
{

	ULONG		MaximumIOs;
	ULONG		ActiveIOs;

} TCD_IO_ACTIVITY, *PTCD_IO_ACTIVITY;


// Used to mark Varrays and XLUs as internal only
#define HS_TYPE_INTERNAL	        0x01
// Used to indicate that the XLU was originally put into ~physical
#define HS_TYPE_NOT_IN_PHYSICAL	    0x02

// Used to indicate that XLU is for PE
#define HS_TYPE_PE				    0x04

// Used to indicate that XLU is for VVol
#define HS_TYPE_VVOL			    0x08

// Traditional LUNs and SGs
#define HS_TYPE_TRADITIONAL_LUN     0
#define HS_TYPE_TRADITIONAL_SG      0

// Masks to get the LUN type or SG type  
#define HS_TYPE_LUN_MASK            (HS_TYPE_PE | HS_TYPE_VVOL)
#define HS_TYPE_SG_MASK             (HS_TYPE_PE | HS_TYPE_VVOL)

#endif
