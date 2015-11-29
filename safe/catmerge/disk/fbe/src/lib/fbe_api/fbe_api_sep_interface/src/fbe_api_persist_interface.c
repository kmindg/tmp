/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_persist_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_persist interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_persist_interface
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
#include "fbe/fbe_api_persist_interface.h"
#include "fbe/fbe_api_utils.h"

/*locals*/
static fbe_status_t fbe_api_persist_commit_transaction_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_persist_set_lun_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_persist_read_sector_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_persist_read_entry_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_persist_write_single_entry_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_persist_modify_single_entry_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_persist_delete_single_entry_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);

/*************************
 *   FUNCTION DEFINITIONS
 ***********************/

/*!***************************************************************
   @fn fbe_api_persist_set_lun(fbe_object_id_t lu_object_id,
							   fbe_persist_completion_function_t completion_function,
							   fbe_persist_completion_context_t completion_context)
 ****************************************************************
 * @brief
 *  This function sets the LU the persist service will use. It is ASYNCHRONOUS
 *  and will complete only one the persist serivce has set the LU and checked
 *  the persistence elements and other things it needs to do upon startup.
 *
 * @param lu_object_id      	- The LU we will use to persist the data into
 * @param completion_function 	- completion function to call 
 * @param completion_context    - context for completion
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_set_lun(fbe_object_id_t lu_object_id,
												fbe_persist_completion_function_t completion_function,
												fbe_persist_completion_context_t completion_context)
{
    fbe_persist_control_set_lun_t *             set_lun = NULL;
	fbe_packet_t *								packet;
	fbe_status_t								status;

	if (lu_object_id >= FBE_OBJECT_ID_INVALID) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s invalid object id: %d\n", __FUNCTION__,lu_object_id);
		return FBE_STATUS_GENERIC_FAILURE;

	}

	if (completion_function == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a valid completion function\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

	packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }


	set_lun = (fbe_persist_control_set_lun_t *)fbe_api_allocate_memory(sizeof(fbe_persist_control_set_lun_t));
	if (set_lun == NULL) {
		fbe_api_return_contiguous_packet(packet);
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for operation\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

    set_lun->lun_object_id = lu_object_id;
	set_lun->completion_function = completion_function;
	set_lun->completion_context = completion_context;

    status =  fbe_api_common_send_control_packet_to_service_asynch (packet,
																   FBE_PERSIST_CONTROL_CODE_SET_LUN,
																   set_lun, 
																   sizeof(fbe_persist_control_set_lun_t),
																   FBE_SERVICE_ID_PERSIST,
																   FBE_PACKET_FLAG_NO_ATTRIB,
																   fbe_api_persist_set_lun_completion,
																   set_lun,
                                                                   FBE_PACKAGE_ID_SEP_0);

	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		 fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
		 fbe_api_return_contiguous_packet(packet);
		 fbe_api_free_memory(set_lun);
	}

	return status;

 
}

/*!***************************************************************
   @fn fbe_api_persist_start_transaction(fbe_persist_transaction_handle_t *transaction_handle)
 ****************************************************************
 * @brief
 *  This function starts a transaction in the persist service
 *
 * @param transaction_handle      - return handle to user
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_start_transaction(fbe_persist_transaction_handle_t *transaction_handle)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_persist_control_transaction_t			transaction;

	if (transaction_handle == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s NULL pointer\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;

	}

    transaction.tran_op_type = FBE_PERSIST_TRANSACTION_OP_START;
    transaction.caller_package = FBE_PACKAGE_ID_INVALID;

    status = fbe_api_common_send_control_packet_to_service (FBE_PERSIST_CONTROL_CODE_TRANSACTION,
															&transaction, 
															sizeof(fbe_persist_control_transaction_t),
															FBE_SERVICE_ID_PERSIST,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_BUSY && status != FBE_STATUS_OK && status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
			return status;
		}else{
			return FBE_STATUS_GENERIC_FAILURE;
		}
    }

	*transaction_handle = transaction.tran_handle;

    return status;

}

/*!***************************************************************
   @fn fbe_api_persist_abort_transaction(fbe_persist_transaction_handle_t transaction_handle)
 ****************************************************************
 * @brief
 *  This function aborts a transaction in progress *
 * @param transaction_handle      - handle of transaction to abort
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_abort_transaction(fbe_persist_transaction_handle_t transaction_handle)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_persist_control_transaction_t			transaction;


	transaction.tran_handle = transaction_handle;
    transaction.tran_op_type = FBE_PERSIST_TRANSACTION_OP_ABORT;
    transaction.caller_package = FBE_PACKAGE_ID_INVALID;

    status = fbe_api_common_send_control_packet_to_service (FBE_PERSIST_CONTROL_CODE_TRANSACTION,
															&transaction, 
															sizeof(fbe_persist_control_transaction_t),
															FBE_SERVICE_ID_PERSIST,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
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
   @fn fbe_api_persist_commit_transaction(fbe_persist_transaction_handle_t transaction_handle,
															 fbe_persist_completion_function_t completion_function,
															 fbe_persist_completion_context_t completion_context)
 ****************************************************************
 * @brief
 *  This function COMMITS a transaction in progress 
 * @param transaction_handle      - handle of transaction to commit
 * @param completion_function      - function to call when we complete
 * @param completion_context      - context to pass to completion
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_commit_transaction(fbe_persist_transaction_handle_t transaction_handle,
															 fbe_persist_completion_function_t completion_function,
															 fbe_persist_completion_context_t completion_context)
{
    fbe_persist_control_transaction_t *			transaction = NULL;
	fbe_packet_t *								packet = NULL;
	fbe_status_t								status;

	if (completion_function == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a valid completion function\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

	packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }


	transaction = (fbe_persist_control_transaction_t *) fbe_api_allocate_memory (sizeof (fbe_persist_control_transaction_t));
    if (transaction == NULL) {
		fbe_api_return_contiguous_packet(packet);
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for transaction info\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

	transaction->tran_handle = transaction_handle;
    transaction->tran_op_type = FBE_PERSIST_TRANSACTION_OP_COMMIT;
	transaction->completion_function = completion_function;
	transaction->completion_context = completion_context;
    transaction->caller_package = FBE_PACKAGE_ID_INVALID;

    status =  fbe_api_common_send_control_packet_to_service_asynch (packet,
																   FBE_PERSIST_CONTROL_CODE_TRANSACTION,
																   transaction, 
																   sizeof(fbe_persist_control_transaction_t),
																   FBE_SERVICE_ID_PERSIST,
																   FBE_PACKET_FLAG_NO_ATTRIB,
																   fbe_api_persist_commit_transaction_completion,
																   transaction,
                                                                   FBE_PACKAGE_ID_SEP_0);

	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		 fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
		 /*we release all resources upon completion which we must get from the infrastructure*/
	}

	return status;
}

static fbe_status_t fbe_api_persist_commit_transaction_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
	fbe_persist_control_transaction_t *		transaction = (fbe_persist_control_transaction_t *)context;
	fbe_status_t							packet_status = fbe_transport_get_status_code(packet);
	fbe_payload_control_status_t			control_status;
	fbe_payload_control_operation_t *   	control_operation = NULL;
	fbe_status_t							callback_status = FBE_STATUS_OK;
	fbe_payload_ex_t *                		sep_payload = NULL;

    sep_payload = fbe_transport_get_payload_ex (packet);
	control_operation = fbe_payload_ex_get_control_operation(sep_payload);
	fbe_payload_control_get_status(control_operation, &control_status);

	/*did we have any errors ?*/
	if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
		callback_status = FBE_STATUS_GENERIC_FAILURE;
	}

	/*and call the user back*/
	transaction->completion_function(callback_status, transaction->completion_context);

    fbe_api_return_contiguous_packet(packet);
	fbe_api_free_memory(transaction);

	return FBE_STATUS_OK;
}

/*!***************************************************************
   @fn fbe_api_persist_write_entry(fbe_persist_transaction_handle_t transaction_handle,
													  fbe_persist_sector_type_t target_sector,
													  fbe_u8_t *data,
													  fbe_u32_t data_length,
													  fbe_persist_entry_id_t *entry_id)
 ****************************************************************
 * @brief
 *  This function writes a data element into the transaction
 * @param transaction_handle      - handle of transaction to write into
 * @param target_sector - which sector we do the write to
 * @param entry_id      - a handle to know how to acess this entry next time
 * @param data      - data location
 * @param data_length      - how long is the data
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_write_entry(fbe_persist_transaction_handle_t transaction_handle,
													  fbe_persist_sector_type_t target_sector,
													  fbe_u8_t *data,
													  fbe_u32_t data_length,
													  fbe_persist_entry_id_t *entry_id)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
	fbe_persist_control_write_entry_t			write_entry;
	fbe_sg_element_t							sg_element[2];

    write_entry.tran_handle = transaction_handle;
	write_entry.target_sector = target_sector;

	fbe_sg_element_init(&sg_element[0], data_length, data);
	fbe_sg_element_terminate(&sg_element[1]);

	status = fbe_api_common_send_control_packet_to_service_with_sg_list (FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY,
																		 &write_entry, 
																		 sizeof(fbe_persist_control_write_entry_t),
																		 FBE_SERVICE_ID_PERSIST,
																		 FBE_PACKET_FLAG_NO_ATTRIB,
																		 sg_element,
																		 2,
																		 &status_info,
																		 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
			return status;
		}else{
			return FBE_STATUS_GENERIC_FAILURE;
		}
    }

	*entry_id = write_entry.entry_id;

    return status;
}

/*!***************************************************************
   @fn fbe_api_persist_modify_entry(fbe_persist_transaction_handle_t transaction_handle,
									fbe_u8_t *data,
									fbe_u32_t data_length,
									fbe_persist_entry_id_t entry_id)
 ****************************************************************
 * @brief
 *  This function modify an existing element
 * @param transaction_handle      - handle of transaction to write into
 * @param entry_id      - a handle into the handle to modify
 * @param data      - data location
 * @param data_length      - how long is the data
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_modify_entry(fbe_persist_transaction_handle_t transaction_handle,
													   fbe_u8_t *data,
													   fbe_u32_t data_length,
													   fbe_persist_entry_id_t entry_id)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
	fbe_persist_control_modify_entry_t			modify_entry;
	fbe_sg_element_t							sg_element[2];

    modify_entry.tran_handle = transaction_handle;
	modify_entry.entry_id = entry_id;

	fbe_sg_element_init(&sg_element[0], data_length, data);
	fbe_sg_element_terminate(&sg_element[1]);

	status = fbe_api_common_send_control_packet_to_service_with_sg_list (FBE_PERSIST_CONTROL_CODE_MODIFY_ENTRY,
																		 &modify_entry, 
																		 sizeof(fbe_persist_control_modify_entry_t),
																		 FBE_SERVICE_ID_PERSIST,
																		 FBE_PACKET_FLAG_NO_ATTRIB,
																		 sg_element,
																		 2,
																		 &status_info,
																		 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
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
   @fn fbe_status_t FBE_API_CALL fbe_api_persist_delete_entry(fbe_persist_transaction_handle_t transaction_handle,
													   fbe_persist_entry_id_t entry_id)
 ****************************************************************
 * @brief
 *  This function deletes an existing element
 * @param transaction_handle      - handle of transaction to write into
 * @param entry_id      - a handle into the handle to delete
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_delete_entry(fbe_persist_transaction_handle_t transaction_handle,
													   fbe_persist_entry_id_t entry_id)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
	fbe_persist_control_delete_entry_t			delete_entry;
    
    delete_entry.tran_handle = transaction_handle;
	delete_entry.entry_id = entry_id;

	status = fbe_api_common_send_control_packet_to_service (FBE_PERSIST_CONTROL_CODE_DELETE_ENTRY,
															&delete_entry, 
															sizeof(fbe_persist_control_delete_entry_t),
															FBE_SERVICE_ID_PERSIST,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
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

static fbe_status_t fbe_api_persist_set_lun_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
	fbe_persist_control_set_lun_t *			set_lun = (fbe_persist_control_set_lun_t *)context;
	fbe_status_t							packet_status = fbe_transport_get_status_code(packet);
	fbe_payload_control_status_t			control_status;
	fbe_payload_control_operation_t *   	control_operation = NULL;
	fbe_status_t							callback_status = FBE_STATUS_OK;
	fbe_payload_ex_t *                		sep_payload = NULL;

    sep_payload = fbe_transport_get_payload_ex (packet);
	control_operation = fbe_payload_ex_get_control_operation(sep_payload);
	fbe_payload_control_get_status(control_operation, &control_status);

	/*did we have any errors ?*/
	if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
		callback_status = FBE_STATUS_GENERIC_FAILURE;
	}

	/*and call the user back*/
	set_lun->completion_function(callback_status, set_lun->completion_context);

    fbe_api_return_contiguous_packet(packet);
	fbe_api_free_memory(set_lun);

	return FBE_STATUS_OK;

}

/*!***************************************************************
   @fn fbe_status_t FBE_API_CALL fbe_api_persist_read_sector(fbe_persist_sector_type_t target_sector,
														   fbe_u8_t *read_buffer,
														    fbe_u32_t data_length,
														   fbe_persist_entry_id_t start_entry_id,
														   fbe_persist_read_multiple_entries_completion_function_t completion_function,
														   fbe_persist_completion_context_t completion_context)
 ****************************************************************
 * @brief
 *  This function read 400 entries at once from the sector selected.
 *  It is asynchronous and the user must pass a buffer in the size of FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE
 *  Once received, the buffer will contains 400 entries and each of them will have fbe_persist_user_header_t
 *  in front of it which the user can use later to get information on how to modify or delete this entry.
 *  The callback is of type fbe_persist_read_multiple_entries_completion_function_t which will also return the next entry to
 *  start the read from or FBE_PERSIST_NO_MORE_ENTRIES_TO_READ if all reads are done. It will also return the number of entries
 *  read so the user can tell how much of the buffer he passed has relevent information.
 *  The user needs to know that some of the entries in his buffer will have empty entries which are spots
 *  in the sector that were either deleted or never used. In any case, the whole sector is read.
 *
 * @param target_sector      - from which sector to read
 * @param read_buffer      - buffer to read to, does not have to be physically contigous, but must be non paged
 * @param data_length  - lenght of the data buffer, must be FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE
 * @param start_entry_id - whete to start reading from. On first read, use FBE_PERSIST_SECTOR_START_ENTRY_ID, after that supply the value in next_start_entry_id
 * @param completion_function   - function to call when we complete
 * @param completion_context      - context to pass to completion
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_read_sector(fbe_persist_sector_type_t target_sector,
													   fbe_u8_t *read_buffer,/*must be of size FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE*/
													   fbe_u32_t data_length,
													   fbe_persist_entry_id_t start_entry_id,
													   fbe_persist_read_multiple_entries_completion_function_t completion_function,
													   fbe_persist_completion_context_t completion_context)
{
	fbe_status_t                                status;
    fbe_persist_control_read_sector_t *			read_sector;
	fbe_sg_element_t *							sg_element, *sg_element_head;
	fbe_packet_t *								packet;

	if (data_length != FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a size of 0x%llu bytes\n", __FUNCTION__, (unsigned long long)FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE); 
        return FBE_STATUS_GENERIC_FAILURE;

	}

    if (completion_function == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a valid completion function\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

	packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }


	read_sector = (fbe_persist_control_read_sector_t *)fbe_api_allocate_memory(sizeof(fbe_persist_control_read_sector_t));
	if (read_sector == NULL) {
		fbe_api_return_contiguous_packet(packet);
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for operation\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

	sg_element = sg_element_head = (fbe_sg_element_t *)fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
	if (sg_element == NULL) {
		fbe_api_return_contiguous_packet(packet);
		fbe_api_free_memory(read_sector);
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for dg list\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

    /*init the sg list*/
	fbe_sg_element_init(sg_element++, FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE, read_buffer);
	fbe_sg_element_terminate(sg_element);

	/*init ther stuff*/
	read_sector->completion_context = completion_context;
	read_sector->completion_function = completion_function;
	read_sector->start_entry_id = start_entry_id;
	read_sector->sector_type = target_sector;

    status = fbe_api_common_send_control_packet_to_service_with_sg_list_asynch (packet,
																				FBE_PERSIST_CONTROL_CODE_READ_SECTOR,
																				read_sector, 
																				sizeof(fbe_persist_control_read_sector_t),
																				FBE_SERVICE_ID_PERSIST,
																				sg_element_head,
																				2,
																				fbe_api_persist_read_sector_completion,
																				read_sector,
																				FBE_PACKAGE_ID_SEP_0);

	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		 fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s error in sending packet, status:%d\n", __FUNCTION__, status);
		 /*no need to destory here, we do it on bad completion*/
	}

    return status;

}

static fbe_status_t fbe_api_persist_read_sector_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
	fbe_persist_control_read_sector_t *			read_sector = (fbe_persist_control_read_sector_t *)context;
	fbe_status_t								packet_status = fbe_transport_get_status_code(packet);
	fbe_payload_control_status_t				control_status;
	fbe_payload_control_operation_t *   		control_operation = NULL;
	fbe_status_t								callback_status = FBE_STATUS_OK;
	fbe_payload_ex_t *                			sep_payload = NULL;
	fbe_sg_element_t *							sg_element = NULL;
	fbe_u32_t									count;

    sep_payload = fbe_transport_get_payload_ex (packet);
	control_operation = fbe_payload_ex_get_control_operation(sep_payload);
	fbe_payload_control_get_status(control_operation, &control_status);
	fbe_payload_ex_get_sg_list(sep_payload, &sg_element, &count);

	/*did we have any errors ?*/
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
        if (packet_status == FBE_STATUS_BUSY)
        {
            callback_status = FBE_STATUS_BUSY;
        }
        else
        {
            callback_status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

	/*and call the user back*/
	read_sector->completion_function(callback_status,
									 read_sector->next_entry_id,
									 read_sector->entries_read,
									 read_sector->completion_context);

    fbe_api_return_contiguous_packet(packet);
	fbe_api_free_memory(read_sector);
	fbe_api_free_memory(sg_element);

	return FBE_STATUS_OK;

}

/*!***************************************************************
   @fn fbe_status_t FBE_API_CALL fbe_api_persist_read_single_entry(fbe_persist_entry_id_t entry_id,
															fbe_u8_t *read_buffer,
															fbe_u32_t data_length,
                                                            fbe_persist_completion_function_t completion_function,
															fbe_persist_completion_context_t completion_context)
 ****************************************************************
 * @brief
 *  This function read a singe entry from the persistence drive. It is asynchronous and the read_buffer
 *  must be from the non paged pool (does not have to be physically contiguos.
 *
 * @param entry_id      - from which entry to read
 * @param read_buffer      - buffer to read to, does not have to be physically contigous, but must be non paged
 * @param data_length  - lenght of the data buffer, must be FBE_PERSIST_DATA_BYTES_PER_ENTRY
 * @param completion_function   - function to call when we complete
 * @param completion_context      - context to pass to completion
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_read_single_entry(fbe_persist_entry_id_t entry_id,
															fbe_u8_t *read_buffer,
															fbe_u32_t data_length,/*MUST BE size of FBE_PERSIST_DATA_BYTES_PER_ENTRY*/
															fbe_persist_single_read_completion_function_t completion_function,
															fbe_persist_completion_context_t completion_context)
{

	fbe_status_t                                status;
    fbe_sg_element_t *							sg_element, *sg_element_head;
	fbe_packet_t *								packet;
	fbe_persist_control_read_entry_t *			read_entry = NULL;

	if (data_length != FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a size of 0x%d bytes\n", __FUNCTION__, FBE_PERSIST_DATA_BYTES_PER_ENTRY); 
		return FBE_STATUS_GENERIC_FAILURE;

	}

	if (completion_function == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a valid completion function\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	packet = fbe_api_get_contiguous_packet();
	if (packet == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}


    read_entry = (fbe_persist_control_read_entry_t *)fbe_api_allocate_memory(sizeof(fbe_persist_control_read_entry_t));
	if (read_entry == NULL) {
		fbe_api_return_contiguous_packet(packet);
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for operation\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

    sg_element = sg_element_head = (fbe_sg_element_t *)fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
	if (sg_element == NULL) {
		fbe_api_return_contiguous_packet(packet);
		fbe_api_free_memory(read_entry);
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for dg list\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*init the sg list*/
	fbe_sg_element_init(sg_element++, FBE_PERSIST_DATA_BYTES_PER_ENTRY, read_buffer);
	fbe_sg_element_terminate(sg_element);

	/*init ther stuff*/
	read_entry->completion_context = completion_context;
	read_entry->completion_function = completion_function;
	read_entry->entry_id = entry_id;

	status = fbe_api_common_send_control_packet_to_service_with_sg_list_asynch (packet,
																				FBE_PERSIST_CONTROL_CODE_READ_ENTRY,
																				read_entry, 
																				sizeof(fbe_persist_control_read_entry_t),
																				FBE_SERVICE_ID_PERSIST,
																				sg_element_head,
																				2,
																				fbe_api_persist_read_entry_completion,
																				read_entry,
																				FBE_PACKAGE_ID_SEP_0);

	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		 fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
		 /*no need to destoy resources here, the packet will be completed all the time*/
	}

	return status;
}

static fbe_status_t fbe_api_persist_read_entry_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
	fbe_persist_control_read_entry_t *		read_entry = (fbe_persist_control_read_entry_t *)context;
	fbe_status_t							packet_status = fbe_transport_get_status_code(packet);
	fbe_payload_control_status_t			control_status;
	fbe_payload_control_operation_t *   	control_operation = NULL;
	fbe_status_t							callback_status = FBE_STATUS_OK;
	fbe_payload_ex_t *                		sep_payload = NULL;
	fbe_sg_element_t *						sg_element = NULL;
	fbe_u32_t								count;

    sep_payload = fbe_transport_get_payload_ex (packet);
	control_operation = fbe_payload_ex_get_control_operation(sep_payload);
	fbe_payload_control_get_status(control_operation, &control_status);
	fbe_payload_ex_get_sg_list(sep_payload, &sg_element, &count);

	/*did we have any errors ?*/
	if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
		callback_status = FBE_STATUS_GENERIC_FAILURE;
	}

	/*and call the user back*/
	read_entry->completion_function(callback_status, read_entry->completion_context, read_entry->next_entry_id);

    fbe_api_return_contiguous_packet(packet);
	fbe_api_free_memory(read_entry);
	fbe_api_free_memory(sg_element);

	return FBE_STATUS_OK;

}


/*!***************************************************************
   @fn fbe_status_t FBE_API_CALL fbe_api_persist_get_layout_info(fbe_persist_control_get_layout_info_t *get_info)
 ****************************************************************
 * @brief
 *  This function get the lba offests and other information about the persist service
 *
 * @param get_info      - pointer to data to fill
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_get_layout_info(fbe_persist_control_get_layout_info_t *get_info)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    
	if (get_info == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s NULL pointer\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;

	}

    status = fbe_api_common_send_control_packet_to_service (FBE_PERSIST_CONTROL_CODE_GET_LAYOUT_INFO,
															get_info, 
															sizeof(fbe_persist_control_get_layout_info_t),
															FBE_SERVICE_ID_PERSIST,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
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
   @fn fbe_status_t FBE_API_CALL fbe_api_persist_write_entry_with_auto_entry_id_on_top(fbe_persist_transaction_handle_t transaction_handle,
																				fbe_persist_sector_type_t target_sector,
																				fbe_u8_t *data,
																				fbe_u32_t data_length,
																				fbe_persist_entry_id_t *entry_id)
 ****************************************************************
 * @brief
 *  Similar to fbe_api_persist_write_entry but will automatically write the first 64 bits in the user data
 *	with the entry ID. This is good for users who send out data in which the entry id is the first data
 *  and they want to save the time of modifying their data again after doing the first write just in order 
 *  to save the entry id.
 *
 * @param transaction_handle      - handle of transaction to write into
 * @param target_sector - which sector we do the write to
 * @param entry_id      - a handle to know how to acess this entry next time
 * @param data      - data location
 * @param data_length      - how long is the data
 *
 * @return
 *  fbe_status_t
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_write_entry_with_auto_entry_id_on_top(fbe_persist_transaction_handle_t transaction_handle,
																				fbe_persist_sector_type_t target_sector,
																				fbe_u8_t *data,
																				fbe_u32_t data_length,
																				fbe_persist_entry_id_t *entry_id)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
	fbe_persist_control_write_entry_t			write_entry;
	fbe_sg_element_t							sg_element[2];

    write_entry.tran_handle = transaction_handle;
	write_entry.target_sector = target_sector;

	fbe_sg_element_init(&sg_element[0], data_length, data);
	fbe_sg_element_terminate(&sg_element[1]);

	status = fbe_api_common_send_control_packet_to_service_with_sg_list (FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY_WITH_AUTO_ENTRY_ID,
																		 &write_entry, 
																		 sizeof(fbe_persist_control_write_entry_t),
																		 FBE_SERVICE_ID_PERSIST,
																		 FBE_PACKET_FLAG_NO_ATTRIB,
																		 sg_element,
																		 2,
																		 &status_info,
																		 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
			return status;
		}else{
			return FBE_STATUS_GENERIC_FAILURE;
		}
    }

	*entry_id = write_entry.entry_id;

    return status;

}


/*!***************************************************************
   @fn fbe_status_t FBE_API_CALL fbe_api_persist_write_single_entry(fbe_persist_sector_type_t target_sector,
																	fbe_u8_t *data,
																	fbe_u32_t data_length,
																	fbe_persist_single_operation_completion_function_t completion_function,
																	fbe_persist_completion_context_t completion_context)
 ****************************************************************
 * @brief
 *  Let the user write a single buffer into the persistence service at a certain sector w/o the need
 *  to open/close a transaction. This is a safer interface since it's guaranteed to to leave a transaction
 *  open in the middle of an operation
 *
 * @param target_sector - which sector we do the write to
 * @param data      - data location
 * @param data_length      - how long is the data
 * @param completion_function - a function that will also return the entry id for future use
 * @param completion_context - context for completion
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_write_single_entry(fbe_persist_sector_type_t target_sector,
															 fbe_u8_t *data,
															 fbe_u32_t data_length,
															 fbe_persist_single_operation_completion_function_t completion_function,
															 fbe_persist_completion_context_t completion_context)
{
	fbe_status_t                                status;
    fbe_persist_control_write_single_entry_t *	write_single_entry= NULL;
    fbe_sg_element_t *							sg_element;
	fbe_packet_t *								packet;

	if (data_length > FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: data size(0x%d) bigger than 0x%d\n", __FUNCTION__, data_length, FBE_PERSIST_DATA_BYTES_PER_ENTRY); 
        return FBE_STATUS_GENERIC_FAILURE;

	}

    if (completion_function == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a valid completion function\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

	packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }


    write_single_entry = (fbe_persist_control_write_single_entry_t *)fbe_api_allocate_memory(sizeof(fbe_persist_control_write_single_entry_t));
	if (write_single_entry == NULL) {
		fbe_api_return_contiguous_packet(packet);
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for operation\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

	sg_element = (fbe_sg_element_t *)fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
	if (sg_element == NULL) {
		fbe_api_return_contiguous_packet(packet);
		fbe_api_free_memory(write_single_entry);
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for dg list\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

    /*init ther stuff*/
    write_single_entry->target_sector = target_sector;
	write_single_entry->completion_function = completion_function;
	write_single_entry->completion_context = completion_context;

	fbe_sg_element_init(&sg_element[0], data_length, data);
	fbe_sg_element_terminate(&sg_element[1]);

    status = fbe_api_common_send_control_packet_to_service_with_sg_list_asynch (packet,
																				FBE_PERSIST_CONTROL_CODE_WRITE_SINGLE_ENTRY,
																				write_single_entry, 
																				sizeof(fbe_persist_control_write_single_entry_t),
																				FBE_SERVICE_ID_PERSIST,
																				sg_element,
																				2,
																				fbe_api_persist_write_single_entry_completion,
																				write_single_entry,
																				FBE_PACKAGE_ID_SEP_0);

	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		 fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
		 /*no need to destory here, we do it on bad completion*/
	}

    return status;

}

static fbe_status_t fbe_api_persist_write_single_entry_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
	fbe_persist_control_write_single_entry_t *	single_write = (fbe_persist_control_write_single_entry_t *)context;
	fbe_status_t								packet_status = fbe_transport_get_status_code(packet);
	fbe_payload_control_status_t				control_status;
	fbe_payload_control_operation_t *   		control_operation = NULL;
	fbe_status_t								callback_status = FBE_STATUS_OK;
	fbe_payload_ex_t *                			sep_payload = NULL;

    sep_payload = fbe_transport_get_payload_ex (packet);
	control_operation = fbe_payload_ex_get_control_operation(sep_payload);
	fbe_payload_control_get_status(control_operation, &control_status);

	/*did we have any errors ?*/
	if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
		if ((control_status == FBE_PAYLOAD_CONTROL_STATUS_BUSY || (packet_status == FBE_STATUS_BUSY))) {
			callback_status = FBE_STATUS_BUSY;
		}else{
			callback_status = FBE_STATUS_GENERIC_FAILURE;
		}
		
	}

	/*and call the user back*/
	single_write->completion_function(callback_status, single_write->entry_id, single_write->completion_context);

    fbe_api_return_contiguous_packet(packet);
	fbe_api_free_memory(single_write);

	return FBE_STATUS_OK;
}

/*!***************************************************************
   @fn fbe_status_t FBE_API_CALL fbe_api_persist_modify_single_entry(fbe_persist_entry_id_t entry_id,
																	fbe_u8_t *data,
																	fbe_u32_t data_length,
																	fbe_persist_single_operation_completion_function_t completion_function,
																	fbe_persist_completion_context_t completion_context)
 ****************************************************************
 * @brief
 *  Let the user modify a single entry in the persistence service w/o the need
 *  to open/close a transaction. This is a safer interface since it's guaranteed to to leave a transaction
 *  open in the middle of an operation
 *
 * @param entry_id - the entry we want to modify
 * @param data      - data location
 * @param data_length      - how long is the data
 * @param completion_function - a function that will also return the entry id for future use
 * @param completion_context - context for completion
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_modify_single_entry(fbe_persist_entry_id_t entry_id,
															  fbe_u8_t *data,
															  fbe_u32_t data_length,
															  fbe_persist_single_operation_completion_function_t completion_function,
															  fbe_persist_completion_context_t completion_context)
{
	fbe_status_t                                status;
    fbe_persist_control_modify_single_entry_t *	modify_single_entry= NULL;
    fbe_sg_element_t *							sg_element;
	fbe_packet_t *								packet;

	if (data_length > FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: data size(0x%d) bigger than 0x%d\n", __FUNCTION__, data_length, FBE_PERSIST_DATA_BYTES_PER_ENTRY); 
        return FBE_STATUS_GENERIC_FAILURE;

	}

    if (completion_function == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a valid completion function\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

	packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }


    modify_single_entry = (fbe_persist_control_modify_single_entry_t *)fbe_api_allocate_memory(sizeof(fbe_persist_control_modify_single_entry_t));
	if (modify_single_entry == NULL) {
		fbe_api_return_contiguous_packet(packet);
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for operation\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

	sg_element = (fbe_sg_element_t *)fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
	if (sg_element == NULL) {
		fbe_api_return_contiguous_packet(packet);
		fbe_api_free_memory(modify_single_entry);
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for dg list\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}

    /*init ther stuff*/
    modify_single_entry->entry_id = entry_id;
	modify_single_entry->completion_function = completion_function;
	modify_single_entry->completion_context = completion_context;

	fbe_sg_element_init(&sg_element[0], data_length, data);
	fbe_sg_element_terminate(&sg_element[1]);

    status = fbe_api_common_send_control_packet_to_service_with_sg_list_asynch (packet,
																				FBE_PERSIST_CONTROL_CODE_MODIFY_SINGLE_ENTRY,
																				modify_single_entry, 
																				sizeof(fbe_persist_control_modify_single_entry_t),
																				FBE_SERVICE_ID_PERSIST,
																				sg_element,
																				2,
																				fbe_api_persist_modify_single_entry_completion,
																				modify_single_entry,
																				FBE_PACKAGE_ID_SEP_0);

	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		 fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
		 /*no need to destory here, we do it on bad completion*/
	}

    return status;

}

static fbe_status_t fbe_api_persist_modify_single_entry_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
	fbe_persist_control_modify_single_entry_t *	single_modify = (fbe_persist_control_modify_single_entry_t *)context;
	fbe_status_t								packet_status = fbe_transport_get_status_code(packet);
	fbe_payload_control_status_t				control_status;
	fbe_payload_control_operation_t *   		control_operation = NULL;
	fbe_status_t								callback_status = FBE_STATUS_OK;
	fbe_payload_ex_t *                			sep_payload = NULL;

    sep_payload = fbe_transport_get_payload_ex (packet);
	control_operation = fbe_payload_ex_get_control_operation(sep_payload);
	fbe_payload_control_get_status(control_operation, &control_status);

	/*did we have any errors ?*/
	if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
		if ((control_status == FBE_PAYLOAD_CONTROL_STATUS_BUSY || (packet_status == FBE_STATUS_BUSY))) {
			callback_status = FBE_STATUS_BUSY;
		}else{
			callback_status = FBE_STATUS_GENERIC_FAILURE;
		}
		
	}

	/*and call the user back*/
	single_modify->completion_function(callback_status, single_modify->entry_id, single_modify->completion_context);

    fbe_api_return_contiguous_packet(packet);
	fbe_api_free_memory(single_modify);

	return FBE_STATUS_OK;

}

/*!***************************************************************
   @fn fbe_status_t FBE_API_CALL fbe_api_persist_delete_single_entry(fbe_persist_entry_id_t entry_id,
																	fbe_persist_single_operation_completion_function_t completion_function,
																	fbe_persist_completion_context_t completion_context)
 ****************************************************************
 * @brief
 *  Let the user delete a single entry in the persistence service w/o the need
 *  to open/close a transaction. This is a safer interface since it's guaranteed to to leave a transaction
 *  open in the middle of an operation
 *
 * @param entry_id - the entry we want to delete
 * @param completion_function - a function that will also return the entry id for future use
 * @param completion_context - context for completion
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_delete_single_entry(fbe_persist_entry_id_t entry_id,
															  fbe_persist_single_operation_completion_function_t completion_function,
															  fbe_persist_completion_context_t completion_context)
{
	fbe_status_t                                status;
	fbe_persist_control_delete_single_entry_t *	delete_single_entry= NULL;
    fbe_packet_t *								packet;

	if (completion_function == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a valid completion function\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	packet = fbe_api_get_contiguous_packet();
	if (packet == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}


	delete_single_entry = (fbe_persist_control_delete_single_entry_t *)fbe_api_allocate_memory(sizeof(fbe_persist_control_delete_single_entry_t));
	if (delete_single_entry == NULL) {
		fbe_api_return_contiguous_packet(packet);
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for operation\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*init ther stuff*/
	delete_single_entry->entry_id = entry_id;
	delete_single_entry->completion_function = completion_function;
	delete_single_entry->completion_context = completion_context;

	status = fbe_api_common_send_control_packet_to_service_asynch (packet,
																	FBE_PERSIST_CONTROL_CODE_DELETE_SINGLE_ENTRY,
																	delete_single_entry, 
																	sizeof(fbe_persist_control_delete_single_entry_t),
																	FBE_SERVICE_ID_PERSIST,
																    FBE_PACKET_FLAG_NO_ATTRIB,
																	fbe_api_persist_delete_single_entry_completion,
																	delete_single_entry,
																	FBE_PACKAGE_ID_SEP_0);

	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		 fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
		 /*no need to destory here, we do it on bad completion*/
	}

	return status;
}

static fbe_status_t fbe_api_persist_delete_single_entry_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
	fbe_persist_control_delete_single_entry_t *	single_delete = (fbe_persist_control_delete_single_entry_t *)context;
	fbe_status_t								packet_status = fbe_transport_get_status_code(packet);
	fbe_payload_control_status_t				control_status;
	fbe_payload_control_operation_t *   		control_operation = NULL;
	fbe_status_t								callback_status = FBE_STATUS_OK;
	fbe_payload_ex_t *                			sep_payload = NULL;

    sep_payload = fbe_transport_get_payload_ex (packet);
	control_operation = fbe_payload_ex_get_control_operation(sep_payload);
	fbe_payload_control_get_status(control_operation, &control_status);

	/*did we have any errors ?*/
	if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
		if ((control_status == FBE_PAYLOAD_CONTROL_STATUS_BUSY || (packet_status == FBE_STATUS_BUSY))) {
			callback_status = FBE_STATUS_BUSY;
		}else{
			callback_status = FBE_STATUS_GENERIC_FAILURE;
		}
	}

	/*and call the user back*/
	single_delete->completion_function(callback_status, single_delete->entry_id, single_delete->completion_context);

    fbe_api_return_contiguous_packet(packet);
	fbe_api_free_memory(single_delete);

	return FBE_STATUS_OK;

}

/*!***************************************************************
   @fn fbe_status_t FBE_API_CALL fbe_api_persist_get_total_blocks_needed(fbe_lba_t *total_blocks)
 ****************************************************************
 * @brief
 *  How many blocks will the persistence service consume.
 *
 * @param total_blocks - pointer for return value
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_get_total_blocks_needed(fbe_lba_t *total_blocks)
{
	fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
	fbe_persist_control_get_required_lun_size_t	get_size;
    
	if (total_blocks == NULL) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s NULL pointer\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;

	}

    status = fbe_api_common_send_control_packet_to_service (FBE_PERSIST_CONTROL_CODE_GET_REQUEIRED_LUN_SIZE,
															&get_size, 
															sizeof(fbe_persist_control_get_required_lun_size_t),
															FBE_SERVICE_ID_PERSIST,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
			return status;
		}else{
			return FBE_STATUS_GENERIC_FAILURE;
		}
    }

	*total_blocks = get_size.total_block_needed;
    
    return status;

}

/*!***************************************************************
   @fn fbe_api_persist_unset_lun(fbe_object_id_t lu_object_id)
 ****************************************************************
 * @brief
 *  This function unsets the LU the persist service uses. 
 *
 * @param none
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_unset_lun(void)
{
	fbe_status_t								status;
    fbe_api_control_operation_status_info_t     status_info;
	
    status =  fbe_api_common_send_control_packet_to_service (FBE_PERSIST_CONTROL_CODE_UNSET_LUN,
														     NULL, 
														     0,
														     FBE_SERVICE_ID_PERSIST,
														     FBE_PACKET_FLAG_NO_ATTRIB,
															 &status_info,
														     FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
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

/****************************************************************/
/* fbe_api_persist_add_hook()
 ****************************************************************
 * @brief
 *  This API sets hook in persist service for testability
 *
 * @param hook_type - the hook type we want to add
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/24/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_add_hook(fbe_persist_hook_type_t  hook_type,
                                                   fbe_package_id_t         target_package)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_persist_control_hook_t                  hook;

    if (hook_type <= FBE_PERSIST_HOOK_TYPE_INVALID || hook_type >= FBE_PERSIST_HOOK_TYPE_LAST )
    {   
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Set Illegal Hook Type\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    hook.hook_type = hook_type;
    hook.hook_op_code = FBE_PERSIST_HOOK_OPERATION_SET_HOOK;
    if (target_package == FBE_PACKAGE_ID_ESP)
    {
            hook.is_set_esp = FBE_TRUE;
            hook.is_set_sep = FBE_FALSE;
    }
    else if (target_package == FBE_PACKAGE_ID_SEP_0)
    {
            hook.is_set_esp = FBE_FALSE;
            hook.is_set_sep = FBE_TRUE;
    }
    else
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Set Illegal Target Package\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    hook.is_triggered = FBE_FALSE;
    
    status =  fbe_api_common_send_control_packet_to_service (FBE_PERSIST_CONTROL_CODE_HOOK,
                                                             &hook, 
                                                             sizeof(hook),
                                                             FBE_SERVICE_ID_PERSIST,
                                                             FBE_PACKET_FLAG_EXTERNAL,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
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

/******************************************************************************/
/* fbe_api_persist_remove_hook()
 ******************************************************************************
 * @brief
 *  This API removes an already set hook in persist service for testability
 *
 * @param hook_type - the hook type we want to add
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/24/2012 - Created. Zhipeng Hu
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_remove_hook(fbe_persist_hook_type_t  hook_type,
                                                      fbe_package_id_t         target_package)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_persist_control_hook_t                  hook;

    if (hook_type <= FBE_PERSIST_HOOK_TYPE_INVALID || hook_type >= FBE_PERSIST_HOOK_TYPE_LAST )
    {   
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Set Illegal Hook Type\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    hook.hook_type = hook_type;
    hook.hook_op_code = FBE_PERSIST_HOOK_OPERATION_REMOVE_HOOK;

    if (target_package == FBE_PACKAGE_ID_ESP)
    {
            hook.is_set_esp = FBE_TRUE;
            hook.is_set_sep = FBE_FALSE;
    }
    else if (target_package == FBE_PACKAGE_ID_SEP_0)
    {
            hook.is_set_esp = FBE_FALSE;
            hook.is_set_sep = FBE_TRUE;
    }
    else
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Set Illegal Target Package\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    
    status =  fbe_api_common_send_control_packet_to_service (FBE_PERSIST_CONTROL_CODE_HOOK,
                                                             &hook, 
                                                             sizeof(hook),
                                                             FBE_SERVICE_ID_PERSIST,
                                                             FBE_PACKET_FLAG_EXTERNAL,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_SEP_0);
    
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
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

/******************************************************************************/
/* fbe_api_persist_wait_hook()
 ******************************************************************************
 * @brief
 *  This API waits for a hook in persist service to be triggered.
 *  The hook must be set using fbe_api_persist_add_hook before for testability
 *
 * @param hook_type - the hook type we want to wait for
 * @param timeout_ms - how many milliseconds do we want to wait
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/24/2012 - Created. Zhipeng Hu
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persist_wait_hook(fbe_persist_hook_type_t  hook_type, fbe_u32_t timeout_ms,
                                                    fbe_package_id_t         target_package)
{
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_persist_control_hook_t                  hook;
    fbe_u32_t                                   wait_count = 0;
    fbe_bool_t                                  do_infinite_wait = FBE_FALSE;

    if (hook_type <= FBE_PERSIST_HOOK_TYPE_INVALID || hook_type >= FBE_PERSIST_HOOK_TYPE_LAST )
    {   
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Set Illegal Hook Type\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    wait_count = (timeout_ms + 100 - 1)/100;
    if(wait_count == 0)
    {
        do_infinite_wait = FBE_TRUE;
    }

    hook.hook_type = hook_type;
    hook.hook_op_code = FBE_PERSIST_HOOK_OPERATION_GET_STATE;

    hook.is_triggered = FBE_FALSE;
    
    if (target_package == FBE_PACKAGE_ID_ESP)
    {    
        hook.is_set_esp = FBE_TRUE;
        hook.is_set_sep = FBE_FALSE;

    }
    else if (target_package == FBE_PACKAGE_ID_SEP_0)
    {
        hook.is_set_sep = FBE_TRUE;
        hook.is_set_esp = FBE_FALSE;
    }
    else
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Set Illegal Target Package\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    while(1)
    {
        status =  fbe_api_common_send_control_packet_to_service (FBE_PERSIST_CONTROL_CODE_HOOK,
                                                                 &hook, 
                                                                 sizeof(hook),
                                                                 FBE_SERVICE_ID_PERSIST,
                                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                                 &status_info,
                                                                 FBE_PACKAGE_ID_SEP_0);
        
        if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
            if (status != FBE_STATUS_OK) {
                return status;
            }else{
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        if(hook.is_triggered == FBE_TRUE)
        {
            break;
        }

        if(do_infinite_wait == FBE_FALSE)
        {
            wait_count--;
            if(wait_count == 0)
                break;
        }

        fbe_api_sleep(100);
    }

    if(hook.is_triggered == FBE_FALSE)
    {
        return FBE_STATUS_TIMEOUT;
    }

    return FBE_STATUS_OK;
}

