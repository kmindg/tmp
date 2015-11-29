/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_metadata_interface.c
***************************************************************************
*
* @brief
*  This file contains database service interface to metadata service
*  
* @version
*  6/15/2011 - Created. 
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database_private.h"
#include "fbe/fbe_metadata_interface.h"
#include "fbe_database_metadata_interface.h"

/*************************
*   LOCAL GLOBALS
*************************/
static fbe_bool_t   fbe_database_metadata_is_nonpaged_loaded = FBE_FALSE;

/*************************
*   INCLUDE FUNCTIONS
*************************/

/*!************************************************************************************************
@fn fbe_status_t fbe_database_metadata_nonpaged_system_load(void)
***************************************************************************************************
* @brief
*  This function 
*
* @param 
*
* @return
*  fbe_status_t
*
***************************************************************************************************/
fbe_status_t fbe_database_metadata_nonpaged_system_load(void)
{
	fbe_status_t status;

	database_trace (FBE_TRACE_LEVEL_INFO,
					FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					"%s: Entry\n", __FUNCTION__);

	status = fbe_database_send_packet_to_service ( FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_LOAD,
                                                   NULL, 
                                                   0,
                                                   FBE_SERVICE_ID_METADATA,
                                                   NULL, /* no sg list*/
                                                   0,    /* no sg list*/
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
	return status;
}

/*!************************************************************************************************
@fn fbe_status_t fbe_database_metadata_nonpaged_system_load_with_drivemask
***************************************************************************************************
* @brief
*  This function loads the system non-paged metadata from specified drives
*
* @param 
*
* @return
*  fbe_status_t
*
***************************************************************************************************/
fbe_status_t fbe_database_metadata_nonpaged_system_load_with_drivemask(fbe_u32_t valid_drive_mask)
{
	fbe_status_t status;

	database_trace (FBE_TRACE_LEVEL_INFO,
					FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					"%s: Entry mask 0x%x\n", __FUNCTION__, valid_drive_mask);

        status = fbe_database_metadata_nonpaged_system_read_persist_with_drivemask(valid_drive_mask, 0, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST + 1);
 
	return status;
}



/*!************************************************************************************************
@fn fbe_status_t fbe_database_metadata_nonpaged_system_clear(void)
***************************************************************************************************
* @brief
*  This function 
*
* @param 
*
* @return
*  fbe_status_t
*
***************************************************************************************************/
fbe_status_t fbe_database_metadata_nonpaged_system_clear(void)
{
	fbe_status_t status;

	database_trace (FBE_TRACE_LEVEL_INFO,
					FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					"%s: Entry\n", __FUNCTION__);

	status = fbe_database_send_packet_to_service ( FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_CLEAR,
                                                   NULL, 
                                                   0,
                                                   FBE_SERVICE_ID_METADATA,
                                                   NULL, /* no sg list*/
                                                   0,    /* no sg list*/
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
	return status;
}

/*!************************************************************************************************
@fn fbe_status_t fbe_database_metadata_nonpaged_system_zero_and_persist()
***************************************************************************************************
* @brief
*  This function will zero and persist the memory of the nonpaged metadata to disk.
*  The range is (start_object_id, start_object_id + object_count)
*
* @param 
*
* @return
*  fbe_status_t
*
***************************************************************************************************/
fbe_status_t fbe_database_metadata_nonpaged_system_zero_and_persist(fbe_object_id_t start_object_id, fbe_block_count_t object_count)
{
    fbe_nonpaged_metadata_persist_system_object_context_t context;
    fbe_status_t status;

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s: Entry\n", __FUNCTION__);

    fbe_zero_memory(&context, sizeof(fbe_nonpaged_metadata_persist_system_object_context_t));
    context.start_object_id = start_object_id;
    context.block_count = object_count;
    
    status = fbe_database_send_packet_to_service ( FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_ZERO_AND_PERSIST,
                                                   (fbe_payload_control_buffer_t)&context, 
                                                   sizeof(fbe_nonpaged_metadata_persist_system_object_context_t),
                                                   FBE_SERVICE_ID_METADATA,
                                                   NULL, /* no sg list*/
                                                   0,    /* no sg list*/
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
    return status;
}

/*!************************************************************************************************
@fn fbe_bool_t fbe_database_get_metadata_nonpaged_loaded(void)
***************************************************************************************************
* @brief
*  This function 
*
* @param 
*
* @return
*  bool
*
***************************************************************************************************/
fbe_bool_t fbe_database_get_metadata_nonpaged_loaded(void)
{
    return fbe_database_metadata_is_nonpaged_loaded;
}

/*!************************************************************************************************
@fn void fbe_database_set_metadata_nonpaged_loaded(void)
***************************************************************************************************
* @brief
*  This function 
*
* @param 
*
* @return
*  None
*
***************************************************************************************************/
void fbe_database_set_metadata_nonpaged_loaded(void)
{
    fbe_database_metadata_is_nonpaged_loaded = FBE_TRUE;
    return;
}

/*!************************************************************************************************
@fn fbe_status_t fbe_database_metadata_nonpaged_load(void)
***************************************************************************************************
* @brief
*  This function 
*
* @param 
*
* @return
*  fbe_status_t
*
***************************************************************************************************/
fbe_status_t fbe_database_metadata_nonpaged_load(void)
{
	fbe_status_t status;

	database_trace (FBE_TRACE_LEVEL_INFO,
					FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					"%s: Entry\n", __FUNCTION__);

	status = fbe_database_send_packet_to_service ( FBE_METADATA_CONTROL_CODE_NONPAGED_LOAD,
                                                   NULL, 
                                                   0,
                                                   FBE_SERVICE_ID_METADATA,
                                                   NULL, /* no sg list*/
                                                   0,    /* no sg list*/
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
    if (status == FBE_STATUS_OK)
    {
        fbe_database_set_metadata_nonpaged_loaded();
    }
	return status;
}

/*!************************************************************************************************
@fn fbe_status_t fbe_database_metadata_set_ndu_in_progress(void)
***************************************************************************************************
* @brief
*  This function sets/clears ndu_in_progress in metadata service.
*
* @param is_set
*
* @return
*  fbe_status_t
*
***************************************************************************************************/
fbe_status_t fbe_database_metadata_set_ndu_in_progress(fbe_bool_t is_set)
{
	fbe_status_t status;
	fbe_metadata_control_code_t control_code;

	if (is_set) {
		control_code = FBE_METADATA_CONTROL_CODE_SET_NDU_IN_PROGRESS;
	} else {
		control_code = FBE_METADATA_CONTROL_CODE_CLEAR_NDU_IN_PROGRESS;
	}
	database_trace (FBE_TRACE_LEVEL_INFO,
					FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					"%s: is set %d\n", __FUNCTION__, is_set);

	status = fbe_database_send_packet_to_service ( control_code,
                                                   NULL, 
                                                   0,
                                                   FBE_SERVICE_ID_METADATA,
                                                   NULL, /* no sg list*/
                                                   0,    /* no sg list*/
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
	return status;
}

/*!************************************************************************************************
@fn fbe_status_t fbe_database_metadata_nonpaged_system_read_persist_with_drivemask
***************************************************************************************************
* @brief
*  This function loads the system non-paged metadata from specified drives
*  The range is (start_object_id, start_object_id + object_count)
*
* @param 
*
* @return
*  fbe_status_t
*
***************************************************************************************************/
fbe_status_t fbe_database_metadata_nonpaged_system_read_persist_with_drivemask(fbe_u32_t valid_drive_mask, fbe_object_id_t object_id, fbe_u32_t object_count)
{
	fbe_status_t status;
	fbe_metadata_nonpaged_system_load_diskmask_t    control_disk_bitmask;

        fbe_zero_memory(&control_disk_bitmask, sizeof(fbe_metadata_nonpaged_system_load_diskmask_t));
	control_disk_bitmask.disk_bitmask = valid_drive_mask;
        control_disk_bitmask.start_object_id = object_id; /* load the specified system object */
        control_disk_bitmask.object_count = object_count;

	database_trace (FBE_TRACE_LEVEL_INFO,
					FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					"%s: Entry mask 0x%x, start 0x%x, count 0x%x\n", __FUNCTION__, valid_drive_mask, object_id, object_count);

	status = fbe_database_send_packet_to_service ( FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_LOAD_WITH_DISKMASK,
                                                   &control_disk_bitmask, 
                                                   sizeof(fbe_metadata_nonpaged_system_load_diskmask_t),
                                                   FBE_SERVICE_ID_METADATA,
                                                   NULL, /* no sg list*/
                                                   0,    /* no sg list*/
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
	return status;
}

/*!************************************************************************************************
@fn fbe_status_t fbe_database_metadata_nonpaged_get_data_ptr(void)
***************************************************************************************************
* @brief
*  This function get the address of the nonpaged metadata. 
*
* @param 
*
* @return
*  fbe_status_t
***************************************************************************************************/
fbe_status_t fbe_database_metadata_nonpaged_get_data_ptr(fbe_database_get_nonpaged_metadata_data_ptr_t *get_nonpaged_metadata)
{
	fbe_status_t status;

	database_trace (FBE_TRACE_LEVEL_INFO,
					FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					"%s: Entry\n", __FUNCTION__);

    if (!get_nonpaged_metadata)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: Illegal argument\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	status = fbe_database_send_packet_to_service ( FBE_METADATA_CONTROL_CODE_NONPAGED_GET_NONPAGED_METADATA_PTR,
                                                   (fbe_payload_control_buffer_t)get_nonpaged_metadata, 
                                                   sizeof(fbe_database_get_nonpaged_metadata_data_ptr_t),
                                                   FBE_SERVICE_ID_METADATA,
                                                   NULL, /* no sg list*/
                                                   0,    /* no sg list*/
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
	return status;
}


/***********************************************
* end file fbe_database_metadata_interface.c
***********************************************/
