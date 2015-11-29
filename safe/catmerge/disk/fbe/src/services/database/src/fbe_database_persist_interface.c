/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_persist_interface.c
***************************************************************************
*
* @brief
*  This file contains database service interface to persist service
*  
* @version
*  2/1/2011 - Created. 
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database_private.h"
#include "fbe_database_registry.h"
#include "fbe_database_cmi_interface.h"

extern fbe_database_service_t fbe_database_service;

/*************************
*   INCLUDE FUNCTIONS
*************************/
static fbe_status_t fbe_database_persist_set_lun_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_database_persist_commit_transaction_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_database_persist_read_entry_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_database_persist_read_sector_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_database_persist_abort_transaction_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
/*!************************************************************************************************
@fn fbe_status_t fbe_database_persist_get_layout_info(fbe_persist_control_get_layout_info_t *get_info)
***************************************************************************************************
* @brief
*  This function get the lba offests and other information about the persist service
*
* @param get_info      - pointer to data to fill
*
* @return
*  fbe_status_t
*
***************************************************************************************************/
fbe_status_t fbe_database_persist_get_layout_info(fbe_persist_control_get_layout_info_t *get_info)
{
    fbe_status_t status;

    if (get_info == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s NULL imput pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_send_packet_to_service ( FBE_PERSIST_CONTROL_CODE_GET_LAYOUT_INFO,
                                                   get_info, 
                                                   sizeof(fbe_persist_control_get_layout_info_t),
                                                   FBE_SERVICE_ID_PERSIST,
                                                   NULL, /* no sg list*/
                                                   0,    /* no sg list*/
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
    return status;
}

/*!**************************************************************************************************
@fn fbe_database_persist_set_lun(fbe_object_id_t lu_object_id,
                                 fbe_persist_completion_function_t completion_function,
                                 fbe_persist_completion_context_t completion_context)
*****************************************************************************************************

* @brief
*  This function sets the LU the persist service will use. It is ASYNCHRONOUS
*  and will complete only when the persist serivce has set the LU and checked
*  the persistence elements and other things it needs to do upon startup.
*
* @param lu_object_id           - The LU we will use to persist the data into
* @param completion_function    - completion function to call 
* @param completion_context     - context for completion
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
fbe_status_t fbe_database_persist_set_lun(fbe_object_id_t lu_object_id,
                                          fbe_persist_completion_function_t completion_function,
                                          fbe_persist_completion_context_t completion_context)
{
    fbe_persist_control_set_lun_t *             set_lun = NULL;
    fbe_packet_t *                              packet;
    fbe_status_t                                status;

    if (lu_object_id >= FBE_OBJECT_ID_INVALID) {        
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s invalid object id: %d\n", __FUNCTION__,lu_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (completion_function == NULL) {      
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Must have a valid completion function\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* allocate the packet, don't need to init the packet.
       packet will be intialized by fbe_database_send_packet_to_service_asynch*/
    packet = fbe_transport_allocate_packet(); 
    if (packet == NULL) {       
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: unable to allocate memory for packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    set_lun = fbe_transport_allocate_buffer();
    if (set_lun == NULL) {
        fbe_transport_release_packet(packet);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: unable to allocate memory for operation\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    set_lun->lun_object_id = lu_object_id;
    set_lun->completion_function = completion_function;
    set_lun->completion_context = completion_context;
    set_lun->journal_replayed = FBE_TRI_STATE_INVALID;

    status = fbe_database_send_packet_to_service_asynch(packet,
                                                        FBE_PERSIST_CONTROL_CODE_SET_LUN,
                                                        set_lun, 
                                                        sizeof(fbe_persist_control_set_lun_t),
                                                        FBE_SERVICE_ID_PERSIST,
                                                        NULL, /* no sg list*/
                                                        0,    /* no sg list*/
                                                        FBE_PACKET_FLAG_NO_ATTRIB,
                                                        fbe_database_persist_set_lun_completion,
                                                        set_lun,
                                                        FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: send_packet_to_service_asynch failed, status 0x%x.\n",
                        __FUNCTION__, status);
    }
    /* If the packet is sent successfully,the packet and buffer are freed in
       the completion function.  We are done here.*/
    return status; 
}

static fbe_status_t fbe_database_persist_set_lun_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_persist_control_set_lun_t *         set_lun = (fbe_persist_control_set_lun_t *)context;
    fbe_status_t                            packet_status = fbe_transport_get_status_code(packet);
    fbe_payload_control_status_t            control_status;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_status_t                            callback_status = FBE_STATUS_OK;
    fbe_payload_ex_t *                      sep_payload = NULL;
    fbe_payload_control_status_qualifier_t  qualifier;

    sep_payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &qualifier);

    /*did we have any errors ?*/
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
        /*the lun may have already been set and we called this function because the peer died so we just ignore the error and continue as ususal*/
        if (FBE_PERSIST_STATUS_LUN_ALREADY_SET != qualifier) {
            database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: control_status: 0x%x, packet_status: 0x%x.\n",
                            __FUNCTION__, control_status, packet_status);
            callback_status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (set_lun != NULL) {
        /* call the user back*/
        fbe_database_service.journal_replayed = set_lun->journal_replayed;
        set_lun->completion_function(callback_status, set_lun->completion_context);
    }else{
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: set_lun is NULL!\n",
                        __FUNCTION__);
    }
    /* free the memory allocated */
    fbe_transport_release_packet(packet);
    fbe_transport_release_buffer(set_lun);  

    return FBE_STATUS_OK;
}


/*!**************************************************************************************************
@fn fbe_database_persist_unset_lun
*****************************************************************************************************

* @brief
*  This function un-sets the LU the persist service will use.
*
* @return
*  fbe_status_t
* 
* @author 
* 6/13/2012   created. Jingcheng Zhang 
*****************************************************************************************************/
fbe_status_t fbe_database_persist_unset_lun(void)
{
    fbe_status_t                                status = FBE_STATUS_OK;


    /*send packet to persist service to un-set the LUN*/
    status = fbe_database_send_packet_to_service (FBE_PERSIST_CONTROL_CODE_UNSET_LUN,
                                                  NULL, 
                                                  0,
                                                  FBE_SERVICE_ID_PERSIST,
                                                  NULL, /* no sg list*/
                                                  0,    /* no sg list*/
                                                  FBE_PACKET_FLAG_NO_ATTRIB,                                                            
                                                  FBE_PACKAGE_ID_SEP_0);

    return status; 
}


/*!**************************************************************************************************
@fn fbe_database_persist_start_transaction(fbe_persist_transaction_handle_t *transaction_handle)
*****************************************************************************************************
* @brief
*  This function starts a transaction in the persist service
*
* @param transaction_handle      - return handle to user
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
fbe_status_t fbe_database_persist_start_transaction(fbe_persist_transaction_handle_t *transaction_handle)
{
    fbe_status_t                                status;    
    fbe_persist_control_transaction_t           transaction;
    fbe_bool_t                                  active_side = database_common_cmi_is_active();

    if (!active_side) {
        return FBE_STATUS_OK;
    }

    /* If you are interested in knowing about the change, go to fbe_database_limit_t and read the comments in it */
    FBE_ASSERT_AT_COMPILE_TIME((DATABASE_MAX_RG_CREATE_USER_ENTRY+DATABASE_MAX_RG_CREATE_OBJECT_ENTRY+DATABASE_MAX_RG_CREATE_EDGE_ENTRY)
                                <= FBE_PERSIST_TRAN_ENTRY_MAX);

    FBE_ASSERT_AT_COMPILE_TIME((DATABASE_MAX_PVD_CREATE_USER_ENTRY+DATABASE_MAX_PVD_CREATE_OBJECT_ENTRY+DATABASE_MAX_PVD_CREATE_EDGE_ENTRY)
                                <= FBE_PERSIST_TRAN_ENTRY_MAX);

    if (transaction_handle == NULL) {   
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s NULL input pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    transaction.tran_op_type = FBE_PERSIST_TRANSACTION_OP_START;
    transaction.caller_package = FBE_PACKAGE_ID_SEP_0;
 
    status = fbe_database_send_packet_to_service (FBE_PERSIST_CONTROL_CODE_TRANSACTION,
                                                    &transaction, 
                                                    sizeof(fbe_persist_control_transaction_t),
                                                    FBE_SERVICE_ID_PERSIST,
                                                    NULL, /* no sg list*/
                                                    0,    /* no sg list*/
                                                    FBE_PACKET_FLAG_NO_ATTRIB,                                                          
                                                    FBE_PACKAGE_ID_SEP_0);

    *transaction_handle = transaction.tran_handle;
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Return handle 0x%llx, Status 0x%x\n", __FUNCTION__, (unsigned long long)(*transaction_handle), status);

    return status;
}

/*!**************************************************************************************************
@fn fbe_database_persist_abort_transaction(fbe_persist_transaction_handle_t transaction_handle)
*****************************************************************************************************
* @brief
*  This function aborts a transaction in progress *
* @param transaction_handle      - handle of transaction to abort
*
* @return
*  fbe_status_t
*
****************************************************************************************************/
fbe_status_t fbe_datbase_persist_abort_transaction(fbe_persist_transaction_handle_t transaction_handle)
{
    fbe_status_t                                status;
    fbe_persist_control_transaction_t           transaction;
    fbe_bool_t                                  active_side = database_common_cmi_is_active();

    if (!active_side) {
        return FBE_STATUS_OK;
    }

    transaction.tran_handle = transaction_handle;
    transaction.tran_op_type = FBE_PERSIST_TRANSACTION_OP_ABORT;
    transaction.caller_package = FBE_PACKAGE_ID_SEP_0;

    status = fbe_database_send_packet_to_service (FBE_PERSIST_CONTROL_CODE_TRANSACTION,
                                                &transaction, 
                                                sizeof(fbe_persist_control_transaction_t),
                                                FBE_SERVICE_ID_PERSIST,
                                                NULL, /* no sg list*/
                                                0,    /* no sg list*/
                                                FBE_PACKET_FLAG_NO_ATTRIB,                                                          
                                                FBE_PACKAGE_ID_SEP_0);   

    return status;
}

/*!**************************************************************************************************
@fn fbe_database_persist_commit_transaction(fbe_persist_transaction_handle_t transaction_handle, 
                                            fbe_persist_completion_function_t completion_function,
                                            fbe_persist_completion_context_t completion_context)
*****************************************************************************************************
* @brief
*  This function COMMITS a transaction in progress.  It is ASYNCHRONOUS
*  and will complete only when the persist serivce has completed the commit. 
 
* @param transaction_handle      - handle of transaction to commit
* @param completion_function     - function to call when we complete
* @param completion_context      - context to pass to completion
*
* @return
*  fbe_status_t
*
****************************************************************************************************/
fbe_status_t fbe_database_persist_commit_transaction(fbe_persist_transaction_handle_t transaction_handle,
                                                     fbe_persist_completion_function_t completion_function,
                                                     fbe_persist_completion_context_t completion_context)
{
    fbe_persist_control_transaction_t *         transaction = NULL;
    fbe_packet_t *                              packet = NULL;
    fbe_status_t                                status;
    fbe_bool_t                                  active_side = database_common_cmi_is_active();

    if (!active_side) {
        return FBE_STATUS_OK;
    }

    if (completion_function == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Must have a valid completion function\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;      
    }
    /* allocate the packet, don't need to init the packet.
       packet will be intialized by fbe_database_send_packet_to_service_asynch*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: unable to allocate memory for packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;         
    }

    transaction = fbe_transport_allocate_buffer();
    if (transaction == NULL) {
        fbe_transport_release_packet(packet);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: unable to allocate memory for transaction\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;                 
    }

    transaction->tran_handle = transaction_handle;
    transaction->tran_op_type = FBE_PERSIST_TRANSACTION_OP_COMMIT;
    transaction->completion_function = completion_function;
    transaction->completion_context = completion_context;
    transaction->caller_package = FBE_PACKAGE_ID_SEP_0;

    status =  fbe_database_send_packet_to_service_asynch (packet,
                                                        FBE_PERSIST_CONTROL_CODE_TRANSACTION,
                                                        transaction, 
                                                        sizeof(fbe_persist_control_transaction_t),
                                                        FBE_SERVICE_ID_PERSIST,
                                                        NULL, /* no sg list*/
                                                        0,    /* no sg list*/
                                                        FBE_PACKET_FLAG_NO_ATTRIB,
                                                        fbe_database_persist_commit_transaction_completion,
                                                        transaction,
                                                        FBE_PACKAGE_ID_SEP_0);
    return status;
}

static fbe_status_t fbe_database_persist_commit_transaction_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_persist_control_transaction_t *     transaction = (fbe_persist_control_transaction_t *)context;
    fbe_status_t                            packet_status = fbe_transport_get_status_code(packet);
    fbe_payload_control_status_t            control_status;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_status_t                            callback_status = FBE_STATUS_OK;
    fbe_payload_ex_t *                      sep_payload = NULL;

    sep_payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /*did we have any errors ?*/
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
        database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: control_status: 0x%x, packet_status: 0x%x.\n",
                        __FUNCTION__, control_status, packet_status);
        callback_status = FBE_STATUS_GENERIC_FAILURE;
    }

    if (transaction != NULL) {
        /* call the user back*/
        transaction->completion_function(callback_status, transaction->completion_context);
    }else{
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: transaction is NULL!\n",
                        __FUNCTION__);
    }
    /* free the memory allocated */
    fbe_transport_release_packet(packet);
    fbe_transport_release_buffer(transaction);

    return FBE_STATUS_OK;
}


/*!**************************************************************************************************
@fn fbe_database_persist_abort_transaction(fbe_persist_transaction_handle_t transaction_handle, 
                                            fbe_persist_completion_function_t completion_function,
                                            fbe_persist_completion_context_t completion_context)
*****************************************************************************************************
* @brief
*  This function abort a transaction in progress.  It is ASYNCHRONOUS
*  and will complete only when the persist serivce has completed the abort operation. 
 
* @param transaction_handle      - handle of transaction to commit
* @param completion_function     - function to call when we complete
* @param completion_context      - context to pass to completion
*
* @return
*  fbe_status_t
* @author  07/20/2012 created. Jingcheng Zhang 
****************************************************************************************************/
fbe_status_t fbe_database_persist_abort_transaction(fbe_persist_transaction_handle_t transaction_handle,
                                                     fbe_persist_completion_function_t completion_function,
                                                     fbe_persist_completion_context_t completion_context)
{
    fbe_persist_control_transaction_t *         transaction = NULL;
    fbe_packet_t *                              packet = NULL;
    fbe_status_t                                status;
    fbe_bool_t                                  active_side = database_common_cmi_is_active();

    if (!active_side) {
        return FBE_STATUS_OK;
    }

    if (completion_function == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Must have a valid completion function\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;      
    }
    /* allocate the packet, don't need to init the packet.
       packet will be intialized by fbe_database_send_packet_to_service_asynch*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: unable to allocate memory for packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;         
    }

    transaction = fbe_transport_allocate_buffer();
    if (transaction == NULL) {
        fbe_transport_release_packet(packet);
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: unable to allocate memory for transaction\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;                 
    }

    transaction->tran_handle = transaction_handle;
    transaction->tran_op_type = FBE_PERSIST_TRANSACTION_OP_ABORT;
    transaction->completion_function = completion_function;
    transaction->completion_context = completion_context;
    transaction->caller_package = FBE_PACKAGE_ID_SEP_0;

    status =  fbe_database_send_packet_to_service_asynch (packet,
                                                        FBE_PERSIST_CONTROL_CODE_TRANSACTION,
                                                        transaction, 
                                                        sizeof(fbe_persist_control_transaction_t),
                                                        FBE_SERVICE_ID_PERSIST,
                                                        NULL, /* no sg list*/
                                                        0,    /* no sg list*/
                                                        FBE_PACKET_FLAG_NO_ATTRIB,
                                                        fbe_database_persist_abort_transaction_completion,
                                                        transaction,
                                                        FBE_PACKAGE_ID_SEP_0);
    return status;
}

static fbe_status_t fbe_database_persist_abort_transaction_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_persist_control_transaction_t *     transaction = (fbe_persist_control_transaction_t *)context;
    fbe_status_t                            packet_status = fbe_transport_get_status_code(packet);
    fbe_payload_control_status_t            control_status;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_status_t                            callback_status = FBE_STATUS_OK;
    fbe_payload_ex_t *                      sep_payload = NULL;

    sep_payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /*did we have any errors ?*/
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
        database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: control_status: 0x%x, packet_status: 0x%x.\n",
                        __FUNCTION__, control_status, packet_status);
        callback_status = FBE_STATUS_GENERIC_FAILURE;
    }

    if (transaction != NULL) {
        /* call the user back*/
        if (transaction->completion_function != NULL) {
            transaction->completion_function(callback_status, transaction->completion_context);
        }
    }else{
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: transaction is NULL!\n",
                        __FUNCTION__);
    }
    /* free the memory allocated */
    fbe_transport_release_packet(packet);
    if (transaction != NULL) {
        fbe_transport_release_buffer(transaction);
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************************************************
@fn fbe_status_t fbe_database_persist_read_entry(fbe_persist_entry_id_t entry_id,
                                                fbe_u8_t *read_buffer,
                                                fbe_u32_t data_length,
                                                fbe_persist_completion_function_t completion_function,
                                                fbe_persist_completion_context_t completion_context)
*****************************************************************************************************
* @brief
*  This function read a singe entry from the persistence drive. It is asynchronous and the read_buffer
*  must be from the non paged pool (does not have to be physically contiguos).
*
* @param entry_id      - from which entry to read
* @param read_buffer   - buffer to read to, does not have to be physically contigous, but must be non paged
* @param data_length   - lenght of the data buffer, must be FBE_PERSIST_DATA_BYTES_PER_ENTRY
* @param completion_function   - function to call when we complete
* @param completion_context    - context to pass to completion
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
fbe_status_t  fbe_database_persist_read_entry(fbe_persist_entry_id_t entry_id,
                                              fbe_persist_sector_type_t persist_sector_type,
                                             fbe_u8_t *read_buffer,
                                             fbe_u32_t data_length,/* MUST BE size of FBE_PERSIST_DATA_BYTES_PER_ENTRY */
                                             fbe_persist_single_read_completion_function_t completion_function,
                                             fbe_persist_completion_context_t completion_context)
{
    fbe_status_t                                status;
    fbe_sg_element_t *                          sg_element, *sg_element_head; 
    fbe_packet_t *                              packet;
    fbe_persist_control_read_entry_t *          read_entry = NULL;

    if (data_length != FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Must have a size of 0x%x bytes\n",
                        __FUNCTION__, FBE_PERSIST_DATA_BYTES_PER_ENTRY);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    if (completion_function == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Must have a valid completion function\n", 
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;      
    }
    /* allocate the packet, don't need to init the packet.
       packet will be intialized by fbe_database_send_packet_to_service_asynch*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to allocate memory for packet\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;          
    }

    read_entry = fbe_transport_allocate_buffer();
    if (read_entry == NULL) {
        fbe_transport_release_buffer(packet);
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to allocate memory for read\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;          
    }

    sg_element = sg_element_head = fbe_transport_allocate_buffer();
    if (sg_element == NULL) {
        fbe_transport_release_packet(packet);
        fbe_transport_release_buffer(read_entry);
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to allocate memory for sg list\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    /*init the sg list*/
    fbe_sg_element_init(sg_element++, FBE_PERSIST_DATA_BYTES_PER_ENTRY, read_buffer);
    fbe_sg_element_terminate(sg_element);

    /*init the stuff*/
    read_entry->completion_context = completion_context;
    read_entry->completion_function = completion_function;
    read_entry->entry_id = entry_id; 
    read_entry->persist_sector_type = persist_sector_type;

    status = fbe_database_send_packet_to_service_asynch (packet,
                                                        FBE_PERSIST_CONTROL_CODE_READ_ENTRY,
                                                        read_entry, 
                                                        sizeof(fbe_persist_control_read_entry_t),
                                                        FBE_SERVICE_ID_PERSIST,
                                                        sg_element_head,
                                                        2,
                                                        FBE_PACKET_FLAG_NO_ATTRIB,
                                                        fbe_database_persist_read_entry_completion,
                                                        read_entry,
                                                        FBE_PACKAGE_ID_SEP_0);
/*
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){                  
        database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,                                 
                        FBE_TRACE_MESSAGE_ID_INFO,                                 
                        "%s: send_packet_to_service_asynch failed, status 0x%x.\n",
                        __FUNCTION__, status);                                     
        fbe_transport_release_packet(packet);                                      
        fbe_transport_release_buffer(read_entry);                                  
        fbe_transport_release_buffer(sg_element_head);                             
    }                                                                              
*/
    /* If the packet is sent successfully,the packet and buffer are freed in
       the completion function.  We are done here.*/
    return status;
}

static fbe_status_t fbe_database_persist_read_entry_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_persist_control_read_entry_t *      read_entry = (fbe_persist_control_read_entry_t *)context;
    fbe_status_t                            packet_status = fbe_transport_get_status_code(packet);
    fbe_u32_t                               packet_status_qualifier = fbe_transport_get_status_qualifier(packet);
    fbe_payload_control_status_t            control_status;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_status_t                            callback_status = FBE_STATUS_OK;
    fbe_payload_ex_t *                      sep_payload = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               count;

    sep_payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);

    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_ex_get_sg_list(sep_payload, &sg_element, &count);

    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
        database_trace (FBE_TRACE_LEVEL_DEBUG_LOW, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: control_status: 0x%x, packet_status: 0x%x.\n",
                        __FUNCTION__, control_status, packet_status);
        if (packet_status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_BAD_BLOCK) {
            callback_status = FBE_STATUS_IO_FAILED_RETRYABLE; 
        }
        else{
            callback_status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (read_entry != NULL) {
        /* call the user back*/
        read_entry->completion_function(callback_status, read_entry->completion_context, read_entry->next_entry_id);
    }else{
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: read_entry is NULL!\n",
                        __FUNCTION__);
    }
    /* free the memory allocated */
    fbe_transport_release_packet(packet);
    fbe_transport_release_buffer(read_entry);
    fbe_transport_release_buffer(sg_element);

    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn fbe_database_persist_write_entry(fbe_persist_transaction_handle_t transaction_handle,
                                    fbe_persist_sector_type_t target_sector,
                                    fbe_u8_t *data,
                                    fbe_u32_t data_length,
                                    fbe_persist_entry_id_t *entry_id)
*****************************************************************************************************
* @brief
*  This function writes a data element into the transaction 
* 
* @param transaction_handle    - handle of transaction to write into
* @param target_sector         - which sector we do the write to
* @param entry_id              - a handle to know how to acess this entry next time
* @param data                  - data location
* @param data_length           - how long is the data
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
fbe_status_t fbe_database_persist_write_entry(fbe_persist_transaction_handle_t transaction_handle,
                                              fbe_persist_sector_type_t target_sector,
                                              fbe_u8_t *data,
                                              fbe_u32_t data_length,
                                              fbe_persist_entry_id_t *entry_id)
{
    fbe_status_t                                status;    
    fbe_persist_control_write_entry_t           write_entry;
    fbe_sg_element_t                            sg_element[2];  /* this a synch operation, so it is ok to have sg on the stack*/
    fbe_bool_t                                  active_side = database_common_cmi_is_active();
    fbe_payload_control_operation_opcode_t      write_op_code;

    if (!active_side) {
        return FBE_STATUS_OK;
    }

    if (data_length > FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: data length must <= 0x%x bytes\n",
                        __FUNCTION__, FBE_PERSIST_DATA_BYTES_PER_ENTRY);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    write_entry.tran_handle = transaction_handle;
    write_entry.target_sector = target_sector;
    write_entry.entry_id = *entry_id;

    fbe_sg_element_init(&sg_element[0], data_length, data);
    fbe_sg_element_terminate(&sg_element[1]);

    /* If caller specifies the valid entry id, we will just use the id to do persist;
     * otherwise, let the persist service automatically allocates a suitable one*/
    if(0 == *entry_id)
    {
        write_op_code = FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY_WITH_AUTO_ENTRY_ID;
    }
    else
    {
        write_op_code = FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY;
    }

    status = fbe_database_send_packet_to_service (write_op_code,
                                                &write_entry, 
                                                sizeof(fbe_persist_control_write_entry_t),
                                                FBE_SERVICE_ID_PERSIST,                                                                      
                                                sg_element,
                                                2,          
                                                FBE_PACKET_FLAG_NO_ATTRIB,                                                  
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: packet send status: 0x%x.\n",
                        __FUNCTION__, status);
    }
    *entry_id = write_entry.entry_id;
    return status;
}
/*!**************************************************************************************************
@fn fbe_database_persist_modify_entry(fbe_persist_transaction_handle_t transaction_handle,
                                        fbe_u8_t *data,
                                        fbe_u32_t data_length,
                                        fbe_persist_entry_id_t entry_id)
*****************************************************************************************************
* @brief
*  This function modify an existing element 
* 
* @param transaction_handle    - handle of transaction to write into
* @param entry_id              - a handle into the handle to modify
* @param data                  - data location
* @param data_length           - how long is the data
*
* @return
*  fbe_status_t
*
****************************************************************************************************/
fbe_status_t fbe_database_persist_modify_entry(fbe_persist_transaction_handle_t transaction_handle,
                                               fbe_u8_t *data,
                                               fbe_u32_t data_length,
                                               fbe_persist_entry_id_t entry_id)
{
    fbe_status_t                                status;
    fbe_persist_control_modify_entry_t          modify_entry;
    fbe_sg_element_t                            sg_element[2];
    fbe_bool_t                                  active_side = database_common_cmi_is_active();

    if (!active_side) {
        return FBE_STATUS_OK;
    }

    if (data_length > FBE_PERSIST_DATA_BYTES_PER_ENTRY) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: data length <= 0x%x bytes\n",
                        __FUNCTION__, FBE_PERSIST_DATA_BYTES_PER_ENTRY);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    modify_entry.tran_handle = transaction_handle;
    modify_entry.entry_id = entry_id;

    fbe_sg_element_init(&sg_element[0], data_length, data);
    fbe_sg_element_terminate(&sg_element[1]);

    status = fbe_database_send_packet_to_service (FBE_PERSIST_CONTROL_CODE_MODIFY_ENTRY,
                                                &modify_entry, 
                                                sizeof(fbe_persist_control_modify_entry_t),
                                                FBE_SERVICE_ID_PERSIST,                                                                      
                                                sg_element,
                                                2,                                                                  
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: packet send status: 0x%x.\n",
                        __FUNCTION__, status);
    }
    return status;
}

/*!**************************************************************************************************
@fn fbe_status_t fbe_database_persist_delete_entry(fbe_persist_transaction_handle_t transaction_handle,
                                                   fbe_persist_entry_id_t entry_id)
*****************************************************************************************************
* @brief
*  This function deletes an existing element 
* 
* @param transaction_handle    - handle of transaction to write into
* @param entry_id              - a handle into the handle to delete
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
fbe_status_t fbe_database_persist_delete_entry(fbe_persist_transaction_handle_t transaction_handle,
                                               fbe_persist_entry_id_t entry_id)
{
    fbe_status_t                                status = FBE_STATUS_OK;    
    fbe_persist_control_delete_entry_t          delete_entry;
    fbe_bool_t                                  active_side = database_common_cmi_is_active();

    if (!active_side) {
        return FBE_STATUS_OK;
    }

    delete_entry.tran_handle = transaction_handle;
    delete_entry.entry_id = entry_id;

    status = fbe_database_send_packet_to_service (FBE_PERSIST_CONTROL_CODE_DELETE_ENTRY,
                                                    &delete_entry, 
                                                    sizeof(fbe_persist_control_delete_entry_t),
                                                    FBE_SERVICE_ID_PERSIST,
                                                    NULL, /* no sg list*/
                                                    0,    /* no sg list*/
                                                    FBE_PACKET_FLAG_NO_ATTRIB,                                                          
                                                    FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: packet send status: 0x%x.\n",
                        __FUNCTION__, status);
    }
    return status;
}

/*!**************************************************************************************************
@fn fbe_status_t fbe_database_persist_read_sector(fbe_persist_sector_type_t target_sector,
                                              fbe_u8_t *read_buffer,
                                              fbe_u32_t data_length,
                                              fbe_persist_entry_id_t start_entry_id,
                                              fbe_persist_read_multiple_entries_completion_function_t completion_function,
                                              fbe_persist_completion_context_t completion_context)
*****************************************************************************************************
* @brief
*  This function read 400 entries at once from the sector selected.
*  It is asynchronous and the user must pass a buffer in the size of FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE
*  Once received, the buffer will contains 400 entries and each of them will have fbe_persist_user_header_t
*  in front of it which the user can use later to get information on how to modify or delete this entry.
*  The callback is of type fbe_persist_read_multiple_entries_completion_function_t which will also return the next entry to
*  start the read from or FBE_PERSIST_NO_MORE_ENTRIES_TO_READ if all reads are done. It will also return the number of entries
*  read so the user can tell how much of the buffer he passed has relevent information.
*  The user needs to know that some of the entries in the buffer will have empty entries which are spots
*  in the sector that were either deleted or never used. In any case, the whole sector is read.
*
* @param target_sector    - from which sector to read
* @param read_buffer      - buffer to read to, does not have to be physically contigous, but must be non paged
* @param data_length      - lenght of the data buffer, must be FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE
* @param start_entry_id   - whete to start reading from. On first read, 
*                           use FBE_PERSIST_SECTOR_START_ENTRY_ID, 
*                           after that supply the value in next_start_entry_id
* @param completion_function   - function to call when we complete
* @param completion_context    - context to pass to completion
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
fbe_status_t fbe_database_persist_read_sector(fbe_persist_sector_type_t target_sector,
                                              fbe_u8_t *read_buffer,
                                              fbe_u32_t data_length,/*must be of size FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE*/
                                              fbe_persist_entry_id_t start_entry_id,
                                              fbe_persist_read_multiple_entries_completion_function_t completion_function,
                                              fbe_persist_completion_context_t completion_context)
{
    fbe_status_t                                status;
    fbe_persist_control_read_sector_t *         read_sector;
    fbe_sg_element_t *                          sg_element, *sg_element_head;
    fbe_packet_t *                              packet;

    if (data_length != FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Must have a buffer size of 0x%llu bytes\n", 
                        __FUNCTION__,
            (unsigned long long)FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE);
        return FBE_STATUS_GENERIC_FAILURE;              
    }

    if (completion_function == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO, 
                        "%s: Must have a valid completion function\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    /* allocate the packet, don't need to init the packet.
       packet will be intialized by fbe_database_send_packet_to_service_asynch*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to allocate memory for packet\n", 
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    read_sector =  fbe_transport_allocate_buffer();
    if (read_sector == NULL) {
        fbe_transport_release_packet(packet);
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to allocate memory for operation\n", 
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_element = sg_element_head = fbe_transport_allocate_buffer();
    if (sg_element == NULL) {
        fbe_transport_release_packet(packet);
        fbe_transport_release_buffer(read_sector);

        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to allocate memory for sg list\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*init the sg list*/
    fbe_zero_memory(read_buffer, data_length);
    fbe_sg_element_init(sg_element++, FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE, read_buffer);
    fbe_sg_element_terminate(sg_element);

    /*init ther stuff*/
    read_sector->completion_context = completion_context;
    read_sector->completion_function = completion_function;
    read_sector->start_entry_id = start_entry_id;
    read_sector->sector_type = target_sector;

    status = fbe_database_send_packet_to_service_asynch (packet,
                                                        FBE_PERSIST_CONTROL_CODE_READ_SECTOR,
                                                        read_sector, 
                                                        sizeof(fbe_persist_control_read_sector_t),
                                                        FBE_SERVICE_ID_PERSIST,
                                                        sg_element_head,
                                                        2,
                                                        FBE_PACKET_FLAG_NO_ATTRIB,
                                                        fbe_database_persist_read_sector_completion,
                                                        read_sector,
                                                        FBE_PACKAGE_ID_SEP_0);
/*
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){                  
        database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,                                 
                        FBE_TRACE_MESSAGE_ID_INFO,                                 
                        "%s: send_packet_to_service_asynch failed, status 0x%x.\n",
                        __FUNCTION__, status);                                     
        fbe_transport_release_packet(packet);                                      
        fbe_transport_release_buffer(read_sector);                                 
        fbe_transport_release_buffer(sg_element_head);                             
    }                                                                              
*/
    /* If the packet is sent successfully,the packet and buffer are freed in
       the completion function.  We are done here.*/
    return status;
}

static fbe_status_t fbe_database_persist_read_sector_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_persist_control_read_sector_t *         read_sector = (fbe_persist_control_read_sector_t *)context;
    fbe_status_t                                packet_status = fbe_transport_get_status_code(packet);
    fbe_u32_t                                   packet_status_qualifier = fbe_transport_get_status_qualifier(packet);
    fbe_payload_control_status_t                control_status;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_status_t                                callback_status = FBE_STATUS_OK;
    fbe_payload_ex_t *                          sep_payload = NULL;
    fbe_sg_element_t *                          sg_element = NULL;
    fbe_u32_t                                   count;

    sep_payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_ex_get_sg_list(sep_payload, &sg_element, &count);
 
    /*did we have any errors ?*/
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
        database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: control_status: 0x%x, packet_status: 0x%x.\n",
                        __FUNCTION__, control_status, packet_status);
        if (packet_status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_BAD_BLOCK) {
            callback_status = FBE_STATUS_IO_FAILED_RETRYABLE; 
        }
        else{
            callback_status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (read_sector != NULL) {
        /* call the user back*/
        read_sector->completion_function(callback_status,
                                        read_sector->next_entry_id,
                                        read_sector->entries_read,
                                        read_sector->completion_context);
    }else{
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: read_sector is NULL!\n",
                        __FUNCTION__);
    }
    /* free the memory allocated */
    fbe_transport_release_packet(packet);
    fbe_transport_release_buffer(read_sector);
    fbe_transport_release_buffer(sg_element);

    return FBE_STATUS_OK;  
}

/*!**************************************************************************************************
@fn fbe_database_persist_get_required_lun_size(fbe_lba_t *required_lun_size)
*****************************************************************************************************
* @brief
*  This function gets the required lun size from persist service
*
* @param required_lun_size      - return the size to user
*
* @return
*  fbe_status_t 
* 
* @History: 
* 05/05/2011 Created 
*
*****************************************************************************************************/
fbe_status_t fbe_database_persist_get_required_lun_size(fbe_lba_t *required_lun_size)
{
    fbe_status_t                                status;    
    fbe_persist_control_get_required_lun_size_t get_required_lun_size;

    status = fbe_database_send_packet_to_service (FBE_PERSIST_CONTROL_CODE_GET_REQUEIRED_LUN_SIZE,
                                                  &get_required_lun_size, 
                                                  sizeof(fbe_persist_control_get_required_lun_size_t),
                                                  FBE_SERVICE_ID_PERSIST,
                                                  NULL, /* no sg list*/
                                                  0,    /* no sg list*/
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  FBE_PACKAGE_ID_SEP_0);
    database_trace (FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Return Status 0x%x\n", __FUNCTION__, status);
    *required_lun_size = get_required_lun_size.total_block_needed;
    return status;
}

/*!***************************************************************
 * @fn fbe_database_get_user_modified_wwn_seed_flag_with_retry(
 *               fbe_bool_t *user_modified_wwn_seed_flag)
 ****************************************************************
 * @brief
 *  This function will send control code to get user modified wwn
 *  seed flag
 *
 * @param
 *   user_modified_wwn_seed_flag - Buffer to store the return information
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *
 * @version
 *    10/26/2012:   created He,Wei
 *
 ****************************************************************/
fbe_status_t fbe_database_get_user_modified_wwn_seed_flag_with_retry(fbe_bool_t *user_modified_wwn_seed_flag)
{
    fbe_u32_t       retry_count = 0;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    do{
        status = fbe_database_get_user_modified_wwn_seed_flag(user_modified_wwn_seed_flag);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to modify usermodifiedWwnSeed flags, status %d\n",
                           __FUNCTION__, status);
                           
            /* Wait 1 second */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
            retry_count++;
        }
    }while((status != FBE_STATUS_OK) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES));

    return status;
}

/*!***************************************************************
 * @fn fbe_database_get_board_resume_prom_wwnseed_with_retry(fbe_u32_t *wwn_seed)
 ****************************************************************
 * @brief
 *  This function will send control code to ESP to get 
 *  wwn seed from chassis prom.
 *
 * @param
 *   wwn_seed - Buffer to store the return information
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *
 * @version
 *    10/26/2012:   created He,Wei
 *
 ****************************************************************/
fbe_status_t fbe_database_get_board_resume_prom_wwnseed_with_retry(fbe_u32_t *wwn_seed)
{
    fbe_u32_t       retry_count = 0;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    do{
        status = fbe_database_get_board_resume_prom_wwnseed_by_pp(wwn_seed);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get resume prom wwn seed, status %d\n",
                           __FUNCTION__, status);
                           
            /* Wait 1 second */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
            retry_count++;
        }
    }while((status != FBE_STATUS_OK) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES));

    return status;
}


/*!***************************************************************
 * @fn fbe_database_set_board_resume_prom_wwnseed(fbe_u32_t wwn_seed)
 ****************************************************************
 * @brief
 *  This function will send control code to ESP to set 
 *  wwn seed to chassis prom.
 *
 * @param
 *   wwn_seed - store the wwn seed
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *
 * @version
 *    10/26/2012:   created He,Wei
 *
 ****************************************************************/
fbe_status_t fbe_database_set_board_resume_prom_wwnseed_with_retry(fbe_u32_t wwn_seed)
{
    fbe_u32_t       retry_count = 0;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    do{
        status = fbe_database_set_board_resume_prom_wwnseed_by_pp(wwn_seed);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to set resume prom wwn seed, status %d\n",
                           __FUNCTION__, status);
                           
            /* Wait 1 second */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
            retry_count++;
        }
    }while((status != FBE_STATUS_OK) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES));

    return status;
}

/*!***************************************************************
 * @fn fbe_database_clear_user_modified_wwn_seed_flag_with_retry(
 *               void)
 ****************************************************************
 * @brief
 *  This function will send control code to clear user modified wwn
 *  seed flag
 *
 * @param
 *   void
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *
 * @version
 *    10/26/2012:   created He,Wei
 *
 ****************************************************************/
fbe_status_t fbe_database_clear_user_modified_wwn_seed_flag_with_retry(void)
{
    fbe_u32_t       retry_count = 0;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    do{
        status = fbe_database_clear_user_modified_wwn_seed_flag();
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to clear usermodifiedWwnSeed, status %d\n",
                           __FUNCTION__, status);
                           
            /* Wait 1 second */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
            retry_count++;
        }
    }while((status != FBE_STATUS_OK) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES));

    return status;
}


/*!***************************************************************
 * @fn fbe_database_get_user_modified_wwn_seed_flag(
 *               fbe_bool_t *user_modified_wwn_seed_flag)
 ****************************************************************
 * @brief
 *  This function will send control code to get user modified wwn
 *  seed flag
 *
 * @param
 *   user_modified_wwn_seed_flag - Buffer to store the return information
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *
 * @version
 *    10/26/2012:   created He,Wei
 *
 ****************************************************************/
fbe_status_t fbe_database_get_user_modified_wwn_seed_flag(fbe_bool_t *user_modified_wwn_seed_flag)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (user_modified_wwn_seed_flag == NULL) 
    {
    
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s:Illegal Parameter\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    status = fbe_database_registry_get_user_modified_wwn_seed_info(user_modified_wwn_seed_flag);

    if (status != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "The usermodifiedWwnSeed is: %d\n", *user_modified_wwn_seed_flag);
                    
    return status;

} 


/*!***************************************************************
 * @fn fbe_database_clear_user_modified_wwn_seed_flag(
 *               void)
 ****************************************************************
 * @brief
 *  This function will send control code to clear user modified wwn
 *  seed flag
 *
 * @param
 *   void
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *
 * @version
 *    10/26/2012:   created He,Wei
 *
 ****************************************************************/
fbe_status_t fbe_database_clear_user_modified_wwn_seed_flag(void)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_database_registry_set_user_modified_wwn_seed_info(FALSE);

    if (status != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Peer SP must clear it's Registry value as well.
    status = fbe_database_cmi_clear_user_modified_wwn_seed();

    if (status != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: usermodifiedWwnSeed is cleared\n", __FUNCTION__);
                    
    return status;
    
}  

/*Get prom wwn seed from prom chip through physical package directly, for test only*/
fbe_status_t fbe_database_get_board_resume_prom_wwnseed_by_pp(fbe_u32_t *wwn_seed)
{
    fbe_topology_control_get_board_t        board;
    fbe_status_t                            status;
    fbe_board_mgmt_get_resume_t             *board_info;

    /* Allocate the non-paged physically contiguous memory. */
    board_info = fbe_transport_allocate_buffer();
    if(board_info == NULL) 
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_zero_memory(board_info, FBE_MEMORY_CHUNK_SIZE);

    board_info->device_type = FBE_DEVICE_TYPE_ENCLOSURE;

    /* First get the object id of the board */
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD,
                                                 &board,
                                                 sizeof(board),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Get the resume info of the board */
    status = fbe_database_send_packet_to_object(FBE_BASE_BOARD_CONTROL_CODE_GET_RESUME,
                                                board_info,
                                                sizeof(fbe_board_mgmt_get_resume_t),
                                                board.board_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get resume prom wwn_seed fail\n",__FUNCTION__);
        return status;
    }

    *wwn_seed = board_info->resume_data.resumePromData.wwn_seed;
    fbe_transport_release_buffer(board_info);
    return status;
 }

/*Set the prom chip wwnseed through physical package directly, for test only*/
fbe_status_t fbe_database_set_board_resume_prom_wwnseed_by_pp(fbe_u32_t wwn_seed)
{

    fbe_resume_prom_cmd_t               prom_write_info;
    RESUME_PROM_STRUCTURE               *resume_body = NULL;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_control_get_board_t    board;

    

    /*Get prom_write_info ready;*/
    fbe_zero_memory(&prom_write_info, sizeof(prom_write_info));
    prom_write_info.deviceType = FBE_DEVICE_TYPE_ENCLOSURE;
    prom_write_info.resumePromField = RESUME_PROM_WWN_SEED;
    prom_write_info.offset = (fbe_u32_t)((fbe_u8_t*)(&(resume_body->wwn_seed)) - (fbe_u8_t*)(resume_body));
    prom_write_info.pBuffer = fbe_transport_allocate_buffer();
    fbe_zero_memory(prom_write_info.pBuffer, FBE_MEMORY_CHUNK_SIZE);
    fbe_copy_memory(prom_write_info.pBuffer, &wwn_seed, sizeof(fbe_u32_t));
    prom_write_info.bufferSize = (fbe_u32_t)sizeof(resume_body->wwn_seed);

     /* First get the object id of the board */
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD,
                                                 &board,
                                                 sizeof(board),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    status = fbe_database_send_packet_to_object(FBE_BASE_BOARD_CONTROL_CODE_RESUME_WRITE,
                                                &prom_write_info,
                                                sizeof(fbe_resume_prom_cmd_t),
                                                board.board_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_PHYSICAL);

    fbe_transport_release_buffer(prom_write_info.pBuffer);

    return status;
}

/*!**************************************************************************************************
@fn fbe_database_persist_validate_entry(fbe_persist_transaction_handle_t transaction_handle,
                                        fbe_persist_entry_id_t entry_id)
*****************************************************************************************************
* @brief
*  This function validates that an existing entry has been persisted. 
* 
* @param transaction_handle    - handle of transaction to write into
* @param entry_id              - a handle into the handle to modify
*
* @return
*  fbe_status_t
*
****************************************************************************************************/
fbe_status_t fbe_database_persist_validate_entry(fbe_persist_transaction_handle_t transaction_handle,
                                                 fbe_persist_entry_id_t entry_id)
{
    fbe_status_t                            status;
    fbe_persist_control_validate_entry_t    validate_entry;
    fbe_bool_t                              active_side = database_common_cmi_is_active();

    if (!active_side) {
        return FBE_STATUS_OK;
    }

    validate_entry.tran_handle = transaction_handle;
    validate_entry.entry_id = entry_id;

    status = fbe_database_send_packet_to_service (FBE_PERSIST_CONTROL_CODE_VALIDATE_ENTRY,
                                                &validate_entry, 
                                                sizeof(fbe_persist_control_validate_entry_t),
                                                FBE_SERVICE_ID_PERSIST,                                                                      
                                                NULL,
                                                0,                                                                  
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: packet send status: 0x%x.\n",
                        __FUNCTION__, status);
    }
    return status;
}

/*!****************************************************************************
 *          fbe_database_persist_read_completion_function()
 ****************************************************************************** 
 * 
 * @brief   Completion function for reading a group of database entries.
 * 
 * @param   read_status - Read status
 * @param   next_entry - Next entry_id expected
 * @param   entries_read - Number of entries in this read data
 * @param   context - Completion context
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_persist_read_completion_function(fbe_status_t read_status, 
                                                                  fbe_persist_entry_id_t next_entry, 
                                                                  fbe_u32_t entries_read, 
                                                                  fbe_persist_completion_context_t context)
{
    database_init_read_context_t    * read_context = (database_init_read_context_t    *)context;

    if (read_status != FBE_STATUS_OK)
    {    
        /* log error*/
        if(read_status == FBE_STATUS_IO_FAILED_RETRYABLE) {
            read_context->status = read_status;
            fbe_semaphore_release(&read_context->sem, 0, 1, FALSE);
            return FBE_STATUS_OK;
        }  
    }
    read_context->next_entry_id= next_entry;
    read_context->entries_read = entries_read;
    read_context->status = FBE_STATUS_OK;
    fbe_semaphore_release(&read_context->sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}
/********************************************************************
 * end fbe_database_persist_read_completion_function()
 ********************************************************************/

/*!****************************************************************************
 *          fbe_database_populate_object_entry_id_from_persist()
 ****************************************************************************** 
 * 
 * @brief   For the entries just read, attempt to locate object id.
 * 
 * @param   persist_object_entry_p - The object entry read from persist
 * @param   transaction_object_entry_p - Transaction entries to populate entry_id for
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/08/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_populate_object_entry_id_from_persist(database_object_entry_t *persist_object_entry_p,
                                                                       database_object_entry_t *transaction_object_entry_p)
{
    fbe_s32_t   index;

    /* Walk thru all transaction entries and check for a match.
     */
    for (index = DATABASE_TRANSACTION_MAX_OBJECTS-1; index >= 0; index--) {
        if ((transaction_object_entry_p[index].header.state == DATABASE_CONFIG_ENTRY_CREATE)                 &&
            (persist_object_entry_p->header.object_id == transaction_object_entry_p[index].header.object_id)    ) {
                database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "database find entry_id: object obj: 0x%x located at entry_id: 0x%llx \n",
                                transaction_object_entry_p[index].header.object_id, persist_object_entry_p->header.entry_id);
                transaction_object_entry_p[index].header.entry_id = persist_object_entry_p->header.entry_id;
                return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_OK;
}
/***********************************************************
 * end fbe_database_populate_object_entry_id_from_persist()
 ***********************************************************/

/*!****************************************************************************
 *          fbe_database_populate_edge_entry_id_from_persist()
 ****************************************************************************** 
 * 
 * @brief   For the entries just read, attempt to locate object id.
 * 
 * @param   persist_edge_entry_p - The edge entry read from persist
 * @param   transaction_edge_entry_p - Transaction entries to populate entry_id for
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/08/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_populate_edge_entry_id_from_persist(database_edge_entry_t *persist_edge_entry_p,
                                                                     database_edge_entry_t *transaction_edge_entry_p)
{
    fbe_s32_t   index;

    /* Walk thru all transaction entries and check for a match.
     */
    for (index = DATABASE_TRANSACTION_MAX_EDGES-1; index >= 0; index--) {
        if ((transaction_edge_entry_p[index].header.state == DATABASE_CONFIG_ENTRY_CREATE)               &&
            (persist_edge_entry_p->header.object_id == transaction_edge_entry_p[index].header.object_id) &&
            (persist_edge_entry_p->server_id == transaction_edge_entry_p[index].server_id)                  ) {
                database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "database find entry: edge obj: [0x%x-0x%x] index: %d located at entry_id: 0x%llx \n",
                                transaction_edge_entry_p[index].header.object_id,
                                transaction_edge_entry_p[index].server_id, 
                                transaction_edge_entry_p[index].client_index,
                                persist_edge_entry_p->header.entry_id);
                transaction_edge_entry_p[index].header.entry_id = persist_edge_entry_p->header.entry_id;
                return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_OK;
}
/***********************************************************
 * end fbe_database_populate_edge_entry_id_from_persist()
 ***********************************************************/

/*!****************************************************************************
 *          fbe_database_populate_user_entry_id_from_persist()
 ****************************************************************************** 
 * 
 * @brief   For the entries just read, attempt to locate object id.
 * 
 * @param   persist_user_entry_p - The user entry read from persist
 * @param   transaction_user_entry_p - Transaction entries to populate entry_id for
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/08/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_populate_user_entry_id_from_persist(database_user_entry_t *persist_user_entry_p,
                                                                     database_user_entry_t *transaction_user_entry_p)
{
    fbe_s32_t   index;

    /* Walk thru all transaction entries and check for a match.
     */
    for (index = DATABASE_TRANSACTION_MAX_USER_ENTRY-1; index >= 0; index--) {
        if ((transaction_user_entry_p[index].header.state == DATABASE_CONFIG_ENTRY_CREATE)               &&
            (persist_user_entry_p->header.object_id == transaction_user_entry_p[index].header.object_id)    ) {
                database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "database find entry_id: user obj: 0x%x located at entry_id: 0x%llx \n",
                                transaction_user_entry_p[index].header.object_id, persist_user_entry_p->header.entry_id);
                transaction_user_entry_p[index].header.entry_id = persist_user_entry_p->header.entry_id;
                return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_OK;
}
/***********************************************************
 * end fbe_database_populate_user_entry_id_from_persist()
 ***********************************************************/

/*!****************************************************************************
 *          fbe_database_populate_global_entry_id_from_persist()
 ****************************************************************************** 
 * 
 * @brief   For the entries just read, attempt to locate object id.
 * 
 * @param   persist_global_entry_p - The global entry read from persist
 * @param   transaction_global_entry_p - Transaction entries to populate entry_id for
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/08/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_populate_global_entry_id_from_persist(database_global_info_entry_t *persist_global_entry_p,
                                                                       database_global_info_entry_t *transaction_global_entry_p)
{
    fbe_s32_t   index;

    /* Walk thru all transaction entries and check for a match.
     */
    for (index = DATABASE_TRANSACTION_MAX_GLOBAL_INFO-1; index >= 0; index--) {
        if ((transaction_global_entry_p[index].header.state == DATABASE_CONFIG_ENTRY_CREATE)                 &&
            (persist_global_entry_p->header.object_id == transaction_global_entry_p[index].header.object_id)    ) {
                database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "database find entry_id: global obj: 0x%x located at entry_id: 0x%llx \n",
                                transaction_global_entry_p[index].header.object_id, persist_global_entry_p->header.entry_id);
                transaction_global_entry_p[index].header.entry_id = persist_global_entry_p->header.entry_id;
                return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_OK;
}
/***********************************************************
 * end fbe_database_populate_global_entry_id_from_persist()
 ***********************************************************/

/*!****************************************************************************
 *          fbe_database_persist_process_read_entries()
 ****************************************************************************** 
 * 
 * @brief   For the entries just read, attempt to locate object id.
 * 
 * @param   read_buffer - Read buffer
 * @param   elements_read - Number of entries read
 * @param   table_type - Database table type
 * @param   transaction_entry_header_p - Array of objects that need the entry_id 
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_persist_process_read_entries(fbe_u8_t *read_buffer, 
                                                              fbe_u32_t elements_read, 
                                                              database_config_table_type_t table_type,
                                                              database_table_header_t *transaction_entry_header_p)


{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u8_t *                      current_data = read_buffer;
    fbe_u32_t                       elements = 0;
    fbe_persist_user_header_t      *user_header = NULL;
    database_table_header_t        *database_table_header = NULL;
    database_object_entry_t        *object_entry_ptr = NULL;
    database_edge_entry_t          *edge_entry_ptr = NULL;
    database_user_entry_t          *user_entry_ptr = NULL;
    database_global_info_entry_t   *global_info_entry_ptr = NULL;

    /*let's go over the read buffer and copy the data to service table */
    for (elements = 0; elements < elements_read; elements++) {
        user_header = (fbe_persist_user_header_t *)current_data;
        current_data += sizeof(fbe_persist_user_header_t);
        database_table_header = (database_table_header_t *)current_data;

        /*! If both the persist header and database headers are 0, then this
         *  is not a valid entry.
         *
         *  @note Not clear if holes in the database are expected.  If not
         *  We could simply note that all the entries have been read.
         */
        if ((user_header->entry_id == 0)           &&
            (database_table_header->entry_id == 0)    ) {
            /* Skip any unused entries.
             */
            database_trace (FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: element: %d is empty\n",
                            __FUNCTION__, elements);
            continue;
        }

        /* Sanity check the entry id */
        if (user_header->entry_id != database_table_header->entry_id) {
            /* We expect persist entry id to be the same as the entry id stored in the database table.
            * If not, log an ERROR and return FBE_STATUS_SERVICE_MODE.
            */ 
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: PersistId 0x%llx != TableId 0x%llx. We have a corrupted database! QUIT booting\n",
                            __FUNCTION__,
                            (unsigned long long)user_header->entry_id,
                            (unsigned long long)database_table_header->entry_id);
            return FBE_STATUS_SERVICE_MODE;
        }

        /* we expect a valid entry */
        if (database_table_header->state != DATABASE_CONFIG_ENTRY_VALID) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: element: %d Entry id 0x%llx has unexpected state 0x%x. Skip it!\n",
                            __FUNCTION__, elements,
                            (unsigned long long)database_table_header->entry_id,
                            database_table_header->state);
            return FBE_STATUS_SERVICE_MODE;
        }

        /* Check if we have located the requested object id.
         */
        switch(table_type) {
        case DATABASE_CONFIG_TABLE_OBJECT:
            object_entry_ptr = (database_object_entry_t *)current_data;
            status = fbe_database_populate_object_entry_id_from_persist(object_entry_ptr,
                                                               (database_object_entry_t *)transaction_entry_header_p);
            break;
        case DATABASE_CONFIG_TABLE_EDGE:
            edge_entry_ptr = (database_edge_entry_t *)current_data;
            status = fbe_database_populate_edge_entry_id_from_persist(edge_entry_ptr,
                                                             (database_edge_entry_t *)transaction_entry_header_p);
            break;
        case DATABASE_CONFIG_TABLE_USER:
            user_entry_ptr = (database_user_entry_t *)current_data;
            status = fbe_database_populate_user_entry_id_from_persist(user_entry_ptr,
                                                             (database_user_entry_t *)transaction_entry_header_p);
            break;
        case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
            global_info_entry_ptr = (database_global_info_entry_t *)current_data;
            status = fbe_database_populate_global_entry_id_from_persist(global_info_entry_ptr,
                                                               (database_global_info_entry_t *)transaction_entry_header_p);
            break;
        case DATABASE_CONFIG_TABLE_SYSTEM_SPARE:
        default:
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Table type:%d is not supported!\n",
                            __FUNCTION__, table_type);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Failed to populate table type:%d - status: 0x%x\n",
                            __FUNCTION__, table_type, status);
            return status;
        }

        /*go to next entry*/
        current_data += (FBE_PERSIST_USER_DATA_BYTES_PER_ENTRY - sizeof(fbe_persist_user_header_t));
    }
    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_database_persist_process_read_entries()
 *******************************************************/

/***************************************************************************************************
    fbe_database_populate_transaction_entry_ids_from_persist()
*****************************************************************************************************
*
* @brief
*  This function reads from persist service to locate the entry_id associated with a particular 
*  table and object id. 
* 
* @param    table_type - Table type to validate 
* @param   transaction_entry_header_p - Array of objects that need the entry_id 
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
fbe_status_t fbe_database_populate_transaction_entry_ids_from_persist(database_config_table_type_t table_type,
                                                                      database_table_header_t *transaction_entry_header_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_status_t                    wait_status = FBE_STATUS_OK;
    fbe_u8_t                       *read_buffer;
    database_init_read_context_t   *read_context;
    fbe_u32_t                       total_entries_read = 0;
    fbe_persist_sector_type_t       persist_sector_type;

    /* Allocate the read buffer.
     */
    read_context = (database_init_read_context_t *)fbe_memory_native_allocate(sizeof(database_init_read_context_t));
    if (read_context == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Can't allocate read_context\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    read_buffer = (fbe_u8_t *)fbe_memory_ex_allocate(FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE);
    if (read_buffer == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Can't allocate read_buffer\n",
            __FUNCTION__);
        fbe_memory_native_release(read_context);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Load Table 0x%x\n",
                    __FUNCTION__, table_type);

    /* look up the sector type */
    fbe_semaphore_init(&read_context->sem, 0, 1);
    persist_sector_type = database_common_map_table_type_to_persist_sector(table_type);
    if (persist_sector_type != FBE_PERSIST_SECTOR_TYPE_INVALID) {
        /*start to read from the first one*/
        read_context->next_entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
        do {
            status = fbe_database_persist_read_sector(persist_sector_type,
                read_buffer,
                FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE,
                read_context->next_entry_id,
                fbe_database_persist_read_completion_function,
                (fbe_persist_completion_context_t)read_context);
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                /* log the error */
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read from sector: %d failed! next_entry 0x%llX\n",
                                persist_sector_type,
                                (unsigned long long)read_context->next_entry_id );
                fbe_semaphore_destroy(&read_context->sem);
                fbe_memory_native_release(read_context);
                fbe_memory_ex_release(read_buffer);
                return status;
            }

            wait_status = fbe_semaphore_wait_ms(&read_context->sem, 30000);
            if(wait_status == FBE_STATUS_TIMEOUT)
            {
                /* log the error */
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read from sector: %d TIMEOUT! next_entry 0x%llX\n",
                                persist_sector_type,
                                (unsigned long long)read_context->next_entry_id );
                fbe_memory_native_release(read_context);
                fbe_memory_ex_release(read_buffer);
                return status;
            }
            if (read_context->status == FBE_STATUS_IO_FAILED_RETRYABLE) 
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read from sector: %d Retryable error next_entry 0x%llX\n",
                                persist_sector_type,
                                (unsigned long long)read_context->next_entry_id );
                fbe_semaphore_destroy(&read_context->sem);
                fbe_memory_native_release(read_context);
                fbe_memory_ex_release(read_buffer);
                return FBE_STATUS_IO_FAILED_RETRYABLE;
            }
            if (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ) {
                database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read %d entries from sector: %d up to entry 0x%llX\n",
                                read_context->entries_read,
                                persist_sector_type,
                                (unsigned long long)(read_context->next_entry_id-1));
            }else{
                database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read %d entries from sector: %d up to sector end\n",
                                read_context->entries_read,
                                persist_sector_type);
            }

            /*now let's copy the read buffer content to the service table */
            status = fbe_database_persist_process_read_entries(read_buffer, read_context->entries_read, 
                                                               table_type, transaction_entry_header_p);
            if (status != FBE_STATUS_OK) {
                /* Error happened when loading the database, quit and hault the system in this state*/
                fbe_semaphore_destroy(&read_context->sem);
                fbe_memory_native_release(read_context);
                fbe_memory_ex_release(read_buffer);
                return status;
            }

            /*increment only AFTER copying*/
            total_entries_read+= read_context->entries_read;
        } while (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ);
    }

    /* Cleanup */
    fbe_semaphore_destroy(&read_context->sem);
    fbe_memory_native_release(read_context);
    fbe_memory_ex_release(read_buffer);

    return status;
}
/***************************************************************
 * end fbe_database_populate_transaction_entry_ids_from_persist()
 ***************************************************************/


/***********************************************
* end file fbe_database_persist_interface.c
***********************************************/
