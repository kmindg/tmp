/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_metadata_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_lun interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_metadata_interface
 *
 * @version
 *   11/12/2009:  Created. MEV
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_metadata.h"
#include "fbe/fbe_api_metadata_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_api_metadata_debug_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


/*!***************************************************************
   @fn fbe_api_metadata_set_single_object_state(fbe_object_id_t object_id,
                                                fbe_metadata_element_state_t    new_state)
 ****************************************************************
 * @brief
 *  This function sets a single object to be active or passive
 *
 * @param object_id     - The object id to set passive or active
 * @param new_state     - the state to set (FBE_METADATA_ELEMENT_STATE_ACTIVE / FBE_METADATA_ELEMENT_STATE_PASSIVE)
 *
 * @return
 *  fbe_status_t   - FBE_STATUS_OK if success
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_metadata_set_single_object_state(fbe_object_id_t object_id, fbe_metadata_element_state_t  new_state)
{
    fbe_metadata_set_elements_state_t           set_state;
     fbe_status_t                               status;
    fbe_api_control_operation_status_info_t     status_info;

    set_state.new_state = new_state;
    set_state.object_id = object_id;

    status = fbe_api_common_send_control_packet_to_service (FBE_METADATA_CONTROL_CODE_SET_ELEMENTS_STATE,
                                                            &set_state, 
                                                            sizeof(fbe_metadata_set_elements_state_t),
                                                            FBE_SERVICE_ID_METADATA,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
   @fn fbe_api_metadata_set_all_objects_state(fbe_metadata_element_state_t  new_state)
 ****************************************************************
 * @brief
 *  This function sets all the objects on the SP to the given state
 *
 * @param new_state     - the state to set (FBE_METADATA_ELEMENT_STATE_ACTIVE / FBE_METADATA_ELEMENT_STATE_PASSIVE)
 *
 * @return
 *  fbe_status_t   - FBE_STATUS_OK if success
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_metadata_set_all_objects_state(fbe_metadata_element_state_t   new_state)
{
    fbe_metadata_set_elements_state_t           set_state;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t     status_info;

    set_state.new_state = new_state;
    set_state.object_id = FBE_OBJECT_ID_INVALID;/*when we set this value, the service will do all objects*/

    status = fbe_api_common_send_control_packet_to_service (FBE_METADATA_CONTROL_CODE_SET_ELEMENTS_STATE,
                                                            &set_state, 
                                                            sizeof(fbe_metadata_set_elements_state_t),
                                                            FBE_SERVICE_ID_METADATA,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

fbe_status_t FBE_API_CALL fbe_api_metadata_debug_lock(fbe_packet_t * packet, fbe_api_metadata_debug_lock_t *  debug_lock)
{
    fbe_metadata_control_debug_lock_t * control_debug_lock = NULL;
    fbe_status_t  status;

	control_debug_lock = fbe_api_allocate_memory(sizeof(fbe_metadata_control_debug_lock_t));

	control_debug_lock->opcode = debug_lock->opcode;		
	control_debug_lock->status = debug_lock->status;

	control_debug_lock->object_id = debug_lock->object_id;

	control_debug_lock->stripe_number = debug_lock->stripe_number;	

	control_debug_lock->stripe_count = debug_lock->stripe_count;	
	control_debug_lock->packet_ptr = debug_lock->packet_ptr; 

    status = fbe_api_common_send_control_packet_to_service_asynch(packet,
                                                                  FBE_METADATA_CONTROL_CODE_DEBUG_LOCK,
                                                                  control_debug_lock,
                                                                  sizeof(fbe_metadata_control_debug_lock_t),
                                                                  FBE_SERVICE_ID_METADATA,
                                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                                  fbe_api_metadata_debug_lock_completion, 
                                                                  debug_lock,
                                                                  FBE_PACKAGE_ID_SEP_0);

    return status;

}

static fbe_status_t 
fbe_api_metadata_debug_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
	fbe_api_metadata_debug_lock_t *  debug_lock = NULL;
	fbe_metadata_control_debug_lock_t * control_debug_lock = NULL;

	debug_lock = (fbe_api_metadata_debug_lock_t *)context;

	sep_payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(sep_payload);
	fbe_payload_control_get_buffer(control_operation, &control_debug_lock);

	debug_lock->status = control_debug_lock->status;

	debug_lock->packet_ptr = control_debug_lock->packet_ptr;

	fbe_api_free_memory(control_debug_lock);

	fbe_payload_ex_release_control_operation(sep_payload, control_operation);

	return FBE_STATUS_OK;
}

fbe_status_t FBE_API_CALL fbe_api_metadata_debug_ext_pool_lock(fbe_packet_t * packet, fbe_api_metadata_debug_lock_t *  debug_lock)
{
    fbe_metadata_control_debug_lock_t * control_debug_lock = NULL;
    fbe_status_t  status;

	control_debug_lock = fbe_api_allocate_memory(sizeof(fbe_metadata_control_debug_lock_t));

	control_debug_lock->opcode = debug_lock->opcode;		
	control_debug_lock->status = debug_lock->status;

	control_debug_lock->object_id = debug_lock->object_id;

	control_debug_lock->stripe_number = debug_lock->stripe_number;	

	control_debug_lock->stripe_count = debug_lock->stripe_count;	
	control_debug_lock->packet_ptr = debug_lock->packet_ptr; 

    status = fbe_api_common_send_control_packet_to_service_asynch(packet,
                                                                  FBE_METADATA_CONTROL_CODE_DEBUG_EXT_POOL_LOCK,
                                                                  control_debug_lock,
                                                                  sizeof(fbe_metadata_control_debug_lock_t),
                                                                  FBE_SERVICE_ID_METADATA,
                                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                                  fbe_api_metadata_debug_lock_completion, 
                                                                  debug_lock,
                                                                  FBE_PACKAGE_ID_SEP_0);

    return status;

}


fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_system_load(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_LOAD,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_METADATA,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_system_persist(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_PERSIST,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_METADATA,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_load(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_METADATA_CONTROL_CODE_NONPAGED_LOAD,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_METADATA,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_persist(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_METADATA_CONTROL_CODE_NONPAGED_PERSIST,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_METADATA,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_system_load_with_disk_bitmask(fbe_u16_t in_disk_bitmask)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
	fbe_metadata_nonpaged_system_load_diskmask_t    control_disk_bitmask;

        fbe_zero_memory(&control_disk_bitmask, sizeof(fbe_metadata_nonpaged_system_load_diskmask_t));
	control_disk_bitmask.disk_bitmask = in_disk_bitmask;
        control_disk_bitmask.start_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_api_common_send_control_packet_to_service (FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_LOAD_WITH_DISKMASK,
                                                            &control_disk_bitmask, 
                                                            sizeof(fbe_metadata_nonpaged_system_load_diskmask_t),
                                                            FBE_SERVICE_ID_METADATA,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}



/*****************************************************************/
/** fbe_api_metadata_nonpaged_get_version_info
 ****************************************************************
 * @brief
 *  This function gets the version info of non-paged metadata of an
 *  object identified by its object id
 *
 * @param
 *  fbe_object_id_t                                 object_id
 *  fbe_api_nonpaged_metadata_version_info_t    get_version_info
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/14/12 - Created by Jingcheng, Zhang for metadata versioning. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_metadata_nonpaged_get_version_info(fbe_object_id_t object_id, 
                                           fbe_api_nonpaged_metadata_version_info_t *get_version_info)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_control_metadata_get_info_t     metadata_get_info;
        
    if (get_version_info == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input and output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*send packet to base_config to get its non-paged metadata. the on-disk version header is 
      contained in it*/
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

    /*copy the return value*/
    fbe_copy_memory(&get_version_info->ondisk_ver_header, 
                    &metadata_get_info.nonpaged_data,
                    sizeof(fbe_sep_version_header_t));
    get_version_info->cur_structure_size = metadata_get_info.nonpaged_data_size;
    get_version_info->commit_nonpaged_size = metadata_get_info.committed_nonpaged_data_size;

    if (metadata_get_info.nonpaged_initialized)
    {
        get_version_info->metadata_initialized = 1;
    } else
        get_version_info->metadata_initialized = 0;

    return status;	
}
/*!***************************************************************
   @fn fbe_api_metadata_get_object_need_rebuild_count
 ****************************************************************
 * @brief
 *  This function get object's need rebuild count
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_metadata_get_system_object_need_rebuild_count(fbe_object_id_t in_object_id,fbe_u32_t*   need_rebuild_count)
{
    fbe_metadata_nonpaged_get_need_rebuild_info_t           control_need_rebuild_info;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t     status_info;

	control_need_rebuild_info.need_rebuild_count = 0;
	control_need_rebuild_info.object_id = in_object_id;

    status = fbe_api_common_send_control_packet_to_service (FBE_METADATA_CONTROL_CODE_GET_SYSTEM_NONPAGED_NEED_REBUILD_INFO,
                                                            &control_need_rebuild_info, 
                                                            sizeof(fbe_metadata_nonpaged_get_need_rebuild_info_t),
                                                            FBE_SERVICE_ID_METADATA,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *need_rebuild_count = control_need_rebuild_info.need_rebuild_count;
    return status;

}

/*****************************************************************/
/** fbe_api_metadata_nonpaged_backup_objects
 ****************************************************************
 * @brief
 *  This function read nonpaged metadata of specified object id range from
 *  the memory and return to the caller. It returns the nonpaged metadata
 *  as opaque bytes with size FBE_METADATA_NONPAGED_MAX_SIZE.
 *
 * @param
 *  fbe_nonpaged_metadata_backup_restore_t    input parameters for backup/restore
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/30/12 - Created by Jingcheng, Zhang for nonpaged metadata backup/restore. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_backup_objects(fbe_nonpaged_metadata_backup_restore_t *backup_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (backup_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input and output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    backup_p->opcode = FBE_NONPAGED_METADATA_BACKUP_OBJECTS;
    backup_p->operation_successful = FBE_FALSE;
    status = fbe_api_common_send_control_packet_to_service (FBE_METADATA_CONTROL_CODE_NONPAGED_BACKUP_RESTORE,
                                                            backup_p, 
                                                            sizeof(fbe_nonpaged_metadata_backup_restore_t),
                                                            FBE_SERVICE_ID_METADATA,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    } else {
        backup_p->operation_successful = FBE_TRUE;
    }

    return status;

}


/*****************************************************************/
/** fbe_api_metadata_nonpaged_restore_objects_to_disk
 ****************************************************************
 * @brief
 *  This function receive the nonpaged metadata from caller and write to the disk
 *  directly. It operates the nonpaged metadata as opaque bytes with size
 *  FBE_METADATA_NONPAGED_MAX_SIZE.
 *
 * @param
 *  fbe_nonpaged_metadata_backup_restore_t    input parameters for backup/restore
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/30/12 - Created by Jingcheng, Zhang for nonpaged metadata backup/restore. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_restore_objects_to_disk(fbe_nonpaged_metadata_backup_restore_t *backup_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (backup_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input and output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    backup_p->opcode = FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_DISK;
    backup_p->operation_successful = FBE_FALSE;
    status = fbe_api_common_send_control_packet_to_service (FBE_METADATA_CONTROL_CODE_NONPAGED_BACKUP_RESTORE,
                                                            backup_p, 
                                                            sizeof(fbe_nonpaged_metadata_backup_restore_t),
                                                            FBE_SERVICE_ID_METADATA,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    } else {
        backup_p->operation_successful = FBE_TRUE;
    }

    return status;

}


/*****************************************************************/
/** fbe_api_metadata_nonpaged_restore_objects_to_memory
 ****************************************************************
 * @brief
 *  This function receive the nonpaged metadata from caller and write to the memory.
 *  The caller can calls the persist FBE API to write from memory to disk in a later
 *  time. It operates the nonpaged metadata as opaque bytes with size
 *  FBE_METADATA_NONPAGED_MAX_SIZE.
 *
 * @param
 *  fbe_nonpaged_metadata_backup_restore_t    input parameters for backup/restore
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/30/12 - Created by Jingcheng, Zhang for nonpaged metadata backup/restore. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_restore_objects_to_memory(fbe_nonpaged_metadata_backup_restore_t *backup_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (backup_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input and output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    backup_p->opcode = FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_MEMORY;
    backup_p->operation_successful = FBE_FALSE;
    status = fbe_api_common_send_control_packet_to_service (FBE_METADATA_CONTROL_CODE_NONPAGED_BACKUP_RESTORE,
                                                            backup_p, 
                                                            sizeof(fbe_nonpaged_metadata_backup_restore_t),
                                                            FBE_SERVICE_ID_METADATA,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    } else {
        backup_p->operation_successful = FBE_TRUE;
    }

    return status;

}

/*!***************************************************************
   @fn fbe_api_metadata_get_sl_info
 ****************************************************************
 * @brief
 *  This function sets a single object to be active or passive
 *
 * @param object_id     - The object id to set passive or active
 * @param sl_info       - Pointer to the info
 *
 * @return
 *  fbe_status_t   - FBE_STATUS_OK if success
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_metadata_get_sl_info(fbe_api_metadata_get_sl_info_t * sl_info)
{
    fbe_api_metadata_get_sl_info_t * get_sl_info = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    get_sl_info = fbe_api_allocate_memory(sizeof(fbe_api_metadata_get_sl_info_t));
    get_sl_info->object_id = sl_info->object_id;		

    status = fbe_api_common_send_control_packet_to_service (FBE_METADATA_CONTROL_CODE_GET_STRIPE_LOCK_INFO,
                                                            get_sl_info, 
                                                            sizeof(fbe_api_metadata_get_sl_info_t),
                                                            FBE_SERVICE_ID_METADATA,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    } else {
        sl_info->large_io_count = get_sl_info->large_io_count;
        sl_info->blob_size = get_sl_info->blob_size;
        fbe_copy_memory(sl_info->slots, get_sl_info->slots, get_sl_info->blob_size);
    }

    return status;

}
