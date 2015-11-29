/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_payload_ex_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the storage extent package payload.
 *
 * @version
 *   5/19/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_payload_ex.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_winddk.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_payload_ex_init()
 ****************************************************************
 * @brief
 *  This function initializes the payload structure by zeroing out
 *  all its memory.
 *
 * @param payload - The sep payload to init.
 *
 * @return FBE_STATUS_OK - Always.
 *
 ****************************************************************/
fbe_status_t 
fbe_payload_ex_init(fbe_payload_ex_t * payload)
{	
    /* We should implement more sophisticated init */
    fbe_zero_memory(payload, sizeof(fbe_payload_ex_t));

	payload->next_operation = NULL;
	payload->current_operation = NULL;

	payload->performace_stats_p = NULL;
	payload->verify_error_counts_p = NULL;

    payload->pvd_object_id = FBE_OBJECT_ID_INVALID;

	payload->retry_wait_msecs = 0;
	payload->scsi_error_code = 0;
	payload->sense_data = 0;

	//fbe_zero_memory(&payload->iots, sizeof(fbe_raid_iots_t));
	//fbe_zero_memory(&payload->siots, sizeof(fbe_raid_siots_t));

    payload->operation_memory_index = 0;
    //payload->iots_ptr = &payload->iots;
    //payload->siots_ptr = &payload->siots;

    payload->sg_list = NULL;
    payload->sg_list_count = 0; 

    payload->pre_sg_array[0].address = NULL;
    payload->pre_sg_array[0].count = 0;

    payload->post_sg_array[0].address = NULL;
    payload->post_sg_array[0].count = 0;

	payload->payload_memory_operation = NULL;

    payload->key_handle = FBE_INVALID_KEY_HANDLE;
    return FBE_STATUS_OK;
}


/* Control operation */
/*!**************************************************************
 * @fn fbe_payload_ex_allocate_control_operation(fbe_payload_ex_t * payload)
 ****************************************************************
 * @brief
 *  Allocate a new control operation from the input payload structure.
 *  We assume there is enough space in the payload, an error is
 *  returned if not.
 *
 * @param  payload - The payload to allocate this control operation
 *                   out of.  We assume that there is enough space
 *                   still reserved in the payload for this control operation.
 *
 * @return fbe_payload_control_operation_t * -
 *         The control operation that was just allocated.
 *         OR
 *         NULL if an error occurs.
 *
 ****************************************************************/

fbe_payload_control_operation_t * 
fbe_payload_ex_allocate_control_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated", __FUNCTION__);
        return NULL;
    }

    /* Check if we have free control operation */
    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index > sizeof(fbe_payload_control_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];
        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_CONTROL_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;
        payload->operation_memory_index += sizeof(fbe_payload_control_operation_t);
		/* This is temporary HACK. We need to make shur that all clients update the status */
		((fbe_payload_control_operation_t * )payload_operation_header)->status = 0;
		((fbe_payload_control_operation_t * )payload_operation_header)->status_qualifier = 0;
        return (fbe_payload_control_operation_t * )payload_operation_header;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);
    return NULL;
}

/*!**************************************************************
 * @fn fbe_payload_ex_release_control_operation(
 *               fbe_payload_ex_t *payload,
 *               fbe_payload_control_operation_t * payload_control_operation)
 ****************************************************************
 * @brief
 *  This function is used to release back to the payload a control
 *  operation that was in use.
 *  This function will "unwind" the payload to point to the previously
 *  allocated operation.
 * 
 *  Note that this call is the only way to "unwind the payload" to get back to
 *  the prior stack level in the payload.
 *
 * @param payload - The payload to free for.
 * @param payload_control_operation - The specific payload to free.
 *        This function checks to make sure that the input control operation
 *        ptr is equal to the current operation that is in use.  If not,
 *        then FBE_STATUS_GENERIC_FAILURE is returned.
 *
 * @return fbe_status_t FBE_STATUS_OK if no error.
 *                      FBE_STATUS_GENERIC_FAILURE if error.
 *
 ****************************************************************/
fbe_status_t 
fbe_payload_ex_release_control_operation(fbe_payload_ex_t * payload, fbe_payload_control_operation_t * payload_control_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    if (payload_control_operation == NULL) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload_control_operation is NULL", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_operation_header = (fbe_payload_operation_header_t *)payload_control_operation;

    /* Check if our operation is control operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free control operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;

    payload->operation_memory_index -= sizeof(fbe_payload_control_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_control_operation){ 
        return FBE_STATUS_OK;
    } else if(payload->current_operation == (fbe_payload_operation_header_t *)payload_control_operation){
        /* Shift operation stack */
        payload->current_operation = payload_operation_header->prev_header;
        return FBE_STATUS_OK;
    } else {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation pointer", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

/*!**************************************************************
 * @fn fbe_payload_ex_get_control_operation(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function fetches the current active payload from the payload structure,
 *  which is assumed to be a control operation.  If the current active payload is
 *  not a control operation, then a NULL is returned.
 * 
 *  Note that this function assumes that after
 *  fbe_payload_ex_allocate_control_operation() was called to allocate the control
 *  operation, then fbe_payload_ex_increment_control_operation_level() was also
 *  called to make the current control operation "active".  Typically the sending
 *  of the control operation on the edge calls the increment, although if no
 *  edge is being used, then the client might need to call the increment
 *  function.
 *
 * @param payload - The payload to get the control operation for.
 *
 * @return fbe_payload_control_operation_t * - The current active payload, which
 *                                           should be a control operation.
 *         NULL is returned if the current active payload is not a control
 *         operation.
 *
 ****************************************************************/
fbe_payload_control_operation_t * 
fbe_payload_ex_get_control_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s NULL payload operation\n", __FUNCTION__);
        return NULL;
    }
    /* Check if current operation is control operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X\n", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_control_operation_t *)payload->current_operation;
}

/*!**************************************************************
 * @fn fbe_payload_ex_get_present_control_operation(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 * 	This function fetches the present control operation from the payload
 * 	structure.
 * 	It returns NULL if it does not find any block operation associated
 * 	with payload.
 * 
 * @param payload - The payload to get the block operation for.
 *
 * @return fbe_payload_block_operation_t * - The previous payload, which
 *                                           should be a block operation.
 * 		   NULL is returned if the payload has no associated block
 *		   operation.
 *
 ****************************************************************/
fbe_payload_control_operation_t * 
fbe_payload_ex_get_present_control_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * current_operation_header = NULL;

    current_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    while(current_operation_header != NULL)
    {
        /* break the loop if the operation is block operation. */
        if(current_operation_header->payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION)
        {
            break;
        }

        /* get the next prevous operation. */
        current_operation_header = (fbe_payload_operation_header_t *)current_operation_header->prev_header;
    }

    /* return the present block operation from payload */
    return (fbe_payload_control_operation_t *)current_operation_header;
}

/*!**************************************************************
 * @fn fbe_payload_ex_increment_control_operation_level(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function moves the level in the payload to make the control operation the
 *  active payload.
 * 
 *  Note that after fbe_payload_ex_allocate_control_operation() is called to
 *  allocate the block operation, then this function must also be called to make
 *  the current control operation "active".  Typically the sending of the control
 *  operation via service manager calls this function.
 *
 * @param payload - The payload to increment the block operation for.
 *
 * @return FBE_STATUS_OK - If no error.
 *         FBE_STATUS_GENERIC_FAILURE - If either no payload was allocated or if
 *                                      the allocated payload is not a control
 *                                      operation.
 *
 ****************************************************************/
fbe_status_t
fbe_payload_ex_increment_control_operation_level(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* If packet is traversed we should not increment operation level */
    if(payload->next_operation != NULL){ /* Do nothing for traverse */
        payload_operation_header = (fbe_payload_operation_header_t *)payload->next_operation;

        /* Check if current operation is block operation */
        if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_payload_ex_get_prev_control_operation(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function fetches the previous control operation from the payload structure,
 *  which is assumed to be a control operation.  If the previous payload is
 *  not a control operation, then a NULL is returned.
 * 
 * @param payload - The payload to get the control operation for.
 *
 * @return fbe_payload_control_operation_t * - The previous payload, which
 *                                           should be a control operation.
 *         NULL is returned if the previous payload is not a control
 *         operation.
 *
 ****************************************************************/
fbe_payload_control_operation_t * 
fbe_payload_ex_get_prev_control_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * current_operation_header = NULL;
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    current_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;
    if(current_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation NULL\n", __FUNCTION__);
        return NULL;
    }

    payload_operation_header = (fbe_payload_operation_header_t *)current_operation_header->prev_header;
    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid payload_operation_header\n", __FUNCTION__);
        return NULL;
    }

    /* Check if operation is control operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_control_operation_t *)payload_operation_header;
}

/*!*****************************************************************************
 * fbe_payload_ex_get_any_control_operation()
 *******************************************************************************
 * @brief
 * Returns the previous allocated control operation in the payload.
 *
 * @param payload - pointer to a control packet from the scheduler
 * 
 * @return fbe_payload_control_operation_t - The first control operation
 *         found in the payload previous header chain or NULL if not found.
 *   
 * @author
 *  3/1/2013 - Created. Rob Foley
 *****************************************************************************/
fbe_payload_control_operation_t * 
fbe_payload_ex_get_any_control_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t *   payload_operation_header = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    
    payload_operation_header = (fbe_payload_operation_header_t *) payload->current_operation;

    while (payload_operation_header) {

        if (payload_operation_header->payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION) {
            payload_control_operation = (fbe_payload_control_operation_t *) payload_operation_header;
            break;
        }

        payload_operation_header = payload_operation_header->prev_header;
    }

   return payload_control_operation; 
}
/* block_operation */

/*!**************************************************************
 * @fn fbe_payload_ex_allocate_block_operation(fbe_payload_ex_t * payload)
 ****************************************************************
 * @brief
 *  Allocate a new block operation from the input payload structure.
 *  We assume there is enough space in the payload, an error is
 *  returned if not.
 *
 * @param  payload - The payload to allocate this block operation
 *                   out of.  We assume that there is enough space
 *                   still reserved in the payload for this block operation.
 *                   If we have already reserved
 *                   @ref FBE_PAYLOAD_EX_MAX_BLOCK_OPERATIONS
 *                   from the payload then an error is returned.
 *
 * @return fbe_payload_block_operation_t * -
 *         The block operation that was just allocated.
 *         OR
 *         NULL if an error occurs.
 *
 ****************************************************************/

fbe_payload_block_operation_t * 
fbe_payload_ex_allocate_block_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;
    fbe_payload_block_operation_t * block_operation;

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated", __FUNCTION__);
        return NULL;
    }

    /* Check if we have free  block operation */
    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index > sizeof(fbe_payload_block_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];

        /* This is ugly and should be reworked */
		//fbe_zero_memory(payload_operation_header, sizeof(fbe_payload_block_operation_t));

        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_BLOCK_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;
        payload->operation_memory_index += sizeof(fbe_payload_block_operation_t);
        block_operation = (fbe_payload_block_operation_t * )payload_operation_header;
        block_operation->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
		block_operation->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
        return block_operation;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);
    return NULL;
}

/*!**************************************************************
 * @fn fbe_payload_ex_release_block_operation(
 *               fbe_payload_ex_t *payload,
 *               fbe_payload_block_operation_t * payload_block_operation)
 ****************************************************************
 * @brief
 *  This function is used to release back to the payload a block
 *  operation that was in use.
 *  This function will "unwind" the payload to point to the previously
 *  allocated operation.
 * 
 *  Note that this call is the only way to "unwind the payload" to get back to
 *  the prior stack level in the payload.
 *
 * @param payload - The payload to free for.
 * @param payload_block_operation - The specific payload to free.
 *        This function checks to make sure that the input block operation
 *        ptr is equal to the current operation that is in use.  If not,
 *        then FBE_STATUS_GENERIC_FAILURE is returned.
 *
 * @return fbe_status_t FBE_STATUS_OK if no error.
 *                      FBE_STATUS_GENERIC_FAILURE if error.
 *
 ****************************************************************/

fbe_status_t 
fbe_payload_ex_release_block_operation(fbe_payload_ex_t * payload, fbe_payload_block_operation_t * payload_block_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    if (payload_block_operation == NULL) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload_block_operation is NULL", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_operation_header = (fbe_payload_operation_header_t *)payload_block_operation;

    /* Check if current operation is block operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_BLOCK_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free block operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    payload->operation_memory_index -= sizeof(fbe_payload_block_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_block_operation){ 
        return FBE_STATUS_OK;
    } else if (payload->current_operation == (fbe_payload_operation_header_t *)payload_block_operation){ 
        /* Shift operation stack */
        payload->current_operation = payload_operation_header->prev_header;
        return FBE_STATUS_OK;
    } else {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation pointer", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}


/*!**************************************************************
 * @fn fbe_payload_ex_get_prev_block_operation(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function fetches the previous block operation from the payload structure,
 *  which is assumed to be a block operation.  If the previous payload is
 *  not a block operation, then a NULL is returned.
 * 
 * @param payload - The payload to get the block operation for.
 *
 * @return fbe_payload_block_operation_t * - The previous payload, which
 *                                           should be a block operation.
 *         NULL is returned if the previous payload is not a block
 *         operation.
 *
 ****************************************************************/
fbe_payload_block_operation_t * 
fbe_payload_ex_get_prev_block_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * current_operation_header = NULL;
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    current_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(current_operation_header == NULL){
        return NULL;
    }

    payload_operation_header = (fbe_payload_operation_header_t *)current_operation_header->prev_header;
    if(payload_operation_header == NULL){
        return NULL;
    }

    /* Check if operation is block operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_BLOCK_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_block_operation_t *)payload_operation_header;
}

fbe_payload_block_operation_t * 
fbe_payload_ex_search_prev_block_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * current_operation_header = NULL;
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    current_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(current_operation_header == NULL){
        return NULL;
    }

    payload_operation_header = (fbe_payload_operation_header_t *)current_operation_header->prev_header;
    if(payload_operation_header == NULL){
        return NULL;
    }

    while(payload_operation_header != NULL)
    {
        /* break the loop if the operation is block operation. */
        if(payload_operation_header->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
        {
            break;
        }

        /* get the next prevous operation. */
        payload_operation_header = (fbe_payload_operation_header_t *)payload_operation_header->prev_header;
    }

    return (fbe_payload_block_operation_t *)payload_operation_header;
}

fbe_payload_block_operation_t * 
fbe_payload_ex_get_next_block_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * next_operation_header = NULL;

    next_operation_header = (fbe_payload_operation_header_t *)payload->next_operation;

    if(next_operation_header == NULL){
        return NULL;
    }

    /* Check if operation is block operation */
    if(next_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_BLOCK_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid next operation opcode %X", __FUNCTION__, next_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_block_operation_t *)next_operation_header;
}

/*!**************************************************************
 * @fn fbe_payload_ex_increment_block_operation_level(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function moves the level in the payload to make the block operation the
 *  active payload.
 * 
 *  Note that after fbe_payload_ex_allocate_block_operation() is called to
 *  allocate the block operation, then this function must also be called to make
 *  the current block operation "active".  Typically the sending of the block
 *  operation on the edge calls this function, although if no edge is being
 *  used, then the client might need to call this function.
 *
 * @param payload - The payload to increment the block operation for.
 *
 * @return FBE_STATUS_OK - If no error.
 *         FBE_STATUS_GENERIC_FAILURE - If either no payload was allocated or if
 *                                      the allocated payload is not a block
 *                                      operation.
 *
 ****************************************************************/
fbe_status_t
fbe_payload_ex_increment_block_operation_level(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* If packet is traversed we should not increment operation level */
    if(payload->next_operation != NULL){
        payload_operation_header = (fbe_payload_operation_header_t *)payload->next_operation;

        /* Check if current operation is block operation */
        if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_BLOCK_OPERATION){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;
    }
    return FBE_STATUS_OK;
}

/* metadata operation */
fbe_payload_metadata_operation_t * 
fbe_payload_ex_allocate_metadata_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated", __FUNCTION__);
        return NULL;
    }

    /* Check if we have free metadata operation */
    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index > sizeof(fbe_payload_metadata_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];
        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_METADATA_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;
        payload->operation_memory_index += sizeof(fbe_payload_metadata_operation_t);
        return (fbe_payload_metadata_operation_t * )payload_operation_header;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);
    return NULL;
}

fbe_status_t 
fbe_payload_ex_release_metadata_operation(fbe_payload_ex_t * payload, fbe_payload_metadata_operation_t * payload_metadata_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    if (payload_metadata_operation == NULL) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload_metadata_operation is NULL", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_operation_header = (fbe_payload_operation_header_t *)payload_metadata_operation;

    /* Check if our operation is metadata operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free metadata operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    payload->operation_memory_index -= sizeof(fbe_payload_metadata_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_metadata_operation){ 
        return FBE_STATUS_OK;
    } else if(payload->current_operation == (fbe_payload_operation_header_t *)payload_metadata_operation){
        /* Shift operation stack */
        payload->current_operation = payload_operation_header->prev_header;
        return FBE_STATUS_OK;
    } else {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation pointer", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

fbe_payload_metadata_operation_t * 
fbe_payload_ex_get_metadata_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s NULL payload operation\n", __FUNCTION__);
        return NULL;
    }
    /* Check if current operation is metadata operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X\n", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_metadata_operation_t *)payload->current_operation;
}

fbe_payload_metadata_operation_t * 
fbe_payload_ex_check_metadata_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s NULL payload operation\n", __FUNCTION__);
        return NULL;
    }
    /* Check if current operation is metadata operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s current operation opcode %X is not metadata operation.\n", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_metadata_operation_t *)payload->current_operation;
}

fbe_status_t
fbe_payload_ex_increment_metadata_operation_level(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* If packet is traversed we should not increment operation level */
    if(payload->next_operation != NULL){ /* Do nothing for traverse */
        payload_operation_header = (fbe_payload_operation_header_t *)payload->next_operation;

        /* Check if current operation is block operation */
        if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;
    }

    return FBE_STATUS_OK;
}

/*!*****************************************************************************
 * fbe_payload_ex_get_prev_metadata_operation()
 *******************************************************************************
 * @brief
 * Returns the previous allocated metadata operation in the payload.
 *
 * @param payload - pointer to a control packet from the scheduler
 * 
 * @return fbe_payload_metadata_operation_t - The first metadata operation
 *         found in the payload previous header chain or NULL if not found.
 *   
 * @author
 *   1/18/2011 - Created. Daniel Cummins.
 *****************************************************************************/
fbe_payload_metadata_operation_t * 
fbe_payload_ex_get_prev_metadata_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t *   payload_operation_header = NULL;
    fbe_payload_metadata_operation_t * payload_metadata_operation = NULL;
    
    payload_operation_header = (fbe_payload_operation_header_t *) payload->current_operation;

    while (payload_operation_header) {

        if (payload_operation_header->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION) {
            payload_metadata_operation = (fbe_payload_metadata_operation_t *) payload_operation_header;
            break;
        }

        payload_operation_header = payload_operation_header->prev_header;
    }

    if(payload_operation_header == NULL) 
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Payload operation header is null",
				 __FUNCTION__);
    }

   return payload_metadata_operation; 
}
/*******************************************************************************
 * end fbe_payload_ex_get_prev_metadata_operation()
 *******************************************************************************/

/*!*****************************************************************************
 * fbe_payload_ex_get_any_metadata_operation()
 *******************************************************************************
 * @brief
 * Returns the previous allocated metadata operation in the payload.
 *
 * @param payload - pointer to a control packet from the scheduler
 * 
 * @return fbe_payload_metadata_operation_t - The first metadata operation
 *         found in the payload previous header chain or NULL if not found.
 *   
 * @author
 *  3/1/2013 - Created. Rob Foley
 *****************************************************************************/
fbe_payload_metadata_operation_t * 
fbe_payload_ex_get_any_metadata_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t *   payload_operation_header = NULL;
    fbe_payload_metadata_operation_t * payload_metadata_operation = NULL;
    
    payload_operation_header = (fbe_payload_operation_header_t *) payload->current_operation;

    while (payload_operation_header) {

        if (payload_operation_header->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION) {
            payload_metadata_operation = (fbe_payload_metadata_operation_t *) payload_operation_header;
            break;
        }

        payload_operation_header = payload_operation_header->prev_header;
    }

   return payload_metadata_operation; 
}
/*******************************************************************************
 * end fbe_payload_ex_get_any_metadata_operation()
 *******************************************************************************/
 
/* lba_lock operation */
fbe_payload_stripe_lock_operation_t * 
fbe_payload_ex_allocate_stripe_lock_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated", __FUNCTION__);
        return NULL;
    }

    /* Check if we have free lba_lock operation */
    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index > sizeof(fbe_payload_stripe_lock_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];
        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;
        payload->operation_memory_index += sizeof(fbe_payload_stripe_lock_operation_t);

        fbe_queue_element_init(&((fbe_payload_stripe_lock_operation_t * )payload_operation_header)->queue_element);
        return (fbe_payload_stripe_lock_operation_t * )payload_operation_header;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);
    return NULL;
}

fbe_status_t 
fbe_payload_ex_release_stripe_lock_operation(fbe_payload_ex_t * payload, fbe_payload_stripe_lock_operation_t * payload_stripe_lock_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    if (payload_stripe_lock_operation == NULL) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload_stripe_lock_operation is NULL", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    payload_operation_header = (fbe_payload_operation_header_t *)payload_stripe_lock_operation;

    /* Check if our operation is lba_lock operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check if our queue_element is not on the queue */
    if(fbe_queue_is_element_on_queue(&payload_stripe_lock_operation->queue_element)){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Operation opcode %X on the queue", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_stripe_lock_operation->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_INVALID;


    /* Free lba_lock operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    payload->operation_memory_index -= sizeof(fbe_payload_stripe_lock_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_stripe_lock_operation){ 
        payload->next_operation = NULL;
        return FBE_STATUS_OK;
    } else if(payload->current_operation == (fbe_payload_operation_header_t *)payload_stripe_lock_operation){
        /* Shift operation stack */
        payload->current_operation = payload_operation_header->prev_header;
        return FBE_STATUS_OK;
    } else {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation pointer", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

fbe_status_t
fbe_payload_ex_increment_stripe_lock_operation_level(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* If packet is traversed we should not increment operation level */
    if(payload->next_operation != NULL){ /* Do nothing for traverse */
        payload_operation_header = (fbe_payload_operation_header_t *)payload->next_operation;

        /* Check if current operation is block operation */
        if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;
    }

    return FBE_STATUS_OK;
}

fbe_payload_stripe_lock_operation_t * 
fbe_payload_ex_get_prev_stripe_lock_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * current_operation_header = NULL;
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    current_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;
    if(current_operation_header == NULL){
        return NULL;
    }

    payload_operation_header = (fbe_payload_operation_header_t *)current_operation_header->prev_header;
    if(payload_operation_header == NULL){
        return NULL;
    }

    /* Check if operation is lba_lock operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_stripe_lock_operation_t *)payload_operation_header;
}


/******************************** Physical Payload **********************************************/


/* Discovery_operation */

/*!**************************************************************
 * @fn fbe_payload_ex_allocate_discovery_operation(fbe_payload_ex_t * payload)
 ****************************************************************
 * @brief
 *  Allocate a new discovery operation from the input payload structure.
 *  We assume there is enough space in the payload, an error is
 *  returned if not.
 *
 * @param  payload - The payload to allocate this discovery operation
 *                   out of.  We assume that there is enough space
 *                   still reserved in the payload for this discovery operation.
 *
 * @return fbe_payload_discovery_operation_t * -
 *         The discovery operation that was just allocated.
 *         OR
 *         NULL if an error occurs.
 *
 ****************************************************************/
fbe_payload_discovery_operation_t * 
fbe_payload_ex_allocate_discovery_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated", __FUNCTION__);
        return NULL;
    }


    /* Check if discovery operation is already allocated */
    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index > sizeof(fbe_payload_discovery_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];
        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_DISCOVERY_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;
        payload->operation_memory_index += sizeof(fbe_payload_discovery_operation_t);
        return (fbe_payload_discovery_operation_t * )payload_operation_header;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);

    return NULL;
}

/*!**************************************************************
 * @fn fbe_payload_ex_release_discovery_operation(
 *               fbe_payload_ex_t *payload,
 *               fbe_payload_discovery_operation_t * payload_discovery_operation)
 ****************************************************************
 * @brief
 *  This function is used to release back to the payload a discovery
 *  operation that was in use.
 *  This function will "unwind" the payload to point to the previously
 *  allocated operation.
 * 
 *  Note that this call is the only way to "unwind the payload" to get back to
 *  the prior stack level in the payload.
 *
 * @param payload - The payload to free for.
 * @param payload_discovery_operation - The specific payload to free.
 *        This function checks to make sure that the input discovery operation
 *        ptr is equal to the current operation that is in use.  If not,
 *        then FBE_STATUS_GENERIC_FAILURE is returned.
 *
 * @return fbe_status_t FBE_STATUS_OK if no error.
 *                      FBE_STATUS_GENERIC_FAILURE if error.
 *
 ****************************************************************/
fbe_status_t 
fbe_payload_ex_release_discovery_operation(fbe_payload_ex_t * payload, fbe_payload_discovery_operation_t * payload_discovery_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload_discovery_operation;

    /* Check if our operation is discovery operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_DISCOVERY_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free discovery operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    payload->operation_memory_index -= sizeof(fbe_payload_discovery_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_discovery_operation){ 
        return FBE_STATUS_OK;
    } else if(payload->current_operation == (fbe_payload_operation_header_t *)payload_discovery_operation){
        /* Shift operation stack */
        payload->current_operation = payload_operation_header->prev_header;
        return FBE_STATUS_OK;
    } else {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation pointer", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

/*!**************************************************************
 * @fn fbe_payload_ex_get_discovery_operation(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function fetches the current active payload from the payload structure,
 *  which is assumed to be a discovery operation.  If the current active payload is
 *  not a discovery operation, then a NULL is returned.
 * 
 *  Note that this function assumes that after
 *  fbe_payload_ex_allocate_discovery_operation() was called to allocate the discovery
 *  operation, then fbe_payload_ex_increment_discovery_operation_level() was also
 *  called to make the current discovery operation "active".  Typically the sending
 *  of the discovery operation on the edge calls the increment, although if no
 *  edge is being used, then the client might need to call the increment
 *  function.
 *
 * @param payload - The payload to get the discovery operation for.
 *
 * @return fbe_payload_discovery_operation_t * - The current active payload, which
 *                                           should be a discovery operation.
 *         NULL is returned if the current active payload is not a discovery
 *         operation.
 *
 ****************************************************************/
fbe_payload_discovery_operation_t * 
fbe_payload_ex_get_discovery_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s NULL payload operation", __FUNCTION__);
        return NULL;
    }
    /* Check if current operation is discovery operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_DISCOVERY_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_discovery_operation_t *)payload->current_operation;
}


/*!**************************************************************
 * @fn fbe_payload_ex_increment_discovery_operation_level(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function moves the level in the payload to make the discovery operation the
 *  active payload.
 * 
 *  Note that after fbe_payload_ex_allocate_discovery_operation() is called to
 *  allocate the discovery operation, then this function must also be called to make
 *  the current discovery operation "active".  Typically the sending of the
 *  discovery operation on the edge calls this function, although if no edge is
 *  being used, then the client might need to call this function.
 *
 * @param payload - The payload to increment the discovery operation for.
 *
 * @return FBE_STATUS_OK - If no error.
 *         FBE_STATUS_GENERIC_FAILURE - If either no payload was allocated or if
 *                                      the allocated payload is not a discovery
 *                                      operation.
 *
 ****************************************************************/
fbe_status_t
fbe_payload_ex_increment_discovery_operation_level(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* If packet is traversed we should not increment operation level */
    if(payload->next_operation != NULL){
        payload_operation_header = (fbe_payload_operation_header_t *)payload->next_operation;

        /* Check if current operation is discovery operation */
        if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_DISCOVERY_OPERATION){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;
    }
    return FBE_STATUS_OK;
}



/* cdb_operation */

/*!**************************************************************
 * @fn fbe_payload_ex_allocate_cdb_operation(fbe_payload_ex_t * payload)
 ****************************************************************
 * @brief
 *  Allocate a new cdb operation from the input payload structure.
 *  We assume there is enough space in the payload, an error is
 *  returned if not.
 *
 * @param  payload - The payload to allocate this cdb operation
 *                   out of.  We assume that there is enough space
 *                   still reserved in the payload for this cdb operation.
 *
 * @return fbe_payload_cdb_operation_t * -
 *         The cdb operation that was just allocated.
 *         OR
 *         NULL if an error occurs.
 *
 ****************************************************************/
fbe_payload_cdb_operation_t * 
fbe_payload_ex_allocate_cdb_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated", __FUNCTION__);
        return NULL;
    }

    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index > sizeof(fbe_payload_cdb_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];

        /* This is ugly and should be reworked */
        //fbe_zero_memory(payload_operation_header, sizeof(fbe_payload_cdb_operation_t));

        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_CDB_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;

        ((fbe_payload_cdb_operation_t * )payload_operation_header)->payload_cdb_scsi_status = 0;
        ((fbe_payload_cdb_operation_t * )payload_operation_header)->port_request_status = 0;
        ((fbe_payload_cdb_operation_t * )payload_operation_header)->port_recovery_status = 0;

        payload->operation_memory_index += sizeof(fbe_payload_cdb_operation_t);
        return (fbe_payload_cdb_operation_t * )payload_operation_header;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);
    return NULL;

}

/*!**************************************************************
 * @fn fbe_payload_ex_release_cdb_operation(
 *               fbe_payload_ex_t *payload,
 *               fbe_payload_cdb_operation_t * payload_cdb_operation)
 ****************************************************************
 * @brief
 *  This function is used to release back to the payload a cdb
 *  operation that was in use.
 *  This function will "unwind" the payload to point to the previously
 *  allocated operation.
 * 
 *  Note that this call is the only way to "unwind the payload" to get back to
 *  the prior stack level in the payload.
 *
 * @param payload - The payload to free for.
 * @param payload_cdb_operation - The specific payload to free.
 *        This function checks to make sure that the input cdb operation
 *        ptr is equal to the current operation that is in use.  If not,
 *        then FBE_STATUS_GENERIC_FAILURE is returned.
 *
 * @return fbe_status_t FBE_STATUS_OK if no error.
 *                      FBE_STATUS_GENERIC_FAILURE if error.
 *
 ****************************************************************/
fbe_status_t 
fbe_payload_ex_release_cdb_operation(fbe_payload_ex_t * payload, fbe_payload_cdb_operation_t * payload_cdb_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload_cdb_operation;

    /* Check if our operation is cdb operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CDB_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free cdb operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    payload->operation_memory_index -= sizeof(fbe_payload_cdb_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_cdb_operation){ 
        return FBE_STATUS_OK;
    }else if (payload->current_operation == (fbe_payload_operation_header_t *)payload_cdb_operation){ 
        /* Shift operation stack */
        payload->current_operation = payload_operation_header->prev_header;
        return FBE_STATUS_OK;
    } else {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation pointer", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

/*!**************************************************************
 * @fn fbe_payload_ex_increment_cdb_operation_level(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function moves the level in the payload to make the cdb operation the
 *  active payload.
 * 
 *  Note that after fbe_payload_ex_allocate_cdb_operation() is called to
 *  allocate the cdb operation, then this function must also be called to make
 *  the current cdb operation "active".  Typically the sending of the cdb
 *  operation on the edge calls this function, although if no edge is being
 *  used, then the client might need to call this function.
 *
 * @param payload - The payload to increment the cdb operation for.
 *
 * @return FBE_STATUS_OK - If no error.
 *         FBE_STATUS_GENERIC_FAILURE - If either no payload was allocated or if
 *                                      the allocated payload is not a cdb
 *                                      operation.
 *
 ****************************************************************/
fbe_status_t
fbe_payload_ex_increment_cdb_operation_level(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* If packet is traversed we should not increment operation level */
    if(payload->next_operation != NULL){
        payload_operation_header = (fbe_payload_operation_header_t *)payload->next_operation;

        /* Check if current operation is cdb operation */
        if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CDB_OPERATION){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;
    }
    return FBE_STATUS_OK;
}

/* dmrb_operation */

/*!**************************************************************
 * @fn fbe_payload_ex_allocate_dmrb_operation(fbe_payload_ex_t * payload)
 ****************************************************************
 * @brief
 *  Allocate a new dmrb operation from the input payload structure.
 *  We assume there is enough space in the payload, an error is
 *  returned if not.
 *
 * @param  payload - The payload to allocate this dmrb operation
 *                   out of.  We assume that there is enough space
 *                   still reserved in the payload for this dmrb operation.
 *                   If we have already reserved dmrb operation
 *                   from the payload then an error is returned.
 *
 * @return fbe_payload_dmrb_operation_t * -
 *         The dmrb operation that was just allocated.
 *         OR
 *         NULL if an error occurs.
 *
 ****************************************************************/

fbe_payload_dmrb_operation_t * 
fbe_payload_ex_allocate_dmrb_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;	

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated", __FUNCTION__);
        return NULL;
    }

    /* Check if we have free lba_lock operation */
    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index >= sizeof(fbe_payload_dmrb_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];
        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_DMRB_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;
        payload->operation_memory_index += sizeof(fbe_payload_dmrb_operation_t);
        return (fbe_payload_dmrb_operation_t * )payload_operation_header;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);
    return NULL;
}

/*!**************************************************************
 * @fn fbe_payload_ex_release_dmrb_operation(
 *               fbe_payload_ex_t *payload,
 *               fbe_payload_dmrb_operation_t * payload_dmrb_operation)
 ****************************************************************
 * @brief
 *  This function is used to release back to the payload a dmrb
 *  operation that was in use.
 *  This function will "unwind" the payload to point to the previously
 *  allocated operation.
 * 
 *  Note that this call is the only way to "unwind the payload" to get back to
 *  the prior stack level in the payload.
 *
 * @param payload - The payload to free for.
 * @param payload_dmrb_operation - The specific payload to free.
 *        This function checks to make sure that the input dmrb operation
 *        ptr is equal to the current operation that is in use.  If not,
 *        then FBE_STATUS_GENERIC_FAILURE is returned.
 *
 * @return fbe_status_t FBE_STATUS_OK if no error.
 *                      FBE_STATUS_GENERIC_FAILURE if error.
 *
 ****************************************************************/

fbe_status_t 
fbe_payload_ex_release_dmrb_operation(fbe_payload_ex_t * payload, fbe_payload_dmrb_operation_t * payload_dmrb_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload_dmrb_operation;

    /* Check if current operation is dmrb operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_DMRB_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free dmrb operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    payload->operation_memory_index -= sizeof(fbe_payload_dmrb_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_dmrb_operation){ 
        return FBE_STATUS_OK;
    } else if (payload->current_operation == (fbe_payload_operation_header_t *)payload_dmrb_operation){ 
        /* Shift operation stack */
        payload->current_operation = payload_operation_header->prev_header;
        return FBE_STATUS_OK;
    } else {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation pointer", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

/*!**************************************************************
 * @fn fbe_payload_ex_increment_dmrb_operation_level(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function moves the level in the payload to make the dmrb operation the
 *  active payload.
 * 
 *  Note that after fbe_payload_ex_allocate_dmrb_operation() is called to
 *  allocate the dmrb operation, then this function must also be called to make
 *  the current dmrb operation "active".  
 *
 * @param payload - The payload to increment the dmrb operation for.
 *
 * @return FBE_STATUS_OK - If no error.
 *         FBE_STATUS_GENERIC_FAILURE - If either no payload was allocated or if
 *                                      the allocated payload is not a dmrb
 *                                      operation.
 *
 ****************************************************************/
fbe_status_t
fbe_payload_ex_increment_dmrb_operation_level(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* If packet is traversed we should not increment operation level */
    if(payload->next_operation != NULL){
        payload_operation_header = (fbe_payload_operation_header_t *)payload->next_operation;

        /* Check if current operation is dmrb operation */
        if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_DMRB_OPERATION){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;
    }
    return FBE_STATUS_OK;
}

/* fis_operation */

/*!**************************************************************
 * @fn fbe_payload_ex_allocate_fis_operation(fbe_payload_ex_t * payload)
 ****************************************************************
 * @brief
 *  Allocate a new fis operation from the input payload structure.
 *  We assume there is enough space in the payload, an error is
 *  returned if not.
 *
 * @param  payload - The payload to allocate this fis operation
 *                   out of.  We assume that there is enough space
 *                   still reserved in the payload for this fis operation.
 *
 * @return fbe_payload_fis_operation_t * -
 *         The fis operation that was just allocated.
 *         OR
 *         NULL if an error occurs.
 *
 ****************************************************************/
fbe_payload_fis_operation_t * 
fbe_payload_ex_allocate_fis_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated", __FUNCTION__);
        return NULL;
    }

    /* Check if we have free lba_lock operation */
    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index > sizeof(fbe_payload_fis_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];
        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_FIS_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;
        payload->operation_memory_index += sizeof(fbe_payload_fis_operation_t);
        return (fbe_payload_fis_operation_t * )payload_operation_header;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);
    return NULL;
}

/*!**************************************************************
 * @fn fbe_payload_ex_release_fis_operation(
 *               fbe_payload_ex_t *payload,
 *               fbe_payload_fis_operation_t * payload_fis_operation)
 ****************************************************************
 * @brief
 *  This function is used to release back to the payload a fis
 *  operation that was in use.
 *  This function will "unwind" the payload to point to the previously
 *  allocated operation.
 * 
 *  Note that this call is the only way to "unwind the payload" to get back to
 *  the prior stack level in the payload.
 *
 * @param payload - The payload to free for.
 * @param payload_fis_operation - The specific payload to free.
 *        This function checks to make sure that the input fis operation
 *        ptr is equal to the current operation that is in use.  If not,
 *        then FBE_STATUS_GENERIC_FAILURE is returned.
 *
 * @return fbe_status_t FBE_STATUS_OK if no error.
 *                      FBE_STATUS_GENERIC_FAILURE if error.
 *
 ****************************************************************/
fbe_status_t 
fbe_payload_ex_release_fis_operation(fbe_payload_ex_t * payload, fbe_payload_fis_operation_t * payload_fis_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload_fis_operation;

    /* Check if current operation is fis operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_FIS_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free fis operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    payload->operation_memory_index -= sizeof(fbe_payload_fis_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_fis_operation){ 
        return FBE_STATUS_OK;
    } else if(payload->current_operation == (fbe_payload_operation_header_t *)payload_fis_operation){ 
        /* Shift operation stack */
        payload->current_operation = payload_operation_header->prev_header;
        return FBE_STATUS_OK;
    } else {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation pointer", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}
/*!**************************************************************
 * @fn fbe_payload_ex_get_fis_operation(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function fetches the current active payload from the payload structure,
 *  which is assumed to be a fis operation.  If the current active payload is
 *  not a fis operation, then a NULL is returned.
 * 
 *  Note that this function assumes that after
 *  fbe_payload_ex_allocate_fis_operation() was called to allocate the fis
 *  operation, then fbe_payload_ex_increment_fis_operation_level() was also
 *  called to make the current fis operation "active".  Typically the sending
 *  of the fis operation on the edge calls the increment, although if no
 *  edge is being used, then the client might need to call the increment
 *  function.
 *
 * @param payload - The payload to get the fis operation for.
 *
 * @return fbe_payload_fis_operation_t * - The current active payload, which
 *                                           should be a fis operation.
 *         NULL is returned if the current active payload is not a fis
 *         operation.
 *
 ****************************************************************/
fbe_payload_fis_operation_t * 
fbe_payload_ex_get_fis_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        /* We will add error trace later
            For now we have to support both io_block and payload, so some packets may not have payload operation at all
        */
        return NULL;
    }

    /* Check if current operation is fis operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_FIS_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_fis_operation_t *)payload->current_operation;
}

/*!**************************************************************
 * @fn fbe_payload_ex_increment_fis_operation_level(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function moves the level in the payload to make the fis operation the
 *  active payload.
 * 
 *  Note that after fbe_payload_ex_allocate_fis_operation() is called to
 *  allocate the fis operation, then this function must also be called to make
 *  the current fis operation "active".  Typically the sending of the fis
 *  operation on the edge calls this function, although if no edge is being
 *  used, then the client might need to call this function.
 *
 * @param payload - The payload to increment the fis operation for.
 *
 * @return FBE_STATUS_OK - If no error.
 *         FBE_STATUS_GENERIC_FAILURE - If either no payload was allocated or if
 *                                      the allocated payload is not a fis
 *                                      operation.
 *
 ****************************************************************/
fbe_status_t
fbe_payload_ex_increment_fis_operation_level(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* If packet is traversed we should not increment operation level */
    if(payload->next_operation != NULL){
        payload_operation_header = (fbe_payload_operation_header_t *)payload->next_operation;

        /* Check if current operation is fis operation */
        if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_FIS_OPERATION){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;
    }
    return FBE_STATUS_OK;
}


/* smp_operation */

/*!**************************************************************
 * @fn fbe_payload_ex_allocate_smp_operation(fbe_payload_ex_t * payload)
 ****************************************************************
 * @brief
 *  Allocate a new smp operation from the input payload structure.
 *  We assume there is enough space in the payload, an error is
 *  returned if not.
 *
 * @param  payload - The payload to allocate this smp operation
 *                   out of.  We assume that there is enough space
 *                   still reserved in the payload for this smp operation.
 *
 * @return fbe_payload_smp_operation_t * -
 *         The smp operation that was just allocated.
 *         OR
 *         NULL if an error occurs.
 *
 ****************************************************************/
fbe_payload_smp_operation_t * 
fbe_payload_ex_allocate_smp_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated", __FUNCTION__);
        return NULL;
    }

    /* Check if we have free lba_lock operation */
    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index > sizeof(fbe_payload_smp_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];
        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_SMP_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;
        payload->operation_memory_index += sizeof(fbe_payload_smp_operation_t);
        return (fbe_payload_smp_operation_t * )payload_operation_header;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);
    return NULL;
}

/*!**************************************************************
 * @fn fbe_payload_ex_release_smp_operation(
 *               fbe_payload_ex_t *payload,
 *               fbe_payload_smp_operation_t * payload_smp_operation)
 ****************************************************************
 * @brief
 *  This function is used to release back to the payload a smp
 *  operation that was in use.
 *  This function will "unwind" the payload to point to the previously
 *  allocated operation.
 * 
 *  Note that this call is the only way to "unwind the payload" to get back to
 *  the prior stack level in the payload.
 *
 * @param payload - The payload to free for.
 * @param payload_smp_operation - The specific payload to free.
 *        This function checks to make sure that the input smp operation
 *        ptr is equal to the current operation that is in use.  If not,
 *        then FBE_STATUS_GENERIC_FAILURE is returned.
 *
 * @return fbe_status_t FBE_STATUS_OK if no error.
 *                      FBE_STATUS_GENERIC_FAILURE if error.
 *
 ****************************************************************/
fbe_status_t 
fbe_payload_ex_release_smp_operation(fbe_payload_ex_t * payload, fbe_payload_smp_operation_t * payload_smp_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload_smp_operation;

    /* Check if current operation is smp operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_SMP_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free smp operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    payload->operation_memory_index -= sizeof(fbe_payload_smp_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_smp_operation){ 
        return FBE_STATUS_OK;
    }

    /* Shift operation stack */
    payload->current_operation = payload_operation_header->prev_header;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_ex_get_smp_operation(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function fetches the current active payload from the payload structure,
 *  which is assumed to be a smp operation.  If the current active payload is
 *  not a smp operation, then a NULL is returned.
 * 
 *  Note that this function assumes that after
 *  fbe_payload_ex_allocate_smp_operation() was called to allocate the smp
 *  operation, then fbe_payload_ex_increment_smp_operation_level() was also
 *  called to make the current smp operation "active".  Typically the sending
 *  of the smp operation on the edge calls the increment, although if no
 *  edge is being used, then the client might need to call the increment
 *  function.
 *
 * @param payload - The payload to get the smp operation for.
 *
 * @return fbe_payload_smp_operation_t * - The current active payload, which
 *                                           should be a smp operation.
 *         NULL is returned if the current active payload is not a smp
 *         operation.
 *
 ****************************************************************/
fbe_payload_smp_operation_t * 
fbe_payload_ex_get_smp_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s NULL payload operation", __FUNCTION__);
        return NULL;
    }
    /* Check if current operation is smp operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_SMP_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_smp_operation_t *)payload->current_operation;
}

/*!**************************************************************
 * @fn fbe_payload_ex_increment_smp_operation_level(fbe_payload_ex_t *payload)
 ****************************************************************
 * @brief
 *  This function moves the level in the payload to make the smp operation the
 *  active payload.
 * 
 *  Note that after fbe_payload_ex_allocate_smp_operation() is called to
 *  allocate the smp operation, then this function must also be called to make
 *  the current smp operation "active".  Typically the sending of the smp
 *  operation on the edge calls this function, although if no edge is being
 *  used, then the client might need to call this function.
 *
 * @param payload - The payload to increment the smp operation for.
 *
 * @return FBE_STATUS_OK - If no error.
 *         FBE_STATUS_GENERIC_FAILURE - If either no payload was allocated or if
 *                                      the allocated payload is not a smp
 *                                      operation.
 *
 ****************************************************************/
fbe_status_t
fbe_payload_ex_increment_smp_operation_level(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* If packet is traversed we should not increment operation level */
    if(payload->next_operation != NULL){
        payload_operation_header = (fbe_payload_operation_header_t *)payload->next_operation;

        /* Check if current operation is smp operation */
        if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_SMP_OPERATION){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;
    }
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_payload_ex_get_current_opcode(fbe_payload_ex_t *payload, 
 *                                    fbe_payload_opcode_t * opcode)
 ****************************************************************
 * @brief
 *  This function returns the opcode from current operation in the payload.
 * 
 * @param payload - The payload to check.
 * @param opcode -  The opcode in current operation.
 *
 * @return FBE_STATUS_OK - If no error.
 *         FBE_STATUS_GENERIC_FAILURE - If either no payload was allocated or if
 *                                      the allocated payload is not a cdb
 *                                      operation.
 *
 ****************************************************************/
fbe_status_t
fbe_payload_ex_get_current_opcode(fbe_payload_ex_t * payload, fbe_payload_opcode_t * opcode)
{
    if((payload == NULL) || (payload->current_operation == NULL)){
        *opcode = FBE_PAYLOAD_OPCODE_INVALID;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *opcode = payload->current_operation->payload_opcode;
    return FBE_STATUS_OK;
}


fbe_status_t
fbe_payload_ex_get_media_error_lba(fbe_payload_ex_t * payload, fbe_lba_t * lba)
{
     *lba = payload->media_error_lba;
     return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_ex_set_media_error_lba(fbe_payload_ex_t * payload, fbe_lba_t lba)
{
    payload->media_error_lba = lba;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_ex_set_verify_error_count_ptr(fbe_payload_ex_t * payload, void *error_counts_p)
{
    payload->verify_error_counts_p = error_counts_p;

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_ex_get_verify_error_count_ptr(fbe_payload_ex_t * payload, void **error_counts_p)
{
    *error_counts_p = payload->verify_error_counts_p;
    return FBE_STATUS_OK;
}


fbe_payload_memory_operation_t * 
fbe_payload_ex_allocate_memory_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated", __FUNCTION__);
        return NULL;
    }

    /* Check if we have free memory operation */
    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index > sizeof(fbe_payload_memory_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];
        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_MEMORY_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;
        payload->operation_memory_index += sizeof(fbe_payload_memory_operation_t);
		fbe_zero_memory(&((fbe_payload_memory_operation_t * )payload_operation_header)->memory_io_master, sizeof(fbe_memory_io_master_t));
		payload->payload_memory_operation = (fbe_payload_memory_operation_t * )payload_operation_header;

        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;

		return (fbe_payload_memory_operation_t * )payload_operation_header;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);
    return NULL;
}


fbe_status_t 
fbe_payload_ex_release_memory_operation(fbe_payload_ex_t * payload, fbe_payload_memory_operation_t * payload_memory_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;
	fbe_u32_t i;

    if (payload_memory_operation == NULL) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload_memory_operation is NULL", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_operation_header = (fbe_payload_operation_header_t *)payload_memory_operation;

    /* Check if our operation is memory operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_MEMORY_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	for(i = 0; i < FBE_MEMORY_DPS_QUEUE_ID_LAST; i++){
		if(payload_memory_operation->memory_io_master.chunk_counter[i] != 0 ||
			payload_memory_operation->memory_io_master.reserved_chunk_counter[i] != 0){
			fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s Invalid chunk counters [%d] %d %d", __FUNCTION__, i,
									payload_memory_operation->memory_io_master.chunk_counter[i],
									payload_memory_operation->memory_io_master.reserved_chunk_counter[i]);
		}
	}

	if(payload_memory_operation->memory_io_master.flags != 0){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid flags %x", __FUNCTION__, 
								payload_memory_operation->memory_io_master.flags);
	}

    /* Free memory operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;

    payload->operation_memory_index -= sizeof(fbe_payload_memory_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_memory_operation){ 
		payload->payload_memory_operation = NULL;
        return FBE_STATUS_OK;
    } else if(payload->current_operation == (fbe_payload_operation_header_t *)payload_memory_operation){
        /* Shift operation stack */
        payload->current_operation = payload_operation_header->prev_header;
		payload->payload_memory_operation = NULL;
        return FBE_STATUS_OK;
    } else {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation pointer", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

/* persistent_memory operation */
fbe_payload_persistent_memory_operation_t * 
fbe_payload_ex_allocate_persistent_memory_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated", __FUNCTION__);
        return NULL;
    }

    /* Check if we have free persistent_memory operation */
    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index > sizeof(fbe_payload_persistent_memory_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];
        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_PERSISTENT_MEMORY_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;
        payload->operation_memory_index += sizeof(fbe_payload_persistent_memory_operation_t);
        return (fbe_payload_persistent_memory_operation_t * )payload_operation_header;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);
    return NULL;
}

fbe_status_t 
fbe_payload_ex_release_persistent_memory_operation(fbe_payload_ex_t * payload, fbe_payload_persistent_memory_operation_t * payload_persistent_memory_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    if (payload_persistent_memory_operation == NULL) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload_persistent_memory_operation is NULL", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_operation_header = (fbe_payload_operation_header_t *)payload_persistent_memory_operation;

    /* Check if our operation is persistent_memory operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_PERSISTENT_MEMORY_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free persistent_memory operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    payload->operation_memory_index -= sizeof(fbe_payload_persistent_memory_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_persistent_memory_operation){ 
        return FBE_STATUS_OK;
    } else if(payload->current_operation == (fbe_payload_operation_header_t *)payload_persistent_memory_operation){
        /* Shift operation stack */
        payload->current_operation = payload_operation_header->prev_header;
        return FBE_STATUS_OK;
    } else {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation pointer", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

fbe_payload_persistent_memory_operation_t * 
fbe_payload_ex_get_persistent_memory_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s NULL payload operation\n", __FUNCTION__);
        return NULL;
    }
    /* Check if current operation is persistent_memory operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_PERSISTENT_MEMORY_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X\n", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_persistent_memory_operation_t *)payload->current_operation;
}

fbe_payload_persistent_memory_operation_t * 
fbe_payload_ex_check_persistent_memory_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s NULL payload operation\n", __FUNCTION__);
        return NULL;
    }
    /* Check if current operation is persistent_memory operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_PERSISTENT_MEMORY_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s current operation opcode %X is not persistent_memory operation.\n", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_persistent_memory_operation_t *)payload->current_operation;
}

fbe_status_t
fbe_payload_ex_increment_persistent_memory_operation_level(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* If packet is traversed we should not increment operation level */
    if(payload->next_operation != NULL){ /* Do nothing for traverse */
        payload_operation_header = (fbe_payload_operation_header_t *)payload->next_operation;

        /* Check if current operation is block operation */
        if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_PERSISTENT_MEMORY_OPERATION){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid current operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;
    }

    return FBE_STATUS_OK;
}

fbe_payload_buffer_operation_t * 
fbe_payload_ex_get_buffer_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;

    if(payload_operation_header == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s NULL payload operation\n", __FUNCTION__);
        return NULL;
    }
    /* Check if current operation is control operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_BUFFER_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid current operation opcode %X\n", __FUNCTION__, payload_operation_header->payload_opcode);
        return NULL;
    }

    return (fbe_payload_buffer_operation_t *)payload->current_operation;
}

/*!*****************************************************************************
 * fbe_payload_ex_get_any_buffer_operation()
 *******************************************************************************
 * @brief
 * Returns the previous allocated buffer operation in the payload.
 *
 * @param payload - pointer to a control packet from the scheduler
 * 
 * @return fbe_payload_buffer_operation_t - The first control operation
 *         found in the payload previous header chain or NULL if not found.
 *   
 * @author
 *  3/5/2015 - Created. Ron Proulx
 *****************************************************************************/
fbe_payload_buffer_operation_t * 
fbe_payload_ex_get_any_buffer_operation(fbe_payload_ex_t * payload)
{
    fbe_payload_operation_header_t *   payload_operation_header = NULL;
    fbe_payload_buffer_operation_t * payload_buffer_operation = NULL;
    
    payload_operation_header = (fbe_payload_operation_header_t *) payload->current_operation;

    while (payload_operation_header) {

        if (payload_operation_header->payload_opcode == FBE_PAYLOAD_OPCODE_BUFFER_OPERATION) {
            payload_buffer_operation = (fbe_payload_buffer_operation_t *) payload_operation_header;
            break;
        }

        payload_operation_header = payload_operation_header->prev_header;
    }

   return payload_buffer_operation; 
}

/*! @note WARNING!!! This is only to be used by specific sep code. */
fbe_payload_buffer_operation_t * 
fbe_payload_ex_allocate_buffer_operation(fbe_payload_ex_t * payload, fbe_u32_t requested_buffer_size)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    /* Check if next operation is already allocated */
    if(payload->next_operation != NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Next operation already allocated\n", __FUNCTION__);
        return NULL;
    }

    /* Check if the requested buffer size is large than available */
    if (requested_buffer_size > FBE_PAYLOAD_BUFFER_OPERATION_BUFFER_SIZE) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s request size: %d larger than available: %d\n", 
                                 __FUNCTION__, requested_buffer_size, FBE_PAYLOAD_BUFFER_OPERATION_BUFFER_SIZE);
        return NULL;
    }
    /* Check if we have free memory operation */
    if(FBE_PAYLOAD_EX_MEMORY_SIZE - payload->operation_memory_index > sizeof(fbe_payload_buffer_operation_t)){
        payload_operation_header = (fbe_payload_operation_header_t *)&payload->operation_memory[payload->operation_memory_index];
        payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_BUFFER_OPERATION;

        payload->next_operation = payload_operation_header;
        payload_operation_header->prev_header = payload->current_operation;
        payload->operation_memory_index += sizeof(fbe_payload_buffer_operation_t);
		fbe_zero_memory(&((fbe_payload_buffer_operation_t * )payload_operation_header)->buffer, FBE_PAYLOAD_BUFFER_OPERATION_BUFFER_SIZE);

        payload->current_operation = payload->next_operation;
        payload->next_operation = NULL;

		return (fbe_payload_buffer_operation_t * )payload_operation_header;
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of memory", __FUNCTION__);
    return NULL;
}


fbe_status_t 
fbe_payload_ex_release_buffer_operation(fbe_payload_ex_t * payload, fbe_payload_buffer_operation_t * payload_buffer_operation)
{
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    if (payload_buffer_operation == NULL) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload_memory_operation is NULL", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_operation_header = (fbe_payload_operation_header_t *)payload_buffer_operation;

    /* Check if our operation is memory operation */
    if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_BUFFER_OPERATION){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation opcode %X", __FUNCTION__, payload_operation_header->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Free buffer operation */
    payload_operation_header->payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    payload->operation_memory_index -= sizeof(fbe_payload_buffer_operation_t);

    /* Check if caller wants to release newly allocated operation due to some error */
    if(payload->next_operation == (fbe_payload_operation_header_t *)payload_buffer_operation){ 
        return FBE_STATUS_OK;
    } else if(payload->current_operation == (fbe_payload_operation_header_t *)payload_buffer_operation){ 
        /* Shift operation stack */
        payload->current_operation = payload_operation_header->prev_header;
        return FBE_STATUS_OK;
    } else {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid operation pointer", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

}

/*************************
 * end file fbe_payload_ex_main.c
 *************************/
