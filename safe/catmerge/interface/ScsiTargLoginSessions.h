/*******************************************************************************
 * Copyright (C) EMC Corporation, 2003 - 2012
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargLoginSession.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing Login Session IOCTLs to TCD.
 *
 ******************************************************************************/

#ifndef ScsiTargLoginSessionPorts_h 
#define ScsiTargLoginSessionPorts_h 1

#include "spid_types.h"
#include <ipexport.h>

// A LOGIN_SESSION_KEY uniquely identifies a session
// on an Initiator/Target nexus.  For Fibre Channel,
// this is an intitiator/port nexus.  For
// iSCSI, this is an initiator/portal group nexus.
//
// I_T_Key : Uniquely identifies the I_T nexus.
// SessionId: The initiator's session ID. Zero for
// Fibre Channel.

typedef struct _LOGIN_SESSION_KEY {

	I_T_NEXUS_KEY	I_T_Key;
	ULONGLONG		SessionId;	

} LOGIN_SESSION_KEY, *PLOGIN_SESSION_KEY;

///////////////////////////////////////////////////////////////////////////////
//
// A LOGIN_SESSION_PARAM may be persisted to specify how 
// a specific I_T nexus with a specific session number should behave. 
//
// Revision:
//		Format of this structure.
//
// Key:	Unique Key identifying initiator, target, and session number.
//
// Behavior: 
//		Defines the behavior initiators see.  This may default to
//      higher levels in the heirarchy.
//
// Reserved:
//		Must be zeroes.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _LOGIN_SESSION_PARAM {

	ULONG				Revision;
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t           Pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches -- padding to avoid changing on disk structure size */
	LOGIN_SESSION_KEY	Key;
	ULONG				Reserved[4];

} LOGIN_SESSION_PARAM, *PLOGIN_SESSION_PARAM;

#define CURRENT_LOGIN_SESSION_PARAM_REV	7

///////////////////////////////////////////////////////////////////////////////
//
// LOGIN_SESSION_ENTRY may be read for previously configured LOGIN_SESSIONs, or 
// previously unknown initiators that have logged in to the array.
//
// Revision:
//		Revision of data structure.
//
// Params:
//      Persistent information about this LOGIN_SESSION, if "Temporary" == FALSE.  
//
// Temporary:
//		If there is no persistent information stored about this LOGIN_SESSION,
//      this is TRUE.
//
// OnLine:
//		Non-zero when Initiator is logged-in.
//
// PathID:
//		NT bus number (provided with login notification).
//
// TargetID:
//		NT device number (provided with login notification).
//
// SourceID:
//		FC ALPA?
//
// TotalOperations:
//      A count of the number of requests serviced for this initiator.
//
// EffectiveBehavior:
//      The array behavior this login session will see, based on the 
//      heirarchy of LOGIN_SESSION, I_T_NEXUS, INITIATOR, HOST.
//
// ReferencedCmiLunOnNonCmiEnabledPort:
//      Set to TRUE if the initiator referenced LUN 0x3FFF since the
//      last login.
//
// Spare:
//		Must be zeroes.
//
// InitiatorIpAddr/FibreReserved:
//		IP Address of iSCSI initiator; Zeroes for fibre channel.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _LOGIN_SESSION_ENTRY {

	ULONG				Revision;
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t           Pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches -- padding to avoid changing on disk structure size */
	LOGIN_SESSION_PARAM	Params;
	BOOLEAN				Temporary;
	UCHAR				Online;
	UCHAR				PathID;
	UCHAR				TargetID;
	ULONG				SourceID;
	ULONGLONG			TotalOperations;
    INITIATOR_BEHAVIOR	EffectiveBehavior;
    BOOLEAN             ReferencedCmiLunOnNonCmiEnabledPort;
    BOOLEAN             spare[3];
	ULONG				Reserved[4];

	union {

		ULONG				FibreReserved[4];
		IN6_ADDR			InitiatorIpAddr;
	};

#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t           Pad2;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
} LOGIN_SESSION_ENTRY, *PLOGIN_SESSION_ENTRY;

#define CURRENT_LOGIN_SESSION_ENTRY_REV	7

// A LOGIN_SESSION_ENTRY_LIST is used to contain a list of LOGIN_SESSION Entries.

typedef struct _LOGIN_SESSION_ENTRY_LIST {

	ULONG					Revision;
	ULONG					Count;
	LOGIN_SESSION_ENTRY	Entry[1];

} LOGIN_SESSION_ENTRY_LIST, *PLOGIN_SESSION_ENTRY_LIST;

// A LOGIN_SESSION_KEY_LIST is used to contain a list of LOGIN_SESSION Keys.

typedef struct _LOGIN_SESSION_KEY_LIST {

	ULONG				Revision;
	ULONG				Count;
	LOGIN_SESSION_KEY	Key[1];

} LOGIN_SESSION_KEY_LIST, *PLOGIN_SESSION_KEY_LIST;

#define CURRENT_LOGIN_SESSION_KEY_LIST_REV	1

// Returns the number of bytes needed to hold a LOGIN_SESSION_KEY_LIST
// with KeyCount keys.

#define LOGIN_SESSION_KEY_LSIZE(KeyCount)							\
	(sizeof(LOGIN_SESSION_KEY_LIST) - sizeof(LOGIN_SESSION_KEY) +	\
	(KeyCount) * sizeof(LOGIN_SESSION_KEY))

#define LOGIN_SESSION_KEY_EQUAL(a, b)	\
	((a).SessionId == (b).SessionId &&	\
	 I_T_NEXUS_KEY_EQUAL((a).I_T_Key,(b).I_T_Key))

// Returns the number of bytes needed to hold a LOGIN_SESSION_ENTRY_LIST
// with EntryCount entries.

#define LOGIN_SESSION_ENTRY_LSIZE(EntryCount)							\
	(sizeof(LOGIN_SESSION_ENTRY_LIST) - sizeof(LOGIN_SESSION_ENTRY) +	\
	(EntryCount) * sizeof(LOGIN_SESSION_ENTRY))

// For IOCTL_SCSITARG_GET_FAILED_LOGIN_SESSIONS.

#define CURRENT_FAILED_LOGIN_SESSION_KEY_LIST_REV	1
#define MAX_FAILED_LOGIN_SESSIONS_PER_SP			(FE_PORT_COUNT_PER_SP)
#define MAX_FAILED_LOGIN_SESSIONS_PER_ARRAY			(MAX_FAILED_LOGIN_SESSIONS_PER_SP * SP_COUNT_PER_ARRAY)

// FAILED_LOGIN_SESSION_KEY is used to identified failed login attempts. 
// The time stamp records the failed time since January 1, 1601 in 100nanoseconds. 

#define LoginFailedReasonNoFailure					0x0
#define LoginFailedReasonBadInitiator				0x1
#define LoginFailedReasonAlreadyLoggedIn			0x2
#define LoginFailedReasonLackOfLoginSession			0x4
#define LoginFailedReasonLackOfVLU					0x8
#define LoginFailedReasonNoFailureTestOnly			0xFFFFFFFF

typedef struct _FAILED_LOGIN_SESSION_KEY {

	I_T_NEXUS_KEY				ITNexusKey;
	ULONGLONG					SessionId;
	EMCPAL_TIME_100NSECS		LoginFailedTime;
	ULONG						LoginFailedReason;
	ULONG						Reserved[3];

} FAILED_LOGIN_SESSION_KEY, *PFAILED_LOGIN_SESSION_KEY;

// Similar to LOGIN_SESSION_KEY_LIST.

typedef struct _FAILED_LOGIN_SESSION_KEY_LIST {

	ULONG						Revision;
	ULONG						Count;
	FAILED_LOGIN_SESSION_KEY	FLSessionKey[1];

} FAILED_LOGIN_SESSION_KEY_LIST, *PFAILED_LOGIN_SESSION_KEY_LIST;

// Returns the number of bytes needed to hold FAILED_LOGIN_SESSION_KEY_LIST with KeyCount keys.

#define FAILED_LOGIN_SESSION_KEY_LSIZE(KeyCount)									\
	(sizeof(FAILED_LOGIN_SESSION_KEY_LIST) - sizeof(FAILED_LOGIN_SESSION_KEY) +		\
	(KeyCount) * sizeof(FAILED_LOGIN_SESSION_KEY))

// Actual constants for the LOGIN_SESSION_ENTRY.Online field.
// When the Initiator (HBA for FC, portal for iSCSI) is logged-out, 
// the field is set to FALSE.

#define	INITIATOR_LOGGED_IN		TRUE
#define INITIATOR_LOGGING_IN	(INITIATOR_LOGGED_IN + 1)
#define	INITIATOR_RESERVED		(INITIATOR_LOGGING_IN + 1)

#endif
