#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_metadata_private.h"
#include "fbe_topology.h"


#define METADATA_CANCEL_THREAD_MAX_QUEUE_DEPTH 0x7FFFFFFF

typedef enum metadata_cancel_thread_flag_e{
    METADATA_CANCEL_THREAD_FLAG_RUN,
    METADATA_CANCEL_THREAD_FLAG_STOP,
    METADATA_CANCEL_THREAD_FLAG_DONE
}
metadata_cancel_thread_flag_t;

static fbe_semaphore_t metadata_cancel_thread_semaphore;
static fbe_thread_t	metadata_cancel_thread_handle;
static metadata_cancel_thread_flag_t metadata_cancel_thread_flag;
static void metadata_cancel_thread_func(void * context);

fbe_status_t fbe_metadata_cancel_thread_init(void)
{
    metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s entry\n", __FUNCTION__);

	fbe_semaphore_init(&metadata_cancel_thread_semaphore, 0, METADATA_CANCEL_THREAD_MAX_QUEUE_DEPTH);

    /* Start thread */
    metadata_cancel_thread_flag = METADATA_CANCEL_THREAD_FLAG_RUN;
    fbe_thread_init(&metadata_cancel_thread_handle, "fbe_meta_cancel", metadata_cancel_thread_func, (void*)(fbe_ptrhld_t)0);
    return FBE_STATUS_OK;
}
fbe_status_t fbe_metadata_cancel_thread_destroy(void)
{
    metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s entry\n", __FUNCTION__);

	/* Stop thread */
    metadata_cancel_thread_flag = METADATA_CANCEL_THREAD_FLAG_STOP; 

    /* notify thread
     */
    fbe_semaphore_release(&metadata_cancel_thread_semaphore, 0, 1, FALSE); 
    fbe_thread_wait(&metadata_cancel_thread_handle);
    fbe_thread_destroy(&metadata_cancel_thread_handle);

	fbe_semaphore_destroy(&metadata_cancel_thread_semaphore);

    return FBE_STATUS_OK;
}
static void metadata_cancel_thread_func(void * context)
{
	fbe_u64_t thread_number;
	fbe_status_t status;

	thread_number = (fbe_u64_t)context;	
	FBE_UNREFERENCED_PARAMETER(context);	

    metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s %llu entry \n", __FUNCTION__,
		   (unsigned long long)thread_number);

    while(1)    
	{
		//status = fbe_semaphore_wait_ms(&metadata_cancel_thread_semaphore, 3000);
		status = fbe_semaphore_wait(&metadata_cancel_thread_semaphore, 0);
		if(metadata_cancel_thread_flag == METADATA_CANCEL_THREAD_FLAG_RUN) {

            /* Scan all metadata elements.  If any have aborted requests, 
             * scan the queues of that metadata element for aborted requests.
             */
			
			if(status == FBE_STATUS_TIMEOUT){
				fbe_metadata_scan_for_cancelled_packets(FBE_TRUE);
			} else 			
			{
				fbe_metadata_scan_for_cancelled_packets(FBE_FALSE);
			}
		} else {
			break;
		}
    }

    metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s %llu done\n", __FUNCTION__,
		   (unsigned long long)thread_number);

    metadata_cancel_thread_flag = METADATA_CANCEL_THREAD_FLAG_DONE;
	fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

fbe_status_t fbe_metadata_cancel_thread_notify(void)
{
    fbe_semaphore_release(&metadata_cancel_thread_semaphore, 0, 1, FALSE); 
	return FBE_STATUS_OK;
}

fbe_status_t fbe_metadata_cancel_function(fbe_packet_completion_context_t context)
{
	fbe_payload_ex_t * sep_payload = NULL;
	fbe_payload_stripe_lock_operation_t * stripe_lock_operation = NULL;
	fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t *metadata_element = NULL;
	fbe_packet_t * packet = (fbe_packet_t *)context;
    
	sep_payload = fbe_transport_get_payload_ex(packet);

    if (sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
    {
        stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(sep_payload);
        if (stripe_lock_operation != NULL)
        {
            metadata_element = (fbe_metadata_element_t *)(fbe_ptrhld_t)stripe_lock_operation->cmi_stripe_lock.header.metadata_element_sender;
        }
        else
        {
            metadata_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s packet %p cancelled NULL SL operation payload operation type: %d\n", 
                       __FUNCTION__, packet, sep_payload->current_operation->payload_opcode);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if (sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION)
    {
        metadata_operation = fbe_payload_ex_get_metadata_operation(sep_payload);
        if (metadata_operation != NULL)
        {
            metadata_element = metadata_operation->metadata_element;
        }
        else
        {
            metadata_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s packet %p cancelled NULL metadata operation payload operation type: %d\n", 
                           __FUNCTION__, packet, sep_payload->current_operation->payload_opcode);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        metadata_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s packet %p cancelled has payload operation type: %d\n", 
                           __FUNCTION__, packet, sep_payload->current_operation->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	fbe_spinlock_lock(&metadata_element->metadata_element_lock);
    metadata_element->attributes |= FBE_METADATA_ELEMENT_ATTRIBUTES_ABORTED_REQUESTS;
	fbe_spinlock_unlock(&metadata_element->metadata_element_lock);
    fbe_metadata_cancel_thread_notify();
	return FBE_STATUS_OK;
}
