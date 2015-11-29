/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#include "fbe_database_private.h"
#include "spid_types.h"
#include "spid_kernel_interface.h"


/*
*   Transfer the database service mode reason to SPID degraded reason,
*   and set the degraded mode reason.
*   As a result, spstate command will show the degraded mode reason.
*/
void fbe_database_set_degraded_mode_reason_in_SPID(fbe_database_service_mode_reason_t reason)
{
    SPID_DEGRADED_REASON degraded_reason;

    switch (reason)
    {
    case FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS:
        degraded_reason = DATABASE_CONFIG_LOAD_RAILURE_OR_DATA_CORRUPT;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_CHASSIS_MISMATCHED:
        degraded_reason = CHASSIS_MISMATCHED_WITH_SYSTEM_DRIVES;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_SYSTEM_DISK_DISORDER:
        degraded_reason = SYSTEM_DRIVES_DISORDER;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_INTEGRITY_BROKEN:
        degraded_reason = THREE_MORE_SYSTEM_DRIVES_INVALID;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_DOUBLE_INVALID_DRIVE_WITH_DRIVE_IN_USER_SLOT:
        degraded_reason = TWO_SYSTEM_DRIVES_INVALID_AND_IN_OTHER_SLOT;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_OPERATION_ON_WWNSEED_CHAOS:
        degraded_reason = SET_ILLEGAL_WWNSEED_FLAG;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_IO_ERROR:
        degraded_reason = SYSTEM_DB_HEADER_IO_ERROR;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_TOO_LARGE:
        degraded_reason = SYSTEM_DB_HEADER_TOO_LARGE;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_DATA_CORRUPT:
        degraded_reason = SYSTEM_DB_HEADER_DATA_CORRUPT;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_INVALID_MEMORY_CONF_CHECK:
        degraded_reason = MEMORY_CONFIG_INVALID;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_PROBLEMATIC_DATABASE_VERSION:
        degraded_reason = PROBLEMATIC_DATABASE_VERSION;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_SMALL_SYSTEM_DRIVE:
        degraded_reason = SMALL_SYSTEM_DRIVE;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_NOT_ALL_DRIVE_SET_ICA_FLAGS:
        degraded_reason = NOT_ALL_DRIVE_SET_ICA;
        break;
    case FBE_DATABASE_SERVICE_MODE_REASON_DB_VALIDATION_FAILED:
        degraded_reason = DATABASE_VALIDATION_FAILED;
        break;
    default:
        degraded_reason = INVALID_DEGRADED_REASON;
        break;
    }

    spidSetDegradedModeReason(degraded_reason);
    return;    
}


