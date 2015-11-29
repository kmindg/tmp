/*******************************************************************************
 * Copyright (C) EMC Corporation, 2003
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargInitiators.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing Initiator IOCTLs to TCD.
 *
 ******************************************************************************/

#ifndef ScsiTargInitiators_h 
#define ScsiTargInitiators_h 1

#include "spid_types.h"
#include <string.h>
#include <ipexport.h>

// An INITIATOR_KEY_FIBRE uniquely identifies a Fibre
// Channel initiator across the set of all Fibre Channel
// ports.  Fibre Channel ports get their uniqueness
// from a registration authority.

typedef struct _INITIATOR_KEY_FIBRE {

	TI_OBJECT_ID	wwn;

} INITIATOR_KEY_FIBRE, *PINITIATOR_KEY_FIBRE;

#define K10_MAX_INITIATOR_KEY_LENGTH 256

// An INITIATOR_KEY_ISCSI uniquely identifies a Fibre
// Channel initiator across the set of all Fibre Channel
// ports.  In iSCSI there are two key formats, one of
// which is based on the domain name.  Note that 
// "unique"ness failures are more likely under this
// scheme than with the Fibre Channel registration 
// approach.
//
// KeyLength: the number of bytes of Key that are used.
// Key:  0..KeyLength-1 bytes are the key to use.

typedef struct _INITIATOR_KEY_ISCSI {

	ULONG	KeyLength;
	CHAR	Key[K10_MAX_INITIATOR_KEY_LENGTH];

} INITIATOR_KEY_ISCSI, *PINITIATOR_KEY_ISCSI;

// A INITIATOR_KEY uniquely identifies an initiator.
// For Fibre Channel, this is by its port WWN.  For
// iSCSI, it is by the initiator name.
//
// PortType:
//	    Defines what type of initiator (iSCSI, Fibre or SAS).
//
// fc:    Fibre Channel specific information.
// iscsi: iSCSI specific information.
// sas: sas specific information.

#pragma pack(4)

typedef struct _INITIATOR_KEY {

	HOST_PORT_TYPE	PortType;		

	union {
		INITIATOR_KEY_FIBRE	fc;
		INITIATOR_KEY_ISCSI	iscsi;
		INITIATOR_KEY_FIBRE	fcoe;
		INITIATOR_KEY_FIBRE	sas;
	};

} INITIATOR_KEY, *PINITIATOR_KEY;

//////////////////////////////////////////////////////////////////////////////
//
// An INITIATOR_PARAM may be set to specify how an Initiator is to be handled.
//
// Revision:
//		Format of this structure.
//
// Key:	Unique Key identifying initiator.
//
// Behavior: Defines the behavior initiators see.  The INITIATOR_PARAM
//		behavior is only used if the session and IT_nexus specify that the default
//		should be used.
//
// HostKey: 
//		Identifies the host object that this initiator is associated with, if any.
//
// ProjectedLogins:
//		Prjected number of logins per intiator. Used to calculate the number of VLUs required for each initiator. 
//		Controlled by gProjectedLoginsPerInitiator. Eventually, it should be from User->CP->Admin->Hostside.
//
// Reserved:
//		Must be zeroes.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _INITIATOR_PARAM {

	ULONG				Revision;
	INITIATOR_KEY		Key;
	INITIATOR_BEHAVIOR	Behavior;
	HOST_KEY		    HostKey;
	ULONG				ProjectedLogins;
	ULONG				Reserved[8];

} INITIATOR_PARAM, *PINITIATOR_PARAM;

#define CURRENT_INITIATOR_PARAM_REV	2

///////////////////////////////////////////////////////////////////////////////
//
// INITIATOR_ENTRY may be read for previously persisted INITIATORs.
//
// Revision:
//		Revision of data structure.
//
// Params:
//      Persistent information about this initiator.
//
// Reserved:
//		Must be zeroes.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _INITIATOR_ENTRY {

	ULONG			Revision;
	INITIATOR_PARAM	Params;
	ULONG			Reserved[4];

} INITIATOR_ENTRY, *PINITIATOR_ENTRY;

#define CURRENT_INITIATOR_ENTRY_REV 1

// An INITIATOR_LIST structure is used to contain a list of Initiators.

typedef struct _INITIATOR_LIST {

	ULONG			Count;
	ULONG			Revision;
	INITIATOR_ENTRY	Info[1];

} INITIATOR_LIST, *PINITIATOR_LIST;


// Initiator List size macro.

#define INITIATOR_LSIZE(InitiatorCount)		\
	(sizeof(INITIATOR_LIST) - sizeof(INITIATOR_ENTRY) +	(InitiatorCount) * sizeof(INITIATOR_ENTRY))


// An INITIATOR_LIST structure is used to contain a list of Initiators.

typedef struct _INITIATOR_PARAM_LIST {

	ULONG			Count;
	ULONG			Revision;
	INITIATOR_PARAM	Info[1];

} INITIATOR_PARAM_LIST, *PINITIATOR_PARAM_LIST;


// Initiator Param List size macro.

#define INITIATOR_PARAM_LSIZE(InitiatorCount)		\
	(sizeof(INITIATOR_PARAM_LIST) - sizeof(INITIATOR_PARAM) +	(InitiatorCount) * sizeof(INITIATOR_PARAM))


// An INITIATOR_KEY_LIST is used to contain a list of Initiator Keys.

typedef struct _INITIATOR_KEY_LIST {

	ULONG			Revision;
	ULONG			Count;
	INITIATOR_KEY	Key[1];

} INITIATOR_KEY_LIST, *PINITIATOR_KEY_LIST;

// Initiator Key List size.

#define INITIATOR_KEY_LSIZE(KeyCount)						\
	(sizeof(INITIATOR_KEY_LIST) - sizeof(INITIATOR_KEY) +	\
	(KeyCount) * sizeof(INITIATOR_KEY))

#pragma pack()

#define CURRENT_INITIATOR_KEY_LIST_REV	1

#define INITIATOR_KEY_EQUAL(a, b) ((a).PortType == (b).PortType &&	\
	(((a).PortType == HOST_ISCSI &&									\
	 (a).iscsi.KeyLength == (b).iscsi.KeyLength &&					\
	 _stricmp((a).iscsi.Key, (b).iscsi.Key) == 0) ||					\
	((a).PortType == HOST_FIBRE_CHANNEL &&										\
	 (a).fc.wwn.u.IdHigh == (b).fc.wwn.u.IdHigh && (a).fc.wwn.u.IdLow == (b).fc.wwn.u.IdLow) ||					\
	((a).PortType == HOST_FCOE &&										\
	 (a).fcoe.wwn.u.IdHigh == (b).fcoe.wwn.u.IdHigh && (a).fcoe.wwn.u.IdLow == (b).fcoe.wwn.u.IdLow)|| \
	((a).PortType == HOST_SAS &&																	\
	 (a).sas.wwn.u.IdHigh == (b).sas.wwn.u.IdHigh && (a).sas.wwn.u.IdLow == (b).sas.wwn.u.IdLow)))

#if __cplusplus
// Define == operators for all of the types defined in this file.

extern "C++" inline bool operator ==(const INITIATOR_KEY_FIBRE & f, const INITIATOR_KEY_FIBRE & s) 
{
	return  (f.wwn == s.wwn);
}
extern "C++" inline bool operator ==(const INITIATOR_KEY_ISCSI & f, const INITIATOR_KEY_ISCSI & s) 
{
	return  (f.KeyLength == s.KeyLength && strncmp(f.Key, s.Key, f.KeyLength)== 0);
}
extern "C++" inline bool operator ==(const INITIATOR_KEY & f, const INITIATOR_KEY & s) 
{
	if  (f.PortType != s.PortType) return false; 

	if (f.PortType == HOST_FIBRE_CHANNEL) {
		return f.fc == s.fc;
	}
	else if (f.PortType == HOST_ISCSI) {
		return f.iscsi == s.iscsi;
	}
	else {
		return false;
	}
}

#endif

#endif
