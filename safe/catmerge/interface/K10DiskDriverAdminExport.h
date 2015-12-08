// K10DiskDriverAdminExport.h
//
// Copyright (C) EMC Corporation 2000-2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//
// Header for all Disk Driver IK10Admin libraries: defines,
//		database codes, opcodes, etc. Also extensive sample
//		code
//
//	Revision History
//	----------------
//	10 Sep 98	D. Zeryck	Initial version.
//	28 Sep 98	D. Zeryck	Put flags bytes in all structs, little endian
//							flag, fixed Read fn args.
//	30 Nov 98	D. Zeryck	Redefined K10_ERROR_BASE_REMOTE_MIRROR
//	11 Jan 99	D. Zeryck	Object names are now 64 ASCII chars, not 32 UNICODE
//	05 Feb 99	D. Zeryck	K10_LAYER_DRV_ISPEC_DATA used for all reads, minor additions
//	19 Feb 99	D. Zeryck	Add Quiesce and Shutdown support, K10_LAYER_DRV_ISPEC_WWID
//	29 Mar 99	D. Zeryck	Mod comment for K10_DISK_ADMIN_ERROR_IOCTL_RETURN_UNEXPECTED_BYTES
//	23 Jun 99	D. Zeryck	Use NO_EXTEND not PRIVATE for denying layering of LU.
//					No more endianness or layering
//	29 Jul 99	D. Zeryck	added K10_DISK_ADMIN_ERROR_STACKOPS_MALFORMED
//	17 Mar 00	D. Zeryck	add K10_DISK_ADMIN_ERROR_INVALID_BIND_NAME,
					//K10_DISK_ADMIN_ERROR_FAILED_TO_USE_BIND_NAME
//	05 Sep 00	B. Yang		Task 1774 - Cleanup the header files
//	12 Sep 00	B. Yang		CHG01856 - Add K10_DISK_ADMIN_WARNING_LU_ACTIVE
//	21 Sep 00	B. Yang		Task 1938
//	 2 Nov 00	D. Zeryck	10203
//	 7 Oct 03	C. Hopkins	Added Create and Delete disk admin opcodes
//   8 Dec 03   C. Hughes   Layered Driver LU Expansion
//	13 May 04	C. Hopkins	Added K10_DISK_ADMIN_OPC_GET_MASTER_WWN
//	10 Jun 05	R. Hicks	Added K10_DISK_ADMIN_OPC_GET_STATS
//	27 Dec 05	M. Brundage	DIM 137265: Added K10_DISK_ADMIN_OPC_GET_WORKLIST_OLDREQ support
//  25 Apr 07	M. Khokhar	DIM 167794: Added new error code K10_DISK_ADMIN_ERROR_DEVICE_IN_STORAGE_GROUP
//	18 Mar 08	V. Chotaliya	DIM 192501 : Added K10_DISK_ADMIN_OPC_CLEAR_CACHE_DIRTY
//	09 Feb 09	R. Hicks	DIMS 219256: Added K10_DISK_ADMIN_INFO_LU_DESTROYING
//	31 Mar 09	R. Hicks	DIMS 223632: Allow device map instance to be passed in K10_WorklistRequest
//	06 Jul 09	B. Myers	Added K10_DISK_ADMIN_OPC_REFRESH_CONFIGURATION
//  26 Feb 10   C. Hopkins  Added specific values for drivers' btAutoInsert flag (flag is no longer just a boolean)
//	22 Jul 10	R. Hicks	Define new K10_ADMIN_AUTO_INSERT value for btAutoInsert flag
//

#ifndef K10_DISK_DRIVER_ADMIN_EXPORT_H
#define K10_DISK_DRIVER_ADMIN_EXPORT_H

////////////////////////////////////////////////////////////////////////////////
// The following information defines general interfaces all layered drivers 
// must support. Looks like a lot but you can borrow almost an entire
// sample library: K10LayerDvr01Lib, found in esd\mgmt\test
//
// IMPORTANT NOTE: We are using the IK10Admin interface for simplicity's sake.
// This uses database IDs, opcodes (DB ID specific), and item specifiers to
// make a standard function interface fairly flexible. Since we are assuming only
// inprocess servers (DLLs) will be used, we know we will be in the same thread 
// context, and thus can pass pointers through the COM layer. 

// The IK10Admin interface uses this to pass a buffer (the pIn or ppIn args)
// to your library. The buffers are defined as structures in K10DiskDriverAdmin.h.

// The ItemSpec parameter holds a pointer in some read operations (standard 
// feature of the Redirector). Thus, there are some IN BUFFER definitions for
// read operations below. These IN BUFFERs all use a standard structure 
// (K10_LAYER_DRV_ISPEC_DATA) for their data so we can determine endianness & check 
// the contents for sanity. 

// ON ENDIANNESS: These interfaces will be called from Redirector
// code that is not a direct passthough from Navisphere. Thus, endianness will
// be native. No action is needed.


// INCLUDES

#ifndef K10_DEFS_H
#include "k10defs.h"		// K10_MAX_LIBDEF_DBID, etc.
#include "csx_ext.h"
#endif
#include "K10CommonExport.h"

#ifndef K10_DISK_DRIVER_ADMIN_H
#include "k10diskdriveradmin.h"
#endif

// MACROS

// Error bases assigned by the Admin Czar. Use these in your admin lib
// header to base the errors you return (see sample code below)

#define K10_ERROR_BASE_REMOTE_MIRROR	0x71050000
#define K10_ERROR_BASE_DISK_ADMIN		0x79000000
#define K10_SYSTEM_ADMIN_ERROR_BASE		0x79500000


// General defines used to check the AutoInsert registry value
#define K10_NOT_AUTO_INSERT     0       // not generally used but here for completeness
#define K10_STD_AUTO_INSERT     1       // Fully AI driver not participating in device map
                                        //  and does not get Clear Cache Dirty command 
#define K10_HYBRID_AUTO_INSERT  2       // Mostly AI driver that has some map participation
                                        //  and needs to receive Clear Cache Dirty command
#define K10_ADMIN_AUTO_INSERT  3       // Driver which is to be automatically inserted by
                                       // the admin layer at lun creation


// GENERAL ADMIN DATABASES & OPCODES


// Data sizes for fixed-size buffers. Apply to current version only.
// Can be used as a validation step or to populate the Data Size
// entry (data size DOES include all bytes defined in buffer, unlike
// Flare Vendor-unique MPs which exclude the first few bytes...)

#define K10_DISK_ADMIN_ENUM_OBJECTS_SIZE		12	// MINIMUM
#define K10_DISK_ADMIN_GENERIC_PROPS_SIZE		220	// EXACT
#define K10_DISK_ADMIN_SPECIFIC_PROPS_SIZE		73	// MINIMUM only
#define K10_DISK_ADMIN_BIND_SIZE				140 // MINIMUM only
#define K10_DISK_ADMIN_UNBIND_SIZE				72	// EXACT
#define K10_DISK_ADMIN_COMPAT_MODE_SIZE			12	// EXACT
#define K10_DISK_ADMIN_QUIESCE_MODE_SIZE		72	// EXACT
#define K10_DISK_ADMIN_REASSIGN_MODE_SIZE		152	// EXACT

// ENUMS

// Database Specifiers. Use in lDbId parameter of Admin interface
// The generic lib DBIDs are above the max allowed for drivers
//
typedef enum K10_disk_admin_db {
	K10_DISK_ADMIN_DB_MIN = K10_MAX_LIBDEF_DBID + 1,
	K10_DISK_ADMIN_DB_ATTACH = K10_DISK_ADMIN_DB_MIN,
	K10_DISK_ADMIN_DB_MAX = K10_DISK_ADMIN_DB_ATTACH
}K10_DISK_ADMIN_DB;

// Error codes for returning when you have encountered a problem in one of the
// general admin operations. Also used by support libs to report errors.

typedef enum K10_disk_admin_error {						// specific error codes
	K10_DISK_ADMIN_ERROR_SUCCESS = 0x00000000L,	

	K10_DISK_ADMIN_ERROR_ERROR_MIN = K10_ERROR_BASE_DISK_ADMIN  | K10_COND_ERROR,
	K10_DISK_ADMIN_ERROR_DBID =		
		K10_DISK_ADMIN_ERROR_ERROR_MIN,	// Unrecog. database ID in header
	K10_DISK_ADMIN_ERROR_OPCODE,		// Unrecog. opcode in header
	K10_DISK_ADMIN_ERROR_ITEMSPEC,		// Invalid item specifier
	K10_DISK_ADMIN_ERROR_IOCTL_TIMEOUT, // timeout when waiting for DeviceIoControl.
	K10_DISK_ADMIN_ERROR_VERSION,		// Wrong version in structure
	K10_DISK_ADMIN_ERROR_BAD_HEADER,	// Size in header of our data does not agree 
										// with the size passed in
	K10_DISK_ADMIN_ERROR_IOCTL_RETURN_UNEXPECTED_BYTES,	// got bytes returned on a Write ioctl,
											// or read ioctl bytes returned not expected size
	K10_DISK_ADMIN_ERROR_OBJECT_NOT_FOUND,	// could not locate desired obj
	K10_DISK_ADMIN_ERROR_NOT_SUPPORTED,		// operation is not supported
	K10_DISK_ADMIN_ERROR_DEVICE_CONSUMED,	// attempt to consume/delete already consumed obj.
	K10_DISK_ADMIN_ERROR_DEVICE_NOT_CONSUMED,
											// attempt to unconsume non-consumed obj.
	K10_DISK_ADMIN_ERROR_WRONG_DEVICE_CONSUMED,	
											// attempt to consumed obj. that has a layer on it
											// and devicename is of lower (pre-layered) device
	K10_DISK_ADMIN_ERROR_WRONG_DEVICE_UNCONSUMED,
											// as above error, in unconsume. Devmap must be broken
	K10_DISK_ADMIN_ERROR_NOT_CONSUMER,		// attempt to unconsume obj when not owner
	K10_DISK_ADMIN_ERROR_DEVICE_NO_EXTEND,	// attempt to layer an obj consumer/creator marked NoExtend
	K10_DISK_ADMIN_ERROR_DEVICE_LAYERED,	// attempt to remove/private consume obj with outstanding layeres
	K10_DISK_ADMIN_ERROR_DEVICE_NOT_LAYERED,// attempt to remove created device from filter stack
	K10_DISK_ADMIN_ERROR_WRONG_DEVICE_LAYERED,// driver reports layering a device which is not that exported by driver below
	K10_DISK_ADMIN_ERROR_DISK_ERROR,		// environmental error writing to local disk (temp files, etc)
	K10_DISK_ADMIN_ERROR_FILE_ERROR,		// a required file is not coherent/proper length, etc.
	K10_DISK_ADMIN_ERROR_DATA_ERROR,		// mutually exclusive values set in data.
	K10_DISK_ADMIN_ERROR_BAD_DATA,			// data does not pass consistency check
	K10_DISK_ADMIN_ERROR_NO_TRANSLOG,		// Nonexistant transaction log.
	K10_DISK_ADMIN_ERROR_STACKOPS_MALFORMED,// "StackOps" string malfomed.
	K10_DISK_ADMIN_ERROR_STACK,				// Trying to add driver in wrong place on stack
	K10_DISK_ADMIN_ERROR_TRANSLOG_ERROR,	// Node we're decrementing not last one in log
	K10_DISK_ADMIN_ERROR_TOO_MANY_OBJECTS,	// See more objects than we can handle
	K10_DISK_ADMIN_ERROR_UNKNOWN_EXCEPTION,	// caught int5/3
	K10_DISK_ADMIN_ERROR_INVALID_BIND_NAME,	// Driver returned a bad name for a bind object
	K10_DISK_ADMIN_ERROR_EMPTY_BIND_NAME,	// Driver returned a empty name for a bind object
	K10_DISK_ADMIN_ERROR_FAILED_TO_USE_BIND_NAME, // driver did not use supplied bind name
	K10_DISK_ADMIN_ERROR_NO_WORKLIST,		// Worklist PSM file not found
	K10_DISK_ADMIN_ERROR_INCOMPLETE,		// Driver configuration appears incomplete
    K10_DISK_ADMIN_ERROR_EMPTY_TRANSLOG,    // Empty transaction log
	K10_DISK_ADMIN_ERROR_WORKLIST_REQUIRED,	// Target wants to process request via worklist
	K10_DISK_ADMIN_ERROR_DEVICE_IN_STORAGE_GROUP, // This LUN is already allocated to a storage group. Check that this is the correct LUN, and if so, remove the LUN from the storage group and try again.
	K10_DISK_ADMIN_ERROR_ERROR_MAX = K10_DISK_ADMIN_ERROR_DEVICE_IN_STORAGE_GROUP,

	K10_DISK_ADMIN_WARNING_MIN = K10_ERROR_BASE_DISK_ADMIN  | K10_COND_WARNING,
	K10_DISK_ADMIN_WARNING_LU_ACTIVE = K10_DISK_ADMIN_WARNING_MIN,
	K10_DISK_ADMIN_WARNING_MAX = K10_DISK_ADMIN_WARNING_LU_ACTIVE,

	K10_DISK_ADMIN_INFO_MIN = K10_ERROR_BASE_DISK_ADMIN  | K10_COND_INFO,
	K10_DISK_ADMIN_INFO_LU_DESTROYING,		//Driver is asychronously destroying the LUN
	K10_DISK_ADMIN_INFO_MAX = K10_DISK_ADMIN_INFO_LU_DESTROYING

}K10_DISK_ADMIN_ERROR;

// Opcode Specifiers. Use in lOpcode parameter of Admin interface
//

// for Database Type K10_DISK_ADMIN_DB_ATTACH

typedef enum K10_disk_admin_opc {
	K10_DISK_ADMIN_OPC_MIN = 0,
	K10_DISK_ADMIN_OPC_ENUM_OBJECTS = K10_DISK_ADMIN_OPC_MIN,	// used in ReadBuffer
	K10_DISK_ADMIN_OPC_GET_GENERIC_PROPS,		// used with read buffer
	K10_DISK_ADMIN_OPC_GET_SPECIFIC_PROPS,		// used with read buffer
	K10_DISK_ADMIN_OPC_PUT_GENERIC_PROPS,		// used with write buffer
	K10_DISK_ADMIN_OPC_PUT_SPECIFIC_PROPS,		// used with write buffer
	K10_DISK_ADMIN_OPC_BIND,					// used with update buffer
	K10_DISK_ADMIN_OPC_UNBIND,					// used with write buffer
	K10_DISK_ADMIN_OPC_GET_COMPAT_MODE,			// used with read buffer
	K10_DISK_ADMIN_OPC_PUT_COMPAT_MODE,			// used with write buffer
	K10_DISK_ADMIN_OPC_QUIESCE,					// used with write buffer
	K10_DISK_ADMIN_OPC_SHUTDOWN,				// used with write buffer
	K10_DISK_ADMIN_OPC_REASSIGN,				// WriteBuffer, for consumed objs only
	K10_DISK_ADMIN_OPC_GET_GENERIC_PROPS_LIST,	// used with readbuf, no ispec, hostadmin only, for perf.
	K10_DISK_ADMIN_OPC_STATS,
	K10_DISK_ADMIN_OPC_CONSUME,					// Tell driver it may privately consume LU
	K10_DISK_ADMIN_OPC_UNCONSUME,				// Tell driver to stop consuming private LU
	K10_DISK_ADMIN_OPC_RENAME,					// Change WWN of LU created by this driver
	K10_DISK_ADMIN_OPC_GET_WORKLIST,			// Get a worklist from a conforming layered driver
	K10_DISK_ADMIN_OPC_COMPLETE,				// Tell LD to complete its tasks
	K10_DISK_ADMIN_OPC_SPOOF,					// Tell driver to "spoof" the LU
	K10_DISK_ADMIN_OPC_CREATE,					// Tell driver it's time to create its device
	K10_DISK_ADMIN_OPC_DELETE,					// Tell driver to delete specified device
	K10_DISK_ADMIN_OPC_ENUM_WWNS,				// Auto Insert driver enum LU wwns it's layering
    	K10_DISK_ADMIN_OPC_POWEROFF,                // Tell flare to turn off power
	K10_DISK_ADMIN_OPC_GET_IDENTIFIER_PROPS,	// Get further ID info from creating driver
	K10_DISK_ADMIN_OPC_LIST_SUPPORTED_OPS,		// Get list of operations
                                                // supported by driver
	K10_DISK_ADMIN_OPC_LIST_ACTIVE_OPS,			// Get list of operations
                                                // currently in progress
	K10_DISK_ADMIN_OPC_GET_MASTER_WWN,			// Get related object WWN to quiesce
	K10_DISK_ADMIN_OPC_GET_STATS,				// Get state of statistics logging from driver
	K10_DISK_ADMIN_OPC_GET_WORKLIST_OLDREQ,			// Get worklist for "old style" request
	K10_DISK_ADMIN_OPC_CLEAR_CACHE_DIRTY,		// Inform about ClearCacheDirtyLun operation, K10_LU_ID is data packet
	K10_DISK_ADMIN_OPC_REFRESH_CONFIGURATION,	// Refresh configuration information that has changed since the last boot or refresh.

	K10_DISK_ADMIN_OPC_DESERIALIZE_OBJ,
	K10_DISK_ADMIN_OPC_MAX = K10_DISK_ADMIN_OPC_DESERIALIZE_OBJ
}K10_DISK_ADMIN_OPC;


// Enum for List Supported Ops reason code
typedef enum K10_Disk_Admin_List_Supported_Reason
{
    K10_DISK_ADMIN_LIST_SUPPORTED_REASON_NONE = 0,
    K10_DISK_ADMIN_LIST_SUPPORTED_REASON_MIN = 1,
    K10_DISK_ADMIN_LIST_SUPPORTED_REASON_SIZE_CHANGE = 
        K10_DISK_ADMIN_LIST_SUPPORTED_REASON_MIN,
    K10_DISK_ADMIN_LIST_SUPPORTED_REASON_INSERT,
    K10_DISK_ADMIN_LIST_SUPPORTED_REASON_CONSUME, 
    K10_DISK_ADMIN_LIST_SUPPORTED_REASON_EXPAND, 
    K10_DISK_ADMIN_LIST_SUPPORTED_REASON_MAX =
        K10_DISK_ADMIN_LIST_SUPPORTED_REASON_EXPAND
} K10_DISK_ADMIN_LIST_SUPPORTED_REASON;

#define K10_DISK_ADMIN_LIST_SUPPORTED_UNKNOWN_SIZE  ((ULONGLONG) -1)


//
// This enumeration defines the LU states returned in the TAG_STATE bubble
// for LU expansion.  (Other states could be added for other features in the
// future.)  (NOTE: this needs to be coordinated with the VOL_STATE_USER
// enum in AggAdminLogical.h.)
//
typedef enum K10_Disk_Admin_Lu_State {
	K10_DISK_ADMIN_LU_STATE_NONE = 0,
	// K10_DISK_ADMIN_LU_STATE_DEGRADED = 1,  reserved for Aggregate
	// K10_DISK_ADMIN_LU_STATE_SHUTDOWN = 2,  reserved for Aggregate
    // 3 is currently unused
	K10_DISK_ADMIN_LU_STATE_EXPANDING = 4,
	K10_DISK_ADMIN_LU_STATE_BLOCKED = 5

} K10_DISK_ADMIN_LU_STATE, *PK10_DISK_ADMIN_LU_STATE;

	
// TYPEDEFS

#pragma pack(1)								// pack byte-tight

// This is essentially an overload of the MM_InbufHdr (MMan.h)
// Used in all read methods to, where necessary, 
// send data. Data type is indicated in bType member.
// Note: use macro below to set dwDataSize member
//
typedef struct K10LayerDrvIspecData {
	unsigned long			dwDataSize;		// size of this struct		
	unsigned char			bReserved;
	unsigned char			bType;			// see defines below
	unsigned char			pData[1];		// payload
} K10_LAYER_DRV_ISPEC_DATA;

// Set size of K10_LAYER_DRV_ISPEC_DATA. PayloadSize is number
// of bytes in pData.
//
#define K10_LAYER_DRV_ISPEC_SIZE(PayloadSize)     \
	sizeof(K10_LAYER_DRV_ISPEC_DATA) - 1 + PayloadSize

#define K10_LAYER_DRV_ISPEC_OBSOLETE	0x00	// PayloadSize = 0)
#define K10_LAYER_DRV_ISPEC_OBJNAME		0x01	// PayloadSize = sizeof(K10_DVR_OBJECT_NAME))
#define K10_LAYER_DRV_ISPEC_WWID		0x02	// PayloadSize = sizeof(K10_WWID))


// for K10_DISK_ADMIN_OPC_GET_GENERIC_PROPS_LIST
// used ONLY when gen props each have 1 element. For performance with
// hostadmin only.

typedef struct K10_DISK_ADMIN_GENERIC_PROPS_LIST {
    unsigned int                count;

	K10_GEN_OBJECT_PROPS		pList[1];
} *PK10DiskAdminGenericPropsList, K10DiskAdminGenericPropsList;

#define K10_DISK_ADMIN_GENERIC_PROPS_LIST_SIZE(count)\
	(sizeof(K10_DISK_ADMIN_GENERIC_PROPS_LIST) - sizeof(K10_GEN_OBJECT_PROPS) +	\
	(count) * sizeof(K10_GEN_OBJECT_PROPS)) 


//
// Structure of lItemSpec argument for K10_DISK_ADMIN_OPC_LIST_SUPPORTED_OPS
//
typedef struct K10DiskAdmin_ListSupportedOps_Data
{
    K10_LU_ID		                        luId;
    csx_u64_t                               new_size;
                                    // New size after expansion/contraction
    K10_DISK_ADMIN_LIST_SUPPORTED_REASON	checkReason;
} K10DiskAdmin_ListSupportedOpsData;

//
// Structures/enums for K10_DISK_ADMIN_GET_WORKLIST_OLDREQ.
// K10_WorklistRequest pointer is passed as lItemSpec parameter.
//

typedef enum K10_WL_REQUEST_TYPE{
	REQUEST_READ = 0,
	REQUEST_WRITE,
	REQUEST_UPDATE,
	REQUEST_PARAMS_MAX = 16,
	REQUEST_READTLDLIST,
	REQUEST_WRITETLDLIST
} K10_WL_REQUEST_TYPE;

typedef struct K10_WL_Request_Params {
	long        lDbField;
	long        lItemSpec;
	long        lOpCode;
	csx_uchar_t *pIn;
	long        lBufSize;
} K10_WL_Request_Params;

typedef struct K10_WL_Response_Params {
	csx_uchar_t *pOut;
	long        lBufSize;
} K10_WL_Response_Params;

typedef union K10_WL_RequestU {
	void *tldReq;					// Must cast to TLD *
	K10_WL_Request_Params paramReq;
} K10_WL_RequestU;

typedef union K10_WL_ResponseU {
	void *tldResp;					// Must cast to TLD *
	K10_WL_Response_Params paramResp;
} K10_WL_ResponseU;

typedef struct K10_WorklistRequest {
	K10_WL_REQUEST_TYPE requestType;
	K10_WL_RequestU req;
	K10_WL_ResponseU resp;
	void *pDeviceMap;				// Must cast to K10DeviceMapPrivate *
	void *pLunList;					// Must cast to K10LunExtendedList *
	unsigned char bDelayMapWrite;	// if non-zero, don't write device map
									// during worklist processing
} K10_WorklistRequest;

// struct to get serialized object pointer to an admin lib
// and get the pointer to the resulting object back on a 
// K10_DISK_ADMIN_OPC_DESERIALIZE_OBJ operation
typedef struct AdminObjectDeserializeInfo {
    void *pListSerData;
    void *pObjList;
} AdminObjectDeserializeInfo;


#pragma pack() // back to cmd-line specified pack

// BUFFER CREATION HINTS
//
// Use pointers for the structs defined in K10DiskDriverAdmin.h as buffers. 
// Use the macros for sizing to determine the buffer size you will need & allocate. 

// BUFFER TYPES USED FOR OPCODES

/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_ENUM_OBJECTS
/*
Item Spec: none, pass 0

Data Buffer: K10_ENUM_OBJECTS

/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_GET_GENERIC_PROPS 
//	

ItemSpec: K10_LAYER_DRV_ISPEC_DATA *
	bType = K10_LAYER_DRV_ISPEC_OBJNAME
	pData =	K10_DVR_OBJECT_NAME

Data Buffer: K10_GEN_OBJECT_PROPS


/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_PUT_GENERIC_PROPS

Item Spec: none, pass 0

Data Buffer: K10_GEN_OBJECT_PROPS

/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_GET_SPECIFIC_PROPS

ItemSpec: K10_LAYER_DRV_ISPEC_DATA *
	bType = K10_LAYER_DRV_ISPEC_OBJNAME
	pData =	K10_DVR_OBJECT_NAME

Data Pointer: K10_SPEC_OBJECT_PROPS*

/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_PUT_SPECIFIC_PROPS

Item Spec: none, pass 0

Data Pointer: K10_SPEC_OBJECT_PROPS*

/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_BIND

Item Spec: none, pass 0

Data Pointer: K10_BIND_OBJECT*

/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_UNBIND

Item Spec: none, pass 0

Data Pointer: K10_UNBIND_OBJECT*

/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_GET_COMPAT_MODE 

Item Spec: none, pass 0
Data Pointer: K10_COMPATIBILITY_MODE_WRAP*

/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_PUT_COMPAT_MODE

Item Spec: none, pass 0
Data Pointer: K10_COMPATIBILITY_MODE_WRAP*

/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_QUIESCE

Item Spec: none, pass 0

Data Pointer: K10_QUIESCE_OBJECT*

/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_REASSIGN

Item Spec: none, pass 0

Data Pointer: K10_DRIVER_REASSIGN

/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_CLEAR_CACHE_DIRTY
   Data Pointer : K10_LU_ID


/////////////////////////////////////
// For K10_DISK_ADMIN_OPC_REFRESH_CONFIGURATION

Item Spec: none, pass 0
Data Pointer: K10_REFRESH_CONFIGURATION_OBJECT*


*/   // End list of opcodes & buffer types



#endif
