/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_scheduler_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_scheduler interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_scheduler_interface
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_scheduler_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!***************************************************************
   @fn fbe_api_scheduler_set_credits(fbe_scheduler_change_credits_t *credits)
 ****************************************************************
 * @brief
 *  This function sets a specific amount of credits in the SEP scheduler
 *
 * @param credits   - how to change the credits
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_set_credits(fbe_scheduler_set_credits_t *credits)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
 
    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_SET_CREDITS,
															credits, 
															sizeof(fbe_scheduler_set_credits_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
   @fn fbe_api_scheduler_get_credits(fbe_scheduler_change_credits_t *credits)
 ****************************************************************
 * @brief
 *  This function gets the credis currently in the SEP scheduler
 *
 * @param credits   - where to put the credit info to
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_get_credits(fbe_scheduler_set_credits_t *credits)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
 
    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_GET_CREDITS,
															credits, 
															sizeof(fbe_scheduler_set_credits_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
   @fn fbe_api_scheduler_remove_cpu_credits_per_core(fbe_scheduler_change_cpu_credits_per_core_t *credits)
 ****************************************************************
 * @brief
 *  This function removes cpu credits from a certain core
 *
 * @param credits   - how to change the credits
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_remove_cpu_credits_per_core(fbe_scheduler_change_cpu_credits_per_core_t *credits)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

	if (credits->cpu_operations_per_second <= 0 ) {
		return FBE_STATUS_GENERIC_FAILURE;
	}
 
    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_REMOVE_CPU_CREDITS_PER_CORE,
															credits, 
															sizeof(fbe_scheduler_change_cpu_credits_per_core_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
   @fn fbe_api_scheduler_add_cpu_credits_per_core(fbe_scheduler_change_cpu_credits_per_core_t *credits)
 ****************************************************************
 * @brief
 *  This function adds cpu credits to a certain core
 *
 * @param credits   - how to change the credits
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_add_cpu_credits_per_core(fbe_scheduler_change_cpu_credits_per_core_t *credits)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

	if (credits->cpu_operations_per_second <=0 ) {
		return FBE_STATUS_GENERIC_FAILURE;
	}
 
    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_ADD_CPU_CREDITS_PER_CORE,
															credits, 
															sizeof(fbe_scheduler_change_cpu_credits_per_core_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
	
}

/*!***************************************************************
   @fn fbe_api_scheduler_remove_cpu_credits_all_cores(fbe_scheduler_change_cpu_credits_all_cores_t *credits)
 ****************************************************************
 * @brief
 *  This function removes the requested credit from all cores at once
 *
 * @param credits   - how to change the credits
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_remove_cpu_credits_all_cores(fbe_scheduler_change_cpu_credits_all_cores_t *credits)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

	if (credits->cpu_operations_per_second <=0 ) {
		return FBE_STATUS_GENERIC_FAILURE;
	}
 
    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_REMOVE_CPU_CREDITS_ALL_CORES,
															credits, 
															sizeof(fbe_scheduler_change_cpu_credits_all_cores_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
   @fn fbe_api_scheduler_add_cpu_credits_all_cores(fbe_scheduler_change_cpu_credits_all_cores_t *credits)
 ****************************************************************
 * @brief
 *  This function adds the requested credit to all cores at once
 *
 * @param credits   - how to change the credits
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_add_cpu_credits_all_cores(fbe_scheduler_change_cpu_credits_all_cores_t *credits)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

	if (credits->cpu_operations_per_second <=0 ) {
		return FBE_STATUS_GENERIC_FAILURE;
	}
 
    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_ADD_CPU_CREDITS_ALL_CORES,
															credits, 
															sizeof(fbe_scheduler_change_cpu_credits_all_cores_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
   @fn fbe_api_scheduler_add_memory_credits(fbe_scheduler_change_memory_credits_t *credits)
 ****************************************************************
 * @brief
 *  This function adds the requested credit to the pool of memory SEP has
 *
 * @param credits   - how to change the credits
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_add_memory_credits(fbe_scheduler_change_memory_credits_t *credits)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

	if (credits->mega_bytes_consumption_change <=0 ) {
		return FBE_STATUS_GENERIC_FAILURE;
	}
 
    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_ADD_MEMORY_CREDITS,
															credits, 
															sizeof(fbe_scheduler_change_memory_credits_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
   @fn fbe_api_scheduler_remove_memory_credits(fbe_scheduler_change_memory_credits_t *credits)
 ****************************************************************
 * @brief
 *  This function removes the requested credit from the pool of memory SEP has
 *
 * @param credits   - how to change the credits
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_remove_memory_credits(fbe_scheduler_change_memory_credits_t *credits)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

	if (credits->mega_bytes_consumption_change <=0 ) {
		return FBE_STATUS_GENERIC_FAILURE;
	}
 
    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_REMOVE_MEMORY_CREDITS,
															credits, 
															sizeof(fbe_scheduler_change_memory_credits_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
   @fn fbe_api_scheduler_set_scale(fbe_scheduler_set_scale_t *scale)
 ****************************************************************
 * @brief
 *  This function sets the 'scale' for the background activities. The scale is basically
 * representing the amount of activity we can do as allocated by the background activity manager.
 * the range is 0 - 100 when 0 means we should do very minimal background IO and 100 means we can do whatever we want.
 *
 * @param scale   - how much we get
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_set_scale(fbe_scheduler_set_scale_t *set_scale)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

	if (set_scale->scale < 0 || set_scale->scale > 100) {
		fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Illegal scale:%llu\n)", __FUNCTION__, set_scale->scale);
		return FBE_STATUS_GENERIC_FAILURE;
	}
 
    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_SET_SCALE,
															set_scale, 
															sizeof(fbe_scheduler_set_scale_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
   @fn fbe_api_scheduler_get_scale(fbe_scheduler_set_scale_t *scale)
 ****************************************************************
 * @brief
 *  This function sets the 'scale' for the background activities. The scale is basically
 * representing the amount of activity we can do as allocated by the background activity manager.
 * the range is 0 - 100 when 0 means we should do very minimal background IO and 100 means we can do whatever we want.
 *
 * @param scale   - how much we get
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_get_scale(fbe_scheduler_set_scale_t *set_scale)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_GET_SCALE,
															set_scale, 
															sizeof(fbe_scheduler_set_scale_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * fbe_api_scheduler_add_debug_hook
 ****************************************************************
 * @brief
 *  This function sets a debug hook in the scheduler for the given object id
 *
 * @param object_id - The object ID of the monitor
 * @param monitor_state - The state of the monitor
 * @param monitor_substate - The substate of the monitor
 * @param val1 - A generic value that can be used for comparisons
 * @param val2 - A generic value that can be used for comparisons
 * @param check_type - The type of checking to be performed when executing the hook
 * @param action - The action to be performed (such as pause)
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_add_debug_hook(fbe_object_id_t object_id,
		                                                   fbe_u32_t monitor_state,
		                                                   fbe_u32_t monitor_substate,
		                                                   fbe_u64_t val1,
		                                                   fbe_u64_t val2,
		                                                   fbe_u32_t check_type,
		                                                   fbe_u32_t action)
{
	fbe_status_t                                status;
	fbe_api_control_operation_status_info_t     status_info;
	fbe_scheduler_debug_hook_t                  hook;

	hook.object_id = object_id;
	hook.monitor_state = monitor_state;
	hook.monitor_substate = monitor_substate;
	hook.val1 = val1;
	hook.val2 = val2;
	hook.check_type = check_type;
	hook.action = action;

	status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_SET_DEBUG_HOOK,
															&hook,
															sizeof(fbe_scheduler_debug_hook_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * fbe_api_scheduler_add_debug_hook_pp
 ****************************************************************
 * @brief
 *  This function sets a debug hook in the Physical Package scheduler for
 *  the given object id
 *
 * @param object_id - The object ID of the monitor
 * @param monitor_state - The state of the monitor
 * @param monitor_substate - The substate of the monitor
 * @param val1 - A generic value that can be used for comparisons
 * @param val2 - A generic value that can be used for comparisons
 * @param check_type - The type of checking to be performed when executing the hook
 * @param action - The action to be performed (such as pause)
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_add_debug_hook_pp(fbe_object_id_t object_id,
                                                              fbe_u32_t monitor_state,
                                                              fbe_u32_t monitor_substate,
                                                              fbe_u64_t val1,
                                                              fbe_u64_t val2,
                                                              fbe_u32_t check_type,
                                                              fbe_u32_t action)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_scheduler_debug_hook_t                  hook;

    hook.object_id = object_id;
    hook.monitor_state = monitor_state;
    hook.monitor_substate = monitor_substate;
    hook.val1 = val1;
    hook.val2 = val2;
    hook.check_type = check_type;
    hook.action = action;

    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_SET_DEBUG_HOOK,
                                                            &hook,
                                                            sizeof(fbe_scheduler_debug_hook_t),
                                                            FBE_SERVICE_ID_SCHEDULER,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * fbe_api_scheduler_get_debug_hook
 ****************************************************************
 * @brief
 *  This function gets a debug hook in the scheduler for the given object id
 *
 * @param object_id - The object ID of the monitor
 * @param monitor_state - The state of the monitor
 * @param monitor_substate - The substate of the monitor
 * @param val1 - A generic value that can be used for comparisons
 * @param val2 - A generic value that can be used for comparisons
 * @param check_type - The type of checking to be performed when executing the hook
 * @param action - The action to be performed (such as pause)
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_get_debug_hook(fbe_scheduler_debug_hook_t *hook)
{
	fbe_status_t                                status;
	fbe_api_control_operation_status_info_t     status_info;

	status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_GET_DEBUG_HOOK,
															hook,
															sizeof(fbe_scheduler_debug_hook_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                      status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * fbe_api_scheduler_get_debug_hook_pp
 ****************************************************************
 * @brief
 *  This function gets a debug hook in the Physical Package scheduler
 *  for the given object id
 *
 * @param object_id - The object ID of the monitor
 * @param monitor_state - The state of the monitor
 * @param monitor_substate - The substate of the monitor
 * @param val1 - A generic value that can be used for comparisons
 * @param val2 - A generic value that can be used for comparisons
 * @param check_type - The type of checking to be performed when executing the hook
 * @param action - The action to be performed (such as pause)
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_get_debug_hook_pp(fbe_scheduler_debug_hook_t *hook)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_GET_DEBUG_HOOK,
                                                            hook,
                                                            sizeof(fbe_scheduler_debug_hook_t),
                                                            FBE_SERVICE_ID_SCHEDULER,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                      status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * fbe_api_scheduler_del_debug_hook
 ****************************************************************
 * @brief
 *  This function deletes a debug hook in the scheduler for the given object id
 *
 * @param object_id - The object ID of the monitor
 * @param monitor_state - The state of the monitor
 * @param monitor_substate - The substate of the monitor
 * @param val1 - A generic value that can be used for comparisons
 * @param val2 - A generic value that can be used for comparisons
 * @param check_type - The type of checking to be performed when executing the hook
 * @param action - The action to be performed (such as pause)
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_del_debug_hook(fbe_object_id_t object_id,
		                                                   fbe_u32_t monitor_state,
		                                                   fbe_u32_t monitor_substate,
		                                                   fbe_u64_t val1,
		                                                   fbe_u64_t val2,
		                                                   fbe_u32_t check_type,
		                                                   fbe_u32_t action)
{
	fbe_status_t                                status;
	fbe_api_control_operation_status_info_t     status_info;
	fbe_scheduler_debug_hook_t                  hook;

	hook.object_id = object_id;
	hook.monitor_state = monitor_state;
	hook.monitor_substate = monitor_substate;
	hook.val1 = val1;
	hook.val2 = val2;
	hook.check_type = check_type;
	hook.action = action;

	status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_DELETE_DEBUG_HOOK,
															&hook,
															sizeof(fbe_scheduler_debug_hook_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * fbe_api_scheduler_del_debug_hook_pp
 ****************************************************************
 * @brief
 *  This function deletes a debug hook in the Physical Package scheduler
 *  for the given object id
 *
 * @param object_id - The object ID of the monitor
 * @param monitor_state - The state of the monitor
 * @param monitor_substate - The substate of the monitor
 * @param val1 - A generic value that can be used for comparisons
 * @param val2 - A generic value that can be used for comparisons
 * @param check_type - The type of checking to be performed when executing the hook
 * @param action - The action to be performed (such as pause)
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/

fbe_status_t FBE_API_CALL fbe_api_scheduler_del_debug_hook_pp(fbe_object_id_t object_id,
                                                           fbe_u32_t monitor_state,
                                                           fbe_u32_t monitor_substate,
                                                           fbe_u64_t val1,
                                                           fbe_u64_t val2,
                                                           fbe_u32_t check_type,
                                                           fbe_u32_t action)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_scheduler_debug_hook_t                  hook;

    hook.object_id = object_id;
    hook.monitor_state = monitor_state;
    hook.monitor_substate = monitor_substate;
    hook.val1 = val1;
    hook.val2 = val2;
    hook.check_type = check_type;
    hook.action = action;

    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_DELETE_DEBUG_HOOK,
                                                            &hook,
                                                            sizeof(fbe_scheduler_debug_hook_t),
                                                            FBE_SERVICE_ID_SCHEDULER,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * fbe_api_scheduler_clear_all_debug_hooks
 ****************************************************************
 * @brief
 *  This function clears all of the debug hooks in the scheduler.
 *
 * @param none
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_clear_all_debug_hooks(fbe_scheduler_debug_hook_t *hook)
{
	fbe_status_t                                status;
	fbe_api_control_operation_status_info_t     status_info;

	status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_CLEAR_DEBUG_HOOKS,
															hook,
															sizeof(fbe_scheduler_debug_hook_t),
															FBE_SERVICE_ID_SCHEDULER,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * fbe_api_scheduler_clear_all_debug_hooks_pp
 ****************************************************************
 * @brief
 *  This function clears all of the debug hooks in the Physical Package
 *  scheduler.
 *
 * @param none
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_scheduler_clear_all_debug_hooks_pp(fbe_scheduler_debug_hook_t *hook)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_CLEAR_DEBUG_HOOKS,
                                                            hook,
                                                            sizeof(fbe_scheduler_debug_hook_t),
                                                            FBE_SERVICE_ID_SCHEDULER,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}
