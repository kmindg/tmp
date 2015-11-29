/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_dest_injection_interface.c
  ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_dest_injection interface 
 *  for the neit package.
 *
 * @ingroup fbe_api_neit_package_interface_class_files
 * @ingroup fbe_api_dest_injection_interface
 * TABLE OF CONTENTS
 * fbe_api_dest_injection_init
 * fbe_api_dest_injection_start
 * fbe_api_dest_injection_stop
 * fbe_api_dest_injection_add_record
 * fbe_api_dest_injection_remove_record
 * fbe_api_dest_injection_remove_all_records
 * fbe_api_dest_injection_get_record
 * fbe_api_dest_injection_get_record_handle
 * fbe_api_dest_injection_get_record_next
 * fbe_api_dest_injection_record_update_times_to_insert
 * 
 *
 * NOTES
 *       
 * History:
 *  05/01/2012  Created. kothal
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
#include "fbe/fbe_dest_service.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_api_discovery_interface.h"

/* global counter that's incremented when a new record is added.*/
fbe_u32_t record_counter = 0;

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

/*!***************************************************************
 * @fn fbe_api_dest_injection_init()
 ****************************************************************
 * @brief
 *  This function starts the error injection.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_dest_injection_init(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    status =fbe_dest_init();
    if (status != FBE_STATUS_OK)
	{
		fbe_api_trace (FBE_TRACE_LEVEL_WARNING," %s\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_dest_injection_start()
 **************************************/
/*!***************************************************************
 * @fn fbe_api_dest_injection_start()
 ****************************************************************
 * @brief
 *  This function starts the error injection.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_dest_injection_start(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_DEST_SERVICE_CONTROL_CODE_START,
                                                             NULL,
                                                             0,
                                                             FBE_SERVICE_ID_DEST,
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
 * end fbe_api_dest_injection_start()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_dest_injection_stop()
 ****************************************************************
 * @brief
 *  This function stops the error injection.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_dest_injection_stop(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_DEST_SERVICE_CONTROL_CODE_STOP,
                                                             NULL,
                                                             0,
                                                             FBE_SERVICE_ID_DEST,
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
 * end fbe_api_dest_injection_stop()
 **************************************/
 
/*!***************************************************************
 * @fn fbe_api_dest_injection_add_record(
 *       fbe_dest_error_record_t *new_record,
 *       fbe_dest_record_handle_t * record_handle)
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
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_dest_injection_add_record(fbe_dest_error_record_t *new_record, fbe_dest_record_handle_t * record_handle)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;   
    fbe_dest_control_add_record_t add_record;        
    fbe_port_number_t port = FBE_PORT_NUMBER_INVALID;
    fbe_enclosure_number_t enclosure = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_enclosure_slot_number_t slot = FBE_SLOT_NUMBER_INVALID;
 
    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "DEST: %s entry\n", __FUNCTION__);		
	
    if ((new_record == NULL) || (record_handle == NULL)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer\n", __FUNCTION__);		
        return FBE_STATUS_GENERIC_FAILURE;
    }        

    if (new_record->object_id != FBE_OBJECT_ID_INVALID)
    {
        /* Get bus-enclosure-slot from object, stop attempt if any of them fail. */
        status = fbe_api_get_object_port_number(new_record->object_id, &port);                    
        if (status == FBE_STATUS_OK)
        {                      
            status = fbe_api_get_object_enclosure_number(new_record->object_id, &enclosure);
            if (status == FBE_STATUS_OK)
            {
                status = fbe_api_get_object_drive_number(new_record->object_id, &slot);
            }
        }
      
        if (status == FBE_STATUS_OK)
        {
            /* If we got here then bus-enclosure-slot was retrieved, populate record.*/
            new_record->bus = port;
            new_record->enclosure = enclosure;    
            new_record->slot = slot;
        }
        else
        {
            /* If bus-enclosure-slot retrieval failed set data to invalid.*/
            new_record->bus = FBE_PORT_NUMBER_INVALID;
            new_record->enclosure = FBE_ENCLOSURE_NUMBER_INVALID;    
            new_record->slot = FBE_SLOT_NUMBER_INVALID;
        }
    }
    else
    {
        fbe_api_trace(FBE_TRACE_LEVEL_INFO, "DEST: Invalid Object ID\n");
    }
    
    new_record->record_id = record_counter++;

    fbe_copy_memory(&add_record.dest_error_record, new_record, sizeof(fbe_dest_error_record_t ));
 
    status = fbe_api_common_send_control_packet_to_service (FBE_DEST_SERVICE_CONTROL_CODE_ADD_RECORD,
			 &add_record,
			 sizeof(fbe_dest_control_add_record_t),
			 FBE_SERVICE_ID_DEST,
			 FBE_PACKET_FLAG_NO_ATTRIB,
			 &status_info,
			 FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					   status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *record_handle = add_record.dest_record_handle;
    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "DEST: error record handle %p\n", add_record.dest_record_handle);
    return status;
}
/**************************************
 * end fbe_api_dest_injection_add_record()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_dest_injection_get_record(
 *       fbe_dest_error_record_t *new_record,
 *       fbe_dest_record_handle_t * record_handle)
 ****************************************************************
 * @brief
 *  This function finds the error injection record.
 *
 * @param new_record - pointer to new record info
 * @param record_handle - pointer to record handle
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_dest_injection_get_record(fbe_dest_error_record_t *new_record,
		fbe_dest_record_handle_t record_handle)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_dest_control_add_record_t get_record;
	
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s entry\n", __FUNCTION__);
	
    fbe_copy_memory(&get_record.dest_error_record, new_record, sizeof(fbe_dest_error_record_t));
    fbe_copy_memory(&get_record.dest_record_handle, &record_handle, sizeof(fbe_dest_record_handle_t));
	
    status = fbe_api_common_send_control_packet_to_service (FBE_DEST_SERVICE_CONTROL_CODE_GET_RECORD,
			 &get_record,
			 sizeof(fbe_dest_control_add_record_t),
			 FBE_SERVICE_ID_DEST,
			 FBE_PACKET_FLAG_NO_ATTRIB,
			 &status_info,
			 FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					   status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }	 
	
    *new_record = get_record.dest_error_record;
    return status;
}
/**************************************
 * end fbe_api_dest_injection_get_record()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_dest_injection_get_record_handle()
 ****************************************************************
 * @brief
 *  This function finds the error injection record handle.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_dest_injection_get_record_handle(fbe_dest_error_record_t *record_p , fbe_dest_record_handle_t * record_handle)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_dest_control_add_record_t get_record;

    if ((record_handle == NULL) || (record_p == NULL)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&get_record.dest_error_record, record_p, sizeof(fbe_dest_error_record_t));

    status = fbe_api_common_send_control_packet_to_service (FBE_DEST_SERVICE_CONTROL_CODE_GET_RECORD_HANDLE,
                                                             &get_record,
                                                             sizeof(fbe_dest_control_add_record_t),
                                                             FBE_SERVICE_ID_DEST,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *record_handle = get_record.dest_record_handle; 

    return status;
}
/**************************************
 * end fbe_api_dest_injection_get_record_handle()
 **************************************/

 
/*!***************************************************************
 * @fn fbe_api_dest_injection_search_record()
 ****************************************************************
 * @brief
 * This function finds the error injection record handle of records that match the search criteria.  
 * If record_handle is invalid, then function will return first match. 
 * If you call this a second time with record_handle equal to first match, then it will return second match.
 * If record_handle return invalid, then no more records found.
 *
 * @param record_p - pointer to record info
 *        record_handle - pointer to record handle
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_dest_injection_search_record(fbe_dest_error_record_t *record_p , fbe_dest_record_handle_t * record_handle)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info = {0};
    fbe_dest_control_add_record_t get_record;
 
    if ((record_handle == NULL) || (record_p == NULL)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_copy_memory(&get_record.dest_error_record, record_p, sizeof(fbe_dest_error_record_t));
    fbe_copy_memory(&get_record.dest_record_handle, record_handle, sizeof(fbe_dest_record_handle_t));
 
    status = fbe_api_common_send_control_packet_to_service (FBE_DEST_SERVICE_CONTROL_CODE_SEARCH_RECORD,
             &get_record,
             sizeof(fbe_dest_control_add_record_t),
             FBE_SERVICE_ID_DEST,
             FBE_PACKET_FLAG_NO_ATTRIB,
             &status_info,
             FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {       
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "dest_srch_rec:pkt: err %d,qual %d; pyld err:%d, qual %d\n",
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    *record_handle = get_record.dest_record_handle; 
    *record_p = get_record.dest_error_record;
    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "DEST: error record handle %p\n", get_record.dest_record_handle);
 
    return status;
}
/**************************************
 * end fbe_api_dest_injection_search_record()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_dest_injection_get_record_next()
 ****************************************************************
 * @brief
 *  This function finds the error injection record handle.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_dest_injection_get_record_next(fbe_dest_error_record_t *record_p , fbe_dest_record_handle_t * record_handle)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_dest_control_add_record_t get_record;
	
    if ((record_handle == NULL) || (record_p == NULL)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
    fbe_copy_memory(&get_record.dest_error_record, record_p, sizeof(fbe_dest_error_record_t));
    fbe_copy_memory(&get_record.dest_record_handle, record_handle, sizeof(fbe_dest_record_handle_t));
	
    status = fbe_api_common_send_control_packet_to_service (FBE_DEST_SERVICE_CONTROL_CODE_GET_RECORD_NEXT,
			 &get_record,
			 sizeof(fbe_dest_control_add_record_t),
			 FBE_SERVICE_ID_DEST,
			 FBE_PACKET_FLAG_NO_ATTRIB,
			 &status_info,
			 FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					   status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
    *record_p = get_record.dest_error_record;
    *record_handle = get_record.dest_record_handle;

    return status;
}
/**************************************
 * end fbe_api_dest_injection_get_record_next()
 **************************************/
/*!***************************************************************
 * @fn fbe_api_dest_injection_remove_record(
    fbe_dest_record_handle_t * record_handle)
 ****************************************************************
 * @brief
 *  This function removes a record.
 *
 * @param record_handle - record handle structure
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_dest_injection_remove_record(fbe_dest_record_handle_t * record_handle)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_dest_control_remove_record_t   remove_record;

    remove_record.dest_record_handle = record_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_DEST_SERVICE_CONTROL_CODE_REMOVE_RECORD,
                                                             &remove_record,
                                                             sizeof(fbe_dest_control_remove_record_t),
                                                             FBE_SERVICE_ID_DEST,
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
 * end fbe_api_dest_injection_remove_record()
 **************************************/
/*!***************************************************************
 * @fn fbe_api_dest_injection_remove_all_records(
    fbe_dest_record_handle_t * record_handle)
 ****************************************************************
 * @brief
 *  This function removes a record.
 *
 * @param record_handle - record handle structure
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_dest_injection_remove_all_records(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
  
    status = fbe_api_common_send_control_packet_to_service (FBE_DEST_SERVICE_CONTROL_CODE_REMOVE_ALL_RECORDS,
                                                             NULL,
                                                             0,
                                                             FBE_SERVICE_ID_DEST,
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
 * end fbe_api_dest_injection_remove_all_records()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_dest_injection_record_update_times_to_insert()
 ****************************************************************
 * @brief
 *  This function updates the error injection record.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_dest_injection_record_update_times_to_insert(fbe_dest_record_handle_t *record_handle, fbe_dest_error_record_t *record_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_dest_control_add_record_t   update_record;

    if (record_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&update_record.dest_error_record, record_p, sizeof(fbe_dest_error_record_t));
    fbe_copy_memory(&update_record.dest_record_handle, record_handle, sizeof(fbe_dest_record_handle_t));	

    status = fbe_api_common_send_control_packet_to_service (FBE_DEST_SERVICE_CONTROL_CODE_RECORD_UPDATE_TIMES_TO_INSERT,
                                                             &update_record,
                                                             sizeof(fbe_dest_control_add_record_t),
                                                             FBE_SERVICE_ID_DEST,
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
 * end fbe_api_dest_injection_record_update_times_to_insert()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_dest_injection_get_state()
 ****************************************************************
 * @brief
 *  This function gets the state of DEST.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_dest_injection_get_state(fbe_dest_state_type *dest_state)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_dest_state_type fbe_dest_state;

    status = fbe_api_common_send_control_packet_to_service (FBE_DEST_SERVICE_CONTROL_CODE_GET_STATE,
                                                            &fbe_dest_state,
                                                             sizeof(fbe_dest_state_type),
                                                             FBE_SERVICE_ID_DEST,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *dest_state = fbe_dest_state;
    return status;
}

/*!***************************************************************
 * @fn fbe_api_dest_injection_init_scsi_error_record()
 ****************************************************************
 * @brief
 *  Utility function for initialize a scsi error record.
 * 
 * @param  record - record to init
 * @return
 *  none
 * 
 * @author  Wayne Garrett - Created.
 *
 ****************************************************************/
void FBE_API_CALL  fbe_api_dest_injection_init_scsi_error_record(fbe_dest_error_record_t *record_p)
{
    fbe_zero_memory(record_p, sizeof(fbe_dest_error_record_t));

    record_p->object_id = FBE_OBJECT_ID_INVALID;

    record_p->lba_start = 0;
    record_p->lba_end = FBE_LBA_INVALID;

    record_p->dest_error_type = FBE_DEST_ERROR_TYPE_SCSI;
    record_p->dest_error.dest_scsi_error.scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
    record_p->dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    record_p->valid_lba = FBE_TRUE;
    record_p->num_of_times_to_insert = 0;
    record_p->react_gap_type = FBE_DEST_REACT_GAP_NONE;
}

/**************************************
 * end ffbe_api_dest_injection_get_state()
 **************************************/
