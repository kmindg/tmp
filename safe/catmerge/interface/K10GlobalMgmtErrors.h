// K10GlobalMgmtErrors.h
//
// Copyright (C) 2000-2010 EMC
//
//
// Header for K10GlobalMgmt errors
//
//	Revision History
//	----------------
//	21 Jun 00	D. Zeryck	Initial version
//	26 Jun 00	D. Zeryck	Add errors for DGSSP
//	 1 Jul 00	D. Zeryck	Bug 1277
//	18 Aug 00	H. Weiner	Addd K10_GLOBALMGMT_INFO_DGSSP_BIOS
//	30 Aug 00	B. Yang		Added K10_GLOBALMGMT_ERROR_LU_SCRUBBED
//	05 Sep 00	B. Yang		Task 1774 - Cleanup the header files
//	02 Oct 00	B. Yang		Task 1955 - Add log and trace info for NduStatusReport.
//	16 Oct 00	D. Zeryck	9925 - K10_GLOBALMGMT_ERROR_MAX_NODES_EXCEEDED
//	 2 Mar 04	H. Weiner	101356 - error codes to support various config command mistakes
//	 9 Oct 04	C. J. Hughes    Device Map Robustness
//	14 Jun 05	R. Hicks	Device Map Scalability Enhancement
//	04 May 07	V. Chotaliya	Added errors for IPv6 Support on Management Port
//	29 Oct 07	R. Hicks	Add errors for Disk Pools
//	16 May 08	E. Carter	DIMS 196493: Add K10_GLOBALMGMT_WARN_MISMATCHED_LUN_IDS
//	24 Nov 08	E. Carter	Added error codes for netidparams errors.
//	21 Jul 09	R. Hicks	DIMS 233092: Add K10_GLOBALMGMT_ERROR_MAX_GATEWAYS_EXCEEDED
//	30 Apr 10	D. Hesi		ARS 361828: Add K10_GLOBALMGMT_ERROR_DISK_IN_PROBATION
//
#ifndef _K10GlobalMgmtErrors_h_
#define _K10GlobalMgmtErrors_h_

#ifndef K10_DEFS_H
#include "k10defs.h"	
#endif

//------------------------------------------------------------------------DEFINES

// Error code
#define K10_GLOBALMGMT_ERROR_BASE	0x76000000

#define K10_GLOBALMGMT_DGSSP_OFFSET	0x100
#define K10_GLOBALMGMT_IPV6_OFFSET  0x200
#define K10_GLOBALMGMT_DISK_POOL_OFFSET  0x300
#define GLOBALMANAGEMENT_STRING	"K10_GlobalManagement"
//-------------------------------------------------------------------------ENUMS

enum K10_GLOBALMGMT_ERROR {						// specific error codes
	K10_GLOBALMGMT_ERROR_SUCCESS = 0x00000000L,
	K10_GLOBALMGMT_INFO_MIN = (K10_GLOBALMGMT_ERROR_BASE |K10_COND_INFO),
	K10_GLOBALMGMT_INFO_LOCKOP= K10_GLOBALMGMT_INFO_MIN,
	K10_GLOBALMGMT_INFO_INHIBIT_IO,
    K10_GLOBALMGMT_INFO_DEVICE_MAP_NO_LONGER_CORRUPT,
        // Device map was corrupt, but it isn't any more
    K10_GLOBALMGMT_INFO_DEVICE_MAP_NO_REGEN,
	// Condition causing device map to be regenerated on every access has cleared
	K10_GLOBALMGMT_INFO_MAX = K10_GLOBALMGMT_INFO_DEVICE_MAP_NO_REGEN,

	K10_GLOBALMGMT_INFO_DGSSP_MIN = 
		(K10_GLOBALMGMT_ERROR_BASE |K10_COND_INFO | K10_GLOBALMGMT_DGSSP_OFFSET),
	K10_GLOBALMGMT_INFO_DGSSP_EXECUTION =  K10_GLOBALMGMT_INFO_DGSSP_MIN,
	K10_GLOBALMGMT_INFO_DGSSP_BIOS,				// Log the BIOS rev & date
	K10_GLOBALMGMT_INFO_DGSSP_PEER_BOOT,		// Peer boot success
	K10_GLOBALMGMT_INFO_DGSSP_POST_INFO,		// found POST informational message
	K10_GLOBALMGMT_INFO_DGSSP_BMC,              // Log BMC SEL
	K10_GLOBALMGMT_INFO_DGSSP_MAX = K10_GLOBALMGMT_INFO_DGSSP_BMC,

	K10_GLOBALMGMT_WARN_MIN = 
		(K10_GLOBALMGMT_ERROR_BASE |K10_COND_WARNING),
	K10_GLOBALMGMT_WARN_TRANSACTLOG_NO_DATA =  K10_GLOBALMGMT_WARN_MIN,
									// Empty transaction log.
	K10_GLOBALMGMT_WARN_LU_SCRUBBED,  //Scrubbed some or all XLUs out of hostside
	K10_GLOBALMGMT_WARN_SCRIPT_FAILURE,	// Script to change IP Port params appears to have failed
    K10_GLOBALMGMT_WARN_TRANSACTION_CLEARED, // Transaction file cleared
	K10_GLOBALMGMT_WARN_DEVICE_MAP_REGEN,		// Device map had to be regenerated due to error
    K10_GLOBALMGMT_WARN_MISMATCHED_LUN_IDS,		// Mismatched LUN IDs were discovered between IDM and hostside.
    K10_GLOBALMGMT_WARN_MAX = K10_GLOBALMGMT_WARN_MISMATCHED_LUN_IDS,

	K10_GLOBALMGMT_WARN_DGSSP_MIN = 
		(K10_GLOBALMGMT_ERROR_BASE |K10_COND_WARNING | K10_GLOBALMGMT_DGSSP_OFFSET),
	K10_GLOBALMGMT_WARN_DGSSP_EXECUTION = K10_GLOBALMGMT_WARN_DGSSP_MIN,
	K10_GLOBALMGMT_WARN_DGSSP_POST_WARN,		// found POST warning
	K10_GLOBALMGMT_WARN_DGSSP_MAX = K10_GLOBALMGMT_WARN_DGSSP_POST_WARN,
	
	
	K10_GLOBALMGMT_ERROR_ERROR_MIN = (K10_GLOBALMGMT_ERROR_BASE |K10_COND_ERROR),
	K10_GLOBALMGMT_ERROR_BAD_DBID = K10_GLOBALMGMT_ERROR_ERROR_MIN,	
									// Unrecog. database ID in CDB
	K10_GLOBALMGMT_ERROR_BAD_OPC,
	K10_GLOBALMGMT_ERROR_BAD_ISPEC,
	K10_GLOBALMGMT_ERROR_IOCTL_TIMEOUT,			// Timeout when waiting for DeviceIoControl
	K10_GLOBALMGMT_ERROR_BAD_DATA,				// wrong size for struct expected
	K10_GLOBALMGMT_ERROR_IOCTL,					// DeviceIoControl failed
	K10_GLOBALMGMT_ERROR_EVENT_CREATE_FAILURE,	// Failure in creating an event. 
	K10_GLOBALMGMT_ERROR_CLOSE_HANDLE,			// Failure in closing a handle. 
	K10_GLOBALMGMT_ERROR_CLOSE_REGKEY,			// Failure in closing a registry key
	K10_GLOBALMGMT_ERROR_OBJECT_NOT_INITIALIZED,	// Did not init an object before using - prog error
	K10_GLOBALMGMT_ERROR_EXECUTION,				// error executing subroutine- see data section
	K10_GLOBALMGMT_ERROR_PROG_ERROR,			// programmer goof
	K10_GLOBALMGMT_ERROR_MEMORY_ERROR,			// error alloc/dealloc of memory
	K10_GLOBALMGMT_ERROR_MALFORMED_KEY,			// reg key not expected format
	K10_GLOBALMGMT_ERROR_REG_QUERY,				// cannot query key/value
	K10_GLOBALMGMT_ERROR_REG_READ,				// read of key/value failed
	K10_GLOBALMGMT_ERROR_NOPSM_DRIVER,			// PSM driver not started
	K10_GLOBALMGMT_ERROR_NOPSM_LU,				// PSM driver not started
	K10_GLOBALMGMT_ERROR_UNINITED_PSM_FILE,		// OpenRead on a fie which has not been initialized by this class
	K10_GLOBALMGMT_ERROR_LU_SCRUBBED,			// An XLU was scrubbed from the database due to inconsistant data
	K10_GLOBALMGMT_ERROR_MAX_NODES_EXCEEDED,	// someone tried to persist too many txn nodes
    K10_GLOBALMGMT_ERROR_MISSING_SP_INFO,       // no SPs; can't complete cmd
	K10_GLOBALMGMT_ERROR_BAD_NET_DATA,			// Error setting up network information class
	K10_GLOBALMGMT_ERROR_ENVIRONMENT,			// Missing Environment variable
	K10_GLOBALMGMT_ERROR_NO_NET_DATA,			// API reports no network adapters
	K10_GLOBALMGMT_ERROR_BAD_INDEX,				// Invalid index for network adapter data
	K10_GLOBALMGMT_ERROR_BAD_PORT,				// Inavlid network port -- no data for it
	K10_GLOBALMGMT_ERROR_IP_223,				// 1st part of IP address > 223
	K10_GLOBALMGMT_ERROR_IP_127,				// 1st part of IP address = 127
	K10_GLOBALMGMT_ERROR_IP_0,					// 1st part of IP address = 0
	K10_GLOBALMGMT_ERROR_IP_ILLEGAL,			// This IP is illegal
	K10_GLOBALMGMT_ERROR_GATEWAY_223,			// 1st part of Gateway IP address > 223
	K10_GLOBALMGMT_ERROR_GATEWAY_127,			// 1st part of Gateway IP address = 127
	K10_GLOBALMGMT_ERROR_SUBNET_END_0,			// Subnet mask must end in 0
	K10_GLOBALMGMT_ERROR_SUBNET_TRANS,			// Subnet mask has >1 transition between 0 and 1 bit values
	K10_GLOBALMGMT_ERROR_SUBNET_ILLEGAL,		// Subnet mask has a value we think should be illegal
    K10_GLOBALMGMT_ERROR_DEVICE_MAP_CORRUPT,    // Device map is corrupt
	K10_GLOBALMGMT_ERROR_STDOUT_READ_ERROR,		// Error reading output from perl script STDOUT
	K10_GLOBALMGMT_ERROR_BUFFER_OVERFLOW,		// Buffer too small to capture STDOUT
	K10_GLOBALMGMT_ERROR_ADAPTER_NOT_FOUND,		// Network adapter not found.
	K10_GLOBALMGMT_ERROR_MAX_GATEWAYS_EXCEEDED,	// Maximum number of default gateways defined
	K10_GLOBALMGMT_ERROR_ERROR_MAX = K10_GLOBALMGMT_ERROR_MAX_GATEWAYS_EXCEEDED,

	K10_GLOBALMGMT_ERROR_DGSSP_MIN = 
		(K10_GLOBALMGMT_ERROR_BASE |K10_COND_ERROR | K10_GLOBALMGMT_DGSSP_OFFSET), // 0x76008100
	K10_GLOBALMGMT_ERROR_DGSSP_EXECUTION =	K10_GLOBALMGMT_ERROR_DGSSP_MIN,	// DGSSP service failure, see data section
	K10_GLOBALMGMT_ERROR_DGSSP_ECC_ERROR,		// found single bit ECC error
	K10_GLOBALMGMT_ERROR_DGSSP_ECC_MULTI_ERROR,	// found multibit ECC error
	K10_GLOBALMGMT_ERROR_DGSSP_POST_ERROR,		// found POST error
	K10_GLOBALMGMT_ERROR_DGSSP_OEM_ERROR,		// found OEM error
	K10_GLOBALMGMT_ERROR_DGSSP_UNKNOWN_ERROR,	// unknown error record in NVRAM
	K10_GLOBALMGMT_ERROR_DGSSP_SP_PANIC,		// A panic record found in the NVRAM panic log
    K10_GLOBALMGMT_ERROR_DGSSP_CPU_IERR_DETECTED,   // IERR SEL entry was detected on this local SP
	K10_GLOBALMGMT_ERROR_DGSSP_MAX = K10_GLOBALMGMT_ERROR_DGSSP_CPU_IERR_DETECTED,

	//IPv6 Related Error Codes
	K10_GLOBALMGMT_ERROR_IPV6_MIN = (K10_GLOBALMGMT_ERROR_BASE |K10_COND_ERROR | K10_GLOBALMGMT_IPV6_OFFSET), // 0x76008200
		
	K10_GLOBALMGMT_ERROR_IPV6_NOT_SUPPORTED = K10_GLOBALMGMT_ERROR_IPV6_MIN,	//IPV6 Address is not supported	
	K10_GLOBALMGMT_ERROR_BAD_IP,				//BAD IP Address
	K10_GLOBALMGMT_ERROR_IP_NOT_SET,			//IPV6 IP Address is not Set
	K10_GLOBALMGMT_ERROR_IPv6_RESERVED_PREFIX,	//IPV6 Prefix is reserved
	K10_GLOBALMGMT_ERROR_IPV6_MAX = K10_GLOBALMGMT_ERROR_IPv6_RESERVED_PREFIX,	

	// Disk Pools Error Codes
	K10_GLOBALMGMT_ERROR_DISK_POOL_MIN = (K10_GLOBALMGMT_ERROR_BASE |K10_COND_ERROR | K10_GLOBALMGMT_DISK_POOL_OFFSET),

	K10_GLOBALMGMT_ERROR_INVALID_DISK_POOL = K10_GLOBALMGMT_ERROR_DISK_POOL_MIN,	// Disk Pool does not exist
	K10_GLOBALMGMT_ERROR_DISK_POOL_NUMBER_IN_USE,	// Disk Pool number already in use
	K10_GLOBALMGMT_ERROR_MAX_DISK_POOLS_CONFIGURED,	// maximum number of Disk Pools configured
	K10_GLOBALMGMT_ERROR_DISK_POOL_FILE,			// Could not create and/or access Disk Pool file
	K10_GLOBALMGMT_ERROR_DISK_IN_DISK_POOL,			// Disk to be added already in Disk Pool
	K10_GLOBALMGMT_ERROR_DISK_IN_RAID_GROUP,		// Disk to be added to Disk Pool is in Raid Group
	K10_GLOBALMGMT_ERROR_DISK_REMOVED,				// Disk to be added to Disk Pool is removed
	K10_GLOBALMGMT_ERROR_DISK_FAULTED,				// Disk to be added to Disk Pool is faulted
	K10_GLOBALMGMT_ERROR_DISK_NOT_IN_DISK_POOL,		// Disk to be removed not in Disk Pool
	K10_GLOBALMGMT_ERROR_DISK_POOL_RG_IN_USE,		// Disk Pool shrink/destroy would remove RGs with bound LUs
	K10_GLOBALMGMT_ERROR_MAX_DISKS_IN_DISK_POOL,				// operation would exceed maximum number of Disks in Disk Pool
	K10_GLOBALMGMT_ERROR_MAX_DISKS_IN_THIN_POOLS_CONFIGURED,	// operation would exceed maximum number of Disks usable for Thin Pools
	K10_GLOBALMGMT_ERROR_DISK_IN_PROBATION,			// Disk to be added to Disk Pool is in probation state
	K10_GLOBALMGMT_ERROR_DISK_POOL_MAX = K10_GLOBALMGMT_ERROR_DISK_IN_PROBATION	
};



#endif
