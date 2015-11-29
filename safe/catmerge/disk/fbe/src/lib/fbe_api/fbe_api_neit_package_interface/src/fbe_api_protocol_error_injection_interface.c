/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_protocol_error_injection_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_protocol_error_injection interface 
 *  for the neit package.
 *
 * @ingroup fbe_api_neit_package_interface_class_files
 * @ingroup fbe_api_protocol_error_injection_interface
 *
 * @version
 *   11/25/2009:  Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_protocol_error_injection_service.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/


/*!***************************************************************
 * @fn fbe_api_protocol_error_injection_add_record(
 *       fbe_protocol_error_injection_error_record_t *new_record,
 *       fbe_protocol_error_injection_record_handle_t * record_handle)
 ****************************************************************
 * @brief
 *  This function adds a record.
 *
 * @param new_record - pointer to new record info
 * @param record_handle - pointer to record handle
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/25/09 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_protocol_error_injection_add_record(fbe_protocol_error_injection_error_record_t *new_record,
                                            fbe_protocol_error_injection_record_handle_t * record_handle)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_protocol_error_injection_control_add_record_t   add_record;

    if ((new_record == NULL) || (record_handle == NULL)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_copy_memory(&add_record.protocol_error_injection_error_record, new_record, sizeof(fbe_protocol_error_injection_error_record_t));

    status = fbe_api_common_send_control_packet_to_service (FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ADD_RECORD,
                                                        &add_record,
                                                        sizeof(fbe_protocol_error_injection_control_add_record_t),
                                                        FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                                                        FBE_PACKET_FLAG_NO_ATTRIB,
                                                        &status_info,
                                                        FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *record_handle = add_record.protocol_error_injection_record_handle;
    return status;
}
/**************************************
 * end fbe_api_protocol_error_injection_add_record()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_protocol_error_injection_remove_record(
 *     fbe_protocol_error_injection_record_handle_t record_handle)
 ****************************************************************
 * @brief
 *  This function removes a record.
 *
 * @param record_handle - record handle structure
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/25/09 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_protocol_error_injection_remove_record(fbe_protocol_error_injection_record_handle_t record_handle)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_protocol_error_injection_control_remove_record_t    remove_record;

    remove_record.protocol_error_injection_record_handle = record_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_RECORD,
                                                             &remove_record,
                                                             sizeof(fbe_protocol_error_injection_control_remove_record_t),
                                                             FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_protocol_error_injection_remove_record()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_protocol_error_injection_remove_object(
 *       fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function removes the records of a object.
 *
 * @param object_id - object ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/25/09 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_protocol_error_injection_remove_object(fbe_object_id_t object_id)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_protocol_error_injection_control_remove_object_t    remove_object;

    remove_object.object_id = object_id;
    status = fbe_api_common_send_control_packet_to_service (FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_OBJECT,
                                                             &remove_object,
                                                             sizeof(fbe_protocol_error_injection_control_remove_object_t),
                                                             FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_protocol_error_injection_remove_object()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_protocol_error_injection_start()
 ****************************************************************
 * @brief
 *  This function starts the error injection.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/25/09 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_protocol_error_injection_start(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_START,
                                                             NULL,
                                                             0,
                                                             FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_protocol_error_injection_start()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_protocol_error_injection_stop()
 ****************************************************************
 * @brief
 *  This function stops the error injection.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/25/09 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_protocol_error_injection_stop(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_STOP,
                                                             NULL,
                                                             0,
                                                             FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_protocol_error_injection_stop()
 **************************************/
/*!***************************************************************
 * @fn fbe_api_protocol_error_injection_get_record()
 ****************************************************************
 * @brief
 *  This function finds the error injection record.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/12/10 - Created. Vinay Patki
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_protocol_error_injection_get_record(fbe_protocol_error_injection_record_handle_t handle_p,
fbe_protocol_error_injection_error_record_t * record_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_protocol_error_injection_control_add_record_t   get_record;

    if ((handle_p == NULL) || (record_p == NULL)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&get_record.protocol_error_injection_error_record, record_p, sizeof(fbe_protocol_error_injection_error_record_t));
    fbe_copy_memory(&get_record.protocol_error_injection_record_handle, &handle_p, sizeof(fbe_protocol_error_injection_record_handle_t));

    status = fbe_api_common_send_control_packet_to_service (FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORD,
                                                             &get_record,
                                                             sizeof(fbe_protocol_error_injection_control_add_record_t),
                                                             FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *record_p = get_record.protocol_error_injection_error_record;

    return status;
}
/**************************************
 * end fbe_api_protocol_error_injection_get_record()
 **************************************/
/*!***************************************************************
 * @fn fbe_api_protocol_error_injection_get_record_next()
 ****************************************************************
 * @brief
 *  This function finds the error injection record.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/12/10 - Created. Vinay Patki
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_protocol_error_injection_get_record_next(fbe_protocol_error_injection_record_handle_t *handle_p,
fbe_protocol_error_injection_error_record_t * record_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_protocol_error_injection_control_add_record_t   get_record;

    if ((handle_p == NULL) || (record_p == NULL)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&get_record.protocol_error_injection_error_record, record_p, sizeof(fbe_protocol_error_injection_error_record_t));
    fbe_copy_memory(&get_record.protocol_error_injection_record_handle, handle_p, sizeof(fbe_protocol_error_injection_record_handle_t));

    status = fbe_api_common_send_control_packet_to_service (FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORD_NEXT,
                                                             &get_record,
                                                             sizeof(fbe_protocol_error_injection_control_add_record_t),
                                                             FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *record_p = get_record.protocol_error_injection_error_record;
    *handle_p = get_record.protocol_error_injection_record_handle;

    return status;
}
/**************************************
 * end fbe_api_protocol_error_injection_get_record_next()
 **************************************/
/*!***************************************************************
 * @fn fbe_api_protocol_error_injection_get_record_handle()
 ****************************************************************
 * @brief
 *  This function finds the error injection record handle.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/12/10 - Created. Vinay Patki
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_protocol_error_injection_get_record_handle(fbe_protocol_error_injection_error_record_t* record_p, fbe_protocol_error_injection_record_handle_t* record_handle)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_protocol_error_injection_control_add_record_t   get_record;

    if ((record_handle == NULL) || (record_p == NULL)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&get_record.protocol_error_injection_error_record, record_p, sizeof(fbe_protocol_error_injection_error_record_t));

    status = fbe_api_common_send_control_packet_to_service (FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORD_HANDLE,
                                                             &get_record,
                                                             sizeof(fbe_protocol_error_injection_control_add_record_t),
                                                             FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *record_handle = get_record.protocol_error_injection_record_handle;

    return status;
}
/**************************************
 * end fbe_api_protocol_error_injection_get_record_handle()
 **************************************/
/*!***************************************************************
 * @fn fbe_api_protocol_error_injection_record_update_times_to_insert()
 ****************************************************************
 * @brief
 *  This function updates the error injection record.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  08/10/10 - Created. Vinay Patki
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_protocol_error_injection_record_update_times_to_insert(fbe_protocol_error_injection_record_handle_t *record_handle, fbe_protocol_error_injection_error_record_t *error_record)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_protocol_error_injection_control_add_record_t   update_record;

    if (error_record == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&update_record.protocol_error_injection_error_record, error_record, sizeof(fbe_protocol_error_injection_error_record_t));
    fbe_copy_memory(&update_record.protocol_error_injection_record_handle, record_handle, sizeof(fbe_protocol_error_injection_record_handle_t));

    status = fbe_api_common_send_control_packet_to_service (FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_RECORD_UPDATE_TIMES_TO_INSERT,
                                                             &update_record,
                                                             sizeof(fbe_protocol_error_injection_control_add_record_t),
                                                             FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/**************************************
 * end fbe_api_protocol_error_injection_record_update_times_to_insert()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_protocol_error_injection_remove_all_records(void)
 ****************************************************************
 * @brief
 *  This function removes all existing records in the system
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_protocol_error_injection_remove_all_records(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    
    status = fbe_api_common_send_control_packet_to_service (FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_ALL_RECORDS,
                                                             NULL,
                                                             0,
                                                             FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_NEIT);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/**************************************
 * end fbe_api_protocol_error_injection_remove_all_records()
 **************************************/


/*************************
 * end file fbe_api_protocol_error_injection_interface.c
 *************************/
