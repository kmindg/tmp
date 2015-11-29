/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_packet_serialize_kerenl.c
 ***************************************************************************
 *
 *  Description
 *      kernel space implementation of packet serialization
 *
 *  History:
 *      12/08/09    sharel - Created
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe_packet_serialize_lib.h"


/**************************************
				Local variables
**************************************/
/*******************************************
				Local functions
********************************************/
static void * fbe_serialize_lib_fix_sg_pointers(fbe_u8_t *ioBuffer, fbe_u32_t element_count, fbe_packet_attr_t packet_attr);


/*********************************************************************
 *            fbe_serialize_lib_serialize_packet ()
 *********************************************************************
 *
 *  Description: serialize a packet to a contigous buffer
 *
 *	Inputs: packet from user
 *          pointer to function allocated memory of contigous buffer
 *
 *  Return Value: status of success
 *
 *********************************************************************/
fbe_status_t fbe_serialize_lib_serialize_packet(fbe_packet_t *packet_in,
												fbe_serialized_control_transaction_t **serialized_transaction_out,
												OUT fbe_u32_t *serialized_transaction_size)
{
	/*No current use case, we don't send from kernel space to another space*/
	return FBE_STATUS_OK;

}

/*********************************************************************
 *            fbe_serialize_lib_unpack_transaction ()
 *********************************************************************
 *
 *  Description: convert the transaction we got into a packet so we can use it in kernel
 *
 *	Inputs: transaction from user
 *          pointer to user allocated packet to fill
 *			sg_elements_list - memory that will be allocated for sg list(user needs to free)
 *
 *  Return Value: status of success
 *
 *********************************************************************/
fbe_status_t fbe_serialize_lib_unpack_transaction(fbe_serialized_control_transaction_t *serialized_transaction_in,
												  fbe_packet_t *packet,
												  void **returned_sg_elements_list)
{
    fbe_u8_t * 								user_buffer = NULL;
    fbe_u32_t								packet_size = (fbe_u32_t)sizeof(fbe_serialized_control_transaction_t);
    fbe_payload_ex_t *						new_payload = NULL;
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
                                                             serialized_transaction_in->sg_elements_count,
															 serialized_transaction_in->packet_attr);
		if (sg_element_list == NULL) {
			fbe_base_library_trace (FBE_LIBRARY_ID_PACKET_SERIALIZE,
						FBE_TRACE_LEVEL_WARNING,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s unable to get memory. SG Count:%d\n",
						__FUNCTION__, serialized_transaction_in->sg_elements_count);
			return FBE_STATUS_GENERIC_FAILURE;
		}

		serialized_transaction_in->spare_opaque = (fbe_u64_t)sg_element_list;/*keep for later since it may be lost in completion*/
	}else{
		serialized_transaction_in->spare_opaque = 0;
	}
    
    fbe_transport_set_address(packet,
							  serialized_transaction_in->packge_id,
							  serialized_transaction_in->service_id,
							  serialized_transaction_in->class_id,
							  serialized_transaction_in->object_id);

	fbe_transport_set_packet_attr(packet, serialized_transaction_in->packet_attr);
	packet->packet_attr &= ~FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED; /* The semaphore can be initialized only locally */

	if (serialized_transaction_in->packge_id == FBE_PACKAGE_ID_PHYSICAL) {
		fbe_payload_ex_set_sg_list (new_payload, (fbe_sg_element_t *)sg_element_list, serialized_transaction_in->sg_elements_count);
        fbe_payload_ex_set_key_handle(new_payload, serialized_transaction_in->key_handle);
	}else{
		fbe_payload_ex_set_sg_list (new_sep_payload, (fbe_sg_element_t *)sg_element_list, serialized_transaction_in->sg_elements_count);
        fbe_payload_ex_set_key_handle(new_sep_payload, serialized_transaction_in->key_handle);
	}

	*returned_sg_elements_list = sg_element_list;
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_serialize_lib_repack_transaction ()
 *********************************************************************
 *
 *  Description: copy data from the packet to the transaction so it can be returned to user space
 *
 *	Inputs: transaction from user to fill with status
 *          pointer to user allocated packet to take data from
 *
 *  Return Value: status of success
 *
 *********************************************************************/
fbe_status_t fbe_serialize_lib_repack_transaction(fbe_serialized_control_transaction_t *serialized_transaction, fbe_packet_t *packet)
{
	fbe_payload_ex_t *							returned_payload = NULL;
	fbe_payload_ex_t *						returned_sep_payload = NULL;
	fbe_payload_control_operation_t *		returned_control_operation = NULL;
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_u32_t								qualifier = 0;
	fbe_sg_element_t *						sg_element;
    fbe_u8_t *								user_buffer = NULL;

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

	/*copy the sg list data to the buffer that will be returned to the user and delte the physically contiguos one we created*/
	sg_element = (fbe_sg_element_t *)serialized_transaction->spare_opaque;

    if (sg_element != NULL) {
		/*where the user buffer starts*/
	    user_buffer = (fbe_u8_t *)((fbe_u8_t *)serialized_transaction + sizeof(fbe_serialized_control_transaction_t));
		user_buffer += serialized_transaction->buffer_length;/*the start of sg_element counts*/
		user_buffer += (serialized_transaction->sg_elements_count * sizeof(sg_element->count));/*skip the counts themselvs*/

		/*and copy everything to the actual sg area from the user*/
		while(sg_element->count != 0) {
			fbe_copy_memory(user_buffer, sg_element->address, sg_element->count);
			if (serialized_transaction->packet_attr & FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED) {
				fbe_memory_ex_release((void *)sg_element->address);/*don't need it anymore*/
			}else{
				fbe_release_contiguous_memory((void *)sg_element->address);/*don't need it anymore*/
			}
			user_buffer += sg_element->count;
			sg_element++;
		}
		if (serialized_transaction->packet_attr & FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED) {
			fbe_memory_ex_release((void *)serialized_transaction->spare_opaque);
		}else{
			fbe_release_contiguous_memory((void *)serialized_transaction->spare_opaque);/*don't need the sg list anymore*/
		}
	}

    return FBE_STATUS_OK;
}

static void * fbe_serialize_lib_fix_sg_pointers(fbe_u8_t *ioBuffer, fbe_u32_t element_count, fbe_packet_attr_t packet_attr)
{
	fbe_sg_element_t *		sg_element = NULL;
    fbe_sg_element_t *		tmp_sg_element = NULL;
	fbe_sg_element_t *		sg_element_head = NULL;
    fbe_u32_t *				count = (fbe_u32_t *)ioBuffer;/*this is where the first count it*/
    fbe_u32_t				user_sg_count = 0;
	fbe_u8_t *				sg_addr = NULL;
	fbe_u8_t *				conitg_sg_addr = NULL;

    /*if we have 0 at the first entry it means we have no sg list there*/
	if (*count == 0) {
		return NULL;/*no sg list to process*/
	}

	/*since the buffer that comes from user space does not have the actual sg entries but only a list of counts, we need to allocate her our
	own sg list buffer and then free it upon completion. we add one for the zero termination*/
	if (packet_attr & FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED) {
		sg_element_head = (fbe_sg_element_t *)fbe_memory_ex_allocate(sizeof(fbe_sg_element_t) * (element_count + 1));
	}else{
		sg_element_head = (fbe_sg_element_t *)fbe_allocate_contiguous_memory(sizeof(fbe_sg_element_t) * (element_count + 1));
	}

	if (sg_element_head == NULL){
	    fbe_base_library_trace (FBE_LIBRARY_ID_PACKET_SERIALIZE,
				    FBE_TRACE_LEVEL_WARNING,
				    FBE_TRACE_MESSAGE_ID_INFO,
				    "%s unable to get memory. Element Count:%d Attr:0x%08x\n",
				    __FUNCTION__, (fbe_u32_t)(sizeof(fbe_sg_element_t) * (element_count + 1)),
				    packet_attr);
	    return NULL;
	}

	tmp_sg_element = sg_element_head;

    /*point to where we placed the data from the user which starts exactly after the location of the count values for each entry*/
	sg_addr = (fbe_u8_t *)(ioBuffer + (sizeof(sg_element->count) * (element_count)));

    /*go over the list and fix the pointers. We would need them to point correctly because we would use the address later
	to convert it to a physical address*/
    while (user_sg_count < element_count) {

		tmp_sg_element->count = *count;

		/*the problem we have here is the buffer we got from user space is now in kernel and taken from the non paged pool,
		HOWEVER, it is not guaranteed to be physically contiguos so we must allocate our own memory and copy the stuff here*/
		if (packet_attr & FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED) {
			conitg_sg_addr = (fbe_u8_t *)fbe_memory_ex_allocate(tmp_sg_element->count);
		}else{
			conitg_sg_addr = (fbe_u8_t *)fbe_allocate_contiguous_memory(tmp_sg_element->count);
		}

		/*Huston, we have a problem*/
		if (conitg_sg_addr == NULL){
		    fbe_base_library_trace (FBE_LIBRARY_ID_PACKET_SERIALIZE,
					    FBE_TRACE_LEVEL_WARNING,
					    FBE_TRACE_MESSAGE_ID_INFO,
					    "%s unable to get memory. Size:%d Attr:0x%08x\n",
					    __FUNCTION__, tmp_sg_element->count,
					    packet_attr);

			/*free the memory for each entry in the sg list we allocated so far*/
			tmp_sg_element = sg_element_head;
            
			while (user_sg_count != 0) {
				if (packet_attr & FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED) {
					fbe_memory_ex_release(tmp_sg_element->address);
				}else{
					fbe_release_contiguous_memory(tmp_sg_element->address);
				}
				fbe_base_library_trace (FBE_LIBRARY_ID_PACKET_SERIALIZE,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s Releasing memory. Size:%d Attr:0x%08x\n",
                                        __FUNCTION__, tmp_sg_element->count,
                                        packet_attr);
				user_sg_count--;
				tmp_sg_element++;
			}

			/*and free the head*/
			if (packet_attr & FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED) {
				fbe_memory_ex_release(sg_element_head);
			}else{
				fbe_release_contiguous_memory(sg_element_head);
			}
			return NULL;
		}

		tmp_sg_element->address = conitg_sg_addr;
              fbe_copy_memory(tmp_sg_element->address, sg_addr, tmp_sg_element->count);
        
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

