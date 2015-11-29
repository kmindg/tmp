/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_state_machine_queue_memory.c
***************************************************************************
*
* @brief
*  This file contains CMS service memory for events and notification queues
*  
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/

#include "fbe_cms_private.h"
#include "fbe_cms_environment.h"
#include "fbe_cms_state_machine.h"
#include "fbe_cms_state_machine_queue_memory.h"

#define FBE_CMS_EVENT_MEMORY_ELEMENTS 200 /*200 concurrent is plenty*/

static fbe_queue_head_t		 			fbe_cms_event_queue_memory_outstanding_queue_head;
static fbe_queue_head_t 				fbe_cms_event_queue_memory_queue_head;
static fbe_spinlock_t					fbe_cms_event_queue_memory_lock;

/*local functions*/
static __forceinline fbe_cms_sm_event_memory_t * fbe_cms_sm_event_to_msg_memory(fbe_cms_sm_event_info_t * user_message);

/*****************************************************************************************************************************************
*/

/*allocate memory for arriving state machine events*/
fbe_status_t fbe_cms_sm_allocate_event_queue_memory(void)
{
    fbe_cms_sm_event_memory_t *	event_memory_element;
    fbe_u32_t					io_strcut_count = 0;
    
    fbe_queue_init (&fbe_cms_event_queue_memory_outstanding_queue_head);
	fbe_queue_init (&fbe_cms_event_queue_memory_queue_head);
	fbe_spinlock_init (&fbe_cms_event_queue_memory_lock);
    
	for (io_strcut_count = 0; io_strcut_count < FBE_CMS_EVENT_MEMORY_ELEMENTS; io_strcut_count++) {
		event_memory_element = (fbe_cms_sm_event_memory_t *)fbe_memory_ex_allocate(sizeof(fbe_cms_sm_event_memory_t));
		if (event_memory_element == NULL) {
			cms_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s, can't allocate memory for IO\n", __FUNCTION__);
		}else{
			fbe_queue_element_init(&event_memory_element->event_memory_queue_element);
			fbe_queue_push(&fbe_cms_event_queue_memory_queue_head, &event_memory_element->event_memory_queue_element);
		}
	}
	

    cms_trace (FBE_TRACE_LEVEL_INFO, "%s alloc %d events\n", __FUNCTION__, FBE_CMS_EVENT_MEMORY_ELEMENTS);
    
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_destroy_event_queue_memory(void)
{
	fbe_u32_t					io_strcut_count = 0;
    fbe_cms_sm_event_memory_t *	event_memory_element = NULL;
	fbe_u32_t					delay_count = 0;
    
    /*do we have events in flight ?*/
	fbe_spinlock_lock(&fbe_cms_event_queue_memory_lock);
	while (!fbe_queue_is_empty(&fbe_cms_event_queue_memory_outstanding_queue_head) && delay_count < 100) {
		fbe_spinlock_unlock(&fbe_cms_event_queue_memory_lock);
		if (delay_count == 0) {
			cms_trace(FBE_TRACE_LEVEL_WARNING, "%s %d outstanding Events when destroying !!\n", __FUNCTION__);
		}
		fbe_thread_delay(100);
		delay_count++;
		fbe_spinlock_lock(&fbe_cms_event_queue_memory_lock);
	}

	if (delay_count == 100) {
		cms_trace(FBE_TRACE_LEVEL_WARNING, "%, outstanding Events after 10sec, we will leak memory !!\n", __FUNCTION__);
	}

	while (!fbe_queue_is_empty(&fbe_cms_event_queue_memory_queue_head)) {
		event_memory_element = (fbe_cms_sm_event_memory_t *)fbe_queue_pop(&fbe_cms_event_queue_memory_queue_head);
		fbe_memory_ex_release((void *)event_memory_element);
	}
	

	fbe_queue_destroy (&fbe_cms_event_queue_memory_outstanding_queue_head);
	fbe_queue_destroy (&fbe_cms_event_queue_memory_queue_head);
	fbe_spinlock_destroy(&fbe_cms_event_queue_memory_lock);

	return FBE_STATUS_OK;
}

/*Get event memory*/
fbe_cms_sm_event_info_t * fbe_cms_sm_get_event_memory(void)
{
    
    fbe_cms_sm_event_info_t *		msg_memory = NULL;
	fbe_cms_sm_event_memory_t *		event_memory_element = NULL;
    
    fbe_spinlock_lock(&fbe_cms_event_queue_memory_lock);

    if (!fbe_queue_is_empty(&fbe_cms_event_queue_memory_queue_head)) {
        event_memory_element = (fbe_cms_sm_event_memory_t *)fbe_queue_pop(&fbe_cms_event_queue_memory_queue_head);
        msg_memory = &event_memory_element->event_memory;
		fbe_queue_push(&fbe_cms_event_queue_memory_outstanding_queue_head,&event_memory_element->event_memory_queue_element);/*move to outstanding queue*/
    }else{
        fbe_spinlock_unlock(&fbe_cms_event_queue_memory_lock);
		/*not good and should nevener happen since we might be at dpc and we can't wait*/
		cms_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s out of event memory\n", __FUNCTION__);
        return NULL;
    }
    fbe_spinlock_unlock(&fbe_cms_event_queue_memory_lock);

	return msg_memory;
}

/*return used event memory*/
void fbe_cms_sm_return_event_memory(fbe_cms_sm_event_info_t *retrunred_memory)
{
    
    fbe_cms_sm_event_memory_t * 	memory_ptr = NULL;

	/*cast back to the actual memory we use*/
	memory_ptr = fbe_cms_sm_event_to_msg_memory(retrunred_memory);

    /*remove from progress queue*/
	fbe_spinlock_lock(&fbe_cms_event_queue_memory_lock);
	fbe_queue_remove(&memory_ptr->event_memory_queue_element);  
    
	/*push back to be used by others*/
    fbe_queue_push_front(&fbe_cms_event_queue_memory_queue_head, &memory_ptr->event_memory_queue_element);
	fbe_spinlock_unlock(&fbe_cms_event_queue_memory_lock);

    return ;
}

/*get the memory of the message itself and extract from it the structure that actually contains it and the rest of what we need*/
static __forceinline fbe_cms_sm_event_memory_t * fbe_cms_sm_event_to_msg_memory(fbe_cms_sm_event_info_t * used_event)
{
    fbe_cms_sm_event_memory_t *event_memory;
    event_memory = (fbe_cms_sm_event_memory_t *)((fbe_u8_t *)used_event - (fbe_u8_t *)(&((fbe_cms_sm_event_memory_t *)0)->event_memory));
    return event_memory;
}
