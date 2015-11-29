/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cmi_kernel_message_pool.c
 ***************************************************************************
 *
 *  Description
 *      This file contains the managament of the message pool
 *      
 *    
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi_private.h"
#include "fbe/fbe_memory.h"
#include "fbe_cmi.h"
#include "fbe/fbe_trace_interface.h"
#include "spid_types.h" 
#include "spid_kernel_interface.h" 
#include "fbe_cmi_kernel_private.h"
#include "fbe_panic_sp.h"

#define FBE_CMI_KERNEL_MAX_SEP_MESSAGES		8000
#define FBE_CMI_KERNEL_MAX_MESSAGES			2000
#define FBE_CMI_KERNEL_IRP_STACK_SIZE	3

/* Local definitions*/
typedef struct fbe_cmi_kernel_message_pool_s{
	fbe_queue_element_t				queue_element;/*always first*/
	fbe_cmi_kernel_message_info_t 	fbe_cmi_msg;
	fbe_bool_t			 			need_to_free_memory;
	/*mark the fact the IRP we used to send the initial message to CMI has completed
	only after this happens + we got the TRANSMITTED ack, only then we can return it to the pool*/
	fbe_bool_t						irp_or_ack_completed;
}fbe_cmi_kernel_message_pool_t;

/* Local members*/
static fbe_queue_head_t	message_pool_head;
static fbe_spinlock_t	message_pool_lock;

/*forward declerations*/
static __forceinline fbe_cmi_kernel_message_pool_t * 
fbe_cmi_kernel_cmi_msg_to_pool_msg(fbe_cmi_kernel_message_info_t *returned_msg);


/***********************************************************************************************************************************************/
fbe_status_t fbe_cmi_kernel_allocate_message_pool(void)
{
	fbe_u32_t							msg_count = 0, total_msg = 0;
	fbe_cmi_kernel_message_pool_t *		pool_msg = NULL;
	fbe_package_id_t					package_id = FBE_PACKAGE_ID_INVALID;
	fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;

	status = fbe_get_package_id(&package_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s Failed to get package ID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	if (package_id == FBE_PACKAGE_ID_SEP_0) {
		total_msg = FBE_CMI_KERNEL_MAX_SEP_MESSAGES;
	}else{
		total_msg = FBE_CMI_KERNEL_MAX_MESSAGES;
	}

	fbe_cmi_trace(FBE_TRACE_LEVEL_INFO,"%s Allocated %d msg for package:%d\n", __FUNCTION__, total_msg, package_id );
    
	/*initialize the message pool locks and queue*/
	fbe_spinlock_init(&message_pool_lock);
	fbe_queue_init(&message_pool_head);

	/*and allocate some structures (no need to lock here since we are at init stage)
	we add FBE_CMI_KERNEL_IRP_STACK_SIZE * stack size to compensate for the memory the IRP will take*/
	while (msg_count < total_msg) {
		pool_msg = (fbe_cmi_kernel_message_pool_t *)fbe_allocate_nonpaged_pool_with_tag( sizeof(fbe_cmi_kernel_message_pool_t), 'imcf');
		if (pool_msg == NULL) {
			fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Unable to allocate cmi message pool\n", __FUNCTION__);
			return FBE_STATUS_GENERIC_FAILURE;
		}

		/*add to the queue*/
		fbe_zero_memory(pool_msg, sizeof(fbe_cmi_kernel_message_pool_t));
		fbe_queue_element_init(&pool_msg->queue_element);
		/*Allocate and initialize the IRP*/
        pool_msg->fbe_cmi_msg.pirp = EmcpalIrpAllocate(FBE_CMI_KERNEL_IRP_STACK_SIZE);/*IoAllocateIrp always allocates from NonPaged so we should be set here*/
		pool_msg->irp_or_ack_completed = FBE_FALSE;

		fbe_queue_push(&message_pool_head, &pool_msg->queue_element);

		msg_count ++;

	}

	return FBE_STATUS_OK;

}

void fbe_cmi_kernel_destroy_message_pool(void)
{
	fbe_cmi_kernel_message_pool_t *		fbe_cmi_msg = NULL;
	fbe_u32_t							msg_count = 0, total_msg = 0;
	fbe_package_id_t					package_id = FBE_PACKAGE_ID_INVALID;
	fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;

	status = fbe_get_package_id(&package_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s Failed to get package ID\n", __FUNCTION__);
        return;
    }

	if (package_id == FBE_PACKAGE_ID_SEP_0) {
		total_msg = FBE_CMI_KERNEL_MAX_SEP_MESSAGES;
	}else{
		total_msg = FBE_CMI_KERNEL_MAX_MESSAGES;
	}

	while (msg_count < total_msg) {
		fbe_cmi_msg = (fbe_cmi_kernel_message_pool_t *)fbe_queue_pop(&message_pool_head);
        EmcpalIrpFree(fbe_cmi_msg->fbe_cmi_msg.pirp);
        fbe_release_nonpaged_pool_with_tag((void *)fbe_cmi_msg, 'imcf');
		msg_count ++;
	}

	fbe_spinlock_destroy(&message_pool_lock);
	fbe_queue_destroy(&message_pool_head);

	return;
}

fbe_cmi_kernel_message_info_t * fbe_cmi_kernel_get_message_info_memory(void)
{
	fbe_cmi_kernel_message_pool_t *		msg_ptr = NULL;

	fbe_spinlock_lock(&message_pool_lock);

	/*let's see if we have enough messages on the queue*/
	if (!fbe_queue_is_empty(&message_pool_head)) {
		msg_ptr = (fbe_cmi_kernel_message_pool_t *)fbe_queue_pop(&message_pool_head);
		fbe_spinlock_unlock(&message_pool_lock);

		/*we reset the IRP for future use*/
		EmcpalIrpReuse(msg_ptr->fbe_cmi_msg.pirp, EMCPAL_STATUS_PENDING);

		/*mark we don't need to free the memory*/
		msg_ptr->need_to_free_memory = FBE_FALSE;
		return &msg_ptr->fbe_cmi_msg;
	}else{
		fbe_spinlock_unlock(&message_pool_lock);

		fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s Out of packets in pool, Need to allocate from OS !\n", __FUNCTION__);

		/*for checked build we panic so we can find it in testing*/
		#if DBG
		FBE_PANIC_SP(FBE_PANIC_SP_BASE, FBE_STATUS_OK);
		#endif
		msg_ptr = (fbe_cmi_kernel_message_pool_t *) fbe_allocate_nonpaged_pool_with_tag(
																		  sizeof(fbe_cmi_kernel_message_pool_t),
																		  'imcf');
		if (msg_ptr == NULL) {
			fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Unable to allocate cmi message\n", __FUNCTION__);
			return NULL;
		}
		msg_ptr->fbe_cmi_msg.pirp = EmcpalIrpAllocate(FBE_CMI_KERNEL_IRP_STACK_SIZE);/*IoAllocateIrp always allocates from NonPaged so we should be set here*/
        
		/*mark we need to free the memory*/
		msg_ptr->need_to_free_memory = FBE_TRUE;
		msg_ptr->irp_or_ack_completed = FBE_FALSE;
		return &msg_ptr->fbe_cmi_msg;

	}

}

void fbe_cmi_kernel_return_message_info_memory(fbe_cmi_kernel_message_info_t *returned_msg)
{
	fbe_cmi_kernel_message_pool_t * pool_msg = NULL;

    fbe_spinlock_lock(&message_pool_lock);

	/*we need to cast up from the data pointer to us*/
	pool_msg = fbe_cmi_kernel_cmi_msg_to_pool_msg(returned_msg);

	/*let's check if we have already did an IRP comletion on this message, only if we did, we can really return it to the pool*/
	if (FBE_IS_FALSE(pool_msg->irp_or_ack_completed)) {
		/*don't touch it yet, just return, when the IRP will complete, we'll stick in back on the queue*/
		pool_msg->irp_or_ack_completed = FBE_TRUE;
		fbe_spinlock_unlock(&message_pool_lock);
		return;
	}

	pool_msg->irp_or_ack_completed = FBE_FALSE;/*rest for next time*/

    /*and return to the pool if it came from it*/
	if (!pool_msg->need_to_free_memory) {
		fbe_queue_push_front(&message_pool_head, &pool_msg->queue_element);
	}else{
		fbe_release_nonpaged_pool_with_tag((void *)pool_msg, 'imcf');
	}

    fbe_spinlock_unlock(&message_pool_lock);
}

static __forceinline fbe_cmi_kernel_message_pool_t * 
fbe_cmi_kernel_cmi_msg_to_pool_msg(fbe_cmi_kernel_message_info_t *returned_msg)
{
	fbe_cmi_kernel_message_pool_t *  pool_msg = NULL;
	pool_msg = (fbe_cmi_kernel_message_pool_t *)((fbe_u8_t *)returned_msg - (fbe_u8_t *)(&((fbe_cmi_kernel_message_pool_t  *)0)->fbe_cmi_msg));
	return pool_msg;
}

void fbe_cmi_kernel_mark_irp_completed(fbe_cmi_kernel_message_info_t *returned_msg)
{
	fbe_cmi_kernel_message_pool_t * pool_msg = NULL;

    fbe_spinlock_lock(&message_pool_lock);

	/*we need to cast up from the data pointer to us*/
	pool_msg = fbe_cmi_kernel_cmi_msg_to_pool_msg(returned_msg);

	/*let's check if we have already did an ACK on this message, only if we did, we can really return it to the pool*/
	if (FBE_IS_FALSE(pool_msg->irp_or_ack_completed)) {
		/*don't touch it yet, just return. When we get the ack we'll put it back in the queue*/
		pool_msg->irp_or_ack_completed = FBE_TRUE;
		fbe_spinlock_unlock(&message_pool_lock);
		return;
	}

	pool_msg->irp_or_ack_completed = FBE_FALSE;/*rest for next time*/

    /*and return to the pool if it came from it*/
	if (!pool_msg->need_to_free_memory) {
		fbe_queue_push_front(&message_pool_head, &pool_msg->queue_element);
	}else{
		fbe_release_nonpaged_pool_with_tag((void *)pool_msg, 'imcf');
	}

    fbe_spinlock_unlock(&message_pool_lock);
}
