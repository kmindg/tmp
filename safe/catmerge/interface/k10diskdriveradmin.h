// K10DiskDriverAdmin.h
//
// Copyright (C) EMC Corporation 2000-2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//
// Header for all Disk Driver Administrative Interfaces
//
//	Revision History
//	----------------
//	09 Sep 98	D. Zeryck	Initial version.
//	14 Sep 98	D. Zeryck	Post Review Version: clean up macros, ifdefs, typedefs
//	28 Sep 98	D. Zeryck	Put flags Bytes in all structs, little endian flag.
//	17 Dec 98	D. Zeryck	Single version for all structs, typedef enums
//	05 Feb 99	D. Zeryck	Notes added on input buffers to GET ops
//	19 Feb 99	D. Zeryck	Add Quiesce & Shutdown support
//	 3 Mar 99	D. Zeryck	Added Reassign
//	23 Mar 99	D. Zeryck	Added do/undo flag to K10_QUIESCE_OBJECT
//	23 Jun 99	D. Zeryck	Changed gen obj props.btPrivateLU to btNoExtend to avoid clash with 
//							concept of private (non-TCD) LU. No more flags/endianness
//	04 Aug 99	D. Zeryck	Flushed out SHUTDOWN, added QuiesceAll flag to QUIESCE.
//							No structures changed size.
//	14 Apr 00	D. Zeryck	Add btFrontEndFlags to gen props, did not change size
//	 7 Oct 03	C. Hopkins	Added K10_CRE_DEL_OBJECT
// 20 Jan 04   M. Salha    Added K10_RENAME_OBJECT
//	13 May 04	C. Hopkins	Added K10_MASTER_WWN_OBJECT
//	16 Sep 04	C. Hopkins	Added luId to K10_QUIESCE_OBJECT
//	27 Oct 05	R. Hicks	Added bIsCreator to gen props, did not change size
//	27 Dec 05	M. Brundage DIM 137265 - Added location fields to K10_GEN_OBJECT_PROPS and K10_BIND_OBJECT
//	19 Jan 06	R. Hicks	DIMS 138321 - added mode to K10_QUIESCE_OBJECT
//	02 Jul 08	R. Hicks	Added lunNumber to K10_CRE_DEL_OBJECT
//	06 Jul 09	B. Myers	Added IOCTL_K10ATTACH_REFRESH_CONFIGURATION
//	27 Oct 09	R. Hicks	DIMS 239841 - Added bDeleteLun to K10_UNBIND_OBJECT


#ifndef K10_DISK_DRIVER_ADMIN_H
#define K10_DISK_DRIVER_ADMIN_H

// INCLUDES

#if !defined (K10DEFS_H)
#include "k10defs.h"
#endif


#ifndef _K10_IOCTL_H_
#include "K10_IOCTL.h"		// for control code macro, etc.
#endif


// MACROS
//

// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.  (Quoted from "/ddk/inc/devioctl.h".)

#define FILE_DEVICE_K10ADMIN			0x80DA		// unique but arbitrary
#define K10ATTACH_CTL_CODE(Op)			(ULONG)\
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_K10ADMIN, (Op), EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// for btFrontEndFlags
#define DISKADMIN_DEFOWN_FLAG			0x01 // 0 = SPA, 1 = SPB
#define DISKADMIN_AT_FLAG				0x02 // 1 = enabled
#define DISKADMIN_AA_FLAG				0x04 // 1 = enabled

// All external data interfaces are in native format; no byteswapping needed


// *Example* Ioctls - use them if it suits.

// STANDARD SEMANTICS
// If you use these IOCTLs and these semantics, you can steal most of the
// code you need from K10LayeredDrv01 (under mgmt\test)
// For any of the 'get' IOCTLS, the caller allocates at least 4-Byte buffer.
// This allows at least the dataset size, which is the first value in the list 
// definitions, to be returned. Thus, the caller can use a unsigned int  pointer
// to call, get the size, and use that value to size the output buffer. The
// driver is responsible for returning as much information as possible, setting
// return value to FALSE and NextError to ERROR_INSUFFICIENT_BUFFER if the buffer
// is too small.
//

// These IOCTLS are used to set and determine the objects connected to and
// 'layered' by the driver. Codes are arbitrary. See SEMANTICS section below.

#define IOCTL_K10ATTACH_ENUM_OBJECTS 					K10ATTACH_CTL_CODE(0xA00)
#define IOCTL_K10ATTACH_GET_GENERIC_PROPS				K10ATTACH_CTL_CODE(0xA01)
#define IOCTL_K10ATTACH_GET_SPECIFIC_PROPS				K10ATTACH_CTL_CODE(0xA02)
#define IOCTL_K10ATTACH_PUT_GENERIC_PROPS				K10ATTACH_CTL_CODE(0xA03)
#define IOCTL_K10ATTACH_PUT_SPECIFIC_PROPS				K10ATTACH_CTL_CODE(0xA04)
#define IOCTL_K10ATTACH_BIND							K10ATTACH_CTL_CODE(0xA05)
#define IOCTL_K10ATTACH_UNBIND							K10ATTACH_CTL_CODE(0xA06)
#define IOCTL_K10ATTACH_GET_COMPAT_MODE					K10ATTACH_CTL_CODE(0xA07)
#define IOCTL_K10ATTACH_PUT_COMPAT_MODE					K10ATTACH_CTL_CODE(0xA08)
#define IOCTL_K10ATTACH_QUIESCE							K10ATTACH_CTL_CODE(0xA09)
#define IOCTL_K10ATTACH_SHUTDOWN						K10ATTACH_CTL_CODE(0xA0A)
#define IOCTL_K10ATTACH_REASSIGN						K10ATTACH_CTL_CODE(0xA0B)
#define IOCTL_K10ATTACH_REFRESH_CONFIGURATION			K10ATTACH_CTL_CODE(0xA0C)

// This definition is used in the wVersion item in the structs
//
#define K10ATTACH_CURRENT_VERSION			0x01

// ENUMS

// A driver is running K10_DRIVER_COMPATIBILITY_MODE_LATEST
// unless ithas been upgraded and the upgrade requires a data
// structure change. Then the driver runs at OLD mode until
// specifically switched to LATEST via IOCTL_K10ATTACH_PUT_COMPAT_MODE
//
typedef enum K10_driver_compatibility_mode {
	K10_DRIVER_COMPATIBILITY_MODE_LATEST = 0,
	K10_DRIVER_COMPATIBILITY_MODE_OLD
} K10_DRIVER_COMPATIBILITY_MODE;

typedef enum K10_driver_quiesce_mode {
	K10_DRIVER_QUIESCE_MODE_QUEUED = 0,
	K10_DRIVER_QUIESCE_MODE_BUSY
} K10_DRIVER_QUIESCE_MODE;

typedef enum K10_ndu_mode {
	K10_DRIVER_NDU_MODE_INVALID = 0,
	K10_DRIVER_NDU_MODE_PRELIMINARY_SP,  //  The SP that reviews this opcode first, which is the secondary SP in the NDU operation.
	K10_DRIVER_NDU_MODE_FINAL_SP             //  Either the second SP or the only SP to receive this opcode, which would be the primary SP in the NDU operation.
} K10_DRIVER_NDU_MODE;



// TYPEDEFS

// These are all considered to store data in native (little-endian) format
// Note: Do not take sizeof(SOME_DEFINITION), use SOME_DEFINITION_SIZE(x).
// Pay attention to macro parameters, they are used to take size of the 
// variable payload sections into account. In all cases, the value returned
// from the SIZE() macro is what you would assign to dwDataSize.
//

// This structure enumerates the objects exported & consumed by the driver
//

typedef struct K10_enum_objects {
	unsigned int 			dwDataSize;
	unsigned short			wVersion;
	unsigned char			bReserved[2];
	unsigned int 			dwNumObjects;
	K10_DVR_OBJECT_NAME		nameList[1];

} K10_ENUM_OBJECTS;

#define K10_ENUM_OBJECTS_SIZE(ObjCount)					\
	(sizeof(K10_ENUM_OBJECTS) - sizeof(K10_DVR_OBJECT_NAME) +	\
	(ObjCount) * sizeof(K10_DVR_OBJECT_NAME))

// This structure is used by auto insert driers to list the WWNs of the
// LUs it is currently layering
//

typedef struct K10_enum_wwns {
	unsigned int 			dwDataSize;
	unsigned short			wVersion;
	unsigned char			bReserved[2];
	unsigned int 			dwNumObjects;
	K10_LU_ID				nameList[1];

} K10_ENUM_WWNS;

#define K10_ENUM_WWNS_SIZE(ObjCount)				\
	(sizeof(K10_ENUM_WWNS) - sizeof(K10_LU_ID) +	\
	(ObjCount) * sizeof(K10_LU_ID))


// This structure defines the object's generic properties
//
typedef struct K10_gen_object_props {
	unsigned int 			dwDataSize;
	unsigned short			wVersion;
	unsigned char			bReserved;
	unsigned char			bConsumed;			// if true, must exist in map already!
	K10_DVR_OBJECT_NAME		objectName;			// redundant in read (fn. parameter)
	K10_LOGICAL_ID			comment;
	K10_LU_ID				luId;				// Read-Only, required in Write
	ULONG				dwCreatorStamp;		// definable by creating driver, Flare == LUN
	unsigned char			btNoExtend;			// Deny layering. Only valid for consumers/creators
	unsigned char			btFrontEndFlags;	// used by TCD only (def own, etc.)
	unsigned char			location;			// Indicates which occurrence in device stack
	unsigned char			bIsCreator;			// if true, driver is creator (not a layer)
	unsigned int 			dwBoundObjectCount;	// Read-Only (0L in Write)
	K10_DVR_OBJECT_NAME		boundObjectList[1];	// Read-Only (not included in Write)
	
} *PK10_GEN_OBJECT_PROPS, K10_GEN_OBJECT_PROPS;

#define K10_GEN_OBJECT_PROPS_SIZE(ObjCount)					\
	(sizeof(K10_GEN_OBJECT_PROPS) - sizeof(K10_DVR_OBJECT_NAME) +	\
	(ObjCount) * sizeof(K10_DVR_OBJECT_NAME)) 


// This structure defines the object's specific properties
// The driver only needs report properties on objects it exports,
// not objects it consumes
//
typedef struct K10_spec_object_props {
	unsigned int 			dwDataSize;		// redundant, but consistent
	unsigned short			wVersion;
	unsigned char			bReserved[2];
	K10_DVR_OBJECT_NAME		objectName;		// redundant in reads
	unsigned int 			dwPayloadSize;	// bytes in data section to follow
	unsigned char			data[1];		// beginning of driver-defined data

} K10_SPEC_OBJECT_PROPS;

#define K10_SPEC_OBJECT_PROPS_SIZE(PayloadSize)     \
	sizeof(K10_SPEC_OBJECT_PROPS) - 1 + PayloadSize


// This structure defines the objects to bind
// and the name to give the new object
//
typedef struct K10_bind_object {
	unsigned int 			dwDataSize;
	unsigned short			wVersion;
	unsigned char			bReserved;
	unsigned char			bIsRebind;
	unsigned int 			dwNumObjects;
	K10_DVR_OBJECT_NAME		newObjectName;
	K10_LU_ID				luId;			// Optional: if zerod, driver must assign
	unsigned char			location;
	K10_DVR_OBJECT_NAME		nameList[1];

} K10_BIND_OBJECT;

#define K10_BIND_OBJECT_SIZE(ObjCount)					\
	(sizeof(K10_BIND_OBJECT) - sizeof(K10_DVR_OBJECT_NAME) +	\
	(ObjCount) * sizeof(K10_DVR_OBJECT_NAME))


// This structure defines the item to unbind,
// and contains a hint as to whether we'll be rebinding.
// There is also a hint as to whether the unbind is part
// of a lun delete operation.
//
typedef struct K10_unbind_object {
	unsigned int 			dwDataSize;
	unsigned short			wVersion;
	unsigned char			bDeleteLun;
	unsigned char			bWillRebind;
	K10_DVR_OBJECT_NAME		objectName;

} K10_UNBIND_OBJECT;

#define K10_UNBIND_OBJECT_SIZE			sizeof(K10_UNBIND_OBJECT)



// This structure defines the object's compatibility mode
//
typedef struct K10_compatibility_mode_wrap {
	ULONG           				dwDataSize;
	unsigned short					wVersion;
	unsigned short                  wLevel;
	K10_DRIVER_COMPATIBILITY_MODE	mode;

} K10_COMPATIBILITY_MODE_WRAP;

#define K10_COMPATIBILITY_MODE_WRAP_SIZE		\
	sizeof(K10_COMPATIBILITY_MODE_WRAP)

typedef struct K10_quiesce_object {
	unsigned int 			dwDataSize;
	unsigned short			wVersion;
	unsigned char			bDoAll;		// if true, quiesce all objects, obj name ignored
	unsigned char			bQuiesce;	// 0x01 quiesce, 0x00  unquiesce
	K10_DVR_OBJECT_NAME		objectName;
	K10_LU_ID				luId;
	K10_DRIVER_QUIESCE_MODE	mode;

} K10_QUIESCE_OBJECT;

#define K10_QUIESCE_OBJECT_SIZE()					\
	sizeof(K10_QUIESCE_OBJECT)


typedef struct K10_rename_object {
	unsigned int 					dwDataSize;
	unsigned short					wVersion;
	unsigned char					bReserved[2];
	K10_LU_ID						luId;    // LuId to be renamed
	K10_LU_ID						newLuId; // LuId to use for renaming
} K10_RENAME_OBJECT;

#define K10_RENAME_OBJECT_SIZE() \
	sizeof(K10_RENAME_OBJECT)


typedef struct K10_master_wwn_object {
	unsigned int 					dwDataSize;
	unsigned short					wVersion;
	unsigned char					bReserved[2];
	K10_LU_ID						luId;    // Other LuId to be quiesced
} K10_MASTER_WWN_OBJECT;

#define K10_MASTER_WWN_OBJECT_SIZE() \
	sizeof(K10_MASTER_WWN_OBJECT)


#define  K10_SHUTDOWN_OPERATION_WARNING		0x00
#define  K10_SHUTDOWN_OPERATION_SHUTDOWN	0x01

typedef struct K10_shutdown {
	unsigned int 			dwDataSize;
	unsigned short			wVersion;
	unsigned char			bType;
	unsigned char			bReserved;
	
} K10_SHUTDOWN;

#define K10_SHUTDOWN_SIZE()					\
	sizeof(K10_SHUTDOWN)

typedef struct K10_driver_reassign {
	unsigned int 					dwDataSize;
	unsigned short					wVersion;
	unsigned char					bReserved[2];
	K10_DVR_OBJECT_NAME				newObjectName;
	K10_LU_ID						luId;
} K10_DRIVER_REASSIGN;

#define K10_DRIVER_REASSIGN_SIZE() \
	sizeof(K10_DRIVER_REASSIGN)



typedef struct K10_spoof_object {
	unsigned int 					dwDataSize;
	unsigned short					wVersion;
	unsigned char					bReserved[2];
	K10_LU_ID						OldId;
	K10_LU_ID						ConsumeId;
	K10_DVR_OBJECT_NAME				ConsumeObjectName;
	K10_DVR_OBJECT_NAME				newObjectName;
} K10_SPOOF_OBJECT;



typedef struct K10_complete_object {
	unsigned int 					dwDataSize;
	unsigned short					wVersion;
	unsigned char					bReserved[2];
	K10_LU_ID						luId;
	ULONG       					opaque0;
	ULONG       					opaque1;
} K10_COMPLETE_OBJECT;


typedef struct K10_create_delete_object {
	unsigned int 					dwDataSize;
	unsigned short					wVersion;
	unsigned char					bReserved[2];
	K10_LU_ID						luId;
	ULONG       					opaque0;
	ULONG       					opaque1;
	unsigned char					**ppBuff;	// Used sometimes on Deletes
	unsigned int 					lunNumber;
} K10_CRE_DEL_OBJECT;



typedef struct K10_identifier_props_object {
	unsigned int 		dwDataSize;
	unsigned short		wVersion;
	unsigned char		bReserved[2];
	unsigned int 		dwBindStamp;
	unsigned char		bRaidType;
	unsigned char		bDefaultOwner; // 0 = SPA, 1 = SPB
	unsigned char		bCurrentOwner; // 0 = SPA, 1 = SPB
	unsigned char		bReserved1[1];
	unsigned int 		dwReserved[2];
}  K10_IDENTIFIER_PROPS_OBJECT, *PK10_IDENTIFIER_PROPS_OBJECT;

// This object is used when refreshing the configuration as part of an install or uninstall.
//
typedef struct K10_refresh_configuration_object {
	unsigned int 			dwDataSize;
	unsigned short			wVersion;
	K10_DRIVER_NDU_MODE		mode;
} K10_REFRESH_CONFIGURATION_OBJECT, *PK10_REFRESH_CONFIGURATION_OBJECT;

#define K10_REFRESH_CONFIGURATION_OBJECT_SIZE		\
	sizeof(K10_REFRESH_CONFIGURATION_OBJECT)



// SEMANTICS

// This section describes use of the IOCTLS. Please adhere to these.
// Enforcement of the conditions are requested for final shipment versions.
//
// Note on 'in' buffers: On read operations, these buffers are
// passed to the Admin library in a struct; this data must be stripped;
// the IOCTL in Buf should be the driver object name only.

//IOCTL_K10ATTACH_ENUM_OBJECTS

//	lpInBuffer		= NULL
//	nInBufferSize	= 0
//	lpOutBuffer		= K10_ENUM_OBJECTS
//	nOutBufferSize	>= 4
//	lpBytesReturned <= nOutBufferSize, >=4


// IOCTL_K10ATTACH_GET_GENERIC_PROPS

//	lpInBuffer		= K10_DVR_OBJECT_NAME
//	nInBufferSize	= sizeof(K10_DVR_OBJECT_NAME)
//	lpOutBuffer		= K10_GEN_OBJECT_PROPS
//	nOutBufferSize	>= 4
//	lpBytesReturned <= nOutBufferSize, >=4


// IOCTL_K10ATTACH_GET_SPECIFIC_PROPS

//	lpInBuffer		= K10_DVR_OBJECT_NAME
//	nInBufferSize	= sizeof(K10_DVR_OBJECT_NAME)
//	lpOutBuffer		= K10_SPEC_OBJECT_PROPS
//	nOutBufferSize	>= 4
//	lpBytesReturned <= nOutBufferSize, >=4


// IOCTL_K10ATTACH_PUT_GENERIC_PROPS

//	lpInBuffer		= K10_GEN_OBJECT_PROPS
//	nInBufferSize	= K10_GEN_OBJECT_PROP_SIZE()
//	lpOutBuffer		= NULL
//	nOutBufferSize	= 0
//	lpBytesReturned	= NULL


// IOCTL_K10ATTACH_PUT_SPECIFIC_PROPS

//	lpInBuffer		= K10_SPEC_OBJECT_PROPS
//	nInBufferSize	= K10_SPEC_OBJECT_PROP_SIZE()
//	lpOutBuffer		= NULL
//	nOutBufferSize	= 0
//	lpBytesReturned	= NULL


// IOCTL_K10ATTACH_BIND

//	lpInBuffer		= K10_BIND_OBJECT
//	nInBufferSize	= K10_BIND_OBJECT_SIZE()
//	lpOutBuffer		= NULL
//	nOutBufferSize	= 0
//	lpBytesReturned	= NULL


// IOCTL_K10ATTACH_UNBIND

//	lpInBuffer		= K10_UNBIND_OBJECT
//	nInBufferSize	= K10_UNBIND_OBJECT_SIZE()
//	lpOutBuffer		= NULL
//	nOutBufferSize	= 0
//	lpBytesReturned	= NULL


// IOCTL_K10ATTACH_GET_COMPAT_MODE

//	lpInBuffer		= NULL
//	nInBufferSize	= 0
//	lpOutBuffer		= K10_COMPATIBILITY_MODE_WRAP
//	nOutBufferSize	= K10_COMPATIBILITY_MODE_WRAP_SIZE()
//	lpBytesReturned	<= nOutBufferSize, >=4


// IOCTL_K10ATTACH_PUT_COMPAT_MODE

//	lpInBuffer		= K10_COMPATIBILITY_MODE_WRAP
//	nInBufferSize	= K10_COMPATIBILITY_MODE_WRAP_SIZE()
//	lpOutBuffer		= NULL
//	nOutBufferSize	= 0
//	lpBytesReturned	= NULL

// IOCTL_K10ATTACH_QUIESCE

//	lpInBuffer		= K10_QUIESCE_OBJECT
//	nInBufferSize	= K10_QUIESCE_OBJECT_SIZE()
//	lpOutBuffer		= NULL
//	nOutBufferSize	= 0
//	lpBytesReturned	= NULL

// Note on Quiesce: if filtering for ability to quiesce an object is done in
// the driver, refusal should be via return of not OK from DeviceIOControl call
// and set of last error to K10_DISK_ADMIN_ERROR_NOT_SUPPORTED

// IOCTL_K10ATTACH_REASSIGN

//	lpInBuffer		= K10_DRIVER_REASSIGN
//	nInBufferSize	= K10_DRIVER_REASSIGN_SIZE()
//	lpOutBuffer		= NULL
//	nOutBufferSize	= 0
//	lpBytesReturned	= NULL

// IOCTL_K10ATTACH_REFRESH_CONFIGURATION

//	lpInBuffer		= K10_REFRESH_CONFIGURATION_OBJECT
//	nInBufferSize	= K10_REFRESH_CONFIGURATION_OBJECT_SIZE
//	lpOutBuffer		= NULL
//	nOutBufferSize	= 0
//	lpBytesReturned	= NULL

#endif // !defined (K10_DISK_DRIVER_ADMIN_H)
