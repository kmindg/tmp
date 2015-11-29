/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_system_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions which use to access the
 *  event log service.
 *  
 * @ingroup fbe_api_system_package_interface_class_files
 * @ingroup fbe_api_system_interface
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_system_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_trace_interface.h"


/*!***************************************************************
 * @fn fbe_api_system_reset_failure_info(fbe_package_notification_id_mask_t package_mask)
 ****************************************************************
 * @brief
 *  This fbe api resets all related error counters and error injections in the specified
 *  package mask. Use FBE_PACKAGE_NOTIFICATION_ID_ALL_PACKAGES to clean all pacakges
 *
 * @param event_log_msg    pointer to message count
 * @param package_id    Package ID
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_system_reset_failure_info(fbe_package_notification_id_mask_t package_mask)
{
    fbe_package_notification_id_mask_t        package_id = 0x1;/*will always be the first package*/
    fbe_u32_t                   package_count = 0x1;
    fbe_package_id_t            fbe_package_id = 0x1;/*start with the first one, no matter what it is*/
	fbe_status_t				status;
	fbe_bool_t					initialized = FBE_FALSE;

	/*deal with logical error injections if they are set*/
	status = fbe_api_logical_error_injection_disable();
	if (status != FBE_STATUS_OK) {
		 fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to disable injection, status: %d\n", __FUNCTION__, status);
	}

	status = fbe_api_logical_error_injection_destroy_objects();
	if (status != FBE_STATUS_OK) {
		 fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to destroy injection objects, status: %d\n", __FUNCTION__, status);
	}

	status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
	if (status != FBE_STATUS_OK) {
		 fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to disable injection records, status: %d\n", __FUNCTION__, status);
	}

	/*deal with backend error injection*/
	status = fbe_api_protocol_error_injection_stop();
	if (status != FBE_STATUS_OK) {
		 fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to stop BE error injection, status: %d\n", __FUNCTION__, status);
	}

	status  = fbe_api_protocol_error_injection_remove_all_records();
	if (status != FBE_STATUS_OK) {
		 fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to stop BE error injection, status: %d\n", __FUNCTION__, status);
	}


    /*go over the packages the user asked for and clear error traces*/
    while (package_count < FBE_PACKAGE_ID_LAST) {
        if ((package_id & package_mask)) {
			fbe_notification_convert_package_bitmap_to_fbe_package(package_id, &fbe_package_id);
			fbe_api_common_package_entry_initialized(fbe_package_id, &initialized);
			if (initialized) {
				status = fbe_api_trace_reset_error_counters(fbe_package_id);
				if (status != FBE_STATUS_OK) {
					fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to stop BE error injection, status: %d\n", __FUNCTION__, status);
				}
			}
        }

        package_id <<= 1;/*go to next one*/
        package_count++;
    }
    
    
    return FBE_STATUS_OK;


}

/*!***************************************************************
 * @fn fbe_api_system_get_failure_info(fbe_api_system_get_failure_info_t *err_info_ptr)
 ****************************************************************
 * @brief
 *  This fbe api retreives any related error information from all pckages.
 *
 * @param event_log_msg    pointer to message count
 * @param package_id    Package ID
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_system_get_failure_info(fbe_api_system_get_failure_info_t *err_info_ptr)
{
	fbe_status_t						status;
	fbe_api_trace_error_counters_t		counters;
	fbe_package_id_t					package_id = FBE_PACKAGE_ID_INVALID + 1;
	fbe_bool_t							initialized = FALSE;

	if (err_info_ptr == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:NULL pointer\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	fbe_zero_memory(err_info_ptr, sizeof(fbe_api_system_get_failure_info_t));

	for (;package_id < FBE_PACKAGE_ID_LAST; package_id++) {
		fbe_api_common_package_entry_initialized(package_id, &initialized);
		if (initialized) {
			status = fbe_api_trace_get_error_counter(&counters, package_id);
			if (status != FBE_STATUS_OK) {
				fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:failed to get races for package %d, status: %d\n", __FUNCTION__, package_id, status);
			}

			fbe_copy_memory(&err_info_ptr->error_counters[package_id], &counters, sizeof(fbe_api_trace_error_counters_t));
		}
	}

	return FBE_STATUS_OK;

}
