#ifndef FBE_JOB_SERVICE_ERROR_H
#define FBE_JOB_SERVICE_ERROR_H

#include "fbe/fbe_types.h"

typedef enum fbe_job_service_error_type_e 
{
    FBE_JOB_SERVICE_ERROR_NO_ERROR = 0,

    /* common job service errors. */
    FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR,
	FBE_JOB_SERVICE_ERROR_INVALID_VALUE,
    FBE_JOB_SERVICE_ERROR_INVALID_ID,                           /* RG/LUN create invalid id*/
    FBE_JOB_SERVICE_ERROR_NULL_COMMAND,
    FBE_JOB_SERVICE_ERROR_UNKNOWN_ID,                           /* RG destroy and LUN update unknown id*/
    FBE_JOB_SERVICE_ERROR_INVALID_JOB_ELEMENT,
    FBE_JOB_SERVICE_ERROR_TIMEOUT,                              /* The job didn't complete in the time specified*/

    /* create/destroy library error codes. */
    FBE_JOB_SERVICE_ERROR_INSUFFICIENT_DRIVE_CAPACITY,          /* One or more drives had insufficient capacity */
    FBE_JOB_SERVICE_ERROR_RAID_GROUP_ID_IN_USE,                 /* RG create rg id is in use*/
    FBE_JOB_SERVICE_ERROR_LUN_ID_IN_USE,                        /* LUN create lun id is in use*/
    FBE_JOB_SERVICE_ERROR_INVALID_CONFIGURATION,                /* RG create invalid configuration*/
    FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION,                /* RG create on a drive that doesn't exist*/
    FBE_JOB_SERVICE_ERROR_DUPLICATE_PVD,                        /* RG create on a duplicate drive*/
    FBE_JOB_SERVICE_ERROR_INCOMPATIBLE_PVDS,                    /* RG create on a incompatible drive*/
    FBE_JOB_SERVICE_ERROR_INCOMPATIBLE_DRIVE_TYPES,             /* RG create with incompatiable drive types*/
    FBE_JOB_SERVICE_ERROR_INVALID_RAID_GROUP_TYPE,              /* RG create with invalid rg type*/
    FBE_JOB_SERVICE_ERROR_INVALID_RAID_GROUP_NUMBER,            /* RG create with invalid rg number*/
    FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY,   /* RG/LUN create request beyond current available capacity*/
    FBE_JOB_SERVICE_ERROR_INVALID_DRIVE_COUNT,                  /* RG create with invalid drive count*/
    FBE_JOB_SERVICE_ERROR_REQUEST_OBJECT_HAS_UPSTREAM_EDGES,    /* RG destroy object which has lun connect to it*/
    FBE_JOB_SERVICE_ERROR_INVALID_CAPACITY,                     /* LUN create got invalid capacity from RG*/
    FBE_JOB_SERVICE_ERROR_INVALID_ADDRESS_OFFSET,               /* LUN create with invalid NDB address offset*/
	FBE_JOB_SERVICE_ERROR_CONFIG_UPDATE_FAILED,
	FBE_JOB_SERVICE_ERROR_RAID_GROUP_NOT_READY,                 /* LUN create on the rg which is not ready*/
	FBE_JOB_SERVICE_ERROR_SYSTEM_LIMITS_EXCEEDED,               /* RG/LUN create exceed system configuration limits*/
	FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED,             /* The DB drives are double degraded */
    FBE_JOB_SERVICE_ERROR_INVALID,
	/**************************************************************
	When Adding more stuff here you MUST update Admin with the 
	translation of this error to the appropriate sunburst code in   
	sunburst_errors.h
	***************************************************************/

    /*! @note The following error code are `non-terminal' errors.  This means 
     *        the operation will continue to be retried until it either succeeds
     *        or encounters a terminal error.
     */ 
    FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE,
    FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE,
    FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED,
    FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DEGRADED,
    FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_BROKEN,
    FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_HAS_COPY_IN_PROGRESS,
    FBE_JOB_SERVICE_ERROR_PRESENTLY_SOURCE_DRIVE_DEGRADED,
    /*! `non-terminal' errors end above */

    /* spare library job service error codes. */
    FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND,
    FBE_JOB_SERVICE_ERROR_INVALID_VIRTUAL_DRIVE_OBJECT_ID,
    FBE_JOB_SERVICE_ERROR_INVALID_EDGE_INDEX,
    FBE_JOB_SERVICE_ERROR_INVALID_PERM_SPARE_EDGE_INDEX,
    FBE_JOB_SERVICE_ERROR_INVALID_PROACTIVE_SPARE_EDGE_INDEX,
    FBE_JOB_SERVICE_ERROR_INVALID_ORIGINAL_OBJECT_ID,
    FBE_JOB_SERVICE_ERROR_INVALID_SPARE_OBJECT_ID,
    FBE_JOB_SERVICE_ERROR_SWAP_UNEXPECTED_ERROR,
    FBE_JOB_SERVICE_ERROR_SWAP_PERMANENT_SPARE_NOT_REQUIRED,
    FBE_JOB_SERVICE_ERROR_SWAP_PROACTIVE_SPARE_NOT_REQUIRED,
    FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN,
    FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT,
    FBE_JOB_SERVICE_ERROR_SWAP_COPY_SOURCE_DRIVE_REMOVED,
    FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_REMOVED,
    FBE_JOB_SERVICE_ERROR_SWAP_COPY_INVALID_DESTINATION_DRIVE,
    FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY,
    FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_HAS_UPSTREAM_RAID_GROUP,
    FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL,
    FBE_JOB_SERVICE_ERROR_SWAP_STATUS_NOT_POPULATED,
    FBE_JOB_SERVICE_ERROR_SWAP_CREATE_EDGE_FAILED,
    FBE_JOB_SERVICE_ERROR_SWAP_DESTROY_EDGE_FAILED,
	/**************************************************************
	When Adding more stuff here you MUST update Admin with the 
	translation of this error to the appropriate sunburst code in   
	sunburst_errors.h
	***************************************************************/

    /* spare library job service error code used for mapping spare validation failure.*/
    FBE_JOB_SERVICE_ERROR_SPARE_NOT_READY,
    FBE_JOB_SERVICE_ERROR_SPARE_REMOVED,
    FBE_JOB_SERVICE_ERROR_SPARE_BUSY,
    FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_UNCONSUMED,
    FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_NOT_REDUNDANT,
    FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_DESTROYED,
    FBE_JOB_SERVICE_ERROR_INVALID_DESIRED_SPARE_DRIVE_TYPE,
    FBE_JOB_SERVICE_ERROR_OFFSET_MISMATCH,
    FBE_JOB_SERVICE_ERROR_BLOCK_SIZE_MISMATCH,
    FBE_JOB_SERVICE_ERROR_CAPACITY_MISMATCH,
    FBE_JOB_SERVICE_ERROR_SYS_DRIVE_MISMATCH,
    FBE_JOB_SERVICE_ERROR_UNSUPPORTED_EVENT_CODE,
    
    /* configure pvd object with different config type - job service error codes. */
    FBE_JOB_SERVICE_ERROR_PVD_INVALID_UPDATE_TYPE,
    FBE_JOB_SERVICE_ERROR_PVD_INVALID_CONFIG_TYPE,
    FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_UNCONSUMED,
    FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_SPARE,
    FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_RAID,
    FBE_JOB_SERVICE_ERROR_PVD_IS_NOT_CONFIGURED,
    FBE_JOB_SERVICE_ERROR_PVD_IS_IN_USE_FOR_RAID_GROUP,
    FBE_JOB_SERVICE_ERROR_PVD_IS_IN_END_OF_LIFE_STATE,
	/**************************************************************
	When Adding more stuff here you MUST update Admin with the 
	translation of this error to the appropriate sunburst code in   
	sunburst_errors.h
	***************************************************************/

    /* system drive copy back error codes*/
    FBE_JOB_SERVICE_ERROR_SDCB_NOT_SYSTEM_PVD,
    FBE_JOB_SERVICE_ERROR_SDCB_FAIL_GET_ACTION,
    FBE_JOB_SERVICE_ERROR_SDCB_FAIL_CHANGE_PVD_CONFIG,
    FBE_JOB_SERVICE_ERROR_SDCB_FAIL_POST_PROCESS,
    FBE_JOB_SERVICE_ERROR_SDCB_START_DB_TRASACTION_FAIL,
    FBE_JOB_SERVICE_ERROR_SDCB_COMMIT_DB_TRASACTION_FAIL,
	/**************************************************************
	When Adding more stuff here you MUST update Admin with the 
	translation of this error to the appropriate sunburst code in   
	sunburst_errors.h
	***************************************************************/

    /*! Add spare library error cases here. */

} fbe_job_service_error_type_t;

#endif /* FBE_JOB_SERVICE_ERROR_H */




