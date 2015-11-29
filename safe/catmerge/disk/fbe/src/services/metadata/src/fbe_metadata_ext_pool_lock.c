#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_base_config.h"
#include "fbe_metadata_private.h"
#include "fbe_cmi.h"
#include "fbe_transport_memory.h"
#include "fbe_metadata_cmi.h"
#include "fbe/fbe_extent_pool.h"

//static fbe_u8_t         *   metadata_stripe_lock_blob_memory = NULL; 
static fbe_queue_head_t     ext_pool_lock_blob_queue;
static fbe_spinlock_t       ext_pool_lock_blob_queue_lock;

static fbe_u8_t         *   ext_pool_lock_peer_sl_memory = NULL; 
static fbe_queue_head_t     ext_pool_lock_peer_sl_queue;
static fbe_spinlock_t       ext_pool_lock_peer_sl_queue_lock;

static fbe_u64_t ext_pool_lock_peer_sl_queue_count = 0; /* This will track alloc and release from peer_sl_queue */

/* Forward declaration */
static fbe_status_t ext_pool_lock_lock(fbe_packet_t *  packet);
static fbe_status_t ext_pool_lock_unlock(fbe_packet_t * packet);

static fbe_status_t ext_pool_lock_start(fbe_packet_t *  packet);
static fbe_status_t ext_pool_lock_stop(fbe_packet_t *  packet);


static fbe_status_t fbe_ext_pool_lock_control_entry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_ext_pool_lock_msg(fbe_metadata_element_t * metadata_element, 
												 fbe_metadata_cmi_message_t * metadata_cmi_msg,
												 fbe_u32_t cmi_message_length);

static fbe_status_t fbe_ext_pool_release_msg(fbe_metadata_element_t * metadata_element, 
												 fbe_metadata_cmi_message_t * metadata_cmi_msg,
												 fbe_u32_t cmi_message_length);

static fbe_status_t fbe_ext_pool_aborted_msg(fbe_metadata_element_t * metadata_element, 
												 fbe_metadata_cmi_message_t * metadata_cmi_msg,
												 fbe_u32_t cmi_message_length);


static fbe_status_t fbe_ext_pool_grant_msg(fbe_metadata_element_t * mde, 
												  fbe_metadata_cmi_message_t * msg,
												  fbe_u32_t cmi_message_length);

static __forceinline fbe_bool_t ext_pool_lock_mark_grant_to_peer(fbe_payload_stripe_lock_operation_t * sl);
static __forceinline fbe_bool_t ext_pool_lock_check_peer(fbe_payload_stripe_lock_operation_t * sl);

/* This will send grants to the peer */
static fbe_status_t ext_pool_lock_dispatch_cmi_grant_queue(fbe_queue_head_t * grant_queue);


static fbe_status_t ext_pool_lock_send_request_sl(fbe_payload_stripe_lock_operation_t * sl, fbe_bool_t * is_cmi_required);

static fbe_status_t fbe_ext_pool_send_release_msg(fbe_metadata_element_t * mde, fbe_payload_stripe_lock_operation_t * sl);
static fbe_status_t fbe_ext_pool_send_aborted_msg(fbe_metadata_element_t * mde, fbe_payload_stripe_lock_operation_t * sl);

static fbe_status_t ext_pool_lock_complete_cancelled_queue(fbe_queue_head_t * queue, fbe_queue_head_t * tmp_queue);

static fbe_status_t ext_pool_lock_dispatch_blob(fbe_metadata_stripe_lock_blob_t * blob, 
													   fbe_queue_head_t * cmi_request_queue, 
													   fbe_queue_head_t * grant_queue);

static fbe_status_t ext_pool_lock_dispatch_cmi_request_queue(fbe_queue_head_t * cmi_request_queue);

#define SL_DEBUG_MODE 0

static fbe_ext_pool_lock_slice_entry_t * 
fbe_ext_pool_hash_get_entry(fbe_metadata_stripe_lock_hash_t * hash, fbe_payload_stripe_lock_operation_t * sl);

static fbe_status_t
fbe_ext_pool_lock_element_abort_queue(fbe_metadata_element_t * mde, 
											 fbe_queue_head_t * queue,
											 fbe_queue_head_t * abort_queue,
											 fbe_queue_head_t * tmp_queue);

static fbe_status_t 
fbe_ext_pool_lock_grant_all_peer_locks(fbe_metadata_element_t* mde, fbe_queue_head_t * tmp_queue, fbe_bool_t * is_cmi_required);

static fbe_status_t fbe_stripe_lock_scan_for_grants(fbe_metadata_element_t* mde, fbe_queue_head_t * grant_queue);
static fbe_status_t ext_pool_lock_disable_hash(fbe_metadata_element_t * mde);
static fbe_status_t ext_pool_lock_enable_hash(fbe_metadata_element_t * mde);
static fbe_status_t ext_pool_lock_push_sorted(fbe_queue_head_t * queue, fbe_payload_stripe_lock_operation_t * sl);
static fbe_status_t ext_pool_lock_reschedule_blob(fbe_metadata_element_t * mde, fbe_bool_t is_cmi_required, fbe_bool_t is_hash_enable_required);

typedef enum ext_pool_lock_thread_flag_e{
    METADATA_STRIPE_LOCK_THREAD_RUN,
    METADATA_STRIPE_LOCK_THREAD_STOP,
    METADATA_STRIPE_LOCK_THREAD_DONE
}ext_pool_lock_thread_flag_t;

static fbe_thread_t                         ext_pool_lock_thread_handle;
static ext_pool_lock_thread_flag_t          ext_pool_lock_thread_flag;


static fbe_rendezvous_event_t				ext_pool_lock_event;

typedef struct ext_pool_lock_callback_context_s{
	void * param_1;
	void * param_2;
	void * param_3;
	void * param_4;
}ext_pool_lock_callback_context_t;

static __forceinline void 
ext_pool_lock_hash_table_entry(fbe_ext_pool_lock_slice_entry_t * entry)
{
#ifndef __SAFE__
	csx_p_spin_pointer_lock(&entry->lock);
#endif
}

static __forceinline void 
ext_pool_unlock_hash_table_entry(fbe_ext_pool_lock_slice_entry_t * entry)
{
#ifndef __SAFE__
	csx_p_spin_pointer_unlock(&entry->lock);
#endif
}

static __forceinline fbe_status_t 
ext_pool_lock_set_region(fbe_payload_stripe_lock_operation_t * sl, fbe_metadata_stripe_lock_region_t * region)
{
	region->first = sl->stripe.first;
	region->last = sl->stripe.last;
	return FBE_STATUS_OK;
}

static __forceinline fbe_status_t 
ext_pool_lock_set_invalid_region(fbe_metadata_stripe_lock_region_t * region)
{
	region->first = FBE_LBA_INVALID;
	region->last = FBE_LBA_INVALID;
	return FBE_STATUS_OK;
}

void fbe_metadata_ext_pool_lock_get_memory_use(fbe_u32_t *memory_bytes_p)
{
    fbe_u32_t memory_bytes = 0;
    fbe_cpu_id_t cpu_count;

	cpu_count = fbe_get_cpu_count();
    memory_bytes += sizeof(fbe_payload_stripe_lock_operation_t) * METADATA_STRIPE_LOCK_MAX_PEER_SL * cpu_count;
    *memory_bytes_p = memory_bytes;
}

/* This will update the region of sl2 */
static __forceinline fbe_status_t 
ext_pool_lock_update_region(fbe_payload_stripe_lock_operation_t * sl1, fbe_payload_stripe_lock_operation_t * sl2)
{
	fbe_metadata_stripe_lock_region_t * region;
    fbe_metadata_element_t * mde = NULL;
    fbe_metadata_stripe_lock_blob_t * blob = NULL;

    mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl1->cmi_stripe_lock.header.metadata_element_sender;
	blob = mde->stripe_lock_blob;

	if(sl1->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ){
		region = &sl2->cmi_stripe_lock.read_region;
	} else { /* write lock */
		region = &sl2->cmi_stripe_lock.write_region;
	}

	if(region->first == FBE_LBA_INVALID){
		region->first = sl1->stripe.first;
		region->last = sl1->stripe.last;
		return FBE_STATUS_OK;
	}

	if(sl1->stripe.first < region->first){
		region->first = sl1->stripe.first;
	}

	if(sl1->stripe.last > region->last){
		region->last = sl1->stripe.last;		
	}
	
#ifdef EXT_POOL_LOCK
	if(sl2->stripe.first < blob->private_slot) { /* Not a paged I/O */
		if(region->last > blob->private_slot){ /* It is a bug */
			metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
				FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"SL: User/Private overlap \n");
		}
	}
#endif
	return FBE_STATUS_OK;
}

/* This will update the region of sl2 */
static __forceinline fbe_status_t 
ext_pool_lock_update_peer_request_regions(fbe_payload_stripe_lock_operation_t * sl1, fbe_payload_stripe_lock_operation_t * sl2)
{
	if(sl2->cmi_stripe_lock.read_region.first == FBE_LBA_INVALID &&
			sl1->cmi_stripe_lock.read_region.first != FBE_LBA_INVALID){
		sl2->cmi_stripe_lock.read_region.first = sl1->cmi_stripe_lock.read_region.first;
		sl2->cmi_stripe_lock.read_region.last = sl1->cmi_stripe_lock.read_region.last;
	}

	if(sl2->cmi_stripe_lock.write_region.first == FBE_LBA_INVALID &&
			sl1->cmi_stripe_lock.write_region.first != FBE_LBA_INVALID){
		sl2->cmi_stripe_lock.write_region.first = sl1->cmi_stripe_lock.write_region.first;
		sl2->cmi_stripe_lock.write_region.last = sl1->cmi_stripe_lock.write_region.last;
	}

	if(sl1->cmi_stripe_lock.read_region.first != FBE_LBA_INVALID){
		if(sl1->cmi_stripe_lock.read_region.first < sl2->cmi_stripe_lock.read_region.first){
			sl2->cmi_stripe_lock.read_region.first = sl1->cmi_stripe_lock.read_region.first;
		}

		if(sl1->cmi_stripe_lock.read_region.last > sl2->cmi_stripe_lock.read_region.last){
			sl2->cmi_stripe_lock.read_region.last = sl1->cmi_stripe_lock.read_region.last;		
		}
	}

	if(sl1->cmi_stripe_lock.write_region.first != FBE_LBA_INVALID){
		if(sl1->cmi_stripe_lock.write_region.first < sl2->cmi_stripe_lock.write_region.first){
			sl2->cmi_stripe_lock.write_region.first = sl1->cmi_stripe_lock.write_region.first;
		}

		if(sl1->cmi_stripe_lock.write_region.last > sl2->cmi_stripe_lock.write_region.last){
			sl2->cmi_stripe_lock.write_region.last = sl1->cmi_stripe_lock.write_region.last;		
		}
	}
	return FBE_STATUS_OK;
}



static __forceinline fbe_bool_t 
ext_pool_lock_is_overlap(fbe_metadata_stripe_lock_region_t * r1, fbe_metadata_stripe_lock_region_t * r2)
{
	if((r1->first  == FBE_LBA_INVALID) || (r1->first  == FBE_LBA_INVALID)){
		return FBE_FALSE; /* No overlap */
	}

	if(r1->first > r2->last){
		return FBE_FALSE; /* No overlap */
	}
	if(r2->first > r1->last){
		return FBE_FALSE; /* No overlap */
	}

	return FBE_TRUE;
}

static __forceinline fbe_bool_t 
ext_pool_lock_is_np_lock(fbe_payload_stripe_lock_operation_t * sl)
{
    return FBE_FALSE;
}

static __forceinline fbe_bool_t 
ext_pool_lock_is_paged_lock(fbe_payload_stripe_lock_operation_t * sl)
{
	return FBE_FALSE;
}

static __forceinline fbe_bool_t
ext_pool_lock_is_monitor_op_abort_needed(fbe_metadata_element_t * mde, fbe_payload_stripe_lock_operation_t * sl, fbe_bool_t abort_peer)
{
    /* Abort peer monitor SLs including paged/NP SL */
    if (abort_peer)
    {
        if ((sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) &&
            !(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANT) &&
            (sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_MONITOR_OP)) {
            return FBE_TRUE;
        } 

        return FBE_FALSE;
    }

    /* Abort SL for this SP, when the peer is dead */
    if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST){
        return FBE_FALSE;
    }

    if (!fbe_transport_is_monitor_packet(sl->cmi_stripe_lock.packet, fbe_base_config_metadata_element_to_object_id(mde))){
        return FBE_FALSE; /* It is not a monitor op */
    }

    if (ext_pool_lock_is_np_lock(sl)){
        return FBE_FALSE; /* We do not abort non-paged lock */
    }

    if (ext_pool_lock_is_paged_lock(sl)){
        return FBE_FALSE; /* We do not abort paged lock */
    }

    return FBE_TRUE;
}

/* The order is IMPORTANT */
/* Returns FBE_TRUE if sl1 can be inserted */
static __forceinline fbe_bool_t 
ext_pool_lock_check_sorted(fbe_payload_stripe_lock_operation_t * sl1, 
									fbe_payload_stripe_lock_operation_t * sl2)
{
	fbe_metadata_stripe_lock_region_t * region = NULL;

	if(!(sl1->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST &&
		 !(sl1->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE)))

	{ /* Local stripe lock */
		if(sl1->stripe.first > sl2->stripe.last){
			if((sl2->cmi_stripe_lock.read_region.last == FBE_LBA_INVALID) || (sl1->stripe.first > sl2->cmi_stripe_lock.read_region.last)){
				if((sl2->cmi_stripe_lock.write_region.last == FBE_LBA_INVALID) || (sl1->stripe.first > sl2->cmi_stripe_lock.write_region.last)){
					return FBE_TRUE;
				}
			}
		}
		return FBE_FALSE;
	}	

	else

	{ /* request from the peer was not "shrinked" */
		/* For the peer request we will check the region */
		if(sl1->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ){
			region = &sl1->cmi_stripe_lock.read_region;
		} else { /* write lock */
			region = &sl1->cmi_stripe_lock.write_region;
		}

		if(region->first > sl2->stripe.last){
			if((sl2->cmi_stripe_lock.read_region.last == FBE_LBA_INVALID) || (region->first > sl2->cmi_stripe_lock.read_region.last)){
				if((sl2->cmi_stripe_lock.write_region.last == FBE_LBA_INVALID) || (region->first > sl2->cmi_stripe_lock.write_region.last)){
					return FBE_TRUE;
				}
			}
		}
		return FBE_FALSE;
	} 
	
	return FBE_FALSE;
}

static __forceinline void 
ext_pool_lock_set_read_overlap(fbe_payload_stripe_lock_operation_t * sl1, 
									fbe_payload_stripe_lock_operation_t * sl2,
									fbe_bool_t * read_overlap)
{
	/* Make sure it is sorted */
	if((sl1->stripe.last > sl2->cmi_stripe_lock.read_region.last || sl2->cmi_stripe_lock.read_region.last == FBE_LBA_INVALID )&&
		(sl1->stripe.last > sl2->cmi_stripe_lock.write_region.last || sl2->cmi_stripe_lock.write_region.last == FBE_LBA_INVALID)){

#if 0 
	/* Fix for AR 552812 when we have a "gap" between read and write regions:
        operation_header: 0xfffff88029d725c0 opcode: (write lock) status: 0x3 flags: 0x2
          stripe: first: 0x218cd850 last: 0x218cd851  priv_flags: 0x5         cmi: metadata_element_sender: 0xfffff8801c367d18  
          write_region: first: 0x218cd850 last: 0x218cd851  read_region: first: 0xffffffff last: 0xffffffff  packet: 0xfffff88029d71f40 
          request_sl_ptr: 0x0 grant_sl_ptr: 0xfffff880342dd500 flags: 0x1 
            queue (wait_queue): EMPTY

        operation_header: 0xfffff88029be83c0 opcode: (read lock) status: 0x3 flags: 0x2
          stripe: first: 0x218cd834 last: 0x218cd835  priv_flags: 0x7         cmi: metadata_element_sender: 0xfffff8801c367d18  
          write_region: first: 0xffffffff last: 0xffffffff  read_region: first: 0x218cd834 last: 0x218cd835  packet: 0xfffff88029be7d40 
          request_sl_ptr: 0x0 grant_sl_ptr: 0x0 flags: 0x0 
            queue (wait_queue): EMPTY

        operation_header: 0xfffff88034a3bcb0 opcode: (write lock) status: 0x2 flags: 0x640000
          stripe: first: 0x218cd86c last: 0x218cd86d  priv_flags: 0x5         cmi: metadata_element_sender: 0xfffff8801c367d18  
          write_region: first: 0x218cd86c last: 0x218cd86d  read_region: first: 0x218cd833 last: 0x218cd834  packet: 0xfffff8802927b280 
          request_sl_ptr: 0xfffff8802927b900 grant_sl_ptr: 0xfffff88034a3bcb0 flags: 0x1 
            queue (wait_queue): EMPTY

        operation_header: 0xfffff88029a81a80 opcode: (write lock) status: 0x3 flags: 0x2
          stripe: first: 0x218cd850 last: 0x218cd851  priv_flags: 0x5         cmi: metadata_element_sender: 0xfffff8801c367d18  
          write_region: first: 0x218cd850 last: 0x218cd851  read_region: first: 0xffffffff last: 0xffffffff  packet: 0xfffff88029a81400 
          request_sl_ptr: 0x0 grant_sl_ptr: 0xfffff880342dd0e0 flags: 0x1 
            queue (wait_queue): EMPTY
	*/
			if((sl1->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && (sl2->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST)){
				ext_pool_lock_update_peer_request_regions(sl2, sl1);
			}
#endif

		*read_overlap = FBE_TRUE;
	}
}



/* The order is IMPORTANT */
static fbe_bool_t 
ext_pool_lock_check_overlap(fbe_payload_stripe_lock_operation_t * sl1, 
									fbe_payload_stripe_lock_operation_t * sl2,
									fbe_bool_t * read_overlap,
									fbe_bool_t * reinsert_requiered)
{

	if(sl1->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ){ /* sl1 is a read */
		/* For the peer request check the region first */
		if(sl1->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST){
			if(ext_pool_lock_is_overlap(&sl1->cmi_stripe_lock.read_region, &sl2->cmi_stripe_lock.write_region)){
				/* Peer requests can not overlap */
				if((sl1->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && (sl2->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST)){
					/* Make sure it is sorted */
					ext_pool_lock_set_read_overlap(sl1, sl2, read_overlap);
					return FBE_FALSE; /* There is no collision */
				}

				if(sl1->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE){
					/* We already shrink the region just report overlap */
					return FBE_TRUE;
				}

				/* We need to shrink read region */
				sl1->cmi_stripe_lock.read_region.first = sl1->stripe.first;
				sl1->cmi_stripe_lock.read_region.last = sl1->stripe.last;
				sl1->cmi_stripe_lock.flags |= FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE;

				*reinsert_requiered = FBE_TRUE; /* This SL will be reevaluated */

				if(ext_pool_lock_is_overlap(&sl1->cmi_stripe_lock.read_region, &sl2->cmi_stripe_lock.write_region)){
					/* Peer requests can not overlap */
					if((sl1->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && (sl2->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST)){
						/* Make sure it is sorted */
						ext_pool_lock_set_read_overlap(sl1, sl2, read_overlap);
						return FBE_FALSE; /* There is no collision */
					}


					return FBE_TRUE;
				}
			}

			/* Check for read overlap */
			if(ext_pool_lock_is_overlap(&sl1->cmi_stripe_lock.read_region, &sl2->cmi_stripe_lock.read_region)){
                if(sl1->cmi_stripe_lock.read_region.last > sl2->cmi_stripe_lock.read_region.last){
				    *read_overlap = FBE_TRUE;
                }
			}

			/* We shrink the region and need to make sure that queue remains sorted */
			if(sl1->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE){
				ext_pool_lock_set_read_overlap(sl1, sl2, read_overlap);
			}

			/* No overlap */
			return FBE_FALSE;
		} /* Done with peer request */

		if(ext_pool_lock_is_overlap(&sl1->stripe, &sl2->cmi_stripe_lock.write_region)){
			return FBE_TRUE;
		}
		if(ext_pool_lock_is_overlap(&sl1->cmi_stripe_lock.read_region, &sl2->cmi_stripe_lock.read_region)){
            if(sl1->cmi_stripe_lock.read_region.last > sl2->cmi_stripe_lock.read_region.last){
			    *read_overlap = FBE_TRUE;
            }
		}
		/* No overlap */
		return FBE_FALSE;
	} else { /* It is a write */
		/* For the peer request check the region first */
		if(sl1->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST){
			if(ext_pool_lock_is_overlap(&sl1->cmi_stripe_lock.write_region, &sl2->cmi_stripe_lock.write_region) ||
				ext_pool_lock_is_overlap(&sl1->cmi_stripe_lock.write_region, &sl2->cmi_stripe_lock.read_region)){

				/* Peer requests can not overlap */
				if((sl1->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && (sl2->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST)){
					/* Make sure it is sorted */
					ext_pool_lock_set_read_overlap(sl1, sl2, read_overlap);
					return FBE_FALSE; /* There is no collision */
				}

				if(sl1->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE){
					/* We already shrink the region just report overlap */
					return FBE_TRUE;
				}

				/* We need to shrink read region */
				sl1->cmi_stripe_lock.write_region.first = sl1->stripe.first;
				sl1->cmi_stripe_lock.write_region.last = sl1->stripe.last;
				sl1->cmi_stripe_lock.flags |= FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE;

				*reinsert_requiered = FBE_TRUE; /* This SL will be reevaluated */

				if(ext_pool_lock_is_overlap(&sl1->cmi_stripe_lock.write_region, &sl2->cmi_stripe_lock.write_region) ||
					ext_pool_lock_is_overlap(&sl1->cmi_stripe_lock.write_region, &sl2->cmi_stripe_lock.read_region)){

					/* Peer requests can not overlap */
					if((sl1->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && (sl2->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST)){
						/* Make sure it is sorted */
						ext_pool_lock_set_read_overlap(sl1, sl2, read_overlap);

						return FBE_FALSE; /* There is no collision */
					}

					return FBE_TRUE;
				}
			}

			/* We shrink the region and need to make sure that queue remains sorted */
			if(sl1->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE){
				ext_pool_lock_set_read_overlap(sl1, sl2, read_overlap);
			}

			/* No overlap */
			return FBE_FALSE;
		} /* Done with peer request */


		if(ext_pool_lock_is_overlap(&sl1->stripe, &sl2->cmi_stripe_lock.write_region)){
			return FBE_TRUE;
		}

		if(ext_pool_lock_is_overlap(&sl1->stripe, &sl2->cmi_stripe_lock.read_region)){
			return FBE_TRUE;
		}
	}
	
	return FBE_FALSE;
}

static void 
ext_pool_lock_thread_func(void * context)
{
	fbe_queue_head_t grant_queue;
	fbe_queue_head_t cmi_request_queue;
	fbe_queue_element_t * queue_element = NULL;

    fbe_queue_init(&grant_queue);
    fbe_queue_init(&cmi_request_queue);

    while(1) {
        fbe_rendezvous_event_wait(&ext_pool_lock_event, 0);
        if(ext_pool_lock_thread_flag == METADATA_STRIPE_LOCK_THREAD_RUN) {
            fbe_spinlock_lock(&ext_pool_lock_blob_queue_lock);
			while(queue_element = fbe_queue_pop(&ext_pool_lock_blob_queue)){
				ext_pool_lock_dispatch_blob((fbe_metadata_stripe_lock_blob_t *)queue_element, &cmi_request_queue, &grant_queue); 
			}
			fbe_rendezvous_event_clear(&ext_pool_lock_event);
			fbe_spinlock_unlock(&ext_pool_lock_blob_queue_lock);			

			ext_pool_lock_dispatch_cmi_grant_queue(&grant_queue);
			ext_pool_lock_dispatch_cmi_request_queue(&cmi_request_queue);
        } else {
            break;
        }
    }/* while(1) */


    fbe_queue_destroy(&grant_queue);
    fbe_queue_destroy(&cmi_request_queue);

    ext_pool_lock_thread_flag = METADATA_STRIPE_LOCK_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

fbe_status_t 
fbe_ext_pool_lock_init(void)
{
	fbe_payload_stripe_lock_operation_t * sl;
    fbe_u32_t i;
    fbe_cpu_id_t cpu_count;

    fbe_queue_init(&ext_pool_lock_blob_queue);
    fbe_spinlock_init(&ext_pool_lock_blob_queue_lock);
	
	cpu_count = fbe_get_cpu_count();

	metadata_trace(FBE_TRACE_LEVEL_INFO, 
				   FBE_TRACE_MESSAGE_ID_INFO, 
				   "MCRMEM: Stripe lock: %d \n", (int)(sizeof(fbe_payload_stripe_lock_operation_t) * METADATA_STRIPE_LOCK_MAX_PEER_SL * cpu_count));

	//ext_pool_lock_peer_sl_memory = fbe_memory_native_allocate(sizeof(fbe_payload_stripe_lock_operation_t) * METADATA_STRIPE_LOCK_MAX_PEER_SL);
	ext_pool_lock_peer_sl_memory = 
		fbe_memory_allocate_required(sizeof(fbe_payload_stripe_lock_operation_t) * METADATA_STRIPE_LOCK_MAX_PEER_SL * cpu_count);
	
    fbe_queue_init(&ext_pool_lock_peer_sl_queue);
    fbe_spinlock_init(&ext_pool_lock_peer_sl_queue_lock);

	sl = (fbe_payload_stripe_lock_operation_t *)ext_pool_lock_peer_sl_memory;
    for(i = 0; i < (METADATA_STRIPE_LOCK_MAX_PEER_SL  * cpu_count); i++){
		fbe_queue_element_init(&sl->queue_element);
		fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);
		sl++;
	}

    
    
	fbe_rendezvous_event_init(&ext_pool_lock_event);

    /* Start thread */
    ext_pool_lock_thread_flag = METADATA_STRIPE_LOCK_THREAD_RUN;
    fbe_thread_init(&ext_pool_lock_thread_handle, "fbe_meta_extpoollock", ext_pool_lock_thread_func, (void*)(fbe_ptrhld_t)i);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_ext_pool_lock_destroy(void)
{
    fbe_cpu_id_t cpu_count;

	cpu_count = fbe_get_cpu_count();
    ext_pool_lock_thread_flag = METADATA_STRIPE_LOCK_THREAD_STOP;  
    fbe_rendezvous_event_set(&ext_pool_lock_event);
    fbe_thread_wait(&ext_pool_lock_thread_handle);
    fbe_thread_destroy(&ext_pool_lock_thread_handle);

    /* Check if we get all Sl released to the queue */
	if(ext_pool_lock_peer_sl_queue_count != 0){
		metadata_trace(FBE_TRACE_LEVEL_WARNING,
					   FBE_TRACE_MESSAGE_ID_INFO,
					   "SL: Destroy - Invalid ext_pool_lock_peer_sl_queue_count %lld \n", ext_pool_lock_peer_sl_queue_count);
	}

	if(fbe_queue_length(&ext_pool_lock_peer_sl_queue) != (METADATA_STRIPE_LOCK_MAX_PEER_SL*cpu_count)){
		metadata_trace(FBE_TRACE_LEVEL_WARNING,
					   FBE_TRACE_MESSAGE_ID_INFO,
					   "SL: Destroy - Invalid queue length %I64d \n", fbe_queue_length(&ext_pool_lock_peer_sl_queue));
	}
    
	fbe_rendezvous_event_destroy(&ext_pool_lock_event);

    fbe_queue_destroy(&ext_pool_lock_blob_queue);
    fbe_spinlock_destroy(&ext_pool_lock_blob_queue_lock);

	fbe_memory_release_required(ext_pool_lock_peer_sl_memory);

    fbe_queue_destroy(&ext_pool_lock_peer_sl_queue);
    fbe_spinlock_destroy(&ext_pool_lock_peer_sl_queue_lock);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_ext_pool_lock_operation_entry(fbe_packet_t *  packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_stripe_lock_operation_t * sl = NULL;
    fbe_payload_stripe_lock_operation_opcode_t  opcode;
    fbe_payload_stripe_lock_status_t stripe_lock_status;
	fbe_packet_attr_t packet_attr;


    sep_payload = fbe_transport_get_payload_ex(packet);
    sl = fbe_payload_ex_get_stripe_lock_operation(sep_payload);

    fbe_transport_get_packet_attr(packet, &packet_attr);
    if ((packet_attr & FBE_PACKET_FLAG_DO_NOT_STRIPE_LOCK) ||
        (sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_DO_NOT_LOCK)) {
        /* No stripe lock required, just return status. */
        fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }


    fbe_payload_stripe_lock_get_opcode(sl, &opcode);
    fbe_payload_stripe_lock_get_status(sl, &stripe_lock_status);

    switch(opcode){
    case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK:
    case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK:
		if(stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_INITIALIZED){
			metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
						   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						   "%s Invalid stripe_lock_status %d \n", __FUNCTION__, stripe_lock_status);    
		}		

        status = ext_pool_lock_lock(packet);
        break;

    case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK:
    case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_UNLOCK:
		if(stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK){
			metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
						   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						   "%s Invalid stripe_lock_status %d \n", __FUNCTION__, stripe_lock_status);    
		}		

        status = ext_pool_lock_unlock(packet);
        break;

    case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_START:
        status = ext_pool_lock_start(packet);
        break;

    case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_STOP:
        status = ext_pool_lock_stop(packet);
        break;

    default:
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s Invalid opcode %d \n", __FUNCTION__, opcode);
        fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ILLEGAL_REQUEST);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }
    return status;
}

#if 0
static __forceinline void
ext_pool_lock_update_write_count(fbe_payload_stripe_lock_operation_t * sl)
{
    fbe_metadata_element_t * mde = NULL;
    fbe_metadata_stripe_lock_blob_t * blob = NULL;

    mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;
	blob = mde->stripe_lock_blob;

	if(sl->opcode == FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK){
		if(sl->status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED || sl->status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_CANCELLED){
			if(blob->write_count == 0){ 
				metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
						FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"SL: write_count is zero \n");
			}
			blob->write_count--;
		} else {
			blob->write_count++;
		}
		return;
	}

	if(sl->opcode == FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK){
		if(blob->write_count == 0){ 
			metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
					FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"SL: write_count is zero \n");
		}
		blob->write_count--;
	}
}
#endif

static fbe_bool_t 
ext_pool_lock_check_queue_overlap(fbe_payload_stripe_lock_operation_t * sl, fbe_queue_head_t * queue)
{
	fbe_queue_element_t * queue_element = NULL;
	fbe_payload_stripe_lock_operation_t * current_sl = NULL;
	fbe_bool_t read_overlap = FBE_FALSE;
	fbe_bool_t reinsert_requiered = FBE_FALSE;

    queue_element = fbe_queue_front(queue);
	while(queue_element != NULL){ 
		current_sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
		if(ext_pool_lock_check_overlap(sl, current_sl, &read_overlap, &reinsert_requiered)){ 
		    metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
	    				    FBE_TRACE_MESSAGE_ID_INFO,
						    "SL: sl %p overlap with sl: %p \n", sl, current_sl);
            return FBE_TRUE;
        }
		queue_element = fbe_queue_next(queue, queue_element); 
	}/* while(queue_element != NULL) */
    return FBE_FALSE;
}

static fbe_bool_t 
ext_pool_lock_check_queue_overlap_with_peer(fbe_queue_head_t * queue)
{
	fbe_queue_element_t * queue_element = NULL;
	fbe_payload_stripe_lock_operation_t * current_sl = NULL;

    queue_element = fbe_queue_front(queue);
	while(queue_element != NULL){ 
		current_sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
		if(!(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST)  &&
			!(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION) &&
			!(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) &&
			!ext_pool_lock_check_peer(current_sl)){ 
		    metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
	    				    FBE_TRACE_MESSAGE_ID_INFO,
						    "SL: sl %p overlap with peer \n", current_sl);
            return FBE_TRUE;
        }
		queue_element = fbe_queue_next(queue, queue_element); 
	}/* while(queue_element != NULL) */
    return FBE_FALSE;
}




static fbe_bool_t
ext_pool_lock_insert(fbe_payload_stripe_lock_operation_t * sl, fbe_queue_head_t * q)
{
	fbe_queue_element_t * queue_element = NULL;
	fbe_payload_stripe_lock_operation_t * current_sl = NULL;
	fbe_payload_stripe_lock_operation_t * tmp_sl = NULL;
	fbe_payload_stripe_lock_operation_t * overlap_sl = NULL;
	fbe_queue_head_t deadlock_queue;
	fbe_bool_t is_deadlock = FBE_FALSE;
	fbe_queue_element_t * wait_element = NULL;
	fbe_bool_t read_overlap = FBE_FALSE;
	fbe_bool_t reinsert_requiered = FBE_FALSE;
	fbe_queue_head_t * queue = NULL;
	fbe_metadata_element_t * mde = NULL;
	fbe_ext_pool_lock_slice_entry_t * he;

	mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;
	he = fbe_ext_pool_lock_get_lock_table_entry(mde, sl->stripe.first);
	queue = &he->head;

	fbe_queue_init(&deadlock_queue);	

	if(!fbe_queue_is_empty(&sl->wait_queue)){
		metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
			FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"SL: wait_queue should be empty \n");
	} else {
		if(!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST)){
			if (sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ) {
				ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.read_region);
				ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.write_region);
			} else { /* write_lock */
				ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.write_region);
				ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.read_region);
			}
		}
	}

	queue_element = fbe_queue_front(queue);
	while(queue_element != NULL){ 
		current_sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);

		if(ext_pool_lock_check_sorted(sl, current_sl)){ /* Order is IMPORTANT */
			// Sanity testing
            //ext_pool_lock_check_queue_overlap(sl, queue);
			if(overlap_sl != NULL){ /* We had overlapped reads */
				fbe_queue_insert(&sl->queue_element, &overlap_sl->queue_element);
			} else {
				fbe_queue_insert(&sl->queue_element, &current_sl->queue_element);
			}

			/* There was no collisions check if it was a dead lock */
			if(is_deadlock == FBE_TRUE){
				while(wait_element = fbe_queue_pop(&deadlock_queue)){ 
					fbe_queue_push(&sl->wait_queue, wait_element); 
					// We need to update the region 
					tmp_sl = fbe_metadata_stripe_lock_queue_element_to_operation(wait_element);
					ext_pool_lock_update_region(sl, tmp_sl);
				}
			}

			fbe_queue_destroy(&deadlock_queue);
			sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
			return FBE_TRUE; /* There is no overlap: We are done */
		}


		//read_overlap = FBE_FALSE;
		if(ext_pool_lock_check_overlap(sl, current_sl, &read_overlap, &reinsert_requiered)){ /* The order of arguments is IMPORTANT */
			if(reinsert_requiered == FBE_TRUE){
				overlap_sl = NULL;
				reinsert_requiered = FBE_FALSE;
				read_overlap = FBE_FALSE;

				if(!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST)){
					if (sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ) {
						ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.read_region);
						ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.write_region);
					} else { /* write_lock */
						ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.write_region);
						ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.read_region);
					}
				}
				queue_element = fbe_queue_front(queue);

				continue;
			}

			/* Update current_sl regions */
			//ext_pool_lock_update_region(sl, current_sl);

			/* Check for deadlock with peer */
			if((sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) &&
						(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) &&
						!(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) &&
						!metadata_common_cmi_is_active()){

				is_deadlock = FBE_TRUE;

				current_sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_DEAD_LOCK;

				/* On the PASSIVE side - take current_sl from the queue */
				current_sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;

				queue_element = fbe_queue_next(queue, queue_element); 
				fbe_queue_remove(&current_sl->queue_element);
				current_sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
				/*if(current_sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
					fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
				}*/

				/* Take off all the waiters */
				fbe_queue_push(&deadlock_queue, &current_sl->queue_element);
				while(wait_element = fbe_queue_pop(&current_sl->wait_queue)){ fbe_queue_push(&deadlock_queue, wait_element); }
				// We "drained the wait queue of current_sl, so we must to set up the regions
				if (current_sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ) {
					ext_pool_lock_set_region(current_sl, &current_sl->cmi_stripe_lock.read_region);
					ext_pool_lock_set_invalid_region(&current_sl->cmi_stripe_lock.write_region);
				} else { /* write_lock */
					ext_pool_lock_set_region(current_sl, &current_sl->cmi_stripe_lock.write_region);
					ext_pool_lock_set_invalid_region(&current_sl->cmi_stripe_lock.read_region);
				}

				continue; /* This will take all conflicting peer waiters from the queue*/
			}

			/* Here we have a collision without deadlock */
			if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST && !metadata_common_cmi_is_active()){
				/* Mark current sl with FBE_PAYLOAD_STRIPE_LOCK_FLAG_HOLDING_PEER */
				current_sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_HOLDING_PEER;
				/* If we have a deadlock we need to "drain" the deadlock queue */
				if(is_deadlock == FBE_TRUE){

					fbe_queue_push(&current_sl->wait_queue, &sl->queue_element);
					ext_pool_lock_update_region(sl, current_sl);

					while(wait_element = fbe_queue_pop(&deadlock_queue)){ 
						// We need to update the region 
						tmp_sl = fbe_metadata_stripe_lock_queue_element_to_operation(wait_element);
						ext_pool_lock_update_region(tmp_sl, current_sl);

						fbe_queue_push(&current_sl->wait_queue, wait_element); 
					}
					//fbe_queue_push_front(&current_sl->wait_queue, &sl->queue_element);
					//fbe_queue_push(&current_sl->wait_queue, &sl->queue_element);

					if(overlap_sl != NULL){ /* We had overlapped reads and need to reorder queue */
						fbe_queue_remove(&current_sl->queue_element);
						fbe_queue_insert(&current_sl->queue_element, &overlap_sl->queue_element);
					}

					fbe_queue_destroy(&deadlock_queue);
					return FBE_FALSE;
				}

				/* Update current_sl regions */
				ext_pool_lock_update_region(sl, current_sl);

				/* On passive side enqueue the request on the front */
				fbe_queue_push_front(&current_sl->wait_queue, &sl->queue_element);
				//fbe_queue_push(&current_sl->wait_queue, &sl->queue_element);

				if(overlap_sl != NULL){ /* We had overlapped reads and need to reorder queue */
					fbe_queue_remove(&current_sl->queue_element);
					fbe_queue_insert(&current_sl->queue_element, &overlap_sl->queue_element);
				}

				fbe_queue_destroy(&deadlock_queue);
				return FBE_FALSE;
			}

			/* Update current_sl regions */
			ext_pool_lock_update_region(sl, current_sl);

			/* Update the next queue to look for */
			fbe_queue_push(&current_sl->wait_queue, &sl->queue_element);
			if(overlap_sl != NULL){ /* We had overlapped reads and need to reorder queue */
				fbe_queue_remove(&current_sl->queue_element);
				fbe_queue_insert(&current_sl->queue_element, &overlap_sl->queue_element);
			}

			fbe_queue_destroy(&deadlock_queue);
			return FBE_FALSE;
		}

		if(reinsert_requiered == FBE_TRUE){
			overlap_sl = NULL;
			reinsert_requiered = FBE_FALSE;
			read_overlap = FBE_FALSE;

			if(!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST)){
				if (sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ) {
					ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.read_region);
					ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.write_region);
				} else { /* write_lock */
					ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.write_region);
					ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.read_region);
				}
			}
			queue_element = fbe_queue_front(queue);

			continue;
		}


		/* We do not have "blocking" overlap. Check if this is overlapping reads */
		if((read_overlap == FBE_TRUE) && (overlap_sl == NULL)){  /* We have read overlap and we did not have overlapping reads yet */			
			overlap_sl = current_sl;
			read_overlap = FBE_FALSE; /* We already updated the overlap_sl */
		}

		queue_element = fbe_queue_next(queue, queue_element); 
	}/* while(queue_element != NULL) */

	/* There was no collisions check if it was a dead lock */
	if(is_deadlock == FBE_TRUE){
		while(wait_element = fbe_queue_pop(&deadlock_queue)){ 
			fbe_queue_push(&sl->wait_queue, wait_element); 
			// If we update the region we will have to implement more complicated logic 
			// when we do the grant
			tmp_sl = fbe_metadata_stripe_lock_queue_element_to_operation(wait_element);
			ext_pool_lock_update_region(tmp_sl, sl);
		}
	}

	// Sanity testing
    //ext_pool_lock_check_queue_overlap(sl, queue);

	if(overlap_sl != NULL){ /* We had overlapped reads */
		fbe_queue_insert(&sl->queue_element, &overlap_sl->queue_element);
	} else {
		fbe_queue_push(queue, &sl->queue_element);
	}
	fbe_queue_destroy(&deadlock_queue);

	sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
	return FBE_TRUE; /* There is no overlap: We are done */
}


/* Check peer collision just before the lock is granted */
static __forceinline fbe_bool_t 
ext_pool_lock_check_peer(fbe_payload_stripe_lock_operation_t * sl)
{	
    fbe_metadata_element_t * mde = NULL;
    fbe_metadata_stripe_lock_blob_t * blob = NULL;

    mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;
	blob = mde->stripe_lock_blob;

	if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER){
		return FBE_FALSE; /* We can not let it go until we get a responce from peer */
	}

	//if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_DEAD_LOCK){ /* It was a dead lock */
		if(sl->cmi_stripe_lock.grant_sl_ptr != NULL) { /* We got the grant */
			if(sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE){ /* Peer has a lock for us */
				sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;
				return FBE_TRUE; /* No collision */
			}
		}
	//}

#ifdef EXT_POOL_LOCK
	/* Non paged lock first */
	if(sl->stripe.first == blob->nonpaged_slot){
		first_slot = blob->size - 1;
		last_slot = first_slot;
	} else {
		if(sl->stripe.first >= blob->private_slot){ /* Paged lock */
			first_slot = blob->size - 2;
			last_slot = first_slot;
		} else {
			/* It is user lock */
			is_user_slot = FBE_TRUE;
			first_slot = (fbe_u32_t)(sl->stripe.first / blob->user_slot_size);
			last_slot = (fbe_u32_t)(sl->stripe.last / blob->user_slot_size);
		}
	}
#endif

	if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ){
		//for(i = first_slot; i <= last_slot; i++){
			if(fbe_ext_pool_lock_get_slot_state(mde, sl->stripe.first) == METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_PEER){
				/* We have collision */
				/* Mark sl with peer collision*/
				sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;

				return FBE_FALSE;
			}
		//}
	} else { /* It is write */
		//for(i = first_slot; i <= last_slot; i++){
			if(fbe_ext_pool_lock_get_slot_state(mde, sl->stripe.first) != METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL){
				/* We have collision */
				/* Mark sl with peer collision*/
				sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;

				return FBE_FALSE;
			}
		//}
	}

	sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;
	return FBE_TRUE; /* No collision */
}

/* Check peer collision just before the lock is granted */
static __forceinline fbe_bool_t 
ext_pool_lock_mark_grant_to_peer(fbe_payload_stripe_lock_operation_t * sl)
{	
    fbe_metadata_element_t * mde = NULL;
    fbe_metadata_stripe_lock_blob_t * blob = NULL;

    mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;
	blob = mde->stripe_lock_blob;

	if(!fbe_metadata_is_peer_object_alive(mde)){ /* If peer is not alive we will not mark the slots */
		sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANT;
		return FBE_TRUE; /* No collision */
	}

	if(sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE){ /* Slots should not be marked */
#if 0
		metadata_trace(FBE_TRACE_LEVEL_INFO,
	    				FBE_TRACE_MESSAGE_ID_INFO,
						"SL: MARK GRANT sl: %p \n", sl);
#endif
		sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANT;
		return FBE_TRUE; /* No collision */	
	}

#ifdef EXT_POOL_LOCK
	/* Non paged lock first */
	if(sl->stripe.first == blob->nonpaged_slot){
		first_slot = blob->size - 1;
		last_slot = first_slot;
	} else {
		if(sl->stripe.first >= blob->private_slot){ /* Paged lock */
			first_slot = blob->size - 2;
			last_slot = first_slot;
		} else {
			/* It is user lock */
			is_user_slot = FBE_TRUE;
			first_slot = (fbe_u32_t)(sl->stripe.first / blob->user_slot_size);
			last_slot = (fbe_u32_t)(sl->stripe.last / blob->user_slot_size);
		}
	}
#endif

	if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ){
		//for(i = first_slot; i <= last_slot; i++){
			if (fbe_ext_pool_lock_get_slot_state(mde, sl->stripe.first) == METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL){
				fbe_ext_pool_lock_set_slot_state(mde, sl->stripe.first, METADATA_STRIPE_LOCK_SLOT_STATE_SHARED);
			}
		//}
	} else { /* It is write */
		//for(i = first_slot; i <= last_slot; i++){
			fbe_ext_pool_lock_set_slot_state(mde, sl->stripe.first, METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_PEER);
		//}
	}

	sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANT;

	// Sanity testing
	//ext_pool_lock_check_queue_overlap_with_peer(&mde->stripe_lock_queue_head);

	return FBE_TRUE; /* No collision */
}

/* This function checks if we need to send cmi request or there is already outstanding one */
static __forceinline fbe_bool_t 
ext_pool_lock_is_cmi_required(fbe_payload_stripe_lock_operation_t * sl)
{
    fbe_metadata_element_t * mde = NULL;
	
    mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;

	if(!fbe_metadata_is_peer_object_alive(mde)){
		return FBE_FALSE;
	}

	if(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_PEER_LOST){
		return FBE_FALSE;
	}

	if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER){
		return FBE_FALSE;
	}

	/* Always send CMI request */
	return FBE_TRUE;
}

/* Mark slots related to the peer grant */
static __forceinline fbe_bool_t 
ext_pool_lock_mark_grant_from_peer(fbe_metadata_element_t * mde, 
											  fbe_metadata_cmi_stripe_lock_t * cmi_stripe_lock, 
											  fbe_payload_stripe_lock_operation_t * sl)
{	
    fbe_metadata_stripe_lock_blob_t * blob = NULL;
	fbe_metadata_stripe_lock_region_t * region;

	blob = mde->stripe_lock_blob;
	region = &sl->stripe;

#ifdef EXT_POOL_LOCK
	/* Non paged lock first */
	if(region->first == blob->nonpaged_slot){
		first_slot = blob->size - 1;
		last_slot = first_slot;
	} else {
		if(region->first >= blob->private_slot){ /* Paged lock */			
			first_slot = blob->size - 2;
			last_slot = first_slot;			
		} else {
			/* It is user lock */
			is_user_slot = FBE_TRUE;
			first_slot = (fbe_u32_t)(region->first / blob->user_slot_size);			
			last_slot = (fbe_u32_t)(region->last / blob->user_slot_size);			
		}
	}

	if (is_user_slot && fbe_metadata_is_ndu_in_progress())
	{
		return FBE_TRUE;
	}
#endif

	if(cmi_stripe_lock->header.metadata_cmi_message_type == FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_GRANT){
		//for(i = first_slot; i <= last_slot; i++){
			if(fbe_ext_pool_lock_get_slot_state(mde, region->first) == METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_PEER){
				fbe_ext_pool_lock_set_slot_state(mde, region->first, METADATA_STRIPE_LOCK_SLOT_STATE_SHARED);
			}
		//}
	} else { /* It is write grant */
		//for(i = first_slot; i <= last_slot; i++){
			fbe_ext_pool_lock_set_slot_state(mde, region->first, METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL);
		//}
	}

	return FBE_TRUE; /* No collision */
}




static __forceinline fbe_status_t
ext_pool_lock_dispatch(fbe_payload_stripe_lock_operation_t * sl, fbe_queue_head_t * tmp_queue, fbe_bool_t * is_cmi_required)
{
	fbe_queue_element_t * queue_element = NULL;
	fbe_payload_stripe_lock_operation_t * current_sl = NULL;
	fbe_metadata_element_t * mde = NULL;
	
	mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;

	while(queue_element = fbe_queue_pop(&sl->wait_queue)){ 
		current_sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);

		if(!(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && /* if it is not peer request */
			!(current_sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE) && /* We do not need to release the peer */
			!(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) && /* We can not cancel this packet */
				(fbe_transport_is_packet_cancelled(current_sl->cmi_stripe_lock.packet) ||
				(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_DESTROY_ABORT_STRIPE_LOCKS) ||
				(!ext_pool_lock_is_np_lock(sl) && (mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_STRIPE_LOCKS)) ||
                ((mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_MONITOR_OPS) && ext_pool_lock_is_monitor_op_abort_needed(mde, current_sl, FBE_FALSE))))
		{
			if(current_sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
				fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
			}

			if(fbe_transport_is_packet_cancelled(current_sl->cmi_stripe_lock.packet)){
				fbe_payload_stripe_lock_set_status(current_sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_CANCELLED);
				fbe_transport_set_status(current_sl->cmi_stripe_lock.packet, FBE_STATUS_CANCELED, 0);
			} else {
				fbe_transport_record_callback_with_action(current_sl->cmi_stripe_lock.packet,
															(fbe_packet_completion_function_t) ext_pool_lock_dispatch, 
															PACKET_SL_ABORT);

				fbe_payload_stripe_lock_set_status(current_sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);
				fbe_transport_set_status(current_sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
			}

            if (fbe_queue_is_element_on_queue(&current_sl->cmi_stripe_lock.packet->queue_element)){
                metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
						        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						        "%s stripe lock %p packet already on the queue.\n", __FUNCTION__, current_sl);
            }
			fbe_queue_push(tmp_queue, &current_sl->cmi_stripe_lock.packet->queue_element);
			continue;
		}

		if(ext_pool_lock_insert(current_sl, &mde->stripe_lock_queue_head)){ /* We done: Stripe lock granted */
			if(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST){
				ext_pool_lock_mark_grant_to_peer(current_sl); /* Mark slots */
				//fbe_queue_push(grant_queue, (fbe_queue_element_t *)current_sl); /* This is ugly */
				* is_cmi_required = FBE_TRUE;
				continue;
			}
			if(ext_pool_lock_check_peer(current_sl)){ /* No collision with peer */
				fbe_payload_stripe_lock_set_status(current_sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
				fbe_transport_set_status(current_sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
                if (fbe_queue_is_element_on_queue(&current_sl->cmi_stripe_lock.packet->queue_element)){
                    metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
						            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						            "%s stripe lock %p packet already on the queue.\n", __FUNCTION__, current_sl);
                }
				fbe_queue_push(tmp_queue, &current_sl->cmi_stripe_lock.packet->queue_element);
			} else {
				ext_pool_lock_send_request_sl(current_sl, is_cmi_required);
			}
		}
	}

	return FBE_STATUS_OK;
}

static fbe_status_t
ext_pool_lock_send_request_sl(fbe_payload_stripe_lock_operation_t * sl, fbe_bool_t * is_cmi_required)
{
	fbe_payload_stripe_lock_operation_t * request_sl = NULL;
	fbe_queue_element_t * queue_element = NULL;
	fbe_metadata_element_t * mde = NULL;

	/* We should be under the mde lock */
	if(!ext_pool_lock_is_cmi_required(sl)){
		return FBE_STATUS_OK;
	}

	mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;

	/* Allocate request_sl */
	fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
	queue_element = fbe_queue_pop(&ext_pool_lock_peer_sl_queue);	
	ext_pool_lock_peer_sl_queue_count++;
	fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);

	/* Fill in the request_sl */
	request_sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
	fbe_copy_memory(request_sl, sl, sizeof(fbe_payload_stripe_lock_operation_t));

#ifdef EXT_POOL_LOCK
	if(request_sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ){
		if(request_sl->cmi_stripe_lock.read_region.first < mde->stripe_lock_blob->private_slot){
			if(request_sl->cmi_stripe_lock.read_region.last > mde->stripe_lock_blob->private_slot){
				metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
					FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"SL: User/Private overlap \n");
			}
		}
	} else { /* It is write */
		if(request_sl->cmi_stripe_lock.write_region.first < mde->stripe_lock_blob->private_slot){
			if(request_sl->cmi_stripe_lock.write_region.last > mde->stripe_lock_blob->private_slot){
				metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
					FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"SL: User/Private overlap \n");
			}
		}
	}
#endif

	request_sl->flags = FBE_PAYLOAD_STRIPE_LOCK_FLAG_LOCAL_REQUEST;
	request_sl->cmi_stripe_lock.request_sl_ptr = sl; /* Remember the original sl */
	request_sl->cmi_stripe_lock.grant_sl_ptr = NULL;
	request_sl->cmi_stripe_lock.flags = 0;

	if (fbe_transport_is_monitor_packet(sl->cmi_stripe_lock.packet, fbe_base_config_metadata_element_to_object_id(mde))) {
		request_sl->cmi_stripe_lock.flags |= FBE_METADATA_CMI_STRIPE_LOCK_FLAG_MONITOR_OP;
	}

	sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER; /* This sl will be granted by it's own grant ONLY */    

	fbe_spinlock_lock(&mde->stripe_lock_blob->peer_sl_queue_lock);
	fbe_queue_push(&mde->stripe_lock_blob->peer_sl_queue, &request_sl->queue_element);
	fbe_spinlock_unlock(&mde->stripe_lock_blob->peer_sl_queue_lock);

	* is_cmi_required = FBE_TRUE;
	return FBE_STATUS_OK;
}

/* This function sends sl requests to the peer */
static fbe_status_t 
ext_pool_lock_dispatch_cmi_request_queue(fbe_queue_head_t * cmi_request_queue)
{
    fbe_payload_stripe_lock_operation_t * sl = NULL;
    fbe_metadata_element_t * mde = NULL;
	fbe_queue_element_t * queue_element  = NULL;

	while(queue_element = fbe_queue_pop(cmi_request_queue)){
		sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
		mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;

		sl->cmi_stripe_lock.header.metadata_element_receiver = mde->peer_metadata_element_ptr;
		sl->cmi_stripe_lock.header.object_id = fbe_base_config_metadata_element_to_object_id(mde);

		if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ){
			sl->cmi_stripe_lock.header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_LOCK;

#if SL_DEBUG_MODE
			if(sl->cmi_stripe_lock.header.object_id == 0x10c)
			metadata_trace(FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"SL: %p READ_LOCK id: %x R: first %llX last %llX\n",sl,
							sl->cmi_stripe_lock.header.object_id,
							sl->cmi_stripe_lock.read_region.first, sl->cmi_stripe_lock.read_region.last);
#endif

		} else { /* It is write */
			sl->cmi_stripe_lock.header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_LOCK;
#if SL_DEBUG_MODE
			if(sl->cmi_stripe_lock.header.object_id == 0x10c)
			metadata_trace(FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"SL: %p WRITE_LOCK id: %x W: first %llX last %llX \n",sl,
							sl->cmi_stripe_lock.header.object_id,
							sl->cmi_stripe_lock.write_region.first, sl->cmi_stripe_lock.write_region.last);
#endif
		}

		//sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER;

		if(fbe_metadata_is_peer_object_alive(mde)){
			fbe_metadata_cmi_send_message((fbe_metadata_cmi_message_t *)&sl->cmi_stripe_lock, sl); 
		} else {
			/* Release peer_sl */
			fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
			fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);
			ext_pool_lock_peer_sl_queue_count--;			
			fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
		}

	}/* while(queue_element = fbe_queue_pop(cmi_request_queue)) */

	return FBE_STATUS_OK;
}

static fbe_status_t 
ext_pool_lock_dispatch_cmi_grant_queue(fbe_queue_head_t * grant_queue)
{
	fbe_queue_element_t * queue_element;
	fbe_payload_stripe_lock_operation_t * sl;
	fbe_metadata_element_t * mde = NULL;
	fbe_metadata_stripe_lock_blob_t * blob = NULL;	

	while(queue_element = fbe_queue_pop(grant_queue)){
		//sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
		sl = (fbe_payload_stripe_lock_operation_t *)queue_element; /* This is ugly and we may not need it anymore */

		mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;
		blob = mde->stripe_lock_blob;
		if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ){
			sl->cmi_stripe_lock.header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_GRANT;
			ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.read_region);
			ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.write_region);	
            
			#if SL_DEBUG_MODE
			if(sl->cmi_stripe_lock.header.object_id == 0x10c){            
			metadata_trace(FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"SL: %p READ_GRANT id: %x R: first %llX last %llX\n",sl,
							sl->cmi_stripe_lock.header.object_id,								
							sl->cmi_stripe_lock.read_region.first, sl->cmi_stripe_lock.read_region.last);
				metadata_trace(FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"SL: SLOT id: %x: %x %x %x %x %x %x %x %x\n", sl->cmi_stripe_lock.header.object_id,
								mde->stripe_lock_blob->slot[0],
								mde->stripe_lock_blob->slot[1],
								mde->stripe_lock_blob->slot[2],
								mde->stripe_lock_blob->slot[3],
								mde->stripe_lock_blob->slot[4],
								mde->stripe_lock_blob->slot[5],
								mde->stripe_lock_blob->slot[6],
								mde->stripe_lock_blob->slot[7]);

			}            
			#endif
		} else {/* It is write lock */
			sl->cmi_stripe_lock.header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_GRANT;
			ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.write_region);
			ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.read_region);            
			if(ext_pool_lock_is_paged_lock(sl)){
				if (sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE) {
					fbe_metadata_paged_release_blobs(mde, sl->cmi_stripe_lock.write_region.first,
                                                 sl->cmi_stripe_lock.write_region.last - sl->cmi_stripe_lock.write_region.first + 1);
				} else {
					fbe_metadata_paged_release_blobs(mde, FBE_LBA_INVALID, 0);
				}
			}
			#if SL_DEBUG_MODE
			if(sl->cmi_stripe_lock.header.object_id == 0x10c){            
			metadata_trace(FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"SL: %p WRITE_GRANT id: %x W: first %llX last %llX \n",sl,
							sl->cmi_stripe_lock.header.object_id,
							sl->cmi_stripe_lock.write_region.first, sl->cmi_stripe_lock.write_region.last);
				metadata_trace(FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"SL: SLOT id: %x: %x %x %x %x %x %x %x %x\n", sl->cmi_stripe_lock.header.object_id,
								mde->stripe_lock_blob->slot[0],
								mde->stripe_lock_blob->slot[1],
								mde->stripe_lock_blob->slot[2],
								mde->stripe_lock_blob->slot[3],
								mde->stripe_lock_blob->slot[4],
								mde->stripe_lock_blob->slot[5],
								mde->stripe_lock_blob->slot[6],
								mde->stripe_lock_blob->slot[7]);            
			#endif
			}
		
		//sl->cmi_stripe_lock.request_sl_ptr = NULL;
		//sl->cmi_stripe_lock.grant_sl_ptr = NULL;
		//sl->cmi_stripe_lock.flags = 0;

		fbe_metadata_cmi_send_message((fbe_metadata_cmi_message_t *)&sl->cmi_stripe_lock, sl);
	} /* while(queue_element = fbe_queue_pop(grant_queue)) */

	return FBE_STATUS_OK;
}



/* Handles requests coming from peer */
static fbe_status_t 
ext_pool_lock_dispatch_blob(fbe_metadata_stripe_lock_blob_t * blob, fbe_queue_head_t * cmi_request_queue, fbe_queue_head_t * grant_queue)
{
    fbe_payload_stripe_lock_operation_t * sl = NULL;
    //fbe_payload_stripe_lock_operation_t * grant_sl = NULL;

	fbe_metadata_element_t * mde = NULL;
	fbe_queue_element_t * queue_element  = NULL;
	fbe_queue_element_t * next_queue_element  = NULL;	

	//fbe_queue_element_t * grant_queue_element  = NULL;	
	fbe_queue_head_t tmp_queue;

	fbe_queue_init(&tmp_queue);
	mde = blob->mde;

    if ((mde==NULL) ||
        (mde->metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)){
        return FBE_STATUS_OK;
	}

	/* Drain peer_sl_queue under the lock */
	fbe_spinlock_lock(&blob->peer_sl_queue_lock);
	while(queue_element = fbe_queue_pop(&blob->peer_sl_queue)){
		fbe_queue_push(&tmp_queue, queue_element);
	}
	fbe_spinlock_unlock(&blob->peer_sl_queue_lock);

	while(queue_element = fbe_queue_pop(&tmp_queue)){
		sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);					
		if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_LOCAL_REQUEST){
			fbe_queue_push(cmi_request_queue, &sl->queue_element);
			continue;
		}
		
		if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST){
            /* Check if we have been notified that the peer is gone. */
            if (mde->attributes & (FBE_METADATA_ELEMENT_ATTRIBUTES_SL_PEER_LOST|FBE_METADATA_ELEMENT_ATTRIBUTES_PEER_DEAD)) {
                 /* Just free the request, we don't need it if the peer is dead */
                fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
                fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);  
                ext_pool_lock_peer_sl_queue_count--;
                fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
            /* Check if the gate has been closed. */
            } else if ((mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_DESTROY_ABORT_STRIPE_LOCKS) ||
                       (!ext_pool_lock_is_np_lock(sl) && (mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_STRIPE_LOCKS)) ||
                       ((mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_MONITOR_OPS) && ext_pool_lock_is_monitor_op_abort_needed(mde, sl, FBE_FALSE))) {
                /* Gate has been closed, abort the incoming request. */
                fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);                    
                fbe_ext_pool_send_aborted_msg(mde, sl);
            } else {
				
				/* We need to handle the Large I/O requests here */

#ifdef EXT_POOL_LOCK
				/* Check that we do not struddle the hash buckets */
				if(mde->stripe_lock_hash != NULL){
					if(((sl->stripe.first / mde->stripe_lock_hash->divisor) != (sl->stripe.last / mde->stripe_lock_hash->divisor)) &&
						(!ext_pool_lock_is_np_lock(sl) && !ext_pool_lock_is_paged_lock(sl)) ){
							fbe_ext_pool_lock_all(mde);
							if(!(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED)) {
								fbe_spinlock_lock(&mde->stripe_lock_spinlock);

								ext_pool_lock_disable_hash(mde);
							}

							/* Mark this sl as a large I/O */
							sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE;

							/* Increment large_io_counter under the all possible locks */
							fbe_atomic_increment(&mde->stripe_lock_large_io_count);

							/* The hash is already disabled unlock_all will do the unlock for stripe_lock_spinlock */
							//fbe_spinlock_unlock(&mde->stripe_lock_spinlock);

							fbe_ext_pool_unlock_all(mde);
					}
				}
#endif

				fbe_ext_pool_lock_hash(mde, sl);
                if(ext_pool_lock_insert(sl, &mde->stripe_lock_queue_head)){ /* We done: Stripe lock granted */					
                    ext_pool_lock_mark_grant_to_peer(sl); /* Mark slots */
                }
				fbe_ext_pool_unlock_hash(mde, sl);
            }
		}
	}/* while(queue_element = fbe_queue_pop(&tmp_queue)) */

	fbe_stripe_lock_scan_for_grants(mde, grant_queue);

	fbe_spinlock_lock(&blob->peer_sl_queue_lock);
	/* Another cycle on peer_sl_queue */
	queue_element = fbe_queue_front(&blob->peer_sl_queue);
	while(queue_element){
		sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);					
		if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_LOCAL_REQUEST){
			next_queue_element = fbe_queue_next(&blob->peer_sl_queue, queue_element);
			fbe_queue_remove(queue_element);
			fbe_queue_push(cmi_request_queue, &sl->queue_element);
			queue_element = next_queue_element;		
		} else {
			queue_element = fbe_queue_next(&blob->peer_sl_queue, queue_element);
		}
	}/* while(queue_element = fbe_queue_pop(&blob->peer_sl_queue)) */
		
	fbe_spinlock_unlock(&blob->peer_sl_queue_lock);
	
#ifdef EXT_POOL_LOCK
	if(mde->stripe_lock_blob->flags & METADATA_STRIPE_LOCK_BLOB_FLAG_ENABLE_HASH){
		mde->stripe_lock_blob->flags &= ~METADATA_STRIPE_LOCK_BLOB_FLAG_ENABLE_HASH;

		fbe_ext_pool_lock_all(mde);
		if(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED) {
			
			ext_pool_lock_enable_hash(mde);
			/* If hash was succesfully enabled we need to unlock the global lock (unlock all will not do it anymore ) */
			if(!(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED)){
				fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
			}
		}

		fbe_ext_pool_unlock_all(mde);		
	}
#endif

	fbe_queue_destroy(&tmp_queue);
	return FBE_STATUS_OK;
}

static fbe_status_t 
ext_pool_lock_lock(fbe_packet_t *  packet)
{   
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_stripe_lock_operation_t * sl = NULL;
    fbe_metadata_element_t * mde = NULL;
    fbe_metadata_stripe_lock_blob_t * blob = NULL;
    fbe_bool_t is_cmi_required = FBE_FALSE;
	fbe_bool_t is_hash_enable_required = FBE_FALSE;
    fbe_object_id_t object_id;

    sep_payload = fbe_transport_get_payload_ex(packet);
    sl = fbe_payload_ex_get_stripe_lock_operation(sep_payload);
    mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;
    object_id = fbe_base_config_metadata_element_to_object_id(mde);
	blob = mde->stripe_lock_blob;


	sl->priv_flags = 0;

#ifdef EXT_POOL_LOCK
	if(sl->stripe.first == blob->nonpaged_slot){
		sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_NP;
	} else if(sl->stripe.first >= blob->private_slot && sl->stripe.first != blob->nonpaged_slot){
		sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_PAGED;
	}

	/* Check that we do not struddle the hash buckets */
	if(mde->stripe_lock_hash != NULL){
		if(((sl->stripe.first / mde->stripe_lock_hash->divisor) != (sl->stripe.last / mde->stripe_lock_hash->divisor)) &&
			(!ext_pool_lock_is_np_lock(sl) && !ext_pool_lock_is_paged_lock(sl)) ){
				fbe_ext_pool_lock_all(mde);
				if(!(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED)) {
					fbe_spinlock_lock(&mde->stripe_lock_spinlock);

					ext_pool_lock_disable_hash(mde);
				}

				/* Mark this sl as a large I/O */
				sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE;

				/* Increment large_io_counter under the all possible locks */
				fbe_atomic_increment(&mde->stripe_lock_large_io_count);

				/* The hash is already disabled unlock_all will do the unlock for stripe_lock_spinlock */
				//fbe_spinlock_unlock(&mde->stripe_lock_spinlock);

				fbe_ext_pool_unlock_all(mde);
		}
	}
#endif

	fbe_ext_pool_lock_hash(mde, sl);
    //fbe_spinlock_lock(&mde->stripe_lock_spinlock);

	/* Evaluate stripe_lock_large_io_count and hash mode */
	if((mde->stripe_lock_hash != NULL) && 
		(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED) &&
		(mde->stripe_lock_large_io_count == 0)){
			is_hash_enable_required = FBE_TRUE;
	} /* HASH enable checking */

    if ((mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_DESTROY_ABORT_STRIPE_LOCKS) ||
        (!ext_pool_lock_is_np_lock(sl) &&
         ((mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_STRIPE_LOCKS)
          /*!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_NO_ABORT)*/   )) ||
        ((mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_MONITOR_OPS) && fbe_transport_is_monitor_packet(packet, object_id) && 
         !ext_pool_lock_is_np_lock(sl) && !ext_pool_lock_is_paged_lock(sl)))
    {
        /* We are aborting, so drop it now. */
		
		/* Handle the large I/O case */
		if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
			fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
		}

        //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
		fbe_ext_pool_unlock_hash(mde, sl);
        fbe_transport_record_callback_with_action(packet, (fbe_packet_completion_function_t) ext_pool_lock_lock, PACKET_SL_ABORT);
        fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        if (!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_SYNC_MODE))
        {
            fbe_transport_complete_packet(packet);
        }
        return FBE_STATUS_OK; 
    }

    if ((mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_PEER_LOST) && 
        (!ext_pool_lock_is_np_lock(sl)) &&
        (fbe_transport_is_monitor_packet(packet, object_id)))
    {

		/* Handle the large I/O case */
		if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
			fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
		}

        //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
		fbe_ext_pool_unlock_hash(mde, sl);
        fbe_transport_record_callback_with_action(packet, (fbe_packet_completion_function_t) ext_pool_lock_lock, PACKET_SL_ABORT);
        fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        if (!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_SYNC_MODE))
        {
            fbe_transport_complete_packet(packet);
        }
        return FBE_STATUS_OK; 
    }
    
    fbe_queue_init(&sl->wait_queue);
    sl->cmi_stripe_lock.packet = packet;
	sl->cmi_stripe_lock.flags = 0;
	sl->cmi_stripe_lock.request_sl_ptr = NULL;
	sl->cmi_stripe_lock.grant_sl_ptr = NULL;

    if (sl->opcode == FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK) {
        ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.read_region);
        ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.write_region);
		sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_PENDING | FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ;
    } else { /* write_lock */
        ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.write_region);
        ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.read_region);
		sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_PENDING;
    }

    //mde->stripe_lock_count++;
	//ext_pool_lock_update_write_count(sl);
	fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING);

	if(ext_pool_lock_insert(sl, &mde->stripe_lock_queue_head)){ /* We done: Stripe lock granted */
		if(ext_pool_lock_check_peer(sl)){ /* No collision with peer */
			fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			//fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
			fbe_ext_pool_unlock_hash(mde, sl);

			if(!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_SYNC_MODE)){
				fbe_transport_complete_packet(packet);
			}

			ext_pool_lock_reschedule_blob(mde, is_cmi_required, is_hash_enable_required);

			return FBE_STATUS_OK;
		} else {
			fbe_transport_set_cancel_function(packet, fbe_metadata_cancel_function, packet);
			/* Send CMI message */
			ext_pool_lock_send_request_sl(sl, &is_cmi_required);
		}
	}
	
    //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
	fbe_ext_pool_unlock_hash(mde, sl);

	ext_pool_lock_reschedule_blob(mde, is_cmi_required, is_hash_enable_required);

	return FBE_STATUS_PENDING;
}

static fbe_status_t 
ext_pool_lock_unlock(fbe_packet_t *  packet)
{    
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_stripe_lock_operation_t * sl = NULL;
    fbe_metadata_element_t * mde = NULL;
    fbe_queue_head_t tmp_queue;
	fbe_bool_t is_cmi_required = FBE_FALSE;
	fbe_metadata_stripe_lock_blob_t *blob = NULL;


    sep_payload = fbe_transport_get_payload_ex(packet);
    sl = fbe_payload_ex_get_stripe_lock_operation(sep_payload);
	mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;
    blob = mde->stripe_lock_blob;

    fbe_queue_init(&tmp_queue);    

	//fbe_spinlock_lock(&mde->stripe_lock_spinlock);
	fbe_ext_pool_lock_hash(mde, sl);

	// Sanity testing
	// if(!ext_pool_lock_check_peer(sl)){ fbe_debug_break(); }

	//ext_pool_lock_update_write_count(sl);
    fbe_queue_remove(&sl->queue_element);
	sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
	if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
		fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
	}

	fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
	ext_pool_lock_dispatch(sl, &tmp_queue, &is_cmi_required); 

    //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
	fbe_ext_pool_unlock_hash(mde, sl);

    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);	

    fbe_queue_destroy(&tmp_queue);

	ext_pool_lock_reschedule_blob(mde, is_cmi_required, FBE_FALSE);

	if(sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE){
        if(ext_pool_lock_is_paged_lock(sl)){
            fbe_metadata_paged_release_blobs(mde, sl->stripe.first, sl->stripe.last - sl->stripe.first + 1);
        }

		fbe_ext_pool_send_release_msg(mde, sl);
		return FBE_STATUS_OK;
	}

    
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

#if 0 
    /* AR 497201: PSM timeout due to an IRP being held in SEP.  SEP has an outstanding packet that still has the stripe lock
     * being held during read unlock due to the SYNC mode is not supported at this point for Unlock case. Discussed this with
     * Peter and commented this check for now until the Stripelock Unlock SYNC mode is supported, and just set the packet to 
     * complete. 
     */
    if(!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_SYNC_MODE))
#endif

    {
        fbe_transport_complete_packet(packet);
    }

    return FBE_STATUS_OK;
}


static fbe_status_t
fbe_ext_pool_lock_abort_paged_waiters(fbe_queue_head_t * wait_queue, fbe_queue_head_t * abort_queue)
{
	fbe_queue_element_t * queue_element = NULL;
	fbe_payload_stripe_lock_operation_t * sl = NULL;

	queue_element = fbe_queue_front(wait_queue);
	while(queue_element != NULL){
		sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
		queue_element = fbe_queue_next(wait_queue, queue_element);

		if(1/*!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_NO_ABORT)*/){ /* We need to abort this sl */
			fbe_queue_remove(&sl->queue_element);
			fbe_queue_push(abort_queue, &sl->queue_element);
		}
	}

	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_ext_pool_lock_element_abort_queue_callback(fbe_metadata_element_t* mde,
                                               fbe_ext_pool_lock_slice_entry_t * entry, 
                                               void * context)
{
    ext_pool_lock_callback_context_t *context_p = (ext_pool_lock_callback_context_t *)context;
    fbe_queue_head_t *abort_queue = context_p->param_1;
    fbe_queue_head_t *tmp_queue = context_p->param_2; 

    fbe_ext_pool_lock_element_abort_queue(mde, &entry->head, abort_queue, tmp_queue);
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_ext_pool_lock_element_abort_impl(fbe_metadata_element_t * mde,
                                            fbe_bool_t             abort_non_paged)
{    
    fbe_queue_head_t abort_queue;
    fbe_queue_head_t tmp_queue;    
	fbe_queue_element_t * abort_element = NULL;
	fbe_payload_stripe_lock_operation_t * abort_sl = NULL;
	fbe_payload_stripe_lock_operation_t * sl = NULL;
	fbe_bool_t is_cmi_required = FBE_FALSE;
	fbe_metadata_stripe_lock_blob_t *blob = NULL;    	
	fbe_object_id_t object_id;
	//fbe_u32_t i;
	ext_pool_lock_callback_context_t context;

	if(mde->metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
		return FBE_STATUS_OK;
	}

	object_id = fbe_base_config_metadata_element_to_object_id(mde);

    metadata_trace(FBE_TRACE_LEVEL_INFO, 
	                FBE_TRACE_MESSAGE_ID_INFO,
					"SL: Abort object_id 0x%x\n", object_id);


	fbe_queue_init(&abort_queue);
	fbe_queue_init(&tmp_queue);	

	fbe_ext_pool_lock_all(mde);

    /* Mark the metadata element so that it will not grant any stripe locks. */
    if(abort_non_paged)
    {
        fbe_atomic_32_or(&mde->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_DESTROY_ABORT_STRIPE_LOCKS);
    }
    else
    {
        fbe_atomic_32_or(&mde->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_STRIPE_LOCKS);
    }

	blob = mde->stripe_lock_blob;

	context.param_1 = &abort_queue;
	context.param_2 = &tmp_queue;
	fbe_ext_pool_lock_traverse_hash_table(mde, fbe_ext_pool_lock_element_abort_queue_callback, &context);

	sl = NULL; /* To prevent future access */

	while(abort_element = fbe_queue_pop(&abort_queue)){
		abort_sl = fbe_metadata_stripe_lock_queue_element_to_operation(abort_element);

		/* This SL's we can not abort */
		if((!abort_non_paged && ext_pool_lock_is_np_lock(abort_sl)) || 
			((abort_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) && fbe_cmi_is_peer_alive())){
				/* This will "reset" the regions */
			if(abort_sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ){
				ext_pool_lock_set_region(abort_sl, &abort_sl->cmi_stripe_lock.read_region);
				ext_pool_lock_set_invalid_region(&abort_sl->cmi_stripe_lock.write_region);
			} else { /* write_lock */
				ext_pool_lock_set_region(abort_sl, &abort_sl->cmi_stripe_lock.write_region);
				ext_pool_lock_set_invalid_region(&abort_sl->cmi_stripe_lock.read_region);
			}

			if(ext_pool_lock_insert(abort_sl, &mde->stripe_lock_queue_head)){ /* We done: Stripe lock granted */
				if(abort_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST){
					ext_pool_lock_mark_grant_to_peer(abort_sl); /* Mark slots */
					//fbe_queue_push(grant_queue, (fbe_queue_element_t *)sl); /* This is ugly */
					is_cmi_required = FBE_TRUE;
					continue;
				}
				if(ext_pool_lock_check_peer(abort_sl)){ /* No collision with peer */
					fbe_payload_stripe_lock_set_status(abort_sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
					fbe_transport_set_status(abort_sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
                    if (fbe_queue_is_element_on_queue(&abort_sl->cmi_stripe_lock.packet->queue_element)){
                        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
						                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						                "%s stripe lock %p packet already on the queue.\n", __FUNCTION__, abort_sl);
                    }
					fbe_queue_push(&tmp_queue, &abort_sl->cmi_stripe_lock.packet->queue_element);
				}else { /* Peer collision */
					ext_pool_lock_send_request_sl(abort_sl, &is_cmi_required);
				}
			}
		} else {
			if(abort_sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
				fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
			}
			if((abort_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && 
				!(abort_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANT)){ /* We need to abort peer requests */

				fbe_payload_stripe_lock_set_status(abort_sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);
				//ext_pool_lock_update_write_count(abort_sl);

				fbe_ext_pool_send_aborted_msg(mde, abort_sl);
				continue;
			}

			if(abort_sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE){
				fbe_transport_record_callback_with_action(abort_sl->cmi_stripe_lock.packet, (fbe_packet_completion_function_t) fbe_ext_pool_lock_element_abort_impl, PACKET_SL_ABORT);
				fbe_payload_stripe_lock_set_status(abort_sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);
				//ext_pool_lock_update_write_count(abort_sl);
				/* need to set packet status here, otherwise, fbe_provision_drive_utils_process_block_status() will complain. */
				fbe_transport_set_status(abort_sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);

               if(ext_pool_lock_is_paged_lock(abort_sl)){
                    fbe_metadata_paged_release_blobs(mde, abort_sl->stripe.first, abort_sl->stripe.last - abort_sl->stripe.first + 1);
                }

				fbe_ext_pool_send_release_msg(mde, abort_sl);
				continue;
			}

            fbe_transport_record_callback_with_action(abort_sl->cmi_stripe_lock.packet, (fbe_packet_completion_function_t) fbe_ext_pool_lock_element_abort_impl, PACKET_SL_ABORT);
			fbe_payload_stripe_lock_set_status(abort_sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);
			//ext_pool_lock_update_write_count(abort_sl);
			/* need to set packet status here, otherwise, fbe_provision_drive_utils_process_block_status() will complain. */
			fbe_transport_set_status(abort_sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
            if (fbe_queue_is_element_on_queue(&abort_sl->cmi_stripe_lock.packet->queue_element)){
                metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
				                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
				                "%s stripe lock %p packet already on the queue.\n", __FUNCTION__, abort_sl);
            }
			fbe_queue_push(&tmp_queue, &abort_sl->cmi_stripe_lock.packet->queue_element);
		}
	}/* while(abort_element = fbe_queue_pop(&abort_queue)) */

	fbe_ext_pool_unlock_all(mde);

    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);

	ext_pool_lock_reschedule_blob(mde, is_cmi_required, FBE_FALSE);

    fbe_queue_destroy(&abort_queue);
    fbe_queue_destroy(&tmp_queue);    

    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_ext_pool_lock_element_abort(fbe_metadata_element_t * mde)
{
    // Abort all stripe locks except non paged.
    return fbe_ext_pool_lock_element_abort_impl(mde, FALSE);
}


fbe_status_t
fbe_ext_pool_lock_element_destroy_abort(fbe_metadata_element_t * mde)
{
    // Abort all stripe locks including non paged.
    return fbe_ext_pool_lock_element_abort_impl(mde, TRUE);
}


static fbe_status_t 
fbe_ext_pool_lock_element_abort_monitor_op_in_queue(fbe_metadata_element_t * mde, 
                                                           fbe_queue_head_t * queue_head,
                                                           fbe_bool_t abort_peer,
                                                           fbe_payload_stripe_lock_operation_t ** abort_sl,
                                                           fbe_queue_head_t *tmp_queue,
                                                           fbe_bool_t * is_cmi_required)
{
    fbe_queue_element_t * queue_element = NULL;
    fbe_queue_element_t * wait_q_element = NULL;
    fbe_payload_stripe_lock_operation_t * sl = NULL;
    fbe_payload_stripe_lock_operation_t * wait_q_sl = NULL;

    queue_element = fbe_queue_front(queue_head);
    while(queue_element != NULL){
        sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
        if ((sl->status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING) && ext_pool_lock_is_monitor_op_abort_needed(mde, sl, abort_peer))
        {
            /* We find the monitor op. */
            fbe_queue_remove(&sl->queue_element);
            sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
			if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
				fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
			}

            ext_pool_lock_dispatch(sl, tmp_queue, is_cmi_required);
            *abort_sl = sl;
            return FBE_STATUS_OK;
        }
        else
        {
            /* The sl is not to be aborted. Check wait_queue. */
            wait_q_element = fbe_queue_front(&sl->wait_queue);
            while (wait_q_element)
            {
                wait_q_sl = fbe_metadata_stripe_lock_queue_element_to_operation(wait_q_element);
                if (ext_pool_lock_is_monitor_op_abort_needed(mde, wait_q_sl, abort_peer))
                {
                    fbe_queue_remove(&wait_q_sl->queue_element);
                    wait_q_sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
					if(wait_q_sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
						fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
					}

                    *abort_sl = wait_q_sl;
                    return FBE_STATUS_OK;
                }
                wait_q_element = fbe_queue_next(&sl->wait_queue, wait_q_element); 
            }
            //if (*abort_sl) {break;}
        }
        queue_element = fbe_queue_next(queue_head, queue_element); 
    }
    sl = NULL; /* To prevent future access */

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_ext_pool_lock_element_abort_monitor_ops_callback(fbe_metadata_element_t* mde,
                                                     fbe_ext_pool_lock_slice_entry_t * entry, 
                                                     void * context)
{
    ext_pool_lock_callback_context_t *context_p = (ext_pool_lock_callback_context_t *)context;
    fbe_bool_t *abort_peer = context_p->param_1;
    fbe_payload_stripe_lock_operation_t ** abort_sl = context_p->param_2;
    fbe_queue_head_t *tmp_queue = context_p->param_3; 
    fbe_bool_t *is_cmi_required = context_p->param_4;

    fbe_ext_pool_lock_element_abort_monitor_op_in_queue(mde, &entry->head, *abort_peer, abort_sl, tmp_queue, is_cmi_required);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_ext_pool_lock_element_abort_monitor_ops(fbe_metadata_element_t * mde, fbe_bool_t abort_peer)
{
    fbe_queue_head_t tmp_queue;    
    fbe_payload_stripe_lock_operation_t * abort_sl = NULL;
    fbe_bool_t is_cmi_required = FBE_FALSE;
    fbe_object_id_t object_id;
    //fbe_u32_t i;
    ext_pool_lock_callback_context_t context;

    if(mde->metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        return FBE_STATUS_OK;
    }

    if(!abort_peer && mde->metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE){
        return FBE_STATUS_OK;
    }

    object_id = fbe_base_config_metadata_element_to_object_id(mde);

    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "SL: Abort monitor ops for object_id 0x%x\n", object_id);

    fbe_queue_init(&tmp_queue);	

    fbe_ext_pool_lock_all(mde);

    context.param_1 = &abort_peer;
    context.param_2 = &abort_sl;
    context.param_3 = &tmp_queue;
    context.param_4 = &is_cmi_required;
    fbe_ext_pool_lock_traverse_hash_table(mde, fbe_ext_pool_lock_element_abort_monitor_ops_callback, &context);

    fbe_ext_pool_unlock_all(mde);

    if (abort_sl){
        metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "SL: Abort monitor ops object_id 0x%x sl %p\n", object_id, abort_sl);

        if (abort_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) {
            metadata_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "SL: Abort monitor ops WAIT_FOR_PEER object_id 0x%x\n", object_id);
        }

        if (!abort_peer) {
            fbe_transport_record_callback_with_action(abort_sl->cmi_stripe_lock.packet, (fbe_packet_completion_function_t)fbe_ext_pool_lock_element_abort_monitor_ops, PACKET_SL_ABORT);
            fbe_payload_stripe_lock_set_status(abort_sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);
            fbe_transport_set_status(abort_sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
            if (fbe_queue_is_element_on_queue(&abort_sl->cmi_stripe_lock.packet->queue_element)){
                metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
			                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
			                   "%s stripe lock %p packet already on the queue.\n", __FUNCTION__, abort_sl);
            }
            fbe_queue_push(&tmp_queue, &abort_sl->cmi_stripe_lock.packet->queue_element);
        } else {
            fbe_payload_stripe_lock_set_status(abort_sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);
            fbe_ext_pool_send_aborted_msg(mde, abort_sl);
        }
    }

    /* Abort everything from the temporary list. */
    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
    fbe_queue_destroy(&tmp_queue);    

	ext_pool_lock_reschedule_blob(mde, is_cmi_required, FBE_FALSE);

    return FBE_STATUS_OK;
}

#ifdef EXT_POOL_LOCK
fbe_status_t 
ext_pool_lock_update(fbe_metadata_element_t * mde)
{
    fbe_metadata_stripe_lock_blob_t * blob = NULL;
	fbe_u32_t blob_size;
    blob = mde->stripe_lock_blob;
	blob_size = (blob->size - 2) * METADATA_STRIPE_LOCK_SLOTS_PER_BYTE;

	blob->private_slot = (mde->stripe_lock_number_of_stripes - mde->stripe_lock_number_of_private_stripes - 1);
	blob->nonpaged_slot = (mde->stripe_lock_number_of_stripes - 1);
	blob->user_slot_size = ((blob->private_slot + 1) / blob_size) + 1;

	/* If we have stripe_lock_hash initialized we will do the math differently */
	if(mde->stripe_lock_hash != NULL){
		fbe_u64_t dd = mde->stripe_lock_hash->default_divisor;
		fbe_u64_t ndd = (blob->private_slot + 1) / dd; /* We have that many default divisors */
		fbe_u64_t sndd; /* Number of divisors per slot */

		/* If not aligned and we need to add one more divisor */
		if((blob->private_slot + 1) % dd){ /* Not aligned and we need to add one more divisor */
			ndd++;
		}

		/* How many divisors will we get per slot */
		sndd = ndd / blob_size;
		/* If not aligned and we need to add one more divisor per slot */
		if(ndd % blob_size){
			sndd++;
		}
		/* So each slot will contain sndd od default divisors, lets translate it back to stripe locks */
		blob->user_slot_size = sndd * dd;
		/* We also have to update hash divisor so it will be the same as slot_size */
		mde->stripe_lock_hash->divisor = blob->user_slot_size;
	}

	blob->write_count = 0;
	mde->stripe_lock_large_io_count = 0;
    return FBE_STATUS_OK;
}
#endif

static fbe_status_t 
ext_pool_lock_start(fbe_packet_t *  packet)
{   
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_stripe_lock_operation_t * sl = NULL;
    fbe_metadata_element_t * mde = NULL;
    fbe_metadata_stripe_lock_blob_t * blob = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    sl = fbe_payload_ex_get_stripe_lock_operation(sep_payload);
    mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;
	blob = mde->stripe_lock_blob;

#if 0
    /* Sanity check for slot alignment */
    if (((fbe_u64_t)&(blob->slot[0]) % 8) != 0) {
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s blob slots not aligned!\n", __FUNCTION__);
    }

	for (i = 0; i < METADATA_STRIPE_LOCK_SLOTS_PER_BYTE; i++){
		el_byte |= (METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL << (i * METADATA_STRIPE_LOCK_BITS_PER_SLOT));
		ep_byte |= (METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_PEER << (i * METADATA_STRIPE_LOCK_BITS_PER_SLOT));
	}
	if(mde->metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE){
		fbe_set_memory(blob->slot, el_byte, blob->size); /* Exclusive local lock */
	} else {
		fbe_set_memory(blob->slot, ep_byte, blob->size); /* Exclusive peer lock */
	}
#endif

	if(!(mde->stripe_lock_blob->flags & METADATA_STRIPE_LOCK_BLOB_FLAG_STARTED)){
		mde->stripe_lock_blob->flags |= METADATA_STRIPE_LOCK_BLOB_FLAG_STARTED;

		fbe_queue_element_init(&blob->queue_element);
		fbe_queue_init(&blob->peer_sl_queue);
		fbe_spinlock_init(&blob->peer_sl_queue_lock);
	}

	blob->mde = mde;
    /* Do all the calculations.
     */
    //ext_pool_lock_update(mde);

    fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
ext_pool_lock_stop(fbe_packet_t *  packet)
{   
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_stripe_lock_operation_t * sl = NULL;
    fbe_metadata_element_t * mde = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    sl = fbe_payload_ex_get_stripe_lock_operation(sep_payload);
    mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;

	mde->stripe_lock_blob->flags &= ~METADATA_STRIPE_LOCK_BLOB_FLAG_STARTED;

	mde->stripe_lock_blob = NULL;

    fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;    
}


fbe_status_t 
fbe_ext_pool_lock_cmi_dispatch(fbe_metadata_element_t * metadata_element, 
									  fbe_metadata_cmi_message_t * metadata_cmi_msg,
									  fbe_u32_t cmi_message_length)
{

    switch(metadata_cmi_msg->header.metadata_cmi_message_type){
        case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_LOCK_START:
            if(!fbe_ext_pool_lock_is_started(metadata_element)){
                //ext_pool_lock_allocate_blob(metadata_element);
            }
            break;

        case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_LOCK_STOP:
            fbe_ext_pool_lock_release_all_blobs(metadata_element);
            break;

        case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_LOCK:
        case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_LOCK:
			fbe_ext_pool_lock_msg(metadata_element, metadata_cmi_msg, cmi_message_length);
            break;

		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_RELEASE:
			fbe_ext_pool_release_msg(metadata_element, metadata_cmi_msg, cmi_message_length);
			break;

		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_ABORTED:
			fbe_ext_pool_aborted_msg(metadata_element, metadata_cmi_msg, cmi_message_length);
			break;

        case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_GRANT:
        case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_GRANT:
			fbe_ext_pool_grant_msg(metadata_element, metadata_cmi_msg, cmi_message_length);	
			break;
        default:
            metadata_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Uknown message type %d \n", __FUNCTION__ , metadata_cmi_msg->header.metadata_cmi_message_type);
            break;

    }
    return FBE_STATUS_OK;
} 

fbe_bool_t 
fbe_ext_pool_lock_is_started(fbe_metadata_element_t * mde)
{
    fbe_bool_t is_started = FBE_FALSE;

    fbe_spinlock_lock(&mde->stripe_lock_spinlock);
    //if(mde->stripe_lock_blob->flags & METADATA_STRIPE_LOCK_BLOB_FLAG_STARTED){
	if(mde->stripe_lock_blob != NULL){
        is_started = FBE_TRUE;
    } else {
        is_started = FBE_FALSE;
    }
    fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
    return is_started;
}

fbe_status_t 
fbe_ext_pool_lock_control_entry(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_stripe_lock_operation_t *   stripe_lock_operation = NULL;
    fbe_metadata_control_debug_lock_t * debug_lock = NULL;
    fbe_u32_t length;
    fbe_status_t status;
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t *                 new_sep_payload = NULL;
    fbe_cpu_id_t cpu_id;
	fbe_metadata_element_t * mde = NULL;

    /* validating the cpu_id */
    fbe_transport_get_cpu_id(packet, &cpu_id);
    if(cpu_id == FBE_CPU_ID_INVALID){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s Invalid cpu_id in packet!\n", __FUNCTION__);
    }

    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);

    fbe_payload_control_get_buffer(control_operation, &debug_lock);
    if (debug_lock == NULL) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_metadata_control_debug_lock_t)) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(debug_lock->opcode) {
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_UNLOCK:
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK:
            new_packet = debug_lock->packet_ptr;
            new_sep_payload = fbe_transport_get_payload_ex(new_packet);
            stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(new_sep_payload);
            stripe_lock_operation->opcode = debug_lock->opcode;
            //stripe_lock_operation->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_INITIALIZED;
            break;
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK:
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK:
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_START:
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_STOP:

            /* We have to allocate new packet */
            new_packet = fbe_transport_allocate_packet();
            fbe_transport_initialize_packet(new_packet);
            new_sep_payload = fbe_transport_get_payload_ex(new_packet);

            /* Allocate stripe_lock operation */
            stripe_lock_operation = fbe_payload_ex_allocate_stripe_lock_operation(new_sep_payload);

            stripe_lock_operation->opcode = debug_lock->opcode;     
            stripe_lock_operation->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_INITIALIZED;
            //stripe_lock_operation->b_allow_hold = debug_lock->b_allow_hold; 
            stripe_lock_operation->flags = 0;

            fbe_metadata_get_element_by_object_id( debug_lock->object_id, &mde);
			stripe_lock_operation->cmi_stripe_lock.header.metadata_element_sender = (fbe_u64_t)mde;

            stripe_lock_operation->stripe.last = debug_lock->stripe_number + debug_lock->stripe_count - 1;
            stripe_lock_operation->stripe.first = debug_lock->stripe_number;
            fbe_queue_element_init(&stripe_lock_operation->queue_element);
            debug_lock->packet_ptr = new_packet;
            break;
        default:
            /* coverity fix: new_packet will be null in the default case, we log an error and return.*/
            metadata_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: opcode 0x%x not supported!\n",
                           __FUNCTION__,
                           debug_lock->opcode);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
    }

            
    fbe_payload_ex_increment_stripe_lock_operation_level(new_sep_payload);

    fbe_transport_add_subpacket(packet, new_packet);

    /* Set the completion function before sending request. */
    fbe_transport_set_completion_function(new_packet, fbe_ext_pool_lock_control_entry_completion, debug_lock);                           

    /* Send a request to allocate the stripe lock. */
    status = fbe_ext_pool_lock_operation_entry(new_packet);
    return status;
}

static fbe_status_t
fbe_ext_pool_lock_control_entry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{    
    fbe_payload_ex_t *                     sep_payload = NULL;
    fbe_payload_stripe_lock_operation_t *   stripe_lock_operation = NULL;
    fbe_payload_stripe_lock_status_t        stripe_lock_status;
    fbe_metadata_control_debug_lock_t * debug_lock = NULL;
    fbe_packet_t * master_packet = NULL;

    master_packet = fbe_transport_get_master_packet(packet);

    fbe_transport_remove_subpacket(packet);

    debug_lock = (fbe_metadata_control_debug_lock_t *)context;

    sep_payload = fbe_transport_get_payload_ex(packet);

    /* Get the stripe lock operation from the sep payload. */
    stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(sep_payload);

    fbe_payload_stripe_lock_get_status(stripe_lock_operation, &stripe_lock_status);

    switch(stripe_lock_operation->opcode) {
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_UNLOCK:
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK:
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_START:
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_STOP:
            /* Release the stripe lock operation. */
            fbe_payload_ex_release_stripe_lock_operation(sep_payload, stripe_lock_operation);
            /* Release packet */
            fbe_transport_release_packet(packet);
            break;
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK:
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK:

            break;
    }


    debug_lock->status = stripe_lock_status;

    /* Set the status code for the master packet.*/
    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);

    /* Complete the master packet.*/
    fbe_transport_complete_packet(master_packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

fbe_status_t 
ext_pool_lock_drop_wait_queue_peer_requests(fbe_payload_stripe_lock_operation_t * sl)
{
    fbe_queue_element_t * wait_queue_element = NULL;
	fbe_payload_stripe_lock_operation_t * wait_sl = NULL;
	fbe_metadata_element_t	* mde = NULL;	

    wait_queue_element = fbe_queue_front(&sl->wait_queue);
    while (wait_queue_element != NULL) {
        wait_sl = fbe_metadata_stripe_lock_queue_element_to_operation(wait_queue_element);
        wait_queue_element = fbe_queue_next(&sl->wait_queue, wait_queue_element); 
        /* drop any peer request */
        if (wait_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) {
            metadata_trace(FBE_TRACE_LEVEL_INFO,
    		                FBE_TRACE_MESSAGE_ID_INFO,
			                "SL: drop peer sl: %p rsl:%p id: %x, held by %p first %llX last %llX \n",
                            wait_sl, 
                            sl->cmi_stripe_lock.request_sl_ptr,  
                            sl->cmi_stripe_lock.header.object_id, wait_sl,
                            wait_sl->stripe.first, wait_sl->stripe.last);
            fbe_queue_remove(&wait_sl->queue_element);
            wait_sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
            if(wait_sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
                mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)wait_sl->cmi_stripe_lock.header.metadata_element_sender;
                fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
            }
            fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
            fbe_queue_push(&ext_pool_lock_peer_sl_queue, &wait_sl->queue_element);
            ext_pool_lock_peer_sl_queue_count --;
            fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
        }
    }

	return FBE_STATUS_OK;
}

fbe_status_t 
ext_pool_lock_drop_peer_request(fbe_payload_stripe_lock_operation_t * sl, fbe_queue_head_t * tmp_queue)
{
	fbe_bool_t is_cmi_required = FBE_FALSE;
	fbe_metadata_element_t	* mde = NULL;	

    fbe_queue_remove(&sl->queue_element);
	sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
	if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
		mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;
		fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
	}

	ext_pool_lock_dispatch(sl, tmp_queue, &is_cmi_required);

	/* just release the sl */
	fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
	fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);    
	ext_pool_lock_peer_sl_queue_count--;
	fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_ext_pool_lock_peer_lost(void)
{
    fbe_queue_element_t		* queue_element = NULL;
    fbe_metadata_element_t	* mde = NULL;
    fbe_queue_head_t		* metadata_element_queue_head = NULL;
    fbe_queue_head_t tmp_queue;
    fbe_metadata_stripe_lock_blob_t * blob = NULL;  
    fbe_queue_element_t * sl_queue_element = NULL;
    fbe_payload_stripe_lock_operation_t * sl = NULL;
    fbe_bool_t is_cmi_required = FBE_FALSE;

    fbe_queue_init(&tmp_queue);

    fbe_metadata_element_queue_lock();

    fbe_metadata_get_element_queue_head(&metadata_element_queue_head);
    queue_element = fbe_queue_front(metadata_element_queue_head);

    while (queue_element != NULL){
        mde = fbe_metadata_queue_element_to_metadata_element(queue_element);

        if (mde->stripe_lock_blob == NULL) { /* LUN for example */
            queue_element = fbe_queue_next(metadata_element_queue_head, queue_element);
            continue;
        }

        //fbe_spinlock_lock(&mde->stripe_lock_spinlock);
		fbe_ext_pool_lock_all(mde);

        blob = mde->stripe_lock_blob;

		fbe_spinlock_lock(&blob->peer_sl_queue_lock);

#ifdef EXT_POOL_LOCK
       /* Free the paged and Non-paged Slots */
        ext_pool_lock_get_paged_slots(mde, &first_slot, &last_slot);
        for(i = first_slot; i<= last_slot; i++) {
            ext_pool_lock_set_slot_state(mde, i, FBE_FALSE, METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL);
        }

        ext_pool_lock_get_non_paged_slots(mde, &first_slot, &last_slot);
        for(i = first_slot; i<= last_slot; i++) {
            ext_pool_lock_set_slot_state(mde, i, FBE_FALSE, METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL);
        }
#endif

		/* Fix for AR 557739 - Drop ALL peer requests from the blob */
		/* FBE_METADATA_ELEMENT_ATTRIBUTES_SL_PEER_LOST may be cleared too soon by:
		fbe_raid_group_monitor_journal_flush_cond_function calling fbe_metadata_element_clear_sl_peer_lost function
		*/
		/* It is safe to get a spin over peer_sl queue of the blob and drop all peer requests */
		sl_queue_element = fbe_queue_front(&blob->peer_sl_queue);
		while(sl_queue_element != NULL){
            sl = fbe_metadata_stripe_lock_queue_element_to_operation(sl_queue_element);
			sl_queue_element = fbe_queue_next(&blob->peer_sl_queue, sl_queue_element); 
			if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST){
					fbe_queue_remove(&sl->queue_element);
					 /* Just free the request, we don't need it if the peer is dead */
					fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
					fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);  
					ext_pool_lock_peer_sl_queue_count--;
					fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
			}
		}

		/* This function will iterate throu all hash queues if needed */
		fbe_ext_pool_lock_peer_lost_mde(mde, &tmp_queue, &is_cmi_required);

		fbe_spinlock_unlock(&blob->peer_sl_queue_lock);

        //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
		fbe_ext_pool_unlock_all(mde);
        queue_element = fbe_queue_next(metadata_element_queue_head, queue_element);
    } /* while (queue_element != NULL) */
    
    fbe_metadata_element_queue_unlock();

    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
    fbe_queue_destroy(&tmp_queue);
    
    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_ext_pool_grant_msg(fbe_metadata_element_t * mde, fbe_metadata_cmi_message_t * msg, fbe_u32_t cmi_message_length)
{
	fbe_metadata_cmi_stripe_lock_t * cmi_stripe_lock = (fbe_metadata_cmi_stripe_lock_t *)msg;
    fbe_payload_stripe_lock_operation_t * sl = NULL;
	fbe_queue_head_t tmp_queue;

	fbe_queue_init(&tmp_queue);
	sl = cmi_stripe_lock->request_sl_ptr;

    //fbe_spinlock_lock(&mde->stripe_lock_spinlock);
	fbe_ext_pool_lock_hash(mde, sl);
	
	/* The flags field is valid */
	if(!(cmi_stripe_lock->flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE)){
	    /* Mark slots */
	    ext_pool_lock_mark_grant_from_peer(mde, cmi_stripe_lock, sl);
	} else {
#if 0
		metadata_trace(FBE_TRACE_LEVEL_INFO,
    	    			FBE_TRACE_MESSAGE_ID_INFO,
						"SL: GRANT NR rsl:%p grl: %p \n", cmi_stripe_lock->request_sl_ptr,  cmi_stripe_lock->grant_sl_ptr);
#endif

	}
	/* We have a grant for specific SL */
	
	sl->cmi_stripe_lock.grant_sl_ptr = cmi_stripe_lock->grant_sl_ptr;
	sl->cmi_stripe_lock.flags = cmi_stripe_lock->flags;

	if(!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION) && !(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_DEAD_LOCK)){ 
			metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
				FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"SL: Illegal flags 0x%X \n",sl->flags);		
	}

	sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;

	if(!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) && !(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_DEAD_LOCK)){ 
			metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
				FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"SL: Illegal flags 0x%X \n",sl->flags);				
	}

	sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER;                

	if(sl->status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING){ 
			metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
				FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"SL: Illegal status 0x%X \n",sl->status);		
	}

	if(!(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT)){ /* If it is not localy granted */
#if 0
		metadata_trace(FBE_TRACE_LEVEL_INFO,
    	    			FBE_TRACE_MESSAGE_ID_INFO,
						"SL: GRANT DEAD_LOCK rsl:%p grl: %p \n", cmi_stripe_lock->request_sl_ptr,  cmi_stripe_lock->grant_sl_ptr);
#endif

		//sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_DEAD_LOCK;
		//fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
		fbe_ext_pool_unlock_hash(mde, sl);
		return FBE_STATUS_OK;		
	}

	// Sanity testing
	//if(!ext_pool_lock_check_peer(sl)){ fbe_debug_break(); }

    fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
    fbe_transport_set_status(sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
    if (fbe_queue_is_element_on_queue(&sl->cmi_stripe_lock.packet->queue_element)){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
		                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                "%s stripe lock %p packet already on the queue.\n", __FUNCTION__, sl);
    }
    fbe_queue_push(&tmp_queue, &sl->cmi_stripe_lock.packet->queue_element);

	//fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
	fbe_ext_pool_unlock_hash(mde, sl);

	fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
	fbe_queue_destroy(&tmp_queue);

	return FBE_STATUS_OK;		
	
}

static fbe_status_t 
fbe_ext_pool_lock_msg(fbe_metadata_element_t * mde, fbe_metadata_cmi_message_t * msg, fbe_u32_t cmi_message_length)
{
	fbe_payload_stripe_lock_operation_t * peer_sl = NULL;
	fbe_queue_element_t * queue_element = NULL;
	fbe_metadata_stripe_lock_blob_t * blob = NULL;
	fbe_metadata_cmi_stripe_lock_t * cmi_stripe_lock = (fbe_metadata_cmi_stripe_lock_t *)msg;

	blob = mde->stripe_lock_blob;

	/* Allocate peer_sl */
	fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
	queue_element = fbe_queue_pop(&ext_pool_lock_peer_sl_queue);	
	ext_pool_lock_peer_sl_queue_count++;
	fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);

	/* Fill in the peer_sl */
	peer_sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
	peer_sl->cmi_stripe_lock.header.object_id = fbe_base_config_metadata_element_to_object_id(mde);

	if(cmi_stripe_lock->header.metadata_cmi_message_type == FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_LOCK){
		peer_sl->priv_flags = FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ | FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_PENDING;
		/* expand region to slot */
#ifdef EXT_POOL_LOCK
		/* User region */
		if(cmi_stripe_lock->read_region.first < blob->private_slot){		
			peer_sl->cmi_stripe_lock.read_region.first = (cmi_stripe_lock->read_region.first / blob->user_slot_size) * blob->user_slot_size;
			peer_sl->cmi_stripe_lock.read_region.last = ((cmi_stripe_lock->read_region.last / blob->user_slot_size) + 1) * blob->user_slot_size - 1;
			peer_sl->cmi_stripe_lock.read_region.last = FBE_MIN(peer_sl->cmi_stripe_lock.read_region.last, (blob->private_slot - 1));
			if (fbe_metadata_is_ndu_in_progress()){
				peer_sl->cmi_stripe_lock.flags |= FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE;
			}
		} 
		/* Paged slot */
        else if (cmi_stripe_lock->read_region.first >= blob->private_slot && cmi_stripe_lock->read_region.first != blob->nonpaged_slot) {
			peer_sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_PAGED;
            peer_sl->cmi_stripe_lock.read_region.first = blob->private_slot;
			peer_sl->cmi_stripe_lock.read_region.last = blob->private_slot + mde->stripe_lock_number_of_private_stripes - 1;			
        }
		/* Nonpaged slot */
        else { 
			peer_sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_NP;
			peer_sl->cmi_stripe_lock.read_region.first = cmi_stripe_lock->read_region.first;
			peer_sl->cmi_stripe_lock.read_region.last = cmi_stripe_lock->read_region.last;
		}
#endif
        fbe_ext_pool_lock_get_slice_region(mde, cmi_stripe_lock->read_region.first,
            &peer_sl->cmi_stripe_lock.read_region.first, &peer_sl->cmi_stripe_lock.read_region.last);

		fbe_payload_stripe_lock_build_read_lock(peer_sl, mde,	
												cmi_stripe_lock->read_region.first,
                                                cmi_stripe_lock->read_region.last - cmi_stripe_lock->read_region.first + 1);
		
		ext_pool_lock_set_invalid_region(&peer_sl->cmi_stripe_lock.write_region);
#if SL_DEBUG_MODE
		if(peer_sl->cmi_stripe_lock.header.object_id == 0x10c)
		    metadata_trace(FBE_TRACE_LEVEL_INFO,
						    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						    "SL: %p PEER_REQUEST id: %x R: first %llX last %llX\n",peer_sl,
						    peer_sl->cmi_stripe_lock.header.object_id,
						    peer_sl->cmi_stripe_lock.read_region.first, peer_sl->cmi_stripe_lock.read_region.last);
#endif
	} else {/* write lock */
		peer_sl->priv_flags = FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_PENDING;
		/* expand region to slot */
#ifdef EXT_POOL_LOCK
		/* User slot */
		if(cmi_stripe_lock->write_region.first < blob->private_slot){
			peer_sl->cmi_stripe_lock.write_region.first = (cmi_stripe_lock->write_region.first / blob->user_slot_size) * blob->user_slot_size;
			peer_sl->cmi_stripe_lock.write_region.last = ((cmi_stripe_lock->write_region.last / blob->user_slot_size) + 1) * blob->user_slot_size - 1;
			peer_sl->cmi_stripe_lock.write_region.last = FBE_MIN(peer_sl->cmi_stripe_lock.write_region.last, (blob->private_slot - 1));
			if (fbe_metadata_is_ndu_in_progress()){
				peer_sl->cmi_stripe_lock.flags |= FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE;
			}
		}  
        /* Paged slot */ 
        else if (cmi_stripe_lock->write_region.first >= blob->private_slot && cmi_stripe_lock->write_region.first != blob->nonpaged_slot) {
			peer_sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_PAGED;
            peer_sl->cmi_stripe_lock.write_region.first = blob->private_slot;
			peer_sl->cmi_stripe_lock.write_region.last = blob->private_slot + mde->stripe_lock_number_of_private_stripes - 1;
        }
		/* Non paged */
        else {
			peer_sl->priv_flags |= FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_NP;
			peer_sl->cmi_stripe_lock.write_region.first = cmi_stripe_lock->write_region.first;
			peer_sl->cmi_stripe_lock.write_region.last = cmi_stripe_lock->write_region.last;
		}
#endif
        fbe_ext_pool_lock_get_slice_region(mde, cmi_stripe_lock->write_region.first,
            &peer_sl->cmi_stripe_lock.write_region.first, &peer_sl->cmi_stripe_lock.write_region.last);

		fbe_payload_stripe_lock_build_write_lock(peer_sl, mde,	
												cmi_stripe_lock->write_region.first,
                                                cmi_stripe_lock->write_region.last - cmi_stripe_lock->write_region.first + 1);

		ext_pool_lock_set_invalid_region(&peer_sl->cmi_stripe_lock.read_region);
#if SL_DEBUG_MODE
		if(peer_sl->cmi_stripe_lock.header.object_id == 0x10c)
    		metadata_trace(FBE_TRACE_LEVEL_INFO,
	    					FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		    				"SL: %p PEER_REQUEST id: %x W: first %llX last %llX \n",peer_sl,
			    			peer_sl->cmi_stripe_lock.header.object_id,
				    		peer_sl->cmi_stripe_lock.write_region.first, peer_sl->cmi_stripe_lock.write_region.last);
#endif
	}

	peer_sl->status = FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING;

	peer_sl->flags = FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST;
	peer_sl->cmi_stripe_lock.header.metadata_element_receiver = mde->peer_metadata_element_ptr;
	peer_sl->cmi_stripe_lock.header.object_id = fbe_base_config_metadata_element_to_object_id(mde);
	peer_sl->cmi_stripe_lock.header.metadata_element_sender = (fbe_u64_t)mde; 

	/* Check if we have fbe_metadata_cmi_stripe_lock_flags_t (Version 1) */
	if((fbe_u64_t)(&((fbe_metadata_cmi_stripe_lock_t *)0)->flags) + sizeof(fbe_metadata_cmi_stripe_lock_flags_t) <= cmi_message_length){
		peer_sl->cmi_stripe_lock.request_sl_ptr = cmi_stripe_lock->request_sl_ptr;
		peer_sl->cmi_stripe_lock.grant_sl_ptr = peer_sl;
		peer_sl->cmi_stripe_lock.flags = cmi_stripe_lock->flags;

		/* We will disable it soon */
		//peer_sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_FULL_SLOT;

	} else { /* old (Version 0) expects to grant the full slot */
		peer_sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_FULL_SLOT;
	}

	if(peer_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_FULL_SLOT){
		if(cmi_stripe_lock->header.metadata_cmi_message_type == FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_LOCK){
			peer_sl->stripe.first = peer_sl->cmi_stripe_lock.read_region.first;
			peer_sl->stripe.last = peer_sl->cmi_stripe_lock.read_region.last;
		} else {
			peer_sl->stripe.first = peer_sl->cmi_stripe_lock.write_region.first;
			peer_sl->stripe.last = peer_sl->cmi_stripe_lock.write_region.last;
		}
	}

#ifdef EXT_POOL_LOCK
	if(peer_sl->stripe.first < blob->private_slot){
		if(peer_sl->stripe.last > blob->private_slot){
			metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
				FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"SL: User/Private overlap \n");				
		}
	}
#endif

	fbe_queue_init(&peer_sl->wait_queue);
	peer_sl->cmi_stripe_lock.packet = cmi_stripe_lock->packet;

	if (cmi_stripe_lock->flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_MONITOR_OP) {
		peer_sl->cmi_stripe_lock.flags |= FBE_METADATA_CMI_STRIPE_LOCK_FLAG_MONITOR_OP;
	}

	/* When hash is enabled this lock will protect blob->peer_sl_queue */
	fbe_spinlock_lock(&blob->peer_sl_queue_lock);
	//ext_pool_lock_update_write_count(peer_sl);
	fbe_queue_push(&blob->peer_sl_queue, &peer_sl->queue_element);
	fbe_spinlock_unlock(&blob->peer_sl_queue_lock);

	/* Submit it to the peer request Q */
	ext_pool_lock_reschedule_blob(mde, FBE_TRUE, FBE_FALSE);

    return FBE_STATUS_OK;
}

fbe_status_t 
ext_pool_lock_clear_wait_for_peer_requests(fbe_payload_stripe_lock_operation_t * sl)
{
    fbe_queue_element_t * wait_queue_element = NULL;
    fbe_payload_stripe_lock_operation_t * wait_sl = NULL;

    wait_queue_element = fbe_queue_front(&sl->wait_queue);
    while (wait_queue_element != NULL) {
        wait_sl = fbe_metadata_stripe_lock_queue_element_to_operation(wait_queue_element);
        wait_queue_element = fbe_queue_next(&sl->wait_queue, wait_queue_element); 
        /* clear WAIT_FOR_PEER and DEAD_LOCK flag */
        if ((wait_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_DEAD_LOCK) &&
            (wait_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER))
        {
            wait_sl->flags &= ~(FBE_PAYLOAD_STRIPE_LOCK_FLAG_DEAD_LOCK | FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER);
        }
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_ext_pool_lock_release_peer_data_stripe_locks()
 ****************************************************************
 * @brief
 * Release of data stripe locks held by peer. This is a delayed release 
 * of locks held by peerSP after that SP going down. Holding onto locks allows
 * write_log flushes to complete before letting IOs on local SP touch the
 * same stripes.  
 *
 * @param metadata_element - The metadata element to release locks for.             
 *
 * @return fbe_status_t  
 *
 * @author
 *  5/16/2012 - Created. Vamsi V.
 *  06/27/2012 - Modified Peter Puhov
 ****************************************************************/
fbe_status_t 
fbe_ext_pool_lock_release_peer_data_stripe_locks(fbe_metadata_element_t* mde)
{
    fbe_queue_head_t tmp_queue;
	fbe_metadata_stripe_lock_blob_t * blob = NULL;  
	fbe_bool_t is_cmi_required = FBE_FALSE;

    fbe_queue_init(&tmp_queue);

    //fbe_spinlock_lock(&mde->stripe_lock_spinlock);
	fbe_ext_pool_lock_all(mde);

    blob = mde->stripe_lock_blob;
    fbe_atomic_32_and(&mde->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_SL_PEER_LOST);
    
    /* if there is no blob we have no locks to release. */
    if(NULL == blob)
    {
        //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
		fbe_ext_pool_unlock_all(mde);
        return FBE_STATUS_OK;
    }

    /* TODO write log fixes */
    //fbe_set_memory(blob->slot, (fbe_u8_t)METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL, blob->size); /* Exclusive local lock */

	fbe_ext_pool_lock_grant_all_peer_locks(mde, &tmp_queue, &is_cmi_required);

    //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
	fbe_ext_pool_unlock_all(mde);

    if (is_cmi_required) {
        fbe_spinlock_lock(&ext_pool_lock_blob_queue_lock);
        if (!fbe_queue_is_element_on_queue(&mde->stripe_lock_blob->queue_element)) {
            fbe_queue_push(&ext_pool_lock_blob_queue, &mde->stripe_lock_blob->queue_element);
            fbe_rendezvous_event_set(&ext_pool_lock_event);
        }
        fbe_spinlock_unlock(&ext_pool_lock_blob_queue_lock); 
    }

    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
    fbe_queue_destroy(&tmp_queue);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_ext_pool_lock_release_all_blobs(fbe_metadata_element_t * metadata_element)
{

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_ext_pool_lock_message_event(fbe_cmi_event_t event, fbe_metadata_cmi_message_t *metadata_cmi_msg, fbe_cmi_event_callback_context_t context)
{
	fbe_queue_head_t tmp_queue;
    fbe_metadata_element_t * mde = NULL;
	fbe_bool_t is_cmi_required = FBE_FALSE;
	fbe_payload_stripe_lock_operation_t * sl;
	fbe_metadata_cmi_stripe_lock_t * cmi_stripe_lock = (fbe_metadata_cmi_stripe_lock_t *)metadata_cmi_msg;

	fbe_payload_stripe_lock_operation_t * peer_sl = (fbe_payload_stripe_lock_operation_t *)context;
	mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)peer_sl->cmi_stripe_lock.header.metadata_element_sender;

 
	fbe_queue_init(&tmp_queue);

	if(cmi_stripe_lock->header.metadata_cmi_message_type == FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_RELEASE){
#if 0
    	metadata_trace(FBE_TRACE_LEVEL_INFO,
	    	    		FBE_TRACE_MESSAGE_ID_INFO,
						"SL: EVT SR rsl:%p grl: %p \n", cmi_stripe_lock->request_sl_ptr,  cmi_stripe_lock->grant_sl_ptr);
#endif
		fbe_transport_set_status(cmi_stripe_lock->packet, FBE_STATUS_OK, 0);
		fbe_transport_run_queue_push_packet( cmi_stripe_lock->packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
		return FBE_STATUS_OK;
	}

	if(cmi_stripe_lock->header.metadata_cmi_message_type == FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_ABORTED){
#if 0
    	metadata_trace(FBE_TRACE_LEVEL_INFO,
	    	    		FBE_TRACE_MESSAGE_ID_INFO,
						"SL: EVT ABORTED rsl:%p grl: %p \n", cmi_stripe_lock->request_sl_ptr,  cmi_stripe_lock->grant_sl_ptr);
#endif
		fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
		fbe_queue_push(&ext_pool_lock_peer_sl_queue, &peer_sl->queue_element);
		ext_pool_lock_peer_sl_queue_count--;
		fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
		return FBE_STATUS_OK;
	}

   /* If the object associated with the stripe lock has been destroyed
     * just return.
     */
    if (mde->metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        return FBE_STATUS_OK;
    }


	/* Fix for AR 560490 */
	/* If grant failed  we would assume the the peer is gone.
	   We will just drop this request and dispatch the wait queue.
	 */

	sl = (fbe_payload_stripe_lock_operation_t *)peer_sl->wait_queue.next;

	if((cmi_stripe_lock->header.metadata_cmi_message_type == FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_GRANT ||
	    cmi_stripe_lock->header.metadata_cmi_message_type == FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_GRANT) && /* This was a grant */
	   (event != FBE_CMI_EVENT_MESSAGE_TRANSMITTED)){ /* And CMI failed */				

		/* Dispatch it */
		//fbe_spinlock_lock(&mde->stripe_lock_spinlock);
		fbe_ext_pool_lock_hash(mde, sl);
		fbe_queue_remove(&sl->queue_element);
		sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
		if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
			fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
		}

		ext_pool_lock_dispatch(sl, &tmp_queue, &is_cmi_required); 
		//fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
		fbe_ext_pool_unlock_hash(mde, sl);

		fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
		fbe_queue_push(&ext_pool_lock_peer_sl_queue, &peer_sl->queue_element);
		ext_pool_lock_peer_sl_queue_count--;
		fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);
		ext_pool_lock_peer_sl_queue_count --;
		fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);

		ext_pool_lock_reschedule_blob(mde, is_cmi_required, FBE_FALSE);

		if(!fbe_queue_is_empty(&tmp_queue)){
			fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
		}
		fbe_queue_destroy(&tmp_queue);

		return FBE_STATUS_OK;
	}

	/* Fix for AR 545033 - peer lost, peer request without need release flag. */
	/* If we granted SL for peer and NR flag is not set - we should dispatch the SL */
	if(!(cmi_stripe_lock->flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE) && 
		(cmi_stripe_lock->header.metadata_cmi_message_type == FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_GRANT ||
			cmi_stripe_lock->header.metadata_cmi_message_type == FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_GRANT)){

		/* We may want to check that peer_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANTED is set */
		//sl = (fbe_payload_stripe_lock_operation_t *)peer_sl->wait_queue.next;

		//fbe_spinlock_lock(&mde->stripe_lock_spinlock);		
		fbe_ext_pool_lock_hash(mde, sl);
		/* Dispatch it */
		fbe_queue_remove(&sl->queue_element);
		sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
		if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
			fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
		}

		//ext_pool_lock_update_write_count(sl);

		ext_pool_lock_dispatch(sl, &tmp_queue, &is_cmi_required); 
		//fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
		fbe_ext_pool_unlock_hash(mde, sl);

		fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
		fbe_queue_push(&ext_pool_lock_peer_sl_queue, &peer_sl->queue_element);
		ext_pool_lock_peer_sl_queue_count--;
		fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);
		ext_pool_lock_peer_sl_queue_count --;
		fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);

		ext_pool_lock_reschedule_blob(mde, is_cmi_required, FBE_FALSE);

		if(!fbe_queue_is_empty(&tmp_queue)){
			fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
		}
		fbe_queue_destroy(&tmp_queue);

		return FBE_STATUS_OK;
	}
		
	if ((!(peer_sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE)) ||
        (event != FBE_CMI_EVENT_MESSAGE_TRANSMITTED)) {
		/* Release peer_sl */
		if(peer_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANTED){

            if (event != FBE_CMI_EVENT_MESSAGE_TRANSMITTED) {
                metadata_trace(FBE_TRACE_LEVEL_INFO,
	    	    		        FBE_TRACE_MESSAGE_ID_INFO,
						        "SL: SEND GRANTED to peer failed (0x%x). sl: %p rsl:%p grl: %p id: %x \n",
                                event, peer_sl, 
                                peer_sl->cmi_stripe_lock.request_sl_ptr,  
                                peer_sl->cmi_stripe_lock.grant_sl_ptr,
                                peer_sl->cmi_stripe_lock.header.object_id);
            }

			//fbe_spinlock_lock(&mde->stripe_lock_spinlock);
			fbe_ext_pool_lock_hash(mde, sl);
            /* peer lost will drop/dispatch granted peer request with need release flag */
            if ((!(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_PEER_LOST)) &&
                (!(peer_sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE)) )
            {
                //sl = (fbe_payload_stripe_lock_operation_t *)peer_sl->wait_queue.next;
			    /* Dispatch it */
			    fbe_queue_remove(&sl->queue_element);
			    sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
				if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
					fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
				}

			    //ext_pool_lock_update_write_count(sl);

			    ext_pool_lock_dispatch(sl, &tmp_queue, &is_cmi_required); 
			    //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
				fbe_ext_pool_unlock_hash(mde, sl);

			    fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
			    fbe_queue_push(&ext_pool_lock_peer_sl_queue, &peer_sl->queue_element);
			    ext_pool_lock_peer_sl_queue_count--;
			    fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);
                ext_pool_lock_peer_sl_queue_count --;
			    fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
            }
            else
            {
				// Fix for AR 543410
				//fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
				fbe_ext_pool_unlock_hash(mde, sl);

			    fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
			    fbe_queue_push(&ext_pool_lock_peer_sl_queue, &peer_sl->queue_element);
			    ext_pool_lock_peer_sl_queue_count--;
			    fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
            }

		} else { /* This was a request to peer */
            if (event != FBE_CMI_EVENT_MESSAGE_TRANSMITTED) {
                metadata_trace(FBE_TRACE_LEVEL_INFO,
	    	    		        FBE_TRACE_MESSAGE_ID_INFO,
						        "SL: SEND SL request to peer failed (0x%x). sl: %p rsl:%p grl: %p id: %x \n",
                                event, peer_sl, 
                                peer_sl->cmi_stripe_lock.request_sl_ptr,  
                                peer_sl->cmi_stripe_lock.grant_sl_ptr,
                                peer_sl->cmi_stripe_lock.header.object_id);
            }
			fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
			fbe_queue_push(&ext_pool_lock_peer_sl_queue, &peer_sl->queue_element);
			ext_pool_lock_peer_sl_queue_count--;
			fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
		}

		

	} else { /* Peer need to release this grant */
			fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
			fbe_queue_push(&ext_pool_lock_peer_sl_queue, &peer_sl->queue_element);
			ext_pool_lock_peer_sl_queue_count--;
			fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
	}

	ext_pool_lock_reschedule_blob(mde, is_cmi_required, FBE_FALSE);

    if(!fbe_queue_is_empty(&tmp_queue)){
        fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
    }
    fbe_queue_destroy(&tmp_queue);

	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_ext_pool_lock_complete_cancelled_callback(fbe_metadata_element_t * mde,
                                               fbe_ext_pool_lock_slice_entry_t * entry, 
                                               void * context)
{
	ext_pool_lock_callback_context_t *context_p = (ext_pool_lock_callback_context_t *)context;
	fbe_queue_head_t * tmp_queue = (fbe_queue_head_t *)(context_p->param_1);
	fbe_bool_t * is_cmi_required = (fbe_bool_t *)(context_p->param_2);
	fbe_payload_stripe_lock_operation_t * sl = NULL;
	fbe_queue_element_t * current_element = NULL;
	fbe_queue_head_t * queue;

	ext_pool_lock_hash_table_entry(entry);

	queue = &entry->head;

	/* find our cancelled SL on the queue */
	current_element = fbe_queue_front(queue);
	while(current_element != NULL){
		sl = fbe_metadata_stripe_lock_queue_element_to_operation(current_element);
		ext_pool_lock_complete_cancelled_queue(&sl->wait_queue, tmp_queue);

		if(!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && /* if it is not peer request */
				(sl->status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) && /* We may want to check that status != OK instead */
					!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION) &&  /* The grant will come for this packet */
					!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) && /* The sl is waiting for peer */
					!(sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE) && /* We do not need to release the peer */
					fbe_transport_is_packet_cancelled(sl->cmi_stripe_lock.packet)) { /* Check if packet cancelled */
								
				fbe_queue_remove(&sl->queue_element);
				sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
				if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
					fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
				}

				ext_pool_lock_dispatch(sl, tmp_queue, is_cmi_required); 
				/* After dispatch we need to start from the front */
				current_element = fbe_queue_front(queue);

				fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_CANCELLED);
				fbe_transport_set_status(sl->cmi_stripe_lock.packet, FBE_STATUS_CANCELED, 0);
				//ext_pool_lock_update_write_count(sl);
				if (fbe_queue_is_element_on_queue(&sl->cmi_stripe_lock.packet->queue_element)){
					metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s stripe lock %p packet already on the queue.\n", __FUNCTION__, sl);
				}
				fbe_queue_push(tmp_queue, &sl->cmi_stripe_lock.packet->queue_element);
				continue;
		}
		current_element = fbe_queue_next(queue, current_element); 
	} /* while(current_element != NULL) */


	ext_pool_unlock_hash_table_entry(entry);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_metadata_element_ext_pool_lock_complete_cancelled(fbe_metadata_element_t * mde)
{
	fbe_queue_head_t tmp_queue;
	fbe_bool_t is_cmi_required = FBE_FALSE;
	ext_pool_lock_callback_context_t context;

	fbe_queue_init(&tmp_queue);

	context.param_1 = &tmp_queue;
	context.param_2 = &is_cmi_required;
	fbe_ext_pool_lock_traverse_hash_table(mde, fbe_ext_pool_lock_complete_cancelled_callback, &context);

	ext_pool_lock_reschedule_blob(mde, is_cmi_required, FBE_FALSE);

	fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
	fbe_queue_destroy(&tmp_queue);

	return FBE_STATUS_OK;
}

static fbe_status_t 
ext_pool_lock_complete_cancelled_queue(fbe_queue_head_t * queue, fbe_queue_head_t * tmp_queue)
{
	fbe_payload_stripe_lock_operation_t * sl = NULL;
	fbe_queue_element_t * current_element = NULL;
	fbe_metadata_element_t	* mde = NULL;

	/* find our cancelled SL on the queue */
	current_element = fbe_queue_front(queue);
	while(current_element != NULL){
		sl = fbe_metadata_stripe_lock_queue_element_to_operation(current_element);
		if(!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && /* if it is not peer request */
				(sl->status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) && /* We may want to check that status != OK instead */
				!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION) && /* The grant will come for this packet */
				!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) && /* The sl is waiting for peer */
				!(sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE) && /* We do not need to release the peer */
					fbe_transport_is_packet_cancelled(sl->cmi_stripe_lock.packet)) { /* Check if packet cancelled */
								
				fbe_queue_remove(&sl->queue_element);
				sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
				if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
					mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;
					fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
				}

#if 0 /* Not a tree nothing to dispatch */
				ext_pool_lock_dispatch(sl, &tmp_queue); 
				/* After dispatch we need to start from the front */
				current_element = fbe_queue_front(queue);
#endif
				fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_CANCELLED);
				fbe_transport_set_status(sl->cmi_stripe_lock.packet, FBE_STATUS_CANCELED, 0);
				//ext_pool_lock_update_write_count(sl);
                if (fbe_queue_is_element_on_queue(&sl->cmi_stripe_lock.packet->queue_element)){
                    metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
					                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					                "%s stripe lock %p packet already on the queue.\n", __FUNCTION__, sl);
                }
				fbe_queue_push(tmp_queue, &sl->cmi_stripe_lock.packet->queue_element);
		}
		current_element = fbe_queue_next(queue, current_element); 
	}

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_ext_pool_release_msg(fbe_metadata_element_t * mde, fbe_metadata_cmi_message_t * msg, fbe_u32_t cmi_message_length)
{
	fbe_payload_stripe_lock_operation_t * sl = NULL;
	fbe_metadata_cmi_stripe_lock_t * cmi_stripe_lock = (fbe_metadata_cmi_stripe_lock_t *)msg;    
    fbe_queue_head_t tmp_queue;
	fbe_bool_t is_cmi_required = FBE_FALSE;

    fbe_queue_init(&tmp_queue);    

	sl = cmi_stripe_lock->grant_sl_ptr;

	/* We need to release paged blobs here */
    if(ext_pool_lock_is_paged_lock(sl)){
        fbe_metadata_paged_release_blobs(mde, sl->stripe.first, sl->stripe.last - sl->stripe.first + 1);
    }

	//fbe_spinlock_lock(&mde->stripe_lock_spinlock);
	fbe_ext_pool_lock_hash(mde, sl);
	if(sl->opcode == FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK){
		sl->opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK;
	} else {
		sl->opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_UNLOCK;
	}

	//ext_pool_lock_update_write_count(sl);
    fbe_queue_remove(&sl->queue_element);
	sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
	if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
		fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
	}

	fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
	ext_pool_lock_dispatch(sl, &tmp_queue, &is_cmi_required); 
    //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
	fbe_ext_pool_unlock_hash(mde, sl);

	fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
	fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);
	ext_pool_lock_peer_sl_queue_count--;
	fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);

    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);	

    fbe_queue_destroy(&tmp_queue);

	ext_pool_lock_reschedule_blob(mde, is_cmi_required, FBE_FALSE);

	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_ext_pool_aborted_msg(fbe_metadata_element_t * mde, fbe_metadata_cmi_message_t * msg, fbe_u32_t cmi_message_length)
{
	fbe_payload_stripe_lock_operation_t * sl = NULL;
	fbe_metadata_cmi_stripe_lock_t * cmi_stripe_lock = (fbe_metadata_cmi_stripe_lock_t *)msg;    
    fbe_queue_head_t tmp_queue;
	fbe_bool_t is_cmi_required = FBE_FALSE;

    fbe_queue_init(&tmp_queue);    

	sl = cmi_stripe_lock->request_sl_ptr;

	//fbe_spinlock_lock(&mde->stripe_lock_spinlock);
	fbe_ext_pool_lock_hash(mde, sl);

	/* If SL got abborted due to destroy state we will do NOTHING.
		We would expect peer lost to come soon
	*/

	if(cmi_stripe_lock->flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_ABORT_DESTROY){
        metadata_trace(FBE_TRACE_LEVEL_INFO, 
		                FBE_TRACE_MESSAGE_ID_INFO,
						"SL: ABORT_DESTROY object_id: 0x%X sl: %p  \n", fbe_base_config_metadata_element_to_object_id(mde), sl);
	
        // AR 554898: Need to clear the _WAITING_FOR_PEER flag since we are in the destroying state.	
        sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER;
        sl->cmi_stripe_lock.flags |= FBE_METADATA_CMI_STRIPE_LOCK_FLAG_ABORT_DESTROY;

		//fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
		fbe_ext_pool_unlock_hash(mde, sl);
		return FBE_STATUS_OK;
	}

	fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);
	//ext_pool_lock_update_write_count(sl);

	/* need to set packet status here, otherwise, fbe_provision_drive_utils_process_block_status() will complain. */
	fbe_transport_set_status(sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);

    fbe_queue_remove(&sl->queue_element);
	sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
	if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
		fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
	}

	sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER;
	sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;

    if (fbe_queue_is_element_on_queue(&sl->cmi_stripe_lock.packet->queue_element)){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
		                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                "%s stripe lock %p packet already on the queue.\n", __FUNCTION__, sl);
    }
	fbe_queue_push(&tmp_queue, &sl->cmi_stripe_lock.packet->queue_element);
	
	ext_pool_lock_dispatch(sl, &tmp_queue, &is_cmi_required); 
    //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
	fbe_ext_pool_unlock_hash(mde, sl);

    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);	

    fbe_queue_destroy(&tmp_queue);

	ext_pool_lock_reschedule_blob(mde, is_cmi_required, FBE_FALSE);

	return FBE_STATUS_OK;
}


static fbe_status_t 
fbe_ext_pool_send_aborted_msg(fbe_metadata_element_t * mde, fbe_payload_stripe_lock_operation_t * sl)
{
	if(fbe_metadata_is_peer_object_alive(mde)){
		sl->cmi_stripe_lock.header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_ABORTED;
		sl->cmi_stripe_lock.header.metadata_element_receiver = mde->peer_metadata_element_ptr;
		sl->cmi_stripe_lock.header.metadata_element_sender = (fbe_u64_t)mde; 
		sl->cmi_stripe_lock.header.object_id = fbe_base_config_metadata_element_to_object_id(mde);

		/* We need to know if we aborting due to destory or not */
		if(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_DESTROY_ABORT_STRIPE_LOCKS){
			sl->cmi_stripe_lock.flags |= FBE_METADATA_CMI_STRIPE_LOCK_FLAG_ABORT_DESTROY;
		}
		fbe_metadata_cmi_send_message((fbe_metadata_cmi_message_t *)&sl->cmi_stripe_lock, sl); 
	}
    else
    {
#if 0
    	metadata_trace(FBE_TRACE_LEVEL_INFO,
	    	    		FBE_TRACE_MESSAGE_ID_INFO,
						"SL: EVT ABORTED no peer:%p grl: %p \n", sl->cmi_stripe_lock.request_sl_ptr,  sl->cmi_stripe_lock.grant_sl_ptr);
#endif
		fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
		fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);
		ext_pool_lock_peer_sl_queue_count--;
		fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
	}

	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_ext_pool_send_release_msg(fbe_metadata_element_t * mde, fbe_payload_stripe_lock_operation_t * sl)
{
	if(fbe_metadata_is_peer_object_alive(mde)){
		sl->cmi_stripe_lock.header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_RELEASE;
		sl->cmi_stripe_lock.header.metadata_element_receiver = mde->peer_metadata_element_ptr;
		sl->cmi_stripe_lock.header.object_id = fbe_base_config_metadata_element_to_object_id(mde);
		sl->cmi_stripe_lock.request_sl_ptr = sl;
#if 0
    	metadata_trace(FBE_TRACE_LEVEL_INFO,
	    	    		FBE_TRACE_MESSAGE_ID_INFO,
						"SL: SEND RELEASE sl: %p rsl:%p grl: %p id: %x \n",sl, 
																		sl->cmi_stripe_lock.request_sl_ptr,  
																		sl->cmi_stripe_lock.grant_sl_ptr,
																		sl->cmi_stripe_lock.header.object_id);
#endif
		fbe_metadata_cmi_send_message((fbe_metadata_cmi_message_t *)&sl->cmi_stripe_lock, sl); 
	}
    else
    {
#if 0
    	metadata_trace(FBE_TRACE_LEVEL_INFO,
	    	    		FBE_TRACE_MESSAGE_ID_INFO,
						"SL: EVT SR no peer:%p grl: %p \n", sl->cmi_stripe_lock.request_sl_ptr,  sl->cmi_stripe_lock.grant_sl_ptr);
#endif
		fbe_transport_set_status(sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
		fbe_transport_run_queue_push_packet( sl->cmi_stripe_lock.packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
	}

	return FBE_STATUS_OK;
}



/* Stripe lock hash implementation */



#ifdef EXT_POOL_LOCK
/******************************************************************************
*	ext_pool_hash_init
*******************************************************************************
*
* DESCRIPTION:
*   Initializes a hash.
*   
* PARAMETERS:
*   hash - Pointer to a Hash.
*
* RETURNS:
*   FBE_STATUS_
*
******************************************************************************/
fbe_status_t 
fbe_ext_pool_hash_init(fbe_u8_t * ptr, fbe_u32_t memory_size, fbe_u64_t default_divisor)
{
    fbe_u32_t i;
	fbe_u32_t table_size;
	fbe_metadata_stripe_lock_hash_t * hash;

	/* Calculate how much memory do we have for the table */
	/* Some memory will be consumed by the header */
	table_size = memory_size - sizeof(fbe_metadata_stripe_lock_hash_t);

	/* Each table entry has a size of fbe_metadata_stripe_lock_hash_table_entry_t */
	table_size = table_size / sizeof(fbe_metadata_stripe_lock_hash_table_entry_t);

	hash = (fbe_metadata_stripe_lock_hash_t *)ptr;
	hash->table = (fbe_metadata_stripe_lock_hash_table_entry_t *) (ptr + sizeof(fbe_metadata_stripe_lock_hash_t));

	hash->table_size = table_size;
	hash->default_divisor = default_divisor;
    hash->divisor       = hash->default_divisor;
    hash->count         = 0;

    for(i = 0; i < hash->table_size; i++) {
        fbe_queue_init(&hash->table[i].head);
		hash->table[i].lock = (void *)&hash->table[i].head.next;
    }

#if 0
	metadata_trace(FBE_TRACE_LEVEL_INFO,
    	    		FBE_TRACE_MESSAGE_ID_INFO,
					"SL: HASH divisor = %lld \n", hash->divisor);
#endif

	return FBE_STATUS_OK;
}


/******************************************************************************
*	ext_pool_hash_destroy
*******************************************************************************
*
* DESCRIPTION:
*   Destroys a hash.
*   
* PARAMETERS:
*   hash - Pointer to a Hash.
*
* RETURNS:
*  FBE_STATUS_
*
******************************************************************************/
fbe_status_t
fbe_ext_pool_hash_destroy(void * ptr)
{
    fbe_u32_t i;
	fbe_metadata_stripe_lock_hash_t * hash = (fbe_metadata_stripe_lock_hash_t *)ptr;

    for(i = 0; i < hash->table_size; i++) {
        fbe_queue_destroy(&hash->table[i].head);
		//fbe_spinlock_destroy(&hash->table[i].lock);
    }


	return FBE_STATUS_OK;
}
#endif

fbe_status_t 
fbe_ext_pool_lock_all(fbe_metadata_element_t * mde)
{
#ifdef EXT_POOL_LOCK
    fbe_u32_t i;

	if(mde->stripe_lock_hash != NULL){ /* lock_all */
		for(i = 0; i < mde->stripe_lock_hash->table_size; i++) {        
#ifndef __SAFE__
			csx_p_spin_pointer_lock(&mde->stripe_lock_hash->table[i].lock);
#endif
		}
		if(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED){
			fbe_spinlock_lock( &mde->stripe_lock_spinlock);
		}
	} else {
		fbe_spinlock_lock( &mde->stripe_lock_spinlock);
	}
#endif
	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_ext_pool_unlock_all(fbe_metadata_element_t * mde)
{
#ifdef EXT_POOL_LOCK
    fbe_u32_t i;

	if(mde->stripe_lock_hash != NULL){ /* unlock_all */
		if(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED){
			fbe_spinlock_unlock( &mde->stripe_lock_spinlock);
		}

		for(i = 1; i <= mde->stripe_lock_hash->table_size; i++) {        
#ifndef __SAFE__
			csx_p_spin_pointer_unlock(&mde->stripe_lock_hash->table[mde->stripe_lock_hash->table_size - i].lock);
#endif
		}
	} else {
		fbe_spinlock_unlock( &mde->stripe_lock_spinlock);
	}
#endif
	return FBE_STATUS_OK;
}

/* This function assumes that spinlock is held */
static fbe_status_t
fbe_ext_pool_lock_element_abort_queue(fbe_metadata_element_t * mde, 
											 fbe_queue_head_t * queue,
											 fbe_queue_head_t * abort_queue,
											 fbe_queue_head_t * tmp_queue)
{    
	fbe_queue_element_t * queue_element = NULL;
	fbe_queue_element_t * abort_element = NULL;
	fbe_payload_stripe_lock_operation_t * sl = NULL;
	fbe_metadata_stripe_lock_blob_t *blob = NULL;    
	fbe_bool_t is_abort_allowed;
	fbe_object_id_t object_id;

	blob = mde->stripe_lock_blob;

	object_id = fbe_base_config_metadata_element_to_object_id(mde);

	queue_element = fbe_queue_front(queue);
	while(queue_element != NULL){
		sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
        /* 1. if request has nothing to do with peer;
         * 2. if peer lost hasn't been processed by the object, but we have a peer collision;
         * 3. if peer lost code marked the requests to be aborted.
         */
		is_abort_allowed = FBE_TRUE;

		if(sl->status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING){
			is_abort_allowed = FBE_FALSE; /* It is granted already */
		}

		if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST){
			is_abort_allowed = FBE_FALSE; /* We can not abort outstanding CMI */
		}


		if((sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) && fbe_cmi_is_peer_alive()){
				//fbe_metadata_is_peer_object_alive(mde) && 
				//!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_LOST)){

			is_abort_allowed = FBE_FALSE; /* We can not abort outstanding CMI */
		}

		if((sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION) && fbe_cmi_is_peer_alive()){
				//fbe_metadata_is_peer_object_alive(mde) && 
				//!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_LOST)){
			// AR 554898: Comment the line below out per Peter's suggestion:
            // is_abort_allowed = FBE_FALSE; /* Peer is alive and we have a collision */
            metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "SL: Abort object_id 0x%X, Peer Collision, flags 0x%X status 0x%X \n", object_id, sl->flags, sl->status);

		}

		if(is_abort_allowed){ /* We are going to abort it */
			queue_element = fbe_queue_next(queue, queue_element); 
			fbe_queue_remove(&sl->queue_element);
			sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
			/*if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
				fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
			}*/

			fbe_queue_push(abort_queue, &sl->queue_element);
			while(abort_element = fbe_queue_pop(&sl->wait_queue)){ fbe_queue_push(abort_queue, abort_element); }
			
			continue;
		} else { /* We will abort all the "waiters" */

			metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
				"SL: Abort object_id 0x%X Not aborted: flags 0x%X status 0x%X \n", object_id, sl->flags, sl->status);		

			/* For paged stripe lock we will use the different logic */
			if(ext_pool_lock_is_paged_lock(sl)){
				fbe_ext_pool_lock_abort_paged_waiters(&sl->wait_queue, abort_queue);
			} else {
				while(abort_element = fbe_queue_pop(&sl->wait_queue)){ fbe_queue_push(abort_queue, abort_element); }
			}

			if(!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST)){
				if (sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ) {
					ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.read_region);
					ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.write_region);
				} else { /* write_lock */
					ext_pool_lock_set_region(sl, &sl->cmi_stripe_lock.write_region);
					ext_pool_lock_set_invalid_region(&sl->cmi_stripe_lock.read_region);
				}
			}
		}
		queue_element = fbe_queue_next(queue, queue_element); 
	}

    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_ext_pool_lock_hash(fbe_metadata_element_t * mde, fbe_payload_stripe_lock_operation_t * sl)
{
	fbe_ext_pool_lock_slice_entry_t * he;

	he = fbe_ext_pool_lock_get_lock_table_entry(mde, sl->stripe.first);
	ext_pool_lock_hash_table_entry(he);

	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_ext_pool_unlock_hash(fbe_metadata_element_t * mde, fbe_payload_stripe_lock_operation_t * sl)
{
	fbe_ext_pool_lock_slice_entry_t * he;

	he = fbe_ext_pool_lock_get_lock_table_entry(mde, sl->stripe.first);
	ext_pool_unlock_hash_table_entry(he);
    
	return FBE_STATUS_OK;
}

static void 
fbe_metadata_stripe_lock_peer_lost_release_peer_lock(fbe_metadata_element_t	* mde)
{
	fbe_metadata_stripe_lock_blob_t * blob = NULL;
	fbe_queue_element_t * sl_queue_element = NULL;
	fbe_payload_stripe_lock_operation_t * sl = NULL;

	blob = mde->stripe_lock_blob;
	/* Fix for AR 557739 - Drop ALL peer requests from the blob */
	/* FBE_METADATA_ELEMENT_ATTRIBUTES_SL_PEER_LOST may be cleared too soon by:
	fbe_raid_group_monitor_journal_flush_cond_function calling fbe_metadata_element_clear_sl_peer_lost function
	*/
	/* It is safe to get a spin over peer_sl queue of the blob and drop all peer requests */
	sl_queue_element = fbe_queue_front(&blob->peer_sl_queue);
	while(sl_queue_element != NULL){
        sl = fbe_metadata_stripe_lock_queue_element_to_operation(sl_queue_element);
		sl_queue_element = fbe_queue_next(&blob->peer_sl_queue, sl_queue_element); 
		if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST){
			fbe_queue_remove(&sl->queue_element);
			 /* Just free the request, we don't need it if the peer is dead */
			fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
			fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);  
			ext_pool_lock_peer_sl_queue_count--;
			fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
		}
	}
}

static fbe_status_t 
fbe_ext_pool_lock_peer_lost_mde_callback(fbe_metadata_element_t* mde,
                                         fbe_ext_pool_lock_slice_entry_t * entry, 
                                         void * context)
{
    ext_pool_lock_callback_context_t * context_p = (ext_pool_lock_callback_context_t *)context;
	fbe_queue_head_t * tmp_queue = (fbe_queue_head_t *)(context_p->param_1);
	fbe_bool_t * is_cmi_required = (fbe_bool_t *)(context_p->param_2);
	fbe_queue_element_t		* sl_queue_element = NULL;
	fbe_payload_stripe_lock_operation_t * sl = NULL;
	fbe_queue_head_t * queue;

	ext_pool_lock_hash_table_entry(entry);

	queue = &entry->head;
	sl_queue_element = fbe_queue_front(queue);
	while (sl_queue_element != NULL) {
		sl = fbe_metadata_stripe_lock_queue_element_to_operation(sl_queue_element);
		sl_queue_element = fbe_queue_next(queue, sl_queue_element); 

		/* we need to drop all the pending peer request first. */
		//if (sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_HOLDING_PEER) {
			sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_HOLDING_PEER;
			ext_pool_lock_drop_wait_queue_peer_requests(sl);
		//}

		/* after dropping pending peer requests, 
		 * we need to take care of 
		 *  1. peer request hasn't been granted,
		 *  2. granted peer request with NEED_RELEASE flag set 
		 */
		if (sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) {                
			if (!(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANTED)) {
				ext_pool_lock_drop_peer_request(sl, tmp_queue);
				/* After dispatch - start from the beginning */
				sl_queue_element = fbe_queue_front(queue);
				continue;
			}


			/* AR 564951. Jornaling need all peer SL to be held untill flushing is complete */
			if((ext_pool_lock_is_np_lock(sl)) || (ext_pool_lock_is_paged_lock(sl))) { /* AR 568238 - Jornal flush needs paged lock. We are not jurnaling paged area */
				/* The peer request is granted and need release flag is set */
				if (sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE) {	
					ext_pool_lock_drop_peer_request(sl, tmp_queue);
					/* After dispatch - start from the beginning */
					sl_queue_element = fbe_queue_front(queue);
					continue;
				}
			}
			continue;
		}

		/* AR 568737. All non-paged and paged locks with DEAD_LOCK flag set need to be granted here. */
		if((sl->status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING) && ((ext_pool_lock_is_np_lock(sl)) || (ext_pool_lock_is_paged_lock(sl)))) {
			sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;

			fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
			if (fbe_queue_is_element_on_queue(&sl->cmi_stripe_lock.packet->queue_element)){
				metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s stripe lock %p packet already on the queue.\n", __FUNCTION__, sl);
			}
			fbe_transport_set_status(sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
			fbe_queue_push(tmp_queue, &sl->cmi_stripe_lock.packet->queue_element);
			continue;
		}
		
		
		if (sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION && sl->status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING) {
		/* we need to release Non-paged and paged locks here. The data stripe locks will be released
		 * at the object level after further processing 
		 */
			if ((ext_pool_lock_is_np_lock(sl)) || (ext_pool_lock_is_paged_lock(sl))) {
				sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;

				fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
				if (fbe_queue_is_element_on_queue(&sl->cmi_stripe_lock.packet->queue_element)){
					metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s stripe lock %p packet already on the queue.\n", __FUNCTION__, sl);
				}
				fbe_transport_set_status(sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
				fbe_queue_push(tmp_queue, &sl->cmi_stripe_lock.packet->queue_element);
				continue;
			}
            
			if (fbe_transport_is_monitor_packet(sl->cmi_stripe_lock.packet, fbe_base_config_metadata_element_to_object_id(mde)) ) {
				/* Since this lock is part of monitor packet, we need to abort this so that peer death
				 * processing condition can be handled */
				sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;
				fbe_queue_remove(&sl->queue_element);
				sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
				if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
					fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
				}

				ext_pool_lock_dispatch(sl, tmp_queue, is_cmi_required);

				/* After dispatch - start from the beginning */
				sl_queue_element = fbe_queue_front(queue);

				fbe_transport_record_callback_with_action(sl->cmi_stripe_lock.packet, (fbe_packet_completion_function_t) fbe_ext_pool_lock_peer_lost, PACKET_SL_ABORT);
				fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);
				//ext_pool_lock_update_write_count(sl);
				fbe_transport_set_status(sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
				if (fbe_queue_is_element_on_queue(&sl->cmi_stripe_lock.packet->queue_element)){
					metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s stripe lock %p packet already on the queue.\n", __FUNCTION__, sl);
				}
				fbe_queue_push(tmp_queue, &sl->cmi_stripe_lock.packet->queue_element);
				continue;
			}

			fbe_transport_record_callback_with_action(sl->cmi_stripe_lock.packet, (fbe_packet_completion_function_t) fbe_ext_pool_lock_peer_lost, PACKET_SL_ABORT);

			if ((mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_DESTROY_ABORT_STRIPE_LOCKS) ||
									(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_STRIPE_LOCKS)) {
				sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;

				/* We are aborting, so drop it now. */
				fbe_queue_remove(&sl->queue_element);
				sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
				if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
					fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
				}

				ext_pool_lock_dispatch(sl, tmp_queue, is_cmi_required);

				/* After dispatch - start from the beginning */
				sl_queue_element = fbe_queue_front(queue);

				fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED);
				fbe_transport_set_status(sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
				//ext_pool_lock_update_write_count(sl);
				fbe_transport_set_status(sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
				if (fbe_queue_is_element_on_queue(&sl->cmi_stripe_lock.packet->queue_element)){
					metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
						FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						"%s stripe lock %p packet already on the queue.\n", __FUNCTION__, sl);
				}
				fbe_queue_push(tmp_queue, &sl->cmi_stripe_lock.packet->queue_element);                                                            
				continue;
			}

			sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_LOST;
		}/* if (sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION && sl->status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING) */
	}/* while (sl_queue_element != NULL) */

	ext_pool_unlock_hash_table_entry(entry);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_ext_pool_lock_peer_lost_mde(fbe_metadata_element_t * mde, fbe_queue_head_t * tmp_queue, fbe_bool_t * is_cmi_required)
{
    ext_pool_lock_callback_context_t context;
    context.param_1 = tmp_queue;
    context.param_2 = is_cmi_required;

    fbe_metadata_stripe_lock_peer_lost_release_peer_lock(mde);
    fbe_ext_pool_lock_traverse_hash_table(mde, fbe_ext_pool_lock_peer_lost_mde_callback, &context);

    return FBE_STATUS_OK;
}


static fbe_status_t 
fbe_ext_pool_lock_grant_all_peer_locks_callback(fbe_metadata_element_t* mde,
                                                fbe_ext_pool_lock_slice_entry_t * entry, 
                                                void * context)
{
    ext_pool_lock_callback_context_t * context_p = (ext_pool_lock_callback_context_t *)context;
	fbe_queue_head_t * tmp_queue = (fbe_queue_head_t *)(context_p->param_1);
	fbe_bool_t * is_cmi_required = (fbe_bool_t *)(context_p->param_2);
	fbe_queue_element_t		* sl_queue_element = NULL;
	fbe_payload_stripe_lock_operation_t * sl = NULL;
	fbe_queue_head_t * queue;

	ext_pool_lock_hash_table_entry(entry);

    entry->slice_lock_state = METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL; /* Exclusive local lock */

	queue = &entry->head;
	sl_queue_element = fbe_queue_front(queue);
	while (sl_queue_element != NULL) {
		sl = fbe_metadata_stripe_lock_queue_element_to_operation(sl_queue_element);

        /* AR 601657. Clear WAIT_FOR_PEER and DEAD_LOCK flag for the request in wait_queue. */
        ext_pool_lock_clear_wait_for_peer_requests(sl);
        
		if ((sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && !(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANTED))
		{
			fbe_queue_remove(&sl->queue_element);
			sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
			if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
				fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
			}

			ext_pool_lock_dispatch(sl, tmp_queue, is_cmi_required);
			/* After dispatch - start from the beginning */
			sl_queue_element = fbe_queue_front(queue);

			/* just release the sl */
			fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
			fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);    
			ext_pool_lock_peer_sl_queue_count--;
			fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);

			continue;
		}

		/* AR 564951. Jornaling need all peer SL to be held untill flushing is complete */
		if((sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && 
			(sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE)) { 

			fbe_queue_remove(&sl->queue_element);
			sl->priv_flags &= ~FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT;
			if(sl->priv_flags & FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE){
				fbe_atomic_decrement(&mde->stripe_lock_large_io_count);
			}

			ext_pool_lock_dispatch(sl, tmp_queue, is_cmi_required);
			/* After dispatch - start from the beginning */
			sl_queue_element = fbe_queue_front(queue);

			/* just release the sl */
			fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
			fbe_queue_push(&ext_pool_lock_peer_sl_queue, &sl->queue_element);    
			ext_pool_lock_peer_sl_queue_count--;
			fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);

			continue;		
		}

		if ((sl->flags & (FBE_PAYLOAD_STRIPE_LOCK_FLAG_DEAD_LOCK | FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION)) && 
			(sl->status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING))
		{
			sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION;
			sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER;
			sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_DEAD_LOCK;

			sl_queue_element = fbe_queue_next(queue, sl_queue_element); 

			fbe_payload_stripe_lock_set_status(sl, FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK);
			fbe_transport_set_status(sl->cmi_stripe_lock.packet, FBE_STATUS_OK, 0);
			if (fbe_queue_is_element_on_queue(&sl->cmi_stripe_lock.packet->queue_element)){
				metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s stripe lock %p packet already on the queue.\n", __FUNCTION__, sl);
			}
			fbe_queue_push(tmp_queue, &sl->cmi_stripe_lock.packet->queue_element);             
		}
		else
		{
			sl_queue_element = fbe_queue_next(queue, sl_queue_element); 
		}
	}/* while (sl_queue_element != NULL) */

	ext_pool_unlock_hash_table_entry(entry);

	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_ext_pool_lock_grant_all_peer_locks(fbe_metadata_element_t* mde, fbe_queue_head_t * tmp_queue, fbe_bool_t * is_cmi_required)
{
    ext_pool_lock_callback_context_t context;
    context.param_1 = tmp_queue;
    context.param_2 = is_cmi_required;

    fbe_ext_pool_lock_traverse_hash_table(mde, fbe_ext_pool_lock_grant_all_peer_locks_callback, &context);
    is_cmi_required = FBE_FALSE;

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_ext_pool_lock_scan_hash_table_callback(fbe_metadata_element_t* mde,
                                           fbe_ext_pool_lock_slice_entry_t * entry, 
                                           void * context)
{
	fbe_queue_head_t * grant_queue = (fbe_queue_head_t *)context;
	fbe_queue_element_t * queue_element = NULL;
	fbe_queue_element_t * grant_queue_element = NULL;
	fbe_payload_stripe_lock_operation_t * sl = NULL;	
	fbe_queue_head_t * queue;
	fbe_payload_stripe_lock_operation_t * grant_sl = NULL;

	ext_pool_lock_hash_table_entry(entry);

	queue = &entry->head;
	/* Scan for the grants */
	queue_element = fbe_queue_front(queue);
	while(queue_element != NULL){
		sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
		if(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANT && !(sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANTED)){			

			/* Allocate grant sl */
			fbe_spinlock_lock(&ext_pool_lock_peer_sl_queue_lock);
			grant_queue_element = fbe_queue_pop(&ext_pool_lock_peer_sl_queue);
			/* Fix for AR 550394 */
			if(grant_queue_element == NULL){ /* We out of memory */
				metadata_trace(FBE_TRACE_LEVEL_WARNING,
							   FBE_TRACE_MESSAGE_ID_INFO,
							   "SL: ext_pool_lock_peer_sl_queue is EMPTY\n");
				fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);
				break;
			}

			ext_pool_lock_peer_sl_queue_count++;
			fbe_spinlock_unlock(&ext_pool_lock_peer_sl_queue_lock);

			sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANTED;
			grant_sl = fbe_metadata_stripe_lock_queue_element_to_operation(grant_queue_element);

			/* Copy sl */
			fbe_copy_memory(grant_sl, sl, sizeof(fbe_payload_stripe_lock_operation_t));
			fbe_queue_element_init(&grant_sl->queue_element);

			grant_sl->cmi_stripe_lock.grant_sl_ptr = sl;

			/* Associate new sl with original one */
			grant_sl->wait_queue.next = sl; /* This is UGLY */			

			/* Put new sl on grant_queue */
			fbe_queue_push(grant_queue, (fbe_queue_element_t *)grant_sl); /* This is ugly. (We may not need it anymore) */
		}
		queue_element = fbe_queue_next(queue, queue_element);
	}

	ext_pool_unlock_hash_table_entry(entry);

	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_stripe_lock_scan_for_grants(fbe_metadata_element_t* mde, fbe_queue_head_t * grant_queue)
{

	fbe_ext_pool_lock_traverse_hash_table(mde, fbe_ext_pool_lock_scan_hash_table_callback, grant_queue);

	return FBE_STATUS_OK;
}



#ifdef EXT_POOL_LOCK
static fbe_status_t 
ext_pool_lock_disable_hash(fbe_metadata_element_t * mde)
{
	fbe_queue_element_t * queue_element;
	fbe_payload_stripe_lock_operation_t * sl;
	fbe_u32_t i;
	fbe_object_id_t object_id;

	object_id = fbe_base_config_metadata_element_to_object_id(mde);

	metadata_trace(FBE_TRACE_LEVEL_INFO, 
				   FBE_TRACE_MESSAGE_ID_INFO, 
				   "SL: HASH %x Disable entry \n", object_id);

	/* Check if hash is already disabled */
	if(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED){
		/* Nothing needs to be done */
		metadata_trace(FBE_TRACE_LEVEL_INFO, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "SL: HASH %x Already disabled \n", object_id);

		return FBE_STATUS_OK;
	}

	/* Loop over all hash queues and insert sl into stripe_lock_queue_head */
	for(i = 0; i < mde->stripe_lock_hash->table_size; i++){
		while(queue_element = fbe_queue_pop(&mde->stripe_lock_hash->table[i].head)){
			sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
			ext_pool_lock_push_sorted(&mde->stripe_lock_queue_head, sl);
		}
	}
	
    fbe_atomic_32_or(&mde->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED);

	metadata_trace(FBE_TRACE_LEVEL_INFO, 
				   FBE_TRACE_MESSAGE_ID_INFO, 
				   "SL: HASH %x disabled \n", object_id);

	return FBE_STATUS_OK;
}

static fbe_status_t 
ext_pool_lock_enable_hash(fbe_metadata_element_t * mde)
{
	fbe_queue_element_t * queue_element;
	fbe_payload_stripe_lock_operation_t * sl;
	fbe_metadata_stripe_lock_hash_table_entry_t * he;
	fbe_object_id_t object_id;

	object_id = fbe_base_config_metadata_element_to_object_id(mde);

	metadata_trace(FBE_TRACE_LEVEL_INFO, 
				   FBE_TRACE_MESSAGE_ID_INFO, 
				   "SL: HASH %x Enable entry \n", object_id);

	/* Check if hash is already enabled */
	if(!(mde->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED)){
		/* Nothing needs to be done */
		metadata_trace(FBE_TRACE_LEVEL_INFO, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "SL: HASH %x Already enabled \n", object_id);
		return FBE_STATUS_OK;
	}

	/* Check that we do not have Large I/O's in flight */
	if(mde->stripe_lock_large_io_count != 0){
		/* There are some number of large I/O's in flight */
		metadata_trace(FBE_TRACE_LEVEL_INFO, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "SL: HASH %x Large I/O in flight \n", object_id);

		return FBE_STATUS_OK;
	}

	/* Loop over stripe_lock_queue_head queue and push sl to hash */
	while(queue_element = fbe_queue_pop(&mde->stripe_lock_queue_head)){
		sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);
		he = fbe_ext_pool_hash_get_entry(mde->stripe_lock_hash, sl);
		ext_pool_lock_push_sorted(&he->head, sl);
	}

	
    fbe_atomic_32_and(&mde->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED);
    

	metadata_trace(FBE_TRACE_LEVEL_INFO, 
				   FBE_TRACE_MESSAGE_ID_INFO, 
				   "SL: HASH %x enabled \n", object_id);

	return FBE_STATUS_OK;	
}

static fbe_status_t 
ext_pool_lock_push_sorted(fbe_queue_head_t * queue, fbe_payload_stripe_lock_operation_t * sl)
{
	fbe_queue_element_t * queue_element;
	fbe_payload_stripe_lock_operation_t * current_sl;

	queue_element = fbe_queue_front(queue);
	while(queue_element != NULL){ 
		current_sl = fbe_metadata_stripe_lock_queue_element_to_operation(queue_element);

		if(ext_pool_lock_check_sorted(sl, current_sl)){ /* Order is IMPORTANT */
			fbe_queue_insert(&sl->queue_element, &current_sl->queue_element);

			return FBE_TRUE; /* There is no overlap: We are done */
		}

		queue_element = fbe_queue_next(queue, queue_element); 
	}/* while(queue_element != NULL) */

	fbe_queue_push(queue, &sl->queue_element);

	return FBE_STATUS_OK;
}
#endif

static fbe_status_t
ext_pool_lock_reschedule_blob(fbe_metadata_element_t * mde, fbe_bool_t is_cmi_required, fbe_bool_t is_hash_enable_required)
{

	if(is_cmi_required || is_hash_enable_required){
		fbe_spinlock_lock(&ext_pool_lock_blob_queue_lock);
		if(is_hash_enable_required){
			mde->stripe_lock_blob->flags |= METADATA_STRIPE_LOCK_BLOB_FLAG_ENABLE_HASH;
		}
		if(!fbe_queue_is_element_on_queue(&mde->stripe_lock_blob->queue_element)){
			fbe_queue_push(&ext_pool_lock_blob_queue, &mde->stripe_lock_blob->queue_element);
			fbe_rendezvous_event_set(&ext_pool_lock_event);
		}		
		fbe_spinlock_unlock(&ext_pool_lock_blob_queue_lock);	
	}

	return FBE_STATUS_OK;
} 
