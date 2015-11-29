/*******************************************************************************
 * Copyright (C) EMC Corporation, 2003
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargHosts.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing Host IOCTLs to TCD.
 *
 ******************************************************************************/

#ifndef ScsiTargHosts_h 
#define ScsiTargHosts_h 1

#include "spid_types.h"
#include <ipexport.h>

#define K10_MAX_HOST_INFO_LEN	3072

//////////////////////////////////////////////////////////////////////////////
//
// An HOST_PARAM may be set to specify how a Initiators for that host are to be handled.
//
// Revision:
//		Format of this structure.
//
// Key:
//      Unique Key identifying the host.
//
// Behavior: Defines the behavior initiators see.  The HOST_PARAM
//		behavior is only used if the session and IT_nexus and
//		INITIATOR_PARAM all specify that the default should be used.
//
// Reserved:
//		Must be zeroes.
//
// HostInfoLength/HostInfo: data that may be stored and retreived.
//		It is not manipluated or used by the kernel drivers.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _HOST_PARAM {

	ULONG				Revision;
	HOST_KEY			Key;
	INITIATOR_BEHAVIOR	Behavior;
	ULONG				Reserved[8];	// must be 0
	ULONG				HostInfoLength;
	UCHAR				HostInfo[K10_MAX_HOST_INFO_LEN];

} HOST_PARAM, *PHOST_PARAM;

#define CURRENT_HOST_PARAM_REV	2

///////////////////////////////////////////////////////////////////////////////
//
// HOST_ENTRY may be read for previously persisted HOSTs.
//
// Revision:
//		Revision of data structure.
//
// Params:
//      Persistent information about this host.
//
// Reserved:
//		Must be zeroes.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _HOST_ENTRY {

	ULONG		Revision;
	HOST_PARAM	Params;
	ULONG		Reserved[4];	// must be 0

} HOST_ENTRY, *PHOST_ENTRY;

#define CURRENT_HOST_ENTRY_REV	1

// A HOST_LIST structure is used to contain a list of HOSTs.

typedef struct _HOST_LIST {

	ULONG		Count;
	ULONG		Revision;
	HOST_ENTRY	Info[1];

} HOST_LIST, *PHOST_LIST;

// HOST List size macro.

#define HOST_LSIZE(HostCount)		\
	(sizeof(HOST_LIST) - sizeof(HOST_ENTRY) +	(HostCount) * sizeof(HOST_ENTRY))


// A HOST_PARAM_LIST structure is used to persist a list of HOSTs.

typedef struct _HOST_PARAM_LIST {

	ULONG		Count;
	ULONG		Revision;
	HOST_PARAM	Info[1];

} HOST_PARAM_LIST, *PHOST_PARAM_LIST;


// HOST Param List size macro.

#define HOST_PARAM_LSIZE(HostCount)		\
	(sizeof(HOST_PARAM_LIST) - sizeof(HOST_PARAM) +	(HostCount) * sizeof(HOST_PARAM))


// A HOST_KEY_LIST is used to contain a list of HOST Keys.

#pragma pack(4)

typedef struct _HOST_KEY_LIST {

	ULONG		Revision;
	ULONG		Count;
	HOST_KEY	Key[1];

} HOST_KEY_LIST, *PHOST_KEY_LIST;

// HOST Key List size.

#define HOST_KEY_LSIZE(KeyCount)					\
	(sizeof(HOST_KEY_LIST) - sizeof(HOST_KEY) +		\
	(KeyCount) * sizeof(HOST_KEY))

#pragma pack()

#define CURRENT_HOST_KEY_LIST_REV	1

#define HOST_KEY_EQUAL(a, b) ((a).KeyLength == (b).KeyLength && strcmp((a).Key, (b).Key) == 0)

#if __cplusplus
// Define == operators for all of the types defined in this file.

extern "C++" inline bool operator ==(const HOST_KEY & f, const HOST_KEY & s) 
{
	return  (f.KeyLength == s.KeyLength && strncmp(f.Key, s.Key, f.KeyLength)== 0);
}

#endif

#endif
