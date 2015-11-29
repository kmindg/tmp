/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cms_cmi.c
 ***************************************************************************
 *
 *  Description
 *      Basic level implementation of the CMI path of Cluster Memory service
 *    
 ***************************************************************************/

#include "fbe_cms_private.h"
#include "fbe_cmm.h"
#include "fbe_cmi.h"
#include "fbe_cms_cmi.h"
#include "fbe_cms_cluster_cmi.h"
#include "fbe_cms_memory_cmi.h"
#include "fbe_cms_sm_cmi.h"
#include "fbe/fbe_multicore_queue.h"


/*******************/
/*local definitions*/
/*******************/
#define FBE_CMS_CMI_CLUSTER_MB_ALLOCATION	4
#define FBE_CMS_CMI_CLUSTER_BUFFERS_PER_CMM_CHUNK (1048576/sizeof(fbe_cms_cmi_cluster_msg_t))
#define FBE_CMS_CMI_CLUSTER_BUFFERS_PER_CORE 8

/*data structure to hold pre allocated CMI messages for cluster*/
typedef struct fbe_cms_cluster_buffer_s{
    fbe_queue_element_t q_element;/*this queue element is used to queue the buffer on a queue of pre allocated messages*/
    fbe_cms_cmi_cluster_msg_t cmi_msg;
}fbe_cms_cluster_buffer_t;

/*internal data related to the CMS CMI module*/
typedef struct fbe_cms_cmi_info_s{
    fbe_multicore_queue_t				cluster_buffer_queue_head;/*queue head for cluster related pre allocated messages*/
    fbe_multicore_queue_t				outstanding_cluster_queue_head;/*outstanding CMI messages for the cluster path*/
    CMM_POOL_ID							cmm_control_pool_id;
	CMM_CLIENT_ID						cmm_client_id;
	void *								cmm_chunk_addr[FBE_CMS_CMI_CLUSTER_MB_ALLOCATION];
	fbe_cms_cmi_memory_msg_t  *			memory_msg_send_buffer;/* the buffer we use to send memory messages across CMI*/
	fbe_cms_cmi_memory_msg_t *	 		memory_msg_rcv_buffer;/* the buffer we use to get memory messages across CMI from peer*/
	fbe_cms_cmi_state_machine_msg_t * 	state_machine_send_buffer;/*the buffer we send SM messages with*/
	fbe_cms_cmi_state_machine_msg_t * 	state_machine_rcv_buffer;/*the buffer we receive SM messages into from peer*/
	fbe_cpu_id_t						cpu_count;/*how many cpus we have on the array*/
	fbe_atomic_t						state_machine_cmi_send_msg_count;/*make sure we have only one outstanding*/
	fbe_atomic_t						memoy_cmi_send_msg_count;/*make sure we have only one outstanding*/
	fbe_atomic_t						state_machine_cmi_rcv_msg_count;/*make sure we have only one outstanding*/
	fbe_atomic_t						memoy_cmi_rcv_msg_count;/*make sure we have only one outstanding*/
}fbe_cms_cmi_info_t;

/*******************/
/* local arguments */
/*******************/

static fbe_cms_cmi_info_t		fbe_cms_cmi_info;/*holds all data related to CMS CMI operation*/

/*******************/
/* local function  */
/*******************/
static fbe_status_t fbe_cms_cmi_allocate_memory(void);
static void fbe_cms_cmi_destroy_memory(void);
static fbe_status_t fbe_cms_cmi_process_contact_lost(void);
static fbe_status_t fbe_cms_cmi_process_message_queue(void *msg);
static fbe_status_t fbe_cms_cmi_process_received_message(fbe_cms_cmi_msg_common_t *cms_cmi_msg);
static fbe_cms_cmi_msg_common_t * fbe_cms_cmi_copy_received_msg(fbe_cms_cmi_msg_common_t * user_message);
static __forceinline fbe_cms_cluster_buffer_t * cluster_msg_to_cluster_buffer(fbe_cms_cmi_cluster_msg_t * cluster_msg);
static void fbe_cms_cmi_process_peer_reply(fbe_cms_cmi_msg_common_t *cms_cmi_msg);
static void fbe_cms_cmi_return_msg_memory(fbe_cms_cmi_msg_common_t *cms_cmi_msg);
/*************************************************************************************************************************************************************/

static fbe_status_t fbe_cms_cmi_callback(fbe_cmi_event_t event, fbe_u32_t user_message_length, fbe_cmi_message_t user_message, fbe_cmi_event_callback_context_t context)
{
    fbe_status_t				status = FBE_STATUS_OK;
    fbe_cms_cmi_msg_common_t *	cms_cmi_msg = NULL;
	fbe_cms_cmi_cluster_msg_t *	cluster_msg = NULL;
    
    /*we don't want to process stuff on the context of the cmi callback sinct it's directly from the hardware
    at DISPATCH_LEVEL so we want to queue it to another thread to do the work*/

    switch (event) {
    case FBE_CMI_EVENT_MESSAGE_RECEIVED:
        /*for new messages which we did not generate, we get a buffer and copy the messge into*/
        cms_cmi_msg = fbe_cms_cmi_copy_received_msg((fbe_cms_cmi_msg_common_t *)user_message);
        if (cms_cmi_msg == NULL) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        break;
    case FBE_CMI_EVENT_SP_CONTACT_LOST:
        /*get the memory for a message, let's steal it from the pool of the cpu we run in right now*/
		cluster_msg = fbe_cms_cmi_get_cluster_msg();
        cms_cmi_msg = &cluster_msg->common_msg_data;
		cms_cmi_msg->cpu_sent_on = fbe_get_cpu_id();
        break;
    case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
    case FBE_CMI_EVENT_FATAL_ERROR:
    case FBE_CMI_EVENT_PEER_NOT_PRESENT:
    case FBE_CMI_EVENT_PEER_BUSY:
        /*no need to allocate since we are the ones who sent the messages so we allocated the memory before*/
        cms_cmi_msg = (fbe_cms_cmi_msg_common_t *)user_message;
        break;
    default:
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s, Invalid state %d\n", __FUNCTION__, event);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*remember the important stuff for processign by the thread*/
    cms_cmi_msg->event_type = event;
    cms_cmi_msg->callback_context = context;

	/*and queue it for processing by the thread the is in charge of the core that sent it.
	when CMID driver will be fixe, we can just complete on the current cpu, 
	but for now it's a affined to a single core so we have to fix it*/
    status =  fbe_cms_run_queue_push(&cms_cmi_msg->run_queue_info,
									 fbe_cms_cmi_process_message_queue,
									 cms_cmi_msg->cpu_sent_on,
									 cms_cmi_msg,
                                     FBE_CMS_RUN_QUEUE_FLAGS_NONE);

	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s, bad status from fbe_cms_run_queue_push, msg dropped\n", __FUNCTION__);
		if (event == FBE_CMI_EVENT_SP_CONTACT_LOST || event == FBE_CMI_EVENT_MESSAGE_RECEIVED) {
			fbe_cms_cmi_return_msg_memory(cms_cmi_msg);/*let's not leak*/

		}

        return FBE_STATUS_GENERIC_FAILURE;
	}

    return status;
}

fbe_status_t fbe_cms_cmi_init(void)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
	EMCPAL_STATUS 	nt_status;
    
	fbe_cms_cmi_info.cpu_count = fbe_get_cpu_count();
	fbe_cms_cmi_info.state_machine_cmi_send_msg_count = 0;
	fbe_cms_cmi_info.memoy_cmi_send_msg_count = 0;
	fbe_cms_cmi_info.state_machine_cmi_rcv_msg_count = 0;
	fbe_cms_cmi_info.memoy_cmi_rcv_msg_count = 0;

	 /*resources for the cluster buffers queue*/
	fbe_multicore_queue_init(&fbe_cms_cmi_info.cluster_buffer_queue_head);
    fbe_multicore_queue_init(&fbe_cms_cmi_info.outstanding_cluster_queue_head);
   
    /*let's allocate memory so we can copy incoming messages into it*/
    status  = fbe_cms_cmi_allocate_memory();
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR,"%s failed to allocate msg memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_cmi_register(FBE_CMI_CLIENT_ID_CMS_TAGS, fbe_cms_cmi_callback, NULL);
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR,"%s failed to init tags CMI connection\n", __FUNCTION__);
		fbe_cms_cmi_destroy_memory();
        return FBE_STATUS_GENERIC_FAILURE;
    }

	status =  fbe_cmi_register(FBE_CMI_CLIENT_ID_CMS_STATE_MACHINE, fbe_cms_cmi_callback, NULL);
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR,"%s failed to init SM CMI connection\n", __FUNCTION__);
		fbe_cms_cmi_destroy_memory();
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    nt_status = fbe_thread_init(&fbe_cms_cmi_thread_handle, "fbe_cms_cmi", fbe_cms_cmi_thread_func, NULL);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_cmi_destroy(void)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;

	status =  fbe_cmi_unregister(FBE_CMI_CLIENT_ID_CMS_TAGS);
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR,"%s failed to unregister Tags CMI connection\n", __FUNCTION__);
    }

	status =  fbe_cmi_unregister(FBE_CMI_CLIENT_ID_CMS_STATE_MACHINE);
    if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s failed to unregister SM CMI connection\n", __FUNCTION__);
    }

	fbe_cms_cmi_destroy_memory();

    fbe_multicore_queue_destroy(&fbe_cms_cmi_info.cluster_buffer_queue_head);
	fbe_multicore_queue_destroy(&fbe_cms_cmi_info.outstanding_cluster_queue_head);

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_cms_cmi_process_contact_lost(void)
{
    return fbe_cms_process_lost_peer();
}

static fbe_status_t fbe_cms_cmi_process_message_queue(void * msg)
{
    fbe_cms_cmi_msg_common_t *	cms_cmi_msg = (fbe_cms_cmi_msg_common_t *)msg;
    
    switch (cms_cmi_msg->event_type) {
	case FBE_CMI_EVENT_MESSAGE_RECEIVED:
		fbe_cms_cmi_process_received_message(cms_cmi_msg);
		/*since we allocate the memory to copy the peer message we also want to free it after the user is done taking care of it*/
		fbe_cms_cmi_return_msg_memory(cms_cmi_msg);
		break;
	case FBE_CMI_EVENT_SP_CONTACT_LOST:
		fbe_cms_cmi_process_contact_lost();
		/*we used the cluster queue to return the memory so we better return to it*/
		fbe_cms_cmi_return_msg_memory(cms_cmi_msg);
		break;
	case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
    case FBE_CMI_EVENT_FATAL_ERROR:
	case FBE_CMI_EVENT_PEER_NOT_PRESENT:
	case FBE_CMI_EVENT_PEER_BUSY:
		fbe_cms_cmi_process_peer_reply(cms_cmi_msg);
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s, Invalid state (%d)\n", __FUNCTION__, cms_cmi_msg->event_type);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static fbe_status_t fbe_cms_cmi_allocate_memory(void)
{
    CMM_ERROR 						cmm_error;
    fbe_cms_cluster_buffer_t *		cluster_msg_ptr;
    fbe_u8_t *						cmm_mem_ptr = NULL;
    fbe_u32_t						count = 0;
    fbe_u32_t						chunk = 0;
	fbe_cpu_id_t					current_cpu = 0;
	fbe_u32_t						allocations_per_cmm_chunk = 0;
	fbe_u32_t						msg_per_cpu = 0;

	/*easy stuff first, let's allocate the memory for the state machine and the buffers messages since we have only two of each,
	and we'll just keep their pointers. We have to remember to allocate Physically Contig. memory here.*/
	fbe_cms_cmi_info.state_machine_rcv_buffer = (fbe_cms_cmi_state_machine_msg_t *)fbe_memory_native_allocate(sizeof(fbe_cms_cmi_state_machine_msg_t));
	fbe_cms_cmi_info.state_machine_send_buffer = (fbe_cms_cmi_state_machine_msg_t *)fbe_memory_native_allocate(sizeof(fbe_cms_cmi_state_machine_msg_t));
	if (fbe_cms_cmi_info.state_machine_rcv_buffer == NULL || fbe_cms_cmi_info.state_machine_send_buffer == NULL) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: Falied to allocate SM msg memory\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}else{
		fbe_cms_cmi_info.state_machine_rcv_buffer->common_msg_data.cms_cmi_msg_type = FBE_CMS_CMI_STATE_MACHINE_MESSAGE;
		fbe_cms_cmi_info.state_machine_send_buffer->common_msg_data.cms_cmi_msg_type = FBE_CMS_CMI_STATE_MACHINE_MESSAGE;
	}

	fbe_cms_cmi_info.memory_msg_rcv_buffer = (fbe_cms_cmi_memory_msg_t *)fbe_memory_native_allocate(sizeof(fbe_cms_cmi_memory_msg_t));
	fbe_cms_cmi_info.memory_msg_send_buffer = (fbe_cms_cmi_memory_msg_t *)fbe_memory_native_allocate(sizeof(fbe_cms_cmi_memory_msg_t));
	if (fbe_cms_cmi_info.memory_msg_rcv_buffer == NULL || fbe_cms_cmi_info.memory_msg_send_buffer == NULL) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: Falied to allocate memory buffers msg memory\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}else{
		fbe_cms_cmi_info.memory_msg_rcv_buffer->common_msg_data.cms_cmi_msg_type = FBE_CMS_CMI_MEMORY_MESSAGE;
		fbe_cms_cmi_info.memory_msg_send_buffer->common_msg_data.cms_cmi_msg_type = FBE_CMS_CMI_MEMORY_MESSAGE;
	}
    
    /*now let's allocate memory for the tags. This has to be per core as well.
	we use CMM and not fbe_sep_memory_allocate because this service will potentially be out of SEP*/
    fbe_cms_cmi_info.cmm_control_pool_id = cmmGetLongTermControlPoolID();
    cmmRegisterClient(fbe_cms_cmi_info.cmm_control_pool_id, 
                      NULL, 
                      0,  /* Minimum size allocation */  
                      CMM_MAXIMUM_MEMORY,   
                      "FBE Cluster Memory CMI memory", 
                       &fbe_cms_cmi_info.cmm_client_id);

	 for (chunk = 0; chunk < FBE_CMS_CMI_CLUSTER_MB_ALLOCATION; chunk ++) {
        /* Allocate a 1MB chunk*/
        cmm_error = cmmGetMemoryVA(fbe_cms_cmi_info.cmm_client_id, 1048576, &fbe_cms_cmi_info.cmm_chunk_addr[chunk]);
        if (cmm_error != CMM_STATUS_SUCCESS) {
            cms_trace (FBE_TRACE_LEVEL_ERROR, "%s can't get cmm memory, err:%X\n", __FUNCTION__, cmm_error);
            return FBE_STATUS_GENERIC_FAILURE;
        }
     }

	 /*now, let's use the memory to allocate CMI messages per core*/
	 chunk = 0;
	 cmm_mem_ptr = fbe_cms_cmi_info.cmm_chunk_addr[chunk];/*start from the top*/

	 for (current_cpu = 0; current_cpu < fbe_cms_cmi_info.cpu_count; current_cpu++) {
		 for (msg_per_cpu = 0; msg_per_cpu < FBE_CMS_CMI_CLUSTER_BUFFERS_PER_CORE; msg_per_cpu++) {
			 cluster_msg_ptr = (fbe_cms_cluster_buffer_t *)cmm_mem_ptr;/*get the memory*/
			 cluster_msg_ptr->cmi_msg.common_msg_data.cms_cmi_msg_type = FBE_CMS_CMI_CLUSTER_MESSAGE;
			 fbe_multicore_queue_push(&fbe_cms_cmi_info.cluster_buffer_queue_head, &cluster_msg_ptr->q_element, current_cpu);
			 allocations_per_cmm_chunk++;
			 cmm_mem_ptr += sizeof(fbe_cms_cluster_buffer_t);
			 if (allocations_per_cmm_chunk == FBE_CMS_CMI_CLUSTER_BUFFERS_PER_CMM_CHUNK) {
				 allocations_per_cmm_chunk = 0;
				 chunk++;/*go to the next chunk*/
				 if (chunk == FBE_CMS_CMI_CLUSTER_MB_ALLOCATION) {
					 cms_trace (FBE_TRACE_LEVEL_ERROR, "%s out of cMM chunks\n", __FUNCTION__);
					 return FBE_STATUS_GENERIC_FAILURE; 
				 }
	
				 cmm_mem_ptr = fbe_cms_cmi_info.cmm_chunk_addr[chunk];
			 }
		 }

	 }
     
     return FBE_STATUS_OK;

}

static void cms_cmi_destroy_heap_allocated_memory(void *mem_ptr, fbe_atomic_t *count)
{
	fbe_atomic_t	val;
	fbe_u32_t		retry_count = 0;

	do {
		val = fbe_atomic_exchange(count, 1);
		if (val == 1) {
			fbe_thread_delay(100);
			retry_count++;
		}
	} while (val == 1 && retry_count < 100);

	if (val == 0) {
		/*we are good, nothing in flight*/
		fbe_memory_native_release(mem_ptr);
	}else{
		 cms_trace (FBE_TRACE_LEVEL_ERROR, "%s can't wait anymore will leak memory\n", __FUNCTION__);
	}

}

static void fbe_cms_cmi_destroy_memory(void)
{
    fbe_u32_t  					chunk = 0;
    fbe_u32_t 					count = 0;
    fbe_queue_element_t *		queue_element_p = NULL;
	fbe_cpu_id_t				current_cpu;

	cms_cmi_destroy_heap_allocated_memory((void *)fbe_cms_cmi_info.state_machine_rcv_buffer, &fbe_cms_cmi_info.state_machine_cmi_rcv_msg_count);
	cms_cmi_destroy_heap_allocated_memory((void *)fbe_cms_cmi_info.state_machine_send_buffer, &fbe_cms_cmi_info.state_machine_cmi_send_msg_count);
	cms_cmi_destroy_heap_allocated_memory((void *)fbe_cms_cmi_info.memory_msg_rcv_buffer, &fbe_cms_cmi_info.memoy_cmi_rcv_msg_count);
	cms_cmi_destroy_heap_allocated_memory((void *)fbe_cms_cmi_info.memory_msg_send_buffer, &fbe_cms_cmi_info.memoy_cmi_send_msg_count);

	for (current_cpu = 0; current_cpu < fbe_cms_cmi_info.cpu_count; current_cpu++) {
		fbe_multicore_queue_lock(&fbe_cms_cmi_info.outstanding_cluster_queue_head, current_cpu);
	
		/*do we have any outstanding ones ?*/
		while (!fbe_multicore_queue_is_empty(&fbe_cms_cmi_info.outstanding_cluster_queue_head, current_cpu) && count < 10) {
			/*let's wait for completion of a while*/
			fbe_multicore_queue_unlock(&fbe_cms_cmi_info.outstanding_cluster_queue_head, current_cpu);
			fbe_thread_delay(500);
			count++;
			fbe_multicore_queue_lock(&fbe_cms_cmi_info.outstanding_cluster_queue_head, current_cpu);
		}
	
		if (!fbe_multicore_queue_is_empty(&fbe_cms_cmi_info.outstanding_cluster_queue_head, current_cpu)) {
			/*there will be a memory leak eventually but we can't stop here forever*/
			cms_trace (FBE_TRACE_LEVEL_ERROR, "%s:outstanding msg not done in 5 sec, no CMM release\n", __FUNCTION__);
			/*just get out, since we won't release CMM anyway to prevent panic while downloading the drivers stack*/
			return;
		}
		
	}
   
    /*free CMM memory*/
    for (chunk = 0; chunk < FBE_CMS_CMI_CLUSTER_MB_ALLOCATION; chunk++) {
        cmmFreeMemoryVA(fbe_cms_cmi_info.cmm_client_id, fbe_cms_cmi_info.cmm_chunk_addr[chunk]);
    }
    
    /*unregister*/
    cmmDeregisterClient(fbe_cms_cmi_info.cmm_client_id);
}

/*get a pice of memory that contains a physically contig. memory with the bits of this message type,
we assume the caller of this function will also send and return the memory on the same core
and that it's core affined*/
fbe_cms_cmi_cluster_msg_t * fbe_cms_cmi_get_cluster_msg()
{
    fbe_cpu_id_t 				current_cpu_id = fbe_get_cpu_id();
    fbe_cms_cluster_buffer_t *	buffer_ptr = NULL;
	
	fbe_multicore_queue_lock(&fbe_cms_cmi_info.cluster_buffer_queue_head, current_cpu_id);

	if (!fbe_multicore_queue_is_empty(&fbe_cms_cmi_info.cluster_buffer_queue_head, current_cpu_id)) {
		buffer_ptr = (fbe_cms_cluster_buffer_t *)fbe_multicore_queue_pop(&fbe_cms_cmi_info.cluster_buffer_queue_head, current_cpu_id);
    }else{
		fbe_multicore_queue_unlock(&fbe_cms_cmi_info.cluster_buffer_queue_head, current_cpu_id);
		cms_trace (FBE_TRACE_LEVEL_ERROR, "%s, out of pre allocated msg\n", __FUNCTION__);
		return NULL;/*no memory, user should wait and try again*/
	}

	fbe_multicore_queue_push(&fbe_cms_cmi_info.outstanding_cluster_queue_head,
							 &buffer_ptr->q_element,
							 current_cpu_id);/*move to outstanding queue*/

	fbe_multicore_queue_unlock(&fbe_cms_cmi_info.cluster_buffer_queue_head, current_cpu_id);
	return &buffer_ptr->cmi_msg;
}

/*get a pice of memory that contains a physically contig. memory with the bits of this message type,
we assume the caller of this function will also send and return the memory on the same core
and that it's core affined*/
fbe_cms_cmi_state_machine_msg_t * fbe_cms_cmi_get_sm_msg()
{
	fbe_atomic_t				existing_count;
    
	existing_count = fbe_atomic_exchange(&fbe_cms_cmi_info.state_machine_cmi_send_msg_count, 1);
	if (existing_count == 1) {
		cms_trace (FBE_TRACE_LEVEL_ERROR, "%s, state machine should have only one msg in progres, check logic\n", __FUNCTION__);
		return NULL;
	}else{
		return fbe_cms_cmi_info.state_machine_send_buffer;
	}
}

/*get a pice of memory that contains a physically contig. memory with the bits of this message type,
we assume the caller of this function will also send and return the memory on the same core
and that it's core affined*/
fbe_cms_cmi_memory_msg_t * fbe_cms_cmi_get_memory_msg()
{
	fbe_atomic_t				existing_count;
	
	existing_count = fbe_atomic_exchange(&fbe_cms_cmi_info.memoy_cmi_send_msg_count, 1);
	if (existing_count == 1) {
		cms_trace (FBE_TRACE_LEVEL_ERROR, "%s, memory should have only one msg in progres, check logic\n", __FUNCTION__);
		return NULL;
	}else{
		return fbe_cms_cmi_info.memory_msg_send_buffer;
	}
     
}

/*the user is using this interface to return the CMI messsage memory once he is done,
we expect him to use the same core he took it from*/
void fbe_cms_cmi_return_cluster_msg(fbe_cms_cmi_cluster_msg_t *cms_clsuter_cmi_msg)
{
    
    fbe_cms_cluster_buffer_t *	buffer_ptr = NULL;
	
	buffer_ptr = cluster_msg_to_cluster_buffer((fbe_cms_cmi_cluster_msg_t *)cms_clsuter_cmi_msg);
	fbe_multicore_queue_lock(&fbe_cms_cmi_info.cluster_buffer_queue_head, cms_clsuter_cmi_msg->common_msg_data.cpu_sent_on);

	fbe_queue_remove(&buffer_ptr->q_element);/*takes it off the outstanding queue*/

	fbe_multicore_queue_push(&fbe_cms_cmi_info.outstanding_cluster_queue_head,
							 &buffer_ptr->q_element,
							 cms_clsuter_cmi_msg->common_msg_data.cpu_sent_on);/*move to regular queue*/

	fbe_multicore_queue_unlock(&fbe_cms_cmi_info.cluster_buffer_queue_head, cms_clsuter_cmi_msg->common_msg_data.cpu_sent_on);
}

/*the user is using this interface to return the CMI messsage memory once he is done,
we expect him to use the same core he took it from*/
void fbe_cms_cmi_return_memory_msg(fbe_cms_cmi_memory_msg_t *cms_memory_cmi_msg)
{
	fbe_atomic_t				existing_count;
    
	/*when we return this message sinc we have one to send and one to receive, we don't know what we return so we'll compare*/
	if (cms_memory_cmi_msg == fbe_cms_cmi_info.memory_msg_rcv_buffer) {
		existing_count = fbe_atomic_exchange(&fbe_cms_cmi_info.memoy_cmi_rcv_msg_count, 0);
		if (existing_count == 0) {
			cms_trace (FBE_TRACE_LEVEL_ERROR, "%s, can't return w/o taking\n", __FUNCTION__);
		}

		return;
	}

	if (cms_memory_cmi_msg == fbe_cms_cmi_info.memory_msg_send_buffer) {
		existing_count = fbe_atomic_exchange(&fbe_cms_cmi_info.memoy_cmi_send_msg_count, 0);
		if (existing_count == 0) {
			cms_trace (FBE_TRACE_LEVEL_ERROR, "%s, can't return w/o taking\n", __FUNCTION__);
		}

		return;
	}

	
	cms_trace (FBE_TRACE_LEVEL_ERROR, "%s, trying to return unrecognized message\n", __FUNCTION__);
}

/*the user is using this interface to return the CMI messsage memory once he is done,
we expect him to use the same core he took it from*/
void fbe_cms_cmi_return_sm_msg(fbe_cms_cmi_state_machine_msg_t *cms_sm_cmi_msg)
{
	fbe_atomic_t				existing_count;
    
	/*when we return this message sinc we have one to send and one to receive, we don't know what we return so we'll compare*/
	if (cms_sm_cmi_msg == fbe_cms_cmi_info.state_machine_send_buffer) {
		existing_count = fbe_atomic_exchange(&fbe_cms_cmi_info.state_machine_cmi_send_msg_count, 0);
		if (existing_count == 0) {
			cms_trace (FBE_TRACE_LEVEL_ERROR, "%s, can't return w/o taking\n", __FUNCTION__);
		}

		return;
	}

	if (cms_sm_cmi_msg == fbe_cms_cmi_info.state_machine_rcv_buffer) {
		existing_count = fbe_atomic_exchange(&fbe_cms_cmi_info.state_machine_cmi_rcv_msg_count, 0);
		if (existing_count == 0) {
			cms_trace (FBE_TRACE_LEVEL_ERROR, "%s, can't return w/o taking\n", __FUNCTION__);
		}

		return;
	}

	
	cms_trace (FBE_TRACE_LEVEL_ERROR, "%s, trying to return unrecognized message\n", __FUNCTION__);
    
}

void fbe_cms_cmi_send_sm_message(fbe_cms_cmi_state_machine_msg_t * cms_sm_msg, void *context)
{
	fbe_cpu_id_t 					current_cpu_id = fbe_get_cpu_id();
	fbe_cms_cmi_msg_common_t *		cms_cmi_msg  = (fbe_cms_cmi_msg_common_t *)cms_sm_msg;

    /*we send to the right conduit based on the message type*/

	cms_cmi_msg->cms_cmi_msg_type = FBE_CMS_CMI_STATE_MACHINE_MESSAGE;/*make sure the user did not forget to set it*/
	cms_cmi_msg->cpu_sent_on = current_cpu_id;/*set for completion, whoever calls us with this interface is core affined(better be)*/

    fbe_cmi_send_message(FBE_CMI_CLIENT_ID_CMS_STATE_MACHINE, 
						 sizeof(fbe_cms_cmi_state_machine_msg_t),
						 (fbe_cmi_message_t)cms_sm_msg,
						 (fbe_cmi_event_callback_context_t)context);

    return;
}

void fbe_cms_cmi_send_cluster_message(fbe_cms_cmi_cluster_msg_t * cms_cluster_msg, void *context)
{
	fbe_cpu_id_t 					current_cpu_id = fbe_get_cpu_id();
	fbe_cms_cmi_msg_common_t *		cms_cmi_msg  = (fbe_cms_cmi_msg_common_t *)cms_cluster_msg;

    /*we send to the right conduit based on the message type*/

	cms_cmi_msg->cms_cmi_msg_type = FBE_CMS_CMI_CLUSTER_MESSAGE;/*make sure the user did not forget to set it*/
	cms_cmi_msg->cpu_sent_on = current_cpu_id;/*set for completion, whoever calls us with this interface is core affined(better be)*/

    fbe_cmi_send_message(FBE_CMI_CLIENT_ID_CMS_TAGS, 
						 sizeof(fbe_cms_cmi_cluster_msg_t),
						 (fbe_cmi_message_t)cms_cluster_msg,
						 (fbe_cmi_event_callback_context_t)context);

    return;
}

void fbe_cms_cmi_send_memory_message(fbe_cms_cmi_memory_msg_t * cms_memory_msg, void *context)
{
	fbe_cpu_id_t 					current_cpu_id = fbe_get_cpu_id();
	fbe_cms_cmi_msg_common_t *		cms_cmi_msg  = (fbe_cms_cmi_msg_common_t *)cms_memory_msg;

    /*we send to the right conduit based on the message type*/

	cms_cmi_msg->cms_cmi_msg_type = FBE_CMS_CMI_MEMORY_MESSAGE;/*make sure the user did not forget to set it*/
	cms_cmi_msg->cpu_sent_on = current_cpu_id;/*set for completion, whoever calls us with this interface is core affined(better be)*/

    fbe_cmi_send_message(FBE_CMI_CLIENT_ID_CMS_TAGS, 
						 sizeof(fbe_cms_cmi_memory_msg_t),
						 (fbe_cmi_message_t)cms_memory_msg,
						 (fbe_cmi_event_callback_context_t)context);

    return;
}

static fbe_cms_cmi_msg_common_t * fbe_cms_cmi_copy_received_msg(fbe_cms_cmi_msg_common_t * user_message)
{
    fbe_atomic_t					existing_count;
	fbe_cms_cmi_cluster_msg_t *		cluster_msg = NULL;
	fbe_cms_cmi_msg_common_t* 		new_msg = NULL;
    
    /*get the memory for a message, and copy the memory*/
	switch (user_message->cms_cmi_msg_type) {
	case FBE_CMS_CMI_CLUSTER_MESSAGE:
		cluster_msg = fbe_cms_cmi_get_cluster_msg();
        if (cluster_msg == NULL) {
			/*FIXME, we may want to try and allocate on the fly here and then delete it when we are done*/
			cms_trace (FBE_TRACE_LEVEL_ERROR, "%s: out of msg memory\n", __FUNCTION__); 
			return NULL;
		}

		new_msg = &cluster_msg->common_msg_data;
		fbe_copy_memory(new_msg, user_message, sizeof(fbe_cms_cmi_cluster_msg_t));
        break;
	case FBE_CMS_CMI_MEMORY_MESSAGE:
		existing_count = fbe_atomic_exchange(&fbe_cms_cmi_info.memoy_cmi_rcv_msg_count, 1);
		if (existing_count == 1) {
			cms_trace (FBE_TRACE_LEVEL_ERROR, "%s, state machine should have only one msg in progres, check logic\n", __FUNCTION__);
			return NULL;
		}else{
			new_msg = (fbe_cms_cmi_msg_common_t *)fbe_cms_cmi_info.memory_msg_rcv_buffer;
			fbe_copy_memory(new_msg, user_message, sizeof(fbe_cms_cmi_memory_msg_t));
		}
		        break;
	case FBE_CMS_CMI_STATE_MACHINE_MESSAGE:
		existing_count = fbe_atomic_exchange(&fbe_cms_cmi_info.state_machine_cmi_rcv_msg_count, 1);
		if (existing_count == 1) {
			cms_trace (FBE_TRACE_LEVEL_ERROR, "%s, state machine should have only one msg in progres, check logic\n", __FUNCTION__);
			return NULL;
		}else{
			new_msg = (fbe_cms_cmi_msg_common_t *)fbe_cms_cmi_info.state_machine_rcv_buffer;
			fbe_copy_memory(new_msg, user_message, sizeof(fbe_cms_cmi_state_machine_msg_t));
		}
        break;
	default:
        cms_trace (FBE_TRACE_LEVEL_ERROR, "%s illegal msg type:%d\n", __FUNCTION__, user_message->cms_cmi_msg_type);
        new_msg = NULL;
	}

    return new_msg;

}

/*memory casting to get the buffer that holds the memory for this message*/
static __forceinline fbe_cms_cluster_buffer_t * cluster_msg_to_cluster_buffer(fbe_cms_cmi_cluster_msg_t * cluster_msg)
{
    fbe_cms_cluster_buffer_t * buffer;
    buffer = (fbe_cms_cluster_buffer_t  *)((fbe_u8_t *)cluster_msg - (fbe_u8_t *)(&((fbe_cms_cluster_buffer_t  *)0)->cmi_msg));
    return buffer;
}

static fbe_status_t fbe_cms_cmi_process_received_message(fbe_cms_cmi_msg_common_t *cms_cmi_msg)
{

	 /*and call the completion based on the message type*/
	switch (cms_cmi_msg->cms_cmi_msg_type) {
	case FBE_CMS_CMI_CLUSTER_MESSAGE:
        fbe_cms_process_received_cluster_cmi_message((fbe_cms_cmi_cluster_msg_t *)cms_cmi_msg);
        break;
	case FBE_CMS_CMI_MEMORY_MESSAGE:
        fbe_cms_process_received_memory_cmi_message((fbe_cms_cmi_memory_msg_t *)cms_cmi_msg);
        break;
	case FBE_CMS_CMI_STATE_MACHINE_MESSAGE:
        fbe_cms_process_received_state_machine_cmi_message((fbe_cms_cmi_state_machine_msg_t *)cms_cmi_msg);
        break;
	default:
        cms_trace (FBE_TRACE_LEVEL_ERROR, "%s illegal msg type:%d\n", __FUNCTION__, cms_cmi_msg->cms_cmi_msg_type);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    return FBE_STATUS_OK;

}

static void fbe_cms_cmi_process_peer_reply(fbe_cms_cmi_msg_common_t *cms_cmi_msg)
{
    /*and call the completion based on the message type*/
	switch (cms_cmi_msg->cms_cmi_msg_type) {
	case FBE_CMS_CMI_CLUSTER_MESSAGE:
		{
			fbe_cms_cmi_cluster_msg_t *	cluster_msg = (fbe_cms_cmi_cluster_msg_t *)cms_cmi_msg;
			if (cluster_msg->completion_callback != NULL) {
				cluster_msg->completion_callback(cluster_msg, cms_cmi_msg->event_type, cms_cmi_msg->callback_context);
			}else{
				cms_trace (FBE_TRACE_LEVEL_ERROR, "%s no completion callback on FBE_CMS_CMI_CLUSTER_MESSAGE\n", __FUNCTION__);
			}
		}
        break;
	case FBE_CMS_CMI_MEMORY_MESSAGE:
		{
			fbe_cms_cmi_memory_msg_t *	memory_msg = (fbe_cms_cmi_memory_msg_t *)cms_cmi_msg;
			if (memory_msg->completion_callback != NULL) {
				memory_msg->completion_callback(memory_msg, cms_cmi_msg->event_type, cms_cmi_msg->callback_context);
			}else{
				cms_trace (FBE_TRACE_LEVEL_ERROR, "%s no completion callback on FBE_CMS_CMI_MEMORY_MESSAGE\n", __FUNCTION__);
			}
		}
        break;
	case FBE_CMS_CMI_STATE_MACHINE_MESSAGE:
		{
			fbe_cms_cmi_state_machine_msg_t *	sm_msg = (fbe_cms_cmi_state_machine_msg_t *)cms_cmi_msg;
			if (sm_msg->completion_callback != NULL) {
				sm_msg->completion_callback(sm_msg, cms_cmi_msg->event_type, cms_cmi_msg->callback_context);
			}else{
				cms_trace (FBE_TRACE_LEVEL_ERROR, "%s no completion callback on FBE_CMS_CMI_STATE_MACHINE_MESSAGE\n", __FUNCTION__);
			}
		}
        break;
	default:
        cms_trace (FBE_TRACE_LEVEL_ERROR, "%s illegal msg type:%d\n", __FUNCTION__, cms_cmi_msg->cms_cmi_msg_type);
		fbe_cms_cmi_return_msg_memory(cms_cmi_msg);
		return ;
	}

    return ;
}

static void fbe_cms_cmi_return_msg_memory(fbe_cms_cmi_msg_common_t *cms_cmi_msg)
{
	switch (cms_cmi_msg->cms_cmi_msg_type) {
	case FBE_CMS_CMI_CLUSTER_MESSAGE:
		fbe_cms_cmi_return_cluster_msg((fbe_cms_cmi_cluster_msg_t *)cms_cmi_msg);
		break;
	case FBE_CMS_CMI_MEMORY_MESSAGE:
		fbe_cms_cmi_return_memory_msg((fbe_cms_cmi_memory_msg_t *)cms_cmi_msg);
		break;
	case FBE_CMS_CMI_STATE_MACHINE_MESSAGE:
		fbe_cms_cmi_return_sm_msg((fbe_cms_cmi_state_machine_msg_t *)cms_cmi_msg);
		break;
	default:
		cms_trace (FBE_TRACE_LEVEL_ERROR, "%s illegal msg type:%d, memory leaked\n", __FUNCTION__, cms_cmi_msg->cms_cmi_msg_type);
	}
}

