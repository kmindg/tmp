// K10StoreCentricInternal.h
//
// Copyright (C) EMC Corporation 1999-2009
//
//
// Public Header for INTERNAL, struct-based K10StorCentric AdminLib usage
//
//	Revision History
//	----------------
//	14 Oct 99	D. Zeryck	Initial version.
//	15 Nov 99	D. Zeryck	Added K10StrCntrcGlobals for WWN retreival
//	31 Aug 00	B. Yang		Task 1774 - Cleanup the header files
//	21 Sep 00	B. Yang		Task 1938
//	10 Sep 01	B. Yang		Added support for initiator and varray in new interface
//	30 Jul 04	H. Weiner	Define ADMIN_INITIATOR_CONNECTION and associated structs & macros
//	25 May 06	E. Carter	Add K10_STRCNTRC_OPC_PORT_SET_PORT_WITHOUT_RESET value.
//	02 Jun 08	D. Hesi		Added K10_STRCNTRC_OPC_PORT_SET_PORT_WWN_ID.
//	30 Oct 09	D. Hesi		DIMS 239950: Added support for iSCSI Multipath IO
// 
#ifndef _K10StorageCentric_Internal_h_
#define _K10StorageCentric_Internal_h_

//----------------------------------------------------------------------INCLUDES

#ifndef K10_DEFS_H
#include "k10defs.h"
#include "csx_ext.h"
#endif
#include "K10CommonExport.h"

#ifndef _K10StorageCentric_Export_h_
#include "K10StoreCentricExport.h"
#endif

#include "scsitarg.h"
//-----------------------------------------------------------------------DEFINES

#define K10_NICE_ARRAY_NAME	K10_LOGICAL_ID

#define K10_ARRAY_MAX_NICE_NAME_LENGTH	256

#define K10_ARRAY_NICE_NAME_LENGTH		K10_LOGICAL_ID_LEN

// when adding a VLU to VA, yuo can ask us to pick the next available.
//
#define K10_SC_VLU_NEXT_FLAG		0xFFFF

// bit definitions for flags word in K10_StoreCentricWriteBuf
//
#define K10_STRCNTRC_INTERFACE_MODE        0x1
#define K10_STRCNTRC_INTERFACE_USE_OPTIONS 0x2
#define K10_STRCNTRC_INTERFACE_USE_TYPE    0x4

#define K10_STORECENTRIC_ADMIN_LIB_NAME		"K10HostAdmin"


#define K10_PHYSICAL_ARRAY_NAME_STRING 		"~physical"

#define MAX_LOGIN_SESSION_PER_INITIATOR		8

////////////////////////////////////////////////////////////////////////////////
//
// INTERNAL STRUCT BASED INTERFACE
//
// NOTE: For Navi, the interface used for this library is the AAS/AAQ-based
// TLD data interface, using the database IDs, tags, etc. specified in the 
// StorageCentric Command Setup Interface as specified by the Flare group.
//
// The database IDs and opcodes below are for the Read and Write methods of the
// IK10Admin interface, used directly by INTERNAL PROCESSES ONLY!
//
// In other words, if you work for Navi, stop reading here.
//
////////////////////////////////////////////////////////////////////////////////
//
// Database Specifiers. Use in lDbId parameter of Admin interface
//

enum K10_STRCNTRC_DB {
	K10_STRCNTRC_DB_MIN = 1,
	K10_STRCNTRC_DB_ARRAY = K10_STRCNTRC_DB_MIN,
	K10_STRCNTRC_DB_PORT,
	K10_STRCNTRC_DB_INITIATOR,
	K10_STRCNTRC_DB_VARRAY,
	K10_STRCNTRC_DB_MAX = K10_STRCNTRC_DB_VARRAY
};

////////////////////////////////////////////////////////////////////////////////
//
// Opcode Specifiers. Use in lOpcode parameter of Admin interface
//

////////////////////////////////////////////// for Database Type = ARRAY

enum K10_STRCNTRC_OPC_ARRAY_GET {			// Used in Read method
	K10_STRCNTRC_OPC_ARRAY_GET_MIN = 0,
	K10_STRCNTRC_OPC_ARRAY_GET_ALL = 0,		// get all data below (not used for internal methods)
	K10_STRCNTRC_OPC_ARRAY_GET_LU_LIST,		// Gets a list of currently-bound Flare LUs
											// returns K10_StoreCentricLU_List
	K10_STRCNTRC_OPC_ARRAY_GET_GLOBAL,		// Gets Storage Centric global data 
								// (Array name: NULL-term WCHAR string, & WWN
	K10_STRCNTRC_OPC_ARRAY_GET_PORT_LIST,	// Gets Port List: HOST_PORT_LIST
	K10_STRCNTRC_OPC_ARRAY_GET_MAX = K10_STRCNTRC_OPC_ARRAY_GET_PORT_LIST
};

enum K10_STRCNTRC_OPC_ARRAY_SET {			// Used in Read method
	K10_STRCNTRC_OPC_ARRAY_SET_MIN = 0,
	K10_STRCNTRC_OPC_ARRAY_SET_GLOBAL = K10_STRCNTRC_OPC_ARRAY_SET_MIN,	// sets HappyName: NULL-term WCHAR string
	K10_STRCNTRC_OPC_ARRAY_SET_MAX = K10_STRCNTRC_OPC_ARRAY_SET_GLOBAL
};

////////////////////////////////////////////// for Database Type = PORT
//
// These are NOT YET in the official SC design, as they don't know
// they need it yet!
//
#define PORT_GET_ISPEC(key)	 ( ((key.PortType) << 16) | ((key.SPId) << 8)	| key.LogicalPortId )

enum K10_STRCNTRC_OPC_PORT_GET {		// Used in Read method
	K10_STRCNTRC_OPC_PORT_GET_MIN = 1,
	K10_STRCNTRC_OPC_PORT_GET_LIST = K10_STRCNTRC_OPC_PORT_GET_MIN,	// Gets a list of the array's ports
										// uses HOST_PORT_LIST
										// same as K10_STRCNTRC_OPC_ARRAY_GET_PORT_LIST
	K10_STRCNTRC_OPC_PORT_GET_RQSD,			// Get a single port
	K10_STRCNTRC_OPC_PORT_GET_MAX = K10_STRCNTRC_OPC_PORT_GET_RQSD
};

enum K10_STRCNTRC_OPC_PORT_SET {			// Used in Read method
	K10_STRCNTRC_OPC_PORT_SET_MIN = 1,
	K10_STRCNTRC_OPC_PORT_SET_PORT = K10_STRCNTRC_OPC_PORT_SET_MIN,		// Sets a port's persistant state & resets port
											// uses HOST_PORT_PARAM
	K10_STRCNTRC_OPC_PORT_DELETE_PORT, 		// Deletes a port's persistant state
											// Uses HOST_PORT_KEY
	K10_STRCNTRC_OPC_PORT_SET_PORT_WITHOUT_RESET,	// Sets a port's persistant state, but doesn't reset port.
											// uses HOST_PORT_PARAM
	K10_STRCNTRC_OPC_PORT_SET_PORT_WWN_ID,	// Sets WWN ID to as a part of recovery to re-create the 
											//	port conversion table
	K10_STRCNTRC_OPC_PORT_SET_MAX = K10_STRCNTRC_OPC_PORT_SET_PORT_WWN_ID
};

////////////////////////////////////////////// for Database Type = INITIATOR

enum K10_STRCNTRC_OPC_INIT_GET {					// Used in Write method
	K10_STRCNTRC_OPC_INIT_GET_MIN = 1,
	K10_STRCNTRC_OPC_INIT_GET_INITIATOR_LIST = K10_STRCNTRC_OPC_INIT_GET_MIN,	// Gets the list of initiators
	K10_STRCNTRC_OPC_INIT_GET_MAX = K10_STRCNTRC_OPC_INIT_GET_INITIATOR_LIST
};													// uses I_T_NEXUS_LIST

enum K10_STRCNTRC_OPC_INIT_SET {			// Used in Write method
	K10_STRCNTRC_OPC_INIT_SET_MIN = 1,
	K10_STRCNTRC_OPC_INIT_SET_INITIATOR = K10_STRCNTRC_OPC_INIT_SET_MIN,// Registers an initiator with the array
											// uses I_T_NEXUS_PARAM
	K10_STRCNTRC_OPC_INIT_SET_REMOVE,		// deletes initiator-target nexus
											// USes I_T_NEXUS_KEY
	K10_STRCNTRC_OPC_INIT_SET_MAX = K10_STRCNTRC_OPC_INIT_SET_REMOVE
};


////////////////////////////////////////////// for Database Type = VARRAY
enum K10_STRCNTRC_OPC_VARRAY_SET {			// Used in Write method
	K10_STRCNTRC_OPC_VARRAY_SET_MIN = 1,

	K10_STRCNTRC_OPC_VARRAY_SET_VARRAY = K10_STRCNTRC_OPC_VARRAY_SET_MIN, // set target, create (& gen WWN) if not there
											// uses STORAGE_GROUP_PARAM
	K10_STRCNTRC_OPC_VARRAY_SET_REMOVE,		// Removes a Virtual Target
											// uses K10_WWID
	K10_STRCNTRC_OPC_VARRAY_SET_DEFAULT,	// set target, create if not there AND
											// make it the default
											// uses STORAGE_GROUP_PARAM
	K10_STRCNTRC_OPC_VARRAY_SET_VLU_NAME,	// sets happy name of VLU (for all instances of underlying)
											// uses K10_SetVluName
	K10_STRCNTRC_OPC_VARRAY_SET_MAX = K10_STRCNTRC_OPC_VARRAY_SET_VLU_NAME
};

enum K10_STRCNTRC_ISPEC_VARRAY_SET {		// Used w/K10_STRCNTRC_OPC_VARRAY_SET
	K10_STRCNTRC_ISPEC_VARRAY_SET_MIN = 0,
	K10_STRCNTRC_ISPEC_VARRAY_SET_NAVIACCESS = K10_STRCNTRC_ISPEC_VARRAY_SET_MIN,	// don't allow manipulation of Phys array
	K10_STRCNTRC_ISPEC_VARRAY_SET_ALLACCESS,		// allow manipulation of Phys array
	K10_STRCNTRC_ISPEC_VARRAY_SET_OPT_EDIT,			// Edit VA
	K10_STRCNTRC_ISPEC_VARRAY_SET_OPT_CREATE,		// Create VA
	K10_STRCNTRC_ISPEC_VARRAY_SET_MAX = K10_STRCNTRC_ISPEC_VARRAY_SET_OPT_CREATE
};


enum K10_STRCNTRC_OPC_VARRAY_GET {			// Used in Read method
	K10_STRCNTRC_OPC_VARRAY_GET_MIN = 1,
	K10_STRCNTRC_OPC_VARRAY_GET_LIST = K10_STRCNTRC_OPC_VARRAY_GET_MIN,	// Gets a list of all Virtual Targets defined 
											// see Ispec for datatype returned
	K10_STRCNTRC_OPC_VARRAY_GET_RQSD,		// Gets one, by WWN (pass in IspecData)
											// datatype is STORAGE_GROUP_ENTRY
	K10_STRCNTRC_OPC_VARRAY_GET_MAX = K10_STRCNTRC_OPC_VARRAY_GET_RQSD
};

enum K10_STRCNTRC_ISPEC_VARRAY_GET {			// Used in GET_LIST method
	K10_STRCNTRC_ISPEC_VARRAY_GET_MIN = 1,
	K10_STRCNTRC_ISPEC_VARRAY_LIST_SERIAL = K10_STRCNTRC_ISPEC_VARRAY_GET_MIN,	// STORAGE_GROUP_LIST, used for
												// DZGUI remoting only
	K10_STRCNTRC_ISPEC_VARRAY_LIST_POINTER,		// K10_STORECENTRIC_TARGET_PTR_LIST
	K10_STRCNTRC_ISPEC_VARRAY_LIST_WITH_ATTRIB,	// with attributes
	K10_STRCNTRC_ISPEC_VARRAY_GET_MAX = K10_STRCNTRC_ISPEC_VARRAY_LIST_WITH_ATTRIB
};

//----------------------------------------------------------------------TYPEDEFS

////////////////////////////////////////////////////////////////////////////////

#pragma pack(1)


////////////////////////////////////////////////////////////////////////////////
// Format of LUN Read Buffer Data Section
//

#define K10_STRCNTRC_LU_LIST_FIXED_SIZE		2

typedef struct _K10_StoreCentricLU {
	USHORT		flareLunId;
	K10_WWID	lunWWN;
	K10_LOGICAL_ID	lunName;

} K10_StoreCentricLU;

typedef struct _K10_StoreCentricLU_List {

	USHORT				numLus;				// number of LUN blocks in pList
	K10_StoreCentricLU	*pList;				// List of blocks.
} K10_StoreCentricLU_List;


////////////////////////////////////////////////////////////////////////////////
// Format of SetVLUName OPC
//

typedef struct _K10_Set_Vlu_Name{

	long			flu;	// ignored.
	K10_LU_ID		wwn;	// ID
	K10_LOGICAL_ID	name;

} K10_SetVluName;

//////////////////////////////////////////////////////////////////
// used in K10_STRCNTRC_OPC_VARRAY_GET_LIST
// Not a serialized struct due to varsize: all pointers GlobalAlloc'd
//


#define K10_STRCNTRC_VARRAY_LIST_FIXED_SIZE		4

///////////////////////////////////////////////////////////////////
// used in K10_STRCNTRC_OPC_ARRAY_GET_GLOBAL

typedef struct _K10StrCntrcGlobals {
	K10_LOGICAL_ID	arrayName;
	K10_ARRAY_ID	arrayWWN;
	csx_uchar_t	reserved[2];
	csx_uchar_t	btRANameInited;
	csx_uchar_t	btWWNameInited;
} K10StrCntrcGlobals;
////////////////////////////////////////////////////////////////////
// used in 
//
//
typedef struct _K10StrCntrcVaListWithAttribs {

	ULONG			Count;		// number of target blocks in pList
	STORAGE_GROUP_PARAM	**ppList;	// List of STORAGE_GROUP_PARAM blocks.
	csx_uchar_t		*pAttribList;

} K10StrCntrcVaListWithAttribs;

typedef struct _ADMIN_LOGIN_SESSION_DETAIL 
{
	ULONGLONG			MgmtSourceID;
	IN6_ADDR			IpAddr;
} ADMIN_LOGIN_SESSION_DETAIL ;

typedef struct _ADMIN_INITIATOR_CONNECTION 
{
	BOOLEAN						MgmtTemporary;
	UCHAR						MgmtOnline;
	UCHAR						reserved[3];
	I_T_NEXUS_ENTRY				Nexus;
	ULONG						MgmtSessionCount;
	ADMIN_LOGIN_SESSION_DETAIL	LoginData[MAX_LOGIN_SESSION_PER_INITIATOR];

} ADMIN_INITIATOR_CONNECTION;

typedef struct _ADMIN_INITIATOR_CONNECTION_LIST 
{
	ULONG						Count;
	ADMIN_INITIATOR_CONNECTION	Info[1];

} ADMIN_INITIATOR_CONNECTION_LIST;

#define ADMIN_INITIATOR_CONNECTION_LSIZE(ITNexusCount)		\
	(sizeof(ADMIN_INITIATOR_CONNECTION_LIST) - sizeof(ADMIN_INITIATOR_CONNECTION) +	(ITNexusCount) * sizeof(ADMIN_INITIATOR_CONNECTION))

#pragma pack() // back to cmd-line specified pack


#endif // header desc
