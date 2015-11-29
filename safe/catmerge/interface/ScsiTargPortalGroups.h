/*******************************************************************************
 * Copyright (C) EMC Corporation, 2003
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * ScsiTargPortalGroups.h
 *
 * This header file contains structures and constants which are needed by other
 * drivers and applications when performing Portal Group IOCTLs to TCD.
 *
 ******************************************************************************/

#ifndef ScsiTargPortalGroups_h 
#define ScsiTargPortalGroups_h 1

#include "spid_types.h"
#include <ipexport.h>

///////////////////////////////////////////////////////////////////////////////
// A PORTAL_GROUP_KEY uniquely identifies a PORTAL_GROUP.  All portals within
// a portal group must be of the same type and on the same SP.  We are not 
// going to allow for fibre channel portals.

typedef ULONG PORTAL_GROUP_KEY, *PPORTAL_GROUP_KEY;


///////////////////////////////////////////////////////////////////////////////
// A PORTAL_KEY_ISCSI uniquely identifies a PORTAL across all iSCSI portals. 
//
// IpKey: Key to associated IP_PARAM object.
// TcpPortNumber: 

typedef struct _PORTAL_KEY_ISCSI {

	IP_KEY			IpKey;		// Key to associated IP_PARAM object
	USHORT			TcpPortNumber;

#ifndef ALAMOSA_WINDOWS_ENV
} CSX_PACKED PORTAL_KEY_ISCSI, *PPORTAL_KEY_ISCSI;
#else
} PORTAL_KEY_ISCSI, *PPORTAL_KEY_ISCSI;
#endif

///////////////////////////////////////////////////////////////////////////////
// A PORTAL_KEY uniquely identifies a PORTAL across all portals. 
//
// PortType : HOST_FIBRE_CHANNEL or HOST_ISCSI
// fc       : the Fibre Channel specific part of the key
// iscsi    : the iSCSI specific part of the key

typedef struct _PORTAL_KEY {

	HOST_PORT_TYPE	PortType;		

	union {
		PORTAL_KEY_ISCSI	iscsi;
	};

#ifndef ALAMOSA_WINDOWS_ENV
} CSX_PACKED PORTAL_KEY, *PPORTAL_KEY;
#else
} PORTAL_KEY, *PPORTAL_KEY;
#endif


#define PORTAL_KEY_EQ(a,b)											\
	(((a).PortType == (b).PortType) &&								\
		(((a).PortType == HOST_ISCSI) &&							\
			((a).iscsi.IpKey.u.IdHigh == (b).iscsi.IpKey.u.IdHigh) &&	\
			((a).iscsi.IpKey.u.IdLow == (b).iscsi.IpKey.u.IdLow) &&		\
			((a).iscsi.TcpPortNumber == (b).iscsi.TcpPortNumber)))

// NOTE: PORTAL_KEY_MATCH() does not compare the TcpPortNumber!!!
#define PORTAL_KEY_MATCH(a,b)										\
	(((a).PortType == (b).PortType) &&								\
		(((a).PortType == HOST_ISCSI) &&							\
			((a).iscsi.IpKey.u.IdHigh == (b).iscsi.IpKey.u.IdHigh) &&	\
			((a).iscsi.IpKey.u.IdLow == (b).iscsi.IpKey.u.IdLow)))

///////////////////////////////////////////////////////////////////////////////
// A PORTAL_PARAMS persistently presents the existance of a portal.
//
// Key: A unique value across all portals on an array.  The key also
// identifies the Fibbre Channel port or the iSCSI TCP port of the portal.

typedef struct _PORTAL_PARAMS {

	PORTAL_KEY		Key;	

} PORTAL_PARAMS, *PPORTAL_PARAMS;

///////////////////////////////////////////////////////////////////////////////
// A PORTAL_GROUP_PARAM persistently represents a portal group.  A portal
// group is primarily a collection of portals. Initiators log into a portal
// group through a particular portal.
//
// Revision:
//		Format of this structure.
//
// Key: A unique value across all portal groups on an array.  
//
// PortalGroupNameLength: 
//		The number of parameters in PortalGroupName.
//
// PortalGroupName: 
//		A bucket for the GUI to store and retrieve additional
//		information about the portal group.  Stored but not interpreted
//		by host side drivers.
//
// Behavior: 
//		Defines the behavior initiators see.  The portal
//		group behavior is only used if the session, I_T nexus,
//		initiator, and host heirarchy specify at the default
//		should be used.
//
// AuthenticationMethod:
//		Specifies the authentication method which will be used
//		by any Initiator that logs-in to this portal group.
//
// Reserved:
//		Must be zeroes.
//
// NumPortals:
//		The number of portals in this portal group.  A 
//		portal group with no portals is legal.
//
// Portals:
//		A variable length array of portal definitions.
//		The portal keys must be unique across all portal
//		groups (portals belong to at most one portal group).

#define MAX_PORTALS_PER_GROUP		4

typedef struct _PORTAL_GROUP_PARAM {

	ULONG				Revision;
	PORTAL_GROUP_KEY	Key;
	ULONG				Resv;
	ULONG				PortalGroupNameLength;
	UCHAR				PortalGroupName[K10_MAX_HPORT_NAME_LEN];
	INITIATOR_BEHAVIOR	Behavior;
	ULONG				AuthenticationMethod;
	ULONG				Reserved[8];
	ULONG				NumPortals;
	PORTAL_PARAMS		Portals[MAX_PORTALS_PER_GROUP];

} PORTAL_GROUP_PARAM, *PPORTAL_GROUP_PARAM;

#define CURRENT_PORTAL_GROUP_PARAM_REV	2

// Valid values for PORTAL_GROUP_PARAM.AuthenticationMethod.
// Note: Must have None=0 and Chap=1, just like LOGIN_AUTHENTICATION_TYPE.

#define PG_AUTHENTICATION_NONE	0x00000000
#define PG_AUTHENTICATION_CHAP	0x00000001

///////////////////////////////////////////////////////////////////////////////
// A PORTAL_GROUP_ENTRY is used to acquire the status a portal group.  
//
// Revision:
//		Revision of data structure.
//
// Reserved:
//		Must be zeroes.
//
// Params:
//      The  current persistent state fo the portal group.  Includes the key.

typedef struct _PORTAL_GROUP_ENTRY {

	ULONG					Revision;
	ULONG					Reserved[3];
	PORTAL_GROUP_PARAM		        Params;

} PORTAL_GROUP_ENTRY, *PPORTAL_GROUP_ENTRY;

#define CURRENT_PORTAL_GROUP_ENTRY_REV	2

// A PORTAL_GROUP_KEY_LIST is used to contain a list of PORTAL_GROUP Keys.

#pragma pack(4)

typedef struct _PORTAL_GROUP_KEY_LIST {

	ULONG				Revision;
	ULONG				Count;
	PORTAL_GROUP_KEY	Key[1];

} PORTAL_GROUP_KEY_LIST, *PPORTAL_GROUP_KEY_LIST;

// PORTAL_GROUP Key List size.
#define PORTAL_GROUP_KEY_LSIZE(KeyCount)							\
	(sizeof(PORTAL_GROUP_KEY_LIST) - sizeof(PORTAL_GROUP_KEY) +		\
	(KeyCount) * sizeof(PORTAL_GROUP_KEY))

#pragma pack()

#define CURRENT_PORTAL_GROUP_KEY_LIST_REV	1

// A PORTAL_GROUP_LIST structure is used to contain a list of PORTAL_GROUPs.

typedef struct _PORTAL_GROUP_LIST {

	ULONG				Count;
	ULONG				Revision;
	PORTAL_GROUP_ENTRY	Info[1];

} PORTAL_GROUP_LIST, *PPORTAL_GROUP_LIST;

// Portal Group List size macro.
#define PORTAL_GROUP_LSIZE(PGCount)		\
	(sizeof(PORTAL_GROUP_LIST) - sizeof(PORTAL_GROUP_ENTRY) + (PGCount) * sizeof(PORTAL_GROUP_ENTRY))

// A PGROUP_PARAM_LIST structure is used to persist a list of PORTAL_GROUPs.

typedef struct _PGROUP_PARAM_LIST {

	ULONG				Count;
	ULONG				Revision;
	PORTAL_GROUP_PARAM	Info[1];

} PGROUP_PARAM_LIST, *PPGROUP_PARAM_LIST;

// Portal Group Param List size macro.
#define PGROUP_PARAM_LSIZE(PGCount)		\
	(sizeof(PGROUP_PARAM_LIST) - sizeof(PORTAL_GROUP_PARAM) + (PGCount) * sizeof(PORTAL_GROUP_PARAM))


#if __cplusplus

// Define == operators for all of the types defined in this file.

extern "C++" inline bool operator ==(const PORTAL_KEY_ISCSI & f, const PORTAL_KEY_ISCSI & s) 
{
	return  (f.IpKey == s.IpKey && f.TcpPortNumber == s.TcpPortNumber);
}
extern "C++" inline bool operator ==(const PORTAL_KEY & f, const PORTAL_KEY & s) 
{
	if  (f.PortType != s.PortType) return false; 

	if (f.PortType == HOST_ISCSI) {
		return f.iscsi == s.iscsi;
	}
	return false;		/* Just to satisfy cpp compiler */
}

#endif

#endif
