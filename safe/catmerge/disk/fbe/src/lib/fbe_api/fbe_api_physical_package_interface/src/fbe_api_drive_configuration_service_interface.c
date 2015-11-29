/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_drive_configuration_service_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Driver Configuration Service related APIs.
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_drive_configuration_service_interface
 * 
 * @version
 *   10/14/08    sharel - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_get_handles(
 *     fbe_api_drive_config_get_handles_list_t *get_handle, 
 *     fbe_api_drive_config_handle_type_t type) 
 *****************************************************************
 * @brief
 *  Get all the handles of a specific type
 *  This is a synchronous function
 *
 * @param get_handle -  structure to get data with.
 * @param type - which table we want to read from.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL  fbe_api_drive_configuration_interface_get_handles(fbe_api_drive_config_get_handles_list_t *get_handle, 
                                                                             fbe_api_drive_config_handle_type_t type)
{

    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_drive_configuration_control_get_handles_list_t *    api_get_handles = NULL;
    fbe_payload_control_operation_opcode_t                  control_code = 0;

    if (get_handle == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    api_get_handles = (fbe_drive_configuration_control_get_handles_list_t *)fbe_api_allocate_memory(sizeof (fbe_drive_configuration_control_get_handles_list_t));

    if (api_get_handles == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Can't allocate memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (type) {
    case FBE_API_HANDLE_TYPE_DRIVE:
        control_code = FBE_DRIVE_CONFIGURATION_CONTROL_GET_HANDLES_LIST;
        break;
    case FBE_API_HANDLE_TYPE_PORT:
        control_code = FBE_DRIVE_CONFIGURATION_CONTROL_GET_PORT_HANDLES_LIST;
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Illegal handle type:%d\n", __FUNCTION__, type);
        fbe_api_free_memory(api_get_handles);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
     status = fbe_api_common_send_control_packet_to_service (control_code,
                                                             api_get_handles,
                                                             sizeof (fbe_drive_configuration_control_get_handles_list_t),
                                                             FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory(api_get_handles);
        return FBE_STATUS_GENERIC_FAILURE;
    }

     /*copy data to user, technically, we can copy a one to one, but we want to do it correctly so if one of the sturcutres ever change
     the code would still work*/
     if (api_get_handles->total_count > MAX_THRESHOLD_TABLE_RECORDS) {
         fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s. Invalid total count returned: %d\n", __FUNCTION__, api_get_handles->total_count);
         api_get_handles->total_count = MAX_THRESHOLD_TABLE_RECORDS;
     }
     fbe_copy_memory (get_handle->handles_list, api_get_handles->handles_list,
                     (api_get_handles->total_count * sizeof (fbe_drive_configuration_handle_t)));

     get_handle->total_count = api_get_handles->total_count;

     fbe_api_free_memory(api_get_handles);
     return status;

}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_get_drive_table_entry(
 *     fbe_drive_configuration_handle_t handle, 
 *     fbe_drive_configuration_record_t *entry) 
 *****************************************************************
 * @brief
 *  This function gets a table entry by handle. 
 *  This is a synchronous function
 *
 * @param handle - handle for table lookup.
 * @param entry - pointer for return info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL  fbe_api_drive_configuration_interface_get_drive_table_entry(fbe_drive_configuration_handle_t handle, 
                                                                                       fbe_drive_configuration_record_t *entry)
{

    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_drive_configuration_control_get_drive_record_t *    api_get_record = NULL;

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    api_get_record = (fbe_drive_configuration_control_get_drive_record_t *)fbe_api_allocate_memory(sizeof (fbe_drive_configuration_control_get_drive_record_t));

    if (api_get_record == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Can't allocate memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    api_get_record->handle = handle;
    
     status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_GET_DRIVE_RECORD,
                                                             api_get_record,
                                                             sizeof (fbe_drive_configuration_control_get_drive_record_t),
                                                             FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory(api_get_record);
        return FBE_STATUS_GENERIC_FAILURE;
    }

     fbe_copy_memory (entry, &api_get_record->record, sizeof (fbe_drive_configuration_record_t));

     fbe_api_free_memory(api_get_record);
     return status;

}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_change_drive_thresholds(
 *     fbe_drive_configuration_handle_t handle, 
 *     fbe_drive_stat_t *new_thresholds) 
 *****************************************************************
 * @brief
 *  This function changes the threshold of a drive using a handle. 
 *  This is a synchronous function
 *
 * @param handle - handle for table lookup.
 * @param new_thresholds - the new threshold to set.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL  fbe_api_drive_configuration_interface_change_drive_thresholds(fbe_drive_configuration_handle_t handle, 
                                                                                         fbe_drive_stat_t *new_thresholds)
{

    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_drive_configuration_control_change_thresholds_t *   change_threshold = NULL;

    if (new_thresholds == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    change_threshold = (fbe_drive_configuration_control_change_thresholds_t *)fbe_api_allocate_memory(sizeof (fbe_drive_configuration_control_change_thresholds_t));

    if (change_threshold == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Can't allocate memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    change_threshold->handle = handle;
    fbe_copy_memory(&change_threshold->new_threshold, new_thresholds, sizeof (fbe_drive_stat_t));
    
     status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_CHANGE_THRESHOLDS,
                                                             change_threshold,
                                                             sizeof (fbe_drive_configuration_control_change_thresholds_t),
                                                             FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory(change_threshold);
        return FBE_STATUS_GENERIC_FAILURE;
    }

     
     fbe_api_free_memory(change_threshold);
     return status;

}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_add_drive_table_entry(
 *     fbe_drive_configuration_record_t *entry, 
 *     fbe_drive_configuration_handle_t *handle) 
 *****************************************************************
 * @brief
 *  This function adds a new entry. 
 *  This is a synchronous function
 *
 * @param entry - pointer to the data to update.
 * @param handle - pointer returned handle.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL  fbe_api_drive_configuration_interface_add_drive_table_entry(fbe_drive_configuration_record_t *entry, 
                                                                                       fbe_drive_configuration_handle_t *handle)
{

    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_drive_configuration_control_add_record_t *          api_add_record = NULL;

    if (handle == NULL || entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    api_add_record = (fbe_drive_configuration_control_add_record_t *)fbe_api_allocate_memory(sizeof (fbe_drive_configuration_control_add_record_t));

    if (api_add_record == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Can't allocate memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory (&api_add_record->new_record, entry,  sizeof (fbe_drive_configuration_record_t));

    status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_ADD_RECORD,
                                                             api_add_record,
                                                             sizeof (fbe_drive_configuration_control_add_record_t),
                                                             FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory(api_add_record);
        return FBE_STATUS_GENERIC_FAILURE;
    }

     *handle = api_add_record->handle;
     
     fbe_api_free_memory(api_add_record);
     return status;

}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_start_update() 
 *****************************************************************
 * @brief
 *  This function marks the user starts a table update. 
 *  This is a synchronous function
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_dcs_dieh_status_t FBE_API_CALL fbe_api_drive_configuration_interface_start_update(void)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info = {0};
    
    status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_START_TABLE_UPDATE,
                                                             NULL,
                                                             0,
                                                             FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK)
     {
         fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                        __FUNCTION__, status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
         return FBE_DCS_DIEH_STATUS_FAILED;
     }
     else if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
     {    
         fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:payload error:%d, payload qualifier:%d\n", 
                        __FUNCTION__, status_info.control_operation_status, status_info.control_operation_qualifier);
         return status_info.control_operation_qualifier;
     }
     return FBE_DCS_DIEH_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_end_update() 
 *****************************************************************
 * @brief
 *  This function marks the user ends a table update. 
 *  This is a synchronous function
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_drive_configuration_interface_end_update(void)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_END_TABLE_UPDATE,
                                                            NULL,
                                                            0,
                                                            FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

     return status;


}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_add_port_table_entry(
 *     fbe_drive_configuration_port_record_t *entry,
 *     fbe_drive_configuration_handle_t *handle)
 *****************************************************************
 * @brief
 *  This function adds a new entry for ports. 
 *  This is a synchronous function
 *
 * @param entry - pointer to the data to update.
 * @param handle - pointer to the returned handle.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_drive_configuration_interface_add_port_table_entry(fbe_drive_configuration_port_record_t *entry, 
                                                                                     fbe_drive_configuration_handle_t *handle)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_drive_configuration_control_add_port_record_t *     api_add_record = NULL;

    if (handle == NULL || entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    api_add_record = (fbe_drive_configuration_control_add_port_record_t *)fbe_api_allocate_memory(sizeof (fbe_drive_configuration_control_add_port_record_t));

    if (api_add_record == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Can't allocate memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory (&api_add_record->new_record, entry,  sizeof (fbe_drive_configuration_port_record_t));

    status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_ADD_PORT_RECORD,
                                                             api_add_record,
                                                             sizeof (fbe_drive_configuration_control_add_port_record_t),
                                                             FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory(api_add_record);
        return FBE_STATUS_GENERIC_FAILURE;
    }

     *handle = api_add_record->handle;
     
     fbe_api_free_memory(api_add_record);
     return status;

}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_change_port_thresholds(
 *     fbe_drive_configuration_handle_t handle,
 *     fbe_port_stat_t *new_thresholds)
 *****************************************************************
 * @brief
 *  This function changes the threshold of the port, using a handle to it. 
 *  This is a synchronous function
 *
 * @param handle - habndle for table lookup.
 * @param new_thresholds - pointer to the new threshold to set.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_drive_configuration_interface_change_port_thresholds(fbe_drive_configuration_handle_t handle,
                                                                                       fbe_port_stat_t *new_thresholds)
{
    fbe_status_t                                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                     status_info;
    fbe_drive_configuration_control_change_port_threshold_t *   change_threshold = NULL;

    if (new_thresholds == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    change_threshold = (fbe_drive_configuration_control_change_port_threshold_t *)fbe_api_allocate_memory(sizeof (fbe_drive_configuration_control_change_port_threshold_t));

    if (change_threshold == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Can't allocate memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    change_threshold->handle = handle;
    fbe_copy_memory(&change_threshold->new_threshold, new_thresholds, sizeof (fbe_port_stat_t));

     status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_CHANGE_PORT_THRESHOLD,
                                                             change_threshold,
                                                             sizeof (fbe_drive_configuration_control_change_port_threshold_t),
                                                             FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory(change_threshold);
        return FBE_STATUS_GENERIC_FAILURE;
    }


     fbe_api_free_memory(change_threshold);
     return status;


}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_get_port_table_entry(
 *     fbe_drive_configuration_handle_t handle,
 *     fbe_drive_configuration_port_record_t *entry)
 *****************************************************************
 * @brief
 *  This function gets port table entry. 
 *  This is a synchronous function
 *
 * @param handle - habndle for table lookup.
 * @param entry - the data to update.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_drive_configuration_interface_get_port_table_entry(fbe_drive_configuration_handle_t handle, 
                                                                                     fbe_drive_configuration_port_record_t *entry)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_drive_configuration_control_get_port_record_t *     api_get_record = NULL;

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    api_get_record = (fbe_drive_configuration_control_get_port_record_t *)fbe_api_allocate_memory(sizeof (fbe_drive_configuration_control_get_port_record_t));

    if (api_get_record == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Can't allocate memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    api_get_record->handle = handle;
    
     status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_GET_PORT_RECORD,
                                                             api_get_record,
                                                             sizeof (fbe_drive_configuration_control_get_port_record_t),
                                                             FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory(api_get_record);
        return FBE_STATUS_GENERIC_FAILURE;
    }

     fbe_copy_memory (entry, &api_get_record->record, sizeof (fbe_drive_configuration_port_record_t));

     fbe_api_free_memory(api_get_record);
     return status;

}


/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_download_firmware(
 *     fbe_drive_configuration_control_fw_info_t *fw_info) 
 *****************************************************************
 * @brief
 *  This function sends download request to DCS. 
 *
 * @param fw_info - pointer for download information.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  11/16/11: chenl6    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL  fbe_api_drive_configuration_interface_download_firmware(fbe_drive_configuration_control_fw_info_t *fw_info)
{

    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_sg_element_t        sg_element[3];  /*header, data, and terminating elem*/
    fbe_u32_t               sg_count;

    if (fw_info == NULL ||
		fw_info->header_image_p == NULL || fw_info->data_image_p == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "fbe_api dcs download. null ptr found.\n"); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* now set up the sg list correctly. Header, Data, then terminating element. 
     */
    fbe_sg_element_init(&sg_element[0], 
                        fw_info->header_size,        
                        fw_info->header_image_p);   
    fbe_sg_element_init(&sg_element[1], 
                        fw_info->data_size,       
                        fw_info->data_image_p);    
    fbe_sg_element_terminate(&sg_element[2]);
    sg_count = 3;

    /* Note, NO_CONTIGUOUS_ALLOCATION_NEEDED is set since DCS will manage breaking up image into physically contingous
       chunks.  */
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_DRIVE_CONFIGURATION_CONTROL_DOWNLOAD_FIRMWARE,
                                                                        NULL,
                                                                        0,
                                                                        FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        sg_element,
                                                                        sg_count,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_abort_download() 
 *****************************************************************
 * @brief
 *  This function sends abort download command to DSC. 
 *  This is a synchronous function
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  11/16/11: chenl6    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_drive_configuration_interface_abort_download(void)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_ABORT_DOWNLOAD,
                                                            NULL,
                                                            0,
                                                            FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_get_download_process(
 *     fbe_drive_configuration_download_get_process_info_t *process_info) 
 *****************************************************************
 * @brief
 *  This function gets download process information from DCS. 
 *
 * @param process_info - pointer for process information.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  11/12/11: chenl6    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL  
fbe_api_drive_configuration_interface_get_download_process(fbe_drive_configuration_download_get_process_info_t *process_info)
{

    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_GET_DOWNLOAD_PROCESS,
                                                            process_info,
                                                            sizeof(fbe_drive_configuration_download_get_process_info_t),
                                                            FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_get_download_process(
 *     fbe_drive_configuration_download_get_drive_info_t *drive_info) 
 *****************************************************************
 * @brief
 *  This function gets download process information from DCS. 
 *
 * @param process_info - pointer for process information.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  11/12/11: chenl6    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL  
fbe_api_drive_configuration_interface_get_download_drive(fbe_drive_configuration_download_get_drive_info_t *drive_info)
{

    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_GET_DOWNLOAD_DRIVE,
                                                            drive_info,
                                                            sizeof(fbe_drive_configuration_download_get_drive_info_t),
                                                            FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_get_download_max_drive_count(
 *     fbe_drive_configuration_control_fw_info_t *fw_info) 
 *****************************************************************
 * @brief
 *  This function gets max concurrent download drive count for ODFU. 
 *
 * @param fw_info - pointer for max download count information.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  05/02/12: created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL  fbe_api_drive_configuration_interface_get_download_max_drive_count(fbe_drive_configuration_get_download_max_drive_count_t *fw_max_dl_count)
{

    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_DRIVE_CONFIGURATION_CONTROL_GET_DOWNLOAD_MAX_DRIVE_COUNT,
                                                            fw_max_dl_count,
                                                            sizeof(fbe_drive_configuration_get_download_max_drive_count_t),
                                                            FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_get_all_download_drives(fbe_drive_configuration_download_get_drive_info_t *drive_info, fbe_u32_t expected_count)
 *****************************************************************
 * @brief
 *  Similar to fbe_api_drive_configuration_interface_get_download_drive but get the info for all at one shot
 *
 * @param drive_info - pointer to start of user allocated buffer, it should be sized at FBE_MAX_DOWNLOAD_DRIVES * sizeof(fbe_drive_configuration_download_get_drive_info_t)
 * @param max_count - usualy must be FBE_MAX_DOWNLOAD_DRIVES in case some drive are in the process of download so we capture all
 * @param actual_drives - How many we returned
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  05/02/12: created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_drive_configuration_interface_get_all_download_drives(fbe_drive_configuration_download_get_drive_info_t *drive_info,
																						fbe_u32_t max_count,
																						fbe_u32_t * actual_drives)
{

	fbe_status_t                              					status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t 					status_info;
	fbe_sg_element_t *                      					sg_list = NULL;
	fbe_drive_configuration_control_get_all_download_drive_t	get_drives;

	if (max_count != FBE_MAX_DOWNLOAD_DRIVES) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: expected_count is %d and not FBE_MAX_DOWNLOAD_DRIVES\n", __FUNCTION__, max_count);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
	if (sg_list == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    /*set up the sg list to point to the list of objects the user wants to get*/
    sg_list[0].address = (fbe_u8_t *)drive_info;
    sg_list[0].count = max_count * sizeof(fbe_drive_configuration_download_get_drive_info_t);
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    status = fbe_api_common_send_control_packet_to_service_with_sg_list (FBE_DRIVE_CONFIGURATION_CONTROL_GET_ALL_DOWNLOAD_DRIVES,
																		 &get_drives,
																		 sizeof(fbe_drive_configuration_control_get_all_download_drive_t),
																		 FBE_SERVICE_ID_DRIVE_CONFIGURATION,
																		 FBE_PACKET_FLAG_NO_ATTRIB,
																		 sg_list,
																		 2,
																		 &status_info,
																		 FBE_PACKAGE_ID_PHYSICAL);

	fbe_api_free_memory (sg_list);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

	*actual_drives = get_drives.actual_returned;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_dieh_force_clear_update(void)
 *****************************************************************
 * @brief
 *  Clears the DCS DIEH update state.  This should be used
 *  if start_update was called and end_update was not, due to detecting
 *  an error.  This resets the DIEH state and allows a subsequent update
 *  to be preformed.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/04/12:  Wayne Garrett - created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_drive_configuration_interface_dieh_force_clear_update(void)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info = {0};

    status = fbe_api_common_send_control_packet_to_service(FBE_DRIVE_CONFIGURATION_CONTROL_DIEH_FORCE_CLEAR_UPDATE,
                                                            NULL,
                                                            0,
                                                            FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_dieh_get_status(void)
 *****************************************************************
 * @brief
 *  Gets status of DIEH table update.  This involves adding new
 *  records and having all PDO clients unregister from old
 *  table.  
 * 
 * @param is_updating - true if table is in process of being updated
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/04/12:  Wayne Garrett - created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_drive_configuration_interface_dieh_get_status(fbe_bool_t *is_updating)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info = {0};

    status = fbe_api_common_send_control_packet_to_service(FBE_DRIVE_CONFIGURATION_CONTROL_DIEH_GET_STATUS,
                                                            is_updating,
                                                            sizeof(fbe_bool_t),
                                                            FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_get_params()
 *****************************************************************
 * @brief
 *  Get the Drive Configuration tunable parameters 
 * 
 * @param params - tunable parameters
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  11/15/12:  Wayne Garrett - created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL  
fbe_api_drive_configuration_interface_get_params(fbe_dcs_tunable_params_t *params)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info = {0};

    status = fbe_api_common_send_control_packet_to_service(FBE_DRIVE_CONFIGURATION_CONTROL_GET_TUNABLE_PARAMETERS,
                                                            params,
                                                            sizeof(fbe_dcs_tunable_params_t),
                                                            FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;    
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_set_params()
 *****************************************************************
 * @brief
 *  Set the Drive Configuration tunable parameters 
 * 
 * @param params - tunable parameters
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  11/15/12:  Wayne Garrett - created
 *
 ****************************************************************/
fbe_status_t  FBE_API_CALL  
fbe_api_drive_configuration_interface_set_params(fbe_dcs_tunable_params_t *params)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info = {0};

    status = fbe_api_common_send_control_packet_to_service(FBE_DRIVE_CONFIGURATION_CONTROL_SET_TUNABLE_PARAMETERS,
                                                            params,
                                                            sizeof(fbe_dcs_tunable_params_t),
                                                            FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK; 
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_get_mode_page_overrides()
 *****************************************************************
 * @brief
 *  Get mode page override table
 * 
 * @param ms_info - contains the mode page override table
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  03/29/13:  Wayne Garrett - created
 *
 ****************************************************************/
fbe_status_t  FBE_API_CALL  
fbe_api_drive_configuration_interface_get_mode_page_overrides(fbe_dcs_mode_select_override_info_t *ms_info)
{
    fbe_status_t                              status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t   status_info = {0};    

    status = fbe_api_common_send_control_packet_to_service(FBE_DRIVE_CONFIGURATION_CONTROL_GET_MODE_PAGE_OVERRIDES,
                                                           ms_info,
                                                           sizeof(fbe_dcs_mode_select_override_info_t),
                                                           FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK; 
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_set_mode_page_byte()
 *****************************************************************
 * @brief
 *  Set a mode page byte in override table.
 * 
 * @param  table_id - ID of specific override table
 * @param  page - mode page
 * @param  byte_offset - byte offset of page
 * @param  mask - identifies which bits to set
 * @param  value - value to or with mask.
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  03/29/13:  Wayne Garrett - created
 *
 ****************************************************************/
fbe_status_t  FBE_API_CALL  
fbe_api_drive_configuration_interface_set_mode_page_byte(fbe_drive_configuration_mode_page_override_table_id_t table_id, fbe_u8_t page, fbe_u8_t byte_offset, fbe_u8_t mask, fbe_u8_t value)
{
    fbe_status_t                              status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t   status_info = {0};
    fbe_dcs_set_mode_page_override_table_entry_t mp_entry;

    mp_entry.table_id = table_id;
    mp_entry.table_entry.page = page;
    mp_entry.table_entry.byte_offset = byte_offset;
    mp_entry.table_entry.mask = mask;
    mp_entry.table_entry.value = value;    

    status = fbe_api_common_send_control_packet_to_service(FBE_DRIVE_CONFIGURATION_CONTROL_SET_MODE_PAGE_BYTE,
                                                           &mp_entry,
                                                           sizeof(mp_entry),
                                                           FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s: mode_page_byte not found.  payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status_info.control_operation_status, status_info.control_operation_qualifier);
        /*TODO: use a different return type to provide reason to caller */
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_OK; 
}


/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_mode_page_addl_override_clear()
 *****************************************************************
 * @brief
 *  Clear the "additional" override table.
 * 
 * @param  none.
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  05/23/14:  Wayne Garrett - created
 *
 ****************************************************************/
fbe_status_t  FBE_API_CALL  
fbe_api_drive_configuration_interface_mode_page_addl_override_clear(void)
{
    fbe_status_t                              status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t   status_info = {0};

    status = fbe_api_common_send_control_packet_to_service(FBE_DRIVE_CONFIGURATION_CONTROL_MODE_PAGE_ADDL_OVERRIDE_CLEAR,
                                                           NULL,  /* no data */
                                                           0,
                                                           FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:  payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_OK; 
}



/*!**************************************************************
 * @fn fbe_api_drive_configuration_set_service_timeout
 ****************************************************************
 * @brief
 *  This function set the service timeout limit for all PDOs.
 * 
 * @param timeout - in milliseconds
 * 
 * @return fbe_status_t 
 *
 * @version
 *  11/15/2012:  Wayne Garrett  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_drive_configuration_set_service_timeout(fbe_time_t timeout)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dcs_tunable_params_t params = {0};

    status = fbe_api_drive_configuration_interface_get_params(&params);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get params.\n", __FUNCTION__);
        return status;
    }

    params.service_time_limit_ms = timeout;

    status = fbe_api_drive_configuration_interface_set_params(&params);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to set params.\n", __FUNCTION__);
        return status;
    }

    return FBE_STATUS_OK;
}

 
/*!**************************************************************
 * @fn fbe_api_drive_configuration_set_remap_service_timeout
 ****************************************************************
 * @brief
 *  This function sets the remap service time limit for all PDOs
 * 
 * @param timeout - in milliseconds
 * 
 * @return fbe_status_t 
 *
 * @version
 *  11/15/2012:  Wayne Garrett  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_drive_configuration_set_remap_service_timeout(fbe_time_t timeout)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dcs_tunable_params_t params = {0};

    status = fbe_api_drive_configuration_interface_get_params(&params);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get params.\n", __FUNCTION__);
        return status;
    }

    params.remap_service_time_limit_ms = timeout;

    status = fbe_api_drive_configuration_interface_set_params(&params);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to set params.\n", __FUNCTION__);
        return status;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_api_drive_configuration_set_fw_image_chunk_size
 ****************************************************************
 * @brief
 *  This function sets the fw image chunk size for breaking up the
 *  imange into multiple sg elements.
 * 
 * @param size - byte size for each sg_element
 * 
 * @return fbe_status_t 
 *
 * @version
 *  07/24/2013:  Wayne Garrett  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_drive_configuration_set_fw_image_chunk_size(fbe_u32_t size)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dcs_tunable_params_t params = {0};

    status = fbe_api_drive_configuration_interface_get_params(&params);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get params.\n", __FUNCTION__);
        return status;
    }

    params.fw_image_chunk_size = size;

    status = fbe_api_drive_configuration_interface_set_params(&params);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to set params.\n", __FUNCTION__);
        return status;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_api_drive_configuration_set_control_flags
 ****************************************************************
 * @brief
 *  This function will overwrite the existing bitmap flags
 * 
 * @param bitmap_flags - bitmap of control flags
 * 
 * @return fbe_status_t 
 *
 * @version
 *  11/15/2012:  Wayne Garrett  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_drive_configuration_set_control_flags(fbe_dcs_control_flags_t bitmap_flags)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dcs_tunable_params_t params = {0};

    status = fbe_api_drive_configuration_interface_get_params(&params);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get params.\n", __FUNCTION__);
        return status;
    }

    params.control_flags = bitmap_flags;

    status = fbe_api_drive_configuration_interface_set_params(&params);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to set params.\n", __FUNCTION__);
        return status;
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_api_drive_configuration_set_control_flag
 ****************************************************************
 * @brief
 *  This function will enable/disable a specific flag or bitmap
 *  of flags.
 * 
 * @param flag - control flag(s) to change
 * @param do_enable - true = enable, false = disable.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  11/15/2012:  Wayne Garrett  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_drive_configuration_set_control_flag(fbe_dcs_control_flags_t flag, fbe_bool_t do_enable)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dcs_tunable_params_t params = {0};

    status = fbe_api_drive_configuration_interface_get_params(&params);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get params.\n", __FUNCTION__);
        return status;
    }

    if (do_enable)
    {
        params.control_flags |= flag;
    }
    else
    {
        params.control_flags &= ~flag;
    }

    status = fbe_api_drive_configuration_interface_set_params(&params);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to set params.\n", __FUNCTION__);
        return status;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_reset_queuing_table() 
 *****************************************************************
 * @brief
 *  This function resets enhanced queuing timer table in DCS. 
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  08/22/2013:  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_drive_configuration_interface_reset_queuing_table(void)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info = {0};
    
    status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_RESET_QUEUING_TABLE,
                                                             NULL,
                                                             0,
                                                             FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
     }

     return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_activate_queuing_table() 
 *****************************************************************
 * @brief
 *  This function activates enhanced queuing timer table in DCS
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  08/22/2013:  Lili Chen  - Created.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_drive_configuration_interface_activate_queuing_table(void)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_ACTIVATE_QUEUING_TABLE,
                                                            NULL,
                                                            0,
                                                            FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
     }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_drive_configuration_interface_add_queuing_entry
 *****************************************************************
 * @brief
 *  This function adds an entry in enhanced queuing timer table in DCS.
 *
 * @param entry - pointer to the data to update.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  08/22/2013:  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL  fbe_api_drive_configuration_interface_add_queuing_entry(fbe_drive_configuration_queuing_record_t *entry)
{

    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_drive_configuration_control_add_queuing_entry_t          add_eq_timer;

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory (&add_eq_timer.queue_entry, entry,  sizeof (fbe_drive_configuration_queuing_record_t));

    status = fbe_api_common_send_control_packet_to_service (FBE_DRIVE_CONFIGURATION_CONTROL_ADD_QUEUING_TABLE_ENTRY,
                                                             &add_eq_timer,
                                                             sizeof (fbe_drive_configuration_control_add_queuing_entry_t),
                                                             FBE_SERVICE_ID_DRIVE_CONFIGURATION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

     if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
