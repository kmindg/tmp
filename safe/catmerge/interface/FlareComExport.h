// FlareComExport.h
//
// Copyright (C) 2010 EMC Corporation
//
//
// Header for FlareCom exports
//
//	Revision History
//	----------------
//
//	22 Aug 01	H. Weiner	Initial version
//	10 May 02	B. Yang		Added ADM_FLARE_ERROR_XPE_NOT_ALLOWED
//	08 Mar 10	E. Carter	ARS 354484 - Added ADM_FLARE_ERROR_CANT_FIND_FLEXPORT
//

#ifndef _FlareComExport_h_
#define _FlareComExport_h_

//----------------------------------------------------------------------INCLUDES
#include "k10defs.h"
#include "K10CommonExport.h"

//------------------------------------------------------------------------DEFINES

// Error code
#define FLARECOM_ERROR_BASE	0x71290000

//-------------------------------------------------------------------------ENUMS

enum FLARECOM_ERROR {						// specific error codes
	FLARECOM_ERROR_SUCCESS = 0x00000000L,
	FLARECOM_INFO_MIN = (FLARECOM_ERROR_BASE |K10_COND_INFO),
	FLARECOM_INFO_MAX = FLARECOM_ERROR_SUCCESS,

	FLARECOM_WARN_MIN = (FLARECOM_ERROR_BASE |K10_COND_WARNING),
	ADM_FLARE_WARNING_UNLOCK_FAILED = FLARECOM_WARN_MIN,		// Unlock of mutex failed	
	ADM_FLARE_WARNING_UNMAP_FAILED,			// Could not unmap view of (POLL) shared memory
	ADM_FLARE_WARNING_MUTEX_CLOSE_FAILED,	// Could not close mutex handle
	ADM_FLARE_WARNING_INCONSISTENT_COUNT,	// Different counting methods of same component yield
											// different results.
	FLARECOM_WARN_MAX = ADM_FLARE_WARNING_INCONSISTENT_COUNT,

	FLARECOM_ERROR_ERROR_MIN = (FLARECOM_ERROR_BASE |K10_COND_ERROR),
	ADM_FLARE_ERROR_MAP_INITIALIZATION= FLARECOM_ERROR_ERROR_MIN,
											// Could not reserve space for shared memory
	ADM_FLARE_ERROR_MAP_ALLOC,				// Could not allocate shared memory 
	ADM_FLARE_ERROR_POLL_MUTEX,				// Could not create/open poll data mutex
	ADM_FLARE_ERROR_POLL_DATA,				// Could not get poll data from Flare
	ADM_FLARE_ERROR_POLL_SIZE,				// Could not get poll data size from Flare
	ADM_FLARE_ERROR_MUTEX_TIMEOUT,			
	ADM_FLARE_ERROR_MUTEX_NO_LOCK,			// Could not obtain the poll data mutex
	ADM_FLARE_ERROR_FLASH_CMD,				// Bad flash control command
	ADM_FLARE_ERROR_WAIT_BAD_ECODE,			// Unexpected error code from waitfor...
	ADM_FLARE_ERROR_TOO_MANY_LUNS,			// More than the allowed # of LUNs seen
	ADM_FLARE_ERROR_TOO_MANY_RGROUPS,		// More than the allowed # of RAID groups seen
	ADM_FLARE_ERROR_BAD_SP_COUNT,			// Unexpected number of SPs (0)
	ADM_FLARE_ERROR_BAD_SP_NUM,				// Unexpected  SP ID (should be 0 or 1)
	ADM_FLARE_ERROR_POLL_OVERFLOW,			// Flare Poll data size larger than allowed maximum
	ADM_FLARE_ERROR_COMPONENT,				// Flaredata class set to wrong compnent type
	ADM_FLARE_ERROR_BAD_COMP_WWN,			// Invalid component WWN
	ADM_FLARE_ERROR_BAD_PTR,				// Attempt to use a NULL of invalid pointer
	ADM_FLARE_ERROR_SIZE_MISMATCH,			// Attempt to use a NULL of invalid pointer
	ADM_FLARE_ERROR_BAD_KEY_COMPONENT,		// Can not construct key with given params
	ADM_FLARE_ERROR_BAD_CDB,				// Something wrong in pass-thru CDB
	ADM_FLARE_ERROR_SPID_DRIVER,			// Error accessing SPID driver
	ADM_FLARE_ERROR_DATA_TOO_BIG,			// Data supplied exceeds buffer length
	ADM_FLARE_ERROR_XPE_NOT_ALLOWED,		// Cannot flash xena xpe
	ADM_FLARE_ERROR_MUTEX_COUNT,			// Unexpected mutex count value
	ADM_FLARE_ERROR_CANT_FIND_FLEXPORT,		// Can't find flexport.
	ADM_FLARE_ERROR_LUN_NOT_FOUND,			// Can't find LUN in MCR
	ADM_FLARE_ERROR_FBE_ERROR,				// Generic MCR error
	FLARECOM_ERROR_ERROR_MAX = ADM_FLARE_ERROR_FBE_ERROR,
};


// Error code
#define FBE_ERROR_BASE	0x717F0000

//-------------------------------------------------------------------------ENUMS

enum FBE_ERROR {						// specific error codes
	FBE_ERROR_ERROR_SUCCESS = 0x00000000L,
	FBE_ERROR_ERROR_MIN = (FBE_ERROR_BASE |K10_COND_ERROR),
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR = FBE_ERROR_ERROR_MIN,
    
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_INTER_OP_FAILURE, 
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_UPSTREAM_RAID_GROUP,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_MORE_THAN_ONE_RAID_GROUP,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_INVALID_ID,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_NOT_READY,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_REMOVED,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_SPARE_BUSY,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_SOURCE_BUSY,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_PERF_TIER_MISMATCH,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_BLOCK_SIZE_MISMATCH,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_CAPACITY_MISMATCH,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_SYS_DRIVE_MISMATCH,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNEXPECTED_ERROR,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_PROACTIVE_SPARE_NOT_REQUIRED,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_PROACTIVE_RAID_GROUP_DEGRADED_OR_UNCONSUMED,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_USER_COPY_RAID_GROUP_DEGRADED_OR_UNCONSUMED,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNSUPPORTED_COMMAND,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_SPARES_AVAILABLE,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_SUITABLE_SPARES,
    ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_RAID_GROUP_IS_DEGRADED,
	ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_COPY_NOT_ALLOWED_TO_NON_REDUNDANT_RAID_GROUP,
	ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_COPY_NOT_ALLOWED_TO_BROKEN_RAID_GROUP,
	ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_COPY_NOT_ALLOWED_TO_UNCONSUMED_RAID_GROUP,
	ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_RG_COPY_ALREADY_IN_PROGRESS,
	ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_BROKEN,
	ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNEXPECTED_DRIVE_CONFIGURATION,
	ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_EXCEEDED_ALLOTTED_TIME,
	ADM_FBE_PROVISION_DRIVE_COPY_TO_STATUS_UNKNOWN,
	ADM_FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY,
  
	FBE_ERROR_ERROR_MAX = ADM_FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY,
};

#endif
