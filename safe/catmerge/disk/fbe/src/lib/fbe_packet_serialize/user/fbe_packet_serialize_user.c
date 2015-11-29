/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_packet_serialize_user.c
 ***************************************************************************
 *
 *  Description
 *      user space implementation of packet serialization
 *
 *  History:
 *      12/08/09    sharel - Created
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe_packet_serialize_lib.h"
#include "fbe/fbe_topology_interface.h"

/**************************************
				Local variables
**************************************/

/*******************************************
				Local functions
********************************************/
static void fbe_serialize_lib_get_sg_area_size(fbe_sg_element_t *sg_element, fbe_u32_t *sg_area_size, fbe_u32_t *elements_count);
static void fbe_serialize_lib_copy_sg_data(fbe_sg_element_t *sg_element, fbe_u8_t * sg_area);
static fbe_status_t fbe_serialize_lib_get_control_operation(fbe_packet_t * packet, fbe_payload_control_operation_t **control_operation);
static void fbe_serialize_lib_copy_back_sg_data (fbe_sg_element_t *	sg_element, fbe_u8_t * sg_area, fbe_u32_t element_count);
static void * fbe_serialize_lib_fix_sg_pointers(fbe_u8_t *ioBuffer, fbe_u32_t element_count);

/*********************************************************************
 *            fbe_serialize_lib_serialize_packet ()
 *********************************************************************
 *
 *  Description: serialize a packet to a contigous buffer
 *
 *	Inputs: packet from user
 *          pointer to function allocated memory of contigous buffer
 *          pointer to return the overall size of the buffer we created
 *
 *  Return Value: status of success
 *
 *********************************************************************/
fbe_status_t fbe_serialize_lib_serialize_packet(IN fbe_packet_t *packet,
												OUT fbe_serialized_control_transaction_t **serialized_transaction,
												OUT fbe_u32_t *serialized_transaction_size)
{
	fbe_u8_t  * 						buffer = NULL;
	fbe_u32_t 							buffer_size;
	fbe_u32_t 							serialized_packet_size;
	fbe_u8_t * 							serialized_packet = NULL;
	fbe_u8_t * 							serialized_buffer = NULL;
    fbe_serialized_control_transaction_t *	new_packet = NULL;
	fbe_payload_ex_t *						payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_payload_control_operation_opcode_t opcode;
	fbe_packet_attr_t					packet_attr = FBE_PACKET_FLAG_NO_ATTRIB;
	fbe_class_id_t						class_id = FBE_CLASS_ID_INVALID;
	fbe_package_id_t					package_id = FBE_PACKAGE_ID_INVALID;
	fbe_service_id_t					service_id = FBE_SERVICE_ID_INVALID;
	fbe_object_id_t						object_id = FBE_OBJECT_ID_INVALID;
	fbe_u32_t							packet_size = (fbe_u32_t)sizeof(fbe_serialized_control_transaction_t);
	fbe_u32_t							sg_area_size = 0;
	fbe_u8_t * 							start_of_sg_area = NULL;
    fbe_u32_t							sg_element_count = 0;
    fbe_sg_element_t *					sg_element = NULL;
	fbe_u32_t							sg_count = 0;
    
    /* Get the buffer and opcode from the packet. */
    fbe_serialize_lib_get_control_operation(packet, &control_operation);
	fbe_payload_control_get_buffer (control_operation, &buffer);
	fbe_payload_control_get_buffer_length(control_operation, &buffer_size);
	fbe_payload_control_get_opcode(control_operation, &opcode);

	/*we also need to get all the sg related information in case the user placed some data in the sg list*/
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_count);

	fbe_serialize_lib_get_sg_area_size (sg_element, &sg_area_size, &sg_element_count);

	/* calculat the size we would need for the packet*/
	serialized_packet_size = packet_size + buffer_size + sg_area_size;

	/* Allocate the memory for the serialized packet. */
	serialized_packet = malloc(serialized_packet_size);
	serialized_buffer = serialized_packet + packet_size;
	start_of_sg_area = serialized_buffer + buffer_size;
    memset (serialized_packet, 0 , packet_size + buffer_size + sg_area_size);

	/*first thing in memory is the packet*/
	new_packet = (fbe_serialized_control_transaction_t *)serialized_packet;

	/*we can't use the original packaet that got in and just do a memcpy because when we send it down
	to the physical pakcage, all the stack pointers would get changed and we will not be able
	to go back up*/
    fbe_transport_get_class_id (packet, &class_id);
	fbe_transport_get_service_id (packet, &service_id);
    fbe_transport_get_package_id(packet, &package_id);
	fbe_transport_get_object_id (packet, &object_id);
	fbe_transport_get_packet_attr(packet, &packet_attr);

	/*set address of new packet*/
	new_packet->buffer_length = buffer_size;
	new_packet->class_id = class_id;
	new_packet->object_id = object_id;
	new_packet->packet_attr = packet_attr;
	new_packet->service_id = service_id;
	new_packet->packge_id = package_id;
	new_packet->user_control_code = opcode;
	new_packet->sg_elements_count = sg_element_count;

    /*we do that on purpose to make sure the user overirde it with good status (in case it really worked)*/
	new_packet->packet_status.code = FBE_STATUS_GENERIC_FAILURE;
	new_packet->packet_status.qualifier = 0;
	new_packet->payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
	new_packet->payload_control_qualifier = 0;
	fbe_payload_ex_get_key_handle(payload, &(new_packet->key_handle));
    
    /* Copy the the buffer into the serialized packet*/
	memcpy (serialized_buffer, buffer, buffer_size);

	/*and now get all the information of the sg data*/
    fbe_serialize_lib_copy_sg_data(sg_element, start_of_sg_area);

	/*let the user get the information*/
	*serialized_transaction = new_packet;
	*serialized_transaction_size = serialized_packet_size;

	return FBE_STATUS_OK;

}

/*********************************************************************
 *            fbe_serialize_lib_deserialize_transaction ()
 *********************************************************************
 *
 *  Description: convert the transaction we got into a packet
 *
 *	Inputs: transaction from user
 *          pointer to user allocated packet to fill
 *
 *  Return Value: status of success
 *
 *********************************************************************/
fbe_status_t fbe_serialize_lib_deserialize_transaction(IN fbe_serialized_control_transaction_t *serialized_transaction, OUT fbe_packet_t *packet)
{
	fbe_payload_control_operation_t *   	original_control_operation = NULL;
	fbe_u8_t * 								original_buffer = NULL;
	fbe_u32_t 								original_buffer_size = 0;
    fbe_serialized_control_transaction_t *	returned_packet = serialized_transaction;
    fbe_u8_t *								serialized_buffer = NULL;
	fbe_u8_t *								start_of_sg_area = NULL;
	fbe_payload_ex_t *						payload = NULL;
	fbe_sg_element_t *						sg_element = NULL;
	fbe_u32_t								sg_count = 0;

	/* get the address of the original buffer of the original packet*/
	fbe_serialize_lib_get_control_operation(packet, &original_control_operation);
	fbe_payload_control_get_buffer(original_control_operation, &original_buffer);
	fbe_payload_control_get_buffer_length(original_control_operation, &original_buffer_size);

	/*get the address of the buffer that has the results (the one that traveled all the way to the destination)*/
    serialized_buffer = (fbe_u8_t*)((fbe_u8_t *)returned_packet + sizeof(fbe_serialized_control_transaction_t));
	start_of_sg_area = (fbe_u8_t*)(serialized_buffer + returned_packet->buffer_length);

    /*and copy the results into the original packet*/
	memcpy(original_buffer, serialized_buffer,  original_buffer_size);

	/*and copy back the sg list*/
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_count);

	fbe_serialize_lib_copy_back_sg_data (sg_element, start_of_sg_area, returned_packet->sg_elements_count);

	/*update the status*/
	memcpy(&packet->packet_status, &returned_packet->packet_status, sizeof(fbe_packet_status_t));
	fbe_payload_control_set_status(original_control_operation, returned_packet->payload_control_status);
	fbe_payload_control_set_status_qualifier(original_control_operation, returned_packet->payload_control_qualifier);

	return FBE_STATUS_OK;
}


/*********************************************************************
 *            fbe_serialize_lib_get_sg_area_size ()
 *********************************************************************
 *
 *  Description:
 *    get the size of memory we would consume when copying the sg lists and data.
 *
 *  Return Value:  none
 *
 *  History:
 *    11/10/08: sharel	created
 *    
 *********************************************************************/
static void fbe_serialize_lib_get_sg_area_size(fbe_sg_element_t * sg_element, fbe_u32_t *sg_area_size, fbe_u32_t *elements_count)
{
    fbe_u32_t			sg_count = 0;
	fbe_u32_t			total_sg_entries_byte_size = 0;
	fbe_u32_t			total_sg_data_byte_size = 0;

    if (sg_element == NULL) {
		*sg_area_size = 0;
		*elements_count = 0;
		return;/*nothing to copy*/
	}

	sg_count = 0;
	/*we don't wnat to rely on the count so we just walk the list until we get the terminating one*/
    while (sg_element->count != 0) {
		total_sg_data_byte_size += sg_element->count;
		total_sg_entries_byte_size += sizeof (sg_element->count);/*we send down only the count itself*/
		sg_element ++;
		sg_count++;
	}

    *sg_area_size = total_sg_entries_byte_size + total_sg_data_byte_size;
	*elements_count = sg_count;

	return;
}

/*********************************************************************
 *            fbe_serialize_lib_copy_sg_data ()
 *********************************************************************
 *
 *  Description:
 *    copy the sg list and the data into the contiguous buffer
 *
 *  Return Value:  none
 *
 *  History:
 *    11/10/08: sharel	created
 *    
 *********************************************************************/
static void fbe_serialize_lib_copy_sg_data(fbe_sg_element_t *	sg_element, fbe_u8_t * sg_area)
{
	fbe_sg_element_t *	temp_sg_element = sg_element;
	fbe_u8_t *			sg_area_copy = sg_area;

	if (sg_element == NULL) {
		return;/*nothing to copy*/
	}

	/*first thing is the sg list entries*/
	while (temp_sg_element->count != 0) {
		memcpy (sg_area_copy ,&temp_sg_element->count, sizeof (temp_sg_element->count));
		temp_sg_element ++;
		sg_area_copy += sizeof (temp_sg_element->count);
	}

	/*and now the data*/
	while (sg_element->count != 0) {
		memcpy (sg_area_copy ,sg_element->address, sg_element->count);
		sg_area_copy += sg_element->count;
		sg_element ++;
	}

	return;

}

static fbe_status_t fbe_serialize_lib_get_control_operation(fbe_packet_t * packet, fbe_payload_control_operation_t **control_operation)
{
	//fbe_payload_ex_t * sep_payload = NULL;
	//fbe_payload_ex_t * 	payload = NULL;

	fbe_payload_ex_t * 	payload_ex = NULL;
	fbe_payload_operation_header_t * payload_operation_header = NULL;

	payload_ex = fbe_transport_get_payload_ex(packet);
	*control_operation = NULL;

	if(payload_ex->current_operation == NULL){	
		if(payload_ex->next_operation != NULL){
			payload_operation_header = (fbe_payload_operation_header_t *)payload_ex->next_operation;

			/* Check if next operation is control operation */
			if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
				return FBE_STATUS_GENERIC_FAILURE;
			}

			*control_operation = (fbe_payload_control_operation_t *)payload_operation_header;
		} else {
			return FBE_STATUS_GENERIC_FAILURE;
		}
	} else {
		payload_operation_header = (fbe_payload_operation_header_t *)payload_ex->current_operation;

		/* Check if current operation is control operation */
		if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
			return FBE_STATUS_GENERIC_FAILURE;
		}

		*control_operation = (fbe_payload_control_operation_t *)payload_operation_header;
	}
	return FBE_STATUS_OK;
}

static void fbe_serialize_lib_copy_back_sg_data (fbe_sg_element_t *	sg_element, fbe_u8_t * sg_area, fbe_u32_t element_count)
{
	if (sg_element == NULL) {
		return;/*nothing to copy*/
	}

	/*we need to roll ourselvs to the beggining of the actual data*/
	sg_area += (sizeof(sg_element->count) * element_count);

	/*now let's copy the data*/
	while (sg_element->count != 0) {
		memcpy (sg_element->address, sg_area, sg_element->count);
		sg_area += sg_element->count;
		sg_element ++;

	}


}

fbe_status_t fbe_serialize_lib_unpack_transaction(fbe_serialized_control_transaction_t *serialized_transaction_in,
												  fbe_packet_t *packet,
												  void **returned_sg_elements_list)
{

	fbe_u8_t * 								user_buffer = NULL;
    fbe_u32_t								packet_size = (fbe_u32_t)sizeof(fbe_serialized_control_transaction_t);
    fbe_payload_ex_t *							new_payload = NULL;
	fbe_payload_ex_t *						new_sep_payload = NULL;
    fbe_payload_control_operation_t *		new_control_operation = NULL;
	void *									sg_element_list = NULL;


	/*get the user buffer address and length*/
    user_buffer = (fbe_u8_t *)((fbe_u8_t *)serialized_transaction_in + packet_size);
    
	/*what payload do we have*/
	if (serialized_transaction_in->packge_id == FBE_PACKAGE_ID_PHYSICAL) {
        new_payload = fbe_transport_get_payload_ex (packet);
        new_control_operation = fbe_payload_ex_allocate_control_operation(new_payload);
	}else{
        new_sep_payload = fbe_transport_get_payload_ex(packet);
		new_control_operation = fbe_payload_ex_allocate_control_operation(new_sep_payload);
	}

	fbe_payload_control_build_operation (new_control_operation,
										 serialized_transaction_in->user_control_code,
										 user_buffer,
										 serialized_transaction_in->buffer_length);
   
    
	/*sg related stuff - we giove the function the start of the sg list area which has only the counts
	it will allocate a memory for us which we would use as the sg list in the packet and delete upon comletion
	*/
	if (serialized_transaction_in->sg_elements_count != 0) {
		sg_element_list = fbe_serialize_lib_fix_sg_pointers((fbe_u8_t *)(user_buffer + serialized_transaction_in->buffer_length),
                                                             serialized_transaction_in->sg_elements_count);
		if (sg_element_list == NULL) {
			return FBE_STATUS_GENERIC_FAILURE;
		}

		serialized_transaction_in->spare_opaque = CSX_CAST_PTR_TO_PTRMAX(sg_element_list);/*keep for later since it may be lost in completion*/
	}else{
		serialized_transaction_in->spare_opaque = 0;
	}
    
    fbe_transport_set_address(packet,
							  serialized_transaction_in->packge_id,
							  serialized_transaction_in->service_id,
							  serialized_transaction_in->class_id,
							  serialized_transaction_in->object_id);

#if 1 /* SAFEBUG - need to be smarter about not wrecking or improperly setting this flag */
    if ((packet->packet_attr & FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED) == 0) {      
    	fbe_transport_set_packet_attr(packet, serialized_transaction_in->packet_attr);
    	packet->packet_attr &= ~FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED; /* The semaphore can be initialized only locally */
    } else {
        fbe_transport_set_packet_attr(packet, serialized_transaction_in->packet_attr | FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED); /* if we're reusing a packet that already has a completion semaphore then don't clear the flag or we'll leak it */
    }
#else
	fbe_transport_set_packet_attr(packet, serialized_transaction_in->packet_attr);
	packet->packet_attr &= ~FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED; /* The semaphore can be initialized only locally */
#endif

    

	if (serialized_transaction_in->packge_id == FBE_PACKAGE_ID_PHYSICAL) {
		fbe_payload_ex_set_sg_list (new_payload, (fbe_sg_element_t *)sg_element_list, serialized_transaction_in->sg_elements_count);
         /*Now Copy key_handle*/
        fbe_payload_ex_set_key_handle(new_payload, serialized_transaction_in->key_handle);
	}else{
		fbe_payload_ex_set_sg_list (new_sep_payload, (fbe_sg_element_t *)sg_element_list, serialized_transaction_in->sg_elements_count);
         /*Now Copy key_handle*/
        fbe_payload_ex_set_key_handle(new_sep_payload, serialized_transaction_in->key_handle);
	}

	*returned_sg_elements_list = sg_element_list;
	return FBE_STATUS_OK;
}

fbe_status_t fbe_serialize_lib_repack_transaction(fbe_serialized_control_transaction_t *serialized_transaction, fbe_packet_t *packet)
{
	fbe_payload_ex_t *							returned_payload = NULL;
	fbe_payload_ex_t *						returned_sep_payload = NULL;
	fbe_payload_control_operation_t *		returned_control_operation = NULL;
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_u32_t								qualifier = 0;
	fbe_sg_element_t *						sg_element;

	if (serialized_transaction->packge_id == FBE_PACKAGE_ID_PHYSICAL) {
		returned_payload = fbe_transport_get_payload_ex (packet);
		returned_control_operation = fbe_payload_ex_get_control_operation(returned_payload);
	}else{
		returned_sep_payload = fbe_transport_get_payload_ex (packet);
		returned_control_operation = fbe_payload_ex_get_control_operation(returned_sep_payload);
	}

    status = fbe_transport_get_status_code(packet);
	qualifier = fbe_transport_get_status_qualifier(packet);
	serialized_transaction->packet_status.code = status;
	serialized_transaction->packet_status.qualifier = qualifier;

    fbe_payload_control_get_status(returned_control_operation, &serialized_transaction->payload_control_status);
	fbe_payload_control_get_status_qualifier(returned_control_operation, &serialized_transaction->payload_control_qualifier);

	/*free the sg elements we created*/
	sg_element = (fbe_sg_element_t *)(fbe_ptrhld_t)serialized_transaction->spare_opaque;
    if (sg_element != NULL) {
		free(sg_element);
	}

	return FBE_STATUS_OK;

}

static void * fbe_serialize_lib_fix_sg_pointers(fbe_u8_t *ioBuffer, fbe_u32_t element_count)
{
	fbe_sg_element_t *		sg_element = NULL;
    fbe_sg_element_t *		tmp_sg_element = NULL;
	fbe_sg_element_t *		sg_element_head = NULL;
    fbe_u32_t *				count = (fbe_u32_t *)ioBuffer;/*this is where the first count it*/
    fbe_u32_t				user_sg_count = 0;
	fbe_u8_t *				sg_addr = NULL;
    
    /*if we have 0 at the first entry it means we have no sg list there*/
	if (*count == 0) {
		return NULL;/*no sg list to process*/
	}

	/*since the buffer that comes from user space does not have the actual sg entries but only a list of counts, we need to allocate her our
	own sg list buffer and then free it upon completion. we add one for the zero termination*/
	sg_element_head = (fbe_sg_element_t *)malloc(sizeof(fbe_sg_element_t) * (element_count + 1));
	tmp_sg_element = sg_element_head;

    /*point to where we placed the data from the user which starts exactly after the location of the count values for each entry*/
	sg_addr = (fbe_u8_t *)(ioBuffer + (sizeof(sg_element->count) * (element_count)));

    /*go over the list and fix the pointers. We would need them to point correctly because we would use the address later
	to convert it to a physical address*/
    while (user_sg_count < element_count) {
		tmp_sg_element->address = sg_addr;
		tmp_sg_element->count = *count;

		/*move everyone forward*/
		sg_addr+= tmp_sg_element->count;
		tmp_sg_element++;
		count++;

		user_sg_count++;/*count how many we did*/
	}

	/*null termination*/
	tmp_sg_element->address = NULL;
	tmp_sg_element->count = 0;

   return (void *)sg_element_head;
}
