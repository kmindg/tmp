// K10GlobalMgmt.h
//
// Copyright (C) 1998-2011 EMC Corporation
//
//
// Header for K10GlobalMgmt types and classes to be used by other
//	K10Admin private libraries. No third-party access
//
//	Revision History
//	----------------
//	24 Feb 99	D. Zeryck	Initial version
//	13 Jul 99	D. Zeryck	Support for transacting the autoassign of LU on bind
//	27 Jul 99	D. Zeryck	added STACK_OPS_NAME_STRING
//	26 Jan 00	D. Zeryck	added DRIVER_ERROR_MASK_STRING
//	18 Feb 00	H. Weiner	removed references to "mirrored" registry key
//	11 Apr 00	D. Zeryck	Added TestTransactionLog
//	14 Apr 00	D. Zeryck	Added btFrontEndFlags
//	23 Aug 00	B. Yang		Added IsConsumed()
//	05 Sep 00	B. Yang		Task 1774 - Cleanup the header files
//	25 Sep 00	D. Zeryck	Bug 1945 - Free & checked builds of user space not compatible
//	 2 Nov 00	D. Zeryck	10344 - have delete on read types
//  19 Apr 01   B. Yang     Gang Trespass - add gangId element to K10_TxnNodeDataAssign
//  26 Sep 01   C. Hopkins	Added GetListPointer method to support logging for bug 63544
//  27 Sep 01   K. Keisling Add K10_DEVICEMAP_TIMER for DLS recovery
//	03 Jun 02	C. Hopkins	Bug 74224: Added GetFlareObjects() method to K10DeviceMapPrivate
//	25 Jun 03	C. Hopkins	Added InternalLDReq class for nesting Admin Lib calls
//   7 Aug 03	E. Petsching	Add support for user enabled packages.
//  19 Oct 04   C. J. Hughes    Device Map Robustness
//	11 Feb 05	K. Boland	DIMS 120715.  Added defines in K10_TransactionControl to allow tracing/logging 
//	27 Jun 05	R. Hicks	DIMS 128185: Added K10DeviceMapPrivate::ScrubHostLUs() method
//							to be called from admintool.
//	15 Mar 05	R. Hicks	Device Map Scalability Enhancement
//	04 Aug 05	R. Hicks	Added IsStackOp() method to InternalLDReq
//	04 Nov 05	R. Hicks	DIMS 128216: K10_DEVICEMAP_DLS_FAIL_FAST_DELAY specifies
//							user DLS timeout on device map DLS lock
//	08 Dec 05	R. Hicks	DIMS 133878: add btIsCreator to K10_LunExtendedEntry
//	27 Dec 05	M. Brundage	DIMS 137265: Added support for drivers in device stack multiple times.
//	10 Jan 06	R. Hicks	Change checkCorruption() to return a value.
//	20 Jun 06	E. Carter	DIMS 147467:  Added reboot DLS lock #defines and GetRebootLock( ) method.
//	11 Jan 07	V. Chotaliya	One Arg Constructor Added in K10DeviceMapPrivate for AdminTool 
//								Verbose Functionality
//	03 May 07	R. Hicks	DIMS 168841: Increased K10_DEVICEMAP_DLS_FAIL_FAST_DELAY to 5 minutes.
//	07 Oct 07	R. Hicks	DIMS 178998: Increase K10_DEVICEMAP_DLS_FAIL_FAST_DELAY to 9 minutes.
//	12 Dec 06	R. Hicks	64-bit OS needs devmap shared memory mapping explicity put in global namespace
//	26 Feb 08	R. Hicks	DIMS 191807: K10_DEVICEMAP_MUTEX also needs to be 
//							explicity put in global namespace.
//	16 May 08	E. Carter	DIMS 196493: Add CheckWWNs( ) method to K10DeviceMapPrivate class.
//	13 Oct 08	R. Hicks	DIMS 210554: Include Thin LUs in K10_MAX_DEVICE_MAP_LUNS calculation
//	24 Oct 08	D. Hesi		DIMS 197128: Added Persist/Clear transaction support for quiesce/unquiesce operations
//	06 Jan 09	R. Hicks	DIMS 217632: Add GetGenericPropsListChangedOnly() and
//							LockDeviceMapSharedMode() methods to K10DeviceMapPrivate
//	09 Feb 09	R. Hicks	DIMS 219256: Add InvalidateDeviceMap() method
//	09 Apr 09	R. Hicks	Replace LockDeviceMapSharedMode() with ConvertToDeviceMapReadLock()
//	20 Aug 09	R. Hicks	DIMS 233627: Don't rebuild device map if StackOps returned error.
//	27 Oct 09	R. Hicks	DIMS 239841: Added lun delete notification to driver unbind
//	01 Feb 10	R. Hicks	Added HAS_COMPOSITE_DATA_NAME_STRING registry value
//	22 Jul 10	R. Hicks	Added mlAdminAutoInsertCount to K10LunExtendedList
//	08 Nov 10	R. Hicks	ARS 390024: Add separate list for features which are not layered drivers
//	10 Feb 11	R, Hicks	ARS 404090: Use HashUtil instead of WWIDAssocHashUtil for mnpLunHashTable
//	27 Aug 13	D. Hesi 	ARS 578073: Added new interface GetGenericPropsFromLib() to read LU objects for a given driver
///
#ifndef _K10GlobalMgmt_h_
#define _K10GlobalMgmt_h_

//--------------------------------------------------------------INCLUDES

#include <windows.h>	// GlobalAlloc stuff

#include <stddef.h>	// offsetof

#include "core_config.h"
#include "UserDefs.h"
#include "MManDispatch.h"
#include "K10GlobalMgmtExport.h"
#include "K10SystemAdminExport.h"
#include "CaptivePointer.h"
#include "K10TraceMsg.h"
#include "ndu_toc_common.h"
#include "HashUtil.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//--------------------------------------------------------------FWD DEFS

class K10_TransactionControl_Impl;
class K10_TransactionListImpl;

//---------------------------------------------------------------DEFINES

#ifndef NULL
#define NULL 0
#endif
//------------------- Psm Stuff

#define K10_PSM_FILE_TYPE	0x19283746 // magic number, to indicate validity

// This struct is used to format data in the transaction file
//
typedef struct K10_Psm_File_Hdr {
	ULONG       	dwFileType; // K10_PSM_FILE_TYPE
	ULONG       	dwFileSize;	// data following header
} K10_PsmFileHdr;

//------------------- Transaction Stuff

#define K10_TRANSACTION_VERSION		0

// Other versions moved to ndu_toc_common.h

#define K10_LUNEXT_ATTRIBS_VERSION_UNK	((unsigned short)-1)

#define K10_TRANSACTION_DUMP_FILE_NAME      EMCUTIL_BASE_TEMP, "K10TransactionDump.txt", NULL

//------------------- Device Map stuff

#define K10_DEVMAP_PSM_DATA_AREA_NAME			"K10_DevmapPsmData"
#define K10_DEVMAP_PSM_SER_DATA_AREA_NAME		"K10_DevmapPsmSerData"

// name of shared memory file mapping object
#ifdef ADMIN_USE_BOOST_FOR_DEVICE_MAP_SHARED_MEMORY
#define K10_DEVMAP_MAPPING_FILE_NAME			"DevMapFile"
#else
#define K10_DEVMAP_MAPPING_FILE_NAME			"Global\\DevMapFile"
#endif

// process providing shared memory for device map
#define K10_DEVMAP_MAPPING_PROCESS				"NduApp"

// file to be created when there is an error accessing the device map file mapping,
// to prevent multiple event log messages from being logged
#define K10_DEVMAP_ERROR_FILE_NAME	    EMCUTIL_BASE_TEMP, "k10_devicemap_error", NULL

//
// define practical max filter depth for calculating PSM file needs
//
#define MAX_FILTER_DEPTH			16
#define MAX_FILTER_DEPTH_V1			64

// see below #define K10_DEVMAP_PSM_DATA_AREA_MAX_SIZE

#define K10_DEVICEMAP_MUTEX		"Global\\K10DevicemapMutex"
#define K10_DEVICEMAP_TIMER		"K10DevicemapTimer"		// devicemap lock timer name

#define K10_DEVICEMAP_DLS_DOMAIN	"K10DevmapDLS"
#define K10_DEVICEMAP_DLS_LOCKNAME	"K10DevmapDLSLock"
#define K10_DEVICEMAP_DLS_FAIL_FAST_DELAY	9	// specified in minutes

#define K10_REBOOT_DLS_DOMAIN	"K10RebootDLS"
#define K10_REBOOT_DLS_LOCKNAME	"K10RebootDLSLock"
#define K10_REBOOT_DLS_FAIL_FAST_DELAY	DLS_INVALID_FAIL_FAST_DELAY		// wait forever

#define DRIVER_ORDER_SUBKEY_STRING	\
	"Initialization\\DriverOrder"		// Reg key of driver ordering, libnames

#define DRIVER_COUNT_STRING	"DriverCount"		// Reg value w/ count of drivers


#define DRIVERS_INSTALLED_SUBKEY_STRING	\
	"Initialization\\Drivers"				// Reg key where driver count & entries are
	
#define FEATURES_INSTALLED_SUBKEY_STRING	\
	"Initialization\\Features"				// Reg key where driver count & entries are
	
#define DRIVER_LIB_NAME_STRING	\
		"DriverLibName"						// Reg value of admin lib name for driver

#define FEATURE_LIB_NAME_STRING	\
		"FeatureLibName"					// Reg value of admin lib name for feature

#define DRIVER_NAME_STRING	\
		"DriverName"						// Reg value of name for driver

#define DRIVER_ERROR_MASK_STRING	\
		"DriverErrorMask"					// Reg value of error mask for driver

#define I_O_INITIATOR_NAME_STRING	\
		"I_O_INITIATOR"						// Reg value of name for optional IO initiator value

#define DRIVER_EXCLUSION_LIST_STRING	\
		"Exclude"							// Reg value of name for optional exclusion list

#define STACK_OPS_NAME_STRING	\
		"StackOps"							// Reg value of name for string holding 
											// stack operations data
#define AUTO_INSERT_NAME_STRING	\
		"AutoInsert"						// Reg value of name for sub-key holding 
											// auto insert flag
#define USER_ENABLED_STRING	\
		"UserEnabled"						// Reg value of name for if driver is enabled for
											// user use (as opposed to just array use) 

#define HAS_COMPOSITE_DATA_NAME_STRING	\
		"HasCompositeData"					// Reg value of name for if driver supplies 
											// composite data

// Values for corruptionType field of K10_Lun_Filter_Attribs
// NOTE: if a new value is added, the g_corruptionTypeStr array
// in mgmt\K10GlobalMgmt\K10LunExtendedList.cpp must also be updated 
// with a descriptive entry
#define K10_DEVICEMAP_NORMAL                    0
#define K10_DEVICEMAP_CORRUPT_NO_DEVICE_BELOW   1
#define K10_DEVICEMAP_CORRUPT_BAD_NAME_BELOW    2
#define K10_DEVICEMAP_CORRUPT_NO_CONSUMER       3
#define K10_DEVICEMAP_CORRUPT_EXTRA_CONSUMER    4
#define K10_DEVICEMAP_CORRUPT_EXTRA_FILTER      5
#define K10_DEVICEMAP_CORRUPT_NO_DEVICE_ABOVE   6
#define K10_DEVICEMAP_CORRUPT_NO_EXTEND         7
#define K10_DEVICEMAP_CORRUPT_ILLEGAL_LOCATION  8

// "file name" for standard output
#define K10_TEST_STDOUT_FLAG    "-"


//--------------------------------------------------------------------------ENUMS
// Note: we start at arb. value for 'magic'
//

#define K10_TXN_NODE_SUBTYPE_DELETEONREAD	0x10000000

enum K10_TXN_NODE_TYPE {
	K10_TXN_NODE_TYPE_NONE = 0,					// !NOT ALLOWED!
	K10_TXN_NODE_TYPE_MIN = 0x20000000, 
	K10_TXN_NODE_TYPE_INSERT_LD = K10_TXN_NODE_TYPE_MIN,	// inserting a layered driver
	K10_TXN_NODE_TYPE_REMOVE_LD,				// removing a layered driver
	K10_TXN_NODE_TYPE_ASSIGN,					// Assigning to TCD, put in PhysArray
	K10_TXN_NODE_TYPE_DEASSIGN,					// Deassigning form TCD, can't be in Varray
	K10_TXN_NODE_TYPE_REGEN,					// regen the devicemap; no other info needed
	K10_TXN_NODE_TYPE_WORKLIST,					// Worklist needs to be backed out
	K10_TXN_NODE_TYPE_UNQUIESCE,				// Unquiesce if LU was left in quiesce state
	K10_TXN_NODE_TYPE_STDADMIN,					// a standard admin page, starts with MM_3PtyParamBlock
	K10_TXN_NODE_TYPE_STDADMIN_DOR = K10_TXN_NODE_TYPE_STDADMIN | K10_TXN_NODE_SUBTYPE_DELETEONREAD,
	K10_TXN_NODE_TYPE_MAX = K10_TXN_NODE_TYPE_STDADMIN_DOR
};

//---------------------------------------------------------------------TYPEDEFS

///////// TRANS LOG
//
// Data structs for the node types
//

#define K10_TXN_NODE_STDADMIN_MAX_DATA		4096	// reasonable MP size...?

// The <K10_DVR_NAME, location> tuple uniquely describes a position in
// the device stack.  A location value of 0 is the lowest position (closest
// to the creator) for the driver in the stack, location 1 is the next 
// allowable position, etc.
typedef struct K10_TxnNodeData_LD {			// For insert/rem L.D's
	
	K10_LU_ID				luId;			// LU being operated on
	K10_DVR_OBJECT_NAME		oldTopDevName;	// If top not consumer, will need old devName
	K10_DVR_NAME			topDvr;			// driver on top of operDvr in stack
											// this is NEVER null--if not top dvr,
											// no coherency issues, no need for
											// a transaction
	K10_DVR_NAME			operDvr;		// operative driver (to rem/insert)
	unsigned char			topLocation;	// location value for topDvr
	unsigned char			operLocation;	// location value for operDvr
	unsigned char			btIsConsumer;	// is topDvr a consumer (else layerer)

} K10_TxnNodeDataLD;

typedef struct K10_TxnNodeData_Assign {		// For Assignment

	K10_LU_ID				luId;			// LU being operated on
	unsigned char			bAT;			// 1 = AutoTrespass enable
	unsigned char			bAA;			// 1 = AutoAssign enable
	unsigned char			bDefOwn;		// 0 = SPA, 1 = SPB
	K10_LU_ID				gangId;			// the master WWN which this lun slaved to
	unsigned char			bInternal;		// 1 = LUN is to be internal only


} K10_TxnNodeDataAssign;

typedef struct K10_TxnNodeData_Deassign {		// For Assignment

	K10_LU_ID				luId;			// LU being operated on

} K10_TxnNodeDataDeassign;

typedef struct K10_TxnNodeData_Unquiesce {		// For Unquiesce

	K10_LU_ID				luId;			// LU being operated on

} K10_TxnNodeDataUnquiesce;


typedef struct K10_TxnNodeData_StdAdmin {	// a idempotent K10StdAdmin page, serialized

	unsigned char			serData[K10_TXN_NODE_STDADMIN_MAX_DATA];

} K10_TxnNodeDataStdAdmin;

////////////// The UNION that Binds them...
//
typedef union K10_TxnNodeData_U {

	K10_TxnNodeDataLD			nodeDataLD;
	K10_TxnNodeDataAssign		nodeDataAssign;
	K10_TxnNodeDataDeassign		nodeDataDeassign;
	K10_TxnNodeDataStdAdmin		nodeDataStdAdmin;
	K10_TxnNodeDataUnquiesce	nodeDataUnquiesce;

} K10_TxnNodeDataU;

typedef struct K10_txn_node {

	LONG                    version;		// K10_TRANSACTION_VERSION
	K10_TXN_NODE_TYPE		type;			// K10_TXN_NODE_TYPE

	K10_TxnNodeData_U		uData;			//payload

} K10_TxnNode;

// DEVICEMAP
//

// This is used to form a linked list of filter attribs for a LU
// Used by K10_LunExtendedAttribs
// pPrev		- previous item in list
// objectName	- device name of object exported
// exporterId	- name of driver which exported it
// corruptionType - type of corruption detected, if any (see also mapCorrupt)
//                  values are K10_DEVICEMAP_CORRUPT_xxx (see above)
// location		- Which of the allowable positions for this driver in the 
//					device stack this filter corresponds to.

// pNext - pointer to object it was attached from

// DEVICEMAP
//
typedef struct K10_Lun_Filter_Attribs {

	K10_Lun_Filter_Attribs	*pPrev;		// NULL if first
	unsigned short				version;
    unsigned char               corruptionType;
	unsigned char				location;
	K10_DVR_OBJECT_NAME			objectName;
	K10_DVR_NAME				exporterId;
	K10_Lun_Filter_Attribs		*pNext;		// NULL if last
	
} K10_LunFilterAttribs;

// DEVICEMAP
// This structure is created by the Modepage Redirector (MManModepageExtended)
// each time a LU must have its state changed. This is because the
// drivers are responsible for keeping their databases cross-SP aware,
// NOT the Redirector, which has no PSM file, cross-SP locks, etc.
//
// consumerId - driver which 'owns' (has consumed) it - all 0's if not consumed
//      Note: this is usually the TCD but could be Snap (using in SnapCache)
// comment - "nice" name assigned by user.
// luId - unique ID of object
// btIsConsumed - 0x1 == consumed by a module.
//              E.g., consumed in a VTARG, SnapCache, etc. 
//              Can't be consumed by another until it is unconsumed
// btNoExtend   - a creating or consuming driver has marked this as
//              non-extendable - do not allow layering of this device
// btPublicLU   - this is visible to hosts (in XLU map)
// dwCreatorStamp - any creator-specific data. Example: if a Flare LU,
//              the FLU #
// dwFilterDepth    - number of objects in filter list
// pFilterList - device names of the object & driver names which export
//              the object the last one is the creator of the object,
//              the first in the list is the name highest in stack
// mapCorrupt   - TRUE if device map corruption has been detected for this LU
typedef struct K10_Lun_Extended_Attribs {
	unsigned short			version;
    unsigned char           mapCorrupt;         // boolean
    unsigned char           corruptionType;     // for consumer
	K10_DVR_NAME			consumerId;
	K10_LOGICAL_ID			comment;			
	K10_LU_ID				luId;	
	unsigned char			btIsConsumed;
	unsigned char			btNoExtend;
	unsigned char			btPublicLU;
	unsigned char			btFrontEndFlags;
	ULONG				dwCreatorStamp;
	ULONG				dwFilterDepth;
	K10_LunFilterAttribs	*pFilterList;

} K10_LunExtendedAttribs;

//DEVICEMAP
// This structure is used by the Redirector to build the list:
// both to add objects, and mark them as consumed.

// exporterId		- Driver which is exporting this object
// layeredObjectName- name of object we layered
// object Name		- name the driver gives the object
// comment			- 'nice' name
// luId			- GUID of object
// btNoExtend	- a creating or consuming driver has marked this as non-extendable - do not
//				allow layering of this device
// btPublicLU	- this is visible to hosts (in XLU map)
// location		- Which of the allowable positions for this driver in the 
//					device stack this filter corresponds to.
// dwcreatorStamp	- a 32-bit value used by the creator

typedef struct K10_Lun_Extended_Entry {
	unsigned short			version;
	K10_DVR_NAME			driverId;
	K10_DVR_OBJECT_NAME		layeredObjectName;
	K10_DVR_OBJECT_NAME		objectName;
	K10_LOGICAL_ID			comment;
	K10_LU_ID				luId;	
	unsigned char			btNoExtend;
	unsigned char			btPublicLU;
	unsigned char			btFrontEndFlags;
	unsigned char			btIsCreator;
	unsigned char			location;
	ULONG				dwCreatorStamp;

} K10_LunExtendedEntry;

// LUNEXTLIST
// this struct is based on K10_LunFilterAttribs 
// It is used to serialize that struct
//
typedef struct K10_Lun_Filter_Attribs_Ser {
	unsigned short				version;
    unsigned char               corruptionType;
	unsigned char 				location;
	K10_DVR_OBJECT_NAME			objectName;
	K10_DVR_NAME				exporterId;
} K10_LunFilterAttribsSer;

// old version
typedef struct K10_Lun_Filter_Attribs_Ser_V1 {
	unsigned short				version;
	unsigned short 				reserved;
	K10_DVR_OBJECT_NAME			objectName;
	K10_DVR_NAME				exporterId;
} K10_LunFilterAttribsSer_V1;


// LUNEXTLIST
#define K10_MAPFILE_HDR_VERSION			3L
#define K10_MAPFILE_HDR_VERSION_2		2L
#define K10_MAPFILE_HDR_VERSION_1           1L

// LUNEXTLIST
// this struct is based on K10_Lun_Extended_Attribs 
// It is used to serialize that struct
//
typedef struct K10_Lun_Ext_Attribs_Ser {
	unsigned short			version;
    unsigned char           mapCorrupt;         // boolean
    unsigned char           corruptionType;     // for consumer
	K10_DVR_NAME			consumerId;
	K10_LOGICAL_ID			comment;			
	K10_LU_ID				luId;	
	unsigned char			btIsConsumed;
	unsigned char			btNoExtend;
	unsigned char			btPublicLU;
	unsigned char			btFrontEndFlags;
	ULONG				dwCreatorStamp;
	ULONG				dwFilterDepth;
	K10_LunFilterAttribsSer	FilterList[1];

} K10_Lun_ExtAttribs_Ser;

// old version
typedef struct K10_Lun_Ext_Attribs_Ser_V1 {
	unsigned short			version;
	K10_DVR_NAME			consumerId;
	K10_LOGICAL_ID			comment;			
	K10_LU_ID				luId;	
	unsigned char			btIsConsumed;
	unsigned char			btNoExtend;
	unsigned char			btPublicLU;
	unsigned char			btFrontEndFlags;
	ULONG				dwCreatorStamp;
	ULONG				dwFilterDepth;
	K10_LunFilterAttribsSer	FilterList[1];

} K10_Lun_ExtAttribs_Ser_V1;

#define K10_INVALID_DEVMAP_GENERATION		0L

// MAPFILE
// this struct is the header for the mapfile
//
typedef struct K10_Map_File_Hdr {
	ULONG dwFileSize;		// number of bytes, incl. hdr
	ULONG dwVersion;		// header version
	ULONG dwObjCount;		// number of K10_Lun_ExtAttribs_Ser entries
}K10_Mapfile_Hdr;

#define K10_MAPFILE_GEN_VERSION		1L

// this struct describes the mapfile generation number
typedef struct K10_Mapfile_Gen {
	ULONG	dwVersion;		// header version
	ULONG	dwGeneration;	// device map generation number
}K10_Mapfile_Gen;

// new version
typedef struct K10_Map_File_Hdr_V2 {
	K10_Mapfile_Hdr mapFileHdr;
	K10_Mapfile_Gen mapFileGen;
}K10_Mapfile_Hdr_V2;

//
// Compute the max size of our PSM area. Header plus fixed chunk for each LU
// plus max number of filteres--wherein lies the rub. How many?
//
#define K10_MAX_DEVICE_MAP_LUNS	\
	( /*Snap&FAR, etc.*/(MAX_HOST_ACCESSIBLE_LUNS-PLATFORM_HOST_ACCESSIBLE_FLARE_LUNS)	\
	+ /* Flare */ PLATFORM_HOST_ACCESSIBLE_FLARE_LUNS	\
	+ (/*# metas */ PLATFORM_HOST_ACCESSIBLE_FLARE_LUNS/2) \
	+ (/*# Pool-based LUs */ MAX_POOL_LUS) )

#define K10_SERIALIZED_DEVMAP_MAX_SIZE	\
	/* Header has a fixed size */ \
	( sizeof(K10_Mapfile_Hdr) + 	\
	/* add fixed bytes in K10_LunExtendedAttribs for each LU */\
	(K10_MAX_DEVICE_MAP_LUNS * offsetof(K10_Lun_ExtAttribs_Ser, FilterList)) + \
	/* Add (100) bytes for EACH LU's MAX number of filters */ \
	(K10_MAX_DEVICE_MAP_LUNS * MAX_FILTER_DEPTH * \
        sizeof(K10_LunFilterAttribsSer)) )

// allow space at the end of the serialized device map for the generation number
#define K10_MAPPED_DEVMAP_SIZE	( K10_SERIALIZED_DEVMAP_MAX_SIZE + sizeof(K10_Mapfile_Gen) )

#define K10_DEVMAP_PSM_DATA_AREA_MAX_SIZE	\
	/* Header has a fixed size */ \
	( sizeof(K10_Mapfile_Hdr) + 	\
	  sizeof(K10_Mapfile_Gen) )

#define K10_DEVMAP_PSM_DATA_AREA_MAX_SIZE_V2	\
	/* Header has a fixed size */ \
	( sizeof(K10_Mapfile_Hdr) + 	\
	/* add fixed bytes in K10_LunExtendedAttribs for each LU */\
	(FLARE_MAX_USER_LUNS * offsetof(K10_Lun_ExtAttribs_Ser, FilterList)) + \
	/* Add (100) bytes for EACH LU's MAX number of filters */ \
	(FLARE_MAX_USER_LUNS * MAX_FILTER_DEPTH_V1 * \
        sizeof(K10_LunFilterAttribsSer)) )

#define K10_DEVMAP_PSM_DATA_AREA_MAX_SIZE_V1	\
	/* Header has a fixed size */ \
	( sizeof(K10_Mapfile_Hdr) + 	\
	/* add fixed bytes in K10_LunExtendedAttribs for each LU */\
	(FLARE_MAX_USER_LUNS * offsetof(K10_Lun_ExtAttribs_Ser_V1, FilterList)) + \
	/* Add (100) bytes for EACH LU's MAX number of filters */ \
	(FLARE_MAX_USER_LUNS * MAX_FILTER_DEPTH_V1 * \
        sizeof(K10_LunFilterAttribsSer_V1)) )


//--------------------------------------------------------------GLOBALS

DWORD CSX_MOD_EXPORT GetBreakParam();

//--------------------------------------------------------------CLASSES

///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// takes handoff of trans log from upgrade
// !FIX how to designate file
//
// ops on this this MUST be protected by DeviceMap lock!
//
class CSX_CLASS_EXPORT K10_TransactionControl {
public:

	K10_TransactionControl();
	~K10_TransactionControl();

	// takes ptr to existing trans log & executes
	//
	// throws
	//
	// returns K10_DISK_ADMIN_ERROR_NO_TRANSLOG or SystemAdmin errors
	// takes ptr to existing trans log & executes
	// Note: This is ONLY CALLED FROM DevicemapLock. This
	//	revents recursive processing, which we do not want
	//
	long ProcessTransLog();

	// THROWS
	//
	void PersistTransNode(K10_TxnNode& Node);

	// THROWS
	//
	void PersistWorkList(K10_WorkList * pList);

	// Will return false if no translog there, not always an error,
	// as one some recoveries we may have scrubbed the trans log
	//
	bool DecrementTransNode(K10_TxnNode& Node);

	// THROWS
	//
	void ClearTransactionLog();

	HRESULT TestTransactionLog();

	HRESULT DumpTransactionLog(const char *pFileName, bool bAscii);

private:
	K10_TransactionControl_Impl	*mpImpl;
	NPtr <K10TraceMsg>		 mnpK;
	char				*mpProc;

};


class CSX_CLASS_EXPORT K10LunExtendedList
{
public:

	// Creates an empty list. You can deserialize into it, or start
	// adding objects.
	//
	K10LunExtendedList();

	// deletes all objects before destructing.
	//
	~K10LunExtendedList();

	// deletes the list, resets to state as just constructed
	//
	void DeleteAll();

	// This allocates internally & copies from the incoming pointer. 
	// Checks for existance of object via WWN and if found adds to filter 
	//	list, else adds new entry into the list as creator.
    // If the regen flag is TRUE, this is a device map regeneration and
    // corruption will be logged, but not return an error.
	//
	long AddObject(const K10_LunExtendedEntry *pIn, 	// add one to the list
                   bool regen = FALSE);

	// this marks the object named by the luId as CONSUMED by the driver whose 
	// ID is the driverId in pIn. If already consumed, returns E_FAIL.
	// regen - if TRUE, this is a device map rebuild and errors should be
	//         logged and marked in the map
	//
	long ConsumeObject(const K10_LunExtendedEntry *pIn, bool regen = FALSE);

	// this marks the object named by the luId as NOT consumed. The driver whose 
	// ID is the driverId in pIn must match current consumer name, else returns E_FAIL
	//
	long UnconsumeObject( const K10_LunExtendedEntry *pIn);

	// This marks a LUN as being created by a different driver.  It's really very
	// special purpose, used only during a WorkList "Spoof" operation.
	//
	long RemapCreator(const K10_LunExtendedEntry *pIn);

	// Remove from the filter list of the object named by the luId the entry of 
	// the driver whose ID is the driverId in pIn. Returns E_FAIL if the named driver 
	// is the creator of the object (for that you must remove all filters & then use 
	// RemoveObjectFromList to delete the object). Returns 0x00000002 if the object is not found
	//
	long RemoveFilterEntry( const K10_LunExtendedEntry *pIn );

	// Completely removes the object from the list. Returns 0x00000002 if it
	// does not exist or E_FAIL if it is consumed or has entries in the filter list.
	//
	long RemoveObjectFromList( K10_LU_ID & objectId ); // find & remove

	// Admin Libs: Do NOT call this when in context of Enumerate or GetXXXProps.
	//
	long GetObjectState( K10_LU_ID & objectId, K10_LunExtendedInfo & extInfo );

	// returns count of objects
	//
	long GetCount();		

	// returns size of file needed for Serialize method
	//
	long  GetMaxFileSize();

	// Write to a memory mapped file. If supplied, then write out to PSM file.
	//
	// hMapFile		IN		Handle to file used to back up Memory Map
	// pFileName	IN		PSM file
	// dwGenNum		IN		generation number
	//
	void Serialize( void *hMapFile, const char *pFileName, DWORD dwGenNum);

	// If given, get data from designated PSM file, dump into MEMORY MAPPED file
	// Read from the map file given in binary format.
	//
	// hMapFile		IN		Handle to file used to back up Memory Map
	//
	long Deserialize( void *hMapFile, const char *pFileName);

	// Invalidates the PSM file, causing the device map to be
	// rebuilt next time it is instantiated
	void InvalidateFile( const char *pFileName );

	unsigned short GetFileVersion(const char *pFileName);
	ULONG GetFileGenerationNumber(const char *pFileName);											  

	// dumps in ASCII format
	//
	HRESULT Dump(const char *pFileName, bool corruptOnly = false);
	HRESULT Dump(const char *pFileName, K10_LU_ID &luID);
	HRESULT Dump(const char *pFileName, unsigned int index);

	// returns a pointer to the indexed object.
	//
	K10_LunExtendedAttribs* operator[](unsigned long) const;		// iterate

	// returns index of object if return is true.
	//
	bool FindObjectInList( const K10_LU_ID & objectId, int &idx);
    bool FindObjectInList( const K10_LOGICAL_ID & niceNameId, int & idx);

	K10_LunExtendedAttribs	** GetListPointer();

	//
	// AddToFilterStack - NOT for the faint of heart...
	//
	// pObject		IN	Master object record to be modified
	// pAdd			IN	Filter record to be added to master
	// position		IN	Position in list: if nonzero, jumps that many places in filter list
	// regen        IN  if TRUE, this is a device map rebuild and errors
	//                  should be logged and marked in the map
	//
	static HRESULT AddToFilterStack(K10_LunExtendedAttribs *pObject,
                                    const K10_LunExtendedEntry * pAdd, 
                                    unsigned long position = 0,
                                    bool regen = FALSE);

	//
	// RemoveFromFilterStack - NOT for the faint of heart...
	//
	// pObject		IN	Master object record to be modified
	// pRemove		IN	Filter record to be removed from master
	//
	static HRESULT RemoveFromFilterStack( K10_LunExtendedAttribs *pObject, const K10_LunExtendedEntry * pRemove);

	bool IsConsumed(int iIndex);
	enum K10_EXTLIST_FILE_ERR {
		K10_EXTLIST_FILE_ERR_LEN =64
	};

    // Accessor members for mversion
    unsigned short GetCompatLevel(void);
    void SetCompatLevel(unsigned short);

    /********** Test Members, for use with AdminTool ********/
	void RemoveLayer(const K10_LunExtendedEntry *pIn);

private:
	// First step in serializing: put it into a memory mapped object for quick
	// dump into PSM
	// throws
	//
	// hMapFile		IN		Handle to file used to back up Memory Map
	//
    // The _V1 version is used to write out the old version of the file.
	void SerializeToFilemap(void *hMapFile, DWORD *pFileSize, DWORD dwGenNum);
    void SerializeToFilemap_V1(void *hMapFile, DWORD *pFileSize, DWORD dwGenNum);

	// Second step in serializing: put it into a PSM file from memory mapped object
	//throws
	//
	// hMapFile		IN		Handle to file used to back up Memory Map
	//
	void SerializeToFile( void *hMapFile, const char *pFileName, DWORD dwFileSize );

	// First step in deserialization: get data from PSM into memory-mapped file
	//
	// hMapFile		IN		Handle to file used to back up Memory Map
	// dwFileSize	OUT		Size of file hMapFile
	//
	long DeserializeFromFile( void *hMapFile, const char *pFileName );

	// Second step in deserialization: get data from memory-mapped file into list
	//
	// hMapFile		IN		Handle to file used to back up Memory Map
    // pMapViewTop  IN      Pointer to data in filemap (used in _V1 function)
	// dwFileSize	IN		Number of bytes in hMapFile
	//
    // The _V1 function is used to read in the old version of the file.
	long DeserializeFromFilemap( void *hMapFile );
	long DeserializeFromFilemap_V1(char *pMapViewTop );

	HRESULT AllocateChunk();
	void WriteFileOrThrow(  fstream &sFile,  char *lpBuffer, bool bUseStdout );
	static char * FormatluId( const K10_LU_ID& wwn);

	void DeleteFilterList(K10_LunExtendedAttribs * pItem);
	void DeleteEntry(K10_LunExtendedAttribs *pItem);
	void DeleteList(K10_LunExtendedAttribs **ppList, long count);

    static K10_LunFilterAttribs *NewFilter(K10_LunExtendedAttribs *pObject,
                                           K10_LunFilterAttribs *pNext,
                                           K10_LunFilterAttribs *pPrev,
                                           const K10_LunExtendedEntry *pAttr);

    void dumpLU(fstream &sFile, const K10_LunExtendedAttribs *luAttr, bool bUseStdout);


	// Attributes

	K10_LunExtendedAttribs	**mppList;
	char					mpErr[K10_EXTLIST_FILE_ERR_LEN];
	long					mlCount;
	long					mlAlloc;
	NPtr <K10TraceMsg>      mnpK;
	char                   *mpProc;
	unsigned short          mversion;        // for commit processing

	// hash table for quick object lookups by WWID
	NPtr< HashUtil<K10_WWID, int> >	mnpLunHashTable;

	long	mlAdminAutoInsertCount;
};

// This class is used to access, and hold, the Device Map. It is multithread
// friendly, and uses a static structure so only 1 instance is necessary.

// Note on return values and exceptions:
//	This class will use the long return values to pass back errors which are
//	received via GetLastError() from Win32 functions. For errors pertaining to
//	class semantics (file size wrong, for example), this class will throw a 
//	K10Exception.
//
class K10DeviceMapImpl;

class CSX_CLASS_EXPORT K10DeviceMapPrivate
{
public:

	K10DeviceMapPrivate();

	//Overloaded constructor which takes argument for AdminTool verbose
	K10DeviceMapPrivate(int iTracePath);

	// calls ReleaseDeviceMap to release locks
	//
	~K10DeviceMapPrivate();

	// Locks the device map without reading the file. If bWait = FALSE, will 
	// not wait but return WAIT_TIMEOUT if anyone else is holding the lock
	// If bWait is true waits infinite.
	//
	// bDualSP_Lock to lock on both SPs (default)
	//
	long LockDeviceMap( bool bWait, bool bDualSP_Lock = true );

	// Converts the Device Map lock from Write to Read mode
	void ConvertToDeviceMapReadLock( );

	// Gets the Reboot Lock and Device Map lock, and returns 
	// without releasing them.
	//
	void GetRebootLock( );

	// Releases the Reboot Lock.
	//
	void ReleaseRebootLock( );

	// Tell us if the map was successfully locked so we know if we have to
	// try unlocking
	//
	bool IsMapLocked();

	//
	// Get a copy of the list of extended LUN attributes (device map)
	// This LOCKS the map until Release is called or object is destructed
	// 
	long GetGenericPropsList( K10LunExtendedList& list, bool bRegen = false );

	//
	// Same as GetGenericPropsList(), but only updates the list
	// if the provided generation number is no longer current
	// 
	long GetGenericPropsListChangedOnly( K10LunExtendedList& list,
		ULONG clientGenNum);

	//
	// Method added to get Device list in K10SystemAdmin
	// 
	void GetGenericPropsFromLib(const char * pLibName, 
								CListPtr <K10_GEN_OBJECT_PROPS> & genObjPropList, 
								DWORD& dwObjectCount);

	// Writes new list
	// throws
	//
	long WriteGenericPropsList( K10LunExtendedList& list );

	// Unlocks the global copy. You cannot call Write until you get another
	// copy. Fails if it cannot release mutex, close file, etc.
	//
	void ReleaseDeviceMap( );

	// gets state of one object
	//
	long GetObjectState( K10_LU_ID & objectId, K10_LunExtendedInfo & extInfo );

	// Special usage to get Flare enum list
	void GetFlareObjects (csx_uchar_t **ppOut, long *lpBufSize);

	HRESULT checkCorruption(K10LunExtendedList &List);

	HRESULT ScrubHostLUs( K10LunExtendedList & List, bool bScrubAllLUs );

	unsigned long GetGenerationNumber();

	HRESULT CheckWWNs(K10LunExtendedList & List);

	HRESULT ScrubCacheVolumes(K10LunExtendedList & List);

	// Invalidates the the device map, causing it to be
	// rebuilt the next time it is instantiated.
	// Note that this does NOT lock the map.
	void InvalidateDeviceMap();

private:
	K10DeviceMapImpl	*pImpl;
	int					miTracePath; //Whether Verbose ON or OFF
};



class CSX_CLASS_EXPORT  K10LayerDriverInternalUtil  
{
public:
	K10LayerDriverInternalUtil();
	virtual ~K10LayerDriverInternalUtil();

	// We want to optimize for normal ops when we may call
	// the same dispatch obj twice, so allow pass in of dispatch OR name of lib
	//
	// driverName	IN	Name of driver to create dispatch. Can be NULL if
	//					pDispatch is non-NULL
	// objectName	IN	Name of object to which we unbind
	// bWillRebind	IN	Hint: will we be rebinding this before the next reboot?
	// pDispatch	IN	Pointer to dispatch object. Can be NULL if driverName
	//					is non-NULL
	//
	HRESULT UnbindFilterObject( K10_DVR_NAME driverName, K10_DVR_OBJECT_NAME objectName, 
		bool bWillRebind, bool bDeleteLun, MManAdminDispatch *pDispatch = NULL);

	// We want to optimize for normal ops when we may call
	// the same dispatch obj twice, so allow pass in of dispatch OR name of lib
	//
	// driverName		IN	Name of driver to create dispatch. Can be NULL if
	//						pDispatch is non-NULL
	// location			IN	Indicates what occurrence of this driver in the 
	//						stack is being bound.  Lowest occurrence of this 
	//						driver in stack is location 0, next is 1, etc.
	// objectName		IN	Name of object to which we bind
	// newObjectName	IN/OUT	Name of object now exported.
	// bIsRebind		IN	Is this a rebind of previously bound object?
	// pDispatch		IN	Pointer to dispatch object. Can be NULL if driverName
	//						is non-NULL
	//
	HRESULT BindFilterObject( K10_DVR_NAME driverName, 
								unsigned char location,
								K10_DVR_OBJECT_NAME objectName, 
								K10_DVR_OBJECT_NAME newObjectName, 
								K10_LU_ID& luId, 
								bool bIsRebind, 
								MManAdminDispatch *pDispatch = NULL);

	// Check to see if the DbId and Opcode combo for this driver is registered as
	// one which perturbs the device stack
	//
	BOOL CheckStackOps( const char *pLibName, DWORD dbId, DWORD opQual );

	// Return true if the dbid:opcode combination are in the StackOps string
	//
	BOOL ParseStackOps(const char *pStackOps, DWORD dbId, DWORD opQual);

	// IS this driver registered as an IO Initiator?
	// THROWS if not instaled.
	//
	BOOL IsIoInitiator( const char *pLibName );

	// See if this in a varray
	//
	bool UsedInVirtualArray(K10_LU_ID &luId);

	bool VerifyBindName( K10_DVR_OBJECT_NAME name );

private:
	K10LayerDriverUtil		mLdUtil;

};

class CSX_CLASS_EXPORT InternalLDReq 
{
public:

	//	Constructors and Destructor
	InternalLDReq();
	~InternalLDReq();

	HRESULT Set(const TLD *pConstraint);
	HRESULT Get(TLD **ppTldReturn,
                    const TLD *pConstraint);

	bool IsStackOp();

private:

	void ParseTLD(const TLD *pConstraint);
	HRESULT PreOp(const TLD *pConstraint);
	void PostOp(HRESULT LDStatus);
	void LDErrorHook();
	bool LDExpectedError(HRESULT LDStatus);

	TLD			*mpTldToSend;
	DWORD		mdwDbId;			// Identifier of DB type to transact.
	DWORD		mdwDbOpQual;		// Operation qualifier for DB

	K10LunExtendedList			* mpLunList;
	K10DeviceMapPrivate			* mpDeviceMap;
	MManAdminDispatch			* mpAdminDispatch;
	K10_DVR_NAME				mLibName;
	K10LayerDriverInternalUtil	mLDUtil;
	K10_TxnNode					mNode;
	bool						mbLock;
	char						mdest[256];
#pragma warning(disable:4251)
	NPtr <K10TraceMsg>			mnpK;
#pragma warning(default:4251)

};

#endif	// _K10GlobalMgmt_h_
