// MMan.h
//
// Copyright (C) 1997-2009	EMC Corporation
//
//
// Header for all MessageManager clients
//
//	Revision History
//	----------------
//	10 Jul 97	D. Zeryck	Initial version.
//	18 Jun 98	D. Zeryck	Add MMAN_3RD_PTY_CURRENT_VERSION, param block
//	 2 Oct 98	D. Zeryck	Remove def of K10FlareAdmin lib name.
//	11 Nov 98	D. Zeryck	MManSendRcv ctor to allow direct TCP/IP
//	17 Dec 98	D. Zeryck	Remove some defines to K10defs.h
//	08 Apr 99	D. Zeryck	Added MMAN_ERROR_UNKNOWN_EXCEPTION, MMAN_ERROR_UNKNOWN_SP
//   3 Jun 99	D. Zeryck	Expanded MMAN_3RD_PTY_ for use in redirector
//	19 Aug 99	D. Zeryck	Added MMAN_ERROR_TRAP_LOGIC_NOERR
//	23 Nov 99	D. Zeryck	Added MMAN_ERROR_UNKNOWN_DBID
//	23 Nov 99	D. Zeryck	Added MMAN_ERROR_FMODE_NOT_SUPPORTED
//	 4 Jan 00	D. Zeryck	Added MMAN_ERROR_RA_NOT_INITIALIZED
//	31 Jan 00	D. Zeryck	Added MDispatcher errors for logging
//	 2 Feb 00	D. Zeryck	Added MMAN_ERROR_PORT_OUT_OF_RANGE
//	15 Feb 00	D. Zeryck	Add MMAN_ERROR_LU_EXP_UNSUPPORTED
//	 1 Mar 00	D. Zeryck	Add MMAN_ERROR_TRESPASS_UNSUPPORTED
//	15 Mar 00	D. Zeryck	Add MMAN_ERROR_DEGRADED_MODE, MMAN_ERROR_PSM_NO_WCE
//	21 Mar 00	H. Weiner	Add MSGDISP errors and several more MMAN errors.
//	11 APR 00	D. Zeryck	Add errs for trespass, broken PSM
//	23 May 00	B. Yang		Add MMAN_ERROR_IOCTL
//	 2 Jun 00	D. Zeryck	Bug 942
//	21 Jun 00	D. Zeryck	ASSERT removal
//	01 Sep 00	B. Yang		Task 1774 - Cleanup the header files
//	21 Sep 00	B. Yang		Task 1938
//	14 Dec 00	B. Yang		Add Lun expansion error codes
//	26 Feb 01	H. Weiner	Add Hi5 Log LUN error code
//  12 Apr 01   K. Keisling	Defect 60336 - Add MMAN_ERROR_NDU_NO_WCE
//	14 May 01	H. Weiner	Add Hi5 Log LUN error code MMAN_ERROR_NEED_COMMIT_FOR_BIND
//	23 May 01	H. Weiner	Add PDH and REGISTRY error codes
//	 5 Jun 01	H. Weiner	Add MMAN_ERROR_NO_HI5 to indicate Hi5 is not supported
//	29 Nov 01	C. Hopkins	Added MMAN_ERROR_LU_EXPANSION_PRIVATE_LU
//	24 Jun 02	B. Yang		Added MMAN_ERROR_BASE_ENTRY_NOT_FOUND_IN_TOC
//  05 Jul 02   K. Keisling DIMS 71764 - Added MMAN_ERROR_LUN_IN_USE_NO_UNBIND	
//	27 Sep 02	H. Weiner	DIMS 77892 - Added MMAN_SET_TIME, so it can be logged
//  03 Oct 02   B. Yang     Add MMAN_ERROR_LUN_INVALID_DEFAULT_OWNER_SPECIFIED
//  29 May 03   G. Peterson DIMS 86750 - Added MMAN_ERROR_ATTEMPT_TO_SET_PRIVATE_LUN_ATTRIBUTE
//  19 Jun 03   G. Peterson DIMS 86750 - changed MMAN_ERROR_ATTEMPT_TO_SET_PRIVATE_LUN_ATTRIBUTE to Informational Level
//  18 Jul 03   L. Glanville Dims 89289 - Added MMAN_ERROR_NDU_LIMITED_CONFIG
//  08 oct 03   A. pattanaik DIMS 93498 - Modified MMAN_ERROR_NEED_COMMIT_FOR_FLARE_BIND to MMAN_ERROR_NEED_COMMIT_FOR_FLARE_CONFIG_CHANGE
//	18 Feb 04	H. Weiner	DIMS 101010 - Added MMAN_ERROR_TRESPASS_FAILED, MMAN_ERROR_TRESPASS_ILLEGAL
//	 1 Mar 04	H. Weiner	DIMS 101357 - Added MMAN_ERROR_IP_IN_USE
//	 30 Mar 04  L. Glanville Added trace file definitions
//	19 Dec 04	M. Brundage	Added MMAN_ERROR_SYSTEM_EXCEPTION
//	13 Jul 05	R. Hicks	Added MMAN_ERROR_BAD_WWN
//	22 Sep 05	R. Hicks	Added MMAN_ERROR_CONVERSION_IN_PROGRESS
//	10 Jan 06	R. Hicks	Added MMAN_ERROR_MAPFILE_CORRUPT
//	24 Jan 06	K. Boland	DIMS 137351 - Changed MMAN_ERROR_BAD_UUID to MMAN_WARNING_BAD_UUID
//	12 Sep 06	D. Hesi		Added MMAN_ERROR_DB_DISKS_UPDATING
//	08 Mar 07	V. Chotaliya	Added MMAN_ERROR_HOT_SPARE_IN_USE
//	29 Oct 07	R. Hicks	Added MMAN_ERROR_LUN_IN_INTERNAL_RAID_GROUP and MMAN_ERROR_RAID_GROUP_IS_INTERNAL
//	10 Jul 08	H.Rajput	Added MMAN_ERROR_LUN_NOT_FOUND
//	08 Aug 08	R. Hicks	DIMS 204304: Added MMAN_ERROR_AT_MAX_PUBLIC_LUS
//	06 Nov 08	R. Hicks	DIMS 212481 - Added MManSendRcv::Write() method which takes a timeout parameter.
//	15 Dec 08	R. Saravanan	Disk Spin Down feature: New error code added MMAN_ERROR_RAID_GROUP_HAS_LAYERED_DRIVER
//	23 Jan 09	R. Saravanan	DIMS 217447: Added new error code MMAN_ERR0R_LUN_IN_POWERSAVING_ENABLED_RAID_GROUP
//	06 July 09	R. Saravanan	DIMS 228751 and DIMS 230312 : Added new error code MMAN_ERROR_NDU_LIMITED_RG_CONFIG 
//								and MMAN_ERROR_AT_MAX_FLARE_LUS
//	30 Oct 06	D. Hesi		DIMS 239950: Added MMAN_ERROR_DUPLICATE_HASH_KEY for iSCSI Multipath IO support
//
// Disable pestiferous warnings about templated methods with
// no DLL-interface.
#ifdef _MSC_VER
#endif	//_MSC_VER

#ifndef MMAN_H
#define MMAN_H

//----------------------------------------------------------------------INCLUDES

#include "EmcPAL.h"
#include "k10defs.h"
#include "wtypes.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//-----------------------------------------------------------------------DEFINES

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT // fix compile prob for Tom
#endif

#define  _WIN32_WINNT					0x0403	// for QueueUserAPC

#define MMAN_MAX_KERNEL_DATA_SIZE		0x40000	// else CMI blows up
#define MMAN_3RD_PTY_CURRENT_VERSION	0x01	

#define MMAN_ITEM_SPEC_AS_POINTER_FLAG	0x01	// pass ptr to data section of
												// param block as item spec

#define MMAN_INTERFACE_TYPE				0x01	// Interface type so Navi knows how
												// to talk to us.

#define MMAN_PEER_DEAD_EVENT_NAME		"K10_MMAN_PEERDEAD_EVENT"
#define MMAN_ERROR_BASE			0x40000000
#define MMAN_ERROR_INFO_BASE	(MMAN_ERROR_BASE | K10_COND_INFO)
#define MMAN_ERROR_WARN_BASE	(MMAN_ERROR_BASE | K10_COND_WARNING)
#define MMAN_ERROR_ERROR_BASE	(MMAN_ERROR_BASE | K10_COND_ERROR)

#define MESSAGE_DISPATCHER_ACK_PEER_DELAY 250 //  Time (in ms) to allow message dispatcher to run.

//-------------------------------------------------------------------------ENUMS

enum MMAN_ERROR {					// MMan - specific error codes
	MMAN_ERROR_SUCCESS = 0x00000000L,

	MMAN_INFO_MIN = MMAN_ERROR_INFO_BASE,
	MMAN_ERROR_TRAP_LOGIC_NOERR = MMAN_INFO_MIN,	// A MP for Flare was handled entirely in pre-
	MSGDISP_INFO_ACTION,			// The MessageDispatcher is entering/exiting
	MMAN_SET_TIME,					// NT time has been set
	MMAN_INFO_ATTEMPT_TO_SET_PRIVATE_LUN_ATTRIBUTE,	// The User attempted to set a meaningless attribute for a private lun
	MMAN_INFO_MAX = MMAN_INFO_ATTEMPT_TO_SET_PRIVATE_LUN_ATTRIBUTE,

	MMAN_WARNING_MIN = MMAN_ERROR_WARN_BASE,
	MSGDISP_WARNING_DISCONNECT = MMAN_WARNING_MIN, // The Dispatcher got a shutdown msg.
	MSGDISP_WARNING,					// The MessageDispatcher detected an warning, see text
	MMAN_WARNING_PDHOPEN,				// PdhOpenQuery() failed
	MMAN_WARNING_PDHADD,				// PdhAddCounter() failed
	MMAN_WARNING_PDHCOLLECT,			// PdhCollectQueryData() failed
	MMAN_WARNING_PDHGETFORMAT,		// PdhGetFormattedCounterValue() failed
	MMAN_WARNING_PDH_CLOSE,			// Pdh CloseQuery() failed
	MMAN_WARNING_REGISTRY,			// Failure accessing the NT Registry
	MMAN_WARNING_BAD_UUID,			// Illegal UUID value in command -- moved from MMAN_ERROR_BAD_UUID
	MMAN_WARNING_MAX = MMAN_WARNING_BAD_UUID,

	MMAN_ERROR_ERROR_MIN = MMAN_ERROR_ERROR_BASE,
	MMAN_ERROR_BAD_SP_CODE = MMAN_ERROR_ERROR_MIN,	// SP is not A or B
	MMAN_ERROR_BAD_ADDR,			// Address is NULL/too long
	MMAN_ERROR_BAD_HEADER,			// Header malformed
	MMAN_ERROR_BAD_LAYER,			// not 'U' or 'K'
	MMAN_ERROR_NO_HEADER,			// No header where header required
	MMAN_ERROR_TIMEOUT,				// Timed out on IO
	MMAN_ERROR_IO,					// OS says good read/write, but bytes != # requested
	MMAN_ERROR_BAD_ACK,				// Malformed Ack struct
	MMAN_ERROR_BAD_DATA,			// data wrong size or did not checksum
	MMAN_ERROR_MEMORY,				// ran out of memory
	MMAN_ERROR_SECURITY,			// could not get security from client
	MMAN_ERROR_LARGE_KERNEL_MSG,	// data too large to send to kernel proc
	MMAN_UNKNOWN_MODEPAGE,			// not a known modepage to dispatch
	MMAN_UNKNOWN_CDB,				// don't know this CDB code in Redirector
	MMAN_INSUFFICIENT_BUFFER,		// Redirector client allowed too little buffer, or IOCTL buffer too small
	MMAN_ERROR_CONFIG,				// configuration is not complete on the machine
	MMAN_ERROR_UNKNOWN_EXCEPTION,	// int3, int5, etc. #8010
	MMAN_ERROR_UNKNOWN_SP,			// MPS got message from SP not in our array
	MMAN_ERROR_INBUF_HDR_BAD,		// Data size in MManInBufHdr bad
	MMAN_ERROR_PEER_DIED,			// well, whaddaya think...?
	MMAN_ERROR_UNKNOWN_DBID,		// Unknown AAQ/AAS database ID
	MMAN_ERROR_FMODE_NOT_SUPPORTED,	// do not support SC VLUs in bind, unbind
	MMAN_ERROR_RA_NOT_INITIALIZED,	// PSM LU not initialized
	MMAN_ERROR_PORT_OUT_OF_RANGE,	// This port value out of range for WWN creation
	MMAN_ERROR_LU_EXP_UNSUPPORTED,	// You cannot expand an LU in the K10 array
	MMAN_ERROR_BAD_OBJECT_NAME,		// Object name illegal or unexpected
	MMAN_ERROR_DLSFAIL,				// DLS service failed
	MMAN_ERROR_LAYER_ORDER,			// Attempt to insert driver in wrong place on stack
	MMAN_ERROR_MAPFILE,				// Could not create and/or access device map file
	MMAN_ERROR_MAPFILE_TOO_SMALL,	// Memory mapped file too small to hold data
	MMAN_ERROR_MAPLOCK,				// Error using device map file lock/mutex
	MMAN_ERROR_NEEDS_LOCK,			// Operation needs lock, but don't own it.
	MMAN_ERROR_PSM_CLOSE,			// Error closing PSM file  #8020
	MMAN_ERROR_WAIT,				// Error in WaitForSingleObject/WaitForMultipleObjects
	MMAN_ERROR_DRIVERS_INACCESSIBLE,// One or more of the required services is not started
	MMAN_ERROR_CANNOT_DEREF_DVR,	// Cannot close a driver handle
	MMAN_ERROR_INVALID_FAIRNESS_VALUE,// fairness value greater than that supported by system
	MMAN_ERROR_IF_VERSION_UNSUPPORTED,// unsupported version of an internal structure (incoherent install)
	MMAN_ERROR_NO_LU_INDIR,			// LU Indirection is not supported in Page 37
	MMAN_ERROR_UNSUPPORTED_HOSTOPTIONS_FLAGS, // cannot change MP8 Or RCE bits in hostoptions
	MMAN_ERROR_NO_AUTOTRESPASS,		// You may not change autotrespass via page37 Genral Feat, Param8
	MMAN_ERROR_TRESPASS_UNSUPPORTED,// You may not use VLU-based trespass with this API
	MMAN_ERROR_NOT_ASYNC,			// IO is pending, but not set for Async IO!
	MMAN_ERROR_LUN_INCONSISTENT,	// Multiple methods of determining the specified LUN
									// Yield different results
	MMAN_ERROR_LUN_COUNT,			// Lun count was not the expected value
	MMAN_ERROR_PSM_BAD,				// PSM incorrectly configured
	MMAN_ERROR_CANT_REMOVE_PARAM,	// unable to remove Page 37 parameter.
	MMAN_ERROR_ILLEGAL_DEFAULT,		// Attempt to set unsupported default in Page 37
	MMAN_ERROR_INVALID_INPUT,		// An input argument was invalid #8030
	MMAN_ERROR_DEGRADED_MODE,		// Array has been booted in degraded mode
	MMAN_ERROR_PSM_NO_WCE,			// Not allowed to set write cache on PSM LU
	MMAN_ERROR_TOO_MANY_CXN,		// Number requested connections exceeded the max.
	MMAN_ERROR_NOT_NULL,			// An expected NULL pointer wasn't
	MMAN_ERROR_NULL_PTR,			// An pointer was unexpectedly NULL
	MMAN_ERROR_INVALID_HANDLE,		// Got invalid handle but no NT error.
	MMAN_ERROR_PSM_LU_BROKEN,		// PSM LU does not function - double fault??	
	MMAN_ERROR_NO_TPASS_PRIVATE,	// Trespass of private LUs not allowed
	MMAN_ERROR_IOCTL,				// Other DeviceIoControl Error, NOT a timeout.  
	MMAN_ERROR_BAD_SSN,				// Bad System serial number
	MMAN_ERROR_PROG_ERROR,			// Programmer error - should NEVER see this in field
	MMAN_ERROR_EXECUTION,			// Generic execution error 
	MMAN_ERROR_LU_EXPANSION_ONLY_ONE_LUN_ALLOWED,	// Only one lun allowd for a lun expansion
	MMAN_ERROR_LU_EXPANSION_FOUND_LAYER,	// Lun expansion doesn't allow layer
	MMAN_ERROR_LU_EXPANSION_LUN_NOT_IN_DEVMAP,	// Lun is not in the devicemap
	MMAN_ERROR_NDU_NO_WCE,			// Can't enable Write Cache while NDU in-progress
	MMAN_ERROR_MPAGE_INTERFERENCE,	// Keep getting wrong mode page data ===> someone else asking
	MMAN_ERROR_HI5_LOG_LUN_MISSING,	// Hi5 RAID group has no Log LUN
	MMAN_ERROR_MULTIPLE_HI5_LOG_LUNS,	// Should only be 1 per Hi5 RAID group
	MMAN_ERROR_NEED_COMMIT_FOR_FLARE_CONFIG_CHANGE,	// Must perform an NDU commit of Base before binding
	MMAN_ERROR_NO_HI5,				// Hi5 LUNs are not currently supported.
	MMAN_ERROR_LU_EXPANSION_PRIVATE_LU,  // Don't expand private LUs
    MMAN_ERROR_OBJECT_ALREADY_OPEN,         // Trying to open something that is already open
    MMAN_ERROR_FILE_GENERIC_ERROR,          // file open, close, read, write, etc error
    MMAN_ERROR_FILE_VERSION_ERROR,          // file is wrong version
    MMAN_ERROR_IOCTL_TIMEOUT,               // Timeout of an internal IOCTL
    MMAN_ERROR_TU_MISSING_INFO,             // error in TldUtils, missing info to proc. cmd 
    MMAN_ERROR_TU_MALFORMED_TLD_TREE,       // error in TldUtils, bad TLD tree
	MMAN_ERROR_COM_OBJ_CREATE,			// Exception thrown from CoCreateInstance() call
	MMAN_ERROR_BAD_UUID_DEPRECATED,		// moved to MMAN_WARNING_BAD_UUID, but keep slot
	MMAN_ERROR_BASE_ENTRY_NOT_FOUND_IN_TOC, // Can not get Base entry from Toc file
	MMAN_ERROR_LUN_IN_USE_NO_UNBIND,     // Cannot unbind LUN, it's consumed by a layered driver
    MMAN_ERROR_LUN_INVALID_DEFAULT_OWNER_SPECIFIED,  // if x1lite is in non-HA mode, 
                                                     // the ownership could only be set to the local SP.
    MMAN_ERROR_GET_FEATURE_FAILURE,      // Can not get feature data from ldAdmin
	MMAN_ERROR_UNBIND_LUN_PRIVATE,		 // Unbind failed - already deassigned, so probably private now
    MMAN_ERROR_NDU_LIMITED_CONFIG,       // Ndu in progress, only limited configuration changes are allowed. No binds or expands 
	MMAN_ERROR_MISSING_CONSTRAINT_BRANCH,	// Constraint branch expected, but missing
	MMAN_ERROR_MISSING_CONSTRAINT_DATA,		// Constraint branch is empty
	MMAN_ERROR_MISSING_CONSTRAINT_ID,		// Constraint branch missing ID tag
	MMAN_ERROR_MISSING_CONSTRAINT_KEY,		// Constraint branch missing TAG_KEY
	MMAN_ERROR_TRESPASS_FAILED,				// Trespass command rejected by hostside
	MMAN_ERROR_TRESPASS_ILLEGAL,			// Cannot trespass this LU
	MMAN_ERROR_IP_IN_USE,					// IP is already being used
	MMAN_ERROR_CANNOT_MODIFY_DEF_SG,	// Cannot modify the default storage group
	MMAN_ERROR_SYSTEM_EXCEPTION,			// Caught hardware generated exception (access violation, etc.)
	MMAN_ERROR_BAD_WWN,						// Illegal WWN value in command
	MMAN_ERROR_WWN_IN_USE,					// WWN is already being used
	MMAN_ERROR_CONVERSION_IN_PROGRESS,		//Array conversion in progress, can't bind or create RG
	MMAN_ERROR_MAPFILE_CORRUPT,				// Device map corruption present, configuration changes which modify device map are disallowed
	MMAN_ERROR_DB_DISKS_UPDATING,			// Cannot bind or unbind the LUN while the flare DB disks are rebuilding
	MMAN_ERROR_HOT_SPARE_IN_USE,			// Cannot unbind the LUN while Hot Spare is in use
	MMAN_ERROR_LUN_IN_INTERNAL_RAID_GROUP,	// Cannot directly bind, modify, or unbind LUN in an internal RAID Group
	MMAN_ERROR_RAID_GROUP_IS_INTERNAL,		// Cannot directly modify or delete internal RAID Group
	MMAN_ERROR_AT_MAX_PUBLIC_LUS,			// Maximum number of public LUs have already been created
	MMAN_ERROR_NDU_LIMITED_RG_CONFIG,		// Software upgrade in progress Unable to defrag the RAID group 
	MMAN_ERROR_AT_MAX_FLARE_LUS,			// The maximum number for LUs has been created.
	MMAN_ERROR_DUPLICATE_HASH_KEY,			// Error to add duplicate Hash table entry
	MMAN_ERROR_ERROR_MAX = MMAN_ERROR_DUPLICATE_HASH_KEY,

	MSGDISP_ERROR_ERROR_MIN = MMAN_ERROR_ERROR_BASE + 0x100,
	MSGDISP_ERROR_INVALID_IO_STATE = MSGDISP_ERROR_ERROR_MIN,	// IO State is invalid
	MSGDISP_ERROR_INVALID_PIPE_STATE,	// Pipe handle state is invalid
	MSGDISP_ERROR_INCONSISTENT_PIPE_STATE,	// Pipe handle state inconsistent with other
									// state data, such as the IO state.
	MSGDISP_ERROR_INVALID_STATE,	// Proc/thread state had illegal value
	MSGDISP_ERROR_INCONSISTENT_STATE,	// State is inconsistent with what just occurred.
	MSGDISP_ERROR_BUFFER_FULL,		// A required buffer was full
	MSGDISP_ERROR_SHORT_PIPE_READ,	// A pipe read got less data then expected, or no data
	MSGDISP_ERROR,					// The MessageDispatcher detected an error, see text
	MSGDISP_ERROR_INIT,		// The MessageDispatcher detected an error during initialization, see text
	MMAN_ATTEMPT_TO_SET_PRIVATE_LUN_ATTRIBUTE,		//The user attempted to set an attribute for a private lun that isn't settable 
	MMAN_ERROR_LUN_NOT_FOUND,		//The specified LUN was not found.
	MMAN_ERROR_RAID_GROUP_HAS_LAYERED_DRIVER,	// The Raid Group has layered driver in it
	MMAN_ERR0R_LUN_IN_POWERSAVING_ENABLED_RAID_GROUP,	// The LUN is in PowerSaving enabled RAID Group
	MMAN_ERROR_END = MMAN_ERR0R_LUN_IN_POWERSAVING_ENABLED_RAID_GROUP,
	MSGDISP_ERROR_ERROR_MAX = MMAN_ERROR_LUN_NOT_FOUND,

	MMAN_ERROR_SM_NO_CONNECTION,		// Cannot connect to System Management Daemon
	MMAN_ERROR_SM_REQUEST_TIMEOUT,		// System Management request timeout has expired
	MMAN_ERROR_SM_CLIENT_IN_SHUTDOWN,	// System Management client is in shutdown
	MMAN_ERROR_SM_REMOTE_SIDE,		// System Management cannot execute the request
	MMAN_ERROR_SM_LOCAL_SIDE,		// Cannot transmit request to System Management because of local side error
	MMAN_ERROR_SM_INTERNAL_LOGIC,		// System Management internal logic malfunctions
	MMAN_ERROR_SM_UNKNOWN 			// System Management unknown error
};

enum MMAN_IO_TYPE {					// used in async ops to keep track of type of outstanding IO
	MMAN_IO_TYPE_MIN = 0,
	MMAN_IO_TYPE_NONE = MMAN_IO_TYPE_MIN,	// no IOs outstanding
	MMAN_IO_TYPE_READ,				// waiting to write
	MMAN_IO_TYPE_WRITE,				// waiting to read
	MMAN_IO_TYPE_READ_ACK,			// waiting to get back an ack
	MMAN_IO_TYPE_WRITE_ACK,			// waiting on write of ack
	MMAN_IO_TYPE_CONNECT,			// waiting to connect as a server
	MMAN_IO_TYPE_MAX = MMAN_IO_TYPE_CONNECT	
};

enum MMAN_IO_STATUS {				// used in async ops to keep track of status of outstanding IO
	MMAN_IO_STATUS_MIN = 0,
	MMAN_IO_STATUS_NONE = MMAN_IO_STATUS_MIN,	// no IOs out
	MMAN_IO_STATUS_PENDING,			// IO in progress
	MMAN_IO_STATUS_COMPLETE,		// IO completed, check result
	MMAN_IO_STATUS_MAX = MMAN_IO_STATUS_COMPLETE
};


enum K10_CDB_TYPE {
	K10_CDB_TYPE_MIN = 0,
	K10_CDB_TYPE_NONE = K10_CDB_TYPE_MIN,	// uninitialized state
	K10_CDB_TYPE_MODE_SENSE,
	K10_CDB_TYPE_MODE_SELECT,
	K10_CDB_TYPE_AAQ,
	K10_CDB_TYPE_AAS,
	K10_CDB_TYPE_STD_ADMIN_READ,
	K10_CDB_TYPE_STD_ADMIN_WRITE,
	K10_CDB_TYPE_MAX = K10_CDB_TYPE_STD_ADMIN_WRITE
};


enum OPERATION_TYPE {
	OPERATION_TYPE_MIN = 0,
	OPERATION_TYPE_NONE = OPERATION_TYPE_MIN,	// uninitialized state
	OPERATION_TYPE_GET,
	OPERATION_TYPE_SET,
	OPERATION_TYPE_CREATE,
	OPERATION_TYPE_DELETE,
	OPERATION_TYPE_STD_ADMIN_READ,				// needed to separate Std Admin
	OPERATION_TYPE_STD_ADMIN_WRITE,				// OPs from the rest of what we're doing
	OPERATION_TYPE_MAX = OPERATION_TYPE_STD_ADMIN_WRITE
};

#ifdef C4_INTEGRATED
//
// Enum to provide indication of the type of enum luns and raid groups
// requested.  SSPG has the need to enum the SSPG Private Luns (Luns 0-257)
// which will include luns bound on private Raid Groups (Raid groups < 479)
// So this enum will indicate what type of enumeration is being requested
//
enum ENUM_OPERATION_TYPE {
    ENUM_TYPE_MIN = 0,
    ENUM_TYPE_USER_ONLY = ENUM_TYPE_MIN, // 0
    ENUM_ALL,                            // 1
    ENUM_TYPE_NEO_PRIVATE,               // 2
    ENUM_TYPE_SPECIFIED_OBJECT,          // 3
    ENUM_MAX = ENUM_TYPE_SPECIFIED_OBJECT
};

//
//  In Neo we declared any lun less then 256 to be a Neo Private LUN
//
#define  NEO_PRIVATE_LUNS_LIMIT     256   

#endif /* C4_INTEGRATED - C4ARCH - admin_topology */

//-------------------------------------------------------------------TYPEDEFS

// defs for the functionSpec property of MM_3PtyParamBlock
// and the dispatch classes in the Redirector
//
#define MMAN_3RD_PTY_NONE			0x0	// value not set yet

#define MMAN_3RD_PTY_READ			0x1
#define MMAN_3RD_PTY_WRITE			0x2
#define MMAN_3RD_PTY_UPDATE			0x3

#define MMAN_3RD_PTY_MODE_SENSE		0x4
#define MMAN_3RD_PTY_MODE_SELECT	0x5

#define MMAN_3RD_PTY_AAS			0x6
#define MMAN_3RD_PTY_AAQ			0x7

#define MMAN_3RD_PTY_ID_LEN

// The param block is used with the general administrative interface,
// where a WriteBuffer does, and a ReadBuffer may, have a data section 
// after the CDB. It holds the data necessary for the Redirector to find 
// the recipient.

typedef struct _MM_3PtyParamBlock {
	unsigned short	version;
	unsigned char	functionSpec;	// can't enum a short, see defines above
	unsigned char	bFlags;			// MMAN_ITEM_SPEC_AS_POINTER_FLAG
	unsigned long	bufSize;		// size of this  AND following data, if any
	char			libraryId[MMAN_LIBRARY_NAME_LEN];
	unsigned short	dbId;
	unsigned short	opCode;
	unsigned long	itemSpec;
} MM_3PtyParamBlock;

// The Inbuf Header is used when doing a ReadBuffer (via general Admin interface),
// and the user desires a pointer to data to be passed as the ItemSpecifier.
// This data must follow the MM_3PtyParamBlock in the ReadBuffer's data area
//
typedef struct _MM_InbufHdr {
	unsigned long	dwDataSize;			// size of this header and data to follow		
	unsigned char	pData[1];			// stub beginning of data (variable size)
} MM_InbufHdr;

#ifdef ALAMOSA_WINDOWS_ENV	// MM_K10LibTrace related definitions, needed for LockWatch clients

#define MAX_LIB_TRACE_ENTRIES 8
#define MAX_LIB_TRACE_OP_LENGTH 10
#define MAX_LIB_TRACE_ARG_LENGTH 30
struct CSX_MOD_EXPORT MM_K10TraceEntry
{
    char AdminLibraryName[MMAN_LIBRARY_NAME_LEN];
    char OpString[MAX_LIB_TRACE_OP_LENGTH];
    char ArgString[MAX_LIB_TRACE_ARG_LENGTH];
    EMCPAL_LARGE_INTEGER Reserved;
};

struct CSX_MOD_EXPORT MM_K10TraceFile
{
    char ProcessName[EMCUTIL_MAX_PATH];
    ULONG EntryCount;
    MM_K10TraceEntry Entry[MAX_LIB_TRACE_ENTRIES];
};

class CSX_CLASS_EXPORT MM_K10LibTrace 
{
public:

	//	Constructors and Destructor
	MM_K10LibTrace();
	~MM_K10LibTrace();

    // Access Functions
    void ClearFile();
    void WriteHeader();
    void WriteEntry(MM_K10TraceEntry *pEntry);
    MM_K10TraceFile *GetFile();
    void ReleaseFile();

private:
    MM_K10TraceFile * OpenFile(bool isRead);
    void CloseFile();
    void GetProcessName(char *szProcessName, int mNameLength);
    HANDLE traceMtx;
    HANDLE hTraceFile;
	MM_K10TraceFile *pMapView;
};
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - admin differences - for K10LibTrace */

//--------------------------------------------------------------FWD DECLARATIONS

class MManSenderImpl;
class MManReceiverImpl;
class MManSendRcvImpl;


//---------------------------------------------------------------------MManSender
//
//	Name: MManSender
//
//	Description: 
//		Interface class for simple message sending: can send 1 msg at a time, 
//		synchronously.
//
class CSX_CLASS_EXPORT MManSender {

public:

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	MManSender()
	//
	//	Description:
	//		Constructor
	//
	//	Arguments:
	//		pMyAddr		IN	MsgMan address string for this endpoint.
	//
	//	Returns:
	//		N/A
	//
	//	Preconditions: pMyAddr is not null, is null-term and < MMAN_ADDR_NAME_LEN
	//
	MManSender(	char *pMyAddr ); 

		//-------------------------Public Method-----------------------------------//
	//
	//	Name:	~MManSender()
	//
	//	Description:
	//		Destructor
	//
	//	Arguments: N/A
	//
	//	Returns:
	//		N/A
	~MManSender(){};

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	SendMessage()
	//
	//	Description:
	//		Sends the supplied message to the designated recipient.
	//
	//	Arguments:
	//		pRcvrAddr	IN	MsgMgr address (string) used to identify the connection
	//		rcvrSpId	IN	Identifier of SP of other endpoint
	//		pData		IN	Pointer to data to send
	//		dwDataSize	IN	Size, in bytes, of data to send
	//
	//	Returns:
	//		MMAN_ERROR_SUCCESS, (System Errors, MMAN_ERROR_XXX)
	// 
	long SendAMessage( char *pRcvrAddr, char rcvrSpId, unsigned char *pData, 
							unsigned long dwDataSize);

private:

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	MManSender()
	//
	//	Description:
	//		Constructor
	//
	//	Arguments: None
	//
	//	Returns:
	//		N/A
	//
	//	Note: Private to keep from instantiation without needed state
	//
	MManSender(){};

	MManSenderImpl *mpImpl;		// Pointer to implementing class
};


//------------------------------------------------------------------MManReceiver
//
//	Name: MManReceiver
//
//	Description: 
//		Interface class for simple message receipt: can receive 1 message per
//		connection, synchronously.
// Note: 
//		Maximum of 100 concurrent instances of a receiver where the pMyAddr 
//		value is the same
//
class CSX_CLASS_EXPORT MManReceiver {

public:
	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	MManReceiver()
	//
	//	Description:
	//		Constructor
	//
	//	Arguments:
	//		pMyAddr			IN	MsgMan address of this connection
	//		hNotifyEvent	IN	Handle to event to signal when a connection is
	//							made. Can be NULL.
	//		dwBufSize		IN	Size of default buffer (if packets are larger,
	//							extra steps are needed to reallocate).
	//
	//	Returns:
	//		N/A
	//
	//	Preconditions: pMyAddr is not null, is null-term and < MMAN_ADDR_NAME_LEN
	//
	MManReceiver( char *pMyAddr, void * hNotifyEvent, unsigned long dwBufSize );

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	~MManReceiver()
	//
	//	Description:
	//		Destructor
	//
	//	Arguments: N/A
	//
	//	Returns:
	//		N/A
	~MManReceiver(){};

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	ConnectAndRead()
	//
	//	Description:
	//		Connects to a sender and reads a single message.
	//
	//	Arguments:
	//		pData		OUT	Address of pointer to hold message data
	//		pBytesRead	OUT	Address of unsigned long to receive size of message data in bytes
	//		pSenderAddr	OUT	Pointer to buffer to receive MsgMgr address of Sender
	//		senderSpId	OUT	Address of char to receive SP ID of Sender
	//
	//	Returns:
	//		MMAN_ERROR_SUCCESS, (System Errors, MMAN_ERROR_XXX)
	// 
	long ConnectAndRead( unsigned char **ppData, DWORD *pBytesRead, char *pSenderAddr,
							char *senderSpId );

private:

	//-------------------------Public Method-----------------------------------//
	//
	//	Name:	MManReceiver()
	//
	//	Description:
	//		Constructor
	//
	//	Arguments: None
	//
	//	Returns:
	//		N/A
	//
	//	Note: Private to keep from instantiation without needed state
	//
	MManReceiver(){};

	MManReceiverImpl *mpImpl;			// Pointer to implementing class

};

//--------------------------------------------------------------------MManSendRcv
//
//	Name: MManSendRcv
//
//	Description: 
//		Can send or receive, read and write, synchronously or asyncronously.
//		Explicit connect/disconnect semantics for multi-message transactions.
//
//
class CSX_CLASS_EXPORT MManSendRcv {

public:

//-------------------------Public Method-----------------------------------//
//
//	Name:
//		MManSendRcv()
//
//	Description:
//		Constructor
//
//	Arguments:
//		pMyAddr		IN	my MsgMan address
// 		dwBufSize	IN	size of standard buffer.
//		bModeDirect IN  bypass MD to do direct TCP/IP to other SP
//
//	Returns:
//		N/A
//
// State at exit: connect state uninited, handle closed
//
	MManSendRcv( char *pMyAddr, unsigned long dwBufSize, bool bModeDirect = false ); 


//-------------------------Public Method-----------------------------------//
//
//	Name:
//		~MManSendRcv()
//
//	Description:
//		Destructor
//
//	Arguments:
//		N/A
//
//	Returns:
//		N/A
//
// State at exit: connect state uninited, handle closed
//
	~MManSendRcv();

//-------------------------Public Method-----------------------------------//
//
//	Name:
//		CloseConnection()
//
//	Description:
// Sender or server, close current connection, close handle
// if server, will check if connected & do a flush (if
// indicated) and a Disconnect first.
//
//	Arguments:
//		bFlush	IN	do flush before disconnect
//
//	Returns:
//		N/A
//
// State at exit: connect state uninited, handle closed
//
	void CloseConnection(bool bFlush);

//-------------------------Public Method-----------------------------------//
//
//	Name:
//		CreateConnect
//
//	Description:
//		Create a new channel to which other processs can connect.
//		This is for operating in 'listen' or server mode.
//
//	Arguments:
//		bIsAsync	IN	make asynchronous channel
//
//	Returns:
//		S_OK, ERROR_IO_PENDING, (System errors)
//
// State at exit: channel inited,  unconnected, connector state uninited
// State at failure: connect state uninited, handle closed
//
	long CreateConnect( bool bIsAsync );

//-------------------------Public Method-----------------------------------//
//
//	Name:
//		Disconnect
//
//	Description:
// As a server, disconnect from current client, do NOT close channel
// bFlush - wait until client has read data in buffers (THIS WILL BLOCK!)
// Safe to use if already disconnected.
// NOTE: This will ASSERT if you are not a server instance
//
//	Arguments:
//		bFlush	IN	do flush before disconnect
//
//	Returns:
//		S_OK, (System errors only)
//
//	Preconditions: Object is in Server mode (called CreateConnect to make last connection)
//
// State at exit: channel preserved, disconnected, connector state uninited
// State at failure: channel may still be inited, handle may be open.
// 
	long Disconnect( bool bFlush );

//-------------------------Public Method-----------------------------------//
//
//	Name:
//		ReConnect
//
//	Description:
// Following Create/disconnect, allow connect on channel
//
//	Arguments: None
//
//	Returns:
//		S_OK, ERROR_IO_PENDING, (System errors)
//
//	Preconditions: Object is in Server mode (called CreateConnect to make last connection)
//
// State at exit: channel preserved,  connected, connector state uninited
// State at failure: channel may still be connecte to previous client
//
	long ReConnect( );

//-------------------------Public Method-----------------------------------//
//
//	Name:
//		WaitConnect
//
//	Description:
//		As sender, wait for avail. connection to receiver.
//		This can be used to initiate commo to local User component,
//		Peer User component, or local Kernel component.
//
//	Arguments:
// pRcvrAddr	IN	receiver's MsgMan address
// rcvrSpId		IN	receiver's SP: A/B
// bIsAsync		IN	true==make the connection and ALL reads/writes asynchronous
//
//	Returns:
//		S_OK, (System Errors, MMAN_ERRORs)
//
// State at exit: channel inited,  connected, connector state inited
// State at failure: channel uninited, handle closed.
//
	long WaitConnect( char *pRcvrAddr, char rcvrSpId, 
						 bool bIsAsync);

//-------------------------Public Method-----------------------------------//
//
//	Name:
//		Read
//
//	Description:
// Read from the client. Always is Message mode
// As we may be doing async IO, this function controls allocation to buffer.
// Callng process may NOT write to/allocate to this address until async result
// returned. Similarly, unsigned long pointers supplied must be left undisturbed and
// their scope MUST persist through to completion of async routine or errors
// will result.
// GlobalFree() MUST be used to free data allocated to the buffer.
//
//
//	Arguments:
// ppData	OUT		Address of pointer which will be GlobalAlloc'ed the data
//					In async mode this MAY happen when CheckIOStatus called
// pBytesRead	OUT		Address of unsigned long to receive # bytes read
//
// 	Preconditions: 
//		connection is open
//		no transfer in progress (no simultaneous async operations),
//		this does not ASSERT, but does PIPE_BUSY
// 		*ppData must be NULL
//		pBytesRead must not be NULL
//
//	Returns:
//		S_OK, ERROR_IO_PENDING, (System errors)
//
// State at exit: channel inited,  connected, connector state inited
// State at failure: channel inited,  connected, connector state inited
//
	long Read( unsigned char **ppData, unsigned long *pBytesRead );

//-------------------------Public Method-----------------------------------//
//
//	Name:
//		Write
//
//	Description:
// Write message to client.
// As we may be doing async IO, must insure scope of buffer passed into this
// function persists through end of async operation, else errors will result.
//
//
//	Arguments:
// pData		IN		Address of data to write
// dwDataSize	IN		Size of data to write, in bytes
// timeout		IN		Timeout (in seconds) for message delivery
//
// 	Preconditions: 
//		connection is open via WaitConnect, or following Read if Server instance
//		no transfer in progress (no simultaneous async operations)
// 		pData must not be NULL
//		dwDataSize must be > 0; if sent to kernel, must be < MMAN_MAX_KNL_MSG_SIZE
//
//	Returns:
//		S_OK, ERROR_IO_PENDING, (System errors)
//
// State at exit: channel inited,  connected, connector state inited
// State at failure: channel inited,  connected, connector state inited
//
	long Write( unsigned char *pData, unsigned long dwDataSize );
	long Write( unsigned char *pData, unsigned long dwDataSize, unsigned short timeout );

//-------------------------Public Method-----------------------------------//
//
//	Name:
//		CheckIOStatus
//
//	Description:
// Check to see if asynchronous IO is completed, or to check what outstanding
// async ops are pending.
// If it is completed, and it is a Read, then all data in current message is guaranteed
// to be in the ppData buffer passed to the Read function, and the data size and message
// ID are also in the buffers passed to the Read function.
//
//	Arguments:
// pIOType		IN		Address of MMAN_IO_TYPE enum to pass back current IO type
// pIOStatus	IN		Address of MMAN_IO_TYPE enum to pass back current IO status
// dwTimeout	IN		Time to wait for the overlapped object
//
// 	Preconditions: 
//		connection is open via WaitConnect, or following Read if Server instance
//		dwTimeout < MMAN_MAX_TIMEOUT
//
//	Returns:
//		MMAN_ERROR_SUCCESS, (MMAN_ERROR_TIMEOUT, MMAN_ERROR_IO, MMAN_ERROR_MEMORY)
//
// State at exit: if outstanding IO is completed, the async state is cleared.
//		Exception: if an IO is a write, the read of the ack may pend, so internal state will
//				reflect an outstanding async IO.
// State at failure: channel closed,  disconnected
//
MMAN_ERROR CheckIOStatus( MMAN_IO_TYPE *pIOType, MMAN_IO_STATUS *pIOStatus, 
									   unsigned long dwTimeout );

	// Check to see if this object is waiting for an acknowledgement.
	bool WaitingForAck();

private:

	MManSendRcv(){};			// Pointer to implementing clas
	MManSendRcvImpl *mpImpl;	// Pointer to implementing class

};

#endif
