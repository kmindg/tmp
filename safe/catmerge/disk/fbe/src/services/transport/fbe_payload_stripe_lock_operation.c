#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_ex.h"
#include "fbe_metadata.h"

fbe_status_t 
fbe_payload_stripe_lock_build_read_lock(fbe_payload_stripe_lock_operation_t * stripe_lock_operation,
                                        fbe_metadata_element_t * metadata_element,
                                        fbe_lba_t	stripe_number,
                                        fbe_block_count_t	stripe_count)
{
	stripe_lock_operation->opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK;		
	stripe_lock_operation->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_INITIALIZED;

	stripe_lock_operation->cmi_stripe_lock.header.metadata_element_sender = CSX_CAST_PTR_TO_PTRMAX(metadata_element);	

	stripe_lock_operation->stripe.first = stripe_number;
	stripe_lock_operation->stripe.last = stripe_number + stripe_count - 1;
	stripe_lock_operation->flags = 0;

	fbe_queue_element_init(&stripe_lock_operation->queue_element);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_stripe_lock_build_read_unlock(fbe_payload_stripe_lock_operation_t * stripe_lock_operation)
{

	if(stripe_lock_operation->opcode != FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid opcode %d", __FUNCTION__, stripe_lock_operation->opcode);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if(stripe_lock_operation->status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid status %d", __FUNCTION__, stripe_lock_operation->status);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	stripe_lock_operation->opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_UNLOCK;		
	//stripe_lock_operation->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_INITIALIZED;

    /* AR 497201: PSM timeout due to an IRP being held in SEP.  SEP has an outstanding packet that still has the stripe lock
     * being held during read unlock due to the SYNC mode is not supported at this point for Unlock case. Discussed this with
     * Peter and commented out the below.  Whoever needs it will have to set it. 
     *  
     *  stripe_lock_operation->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_SYNC_MODE;
     */

	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_payload_stripe_lock_build_write_lock(fbe_payload_stripe_lock_operation_t * stripe_lock_operation,
                                         fbe_metadata_element_t * metadata_element,
                                         fbe_lba_t	stripe_number,
                                         fbe_block_count_t	stripe_count)
{
	stripe_lock_operation->opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK;		
	stripe_lock_operation->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_INITIALIZED;

	stripe_lock_operation->cmi_stripe_lock.header.metadata_element_sender = CSX_CAST_PTR_TO_PTRMAX(metadata_element);	

	stripe_lock_operation->stripe.first = stripe_number;
	stripe_lock_operation->stripe.last = stripe_number + stripe_count - 1;
	stripe_lock_operation->flags = 0;

	fbe_queue_element_init(&stripe_lock_operation->queue_element);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_stripe_lock_build_write_unlock(fbe_payload_stripe_lock_operation_t * stripe_lock_operation)
{
	if(stripe_lock_operation->opcode != FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid opcode %d", __FUNCTION__, stripe_lock_operation->opcode);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if(stripe_lock_operation->status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid status %d", __FUNCTION__, stripe_lock_operation->status);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	stripe_lock_operation->opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK;		
	//stripe_lock_operation->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_INITIALIZED;

    /* AR 497201: PSM timeout due to an IRP being held in SEP.  SEP has an outstanding packet that still has the stripe lock
     * being held during read unlock due to the SYNC mode is not supported at this point for Unlock case. Discussed this with
     * Peter and commented out the below.  Whoever needs it will have to set it. 
     *  
     *  stripe_lock_operation->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_SYNC_MODE;
     */

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_stripe_lock_build_start(fbe_payload_stripe_lock_operation_t * stripe_lock_operation,
									fbe_metadata_element_t * metadata_element)
{
	stripe_lock_operation->opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_START;		
	stripe_lock_operation->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_INITIALIZED;

	stripe_lock_operation->cmi_stripe_lock.header.metadata_element_sender = CSX_CAST_PTR_TO_PTRMAX(metadata_element);	
	stripe_lock_operation->flags = 0;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_stripe_lock_build_stop(fbe_payload_stripe_lock_operation_t * stripe_lock_operation,
									fbe_metadata_element_t * metadata_element)
{
	stripe_lock_operation->opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_STOP;		
	stripe_lock_operation->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_INITIALIZED;

	stripe_lock_operation->cmi_stripe_lock.header.metadata_element_sender = CSX_CAST_PTR_TO_PTRMAX(metadata_element);	
	stripe_lock_operation->flags = 0;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_stripe_lock_set_sync_mode(fbe_payload_stripe_lock_operation_t * stripe_lock_operation)
{
	stripe_lock_operation->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_SYNC_MODE;
	return FBE_STATUS_OK;
}
