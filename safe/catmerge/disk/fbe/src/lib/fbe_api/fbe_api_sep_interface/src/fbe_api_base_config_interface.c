/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_base_config_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_base_config interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_base_config_interface
 *
 * @version
 *   12/11/2009:  Created. Peter Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_system_bg_service_interface.h"

/**************************************
                Local variables
**************************************/
#define FBE_API_POLLING_INTERVAL   100 /*ms*/
#define FBE_API_TIMEOUT_MS       30000 /*ms*/

/*******************************************
                Local functions
********************************************/


/*!***************************************************************************
 * @fn       fbe_api_base_config_disable_quiesce()
 *****************************************************************************
 *
 * @brief   Quiesce does not play well with usurper request (i.e. metadata
 *          requests etc).  This method sets a debug hook (on the acitve sp)
 *          prior to sending the usurper request to prevent the usurper request 
 *          from being quiesced.
 *
 * @param   object_id - object ID
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note    This only disables quiesce on the local SP
 *
 * @author
 *  02/01/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_api_base_config_disable_quiesce(fbe_object_id_t object_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_scheduler_debug_hook_t              hook;
    fbe_u32_t                               monitor_state;
	fbe_u32_t                               monitor_substate;
    fbe_base_object_mgmt_get_class_id_t     base_object_mgmt_get_class_id;  

    status = fbe_api_common_send_control_packet (FBE_BASE_OBJECT_CONTROL_CODE_GET_CLASS_ID,
                                                 &base_object_mgmt_get_class_id,
                                                 sizeof (fbe_base_object_mgmt_get_class_id_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return status;
    }

   /* Only the following classes are supported:
    *   o Any raid group class
    *   o Virtual drive class
    *   o Provision drive class
    */
   switch(base_object_mgmt_get_class_id.class_id)
   {
       case FBE_CLASS_ID_VIRTUAL_DRIVE:
           /* The virtual drive uses the raid group quiesce.
            */
           monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE;
           monitor_substate = FBE_RAID_GROUP_SUBSTATE_QUIESCE_START;
           break;

       case FBE_CLASS_ID_PROVISION_DRIVE:
           /* The provision drive has it's own quiesce.
            */
           monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE;
           monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY;
           break;

       default:
           /* All raid types are allowed
            */
           if ((base_object_mgmt_get_class_id.class_id > FBE_CLASS_ID_RAID_GROUP) &&
               (base_object_mgmt_get_class_id.class_id < FBE_CLASS_ID_RAID_LAST)     )
           {
               /* All raid group types are supported.
                */
               monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE;
               monitor_substate = FBE_RAID_GROUP_SUBSTATE_QUIESCE_START;
           }
           else
           {
               fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                              "%s: Unsupported class id: %d\n", 
                              __FUNCTION__, base_object_mgmt_get_class_id.class_id);
               return FBE_STATUS_GENERIC_FAILURE;
           }
           break;
   }

    /* Disable quiesce since we cannot send a usurper that gets quiesced.
     */
    hook.object_id = object_id;
    hook.monitor_state = monitor_state;
    hook.monitor_substate = monitor_substate;
    hook.val1 = 0;
    hook.val2 = 0;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
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
/******************************************** 
 * end fbe_api_base_config_disable_quiesce()
 ********************************************/

/*!***************************************************************************
 * @fn       fbe_api_base_config_enable_quiesce()
 *****************************************************************************
 *
 * @brief   Quiesce does not play well with usurper reqyest (i.e. metadata
 *          requests etc).  This method disables a debug hook prior to send the
 *          usurper request to prevent the usurper request from being quiesced.
 *
 * @param   object_id - object ID
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note    This only enables quiesce on the local SP
 *
 * @author
 *  02/01/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_api_base_config_enable_quiesce(fbe_object_id_t object_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_scheduler_debug_hook_t              hook;
    fbe_u32_t                               monitor_state;
	fbe_u32_t                               monitor_substate;
    fbe_base_object_mgmt_get_class_id_t     base_object_mgmt_get_class_id;  

    status = fbe_api_common_send_control_packet (FBE_BASE_OBJECT_CONTROL_CODE_GET_CLASS_ID,
                                                 &base_object_mgmt_get_class_id,
                                                 sizeof (fbe_base_object_mgmt_get_class_id_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return status;
    }

   /* Only the following classes are supported:
    *   o Any raid group class
    *   o Virtual drive class
    *   o Provision drive class
    */
   switch(base_object_mgmt_get_class_id.class_id)
   {
       case FBE_CLASS_ID_VIRTUAL_DRIVE:
           /* The virtual drive uses the raid group quiesce.
            */
           monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE;
           monitor_substate = FBE_RAID_GROUP_SUBSTATE_QUIESCE_START;
           break;

       case FBE_CLASS_ID_PROVISION_DRIVE:
           /* The provision drive has it's own quiesce.
            */
           monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE;
           monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY;
           break;

       default:
           /* All raid types are allowed
            */
           if ((base_object_mgmt_get_class_id.class_id > FBE_CLASS_ID_RAID_GROUP) &&
               (base_object_mgmt_get_class_id.class_id < FBE_CLASS_ID_RAID_LAST)     )
           {
               /* All raid group types are supported.
                */
               monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE;
               monitor_substate = FBE_RAID_GROUP_SUBSTATE_QUIESCE_START;
           }
           else
           {
               fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                              "%s: Unsupported class id: %d\n", 
                              __FUNCTION__, base_object_mgmt_get_class_id.class_id);
               return FBE_STATUS_GENERIC_FAILURE;
           }
           break;
    }

    /* Disable quiesce since we cannot send a usurper that gets quiesced.
     */
    hook.object_id = object_id;
    hook.monitor_state = monitor_state;
    hook.monitor_substate = monitor_substate;
    hook.val1 = 0;
    hook.val2 = 0;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    status = fbe_api_common_send_control_packet_to_service (FBE_SCHEDULER_CONTROL_CODE_DELETE_DEBUG_HOOK,
                                                            &hook,
                                                            sizeof(fbe_scheduler_debug_hook_t),
                                                            FBE_SERVICE_ID_SCHEDULER,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/******************************************** 
 * end fbe_api_base_config_enable_quiesce()
 ********************************************/

/*!*****************************************************************************
* @fn fbe_api_base_config_metadata_set_default_nonpaged_metadata
*******************************************************************************
* @brief
*   This function is for fixing the object stuck in SPECIALIZED state
*    it sets the default nonpaged metadata of the object 
*
* @param object_id - object ID
*
* @return
*   fbe_status_t - FBE_STATUS_OK - if no error.
*
* @version
*   5/15/2012 - Created. zhangy26
*
******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_set_default_nonpaged_metadata(fbe_object_id_t object_id)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_set_nonpaged_metadata_t         set_nonpaged;
    
    fbe_zero_memory(&set_nonpaged,sizeof(fbe_base_config_control_set_nonpaged_metadata_t));
    set_nonpaged.set_default_nonpaged_b = FBE_TRUE;
    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_SET_NONPAGED,
                                                 &set_nonpaged,
                                                 sizeof(fbe_base_config_control_set_nonpaged_metadata_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_INTERNAL,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
    
        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}
/*!*****************************************************************************
* @fn fbe_api_base_config_metadata_set_nonpaged_metadata
*******************************************************************************
* @brief
*   This function is for fixing the object stuck in SPECIALIZED state
*    it sets the nonpaged metadata of the object accoding to the in param
*
* @param object_id - object ID
*
* @return
*   fbe_status_t - FBE_STATUS_OK - if no error.
*
* @version
*   5/15/2012 - Created. zhangy26
*
******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_set_nonpaged_metadata(fbe_object_id_t object_id,fbe_u8_t* metadata_record_data_p,fbe_u32_t record_data_size)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_set_nonpaged_metadata_t         set_nonpaged;

    if(record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: search size 0x%x is greater than max nonpaged size 0x%x",
                      __FUNCTION__, record_data_size, FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

        return FBE_STATUS_GENERIC_FAILURE;

    }
    
    fbe_copy_memory(set_nonpaged.np_record_data,
                    metadata_record_data_p, 
                    FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);
    set_nonpaged.set_default_nonpaged_b = FBE_FALSE;
    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_SET_NONPAGED,
                                                 &set_nonpaged,
                                                 sizeof(fbe_base_config_control_set_nonpaged_metadata_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_INTERNAL,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
    
        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}

/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_paged_set_bits(
 *       fbe_object_id_t object_id, 
 *       fbe_api_base_config_metadata_paged_change_bits_t * paged_change_bits)
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - Object ID
 * @param paged_change_bits - page change bits
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/09/09 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_paged_set_bits(fbe_object_id_t object_id, fbe_api_base_config_metadata_paged_change_bits_t * paged_change_bits)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_paged_change_bits_t base_config_control_metadata_paged_change_bits;

    base_config_control_metadata_paged_change_bits.metadata_offset = paged_change_bits->metadata_offset;
    
    fbe_copy_memory(base_config_control_metadata_paged_change_bits.metadata_record_data, 
                    paged_change_bits->metadata_record_data, 
                    FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

    base_config_control_metadata_paged_change_bits.metadata_record_data_size = paged_change_bits->metadata_record_data_size;
    base_config_control_metadata_paged_change_bits.metadata_repeat_count = paged_change_bits->metadata_repeat_count;
    base_config_control_metadata_paged_change_bits.metadata_repeat_offset = paged_change_bits->metadata_repeat_offset;

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_SET_BITS,
                                                    &base_config_control_metadata_paged_change_bits,
                                                    sizeof(fbe_base_config_control_metadata_paged_change_bits_t),
                                                    object_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    &status_info,
                                                    FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}


/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_paged_clear_bits(
 *        fbe_object_id_t object_id, 
 *        fbe_api_base_config_metadata_paged_change_bits_t * paged_change_bits)
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param paged_change_bits - pointer to the paged change bits
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/09/09 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_paged_clear_bits(fbe_object_id_t object_id, fbe_api_base_config_metadata_paged_change_bits_t * paged_change_bits)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_paged_change_bits_t base_config_control_metadata_paged_change_bits;

    base_config_control_metadata_paged_change_bits.metadata_offset = paged_change_bits->metadata_offset;
    
    fbe_copy_memory(base_config_control_metadata_paged_change_bits.metadata_record_data, 
                    paged_change_bits->metadata_record_data, 
                    FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

    base_config_control_metadata_paged_change_bits.metadata_record_data_size = paged_change_bits->metadata_record_data_size;
    base_config_control_metadata_paged_change_bits.metadata_repeat_count = paged_change_bits->metadata_repeat_count;
    base_config_control_metadata_paged_change_bits.metadata_repeat_offset = paged_change_bits->metadata_repeat_offset;

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_CLEAR_BITS,
                                                    &base_config_control_metadata_paged_change_bits,
                                                    sizeof(fbe_base_config_control_metadata_paged_change_bits_t),
                                                    object_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    &status_info,
                                                    FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}
/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_paged_write
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param paged_change_bits - pointer to the paged change bits
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *   3/13/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_paged_write(fbe_object_id_t object_id, fbe_api_base_config_metadata_paged_change_bits_t * paged_change_bits)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_paged_change_bits_t base_config_control_metadata_paged_change_bits;

    base_config_control_metadata_paged_change_bits.metadata_offset = paged_change_bits->metadata_offset;
    
    fbe_copy_memory(base_config_control_metadata_paged_change_bits.metadata_record_data, 
                    paged_change_bits->metadata_record_data, 
                    FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

    base_config_control_metadata_paged_change_bits.metadata_record_data_size = paged_change_bits->metadata_record_data_size;
    base_config_control_metadata_paged_change_bits.metadata_repeat_count = paged_change_bits->metadata_repeat_count;
    base_config_control_metadata_paged_change_bits.metadata_repeat_offset = paged_change_bits->metadata_repeat_offset;

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_WRITE,
                                                    &base_config_control_metadata_paged_change_bits,
                                                    sizeof(fbe_base_config_control_metadata_paged_change_bits_t),
                                                    object_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    &status_info,
                                                    FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}

/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_paged_get_bits(
 *        fbe_object_id_t object_id, 
 *        fbe_api_base_config_metadata_paged_get_bits_t * paged_get_bits)
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param paged_get_bits - pointer to the paged get bits
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/09/09 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_paged_get_bits(fbe_object_id_t object_id, fbe_api_base_config_metadata_paged_get_bits_t * paged_get_bits)
{
    fbe_status_t                                        status;
    fbe_api_control_operation_status_info_t             status_info;
    fbe_base_config_control_metadata_paged_get_bits_t   base_config_control_metadata_paged_get_bits;

    base_config_control_metadata_paged_get_bits.metadata_offset = paged_get_bits->metadata_offset;  
    base_config_control_metadata_paged_get_bits.metadata_record_data_size = paged_get_bits->metadata_record_data_size;
    base_config_control_metadata_paged_get_bits.get_bits_flags = 0;
    if ((paged_get_bits->get_bits_flags & FBE_API_BASE_CONFIG_METADATA_PAGED_GET_BITS_FLAGS_FUA_READ) != 0)
    {
        base_config_control_metadata_paged_get_bits.get_bits_flags |= FBE_BASE_CONFIG_CONTROL_METADATA_PAGED_GET_BITS_FLAGS_FUA_READ;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_GET_BITS,
                                                    &base_config_control_metadata_paged_get_bits,
                                                    sizeof(fbe_base_config_control_metadata_paged_get_bits_t),
                                                    object_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    &status_info,
                                                    FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        /* For various reasons (quiesce, abort etc) this request can fail.
         * Trace a warning
         */
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
                       "%s Failed - status: 0x%x operation status: 0x%x\n", 
                       __FUNCTION__, status, status_info.control_operation_status);

        if (status != FBE_STATUS_OK) 
        {
            return status;
        }
        else
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    fbe_copy_memory(paged_get_bits->metadata_record_data, 
                    base_config_control_metadata_paged_get_bits.metadata_record_data,
                    FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

    return status;
}

/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_paged_clear_cache(
 *        fbe_object_id_t object_id)
 ******************************************************************************
 * @brief
 *  This function is for clearing the paged metadata cache.
 *  It is currently used for testing purposes.
 *
 * @param object_id - object ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/2011 - Created. Susan Rundbaken
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_paged_clear_cache(fbe_object_id_t object_id)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_api_control_operation_status_info_t status_info;
    fbe_u32_t                               timeout_ms = FBE_API_TIMEOUT_MS;
    fbe_u32_t                               current_time = 0;
    
    while (current_time < timeout_ms){

        status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_CLEAR_CACHE,
                                                NULL,   /* no struct*/
                                                0,      /* no size */
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);
        if (status == FBE_STATUS_BUSY) {
            fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
                           "%s: Busy - One or more metadata pages in use.\n", 
                           __FUNCTION__);
            current_time = current_time + FBE_API_POLLING_INTERVAL;
            fbe_api_sleep (FBE_API_POLLING_INTERVAL);
        } else if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

            if (status != FBE_STATUS_OK) {
                return status;
            }else{
                return FBE_STATUS_GENERIC_FAILURE;
            }
        } else{
            return status;
        }
    }

    /* Timed-out */
    fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                  "%s: Failed with status %d possibly due to heavy I/O load.\n", 
                  __FUNCTION__, status);          

    return status;
}

/*!*****************************************************************************
 * @fn fbe_api_base_config_metadata_get_info(
 *        fbe_object_id_t object_id, 
 *        fbe_metadata_info_t * metadata_info)
 *******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param metadata_info - pointer to the metadata info
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  2/03/10 - Created. shay harel
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_get_info(fbe_object_id_t object_id, fbe_metadata_info_t * metadata_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_get_info_t     metadata_get_info;

    
    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_GET_INFO,
                                                 &metadata_get_info,
                                                 sizeof(fbe_base_config_control_metadata_get_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_copy_memory(metadata_info, 
                    &metadata_get_info.metadata_info,
                    sizeof(fbe_metadata_info_t));

    return status;

}

/*!*****************************************************************************
 * @fn fbe_api_base_config_metadata_get_data(
 *        fbe_object_id_t object_id, 
 *        fbe_metadata_info_t * metadata_info)
 *******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param metadata_info - pointer to the metadata info
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  4/24/2012 - Created. Jingcheng Zhang
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_get_data(fbe_object_id_t object_id, fbe_metadata_nonpaged_data_t * metadata)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_get_info_t     metadata_get_info;

    
    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_GET_INFO,
                                                 &metadata_get_info,
                                                 sizeof(fbe_base_config_control_metadata_get_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_copy_memory(metadata, 
                    &metadata_get_info.nonpaged_data,
                    sizeof(fbe_metadata_nonpaged_data_t));

    return status;
}


/*!*****************************************************************************
 * @fn fbe_api_base_config_metadata_set_nonpaged_size
 *******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY. it intends to set the
 *  nonpaged metadata size in metadata element or version header
 *
 * @param object_id - object ID
 * @param metadata_info - pointer to set nonpaged size info
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  5/2/2012 - Created. Jingcheng Zhang
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_set_nonpaged_size(fbe_object_id_t object_id, fbe_base_config_control_metadata_set_size_t * set_size_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (set_size_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_SET_NONPAGED_METADATA_SIZE,
                                                 set_size_info,
                                                 sizeof(fbe_base_config_control_metadata_set_size_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }


    return status;
}


/*!****************************************************************************
 * fbe_api_base_config_get_width()
 ******************************************************************************
 * @brief
 *  This function is used to get the width information.
 * 
 * @param 
 *  object_id -                         - Object id for which we need to get the
 *                                              width.
 *  get_width_p -                    - Width pointer.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/28/10 - Created. Shanmugam
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_get_width(fbe_object_id_t object_id,
                                                              fbe_api_base_config_get_width_t * get_width_p)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_get_width_t             get_width;

    get_width.width = 0;

    /* Send a control packet to get the signature status.
     */
    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_WIDTH,
                                                &get_width,
                                                sizeof(fbe_base_config_control_get_width_t),
                                                object_id,
                                                FBE_PACKET_FLAG_INTERNAL,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Copy the width  to caller's memory. 
     */
    get_width_p->width = get_width.width;
    
    return status;
}
/******************************************************************************
 * end fbe_api_base_config_get_width()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_api_base_config_get_encryption_state()
 ******************************************************************************
 * @brief
 *  This function is used to get the encryption state of the object.
 * 
 * @param 
 *  object_id -                         - Object id for which we need to get the
 *                                              encryption state
 *  get_width_p -                    - Width pointer.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/28/13 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_get_encryption_state(fbe_object_id_t object_id,
                                                                   fbe_base_config_encryption_state_t * encryption_state_p)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_get_encryption_state_t encryption_info;

    /* Send a control packet to get the encryption state.
     */
    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_STATE,
                                                &encryption_info,
                                                sizeof(fbe_base_config_control_get_encryption_state_t),
                                                object_id,
                                                FBE_PACKET_FLAG_INTERNAL|FBE_PACKET_FLAG_DESTROY_ENABLED,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    *encryption_state_p = encryption_info.encryption_state;

    return status;
}
/******************************************************************************
 * end fbe_api_base_config_get_encryption_state()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_api_base_config_set_encryption_state()
 ******************************************************************************
 * @brief
 *  This function is used to set the encryption state of the object. This is an
 *  INTERNAL USE ONLY FUNCTION FOR DEBUGGING AND TESTING PURPOSES
 * 
 * @param 
 *  object_id -                         - Object id for which we need to get the
 *                                              encryption state
 *  encryption_state -                    - encryption state
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/28/13 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_set_encryption_state(fbe_object_id_t object_id,
                                                                   fbe_base_config_encryption_state_t encryption_state)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_set_encryption_state_t encryption_info;

    encryption_info.encryption_state = encryption_state;

    /* Send a control packet to set the encryption state.
     */
    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_SET_ENCRYPTION_STATE,
                                                &encryption_info,
                                                sizeof(fbe_base_config_control_set_encryption_state_t),
                                                object_id,
                                                FBE_PACKET_FLAG_INTERNAL,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}
/******************************************************************************
 * end fbe_api_base_config_get_encryption_state()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_api_base_config_get_encryption_mode()
 ******************************************************************************
 * @brief
 *  This function is used to get the encryption mode of the object.
 * 
 * @param 
 *  object_id -                         - Object id for which we need to get the
 *                                              encryption state
 *  get_width_p -                    - Width pointer.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/28/13 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_get_encryption_mode(fbe_object_id_t object_id,
                                                                  fbe_base_config_encryption_mode_t * encryption_mode_p)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_encryption_mode_t       control_encryption_mode;

    /* Send a control packet to get the encryption state.
     */
    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_MODE,
                                                &control_encryption_mode,
                                                sizeof(fbe_base_config_control_encryption_mode_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    *encryption_mode_p = control_encryption_mode.encryption_mode;

    return status;
}
/******************************************************************************
 * end fbe_api_base_config_get_encryption_mode()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_api_base_config_set_encryption_mode()
 ******************************************************************************
 * @brief
 *  This function is for INTERNAL DEBUGGING PURPOSES ONLY
 *  This function is used to set the encryption mode of the object.
 * 
 * @param 
 *  object_id -                         - Object id for which we need to get the
 *                                              encryption mode
 *  encryption_mode -                    - Encryption Mode
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/28/13 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_set_encryption_mode(fbe_object_id_t object_id,
                                                                   fbe_base_config_encryption_mode_t encryption_mode)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_encryption_mode_t       control_encryption_mode;

    /* Send a control packet to get the encryption state.
     */
    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_SET_ENCRYPTION_MODE,
                                                &control_encryption_mode,
                                                sizeof(fbe_base_config_control_encryption_mode_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
}
/******************************************************************************
 * end fbe_api_base_config_set_encryption_mode()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_api_base_config_get_generation_number()
 ******************************************************************************
 * @brief
 *  This function is used to get the generation number of the object.
 * 
 * @param 
 *  object_id -                         - Object id for which we need to get the
 *                                              generation number
 *  gen_num -                    - buffer to store the generation number.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/28/13 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_get_generation_number(fbe_object_id_t object_id,
                                                                     fbe_config_generation_t *gen_num)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_object_get_generation_number_t   generation_number_info;

    /* Send a control packet to get the encryption state.
     */
    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_GENERATION_NUMBER,
                                                &generation_number_info,
                                                sizeof(fbe_base_config_object_get_generation_number_t),
                                                object_id,
                                                FBE_PACKET_FLAG_INTERNAL,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    *gen_num = generation_number_info.generation_number;

    return status;
}
/******************************************************************************
 * end fbe_api_base_config_get_generation_number()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_api_base_config_send_io_control_operation()
 ******************************************************************************
 * @brief
 *  It builds an I/O request and send a control command to base config object
 *  to perform the I/O operation.
 * 
 *  This function is used to only for the internal debugging purpose.
 *  @todo we will change this interface to make it work for all the objects
 * 
 * @param 
 *  packet_p                            - Pointer to the packet.
 *  object_id                           - Object id.
 *  base_config_io_control_operation_p  - I/O control operation pointer.
 *  packet_completion_function          - Packet completion function pointer.
 *  packet_completion_context           - Packet completion context pointer.
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/17/10 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_send_io_control_operation(fbe_packet_t * packet_p,
                                                                        fbe_object_id_t object_id,
                                                                        fbe_base_config_io_control_operation_t * base_config_io_control_operation_p,
                                                                        fbe_packet_completion_function_t packet_completion_function,
                                                                        fbe_packet_completion_context_t packet_completion_context)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_ex_t *                             sep_payload_p = NULL;

    /* If it finds I/O control operation is NULL then return the error. */
    if(base_config_io_control_operation_p == NULL)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:signature status list is NULL.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sep_payload_p = fbe_transport_get_payload_ex (packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    /* Build the I/O control operation. */
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_BASE_CONFIG_CONTROL_CODE_IO_CONTROL_OPERATION,
                                        base_config_io_control_operation_p,
                                        sizeof(fbe_base_config_io_control_operation_t));
    
    /* Send a control packet asynchronously for the I/O control operation. */
    status = fbe_api_common_send_control_packet_asynch(packet_p,
                                                       object_id,
                                                       packet_completion_function,
                                                       packet_completion_context,
                                                       FBE_PACKET_FLAG_NO_ATTRIB,
                                                       FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/******************************************************************************
 * end fbe_api_base_config_send_io_control_operation()
 ******************************************************************************/

/*!*****************************************************************************
 * @fn fbe_status_t FBE_API_CALL fbe_api_base_config_get_downstream_object_list(
        fbe_object_id_t object_id,
        fbe_api_base_config_downstream_object_list_t * downstrea_object_list_p)
 *******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id               - object ID
 * @param downstrea_object_list_p - Pointer to the downstream object list.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  2/03/10 - Created. shay harel
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_get_downstream_object_list(fbe_object_id_t object_id,
                                                                         fbe_api_base_config_downstream_object_list_t * downstrea_object_list_p)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_downstream_object_list_t        downstream_object_list;
    fbe_u16_t                                       index = 0;

    /* Initialize downstream object list before sending control command to get info. */
    downstream_object_list.number_of_downstream_objects = 0;
    for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
    {
        downstream_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }
    
    /* Send control code to get downstream object list. */
    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                 &downstream_object_list,
                                                 sizeof(fbe_base_config_downstream_object_list_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* copy information to client memory. */
    fbe_copy_memory(downstrea_object_list_p, 
                    &downstream_object_list,
                    sizeof(fbe_base_config_downstream_object_list_t));

    return status;

}
/******************************************************************************
 * end fbe_api_base_config_get_downstream_object_list()
 ******************************************************************************/

/*!*****************************************************************************
 * fbe_api_base_config_get_downstream_object_list()
 *******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id               - object ID
 * @param downstrea_object_list_p - Pointer to the downstream object list.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  3/6/2014 - Created. Rob Foley
 *
 ******************************************************************************/

fbe_status_t FBE_API_CALL fbe_api_base_config_get_ds_object_list(fbe_object_id_t object_id,
                                                                 fbe_api_base_config_downstream_object_list_t * downstrea_object_list_p,
                                                                 fbe_packet_attr_t attrib)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_downstream_object_list_t        downstream_object_list;
    fbe_u16_t                                       index = 0;

    /* Initialize downstream object list before sending control command to get info. */
    downstream_object_list.number_of_downstream_objects = 0;
    for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
    {
        downstream_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }
    
    /* Send control code to get downstream object list. */
    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                 &downstream_object_list,
                                                 sizeof(fbe_base_config_downstream_object_list_t),
                                                 object_id,
                                                 /* We can send this even if the object is not ready. */
                                                 attrib,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* copy information to client memory. */
    fbe_copy_memory(downstrea_object_list_p, 
                    &downstream_object_list,
                    sizeof(fbe_base_config_downstream_object_list_t));

    return status;

}
/******************************************************************************
 * end fbe_api_base_config_get_downstream_object_list()
 ******************************************************************************/

/*!*****************************************************************************
 * @fn fbe_status_t FBE_API_CALL fbe_api_base_config_get_upstream_object_list(
        fbe_object_id_t object_id,
        fbe_api_base_config_upstream_object_list_t * upstream_object_list_p)
 *******************************************************************************
 * @brief
 *  This function is called by clients which needs to know about upstream 
 *  object IDs of particular object ID.
 *
 * @param object_id               - object ID
 * @param upstream_object_list_p  - Pointer to the upstream object list.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK  - if no error.
 *
 * @version
 *  04/29/2010 - Created. Sanjay Bhave
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_get_upstream_object_list(fbe_object_id_t object_id,
                                                                         fbe_api_base_config_upstream_object_list_t * upstream_object_list_p)
{
    fbe_status_t                              status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t   status_info;
    fbe_base_config_upstream_object_list_t    upstream_object_list;
    fbe_u16_t                                 index = 0;

    /* Initialize upstream object list before sending control command to get info. */
    upstream_object_list.number_of_upstream_objects = 0;
    for(index = 0; index < FBE_MAX_UPSTREAM_OBJECTS; index++)
    {
        upstream_object_list.upstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }
    
    /* Send control code to get upstream object list. */
    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                 &upstream_object_list,
                                                 sizeof(fbe_base_config_upstream_object_list_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* copy information to client memory. */
    fbe_copy_memory(upstream_object_list_p, 
                    &upstream_object_list,
                    sizeof(fbe_base_config_upstream_object_list_t));

    return status;

}
/******************************************************************************
 * end fbe_api_base_config_get_upstream_object_list()
 ******************************************************************************/


/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_nonpaged_set_checkpoint(
 *       fbe_object_id_t object_id, 
 *       fbe_api_base_config_metadata_nonpaged_change_bits_t * nonpaged_change)
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - Object ID
 * @param nonpaged_change - page change bits
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note        Even thought set checkpoint allows a second metadata offset
 *              this API only sets (1) checkpoint.
 *
 * @version
 *  06/02/11 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_base_config_metadata_nonpaged_set_checkpoint(fbe_object_id_t object_id, 
                                                        fbe_u64_t   metadata_offset,
                                                        fbe_u64_t   checkpoint)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_nonpaged_change_t base_config_control_metadata_nonpaged_change_bits;

    fbe_zero_memory(&base_config_control_metadata_nonpaged_change_bits, sizeof(fbe_base_config_control_metadata_nonpaged_change_t));

    /* This API only support (1) offset.
     */
    base_config_control_metadata_nonpaged_change_bits.metadata_offset = metadata_offset;
    base_config_control_metadata_nonpaged_change_bits.second_metadata_offset = 0;

    fbe_copy_memory(base_config_control_metadata_nonpaged_change_bits.metadata_record_data, &checkpoint, sizeof(fbe_u64_t));
    base_config_control_metadata_nonpaged_change_bits.metadata_record_data_size = sizeof(fbe_u64_t);
    base_config_control_metadata_nonpaged_change_bits.metadata_repeat_count = 0;
    base_config_control_metadata_nonpaged_change_bits.metadata_repeat_offset = 0;

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_SET_CHECKPOINT,
                                                    &base_config_control_metadata_nonpaged_change_bits,
                                                    sizeof(fbe_base_config_control_metadata_nonpaged_change_t),
                                                    object_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    &status_info,
                                                    FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}


/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_nonpaged_incr_checkpoint(
 *        fbe_object_id_t object_id, 
 *        fbe_api_base_config_metadata_nonpaged_change_bits_t * nonpaged_change)
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - object ID
 * @param nonpaged_change - pointer to the nonpaged change bits
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  06/02/11 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_nonpaged_incr_checkpoint(fbe_object_id_t object_id,
                                                fbe_u64_t   metadata_offset,
                                                fbe_u64_t   checkpoint,
                                                fbe_u32_t   repeat_count)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_nonpaged_change_t base_config_control_metadata_nonpaged_change_bits;

    base_config_control_metadata_nonpaged_change_bits.metadata_offset = metadata_offset;
    
    fbe_copy_memory(base_config_control_metadata_nonpaged_change_bits.metadata_record_data, &checkpoint, sizeof(fbe_u64_t));

    base_config_control_metadata_nonpaged_change_bits.metadata_record_data_size = sizeof(fbe_u64_t);
    base_config_control_metadata_nonpaged_change_bits.metadata_repeat_count = repeat_count;
    base_config_control_metadata_nonpaged_change_bits.metadata_repeat_offset = 0;
    base_config_control_metadata_nonpaged_change_bits.second_metadata_offset = 0;

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_INCR_CHECKPOINT,
                                                    &base_config_control_metadata_nonpaged_change_bits,
                                                    sizeof(fbe_base_config_control_metadata_nonpaged_change_t),
                                                    object_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    &status_info,
                                                    FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}

/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_nonpaged_persist(
 *       fbe_object_id_t object_id)
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - Object ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  2/10/2011 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_nonpaged_persist(fbe_object_id_t object_id)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_NONPAGED_PERSIST,
                                                    NULL,
                                                    0,
                                                    object_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    &status_info,
                                                    FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}



/*!***************************************************************
   @fn fbe_api_base_config_disable_all_bg_services()
 ****************************************************************
 * @brief
 *  Disable all BG services on the system, nothing runs and all objects are starved until 
 *  fbe_api_scheduler_enable_all_bg_services is issued. This apply even to new objects
 *  that were added after this command is issued.
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_disable_all_bg_services()
{
    fbe_status_t                                status;

    status = fbe_api_disable_all_bg_services();
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to disable all system background services.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_sleep(1000);  /* Wait a second for all BG I/O's to complete */

    return status;

}

/*!***************************************************************
   @fn fbe_api_base_config_enable_all_bg_services()
 ****************************************************************
 * @brief
 *  Enable all BG services on the system. If some object are explicitly disabled they will remain disable.
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_base_config_enable_all_bg_services()
{
    fbe_status_t                                status;

    status = fbe_api_enable_all_bg_services();
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to enable all system background services.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}


fbe_status_t FBE_API_CALL 
fbe_api_base_config_get_metadata_statistics(fbe_object_id_t object_id, fbe_api_base_config_get_metadata_statistics_t * metadata_statistics)
{
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t             status_info;
    fbe_base_config_control_get_metadata_statistics_t   stats;


    fbe_zero_memory(&stats, sizeof(fbe_base_config_control_get_metadata_statistics_t));

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_METADATA_STATISTICS,
                                                 &stats,
                                                 sizeof(fbe_base_config_control_get_metadata_statistics_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    metadata_statistics->stripe_lock_count = stats.stripe_lock_count;
    metadata_statistics->local_collision_count = stats.local_collision_count;
    metadata_statistics->peer_collision_count = stats.peer_collision_count;
    metadata_statistics->cmi_message_count = stats.cmi_message_count;

    return status;

}

/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_nonpaged_clear_bits(
 *       fbe_object_id_t object_id, 
 *       fbe_api_base_config_metadata_nonpaged_change_bits_t * nonpaged_change)
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - Object ID
 * @param nonpaged_change - page change bits
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/09/09 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_nonpaged_clear_bits(fbe_object_id_t object_id, fbe_api_base_config_metadata_nonpaged_change_bits_t * nonpaged_change)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_nonpaged_change_t base_config_control_metadata_nonpaged_change_bits;

    base_config_control_metadata_nonpaged_change_bits.metadata_offset = nonpaged_change->metadata_offset;
    
    fbe_copy_memory(base_config_control_metadata_nonpaged_change_bits.metadata_record_data, 
                    nonpaged_change->metadata_record_data, 
                    FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

    base_config_control_metadata_nonpaged_change_bits.metadata_record_data_size = nonpaged_change->metadata_record_data_size;
    base_config_control_metadata_nonpaged_change_bits.metadata_repeat_count = nonpaged_change->metadata_repeat_count;
    base_config_control_metadata_nonpaged_change_bits.metadata_repeat_offset = nonpaged_change->metadata_repeat_offset;

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_CLEAR_BITS,
                                                    &base_config_control_metadata_nonpaged_change_bits,
                                                    sizeof(fbe_base_config_control_metadata_nonpaged_change_t),
                                                    object_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    &status_info,
                                                    FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}

/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_nonpaged_set_bits(
 *       fbe_object_id_t object_id, 
 *       fbe_api_base_config_metadata_nonpaged_change_bits_t * nonpaged_change)
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - Object ID
 * @param nonpaged_change - page change bits
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/09/09 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_nonpaged_set_bits(fbe_object_id_t object_id, fbe_api_base_config_metadata_nonpaged_change_bits_t * nonpaged_change)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_nonpaged_change_t base_config_control_metadata_nonpaged_change_bits;

    base_config_control_metadata_nonpaged_change_bits.metadata_offset = nonpaged_change->metadata_offset;
    
    fbe_copy_memory(base_config_control_metadata_nonpaged_change_bits.metadata_record_data, 
                    nonpaged_change->metadata_record_data, 
                    FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

    base_config_control_metadata_nonpaged_change_bits.metadata_record_data_size = nonpaged_change->metadata_record_data_size;
    base_config_control_metadata_nonpaged_change_bits.metadata_repeat_count = nonpaged_change->metadata_repeat_count;
    base_config_control_metadata_nonpaged_change_bits.metadata_repeat_offset = nonpaged_change->metadata_repeat_offset;

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_SET_BITS,
                                                    &base_config_control_metadata_nonpaged_change_bits,
                                                    sizeof(fbe_base_config_control_metadata_nonpaged_change_t),
                                                    object_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    &status_info,
                                                    FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}

/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_nonpaged_write_persist(
 *       fbe_object_id_t object_id,
 *       fbe_api_base_config_metadata_nonpaged_change_bits_t * nonpaged_change)
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - Object ID
 * @param nonpaged_change - page change bits
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/22/13 - Created. Jamin Kang
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_nonpaged_write_persist(fbe_object_id_t object_id, fbe_api_base_config_metadata_nonpaged_change_bits_t * nonpaged_change)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_nonpaged_change_t base_config_control_metadata_nonpaged_change_bits;

    fbe_zero_memory(&base_config_control_metadata_nonpaged_change_bits, sizeof(fbe_base_config_control_metadata_nonpaged_change_t));

    base_config_control_metadata_nonpaged_change_bits.metadata_offset = nonpaged_change->metadata_offset;
    /* This API support only 1 offset */
    base_config_control_metadata_nonpaged_change_bits.metadata_record_data_size = nonpaged_change->metadata_record_data_size;
    base_config_control_metadata_nonpaged_change_bits.metadata_repeat_count = 0;
    base_config_control_metadata_nonpaged_change_bits.metadata_repeat_offset = 0;

    fbe_copy_memory(base_config_control_metadata_nonpaged_change_bits.metadata_record_data, 
                    nonpaged_change->metadata_record_data, 
                    FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_WRITE_PERSIST,
                                                    &base_config_control_metadata_nonpaged_change_bits,
                                                    sizeof(fbe_base_config_control_metadata_nonpaged_change_t),
                                                    object_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    &status_info,
                                                    FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}

/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_memory_read(
 *       fbe_object_id_t object_id,
 *       fbe_api_base_config_metadata_memory_read_t * mm_data)
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - Object ID
 * @param mm_data - buffer to store metadata
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/29/13 - Created. Jamin Kang
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_memory_read(fbe_object_id_t object_id, fbe_base_config_control_metadata_memory_read_t * md_data)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_memory_read_t mm_read_data;

    fbe_copy_memory(&mm_read_data, md_data, sizeof(mm_read_data));
    if (mm_read_data.memory_size > sizeof(mm_read_data.memory_data)) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s: invalid metadata size %u. Limit to %lu\n",
                      __FUNCTION__, mm_read_data.memory_size,
                      (unsigned long)sizeof(mm_read_data.memory_data));
        mm_read_data.memory_size = sizeof(mm_read_data.memory_data);
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_METADATA_MEMORY_READ,
                                                &mm_read_data, sizeof(mm_read_data),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    fbe_copy_memory(md_data, &mm_read_data, sizeof(mm_read_data));

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}

/*!****************************************************************************
 * @fn fbe_api_base_config_metadata_memory_update(
 *       fbe_object_id_t object_id,
 *       fbe_base_config_control_metadata_memory_update_t * mm_data)
 ******************************************************************************
 * @brief
 *  This function is for internal (debugging) use ONLY.
 *
 * @param object_id - Object ID
 * @param md_data -  metadata buffer
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/29/13 - Created. Jamin Kang
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_metadata_memory_update(fbe_object_id_t object_id, fbe_base_config_control_metadata_memory_update_t * md_data)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_memory_update_t update_data;

    fbe_copy_memory(&update_data, md_data, sizeof(update_data));

    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_METADATA_MEMORY_UPDATE,
                                                &update_data, sizeof(update_data),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_base_config_send_event(fbe_packet_t *packet_p, 
                                                                fbe_object_id_t object_id, 
                                                                fbe_base_config_control_send_event_t *send_event,
                                                                fbe_packet_completion_function_t packet_completion_function,
                                                                fbe_packet_completion_context_t packet_completion_context)
 ****************************************************************
 * @brief
 *  This function sends a event packet asynchronously to given object.
 *
 *  This function is used to only for the internal debugging purpose.
 *  @todo we will change this interface to make it work for all type of events 
 *     to send. presently it only supports to send zero permit request event.
 * 
 * @param 
 *  packet_p                            - Pointer to the packet.
 *  object_id                           - Object id.
 *  fbe_base_config_control_send_event_t  - pointer to event data.
 *  packet_completion_function          - Packet completion function pointer.
 *  packet_completion_context           - Packet completion context pointer.
 *
 * @return
 *  fbe_status_t
 *
 * @version
 *  11/07/11 - Created. Amit Dhaduk
 *
 ****************************************************************/

fbe_status_t FBE_API_CALL
fbe_api_base_config_send_event(fbe_packet_t *packet_p, 
                                                                fbe_object_id_t object_id, 
                                                                fbe_base_config_control_send_event_t *send_event,
                                                                fbe_packet_completion_function_t packet_completion_function,
                                                                fbe_semaphore_t *sem)
{

    fbe_status_t status;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;

    payload = fbe_transport_get_payload_ex (packet_p);

    /* If the payload is NULL, exit and return generic failure.
    */
    if (payload == NULL) {
        fbe_api_return_contiguous_packet(packet_p);/*no need to destory !*/
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: payload is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation_p = fbe_payload_ex_allocate_control_operation(payload);

    /* If the control operation cannot be allocated, exit and return generic failure.
    */
    if (control_operation_p == NULL) {
        fbe_api_return_contiguous_packet(packet_p);/*no need to destory !*/
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate new control operation\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* build control operation with data */
    fbe_payload_control_build_operation (control_operation_p,
                                         FBE_BASE_CONFIG_CONTROL_CODE_SEND_EVENT,
                                         send_event,
                                         sizeof(fbe_base_config_control_send_event_t));

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NO_ATTRIB);

       
    /* Send a event packet asynchronously to given object */
    status = fbe_api_common_send_control_packet_asynch(packet_p,
                                                       object_id,
                                                       packet_completion_function,
                                                       sem,
                                                       FBE_PACKET_FLAG_NO_ATTRIB,
                                                       FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_api_base_config_is_background_operation_enabled(
               fbe_object_id_t object_id, 
               fbe_base_config_background_operation_t background_op,
               fbe_bool_t * is_enabled_p)
 ****************************************************************
 * @brief
 *  This function is used to send a request to determine if background
 *  operation is enabled or not.
 *
 * @param object_id     - object ID
 * @param background_op - background operation
 * @param is_enabled_p  - returns FBE_TRUE if background operation is enabled
 *                        Otherwise returns FBE_FALSE
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  08/16/2011 - Created. Amit Dhaduk.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_base_config_is_background_operation_enabled(fbe_object_id_t object_id, 
                        fbe_base_config_background_operation_t background_op, fbe_bool_t * is_enabled)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_background_op_enable_check_t background_op_enable_check;

    background_op_enable_check.background_operation = background_op;

    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_BACKGROUND_OP_ENABLE_CHECK,
                                                &background_op_enable_check,
                                                sizeof(fbe_base_config_background_op_enable_check_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    *is_enabled = background_op_enable_check.is_enabled;
        
    return status;
}
/****************************************************************
 * end fbe_api_base_config_is_background_operation_enabled()
 ****************************************************************/



/*!***************************************************************
 * @fn fbe_api_base_config_enable_background_operation(
 *      fbe_object_id_t object_id, 
 *      fbe_base_config_background_operation_t background_op)
 ****************************************************************
 * @brief
 *  This function is used to send enable background operation request.
 *
 * @param object_id     - object ID
 * @param background_op - background operation
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  08/16/2011 - Created. Amit Dhaduk.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_base_config_enable_background_operation(fbe_object_id_t object_id, 
                        fbe_base_config_background_operation_t background_op)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_background_op_t         enable_operation;

    enable_operation.background_operation = background_op;
    enable_operation.enable = FBE_TRUE;

    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_BACKGROUND_OPERATION,
                                                &enable_operation,
                                                sizeof(fbe_base_config_control_background_op_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}
/****************************************************************
 * end fbe_api_base_config_enable_background_operation()
 ****************************************************************/

/*!***************************************************************
 * @fn fbe_api_base_config_disable_background_operation(
 *      fbe_object_id_t object_id, 
 *      fbe_base_config_background_operation_t background_op)
 ****************************************************************
 * @brief
 *  This function is used to send disable background operation request.
 *
 * @param object_id     - object ID
 * @param background_op - background operation
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  08/16/2011 - Created. Amit Dhaduk.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_base_config_disable_background_operation(fbe_object_id_t object_id, 
                        fbe_base_config_background_operation_t background_op)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_background_op_t         disable_operation;

    disable_operation.background_operation = background_op;
    disable_operation.enable = FBE_FALSE;

    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_BACKGROUND_OPERATION,
                                                &disable_operation,
                                                sizeof(fbe_base_config_control_background_op_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}
/****************************************************************
 * end fbe_api_base_config_disable_background_operation()
 ****************************************************************/

/*!****************************************************************************
 * @fn fbe_api_base_config_passive_request(
 *        fbe_object_id_t object_id)
 ******************************************************************************
 * @brief
 *  This function is for sending passive request to the object.
 *  The ACTIVE object suppose to become PASSIVE if the other SP is UP.
 *
 * @param object_id - object ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  5/25/2012 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_base_config_passive_request(fbe_object_id_t object_id)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_PASSIVE_REQUEST,
                                                NULL,   /* no struct*/
                                                0,      /* no size */
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}

/*!***************************************************************************
 * @fn fbe_api_base_config_set_deny_operation_permission(
 *                              fbe_object_id_t object_id)
 ***************************************************************************** 
 * 
 * @brief   This function is used to `deny' permission for ALL background
 *          operations for a particular object.
 *
 * @param   object_id     - object ID
 *
 * @return  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/05/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_base_config_set_deny_operation_permission(fbe_object_id_t object_id) 
{
    fbe_status_t                            status;
    fbe_api_control_operation_status_info_t status_info;

    /* There is no parameter other than the object id.*/
    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_SET_DENY_OPERATION_PERMISSION,
                                                NULL,
                                                0,
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}
/****************************************************************
 * end fbe_api_base_config_set_deny_operation_permission()
 ****************************************************************/

/*!***************************************************************************
 * @fn fbe_api_base_config_clear_deny_operation_permission(
 *                              fbe_object_id_t object_id)
 ***************************************************************************** 
 * 
 * @brief   This function is used to clear the `deny' permission for ALL background
 *          operations for a particular object.
 *
 * @param   object_id     - object ID
 *
 * @return  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/05/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_base_config_clear_deny_operation_permission(fbe_object_id_t object_id) 
{
    fbe_status_t                            status;
    fbe_api_control_operation_status_info_t status_info;

    /* There is no parameter other than the object id.*/
    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_CLEAR_DENY_OPERATION_PERMISSION,
                                                NULL,
                                                0,
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}
/****************************************************************
 * end fbe_api_base_config_clear_deny_operation_permission()
 ****************************************************************/

/*!***************************************************************************
 * @fn fbe_api_base_config_get_stripe_blob(fbe_object_id_t object_id, fbe_base_config_control_get_stripe_blob_t * blob) 
 *
 ***************************************************************************** 
 * 
 * @brief   This function is used to get stripe lock information
 *          for a particular object.
 *
 * @param   object_id     - object ID
 *
 * @return  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/19/2012  Peter Puhov  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_base_config_get_stripe_blob(fbe_object_id_t object_id, fbe_base_config_control_get_stripe_blob_t * blob) 
{
    fbe_status_t                            status;
    fbe_api_control_operation_status_info_t status_info;

    /* There is no parameter other than the object id.*/
    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_STRIPE_BLOB,
                                                blob,
                                                sizeof(fbe_base_config_control_get_stripe_blob_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
            return status;
        }else{
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}
/****************************************************************
 * end fbe_api_base_config_get_stripe_blob()
 ****************************************************************/





/*******************************************
 * end file fbe_api_base_config_interface.c
 *******************************************/
