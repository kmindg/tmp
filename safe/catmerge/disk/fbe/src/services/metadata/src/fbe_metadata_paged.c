#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"
#include "fbe_metadata.h"
#include "fbe/fbe_base_config.h"
#include "fbe_topology.h"
#include "fbe_cmi.h"
#include "fbe_metadata_private.h"
#include "fbe_metadata_cmi.h"
#include "fbe/fbe_xor_api.h"
 
#define PAGED_SERIAL 0

enum fbe_metadata_paged_constants_e{
    METADATA_PAGED_BLOB_SLOTS       = 16, /* Recommended number of slots per blob aligned to 4k */
    METADATA_PAGED_BLOB_SLOTS_MIN   = 2, /* Recommended number of slots per blob aligned to 4k */
    METADATA_PAGED_MAX_SGL			= 64, /* MAX SG list size for blob 64 */
    METADATA_PAGED_MAX_SGL_ALIGNED	= 64 + 16, /* MAX SG list size for blob 64 + 4k alignment blocks */
#if defined(SIMMODE_ENV)
    METADATA_PAGED_MAX_BLOBS        = 1000,
#else
    METADATA_PAGED_MAX_BLOBS        = 8192, /* TODO:  Peter needs to change this. 02-07-11 */
#endif
    METADATA_PAGED_MAX_SLOTS        = METADATA_PAGED_BLOB_SLOTS_MIN * METADATA_PAGED_MAX_BLOBS,
};

enum metadata_paged_blob_flags_e{
    METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS			= 0x0001, /* We have outstanding read or write for this blob */
    METADATA_PAGED_BLOB_FLAG_WRITE_VERIFY_NEEDED	= 0x0002, /* This blob had a correctable error on read, needs to do a write verify on the next write. */
    METADATA_PAGED_BLOB_FLAG_PRIVATE				= 0x0004, /* This blob has to be released after operation */
    METADATA_PAGED_BLOB_FLAG_CLIENT_BLOB			= 0x0008, /* This blob is allocated by client */
    METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT			= 0x0010, /* IO outstanding to disk */
    METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET			= 0x0020, /* Use client packet */
};

typedef struct metadata_paged_slot_s{ /* Just a piece of memory (for one block) with queue element */
    fbe_queue_element_t  queue_element;
    fbe_u8_t data[FBE_METADATA_BLOCK_SIZE];
}metadata_paged_slot_t;

typedef struct metadata_paged_blob_s{
    fbe_queue_element_t queue_element; /* Will be used to queue with paged_blob_queue_head  and with internal queue */
    fbe_sg_element_t    sg_list[METADATA_PAGED_MAX_SGL_ALIGNED + 1]; /* +1 for NULL termination */
    fbe_metadata_element_t    * metadata_element;
    fbe_lba_t                   lba; /* lba offset of this blob (we need to add metadata paged offset) */
    fbe_block_count_t           block_count; /* number of slots currently allocated */
    fbe_u32_t                   flags;
    fbe_u32_t					slot_count; /* How many slots where allocated */

    fbe_lba_t					stripe_offset; /* When exclusive access for this stripe is granted to peer we must to release this blob */
    fbe_block_count_t			stripe_count; /* number of slots currently allocated */

    //fbe_queue_head_t            packet_queue; /* If read in progress we will queue other reads for same are here */
    fbe_time_t					time_stamp; /* Last time we uses this blob */
    fbe_packet_t              * packet_ptr; /* Packet pointer */
    fbe_packet_t				packet; /* We will use this packet to send I/O */
}metadata_paged_blob_t;

static fbe_u8_t         *   metadata_paged_blob_memory = NULL; 
static fbe_u8_t         *   metadata_paged_slot_memory = NULL; 

static fbe_queue_head_t     metadata_paged_slot_queue;
static fbe_queue_head_t     metadata_paged_blob_queue;

static fbe_spinlock_t       metadata_paged_blob_queue_lock;

static fbe_atomic_t metadata_paged_write_lock_counter = 0;
static fbe_atomic_t metadata_paged_bundle_counter = 0;

/* Forward declarations */
static fbe_status_t metadata_paged_switch(fbe_packet_t * packet, fbe_packet_completion_context_t * context);
static fbe_status_t metadata_paged_write(fbe_packet_t * packet);
static fbe_status_t metadata_paged_write_verify(fbe_packet_t * packet);
static fbe_status_t metadata_paged_change(fbe_packet_t * packet);
static fbe_status_t metadata_paged_read(fbe_packet_t * packet);

static fbe_status_t metadata_paged_write_blob(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t metadata_paged_write_blob_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t metadata_paged_read_blob(fbe_packet_t * packet, fbe_memory_completion_context_t context);
static fbe_status_t metadata_paged_read_blob_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_paged_metadata_dispatch_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_paged_metadata_operation_restart_io_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_metadata_paged_abort_io_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_metadata_paged_drain_io_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t metadata_paged_dispatch(fbe_packet_t* packet, fbe_bool_t is_completion);

static fbe_payload_metadata_status_t 
fbe_metadata_determine_status(fbe_status_t status,
                              fbe_payload_block_operation_status_t  block_operation_status,
                              fbe_payload_block_operation_qualifier_t  block_operation_qualifier);


static metadata_paged_blob_t * metadata_paged_get_blob(fbe_payload_metadata_operation_t * mdo, fbe_bool_t * in_progress);
static metadata_paged_blob_t * metadata_paged_allocate_blob(fbe_payload_metadata_operation_t * mdo, fbe_u32_t slot_count);
static fbe_status_t metadata_paged_release_blob(metadata_paged_blob_t * paged_blob);
static fbe_status_t metadata_paged_release_overlapping_blobs(fbe_payload_metadata_operation_t * mdo);

/* Copy blob data to metadata operation */
static fbe_status_t metadata_paged_blob_to_mdo(metadata_paged_blob_t * paged_blob, fbe_payload_metadata_operation_t * mdo);

/* Update blob data from metadata operation */
static fbe_status_t metadata_paged_mdo_to_blob(metadata_paged_blob_t * paged_blob, 
                                                              fbe_payload_metadata_operation_t * mdo,
                                                              fbe_bool_t * is_write_requiered);


static fbe_payload_metadata_operation_t * metadata_paged_get_metadata_operation(fbe_payload_ex_t * sep_payload);

static fbe_status_t metadata_paged_dispatch_acquire_stripe_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t metadata_paged_release_stripe_lock(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t metadata_paged_release_stripe_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t metadata_paged_send_io_packet(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t metadata_paged_get_next_marked_chunk(metadata_paged_blob_t * paged_blob, fbe_payload_metadata_operation_t * mdo);

static fbe_status_t fbe_metadata_paged_read(fbe_packet_t * packet);
static fbe_status_t fbe_metadata_paged_update(fbe_packet_t * packet);
static fbe_status_t fbe_metadata_paged_read_blob(fbe_packet_t * packet, fbe_memory_completion_context_t context);
static fbe_status_t fbe_metadata_paged_read_blob_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_metadata_paged_write_blob(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_metadata_paged_write_blob_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static __forceinline fbe_status_t fbe_metadata_paged_call_callback_function(fbe_payload_metadata_operation_t * mdo,
                                                                            metadata_paged_blob_t * paged_blob,
                                                                            fbe_packet_t *packet);
static __forceinline fbe_status_t fbe_metadata_paged_call_object_cache_function(fbe_metadata_paged_cache_action_t action,
                                                           fbe_metadata_element_t * mde,
                                                           fbe_payload_metadata_operation_t * mdo,
                                                           fbe_lba_t start_lba, fbe_block_count_t block_count,
                                                           metadata_paged_blob_t * paged_blob);

void fbe_metadata_paged_get_memory_use(fbe_u32_t *memory_bytes_p)
{
    fbe_u32_t memory_bytes = 0;

    memory_bytes += sizeof(metadata_paged_blob_t) * METADATA_PAGED_MAX_BLOBS;
    memory_bytes += sizeof(metadata_paged_slot_t) * METADATA_PAGED_MAX_SLOTS;

    *memory_bytes_p = memory_bytes;
}

fbe_status_t 
fbe_metadata_paged_init(void)
{
    metadata_paged_blob_t * blob_ptr = NULL;
    metadata_paged_slot_t * slot_ptr = NULL;
    //fbe_memory_allocation_function_t memory_allocation_function = NULL;
    fbe_u32_t i;

    fbe_queue_init(&metadata_paged_blob_queue);
    fbe_queue_init(&metadata_paged_slot_queue);    

    fbe_spinlock_init(&metadata_paged_blob_queue_lock);
    
    /* Allocate memory for blobs */
    //memory_allocation_function = fbe_memory_get_allocation_function();
    metadata_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "MCRMEM: Paged blob: %d \n", (int)(sizeof(metadata_paged_blob_t) * METADATA_PAGED_MAX_BLOBS));

    metadata_paged_blob_memory = fbe_memory_allocate_required(sizeof(metadata_paged_blob_t) * METADATA_PAGED_MAX_BLOBS);

#if 0
    if(memory_allocation_function != NULL) {
        metadata_paged_blob_memory = memory_allocation_function(sizeof(metadata_paged_blob_t) * METADATA_PAGED_MAX_BLOBS);
    } else {
        metadata_paged_blob_memory = fbe_memory_native_allocate(sizeof(metadata_paged_blob_t) * METADATA_PAGED_MAX_BLOBS);
    }
#endif

    fbe_zero_memory(metadata_paged_blob_memory, sizeof(metadata_paged_blob_t) * METADATA_PAGED_MAX_BLOBS);
    
    blob_ptr = (metadata_paged_blob_t *)metadata_paged_blob_memory;
    for(i = 0; i < METADATA_PAGED_MAX_BLOBS; i++){
        blob_ptr->packet_ptr = &blob_ptr->packet;
        fbe_queue_push(&metadata_paged_blob_queue, &blob_ptr->queue_element);
        blob_ptr->metadata_element = NULL;
        blob_ptr->flags = 0;
        blob_ptr++;
    }

    /* Allocate memory for slots */
    metadata_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "MCRMEM: Paged slot: %d \n", (int)(sizeof(metadata_paged_slot_t) * METADATA_PAGED_MAX_SLOTS));

    
    metadata_paged_slot_memory = fbe_memory_allocate_required(sizeof(metadata_paged_slot_t) * METADATA_PAGED_MAX_SLOTS);

#if 0
    if(memory_allocation_function != NULL) {
        metadata_paged_slot_memory = memory_allocation_function(sizeof(metadata_paged_slot_t) * METADATA_PAGED_MAX_SLOTS);
    } else {
        metadata_paged_slot_memory = fbe_memory_native_allocate(sizeof(metadata_paged_slot_t) * METADATA_PAGED_MAX_SLOTS);
    }
#endif

    fbe_zero_memory(metadata_paged_slot_memory, sizeof(metadata_paged_slot_t) * METADATA_PAGED_MAX_SLOTS);
    
    slot_ptr = (metadata_paged_slot_t *)metadata_paged_slot_memory;
    for(i = 0; i < METADATA_PAGED_MAX_SLOTS; i++){
        fbe_queue_element_init(&slot_ptr->queue_element);
        fbe_queue_push(&metadata_paged_slot_queue, &slot_ptr->queue_element);
        slot_ptr++;
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_metadata_paged_destroy(void)
{
    metadata_paged_blob_t * blob_ptr = NULL;
    //fbe_memory_release_function_t memory_release_function = NULL;

    blob_ptr = (metadata_paged_blob_t *)metadata_paged_blob_memory;
    fbe_queue_destroy(&metadata_paged_blob_queue);
    fbe_queue_destroy(&metadata_paged_slot_queue);

    fbe_spinlock_destroy(&metadata_paged_blob_queue_lock);

    fbe_memory_release_required(metadata_paged_blob_memory);
    fbe_memory_release_required(metadata_paged_slot_memory);

#if 0
    memory_release_function = fbe_memory_get_release_function();

    if(memory_release_function != NULL){
        memory_release_function(metadata_paged_blob_memory);
        memory_release_function(metadata_paged_slot_memory);
    } else {
        fbe_memory_native_release(metadata_paged_blob_memory);
        fbe_memory_native_release(metadata_paged_slot_memory);
    }
#endif

    if(metadata_paged_bundle_counter > 0){
        metadata_trace(FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "PAGED: write_lock_counter %lld, bundle_counter %lld \n", 
                               metadata_paged_write_lock_counter, metadata_paged_bundle_counter);    	
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_metadata_paged_operation_entry(fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                       * sep_payload = NULL;
    fbe_payload_metadata_operation_t        * mdo = NULL;
    fbe_metadata_element_t                  * metadata_element = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo = metadata_paged_get_metadata_operation(sep_payload);
    mdo->paged_blob = NULL;
    metadata_element = mdo->metadata_element;

    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s start mde: %p; pkt: %p\n",__FUNCTION__, metadata_element, packet);

    /* Get the queue lock*/
    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

    if (metadata_element->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_FAIL_PAGED) {
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_ABORTED);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_run_queue_push_packet(packet,FBE_TRANSPORT_RQ_METHOD_SAME_CORE);        
        return FBE_STATUS_PENDING;
    }

    /* Increment the outstanding io count since the packet is being processed. */
    metadata_element->paged_record_request_count++;

    /* Release the queue lock */
    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s starting packet %p mde_flags: 0x%x\n",
                   __FUNCTION__, packet, mdo->metadata_element->attributes);

    status = metadata_paged_dispatch(packet, FBE_FALSE);
    
    return status;

}// End fbe_metadata_paged_operation_entry


/*!****************************************************************************
 * fbe_paged_metadata_dispatch_packet_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for the paged metadata operation. This
 *   function will decrement the outstanding count since the io completed.
 *   It will call the restart io to see whether there are any more packets
 *   that need to be processed or not.
 * 
 * 
 * @param packet              - pointer to the packet
 * @param context                  - Completion context
 *
 * @return  fbe_status_t            
 *
 * @author
 *   05/10/2010 - Created. Ashwin Tamilarasan. 
 *   11/16/2010 - Modified. Peter Puhov.
 ******************************************************************************/

static fbe_status_t 
fbe_paged_metadata_dispatch_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{    
    fbe_metadata_element_t      * metadata_element = NULL;
    fbe_packet_t                * new_packet = NULL;
    fbe_queue_head_t            tmp_queue;

    fbe_queue_init(&tmp_queue);

    metadata_element = (fbe_metadata_element_t *)context;
    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s mde: %p; pkt: %p\n", __FUNCTION__, metadata_element,
           packet);
    
    /* Grab the queue lock and decrement the otstanding the io count since the io completed */
    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

    if (metadata_element->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_FAIL_PAGED){
        //metadata_paged_blob_t * paged_blob = NULL;
        fbe_payload_ex_t                       * sep_payload = NULL;
        fbe_payload_metadata_operation_t        * mdo = NULL;

        while(new_packet = fbe_transport_dequeue_packet(&metadata_element->paged_record_queue_head)){
            metadata_element->paged_record_request_count--;
            sep_payload = fbe_transport_get_payload_ex(new_packet);
            mdo = metadata_paged_get_metadata_operation(sep_payload);
            fbe_transport_set_status(new_packet, FBE_STATUS_OK, 0);
            fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_ABORTED);
            fbe_queue_push(&tmp_queue, &new_packet->queue_element);
        }

#if 0
        paged_blob = (metadata_paged_blob_t *)fbe_queue_front(&metadata_element->paged_blob_queue_head);

        if(paged_blob != NULL){
            while(new_packet = fbe_transport_dequeue_packet(&paged_blob->packet_queue)){
                sep_payload = fbe_transport_get_payload_ex(new_packet);
                mdo = metadata_paged_get_metadata_operation(sep_payload);
                fbe_transport_set_status(new_packet, FBE_STATUS_OK, 0);
                fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_ABORTED);
                fbe_queue_push(&tmp_queue, &new_packet->queue_element);
            }
        }
#endif
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
        fbe_queue_destroy(&tmp_queue);
        return FBE_STATUS_OK;
    }

    while(new_packet = fbe_transport_dequeue_packet(&metadata_element->paged_record_queue_head)){
        /* Increment the number of paged record request count. */
        metadata_element->paged_record_request_count++;
        /* Enqueue this packet temporary queue. */
        fbe_queue_push(&tmp_queue, &new_packet->queue_element);
    }

    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    /* Enqueue the request to run queue. */
    if(!fbe_queue_is_empty(&tmp_queue)){
        fbe_transport_run_queue_push(&tmp_queue, fbe_paged_metadata_operation_restart_io_completion, NULL);
    }

    fbe_queue_destroy(&tmp_queue);
    return FBE_STATUS_OK;     
}// End fbe_paged_metadata_dispatch_packet_completion


fbe_status_t 
fbe_metadata_paged_operation_restart_io(fbe_metadata_element_t * metadata_element)
{    
    fbe_packet_t                * new_packet = NULL;
    fbe_queue_head_t            tmp_queue;

    fbe_queue_init(&tmp_queue);
    
    /* Grab the queue lock and decrement the otstanding the io count since the io completed */
    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
    if(metadata_element->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_FAIL_PAGED){
        //metadata_paged_blob_t * paged_blob = NULL;
        fbe_payload_ex_t                       * sep_payload = NULL;
        fbe_payload_metadata_operation_t        * mdo = NULL;

        while(new_packet = fbe_transport_dequeue_packet(&metadata_element->paged_record_queue_head)){
            sep_payload = fbe_transport_get_payload_ex(new_packet);
            mdo = metadata_paged_get_metadata_operation(sep_payload);

			fbe_transport_set_status(new_packet, FBE_STATUS_OK, 0);
			fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_ABORTED);

			fbe_queue_push(&tmp_queue, &new_packet->queue_element);
		}

#if 0
        paged_blob = (metadata_paged_blob_t *)fbe_queue_front(&metadata_element->paged_blob_queue_head);

        if(paged_blob != NULL){
            while(new_packet = fbe_transport_dequeue_packet(&paged_blob->packet_queue)){
                sep_payload = fbe_transport_get_payload_ex(new_packet);
                mdo = metadata_paged_get_metadata_operation(sep_payload);

                fbe_transport_set_status(new_packet, FBE_STATUS_OK, 0);
                fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_ABORTED);

                fbe_queue_push(&tmp_queue, &new_packet->queue_element);
            }
        }
#endif

        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
        fbe_queue_destroy(&tmp_queue);
        return FBE_STATUS_OK;
    }

    if((!fbe_queue_is_empty(&metadata_element->paged_record_queue_head)) && metadata_element->paged_record_request_count == 0) {
        new_packet = fbe_transport_dequeue_packet(&metadata_element->paged_record_queue_head);
        /* Increment the number of paged record request count. */
        metadata_element->paged_record_request_count++;
        /* Enqueue this packet temporary queue. */
        fbe_queue_push(&tmp_queue, &new_packet->queue_element);
    }

    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);


    /* Enqueue the request to run queue. */
    if(!fbe_queue_is_empty(&tmp_queue)){
        fbe_transport_run_queue_push(&tmp_queue, fbe_paged_metadata_operation_restart_io_completion, NULL);
    }

    fbe_queue_destroy(&tmp_queue);

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_paged_metadata_operation_restart_io_completion()
 ******************************************************************************
 * @brief
 *   This function will crank out IO.
 * 
 * @param packet        - pointer to the packet
 * @param context       - pointer to the context
 *
 * @return  fbe_status_t            
 *
 * @author
 *   09/03/2010 - Created. NCHIU
 *   11/16/2010 - Modified. Peter Puhov.
 ******************************************************************************/
static fbe_status_t 
fbe_paged_metadata_operation_restart_io_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;

    /* send the packet to start the paged metadata operation. */
    status = metadata_paged_dispatch(packet, FBE_TRUE);

    return status;
}// End fbe_paged_metadata_operation_restart_io_completion


/*!****************************************************************************
 * fbe_metadata_paged_abort_io()
 ******************************************************************************
 * @brief
 *   This function will drain the queued paged metadata operations and
 *   complete them with aborted status.
 * 
 * @param metadata_element - pointer to the metadata element
 *
 * @return  fbe_status_t            
 *
 * @author
 *  10/1/2010 - Created. Rob Foley
 *  11/16/2010 - Modified. Peter Puhov
 ******************************************************************************/
fbe_status_t 
fbe_metadata_paged_abort_io(fbe_metadata_element_t * metadata_element)
{    
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_packet_t*               new_packet = NULL;
    fbe_queue_head_t tmp_queue;

    fbe_queue_init(&tmp_queue);

    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

   /* Move the queue to the temporary queue.
    */
    while (!fbe_queue_is_empty(&metadata_element->paged_record_queue_head))
    {
        new_packet = fbe_transport_dequeue_packet(&metadata_element->paged_record_queue_head);
        fbe_transport_set_status(new_packet, FBE_STATUS_IO_FAILED_NOT_RETRYABLE, 0);
        /* Enqueue this packet to temporary queue. */
        fbe_queue_push(&tmp_queue, &new_packet->queue_element);
    }
    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    /* Enqueue the request to run queue. */
    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);

    fbe_queue_destroy(&tmp_queue);
    return status;

}// End fbe_metadata_paged_abort_io


/*!****************************************************************************
 * fbe_metadata_paged_drain_io()
 ******************************************************************************
 * @brief
 *   This function will drain the queued paged metadata operations and
 *   complete them with "retryable" status.
 * 
 * @param metadata_element - pointer to the metadata element
 *
 * @return  fbe_status_t            
 *
 * @author
 *  3/28/2011 - Created based on fbe_metadata_paged_abort_io()
 *
 ******************************************************************************/
fbe_status_t 
fbe_metadata_paged_drain_io(fbe_metadata_element_t * metadata_element)
{    
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_packet_t*               new_packet = NULL;
    fbe_queue_head_t            tmp_queue;
    fbe_u32_t                   i = 0;

    fbe_queue_init(&tmp_queue);

    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

   /* Move the queue to the temporary queue.
    */
    while (!fbe_queue_is_empty(&metadata_element->paged_record_queue_head))
    {
        new_packet = fbe_transport_dequeue_packet(&metadata_element->paged_record_queue_head);
        
        fbe_transport_set_status(new_packet, FBE_STATUS_IO_FAILED_RETRYABLE, 0);
        /* Enqueue this packet to temporary queue. */
        fbe_queue_push(&tmp_queue, &new_packet->queue_element);
        if(i++ > 0x00ffffff){
            metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Critical error looped queue\n", __FUNCTION__);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
    }
    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    /* Enqueue the request to run queue. */
    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);

    fbe_queue_destroy(&tmp_queue);
    return status;

}// End fbe_metadata_paged_drain_io

static fbe_status_t 
metadata_paged_switch(fbe_packet_t * packet, fbe_packet_completion_context_t * context)
{
    fbe_status_t status;

    fbe_payload_metadata_operation_t * mdo = (fbe_payload_metadata_operation_t *)context;
    switch(mdo->opcode) {
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_CLEAR_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_XOR_BITS:
            status = metadata_paged_change(packet);
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE:
            status = metadata_paged_write(packet);
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE:
            status = metadata_paged_write_verify(packet);
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_NEXT_MARKED_BITS:
            status = metadata_paged_read(packet);
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_READ:
            status = fbe_metadata_paged_read(packet);
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY_UPDATE:
            status = fbe_metadata_paged_update(packet);
            break;
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t 
metadata_paged_dispatch(fbe_packet_t * packet, fbe_bool_t is_completion)
{    
    fbe_payload_ex_t                       * sep_payload = NULL;
    fbe_payload_metadata_operation_t        * mdo = NULL;
    fbe_metadata_element_t                  * metadata_element = NULL;
    fbe_payload_metadata_operation_opcode_t  opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_INVALID;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_lba_t                               stripe_offset = 0;
    fbe_block_count_t                       stripe_count = 0;
    fbe_payload_stripe_lock_operation_t *   stripe_lock_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo = metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;

    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                   "%s mde: %p\n", __FUNCTION__, metadata_element);

    /* Check if we need to get Stripe Lock */
    fbe_payload_metadata_get_metadata_stripe_offset(mdo, &stripe_offset);
    fbe_payload_metadata_get_metadata_stripe_count(mdo, &stripe_count);
    fbe_transport_set_completion_function(packet, fbe_paged_metadata_dispatch_packet_completion, metadata_element);
    if(stripe_count != 0) { /* We MUST to aquiere stripe lock */
        fbe_payload_metadata_get_opcode(mdo, &opcode);
        stripe_lock_operation = fbe_payload_ex_allocate_stripe_lock_operation(sep_payload);
        switch(opcode) {
            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS:
            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_CLEAR_BITS:
            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_XOR_BITS:
            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE:
            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY:
            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE:
                /* Acquire write stripe lock. */
                fbe_payload_stripe_lock_build_write_lock(stripe_lock_operation, metadata_element, stripe_offset, stripe_count); 
                //fbe_atomic_increment(&metadata_paged_write_lock_counter);
                break;

            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_BITS:
            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_NEXT_MARKED_BITS:
                /* Acquire read stripe lock. */
                fbe_payload_stripe_lock_build_read_lock(stripe_lock_operation, metadata_element, stripe_offset, stripe_count);
                break;

            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE:
            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY_UPDATE:
                /* Acquire write stripe lock. */
                fbe_payload_stripe_lock_build_write_lock(stripe_lock_operation, metadata_element, stripe_offset, stripe_count); 
                break;
            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_READ:
                /* Acquire read stripe lock. */
                fbe_payload_stripe_lock_build_read_lock(stripe_lock_operation, metadata_element, stripe_offset, stripe_count);
                break; 
        }
        
        fbe_payload_stripe_lock_set_sync_mode(stripe_lock_operation);
        fbe_payload_ex_increment_stripe_lock_operation_level(sep_payload);
        /* Set the completion function before sending request to acquire stripe lock. */
        fbe_transport_set_completion_function(packet, metadata_paged_dispatch_acquire_stripe_lock_completion, NULL);                           

        /* Send a request to allocate the stripe lock. */
        status = fbe_stripe_lock_operation_entry(packet);
        if(status == FBE_STATUS_OK){ /* sync mode */        
            if(is_completion){
                return FBE_STATUS_CONTINUE;
            } else {
                fbe_transport_complete_packet(packet);
            }
        }
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* TODO Operations without stripe lock is UNSAFE */
    status = metadata_paged_switch(packet, (fbe_packet_completion_context_t *)mdo);

    return status;

}// End metadata_paged_dispatch

static fbe_status_t 
metadata_paged_change(fbe_packet_t * packet)
{
    fbe_status_t           status;    
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    metadata_paged_blob_t * paged_blob = NULL;
	fbe_bool_t is_write_required;
	fbe_bool_t in_progress = FBE_FALSE;
	fbe_block_count_t block_count;
	fbe_lba_t lba_offset;
	

    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo =  metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;

    if(mdo->u.metadata.repeat_count > 1){ /* No caching */
        /* How many blocks remains to be written ? */
        block_count = ((mdo->u.metadata.repeat_count - mdo->u.metadata.current_count_private) * mdo->u.metadata.record_data_size) / FBE_METADATA_BLOCK_DATA_SIZE;
        if(((mdo->u.metadata.repeat_count - mdo->u.metadata.current_count_private) * mdo->u.metadata.record_data_size) % FBE_METADATA_BLOCK_DATA_SIZE){
            block_count += 1; /* If we are not block aligned we need to read additional block */
        }

        /* Align block_count to 4K */
        if(block_count & 0x0000000000000007){ /* Not aligned  to 4K */
            block_count = (block_count + 8) & ~0x0000000000000007;
        }

        block_count = FBE_MIN(METADATA_PAGED_MAX_SGL, block_count); /* How much we can write ? */

        if(mdo->u.metadata.current_count_private == 0){ /* Check if it is first change */
            fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
            metadata_paged_release_overlapping_blobs(mdo);
            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

            if (mdo->u.metadata.client_blob == NULL)
            {
                paged_blob = metadata_paged_allocate_blob(mdo, (fbe_u32_t)block_count);
            }
            else
            {
                paged_blob = (metadata_paged_blob_t *)(mdo->u.metadata.client_blob);
                paged_blob->slot_count = (fbe_u32_t)block_count;
                paged_blob->block_count = (fbe_u32_t)block_count;
            }
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
            mdo->paged_blob = paged_blob;
            fbe_queue_element_init(&paged_blob->queue_element);
        } else {
            paged_blob = mdo->paged_blob;
            /* Check if block count much the number of slots */
            if(paged_blob->slot_count != block_count){ /* release the blob and allocate new one */
                if (mdo->u.metadata.client_blob == NULL)
                {
                    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
                    metadata_paged_release_blob(paged_blob);
                    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
                    paged_blob = metadata_paged_allocate_blob(mdo, (fbe_u32_t)block_count);
                    paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
                    paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
                    mdo->paged_blob = paged_blob;
                }
                else
                {
                    paged_blob->slot_count = (fbe_u32_t)block_count;
                    paged_blob->block_count = (fbe_u32_t)block_count;
                }
            }
        }

        lba_offset = (mdo->u.metadata.offset + mdo->u.metadata.current_count_private *  mdo->u.metadata.record_data_size) / FBE_METADATA_BLOCK_DATA_SIZE;
        //metadata_align_io(mdo->metadata_element, &lba_offset, &block_count);
        
        /* Align lba_offset to 4K */
        lba_offset &= ~0x0000000000000007;

        paged_blob->lba = lba_offset; 
        paged_blob->block_count = block_count;
        status = metadata_paged_read_blob(packet, paged_blob);

        return FBE_STATUS_PENDING;
    }/* if(mdo->u.metadata.repeat_count > 1){  No caching */
    
    /* Small change */
    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
    paged_blob = metadata_paged_get_blob(mdo, &in_progress);

    if(paged_blob == NULL){         
        if(in_progress == FBE_TRUE){/* Some other blob in progress - we will perform non cached read */
            paged_blob = metadata_paged_allocate_blob(mdo, METADATA_PAGED_BLOB_SLOTS);
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
            mdo->paged_blob = paged_blob;
        } else { /* We will perform cached read */
            paged_blob = metadata_paged_allocate_blob(mdo, METADATA_PAGED_BLOB_SLOTS);
            fbe_queue_push(&metadata_element->paged_blob_queue_head, &paged_blob->queue_element);
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
        }

        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

        status = metadata_paged_read_blob(packet, paged_blob);
        return status;
    }

    /* We have outstanding blob for this area */
    if(in_progress == FBE_TRUE){ /* We have outstanding I/O for this area */
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "Paged Stripe Lock Error\n");
        fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } 	
    
    /* We have relevant blob to make a change */
    metadata_paged_mdo_to_blob(paged_blob, mdo, &is_write_required);

    if(is_write_required && !(mdo->u.metadata.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NO_PERSIST)){
        /* Modify bits and do the write */
        paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
    } else {
        if(mdo->u.metadata.repeat_count == 1){
            metadata_paged_blob_to_mdo(paged_blob, mdo);
            mdo->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_VALID_DATA;
        }   

        paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    metadata_paged_write_blob(packet, paged_blob);

    return FBE_STATUS_PENDING;
}

static fbe_status_t 
metadata_paged_write_verify(fbe_packet_t * packet)
{
    //fbe_status_t           status;    
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    metadata_paged_blob_t * paged_blob = NULL;
    fbe_lba_t lba_offset;
    fbe_block_count_t block_count;
    fbe_bool_t is_write_requiered;
    fbe_bool_t is_read_requiered = FBE_FALSE;

    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo =  metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;
	
	/* How many blocks remains to be written ? */
	block_count = ((mdo->u.metadata.repeat_count - mdo->u.metadata.current_count_private) * mdo->u.metadata.record_data_size) / FBE_METADATA_BLOCK_DATA_SIZE;
	if(((mdo->u.metadata.repeat_count - mdo->u.metadata.current_count_private) * mdo->u.metadata.record_data_size) % FBE_METADATA_BLOCK_DATA_SIZE){
		block_count += 1; /* If we are not block aligned we need to read additional block */
		is_read_requiered = FBE_TRUE;
	}

	/* Align block_count to 4K */
	if(block_count & 0x0000000000000007){ /* Not aligned  to 4K */
		block_count = (block_count + 8) & ~0x0000000000000007;
	}

	block_count = FBE_MIN(METADATA_PAGED_MAX_SGL, block_count); /* How much we can write ? */

	if(mdo->u.metadata.current_count_private == 0){ /* Check if it is first write */
		fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
		metadata_paged_release_overlapping_blobs(mdo);
		fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

           /* the paged data is NOT cached, read from disk */
        if (mdo->u.metadata.client_blob == NULL)
        {
            paged_blob = metadata_paged_allocate_blob(mdo, (fbe_u32_t)block_count);
        }
        else
        {
            paged_blob = (metadata_paged_blob_t *)(mdo->u.metadata.client_blob);
            paged_blob->slot_count = (fbe_u32_t)block_count;
            paged_blob->block_count = (fbe_u32_t)block_count;
        }
        paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
        paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
        mdo->paged_blob = paged_blob;
    } else { /* Blob is already allocated */
        paged_blob = mdo->paged_blob;
        /* Check if block count much the number of slots */
        if(paged_blob->slot_count != block_count){ /* release the blob and allocate new one */
            if (mdo->u.metadata.client_blob == NULL)
            {
                fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
                metadata_paged_release_blob(paged_blob);
                fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
                paged_blob = metadata_paged_allocate_blob(mdo, (fbe_u32_t)block_count);
                paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
                paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
                mdo->paged_blob = paged_blob;
                //metadata_paged_mdo_to_blob(paged_blob, mdo, &is_write_requiered);
            }
            else
            {
                paged_blob->slot_count = (fbe_u32_t)block_count;
                paged_blob->block_count = (fbe_u32_t)block_count;
            }
        }
    }

    lba_offset = (mdo->u.metadata.offset + mdo->u.metadata.current_count_private *  mdo->u.metadata.record_data_size) / FBE_METADATA_BLOCK_DATA_SIZE;    	
    paged_blob->lba = lba_offset; 
    paged_blob->block_count = block_count;

    /* Check alignment */
    if((lba_offset * FBE_METADATA_BLOCK_DATA_SIZE) != (mdo->u.metadata.offset + mdo->u.metadata.current_count_private *  mdo->u.metadata.record_data_size)){
        is_read_requiered = FBE_TRUE;
    }

    if(is_read_requiered){
        metadata_paged_read_blob(packet, paged_blob);
    } else {
        metadata_paged_mdo_to_blob(paged_blob, mdo, &is_write_requiered);	
        metadata_paged_write_blob(packet, paged_blob);
    }

    return FBE_STATUS_PENDING;
}



static fbe_status_t 
metadata_paged_write(fbe_packet_t * packet)
{
    //fbe_status_t           status;    
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    metadata_paged_blob_t * paged_blob = NULL;
    fbe_lba_t lba_offset;
    fbe_block_count_t block_count;
    fbe_bool_t is_write_requiered;

    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo =  metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;
    
    /* How many blocks remains to be written ? */
    block_count = ((mdo->u.metadata.repeat_count - mdo->u.metadata.current_count_private) * mdo->u.metadata.record_data_size) / FBE_METADATA_BLOCK_DATA_SIZE;
    if(((mdo->u.metadata.repeat_count - mdo->u.metadata.current_count_private) * mdo->u.metadata.record_data_size) % FBE_METADATA_BLOCK_DATA_SIZE){
        block_count += 1; /* If we are not block aligned we need to read additional block */
    }

    /* Align block_count to 4K */
    if(block_count & 0x0000000000000007){ /* Not aligned  to 4K */
        block_count = (block_count + 8) & ~0x0000000000000007;
    }

    block_count = FBE_MIN(METADATA_PAGED_MAX_SGL, block_count); /* How much we can write ? */

    if(mdo->u.metadata.current_count_private == 0){ /* Check if it is first write */
        /* Sanity checks */
        if(mdo->u.metadata.offset % FBE_METADATA_BLOCK_DATA_SIZE){ /* Offset is not block aligned */
            metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "Paged Alignment Error\n");
            
            fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        if((mdo->u.metadata.record_data_size * mdo->u.metadata.repeat_count) % FBE_METADATA_BLOCK_DATA_SIZE){ /* repeat_count is not block aligned */
            metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "Paged Alignment Error\n");
            fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
        metadata_paged_release_overlapping_blobs(mdo);
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

        /* the paged data is NOT cached, read from disk */
        if (mdo->u.metadata.client_blob == NULL)
        {
            paged_blob = metadata_paged_allocate_blob(mdo, (fbe_u32_t)block_count);
        }
        else
        {
            paged_blob = (metadata_paged_blob_t *)(mdo->u.metadata.client_blob);
            paged_blob->slot_count = (fbe_u32_t)block_count;
            paged_blob->block_count = (fbe_u32_t)block_count;
        }
        paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
        paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
        mdo->paged_blob = paged_blob;
    } else { /* Blob is already allocated */
        paged_blob = mdo->paged_blob;
        /* Check if block count much the number of slots */
        if(paged_blob->slot_count != block_count){ /* release the blob and allocate new one */
            if (mdo->u.metadata.client_blob == NULL)
            {
            fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
            metadata_paged_release_blob(paged_blob);
            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
            paged_blob = metadata_paged_allocate_blob(mdo, (fbe_u32_t)block_count);
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
            mdo->paged_blob = paged_blob;
            //metadata_paged_mdo_to_blob(paged_blob, mdo, &is_write_requiered);
            }
            else
            {
                paged_blob->slot_count = (fbe_u32_t)block_count;
                paged_blob->block_count = (fbe_u32_t)block_count;
            }
        }
    }

    lba_offset = (mdo->u.metadata.offset + mdo->u.metadata.current_count_private *  mdo->u.metadata.record_data_size) / FBE_METADATA_BLOCK_DATA_SIZE;    	
    /* Align lba_offset to 4K */
    lba_offset &= ~0x0000000000000007;

    paged_blob->lba = lba_offset; 
    paged_blob->block_count = block_count;

    metadata_paged_mdo_to_blob(paged_blob, mdo, &is_write_requiered);	

    metadata_paged_write_blob(packet, paged_blob);

    return FBE_STATUS_PENDING;
}

static fbe_status_t 
metadata_paged_read(fbe_packet_t * packet)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    metadata_paged_blob_t * paged_blob = NULL;
    fbe_status_t status;
    fbe_bool_t in_progress = FBE_FALSE;
    
    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo =  metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;
    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

    /* If FUA is specified we must read from disk */
    if ((mdo->u.metadata.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_FLAGS_FUA_READ) != 0) {
        /* We must read from disk */
        if(mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_NEXT_MARKED_BITS){
            paged_blob = metadata_paged_allocate_blob(mdo, METADATA_PAGED_BLOB_SLOTS);
        } else { /* It is just get bits, so 2 slots would be enough */
            paged_blob = metadata_paged_allocate_blob(mdo, METADATA_PAGED_BLOB_SLOTS);
        }
        paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
        mdo->paged_blob = paged_blob;
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        status = metadata_paged_read_blob(packet, paged_blob);
        return status;
    }

    /* Check if we have this area in the cache */
    /* Attention! get bits size sould fit well in 512 bytes */
    paged_blob = metadata_paged_get_blob(mdo, &in_progress);
    if(paged_blob != NULL){ /* We have outstanding blob for this area */
        if(in_progress == FBE_TRUE){ /* We have outstanding read for this area */
#if 0
            fbe_transport_enqueue_packet(packet, &paged_blob->packet_queue); /* Enqueue the packet and it will be processed when the read completed */
            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
#endif
            /* we will perform non cached read */
            paged_blob = NULL;

        } else { /* We have valid data in the blob */
            status = metadata_paged_blob_to_mdo(paged_blob, mdo); /* This function will set metadata status in mdo */
            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }        
    }

    if(paged_blob == NULL){
        if ((mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_NEXT_MARKED_BITS) &&
            (mdo->u.metadata.client_blob != NULL)) {
            /* Allow client to provide their own blob to perform long get next scans.
             */
            paged_blob = mdo->u.next_marked_chunk.metadata.client_blob;
        } else if(in_progress == FBE_TRUE){/* Some other blob in progress - we will perform non cached read */
            if(mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_NEXT_MARKED_BITS){
                paged_blob = metadata_paged_allocate_blob(mdo, METADATA_PAGED_BLOB_SLOTS);
            } else { /* It is just get bits, so 2 slots would be enough */
                paged_blob = metadata_paged_allocate_blob(mdo, METADATA_PAGED_BLOB_SLOTS);
            }
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
        } else { /* We will perform cached read */
            paged_blob = metadata_paged_allocate_blob(mdo, METADATA_PAGED_BLOB_SLOTS);
            fbe_queue_push(&metadata_element->paged_blob_queue_head, &paged_blob->queue_element);
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
        }

        mdo->paged_blob = paged_blob;

        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

        status = metadata_paged_read_blob(packet, paged_blob);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t 
metadata_paged_send_io_packet(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * new_packet = (fbe_packet_t *)context;
    fbe_status_t status;

    fbe_transport_set_packet_attr(new_packet, FBE_PACKET_FLAG_CONSUMED);

    status = fbe_topology_send_io_packet(new_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t 
metadata_paged_read_blob_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t *  master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_ex_t * master_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_status_t status;
    fbe_payload_metadata_status_t metadata_status;
    fbe_payload_metadata_status_t previous_metadata_status;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    metadata_paged_blob_t * paged_blob = NULL;
    fbe_object_id_t object_id;
    fbe_queue_head_t            tmp_queue;
    fbe_u32_t i = 0;
    fbe_bool_t is_write_requiered;

    fbe_queue_init(&tmp_queue);
    
    paged_blob = (metadata_paged_blob_t *)context;

    paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_payload = fbe_transport_get_payload_ex(master_packet);
    mdo = metadata_paged_get_metadata_operation(master_payload);
    fbe_payload_metadata_get_status(mdo, &previous_metadata_status);
    metadata_element = mdo->metadata_element;
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);
    
    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation,&block_operation_status);

    if(block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS){
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s object_id: 0x%X lba 0x%llX bc 0x%llX BOS %d\n", __FUNCTION__, object_id,
                                                            metadata_element->paged_record_start_lba + paged_blob->lba,
                                                            paged_blob->block_count,
                                                            block_operation_status);
    }

    fbe_payload_block_get_qualifier(block_operation,&block_operation_qualifier);

    fbe_payload_ex_release_block_operation(sep_payload, block_operation);

    /* remove the subpacket */
    fbe_transport_remove_subpacket(packet);

    /* Release the memory */
    fbe_transport_destroy_packet(packet); /* This just free up packet resources, but not packet itself */

    metadata_status = fbe_metadata_determine_status(status, block_operation_status, block_operation_qualifier);
    if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE){
        mdo->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NEED_VERIFY;
    }

    if(metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK){
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s object_id: 0x%X lba 0x%llX bc 0x%llX MDS %d\n", __FUNCTION__, object_id,
                                                            metadata_element->paged_record_start_lba + paged_blob->lba,
                                                            paged_blob->block_count,
                                                            metadata_status);
    }

    if ((metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK) && (metadata_status != FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE)) {
        metadata_trace(FBE_TRACE_LEVEL_WARNING, object_id,
                       "mdata_paged_rd_blob_compl I/O fail, stat: %d bl_stat: %d bl_qualifier: %d\n",
                       status, block_operation_status, block_operation_qualifier);

        fbe_payload_metadata_set_status(mdo, metadata_status);

        /* This is the case where the client calls write verify because of errors during a read
         * in the first place and so, if this is a write verify opcode, just write what the client
         * wants and fill the rest with Zero and write it out
         */
        if(mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY /*||
            mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE*/){
            if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE){
                /* Zero blob */
                for(i = 0; i < paged_blob->slot_count; i++){
                        fbe_zero_memory(paged_blob->sg_list[i].address, paged_blob->sg_list[i].count);
                        fbe_metadata_calculate_checksum(paged_blob->sg_list[i].address,  1);
                }
                        
                metadata_paged_mdo_to_blob(paged_blob, mdo, &is_write_requiered);
                metadata_paged_write_blob(master_packet, paged_blob);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
        }

        /* We have to release blob here */
        fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

#if 0
        /* We need to process all requests from packet_queue */
        if(!fbe_queue_is_empty(&paged_blob->packet_queue)){
            /* This can be get or scan for bits operations ONLY */
            fbe_packet_t *  p = NULL;
            fbe_payload_ex_t * pl = NULL;
            fbe_payload_metadata_operation_t * mdo = NULL;

            while(p = fbe_transport_dequeue_packet(&paged_blob->packet_queue)){
                pl = fbe_transport_get_payload_ex(p);
                mdo = metadata_paged_get_metadata_operation(pl);
                fbe_payload_metadata_set_status(mdo, metadata_status);

                fbe_transport_set_status(p, FBE_STATUS_OK, 0);
                fbe_queue_push(&tmp_queue, &p->queue_element);
            }
        }
#endif

        metadata_paged_release_blob(paged_blob);
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

        fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
        fbe_queue_destroy(&tmp_queue);		

        fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    
    if(mdo->u.metadata.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NEED_VERIFY){
        fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE);
    } else {
        fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
    }

    if(mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS		||
        mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_CLEAR_BITS	||
        mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_XOR_BITS		||
        mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY ||
        mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE){
        
        /* Change the data and issue blob write */
        metadata_paged_mdo_to_blob(paged_blob, mdo, &is_write_requiered);
        if(is_write_requiered || metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE){
            if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE){
                paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_WRITE_VERIFY_NEEDED;
            }
            metadata_paged_write_blob(master_packet, paged_blob);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        /* Let's see if we need to continue (check the repeat count) */
        if((mdo->u.metadata.repeat_count > 1) && (mdo->u.metadata.repeat_count > mdo->u.metadata.current_count_private)){
            /* OK let's do it again */
            metadata_paged_switch(master_packet, (fbe_packet_completion_context_t *)mdo);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        if(mdo->u.metadata.repeat_count == 1){
            metadata_paged_blob_to_mdo(paged_blob, mdo);
            mdo->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_VALID_DATA;
        }   

        fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
        paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
        if(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_PRIVATE){
            metadata_paged_release_blob(paged_blob);
        } else {
            paged_blob->time_stamp = fbe_get_time(); 
        }

        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

    /* read was successful, free the resources and complete the packet */
    status = metadata_paged_blob_to_mdo(paged_blob, mdo);
    fbe_payload_metadata_set_status(mdo, metadata_status);

#if 0
    /* We need to process all requests from packet_queue */
    if(!fbe_queue_is_empty(&paged_blob->packet_queue)){
        /* This can be get or scan for bits operations ONLY */
        fbe_packet_t *  p = NULL;
        fbe_payload_ex_t * pl = NULL;
        fbe_payload_metadata_operation_t * mdo = NULL;

        while(p = fbe_transport_dequeue_packet(&paged_blob->packet_queue)){
            pl = fbe_transport_get_payload_ex(p);
            mdo = metadata_paged_get_metadata_operation(pl);
            status = metadata_paged_blob_to_mdo(paged_blob, mdo);

            if(status == FBE_STATUS_OK){
                fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
            } else {
                fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
            }
            fbe_transport_set_status(p, FBE_STATUS_OK, 0);
            fbe_queue_push(&tmp_queue, &p->queue_element);
        }
    }
#endif

    paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
    if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE) {
        /* mark the blob, it needs a write_verify on the next write */
        paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_WRITE_VERIFY_NEEDED;
    }

    if(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_PRIVATE){
        metadata_paged_release_blob(paged_blob);
    } else {
        paged_blob->time_stamp = fbe_get_time(); 
    }

    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
    fbe_queue_destroy(&tmp_queue);		

    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

fbe_status_t fbe_metadata_element_abort_paged(fbe_metadata_element_t* metadata_element)
{
    //metadata_paged_blob_t * paged_blob = NULL;
    fbe_packet_t						* new_packet = NULL;
    fbe_payload_ex_t                    * sep_payload = NULL;
    fbe_payload_metadata_operation_t    * mdo = NULL;
    fbe_queue_head_t tmp_queue;

    fbe_queue_init(&tmp_queue);

    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

    fbe_atomic_32_or(&metadata_element->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_FAIL_PAGED);

	while(new_packet = fbe_transport_dequeue_packet(&metadata_element->paged_record_queue_head)){
		sep_payload = fbe_transport_get_payload_ex(new_packet);
		mdo = metadata_paged_get_metadata_operation(sep_payload);
		fbe_transport_set_status(new_packet, FBE_STATUS_OK, 0);
		fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_ABORTED);
		fbe_queue_push(&tmp_queue, &new_packet->queue_element);
	}

#if 0

	paged_blob = (metadata_paged_blob_t *)fbe_queue_front(&metadata_element->paged_blob_queue_head);

    if(paged_blob != NULL){
        while(new_packet = fbe_transport_dequeue_packet(&paged_blob->packet_queue)){
            sep_payload = fbe_transport_get_payload_ex(new_packet);
            mdo = metadata_paged_get_metadata_operation(sep_payload);
            fbe_transport_set_status(new_packet, FBE_STATUS_OK, 0);
            fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_ABORTED);
            fbe_queue_push(&tmp_queue, &new_packet->queue_element);
        }
    }
#endif

    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
    fbe_queue_destroy(&tmp_queue);

    /* We drain the metadata element first.
     * We need to stop initiaing I/Os on this path before we can 
     * quiesce the block transport server.  Otherwise a deadlock will occur. 
     */ 
    //return fbe_metadata_paged_drain_io(metadata_element);
    return FBE_STATUS_OK;
}   

fbe_status_t fbe_metadata_element_is_quiesced(fbe_metadata_element_t* metadata_element,
                                              fbe_bool_t *b_quiesced_p)

{
    fbe_status_t status;

    /* Before we drain, we need to prevent any new requests from coming in.
     * This prevents new requests from starting while we are draining and 
     * after we have finished draining, but before we have quiesced the client object. 
     */
    status = fbe_metadata_element_abort_paged(metadata_element);

    if (status != FBE_STATUS_OK)
    {
         metadata_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s status from abort paged %d\n", __FUNCTION__, status);
    }

    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
    if(!fbe_queue_is_empty(&metadata_element->paged_record_queue_head)) { /* We have outstanding paged requests */
        *b_quiesced_p = FBE_FALSE;
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
    } else {
        /* Mark the metadata element held for stripe locks. 
         * This needs to occur after we hold the paged metadata. 
         */
        /* Fix for AR 546819 - 
            sep!fbe_metadata_paged_release_blobs             ---> try to acquire the metadata_element->paged_record_queue_lock lock again
            sep!fbe_metadata_stripe_lock_element_abort_impl
            sep!fbe_metadata_stripe_lock_element_abort
        */
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

        fbe_metadata_stripe_lock_element_abort(metadata_element);
        *b_quiesced_p = FBE_TRUE;
    }
    return FBE_STATUS_OK;
}


fbe_status_t
fbe_metadata_paged_init_queue(fbe_metadata_element_t * metadata_element)
{
    fbe_queue_init(&metadata_element->paged_record_queue_head);

    if(FBE_FALSE == fbe_metadata_element_is_element_reinited(metadata_element))
    {
        fbe_spinlock_init(&metadata_element->paged_record_queue_lock);
    }
    else
    {
        /*metadata element reinitialize case: spinlock is never destroyed, so no need
         *to init it here*/
    }
    
    metadata_element->paged_record_request_count = 0;
    /* Initialize blob queue */
    fbe_queue_init(&metadata_element->paged_blob_queue_head);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_metadata_paged_destroy_queue(fbe_metadata_element_t * metadata_element)
{
    fbe_queue_destroy(&metadata_element->paged_record_queue_head);
    /* Release all blobs here */

    fbe_spinlock_destroy(&metadata_element->paged_record_queue_lock);
    return FBE_STATUS_OK;
}

static fbe_payload_metadata_status_t fbe_metadata_determine_status(fbe_status_t status,
                              fbe_payload_block_operation_status_t  block_operation_status,
                              fbe_payload_block_operation_qualifier_t  block_operation_qualifier)
{   
    fbe_payload_metadata_status_t metadata_status;

    if ((status != FBE_STATUS_OK) ||
        (block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        if (status != FBE_STATUS_OK)
        {
            if (status == FBE_STATUS_EDGE_NOT_ENABLED)
            {
                metadata_status = FBE_PAYLOAD_METADATA_STATUS_IO_RETRYABLE;
            }
            else
            {
                metadata_status = FBE_PAYLOAD_METADATA_STATUS_FAILURE;
            }
        }
        else
        {
            switch (block_operation_status)
            {
            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT:
                metadata_status = FBE_PAYLOAD_METADATA_STATUS_TIMEOUT;
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:
            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
                switch (block_operation_qualifier)
                {
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE:
                    metadata_status = FBE_PAYLOAD_METADATA_STATUS_IO_NOT_RETRYABLE;
                    break;
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR:
                    metadata_status = FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE;
                    break;
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_LOCK_FAILED:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WRITE_LOG_ABORTED:
                    metadata_status = FBE_PAYLOAD_METADATA_STATUS_IO_RETRYABLE;
                    break;
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_KEY_WRAP_ERROR:
                    metadata_status = FBE_PAYLOAD_METADATA_STATUS_KEY_WRAP_ERROR;
                    break;
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_BAD_KEY_HANDLE:
                    metadata_status = FBE_PAYLOAD_METADATA_STATUS_BAD_KEY_HANDLE;
                    break;
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ENCRYPTION_NOT_ENABLED:
                    metadata_status = FBE_PAYLOAD_METADATA_STATUS_ENCRYPTION_NOT_ENABLED;
                    break;
                default:
                    metadata_status = FBE_PAYLOAD_METADATA_STATUS_FAILURE;
                    metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "mdata_determine_stat unhandled block stat:%d qualifier:%d\n", 
                            block_operation_status, block_operation_qualifier);
                    break;
                }
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST:
                metadata_status = FBE_PAYLOAD_METADATA_STATUS_FAILURE;
                break;
            default:
                metadata_status = FBE_PAYLOAD_METADATA_STATUS_FAILURE;
                metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "mdata_determine_stat unhandled block stat:%d qualifier:%d\n", 
                        block_operation_status, block_operation_qualifier);
                break;
            }
        }
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "mdata_determine_stat I/O fail,packet stat:%d block stat:%d qualifier:%d,mdata stat: %d\n", 
                       status, block_operation_status, block_operation_qualifier, metadata_status);
    }
    else if (block_operation_qualifier ==  FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED) {
        /* Correctable/recoverable errors Like soft media errors.
           It is possible for the read to succeed but for an error to get returned.
           This corresponds to block status of FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS
           with a qualifier of FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED.
           These blocks can be read but need to be remapped by writing them with a write-verify opcode.
           */
        metadata_status = FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE;
    }
    else{
        metadata_status = FBE_PAYLOAD_METADATA_STATUS_OK;
    }
    
    return metadata_status;
}


/*!****************************************************************************
 * fbe_metadata_translate_metadata_status_to_block_status()
 ******************************************************************************
 * @brief
 *   This function will take a metadata status as input and determine the 
 *   appropriate block operation status for it.
 * 
 * @param in_metadata_status    - metadata status
 * @param out_packet_status_p   - pointer to data that gets block status.
 * @param out_block_qualifier_p - pointer to data that gets qualifier.
 * 
 * @return fbe_status_t     
 *
 * @author
 *   06/06/2012 - Created. Prahlad Purohit. 
 *
 ******************************************************************************/
fbe_status_t fbe_metadata_translate_metadata_status_to_block_status(
                                    fbe_payload_metadata_status_t         in_metadata_status,                   
                                    fbe_payload_block_operation_status_t* out_block_status_p,
                                    fbe_payload_block_operation_qualifier_t *out_block_qualifier_p)
{
    switch (in_metadata_status)
    {
        case FBE_PAYLOAD_METADATA_STATUS_OK:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
            break;

        case FBE_PAYLOAD_METADATA_STATUS_FAILURE:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;            
            break;

        case FBE_PAYLOAD_METADATA_STATUS_BUSY:
        case FBE_PAYLOAD_METADATA_STATUS_TIMEOUT:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
            break;

        case FBE_PAYLOAD_METADATA_STATUS_CANCELLED:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED;
            break;

            /* If the client aborted the request, it is retryable.
             */
        case FBE_PAYLOAD_METADATA_STATUS_ABORTED:
        case FBE_PAYLOAD_METADATA_STATUS_IO_RETRYABLE:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
            break;

        case FBE_PAYLOAD_METADATA_STATUS_IO_NOT_RETRYABLE:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;            
            break;

        case FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST;
            break;            

        case FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED;
            break;

        default:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
            break;
    }

    //  Return status  
    return FBE_STATUS_OK;  

} // End fbe_metadata_translate_metadata_status_to_block_status()

/*!****************************************************************************
 * fbe_metadata_translate_metadata_status_to_packet_status()
 ******************************************************************************
 * @brief
 *   This function will take a metadata status as input and determine the 
 *   appropriate packet status for it.  It will return that packet status in 
 *   the output parameter.
 * 
 * @param in_metadata_status    - metadata status
 * @param out_packet_status_p   - pointer to data that gets set to the packet status
 *                                that corresponds to this metadata status 
 * 
 * @return fbe_status_t     
 *
 * @author
 *  04/10/2013  Ron Proulx  - Created from raid group translate. 
 *
 ******************************************************************************/
fbe_status_t fbe_metadata_translate_metadata_status_to_packet_status(fbe_payload_metadata_status_t in_metadata_status,
                                                                     fbe_status_t *out_packet_status_p)                  
{
    /* Determine the packet status corresponding to the type of metadata status
     */
    switch (in_metadata_status)
    {
        case FBE_PAYLOAD_METADATA_STATUS_OK:
        case FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE:
            /*! @note correctable has correct data, return ok for now 
             */
            *out_packet_status_p = FBE_STATUS_OK;
            break;

        case FBE_PAYLOAD_METADATA_STATUS_BUSY:
        case FBE_PAYLOAD_METADATA_STATUS_TIMEOUT:
        case FBE_PAYLOAD_METADATA_STATUS_ABORTED:
        case FBE_PAYLOAD_METADATA_STATUS_IO_RETRYABLE:
            /* A metadata status "not retryable" error is set when quiescing 
             * metadata I/O or when a "retry not possible" error is returned 
             * from RAID for metadata I/O.  This is a retryable error now.
             */
            *out_packet_status_p = FBE_STATUS_IO_FAILED_RETRYABLE;
            break;


        case FBE_PAYLOAD_METADATA_STATUS_IO_NOT_RETRYABLE:
        case FBE_PAYLOAD_METADATA_STATUS_FAILURE:
        case FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE:
            /* Set packet status to "not retryable" for those metadata status 
             * values that imply the I/O should be failed.
             */
            *out_packet_status_p = FBE_STATUS_IO_FAILED_NOT_RETRYABLE;
            break;

        case FBE_PAYLOAD_METADATA_STATUS_CANCELLED:
            /* Set packet status to cancelled if the metadata request was 
             * aborted.
             */ 
            *out_packet_status_p = FBE_STATUS_CANCELED;
            break;

        default:
            /* For an unsupported status, return generic failure.
             */
            metadata_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s unsupported metadata status: 0x%x \n",
                           __FUNCTION__, in_metadata_status);
            *out_packet_status_p = FBE_STATUS_GENERIC_FAILURE;
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return success
     */  
    return FBE_STATUS_OK;  

}
/*****************************************************************
 * end fbe_metadata_translate_metadata_status_to_packet_status()
 ****************************************************************/

static fbe_status_t
metadata_paged_mdo_to_lba(fbe_payload_metadata_operation_t * mdo, fbe_lba_t * first, fbe_lba_t * last)
{
    fbe_lba_t first_lba = 0;
    fbe_lba_t last_lba = 0;

    first_lba = mdo->u.metadata.offset / FBE_METADATA_BLOCK_DATA_SIZE;

    /* For the 4K support we have to align first LBA to 4K */
    first_lba &= ~0x0000000000000007;

    if(mdo->u.metadata.repeat_count >= 1){
        last_lba = (mdo->u.metadata.offset + mdo->u.metadata.repeat_count * mdo->u.metadata.record_data_size) / FBE_METADATA_BLOCK_DATA_SIZE;
        if((mdo->u.metadata.offset + mdo->u.metadata.repeat_count * mdo->u.metadata.record_data_size) % FBE_METADATA_BLOCK_DATA_SIZE){
            last_lba += 1;
        }
    } else {
        last_lba = (mdo->u.metadata.offset + mdo->u.metadata.record_data_size) / FBE_METADATA_BLOCK_DATA_SIZE;
        if((mdo->u.metadata.offset + mdo->u.metadata.record_data_size) % FBE_METADATA_BLOCK_DATA_SIZE){
            last_lba += 1;
        }
    }

    /* For the 4K support we have to operate in 4K * 2 minimum resolution */
    if(last_lba - first_lba < METADATA_PAGED_BLOB_SLOTS){
        last_lba = first_lba + METADATA_PAGED_BLOB_SLOTS;
    } else if(last_lba - first_lba > METADATA_PAGED_BLOB_SLOTS) {
        /* Align last_lba to 4K if needed */ 
        if(last_lba & 0x0000000000000007){ /* Not aligned */
            last_lba = (last_lba + 8) & ~0x0000000000000007;
        }
    }

    /* if last_lba crosses over metadata end range, then back up the lba range.*/
    /*    meta data range    |-------------|
          lba range                      |--|  <- shift by 4K */
    if (last_lba > mdo->metadata_element->paged_mirror_metadata_offset)
    {
        first_lba -= 8;
        last_lba -= 8;
    }

    * first = first_lba;
    * last = last_lba - 1;

    return FBE_STATUS_OK;
}


static metadata_paged_blob_t *
metadata_paged_get_blob(fbe_payload_metadata_operation_t * mdo, fbe_bool_t * in_progress)
{
    fbe_metadata_element_t * metadata_element = NULL;
    metadata_paged_blob_t * paged_blob = NULL;
    fbe_lba_t first_lba;
    fbe_lba_t last_lba;

    * in_progress = FBE_FALSE;
    metadata_element = mdo->metadata_element;

    /* If local cache is used, we should have no paged blob cached here */
    if (fbe_metadata_element_is_paged_object_cache(metadata_element)) {
        paged_blob = (metadata_paged_blob_t *)fbe_queue_front(&metadata_element->paged_blob_queue_head);
        while(paged_blob != NULL){
            metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "Found cached paged blob!\n");
        }
        *in_progress = FBE_TRUE; /* Force to use private blob instead */
        return NULL;
    }

    metadata_paged_mdo_to_lba(mdo, &first_lba, &last_lba);
    
    paged_blob = (metadata_paged_blob_t *)fbe_queue_front(&metadata_element->paged_blob_queue_head);
    while(paged_blob != NULL){
        if((last_lba < paged_blob->lba) || ((paged_blob->lba + paged_blob->block_count) <= first_lba)){ /* No overlap */
            if(!(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS)){
                /* Check if the last access to this blob was a long time ago */
                if((paged_blob->time_stamp != 0) && ((fbe_get_time() - paged_blob->time_stamp) > 100)){
                    metadata_paged_release_blob(paged_blob); /* We did not used it for 100 ms. */
                } else {
                    * in_progress = FBE_TRUE; /* This will keep this blob in the cache */
                }
            } else {
                * in_progress = FBE_TRUE;
            }
        } else if((first_lba >= paged_blob->lba) && (last_lba < paged_blob->lba + paged_blob->block_count)) { /* Full overlapped */
            if(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS){
                * in_progress = FBE_TRUE;
            }
            paged_blob->time_stamp = fbe_get_time(); 
            return paged_blob;
        } else { /* Partial overlap */
            if(!(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS)){
                metadata_paged_release_blob(paged_blob);
            } else {
                * in_progress = FBE_TRUE;
            }
        }
        /* Temporarily we will have one blob per metadata element */
        //paged_blob = (metadata_paged_blob_t *)fbe_queue_next(&metadata_element->paged_blob_queue_head, &current_paged_blob->queue_element);
        paged_blob = NULL;
    }
    return NULL;
}



/* paged_record_queue_lock MUST be held */
static fbe_status_t
metadata_paged_release_overlapping_blobs(fbe_payload_metadata_operation_t * mdo)
{
    fbe_metadata_element_t * metadata_element = NULL;
    metadata_paged_blob_t * paged_blob = NULL;
    fbe_lba_t first_lba = 0;
    fbe_lba_t last_lba = 0;

    metadata_element = mdo->metadata_element;

    metadata_paged_mdo_to_lba(mdo, &first_lba, &last_lba);
    
    paged_blob = (metadata_paged_blob_t *)fbe_queue_front(&metadata_element->paged_blob_queue_head);
    while(paged_blob != NULL){
        if((last_lba < paged_blob->lba) || ((paged_blob->lba + paged_blob->block_count) <= first_lba)){ /* No overlap */

        } else { /* Overlapped */
            if(!(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS)){
                metadata_paged_release_blob(paged_blob);
            } else {
                metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "Paged Stripe Lock Error\n");
            }
            return FBE_STATUS_OK;
        }
        /* Temporarily we will have one blob per metadata element */
        //paged_blob = (metadata_paged_blob_t *)fbe_queue_next(&metadata_element->paged_blob_queue_head, &current_paged_blob->queue_element);
        paged_blob = NULL;
    }
    return FBE_STATUS_OK;
}

/* paged_record_queue_lock MUST be held */
static metadata_paged_blob_t *
metadata_paged_allocate_blob(fbe_payload_metadata_operation_t * mdo, fbe_u32_t slot_count)
{
    fbe_metadata_element_t * metadata_element = NULL;
    metadata_paged_blob_t * paged_blob = NULL;
    metadata_paged_slot_t * paged_slot = NULL;
    fbe_lba_t operation_lba;
    fbe_u32_t i;
    fbe_lba_t								stripe_offset;
    fbe_block_count_t						stripe_count;
    fbe_lba_t first_lba;
    fbe_lba_t last_lba;

    metadata_element = mdo->metadata_element;

    metadata_paged_mdo_to_lba(mdo, &first_lba, &last_lba);

    operation_lba = first_lba;

    //metadata_align_io(mdo->metadata_element, &operation_lba, &blocks);

    /* Check if slot count aligned */
    if(slot_count & 0x07){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s Unaligned slot_count 0x%x \n", __FUNCTION__, slot_count);
    }


    /* Take the blob out of blob queue */
    fbe_spinlock_lock(&metadata_paged_blob_queue_lock);

    paged_blob = (metadata_paged_blob_t *)fbe_queue_pop(&metadata_paged_blob_queue);
    if(paged_blob == NULL){
        fbe_spinlock_unlock(&metadata_paged_blob_queue_lock);
        return NULL;
    }
    for(i = 0; i < slot_count; i++){
        paged_slot = (metadata_paged_slot_t *)fbe_queue_pop(&metadata_paged_slot_queue);
        if(paged_slot == NULL){
            metadata_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s no slot left slot_count 0x%x \n", __FUNCTION__, slot_count);
            break;
        }
        paged_blob->sg_list[i].address = paged_slot->data;
        paged_blob->sg_list[i].count = FBE_METADATA_BLOCK_SIZE;
    }

    paged_blob->slot_count = slot_count;
    fbe_spinlock_unlock(&metadata_paged_blob_queue_lock);

    paged_blob->sg_list[i].address = NULL;
    paged_blob->sg_list[i].count = 0;
        
    paged_blob->metadata_element = metadata_element;
    paged_blob->lba = operation_lba /* - (operation_lba % (paged_blob->slot_count / 2))*/; 
    paged_blob->block_count = slot_count; /* number of slots currently allocated */

    fbe_payload_metadata_get_metadata_stripe_offset(mdo, &stripe_offset);
    fbe_payload_metadata_get_metadata_stripe_count(mdo, &stripe_count);

    paged_blob->stripe_offset = stripe_offset; 
    paged_blob->stripe_count = stripe_count;

    paged_blob->flags = 0;
    //fbe_queue_init(&paged_blob->packet_queue);
    paged_blob->time_stamp = 0;

    return paged_blob;
}

static __forceinline metadata_paged_slot_t *
metadata_paged_slot_data_to_paged_slot_base(fbe_u8_t *paged_slot_p)
{
    metadata_paged_slot_t * paged_slot_base_p;
    paged_slot_base_p = (metadata_paged_slot_t  *)(paged_slot_p - (fbe_u8_t *)(&((metadata_paged_slot_t  *)0)->data));
    return paged_slot_base_p;
}

/* Caller MUST hold the &metadata_element->paged_record_queue_lock */
static fbe_status_t
metadata_paged_release_blob(metadata_paged_blob_t * paged_blob)
{
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_u32_t i;

    if (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT)
    {
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s paged blob %p IO already in progress \n", __FUNCTION__, paged_blob);
    }

    if (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_BLOB)
    {
        return FBE_STATUS_OK;
    }

    metadata_element = paged_blob->metadata_element;
    
    /* Check packet queue */

    fbe_queue_remove(&paged_blob->queue_element);

    fbe_spinlock_lock(&metadata_paged_blob_queue_lock);

    /* Release all slots */
    for(i = 0; i < paged_blob->slot_count; i++){
        metadata_paged_slot_t * paged_slot_p = NULL;
        /* We need to upcast from the paged slot data (inside the sg entry), 
         * up to the paged slot info. 
         */
        paged_slot_p = metadata_paged_slot_data_to_paged_slot_base(paged_blob->sg_list[i].address);
        if(paged_slot_p == NULL){
            break;
        }
        fbe_queue_push(&metadata_paged_slot_queue, &paged_slot_p->queue_element);
    }

    fbe_queue_push(&metadata_paged_blob_queue, &paged_blob->queue_element);

    fbe_spinlock_unlock(&metadata_paged_blob_queue_lock);

    return FBE_STATUS_OK;
}

/* paged_record_queue_lock MUST be held */
static fbe_status_t
metadata_paged_blob_to_mdo(metadata_paged_blob_t * paged_blob, fbe_payload_metadata_operation_t * mdo)
{
    fbe_lba_t lba_offset;
    fbe_u64_t slot_number;
    fbe_u8_t * data_ptr;
    fbe_u32_t slot_offset;
    fbe_u8_t * dst_ptr;
    fbe_u32_t dst_size;
    fbe_payload_metadata_operation_opcode_t      opcode;

    lba_offset = mdo->u.metadata.offset / FBE_METADATA_BLOCK_DATA_SIZE;
    slot_number = lba_offset - paged_blob->lba;

    slot_offset = (fbe_u32_t)(mdo->u.metadata.offset - lba_offset * FBE_METADATA_BLOCK_DATA_SIZE);
    dst_size = mdo->u.metadata.record_data_size;
    if(slot_offset + dst_size > FBE_METADATA_BLOCK_DATA_SIZE){
        if(slot_number + 1 >= paged_blob->slot_count){ /* +1 we may need two blocks in edge case */
            metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s Invalid slot number = %d\n", __FUNCTION__, (int)slot_number);
            fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_payload_metadata_get_opcode(mdo, &opcode);
    if(opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_NEXT_MARKED_BITS){

        return metadata_paged_get_next_marked_chunk(paged_blob, mdo);
    } 

    data_ptr = ((fbe_u8_t *)paged_blob->sg_list[slot_number].address);
    slot_offset = (fbe_u32_t)(mdo->u.metadata.offset - lba_offset * FBE_METADATA_BLOCK_DATA_SIZE);
    data_ptr += slot_offset;

    dst_ptr = mdo->u.metadata.record_data;
    dst_size = mdo->u.metadata.record_data_size;

    if(slot_offset + dst_size <= FBE_METADATA_BLOCK_DATA_SIZE){
        fbe_copy_memory(dst_ptr, data_ptr, dst_size);
    } else {
        fbe_copy_memory(dst_ptr, data_ptr, FBE_METADATA_BLOCK_DATA_SIZE - slot_offset);
        dst_ptr += FBE_METADATA_BLOCK_DATA_SIZE - slot_offset;
        data_ptr = ((fbe_u8_t *)paged_blob->sg_list[slot_number+1].address);
        fbe_copy_memory(dst_ptr, data_ptr, dst_size - (FBE_METADATA_BLOCK_DATA_SIZE - slot_offset));
    }

    fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *  metadata_paged_get_next_marked_chunk()
 ******************************************************************************
 * @brief
 *  This function will go over blocks of data that is read from the drive
 *  and find out the next chunk that is marked for some operation that is requested
 *  by the client. We will loop over (METADATA_PAGED_BLOB_SLOTS >> 1) slots of data with each slot containing
 *  512 bytes of data. We will pass in the data that is requested by the client
 *  to the client provided callback function. The callback function will go over the data
 * and tell metadata service whether the chunk is marked or not.If the chunk is marked
 * then we will populate the current offset that is marked for rebuild,verify or zeroing
 * depending upon the client who called this function and return.
 * Current offset is basically the metadata offset of the chunk that is marked for
 * zeroing, verify or rebuild
 *  Here is the logic in a nutshell.
 * 
 *   1) Loop through from slot number to (METADATA_PAGED_BLOB_SLOTS >> 1) slots
     2) Loop through each slot which has 512 bytes of data starting from slot offset
     3) copy data to the search data and call the function
     4) If the function returns found, then calculate the current offset and return.
        else go to the next chunk of data either in the same slot or we might have to move
        to the next slot depending upon the slot offset and destination size
 * 
 * @param paged_blob 
 * @param mdo 
 * @param provision_drive_p
 * @param slot_number
 * @param lba_offset
 *
 * @return fbe_status_t.
 *
 * @author
 *  02/21/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t metadata_paged_get_next_marked_chunk(metadata_paged_blob_t * paged_blob, 
                                                         fbe_payload_metadata_operation_t * mdo)
{

    fbe_u8_t                    *data_ptr;
    fbe_u32_t                   slot_offset;
    fbe_u64_t                   index;
    fbe_u8_t                    *dst_ptr;
    fbe_u32_t                   dst_size;
    metadata_search_fn_t        search_fn;
    fbe_bool_t                  is_chunk_marked = FBE_FALSE;
    fbe_u64_t                   metadata_offset;
    fbe_u64_t                   max_slots;
    fbe_u64_t   slot_number;
    fbe_lba_t   lba_offset;

    /* Max slots to loop through */
    max_slots = paged_blob->slot_count; // was -2 

    lba_offset = mdo->u.metadata.offset / FBE_METADATA_BLOCK_DATA_SIZE;
    slot_number = lba_offset - paged_blob->lba;

    metadata_offset = mdo->u.metadata.offset;
    data_ptr = ((fbe_u8_t *)paged_blob->sg_list[slot_number].address);
    slot_offset = (fbe_u32_t)(mdo->u.metadata.offset - lba_offset * FBE_METADATA_BLOCK_DATA_SIZE);
    data_ptr += slot_offset;
     
    dst_size = mdo->u.next_marked_chunk.search_data_size;
    fbe_payload_metadata_get_search_function(mdo, &search_fn);

   /* Loop through (METADATA_PAGED_BLOB_SLOTS >> 1) slots */
   for(index = slot_number; index < max_slots; index ++) {
       /* Loop through 512 bytes of data */
       while(slot_offset < FBE_METADATA_BLOCK_DATA_SIZE) {
           /* If the data to copy falls within 512 bytes just copy it and and move on next chunk */
           if(slot_offset + dst_size <= FBE_METADATA_BLOCK_DATA_SIZE) {
                //fbe_copy_memory(dst_ptr, data_ptr, dst_size);
                dst_ptr = data_ptr;
                slot_offset += dst_size;
                data_ptr += dst_size;

                is_chunk_marked = search_fn(dst_ptr, dst_size, mdo->u.next_marked_chunk.context);

                /* If the chunk is marked, we have found the chunk to do operation upon,
                   populate the current offset and return */
                if(is_chunk_marked) {
                    mdo->u.next_marked_chunk.current_offset = metadata_offset;

                    fbe_copy_memory(mdo->u.metadata.record_data, dst_ptr, dst_size);
                    fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
                    return FBE_STATUS_OK;
                }

                /* Chunk is not marked. update the metadata offset and check whether
                   we need to move to the next slot or remain within the same slot
                 */
                metadata_offset += dst_size; 
                if( (slot_offset >= FBE_METADATA_BLOCK_DATA_SIZE) && (index != (max_slots - 1)) ) {
                    data_ptr = ((fbe_u8_t *)paged_blob->sg_list[index+1].address);
                    slot_offset = 0;
                    break;
                }

                /* If it is the last slot and the chunk is not marked just populate the current offset and return */
                if(((slot_offset + dst_size) > FBE_METADATA_BLOCK_DATA_SIZE) && (index == (max_slots - 1))) {
                    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Reached the last slot. metadata_offset:0x%llx\n", __FUNCTION__, metadata_offset); 
                    mdo->u.next_marked_chunk.current_offset = metadata_offset;
                    fbe_copy_memory(mdo->u.metadata.record_data, dst_ptr, dst_size);
                    fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
                    return FBE_STATUS_OK;
                }
                /* If we have reached the end of the slot and the search size is not a multiple of the 
                 * data size, then stop here since we do not handle a search size crossing block boundary. 
                 */
                if ( ((slot_offset + dst_size) >= FBE_METADATA_BLOCK_DATA_SIZE) &&
                     (FBE_METADATA_BLOCK_DATA_SIZE % dst_size) ) {
                    metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                   "Get Next offset crossing metadata_offset:0x%llx next_end: 0x%x dst_size: 0x%x\n", 
                                   metadata_offset, (slot_offset + dst_size), dst_size); 
                    mdo->u.next_marked_chunk.current_offset = metadata_offset;
                    fbe_copy_memory(mdo->u.metadata.record_data, dst_ptr, dst_size);
                    fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
                    return FBE_STATUS_OK;
                }
                /* Move on to the next chunk of data within the same slot */
                continue;
           } else  { /* The data to copy overlaps 2 slots */
                //if(index != max_slots) {					
                fbe_copy_memory(mdo->u.metadata.record_data, data_ptr, FBE_METADATA_BLOCK_DATA_SIZE - slot_offset);
                data_ptr = ((fbe_u8_t *)paged_blob->sg_list[index+1].address);

                fbe_copy_memory(mdo->u.metadata.record_data + FBE_METADATA_BLOCK_DATA_SIZE - slot_offset,
                                data_ptr, dst_size - (FBE_METADATA_BLOCK_DATA_SIZE - slot_offset));

                slot_offset = dst_size - (FBE_METADATA_BLOCK_DATA_SIZE - slot_offset);
                data_ptr += slot_offset;
                //dst_ptr = orig_dst_ptr;

                //call the client function here
                is_chunk_marked = search_fn(mdo->u.metadata.record_data, dst_size, mdo->u.next_marked_chunk.context);
                if(is_chunk_marked) {
                    mdo->u.next_marked_chunk.current_offset = metadata_offset;
                    fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
                    return FBE_STATUS_OK;
                }
                /* If the chunk is not marked for then break out of while loop since we need to move
                   on to the next slot of data
                */
                metadata_offset += dst_size; 
                if(index == max_slots) {
                    /* If it is the last slot and the chunk is not marked, just populate the current offset and return */
                    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s Reached the last slot. metadata_offset:0x%llx\n", __FUNCTION__, metadata_offset); 

                    mdo->u.next_marked_chunk.current_offset = metadata_offset;
                    fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
                    return FBE_STATUS_OK;
                }
                break; /* break the while loop */
            }/* The data to copy overlaps 2 slots */
       } /* while(slot_offset < FBE_METADATA_BLOCK_DATA_SIZE)  */
   } /* for(index = slot_number; index <=max_slots; index ++) */

   fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
   return FBE_STATUS_OK;

}

static fbe_status_t
metadata_paged_memory_update(fbe_payload_metadata_operation_opcode_t opcode, 
                             fbe_u8_t * data_ptr, 
                             fbe_u8_t * src_ptr, 
                             fbe_u64_t count, 
                             fbe_bool_t * is_write_requiered)
{
    fbe_u32_t j;

    for(j = 0; j < count; j++){
        /* Make the data changes */
        switch(opcode){
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS:
                if((data_ptr[j] & src_ptr[j]) != src_ptr[j]) { * is_write_requiered = FBE_TRUE; }
                data_ptr[j] |= src_ptr[j];
            break;
            case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_XOR_BITS:
                data_ptr[j] ^= src_ptr[j];
                * is_write_requiered = FBE_TRUE; /* We may want to eliminate XOR all together from PVD */
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_CLEAR_BITS:
                if((data_ptr[j] & src_ptr[j]) != 0) { * is_write_requiered = FBE_TRUE; }
                data_ptr[j] &= ~src_ptr[j];
            break;

        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE:
                if(data_ptr[j] != src_ptr[j]) { * is_write_requiered = FBE_TRUE; }
                data_ptr[j] = src_ptr[j];
            break;
        } /* switch(mdo->opcode) */
    } /* for(j = 0; j < count; j++) */
    return FBE_STATUS_OK;
}

static fbe_status_t
metadata_paged_mdo_to_blob(metadata_paged_blob_t * paged_blob, fbe_payload_metadata_operation_t * mdo, fbe_bool_t * is_write_requiered)
{
    fbe_lba_t lba_offset;
    fbe_u64_t slot;
    fbe_u64_t i;
    fbe_u8_t * data_ptr = NULL;
    fbe_u64_t slot_offset;
    fbe_u8_t * src_ptr;    
    fbe_u64_t j;
    fbe_u64_t current_offset;
    fbe_bool_t is_done = FBE_FALSE;

    * is_write_requiered = FBE_FALSE;

    if(mdo->u.metadata.repeat_count == 0){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s Invalid repeat_count\n", __FUNCTION__);
    }

    /* Make the data changes */
    current_offset = mdo->u.metadata.offset + mdo->u.metadata.current_count_private * mdo->u.metadata.record_data_size;
    lba_offset = current_offset / FBE_METADATA_BLOCK_DATA_SIZE;
    slot = lba_offset - paged_blob->lba;
    slot_offset = (fbe_u32_t)(current_offset - lba_offset * FBE_METADATA_BLOCK_DATA_SIZE);

    for(slot = slot; slot < paged_blob->slot_count && !is_done; slot++){
        data_ptr = ((fbe_u8_t *)paged_blob->sg_list[slot].address);
        data_ptr += slot_offset;

        for(i = slot_offset ; i < FBE_METADATA_BLOCK_DATA_SIZE && !is_done; i += mdo->u.metadata.record_data_size){			
            src_ptr = mdo->u.metadata.record_data;

            if(i + mdo->u.metadata.record_data_size <= FBE_METADATA_BLOCK_DATA_SIZE){			
                metadata_paged_memory_update(mdo->opcode, data_ptr, src_ptr, mdo->u.metadata.record_data_size, is_write_requiered);
                j = 0;
            } else { /* We have to split the copy */
                j = FBE_METADATA_BLOCK_DATA_SIZE - i;
                metadata_paged_memory_update(mdo->opcode, data_ptr, src_ptr, j, is_write_requiered);

                data_ptr = ((fbe_u8_t *)paged_blob->sg_list[slot + 1].address);
                metadata_paged_memory_update(mdo->opcode, data_ptr, src_ptr + j, mdo->u.metadata.record_data_size - j, is_write_requiered);
            }

            mdo->u.metadata.current_count_private++;
            data_ptr += mdo->u.metadata.record_data_size;

            if (mdo->u.metadata.current_count_private >= mdo->u.metadata.repeat_count) { is_done = FBE_TRUE; }
            if ((mdo->u.metadata.repeat_count == 1) &&
                (mdo->u.metadata.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_FLAGS_REPEAT_TO_END) &&
                ((*is_write_requiered) == FBE_TRUE) ){
                is_done = FBE_FALSE;
                mdo->u.metadata.current_count_private = 1;
            }
        } /* for(i = slot_offset ; i < FBE_METADATA_BLOCK_DATA_SIZE / mdo->u.metadata.record_data_size; i++) */

        slot_offset = j;		
        fbe_metadata_calculate_checksum(paged_blob->sg_list[slot].address,  1);
        if(j != 0){
            fbe_metadata_calculate_checksum(paged_blob->sg_list[slot + 1].address,  1);
        }
    } /* for(i = slot; slot < paged_blob->slot_count; slot++)  */

    //fbe_metadata_calculate_checksum(paged_blob->sg_list[0].address, paged_blob->slot_count);
    return FBE_STATUS_OK;
}

/* This function checks if mdo can "fit" into paged_blob */ 
static fbe_bool_t
metadata_paged_bundle_write_check(fbe_payload_metadata_operation_t * mdo, metadata_paged_blob_t * paged_blob)
{
    fbe_lba_t first_lba = 0;
    fbe_lba_t last_lba = 0;

    switch(mdo->opcode) {
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_CLEAR_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_XOR_BITS:
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE:
            if (mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY) 
            {
                return FBE_FALSE;			
            }
            break;
        default:
            return FBE_FALSE;			
    }

    metadata_paged_mdo_to_lba(mdo, &first_lba, &last_lba);
    
    if((first_lba >= paged_blob->lba) && (last_lba < paged_blob->lba + paged_blob->block_count)) { /* Full overlapped */
        return FBE_TRUE;
    }	

    return FBE_FALSE;
}

/* This function checks if mdo can "fit" into paged_blob */ 
static fbe_bool_t
metadata_paged_bundle_read_check(fbe_payload_metadata_operation_t * mdo, metadata_paged_blob_t * paged_blob)
{
    fbe_lba_t first_lba;
    fbe_lba_t last_lba;

    switch(mdo->opcode) {
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_READ:
            break;
        default:
            return FBE_FALSE;			
    }

    metadata_paged_mdo_to_lba(mdo, &first_lba, &last_lba);
    
    if((first_lba >= paged_blob->lba) && (last_lba < paged_blob->lba + paged_blob->block_count)) { /* Full overlapped */
        return FBE_TRUE;
    }	

    return FBE_FALSE;
}


static fbe_status_t
metadata_paged_mark_bundle_start(fbe_packet_t * packet, fbe_payload_metadata_operation_t * mdo, metadata_paged_blob_t * paged_blob)
{
    fbe_metadata_element_t * mde = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_ex_t * current_payload = NULL;

    fbe_payload_stripe_lock_operation_t * sl = NULL;
    fbe_payload_stripe_lock_operation_t * current_sl = NULL;
    fbe_queue_element_t * current_element = NULL;
    fbe_payload_metadata_operation_t * current_mdo = NULL;
    fbe_bool_t is_write_requiered;
    fbe_bool_t calculate_crc = FBE_FALSE;
    fbe_u32_t slot;

    sep_payload = fbe_transport_get_payload_ex(packet);
    sl = fbe_payload_ex_get_stripe_lock_operation(sep_payload);
    mde = mdo->metadata_element;

    /* Get the stripe lock spin lock */
    //fbe_spinlock_lock(&mde->stripe_lock_spinlock);
    fbe_metadata_stripe_lock_hash(mde, sl);
    current_element = fbe_queue_front(&sl->wait_queue);
    while(current_element != NULL){
        current_sl = fbe_metadata_stripe_lock_queue_element_to_operation(current_element);
        if(!(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) &&
            !(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) &&
            !(current_sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE)){

            current_payload = fbe_transport_get_payload_ex(current_sl->cmi_stripe_lock.packet);
            current_mdo =  metadata_paged_get_metadata_operation(current_payload);
            /* Evaluate operation range */
            if((current_mdo != NULL) && metadata_paged_bundle_write_check(current_mdo, paged_blob)){
                /* Mark stripe lock so it will not be aborted */
                //current_sl->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_NO_ABORT;

                if (current_mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE) {
                    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "bundle start update mdo %p packet %p\n", current_mdo, current_sl->cmi_stripe_lock.packet);
                    fbe_metadata_paged_call_callback_function(current_mdo, paged_blob, current_sl->cmi_stripe_lock.packet);
                    calculate_crc = FBE_TRUE;
                } else {
                    /* Update the paged blob with data from current mdo */
                    metadata_paged_mdo_to_blob(paged_blob, current_mdo, &is_write_requiered);
                }

                /* Mark current_mdo so it will be completed without action */
                current_mdo->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_START;

                /* Mark mdo as well and we will check this flag in blob_write_completion function */
                mdo->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_MASTER;
            }
        }

        current_element = fbe_queue_next(&sl->wait_queue, current_element); 
    }


    //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
    fbe_metadata_stripe_unlock_hash(mde, sl);

    /* We allocating exact number of slots we need for I/O (memory resource) */
    if (calculate_crc) {
        for (slot = 0; slot < paged_blob->slot_count; slot++) {
            fbe_metadata_calculate_checksum(paged_blob->sg_list[slot].address,  1);
        }
    }

    return FBE_STATUS_OK;
}
static fbe_status_t
metadata_paged_mark_bundle_done(fbe_packet_t * packet, fbe_payload_metadata_operation_t * mdo, metadata_paged_blob_t * paged_blob)
{
    fbe_metadata_element_t * mde = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_ex_t * current_payload = NULL;
    fbe_queue_head_t tmp_queue;

    fbe_payload_stripe_lock_operation_t * sl = NULL;
    fbe_payload_stripe_lock_operation_t * current_sl = NULL;
    fbe_queue_element_t * current_element = NULL;
    fbe_payload_metadata_operation_t * current_mdo = NULL;	

    fbe_queue_init(&tmp_queue);
    
    sep_payload = fbe_transport_get_payload_ex(packet);
    sl = fbe_payload_ex_get_stripe_lock_operation(sep_payload);
    mde = mdo->metadata_element;

    /* Get the stripe lock spin lock */
    //fbe_spinlock_lock(&mde->stripe_lock_spinlock);
    fbe_metadata_stripe_lock_hash(mde, sl);
    current_element = fbe_queue_front(&sl->wait_queue);
    while(current_element != NULL){
        current_sl = fbe_metadata_stripe_lock_queue_element_to_operation(current_element);
        current_element = fbe_queue_next(&sl->wait_queue, current_element); 

        if(!(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) &&
            !(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) &&
            !(current_sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE)){
            current_payload = fbe_transport_get_payload_ex(current_sl->cmi_stripe_lock.packet);
            current_mdo =  metadata_paged_get_metadata_operation(current_payload);

            /* Check if we can satisfy some reads */
            if((current_mdo != NULL) && metadata_paged_bundle_read_check(current_mdo, paged_blob)){
                current_mdo->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_DONE;

                /* Copy the data */
                if (current_mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_READ) {
                    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "bundle done read mdo %p packet %p\n", current_mdo, current_sl->cmi_stripe_lock.packet);
                    fbe_metadata_paged_call_object_cache_function(FBE_METADATA_PAGED_CACHE_ACTION_POST_READ, current_mdo->metadata_element, current_mdo, 0, 0, paged_blob);
                    fbe_metadata_paged_call_callback_function(current_mdo, paged_blob, current_sl->cmi_stripe_lock.packet);
                } else {
                    metadata_paged_blob_to_mdo(paged_blob, current_mdo);
                }

                fbe_payload_metadata_set_status(current_mdo, FBE_PAYLOAD_METADATA_STATUS_OK);

                /* Get current_sl off the wait queue */
                fbe_queue_remove(&current_sl->queue_element);
                /* Complete the packet */
                fbe_queue_push(&tmp_queue, &current_sl->cmi_stripe_lock.packet->queue_element);
                continue;
            }

            if((current_mdo != NULL) && (current_mdo->u.metadata.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_START)){
                current_mdo->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_DONE;
                //current_sl->flags &= ~FBE_PAYLOAD_STRIPE_LOCK_FLAG_NO_ABORT;
                
                if (current_mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE) {
                    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "bundle done update mdo %p packet %p\n", current_mdo, current_sl->cmi_stripe_lock.packet);
                    /* Update object cache */
                    fbe_metadata_paged_call_object_cache_function(FBE_METADATA_PAGED_CACHE_ACTION_POST_WRITE, current_mdo->metadata_element, current_mdo, 0, 0, paged_blob);
                } else {
                    metadata_paged_blob_to_mdo(paged_blob, current_mdo);
                }
                current_mdo->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_VALID_DATA;

                fbe_payload_metadata_set_status(current_mdo, FBE_PAYLOAD_METADATA_STATUS_OK);

                /* Get current_sl off the wait queue */
                fbe_queue_remove(&current_sl->queue_element);
                /* Complete the packet */
                fbe_queue_push(&tmp_queue, &current_sl->cmi_stripe_lock.packet->queue_element);
            }
        }
    }

    //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
    fbe_metadata_stripe_unlock_hash(mde, sl);

    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
    fbe_queue_destroy(&tmp_queue);

    return FBE_STATUS_OK;
}


/* This function will remove all bundle related flags */
static fbe_status_t
metadata_paged_clear_bundle(fbe_packet_t * packet, 
                            fbe_payload_metadata_operation_t * mdo, 
                            metadata_paged_blob_t * paged_blob, 
                            fbe_payload_metadata_status_t metadata_status)
{
    fbe_metadata_element_t * mde = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_ex_t * current_payload = NULL;

    fbe_payload_stripe_lock_operation_t * sl = NULL;
    fbe_payload_stripe_lock_operation_t * current_sl = NULL;
    fbe_queue_element_t * current_element = NULL;
    fbe_payload_metadata_operation_t * current_mdo = NULL;
    fbe_queue_head_t tmp_queue;


    fbe_queue_init(&tmp_queue);
    
    sep_payload = fbe_transport_get_payload_ex(packet);
    sl = fbe_payload_ex_get_stripe_lock_operation(sep_payload);
    mde = mdo->metadata_element;

    /* Get the stripe lock spin lock */
    //fbe_spinlock_lock(&mde->stripe_lock_spinlock);
    fbe_metadata_stripe_lock_hash(mde, sl);

    current_element = fbe_queue_front(&sl->wait_queue);
    while(current_element != NULL){

        current_sl = fbe_metadata_stripe_lock_queue_element_to_operation(current_element);
        current_element = fbe_queue_next(&sl->wait_queue, current_element); 
        if(	!(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) &&
            !(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) &&
            !(current_sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE)){

            current_payload = fbe_transport_get_payload_ex(current_sl->cmi_stripe_lock.packet);
            current_mdo =  metadata_paged_get_metadata_operation(current_payload);

            if((current_mdo != NULL) && (current_mdo->u.metadata.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_START)){
                //current_mdo->u.metadata.operation_flags &= ~FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_START;

                /* Because of PVD XOR we have to fail all bundled operations */
                fbe_payload_metadata_set_status(current_mdo, metadata_status);

                current_mdo->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_DONE;

                /* Get current_sl off the wait queue */
                fbe_queue_remove(&current_sl->queue_element);
                /* Complete the packet */
                fbe_queue_push(&tmp_queue, &current_sl->cmi_stripe_lock.packet->queue_element);
            }
        }

    }

    //fbe_spinlock_unlock(&mde->stripe_lock_spinlock);
    fbe_metadata_stripe_unlock_hash(mde, sl);

    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
    fbe_queue_destroy(&tmp_queue);

    return FBE_STATUS_OK;
}



static fbe_status_t
metadata_paged_write_blob(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_object_id_t object_id;
    fbe_status_t status;
    metadata_paged_blob_t * paged_blob = NULL;
    fbe_payload_block_operation_opcode_t block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    void * verify_errors_p = NULL;
    fbe_time_t pdo_service_time_ms = 0;

    paged_blob = (metadata_paged_blob_t *)context;

    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo =  metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

    /*let's make sure we propagate up the errors (if any), by giving the pointer to the sub packet,
    it will go al the way down to the RG handling the IO, so if it sees and errors, it will populate them 
    so the sender of the IO gets back the correct counts*/
    fbe_payload_ex_get_verify_error_count_ptr(sep_payload, &verify_errors_p);

    new_packet = paged_blob->packet_ptr;

    if (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT)
    {
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s paged blob %p IO already in progress \n", __FUNCTION__, paged_blob);
    }
    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    fbe_transport_propagate_expiration_time(new_packet, packet);
    fbe_transport_set_np_lock_from_master(packet, new_packet);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, paged_blob->sg_list, METADATA_PAGED_BLOB_SLOTS);

    /* Allocate block operation */
    block_operation = fbe_payload_ex_allocate_block_operation(sep_payload);
    switch(mdo->opcode) {
    case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY:
        block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY;
        break;
    case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE:
        block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE;
        break;
    case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE:
        block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
        break;
    default:
        block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
        fbe_transport_set_packet_attr(new_packet, FBE_PACKET_FLAG_CONSUMED);
        break;
    }

    /* let's check the flags of paged_blob, if marked write_verify_needed, issue a write_verify */
    if((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) && (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_WRITE_VERIFY_NEEDED)) {
        block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY;
    }

    /* Check 4K alignment here */
    if(paged_blob->lba & 0x07){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s Unaligned paged_blob->lba 0x%llx \n", __FUNCTION__, paged_blob->lba);
    }



    fbe_payload_block_build_operation(block_operation,
                                      block_opcode,
                                      metadata_element->paged_record_start_lba + paged_blob->lba,    /* LBA */
                                      paged_blob->block_count,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    fbe_payload_block_set_flag(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP);

    /* set the pdo_service_time_ms to 0 (default value) in the subpacket */
    fbe_transport_modify_physical_drive_service_time(new_packet, pdo_service_time_ms);

    status = fbe_transport_add_subpacket(packet, new_packet);

    if (status != FBE_STATUS_OK) { /* We did not added subpacket ( Most likely packet was cancelled ) */
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s failed add subpacket status %d", __FUNCTION__, status);

        fbe_transport_destroy_packet(new_packet);

        fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
        metadata_paged_release_blob(paged_blob);
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        
        fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_CANCELLED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }


    /* We already have a write lock , let's check if we another writes we can "bundle" together */
    /* The current operation of packet should be a stripe lock */

    /* We will "bundle" paged operations if repeat_count is 1 and stripe lock is acquired */
    if((mdo->u.metadata.repeat_count == 1) && (mdo->u.metadata.stripe_count != 0)){
        metadata_paged_mark_bundle_start(packet, mdo, paged_blob);
    }

    paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT;

   /* Set completion function */
    fbe_transport_set_completion_function(new_packet, metadata_paged_write_blob_completion, paged_blob);

    fbe_payload_ex_set_verify_error_count_ptr(sep_payload, verify_errors_p);
    fbe_transport_set_address(new_packet, FBE_PACKAGE_ID_SEP_0, FBE_SERVICE_ID_TOPOLOGY, FBE_CLASS_ID_INVALID, object_id);

    new_packet->base_edge = NULL;
    fbe_payload_ex_increment_block_operation_level(sep_payload);

    //fbe_transport_set_packet_attr(new_packet, FBE_PACKET_FLAG_CONSUMED);
    status = fbe_topology_send_io_packet(new_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t 
metadata_paged_write_blob_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t *  master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_ex_t * master_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_status_t status;
    fbe_payload_metadata_status_t metadata_status;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_payload_block_operation_opcode_t        block_opcode;
    metadata_paged_blob_t * paged_blob = NULL;  
    fbe_object_id_t object_id;

    paged_blob = (metadata_paged_blob_t *)context;

    paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_payload = fbe_transport_get_payload_ex(master_packet);
    mdo = metadata_paged_get_metadata_operation(master_payload);
    metadata_element = mdo->metadata_element;
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);
    
    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation,&block_operation_status);

    if(block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS){
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s object_id: 0x%X lba 0x%llX bc 0x%llX BOS %d\n", __FUNCTION__, object_id,
                                                            metadata_element->paged_record_start_lba + paged_blob->lba,
                                                            paged_blob->block_count,
                                                            block_operation_status);
    }



    fbe_payload_block_get_qualifier(block_operation,&block_operation_qualifier);
    fbe_payload_block_get_opcode(block_operation,&block_opcode);

    metadata_status = fbe_metadata_determine_status(status, block_operation_status, block_operation_qualifier);
    fbe_payload_metadata_set_status(mdo, metadata_status);

    fbe_payload_ex_release_block_operation(sep_payload, block_operation);

    /* remove the subpacket */
    fbe_transport_remove_subpacket(packet);

    /* destroy the packet */
    fbe_transport_destroy_packet(packet);

    if (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK) {
        /* We can not do the write !!! This is bad! We done with this operation */
        if(mdo->u.metadata.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_MASTER){
            metadata_paged_clear_bundle(master_packet, mdo, paged_blob, metadata_status);
        }

        fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
        metadata_paged_release_blob(paged_blob);
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s object_id: 0x%X lba 0x%llX bc 0x%llX MDS %d\n", __FUNCTION__, object_id,
                                                            metadata_element->paged_record_start_lba + paged_blob->lba,
                                                            paged_blob->block_count, metadata_status);

        fbe_payload_metadata_set_status(mdo, metadata_status);
        fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_OK;
    }

    /* Let's see if we need to continue (check the repeat count) */
    if((mdo->u.metadata.repeat_count > 1) && (mdo->u.metadata.repeat_count > mdo->u.metadata.current_count_private)){
        /* OK let's do it again */
        metadata_paged_switch(master_packet, (fbe_packet_completion_context_t *)mdo);
        return FBE_STATUS_OK;
    }

    /* If we succesfully completed small change bits operation we may update the record_data of mdo
        to avoid additional read.
    */
    if ((metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK) && (mdo->u.metadata.repeat_count == 1)){
        metadata_paged_blob_to_mdo(paged_blob, mdo);
        mdo->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_VALID_DATA;
    }   

    if(mdo->u.metadata.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_MASTER){
        metadata_paged_mark_bundle_done(master_packet, mdo, paged_blob);
    }


    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
    paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
    if ((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY) 
        && (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) {
        /* clear the write_verify flag*/
        paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_WRITE_VERIFY_NEEDED;
    }

    if(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_PRIVATE){
        metadata_paged_release_blob(paged_blob);
    } else {
        paged_blob->time_stamp = fbe_get_time(); 
    }

    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    if ((metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK) && (metadata_status != FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE)) {
        /* all the other errors */
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "mdata_paged_wt_blob_compl I/O fail,stat:0x%x bl_stat:0x%x bl_qualifier:0x%x\n",
                       status, block_operation_status, block_operation_qualifier);
    }


    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_paged_get_lock_range(fbe_payload_metadata_operation_t * mdo, fbe_lba_t * lba, fbe_block_count_t * block_count)
{
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_lba_t first_lba;
    fbe_lba_t last_lba;
    fbe_block_count_t bc;

    metadata_paged_mdo_to_lba(mdo, &first_lba, &last_lba);
    
    metadata_element = mdo->metadata_element;

    // Check that the last_lba is within the extent of the paged metadata
    if(last_lba > metadata_element->paged_mirror_metadata_offset){ 
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s last_lba 0x%llX > metadata capacity\n",
                       __FUNCTION__, last_lba);
    }

    *lba = metadata_element->paged_record_start_lba + first_lba/* - (operation_lba % (METADATA_PAGED_BLOB_SLOTS / 2))*/; 
    bc = (last_lba - first_lba + 1);

#if 0 /* I do not understand this code */
    // Add the blob slots if they are still within the paged metadata extent
    if(bc + METADATA_PAGED_BLOB_SLOTS <= metadata_element->paged_mirror_metadata_offset){
        bc += METADATA_PAGED_BLOB_SLOTS;
    }
#endif
    *block_count = bc;

    // I think we do not need it */
    //metadata_align_io(mdo->metadata_element, lba, block_count);
    return FBE_STATUS_OK;
}

/* This function will "skip" stripe lock operation if needed */
static fbe_payload_metadata_operation_t *
metadata_paged_get_metadata_operation(fbe_payload_ex_t * sep_payload)
{
    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        return fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        return fbe_payload_ex_get_any_metadata_operation(sep_payload);
    }

    return NULL;
}

static fbe_status_t
metadata_paged_dispatch_acquire_stripe_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t                       * sep_payload = NULL;
    fbe_payload_metadata_operation_t        * mdo = NULL;
    fbe_payload_stripe_lock_operation_t     * stripe_lock_operation = NULL;
    fbe_metadata_element_t                  * metadata_element = NULL;
    fbe_status_t                             status;
    fbe_payload_stripe_lock_status_t         stripe_lock_status;                    
    fbe_status_t                             packet_status;                      // overall status from the packet
    fbe_payload_metadata_status_t            metadata_status;

    sep_payload = fbe_transport_get_payload_ex(packet);

    mdo = metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;
    
    stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(sep_payload);

    if(mdo->u.metadata.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_DONE){
        /* It was successful bundle operation. Just release stripe lock operation and complete the packet */
        //fbe_atomic_increment(&metadata_paged_bundle_counter);

        fbe_payload_ex_release_stripe_lock_operation(sep_payload, stripe_lock_operation);

        //fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK); /* It may be failed bundle */

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        return FBE_STATUS_OK; /* Continue completion loop */
    }

    fbe_payload_stripe_lock_get_status(stripe_lock_operation, &stripe_lock_status);
    if(stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) {
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "mdata_paged_dispatch_packet_acq_sl_compl:cannot acquire the stripe lock,status %d\n",stripe_lock_status);  

        /* We need to clear BUNDLE_START flag */
        mdo->u.metadata.operation_flags &= ~FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_START;

        //  Get the status of the packet
        packet_status = fbe_transport_get_status_code(packet);
        fbe_payload_ex_release_stripe_lock_operation(sep_payload, stripe_lock_operation);        
        switch (packet_status)
        {
        case FBE_STATUS_OK: 
            switch (stripe_lock_status)
            {
            case FBE_PAYLOAD_STRIPE_LOCK_STATUS_CANCELLED:
                metadata_status = FBE_PAYLOAD_METADATA_STATUS_CANCELLED;
                break;
            case FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED:
                metadata_status = FBE_PAYLOAD_METADATA_STATUS_ABORTED;
                break;
            default:
                metadata_status = FBE_PAYLOAD_METADATA_STATUS_IO_RETRYABLE;
                break;
            }
            break;
        case FBE_STATUS_CANCELED:
            metadata_status = FBE_PAYLOAD_METADATA_STATUS_CANCELLED;
            break;
        default:
            metadata_status = FBE_PAYLOAD_METADATA_STATUS_FAILURE;
            break;
        }
        fbe_payload_metadata_set_status(mdo, metadata_status);

        return FBE_STATUS_CONTINUE;
    }    

    fbe_transport_set_completion_function(packet, metadata_paged_release_stripe_lock, NULL);

    status = metadata_paged_switch(packet, (fbe_packet_completion_context_t *)mdo);

    /* This is completion context */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t
metadata_paged_release_stripe_lock(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_stripe_lock_operation_t *   stripe_lock_operation = NULL;
    fbe_payload_stripe_lock_operation_opcode_t opcode;
    fbe_payload_stripe_lock_status_t  stripe_lock_status;
    fbe_payload_ex_t                       * sep_payload = NULL;
    

    sep_payload = fbe_transport_get_payload_ex(packet);
    stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(sep_payload);

    fbe_payload_stripe_lock_get_opcode(stripe_lock_operation, &opcode);
    fbe_payload_stripe_lock_get_status(stripe_lock_operation, &stripe_lock_status);

    if(stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK){
        /* We should continue with completion */
        fbe_payload_ex_release_stripe_lock_operation(sep_payload, stripe_lock_operation);
        return FBE_STATUS_OK;
    }

    switch(opcode){
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK:
            fbe_payload_stripe_lock_build_read_unlock(stripe_lock_operation);
            break;
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK:
            fbe_payload_stripe_lock_build_write_unlock(stripe_lock_operation);
            break;
    }

    fbe_transport_set_completion_function(packet, metadata_paged_release_stripe_lock_completion, NULL);
    fbe_stripe_lock_operation_entry(packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t
metadata_paged_release_stripe_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_stripe_lock_operation_t *   stripe_lock_operation = NULL;
    fbe_payload_ex_t                       * sep_payload = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(sep_payload);
    fbe_payload_ex_release_stripe_lock_operation(sep_payload, stripe_lock_operation);

    return FBE_STATUS_OK;
}

/* Warning! if STATUS_OK is returned the stripe lock grant function will NOT call this function again 
with the next exclusive stripe */
fbe_status_t 
fbe_metadata_paged_release_blobs(fbe_metadata_element_t * metadata_element, fbe_lba_t stripe_offset, fbe_block_count_t stripe_count)
{
    metadata_paged_blob_t * current_paged_blob = NULL;
    metadata_paged_blob_t * blob = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    /* Callback to object to invalidate local paged cache */
    if (fbe_metadata_element_is_paged_object_cache(metadata_element)) {
        fbe_metadata_paged_call_object_cache_function(FBE_METADATA_PAGED_CACHE_ACTION_INVALIDATE, metadata_element, NULL, stripe_offset, stripe_count, NULL);
        //fbe_metadata_paged_call_object_cache_function(FBE_METADATA_PAGED_CACHE_ACTION_INVALIDATE, metadata_element, NULL, FBE_LBA_INVALID, 0, NULL);
    }

    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

    if(fbe_queue_is_empty(&metadata_element->paged_blob_queue_head)){
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        return FBE_STATUS_OK;
    }

    current_paged_blob = (metadata_paged_blob_t *)fbe_queue_front(&metadata_element->paged_blob_queue_head);
    while(current_paged_blob != NULL){
#if 0
        if(current_paged_blob->stripe_count == 0){ /* Temporary skip blobs without stripe lock */
            current_paged_blob = (metadata_paged_blob_t *)fbe_queue_next(&metadata_element->paged_blob_queue_head, &current_paged_blob->queue_element);
            continue;
        }

        if(current_paged_blob->stripe_offset >= stripe_offset && 
            current_paged_blob->stripe_offset < stripe_offset + stripe_count){
                blob = current_paged_blob;
                current_paged_blob = (metadata_paged_blob_t *)fbe_queue_next(&metadata_element->paged_blob_queue_head, &current_paged_blob->queue_element);
                metadata_paged_release_blob(blob);
                continue;
        }
        
        if(stripe_offset >= current_paged_blob->stripe_offset && 
            stripe_offset < current_paged_blob->stripe_offset + current_paged_blob->stripe_count){
                blob = current_paged_blob;
                current_paged_blob = (metadata_paged_blob_t *)fbe_queue_next(&metadata_element->paged_blob_queue_head, &current_paged_blob->queue_element);
                metadata_paged_release_blob(blob);
                continue;
        }

        /* It is very unlikely, but the another exclusive slot with lower stripe number may cover the metadata region as well */
        if(current_paged_blob->stripe_offset < stripe_offset){			
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
#endif
        blob = current_paged_blob;
        current_paged_blob = (metadata_paged_blob_t *)fbe_queue_next(&metadata_element->paged_blob_queue_head, &current_paged_blob->queue_element);
        if(!(blob->flags & METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS)){
            metadata_paged_release_blob(blob);
        }
    }

    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    return status;
}

fbe_status_t 
fbe_metadata_paged_clear_cache(fbe_metadata_element_t * metadata_element)
{
    metadata_paged_blob_t * current_paged_blob = NULL;
    metadata_paged_blob_t * blob = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    /* Callback to object to invalidate local paged cache */
    if (fbe_metadata_element_is_paged_object_cache(metadata_element)) {
        fbe_metadata_paged_call_object_cache_function(FBE_METADATA_PAGED_CACHE_ACTION_INVALIDATE, metadata_element, NULL, FBE_LBA_INVALID, 0, NULL);
    }
    
    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

    if (fbe_queue_is_empty(&metadata_element->paged_blob_queue_head))
    {
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        return status;
    }

    current_paged_blob = (metadata_paged_blob_t *)fbe_queue_front(&metadata_element->paged_blob_queue_head);
    while (current_paged_blob != NULL)
    {
        blob = current_paged_blob;
        current_paged_blob = (metadata_paged_blob_t *)fbe_queue_next(&metadata_element->paged_blob_queue_head, &current_paged_blob->queue_element);
        if ((blob->flags & METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT) != 0)
        {
            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
            metadata_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "mdata_paged_clear_cache: found in-flight blob\n");
            return FBE_STATUS_BUSY;
        }
        metadata_paged_release_blob(blob);
    }

    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    return status;
}

/*!**************************************************************
 * fbe_metadata_element_paged_complete_cancelled()
 ****************************************************************
 * @brief
 *  This function will search for any queued packets that are
 *  cancelled, and will remove these packets and complete them.
 *
 * @param metadata_element - metadata element to complete packets for.        
 *
 * @return fbe_status_t   
 *
 * @author
 *  4/6/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_metadata_element_paged_complete_cancelled(fbe_metadata_element_t *metadata_element)
{
    /*! @todo, add code to complete cancelled packets here.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_metadata_element_paged_complete_cancelled()
 ******************************************/

static fbe_status_t
metadata_paged_read_blob(fbe_packet_t * packet, fbe_memory_completion_context_t context)
{
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_status_t status;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_object_id_t object_id;
    metadata_paged_blob_t * paged_blob = NULL;
    void *verify_errors_p = NULL;
    fbe_time_t pdo_service_time_ms = 0;

    paged_blob = (metadata_paged_blob_t *)context;

    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo =  metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

    /*let's make sure we propagate up the errors (if any), by giving the pointer to the sub packet,
    it will go al the way down to the RG handling the IO, so if it sees and errors, it will populate them 
    so the sender of the IO gets back the correct counts*/
    fbe_payload_ex_get_verify_error_count_ptr(sep_payload, &verify_errors_p);

    new_packet = paged_blob->packet_ptr;
    if (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT)
    {
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s paged blob %p IO already in progress \n", __FUNCTION__, paged_blob);
    }
    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    fbe_transport_propagate_expiration_time(new_packet, packet);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, paged_blob->sg_list, METADATA_PAGED_BLOB_SLOTS);

    if( paged_blob->sg_list[0].count == 0){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s Bad SGL\n", __FUNCTION__);

    }

    /* Check 4K alignment here */
    if(paged_blob->lba & 0x07){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s Unaligned paged_blob->lba 0x%llx \n", __FUNCTION__, paged_blob->lba);
    }


    /* Allocate block operation */
    block_operation = fbe_payload_ex_allocate_block_operation(sep_payload);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      metadata_element->paged_record_start_lba + paged_blob->lba,    /* LBA */
                                      paged_blob->slot_count,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */
    fbe_payload_block_set_flag(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP);	

    /* set the pdo_service_time_ms to 0 (default value) in the subpacket */
    fbe_transport_modify_physical_drive_service_time(new_packet, pdo_service_time_ms);

    status = fbe_transport_add_subpacket(packet, new_packet);

    if (status != FBE_STATUS_OK) { 
        fbe_queue_head_t tmp_queue;
        fbe_queue_init(&tmp_queue);

        metadata_trace(FBE_TRACE_LEVEL_WARNING, object_id,
                       "%s failed add subpacket 0x%p to master 0x%p status: 0x%x\n", 
                       __FUNCTION__, new_packet, packet, status);

        fbe_payload_ex_release_block_operation(sep_payload, block_operation);
        /* Release the memory */
        fbe_transport_destroy_packet(new_packet); 

        /* We have to release blob here */
        fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

#if 0
        if(!(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_PRIVATE)){
            fbe_payload_ex_t * p = NULL;
            fbe_payload_metadata_operation_t * m = NULL;

            /* Check packet queue */
            while(new_packet = fbe_transport_dequeue_packet(&paged_blob->packet_queue)){
                p = fbe_transport_get_payload_ex(new_packet);
                m = metadata_paged_get_metadata_operation(sep_payload);
                fbe_transport_set_completion_function(new_packet, (fbe_packet_completion_function_t)metadata_paged_switch, m);
                fbe_queue_push(&tmp_queue, &new_packet->queue_element);
            }
        }
#endif

        metadata_paged_release_blob(paged_blob);
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

        fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
        fbe_queue_destroy(&tmp_queue);

        fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_CANCELLED);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /*add the error information to the packet so RG can fill it if needed*/
    fbe_payload_ex_set_verify_error_count_ptr(sep_payload, verify_errors_p);

    paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT;

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet, metadata_paged_read_blob_completion, paged_blob);

    fbe_transport_set_address(  new_packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    new_packet->base_edge = NULL;
    fbe_payload_ex_increment_block_operation_level(sep_payload);

    metadata_paged_send_io_packet(packet, new_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

fbe_status_t
fbe_metadata_paged_init_client_blob(fbe_memory_request_t * memory_request_p, fbe_payload_metadata_operation_t * mdo, fbe_u32_t skip_bytes, fbe_bool_t use_client_packet)
{
    fbe_memory_header_t *       master_memory_header = NULL;
    fbe_memory_header_t *       next_memory_header_p = NULL;
    fbe_u8_t            *       buffer = NULL;
    metadata_paged_blob_t *     paged_blob = NULL;
    metadata_paged_slot_t *     paged_slot = NULL;
    fbe_memory_chunk_size_t     chunk_size;
    fbe_u32_t                   number_of_chunks;
    fbe_u32_t                   bytes_remaining;
    fbe_lba_t                   stripe_offset;
    fbe_block_count_t           stripe_count;
    fbe_metadata_element_t *    metadata_element = NULL;
    fbe_lba_t                   operation_lba;
    fbe_u32_t                   i = 0;
    fbe_u32_t                   paged_blob_size;
    fbe_lba_t                   end_lba;
    fbe_block_count_t           block_count;

    metadata_element = mdo->metadata_element;
    metadata_paged_mdo_to_lba(mdo, &operation_lba, &end_lba);
    block_count = end_lba - operation_lba + 1;
#if 0
    operation_lba = mdo->u.metadata.offset / FBE_METADATA_BLOCK_DATA_SIZE;

    /* Align lba_offset to 4K */
    operation_lba &= ~0x0000000000000007;

    /* Get block count */
    end_lba = (mdo->u.metadata_callback.offset + mdo->u.metadata_callback.repeat_count * mdo->u.metadata_callback.record_data_size - 1) / FBE_METADATA_BLOCK_DATA_SIZE;
    block_count = end_lba - operation_lba + 1;
    block_count = FBE_MAX(METADATA_PAGED_BLOB_SLOTS, block_count); /* Read at least 2 slots */
    if(block_count & 0x0000000000000007){ /* Not aligned  to 4K */
        block_count = (block_count + 8) & ~0x0000000000000007;
    }
#endif
    block_count = FBE_MIN(METADATA_PAGED_MAX_SGL_ALIGNED, block_count); /* How much we can read ? */

    master_memory_header = fbe_memory_get_first_memory_header(memory_request_p);
    chunk_size = master_memory_header->memory_chunk_size;
    number_of_chunks = master_memory_header->number_of_chunks;
    buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);

    if (use_client_packet) {
        metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "Use client packet instead\n");
        paged_blob_size = (fbe_u32_t) &((metadata_paged_blob_t *)0)->packet;
    } else {
        paged_blob_size = sizeof(metadata_paged_blob_t);
    }

    /* Check chunk size */
    if ((chunk_size < skip_bytes) || (chunk_size < paged_blob_size)) {
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "Paged set client blob Error\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer += skip_bytes;
    bytes_remaining = chunk_size - skip_bytes;
    if (bytes_remaining < paged_blob_size) {
        number_of_chunks--;
        if (number_of_chunks == 0) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_memory_get_next_memory_header(master_memory_header, &next_memory_header_p);
        master_memory_header = next_memory_header_p;
        buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);
        bytes_remaining = chunk_size;
    }

    /* Initialize the paged blob */
    paged_blob = (metadata_paged_blob_t *)buffer;
    paged_blob->packet_ptr = NULL;
    if (!use_client_packet) {
        paged_blob->packet_ptr = &paged_blob->packet;
    }
    buffer += paged_blob_size;
    bytes_remaining -= paged_blob_size;

    /* Initialize the paged slots */
    paged_slot = (metadata_paged_slot_t *)buffer;
    while (i < block_count)
    {
        if (bytes_remaining >= sizeof(metadata_paged_slot_t))
        {
            paged_blob->sg_list[i].address = paged_slot->data;
            paged_blob->sg_list[i].count = FBE_METADATA_BLOCK_SIZE;
            i++;
            buffer += sizeof(metadata_paged_slot_t);
            bytes_remaining -= sizeof(metadata_paged_slot_t);
            paged_slot++;
        }
        else if (number_of_chunks > 1)
        {
            number_of_chunks--;
            fbe_memory_get_next_memory_header(master_memory_header, &next_memory_header_p);
            master_memory_header = next_memory_header_p;
            buffer = (fbe_u8_t *)fbe_memory_header_to_data(next_memory_header_p);
            bytes_remaining = chunk_size;
            paged_slot = (metadata_paged_slot_t *)buffer;
        }
        else
        {
            break;
        }
    }

    if (i != block_count) {
        i &= ~0x0000000000000007;
        metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "Paged client blob alloc %d slots\n", i);
    }

    if (i < METADATA_PAGED_BLOB_SLOTS) {
#if 0
        fbe_u8_t *bitbucket_addr = fbe_memory_get_bit_bucket_addr();
        while (i < METADATA_PAGED_BLOB_SLOTS) {
            paged_blob->sg_list[i].address = bitbucket_addr;
            paged_blob->sg_list[i].count = FBE_METADATA_BLOCK_SIZE;
            i++;
            bitbucket_addr += FBE_METADATA_BLOCK_SIZE;
        }
#endif
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "Paged client blob no enough slots\n");
    }

    paged_blob->slot_count = i;
    paged_blob->sg_list[i].address = NULL;
    paged_blob->sg_list[i].count = 0;
        
    paged_blob->metadata_element = metadata_element;
    paged_blob->lba = operation_lba /* - (operation_lba % (paged_blob->slot_count / 2))*/; 
    paged_blob->block_count = i; /* number of slots currently allocated */

    fbe_payload_metadata_get_metadata_stripe_offset(mdo, &stripe_offset);
    fbe_payload_metadata_get_metadata_stripe_count(mdo, &stripe_count);

    paged_blob->stripe_offset = stripe_offset; 
    paged_blob->stripe_count = stripe_count;

    paged_blob->flags = METADATA_PAGED_BLOB_FLAG_CLIENT_BLOB;
    if (use_client_packet) {
        paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET;
    }

    //fbe_queue_init(&paged_blob->packet_queue);

    mdo->u.metadata.client_blob = paged_blob;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_paged_init_client_blob_with_sg_list(fbe_sg_element_t *	sg_element_p, fbe_payload_metadata_operation_t * mdo)
{
    fbe_u8_t            *       buffer = NULL;
    metadata_paged_blob_t *     paged_blob = NULL;
    metadata_paged_slot_t *     paged_slot = NULL;
    fbe_u32_t                   bytes_remaining;
    fbe_lba_t                   stripe_offset;
    fbe_block_count_t           stripe_count;
    fbe_metadata_element_t *    metadata_element = NULL;
    fbe_lba_t                   operation_lba;
    fbe_u32_t                   i = 0;
    fbe_sg_element_t *	        sg_p = sg_element_p;

    metadata_element = mdo->metadata_element;
    operation_lba = mdo->u.metadata.offset / FBE_METADATA_BLOCK_DATA_SIZE;

    /* Align lba_offset to 4K */
    operation_lba &= ~0x0000000000000007;

    /* Initialize the paged blob */
    buffer = sg_p->address;
    paged_blob = (metadata_paged_blob_t *)buffer;
    paged_blob->packet_ptr = &paged_blob->packet;
    buffer += sizeof(metadata_paged_blob_t);
    bytes_remaining = sg_p->count - sizeof(metadata_paged_blob_t);

    /* Initialize the paged slots */
    paged_slot = (metadata_paged_slot_t *)buffer;
    while (i < METADATA_PAGED_MAX_SGL)
    {
        if (bytes_remaining >= sizeof(metadata_paged_slot_t))
        {
            paged_blob->sg_list[i].address = paged_slot->data;
            paged_blob->sg_list[i].count = FBE_METADATA_BLOCK_SIZE;
            i++;
            buffer += sizeof(metadata_paged_slot_t);
            bytes_remaining -= sizeof(metadata_paged_slot_t);
            paged_slot++;
        }
        else if ((++sg_p)->address != NULL)
        {
            buffer = sg_p->address;
            bytes_remaining = sg_p->count;
            paged_slot = (metadata_paged_slot_t *)buffer;
        }
        else
        {
            break;
        }
    }

    paged_blob->slot_count = i;
    paged_blob->sg_list[i].address = NULL;
    paged_blob->sg_list[i].count = 0;
        
    paged_blob->metadata_element = metadata_element;
    paged_blob->lba = operation_lba /* - (operation_lba % (paged_blob->slot_count / 2))*/; 
    paged_blob->block_count = i; /* number of slots currently allocated */

    fbe_payload_metadata_get_metadata_stripe_offset(mdo, &stripe_offset);
    fbe_payload_metadata_get_metadata_stripe_count(mdo, &stripe_count);

    paged_blob->stripe_offset = stripe_offset; 
    paged_blob->stripe_count = stripe_count;

    paged_blob->flags = METADATA_PAGED_BLOB_FLAG_CLIENT_BLOB;

    //fbe_queue_init(&paged_blob->packet_queue);

    mdo->u.metadata.client_blob = paged_blob;
    return FBE_STATUS_OK;
}


static __forceinline fbe_block_count_t fbe_metadata_paged_get_block_count(fbe_payload_metadata_operation_t * mdo)
{
    fbe_lba_t start_lba;
    fbe_lba_t end_lba;
    fbe_u64_t offset;
    fbe_block_count_t block_count;

    start_lba = fbe_metadata_paged_get_lba_offset(mdo);
    start_lba &= ~0x0000000000000007;
    offset = mdo->u.metadata_callback.offset + mdo->u.metadata_callback.repeat_count * mdo->u.metadata_callback.record_data_size - 1;
    end_lba = offset / FBE_METADATA_BLOCK_DATA_SIZE;

    block_count = end_lba - start_lba + 1;
    block_count = (block_count + 7) & ~0x0000000000000007;
    block_count = FBE_MIN(METADATA_PAGED_MAX_SGL, block_count); /* How much we can read ? */
    block_count = FBE_MAX(METADATA_PAGED_BLOB_SLOTS, block_count); /* Read at least 2 slots */

    return (block_count);
}

static __forceinline 
fbe_status_t fbe_metadata_paged_call_object_cache_function(fbe_metadata_paged_cache_action_t action,
                                                           fbe_metadata_element_t * mde,
                                                           fbe_payload_metadata_operation_t * mdo,
                                                           fbe_lba_t start_lba, fbe_block_count_t block_count,
                                                           metadata_paged_blob_t * paged_blob)
{
    if (mde->cache_callback) {
        if (action == FBE_METADATA_PAGED_CACHE_ACTION_POST_READ || action == FBE_METADATA_PAGED_CACHE_ACTION_POST_WRITE) {
            mdo->u.metadata_callback.start_lba = paged_blob->lba;
            mdo->u.metadata_callback.sg_list = &paged_blob->sg_list[0];
        }
        return (mde->cache_callback)(action, mde, mdo, start_lba, block_count);
    } else {
        return FBE_STATUS_CONTINUE;
    }
}

static __forceinline fbe_status_t fbe_metadata_paged_call_callback_function(fbe_payload_metadata_operation_t * mdo,
                                                                            metadata_paged_blob_t * paged_blob,
                                                                            fbe_packet_t *packet)
{
    mdo->u.metadata_callback.start_lba = paged_blob->lba;
    mdo->u.metadata_callback.sg_list = &paged_blob->sg_list[0];
    return (mdo->u.metadata_callback.callback_function)(packet, mdo->u.metadata_callback.callback_context);
}

static fbe_status_t 
fbe_metadata_paged_read(fbe_packet_t * packet)
{
    fbe_status_t           status;    
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    metadata_paged_blob_t * paged_blob = NULL;
    fbe_bool_t in_progress = FBE_FALSE;
    fbe_block_count_t block_count;

    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo =  metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;

    /* Check if we have object caches */
    status = fbe_metadata_paged_call_object_cache_function(FBE_METADATA_PAGED_CACHE_ACTION_PRE_READ, mdo->metadata_element, mdo, 0, 0, NULL);
    if (status == FBE_STATUS_OK) {
        fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_PAGED_CACHE_HIT);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    block_count = fbe_metadata_paged_get_block_count(mdo);

    /* Check if we have this area in the cache */
    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

    /* If FUA is specified we must read from disk */
    if ((mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_FLAGS_FUA_READ) != 0) {
        /* We must read from disk */
        paged_blob = metadata_paged_allocate_blob(mdo, (fbe_u32_t)block_count);
        paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
        mdo->paged_blob = paged_blob;
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        status = fbe_metadata_paged_read_blob(packet, paged_blob);
        return status;
    }

    /* Check if we have this area in the cache */
    paged_blob = metadata_paged_get_blob(mdo, &in_progress);

    if (paged_blob != NULL){ /* We have outstanding blob for this area */
        if (in_progress == FBE_TRUE){ /* We have outstanding read for this area */
            /* we will perform non cached read */
            paged_blob = NULL;
        } else { /* We have valid data in the blob */
            status = fbe_metadata_paged_call_callback_function(mdo, paged_blob, packet);
            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }        
    }

    if(paged_blob == NULL){         
        if (mdo->u.metadata_callback.client_blob != NULL) {
            paged_blob = (metadata_paged_blob_t *)(mdo->u.metadata_callback.client_blob);
            paged_blob->slot_count = FBE_MIN((fbe_u32_t)block_count, paged_blob->slot_count);
            paged_blob->block_count = paged_blob->slot_count;
            paged_blob->sg_list[paged_blob->block_count].address = NULL;
            paged_blob->sg_list[paged_blob->block_count].count = 0;
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
            mdo->paged_blob = paged_blob;
        } else if (in_progress == FBE_TRUE){/* Some other blob in progress - we will perform non cached read */
            paged_blob = metadata_paged_allocate_blob(mdo, (fbe_u32_t)block_count);
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
            mdo->paged_blob = paged_blob;
        } else { /* We will perform cached read */
            paged_blob = metadata_paged_allocate_blob(mdo, (fbe_u32_t)block_count);
            fbe_queue_push(&metadata_element->paged_blob_queue_head, &paged_blob->queue_element);
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
        }

        mdo->paged_blob = paged_blob;

        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

        status = fbe_metadata_paged_read_blob(packet, paged_blob);
        return status;
    }
    
    return FBE_STATUS_PENDING;
}

static fbe_status_t 
fbe_metadata_paged_update(fbe_packet_t * packet)
{
    fbe_status_t           status;    
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    metadata_paged_blob_t * paged_blob = NULL;
    fbe_bool_t in_progress = FBE_FALSE;
    fbe_block_count_t block_count;
    fbe_lba_t lba_offset;
    fbe_u64_t slot;
    fbe_bool_t is_read_requiered = FBE_TRUE;

    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo =  metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;

    /* How many blocks remains to be update ? */
    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    lba_offset &= ~0x0000000000000007;
    block_count = fbe_metadata_paged_get_block_count(mdo);

    /* Check if we have this area in the cache */
    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
    paged_blob = metadata_paged_get_blob(mdo, &in_progress);

    if (paged_blob == NULL){         
        if (mdo->u.metadata_callback.client_blob != NULL) {
            paged_blob = (metadata_paged_blob_t *)(mdo->u.metadata_callback.client_blob);
            //paged_blob->slot_count = (fbe_u32_t)block_count;
            //paged_blob->block_count = (fbe_u32_t)block_count;
            paged_blob->slot_count = FBE_MIN((fbe_u32_t)block_count, paged_blob->slot_count);
            paged_blob->block_count = paged_blob->slot_count;
            /* We could re-use the client blob. */
            paged_blob->sg_list[paged_blob->block_count].address = NULL;
            paged_blob->sg_list[paged_blob->block_count].count = 0;
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
            mdo->paged_blob = paged_blob;
        } else if (in_progress == FBE_TRUE){/* Some other blob in progress - we will perform non cached read */
            paged_blob = metadata_paged_allocate_blob(mdo, (fbe_u32_t)block_count);
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_PRIVATE;
            mdo->paged_blob = paged_blob;
        } else { /* We will perform cached read */
            paged_blob = metadata_paged_allocate_blob(mdo, (fbe_u32_t)block_count);
            fbe_queue_push(&metadata_element->paged_blob_queue_head, &paged_blob->queue_element);
            paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
        }

        /* Make sure it is 4K aligned */
        paged_blob->lba = lba_offset & ~0x0000000000000007;
        //paged_blob->block_count = block_count;

        /* Check whether it is update-to-cache only */
        if (mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_USE_BGZ_CACHE) 
        {
            status = fbe_metadata_paged_call_object_cache_function(FBE_METADATA_PAGED_CACHE_ACTION_PRE_UPDATE, mdo->metadata_element, mdo, 0, 0, NULL);
            if (status == FBE_STATUS_OK) {
                fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
                metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "Update to cache complete\n");
                fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
                fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
        }

        if (mdo->u.metadata_callback.operation_flags & 
            (FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE | FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY)) 
        {
            if (((lba_offset * FBE_METADATA_BLOCK_DATA_SIZE) >= mdo->u.metadata.offset) && 
                (((lba_offset + paged_blob->block_count) * FBE_METADATA_BLOCK_DATA_SIZE) <= 
                  (mdo->u.metadata_callback.offset + mdo->u.metadata_callback.repeat_count * mdo->u.metadata_callback.record_data_size))) {
                /* Full overlap, we do not need to read blob first */
                is_read_requiered = FBE_FALSE;
            }
        }

        if (is_read_requiered) {
            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
            status = fbe_metadata_paged_read_blob(packet, paged_blob);
            return status;
        }
    }
    /* We have outstanding blob for this area */
    else if (in_progress == FBE_TRUE){ /* We have outstanding I/O for this area */
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "Paged Stripe Lock Error\n");
        fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } 	
    
    /* We have relevant blob to make a change */
    status = fbe_metadata_paged_call_callback_function(mdo, paged_blob, packet);

    if ((status == FBE_STATUS_MORE_PROCESSING_REQUIRED) && !(mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NO_PERSIST)){
        /* Modify bits and do the write */
        paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
    } else {
#if 0
        if(mdo->u.metadata.repeat_count == 1){
            metadata_paged_blob_to_mdo(paged_blob, mdo);
            mdo->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_VALID_DATA;
        }   
#endif

        paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)) {
            fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        } else {
            fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
        }
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

#if 0
    for (slot = 0; slot < block_count; slot++) {
        fbe_metadata_calculate_checksum(paged_blob->sg_list[lba_offset - paged_blob->lba + slot].address,  1);
    }
#endif

    /* We allocating exact number of slots we need for I/O (memory resource) */
    for (slot = 0; slot < paged_blob->slot_count; slot++) {
        fbe_metadata_calculate_checksum(paged_blob->sg_list[slot].address,  1);
    }


    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    fbe_metadata_paged_write_blob(packet, paged_blob);

    return FBE_STATUS_PENDING;
}

static fbe_status_t
fbe_metadata_paged_read_blob(fbe_packet_t * packet, fbe_memory_completion_context_t context)
{
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_status_t status;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_object_id_t object_id;
    metadata_paged_blob_t * paged_blob = NULL;
    void *verify_errors_p = NULL;
    fbe_time_t pdo_service_time_ms = 0;

    paged_blob = (metadata_paged_blob_t *)context;
    if (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT)
    {
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s paged blob %p IO already in progress \n", __FUNCTION__, paged_blob);
    }

    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo =  metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

    /*let's make sure we propagate up the errors (if any), by giving the pointer to the sub packet,
    it will go al the way down to the RG handling the IO, so if it sees and errors, it will populate them 
    so the sender of the IO gets back the correct counts*/
    fbe_payload_ex_get_verify_error_count_ptr(sep_payload, &verify_errors_p);

    if (!(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET)) {
        new_packet = paged_blob->packet_ptr;
        fbe_transport_initialize_sep_packet(new_packet);    
        sep_payload = fbe_transport_get_payload_ex(new_packet);
        fbe_transport_propagate_expiration_time(new_packet, packet);
    } else {
        new_packet = packet;
    }

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, paged_blob->sg_list, METADATA_PAGED_BLOB_SLOTS);
    if (paged_blob->sg_list[0].count == 0){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Bad SGL\n", __FUNCTION__);
    }


    /* Check 4K alignment here */
    if(paged_blob->lba & 0x07){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s Unaligned paged_blob->lba 0x%llx \n", __FUNCTION__, paged_blob->lba);
    }



    /* Allocate block operation */
    block_operation = fbe_payload_ex_allocate_block_operation(sep_payload);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      metadata_element->paged_record_start_lba + paged_blob->lba,    /* LBA */
                                      paged_blob->block_count,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    fbe_payload_block_set_flag(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP);	

    if (!(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET)) {

        /* set the pdo_service_time_ms to 0 (default value) in the subpacket */
        fbe_transport_modify_physical_drive_service_time(new_packet, pdo_service_time_ms);

        status = fbe_transport_add_subpacket(packet, new_packet);

        if (status != FBE_STATUS_OK) { 

            metadata_trace(FBE_TRACE_LEVEL_WARNING, object_id,
                           "%s failed add subpacket 0x%p to master 0x%p status: 0x%x\n", 
                           __FUNCTION__, new_packet, packet, status);

            fbe_payload_ex_release_block_operation(sep_payload, block_operation);
            /* Release the memory */
            fbe_transport_destroy_packet(new_packet); 

            /* We have to release blob here */
            fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

            metadata_paged_release_blob(paged_blob);
            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

            fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_CANCELLED);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        /*add the error information to the packet so RG can fill it if needed*/
        fbe_payload_ex_set_verify_error_count_ptr(sep_payload, verify_errors_p);
    }

    paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT;

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_paged_read_blob_completion, paged_blob);

    fbe_transport_set_address(  new_packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    new_packet->base_edge = NULL;
    fbe_payload_ex_increment_block_operation_level(sep_payload);

    metadata_paged_send_io_packet(packet, new_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static __forceinline fbe_status_t 
fbe_metadata_paged_complete_packet(fbe_packet_t * packet, fbe_bool_t use_client_packet)
{
    if (use_client_packet) {
        return FBE_STATUS_OK;
    } else {
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
}

static fbe_status_t 
fbe_metadata_paged_read_blob_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t *  master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_ex_t * master_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_status_t status;
    fbe_payload_metadata_status_t metadata_status;
    fbe_payload_metadata_status_t previous_metadata_status;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    metadata_paged_blob_t * paged_blob = NULL;
    fbe_object_id_t object_id;
    fbe_time_t pdo_service_time_ms = 0;
    fbe_u64_t slot;
    
    paged_blob = (metadata_paged_blob_t *)context;

    paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT;

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);
    
    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation,&block_operation_status);
    fbe_payload_block_get_qualifier(block_operation,&block_operation_qualifier);
    fbe_payload_ex_release_block_operation(sep_payload, block_operation);

    if (!(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET)) {
        master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
        master_payload = fbe_transport_get_payload_ex(master_packet);
        mdo = metadata_paged_get_metadata_operation(master_payload);
        fbe_payload_metadata_get_status(mdo, &previous_metadata_status);
        metadata_element = mdo->metadata_element;
        object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

        /* set the pdo_service_time_ms to 0 (default value) in the subpacket */
        fbe_transport_modify_physical_drive_service_time(master_packet, pdo_service_time_ms);

        /* remove the subpacket */
        fbe_transport_remove_subpacket(packet);

        /* Release the memory */
        fbe_transport_destroy_packet(packet); /* This just free up packet resources, but not packet itself */
    } else {
        mdo = metadata_paged_get_metadata_operation(sep_payload);
        fbe_payload_metadata_get_status(mdo, &previous_metadata_status);
        metadata_element = mdo->metadata_element;
        object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
        /* We do not have a master packet, so set master to be itself */
        master_packet = packet;
    }

    if (block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS){
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s object_id: 0x%X lba 0x%llX bc 0x%llX BOS %d\n", __FUNCTION__, object_id,
                       metadata_element->paged_record_start_lba + paged_blob->lba,
                       paged_blob->block_count,
                       block_operation_status);
    }

    metadata_status = fbe_metadata_determine_status(status, block_operation_status, block_operation_qualifier);
    if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE){
        mdo->u.metadata_callback.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NEED_VERIFY;
    }

    if (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK){
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s object_id: 0x%X lba 0x%llX bc 0x%llX MDS %d\n", __FUNCTION__, object_id,
                       metadata_element->paged_record_start_lba + paged_blob->lba,
                       paged_blob->block_count,
                       metadata_status);
    }

    if ((metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK) && (metadata_status != FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE)) {
        metadata_trace(FBE_TRACE_LEVEL_WARNING, object_id,
                       "mdata_paged_rd_blob_compl I/O fail, op: %d stat: %d bl_stat: %d bl_qualifier: %d\n",
                       mdo->opcode, status, block_operation_status, block_operation_qualifier);

        fbe_payload_metadata_set_status(mdo, metadata_status);

        /* We are trying to fix errors.  Zero out the blob and continue below to let the
         * client callback react to the invalid paged. 
         */
        if ((mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY_UPDATE) &&
            (metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE)) {
            /* For blocks that are uncorrectable they will be marked invalidated by RAID.
             * Check for the blocks with bad checksums and zero them to mark them not valid.
             */
            for (slot = 0; slot < paged_blob->slot_count; slot++) {
                fbe_sector_t *sector_p = (fbe_sector_t*)paged_blob->sg_list[slot].address;
                fbe_u16_t crc;
                crc = fbe_xor_lib_calculate_checksum((fbe_u32_t*)sector_p);

                if (sector_p->crc != crc) {
                    fbe_zero_memory(paged_blob->sg_list[slot].address, paged_blob->sg_list[slot].count);
                    fbe_metadata_calculate_checksum(paged_blob->sg_list[slot].address,  1);
                    metadata_trace(FBE_TRACE_LEVEL_WARNING, object_id,
                                   "mdata_paged_rd_blob_compl slot %lld CRC is BAD. %x != %x\n", 
                                   slot, sector_p->crc, crc);
                } else {
                    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW, object_id,
                                   "mdata_paged_rd_blob_compl slot %lld CRC OK\n", slot);
                }
            }
        } else if ((mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY) &&
                   (metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE)) {
            metadata_trace(FBE_TRACE_LEVEL_WARNING, object_id,
                           "mdata_paged_rd_blob_compl I/O fail for WRITE_VERIFY\n");
            for (slot = 0; slot < paged_blob->slot_count; slot++) {
                fbe_zero_memory(paged_blob->sg_list[slot].address, paged_blob->sg_list[slot].count);
                fbe_metadata_calculate_checksum(paged_blob->sg_list[slot].address,  1);
            }
        } else {
            /* We have to release blob here */
            fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

            metadata_paged_release_blob(paged_blob);
            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

            fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
            return fbe_metadata_paged_complete_packet(master_packet, (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET));
        }
    }

    if (mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NEED_VERIFY){
        fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE);
    } else {
        fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_OK);
    }

    //lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    //block_count = fbe_metadata_paged_get_block_count(mdo);
    if ((mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE) ||
        (mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY_UPDATE)) {
        
        fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
        /* Change the data and issue blob write */
        status = fbe_metadata_paged_call_callback_function(mdo, paged_blob, master_packet);

        if ((status == FBE_STATUS_MORE_PROCESSING_REQUIRED) || metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE){
            if ((metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE) || 
                (metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE)) {
                paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_WRITE_VERIFY_NEEDED;
            }
#if 0
            for (slot = 0; slot < block_count; slot++)
            {
                fbe_metadata_calculate_checksum(paged_blob->sg_list[lba_offset - paged_blob->lba + slot].address,  1);
            }
#endif
            /* We allocating exact number of slots we need for I/O (memory resource) */
            for (slot = 0; slot < paged_blob->slot_count; slot++) {
                fbe_metadata_calculate_checksum(paged_blob->sg_list[slot].address,  1);
            }

            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
            fbe_metadata_paged_write_blob(master_packet, paged_blob);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    
        /* Update object cache */
        fbe_metadata_paged_call_object_cache_function(FBE_METADATA_PAGED_CACHE_ACTION_POST_READ, mdo->metadata_element, mdo, 0, 0, paged_blob);

        /* Let's see if we need to continue (check the repeat count) */
        if ((mdo->u.metadata_callback.repeat_count > 1) && (mdo->u.metadata_callback.repeat_count > mdo->u.metadata_callback.current_count_private)){
            /* OK let's do it again */
            metadata_paged_release_blob(paged_blob);
            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
            metadata_paged_switch(master_packet, (fbe_packet_completion_context_t *)mdo);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

#if 0
        if (mdo->u.metadata_callback.repeat_count == 1){
            metadata_paged_blob_to_mdo(paged_blob, mdo);
            mdo->u.metadata_callback.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_VALID_DATA;
        }   
#endif

        //fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
        paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
        if (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_PRIVATE){
            metadata_paged_release_blob(paged_blob);
        } else {
            paged_blob->time_stamp = fbe_get_time(); 
        }

        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
        return fbe_metadata_paged_complete_packet(master_packet, (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET));
    }/* if (mdo->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE) */
    
    /* Update object cache */
    fbe_metadata_paged_call_object_cache_function(FBE_METADATA_PAGED_CACHE_ACTION_POST_READ, mdo->metadata_element, mdo, 0, 0, paged_blob);

    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);

    /* read was successful, free the resources and complete the packet */
    status = fbe_metadata_paged_call_callback_function(mdo, paged_blob, master_packet);
    fbe_payload_metadata_set_status(mdo, metadata_status);

    paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
    if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE) {
        /* mark the blob, it needs a write_verify on the next write */
        paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_WRITE_VERIFY_NEEDED;
    }

    if(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_PRIVATE){
        metadata_paged_release_blob(paged_blob);
    } else {
        paged_blob->time_stamp = fbe_get_time(); 
    }

    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    return fbe_metadata_paged_complete_packet(master_packet, (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET));
}

static fbe_status_t
fbe_metadata_paged_write_blob(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_object_id_t object_id;
    fbe_status_t status;
    metadata_paged_blob_t * paged_blob = NULL;
    fbe_payload_block_operation_opcode_t block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    void * verify_errors_p = NULL;
    fbe_time_t pdo_service_time_ms = 0;

    paged_blob = (metadata_paged_blob_t *)context;
    if (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT)
    {
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s paged blob %p IO already in progress \n", __FUNCTION__, paged_blob);
    }

    sep_payload = fbe_transport_get_payload_ex(packet);
    mdo =  metadata_paged_get_metadata_operation(sep_payload);
    metadata_element = mdo->metadata_element;
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
 
    /*let's make sure we propagate up the errors (if any), by giving the pointer to the sub packet,
    it will go al the way down to the RG handling the IO, so if it sees and errors, it will populate them 
    so the sender of the IO gets back the correct counts*/
    fbe_payload_ex_get_verify_error_count_ptr(sep_payload, &verify_errors_p);

    if (!(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET)) {
        new_packet = paged_blob->packet_ptr;
        fbe_transport_initialize_sep_packet(new_packet);    
        sep_payload = fbe_transport_get_payload_ex(new_packet);
        fbe_transport_propagate_expiration_time(new_packet, packet);
        fbe_transport_set_np_lock_from_master(packet, new_packet);
    } else {
        new_packet = packet;
    }

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, paged_blob->sg_list, METADATA_PAGED_BLOB_SLOTS);

    /* Allocate block operation */
    block_operation = fbe_payload_ex_allocate_block_operation(sep_payload);
    block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
    fbe_transport_set_packet_attr(new_packet, FBE_PACKET_FLAG_CONSUMED);

    /* let's check the flags of paged_blob, if marked write_verify_needed, issue a write_verify */
    if ((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) && (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_WRITE_VERIFY_NEEDED)) {
        block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY;
    }
    if(mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY) {
        block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY;
    }

    /* Check 4K alignment here */
    if(paged_blob->lba & 0x07){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s Unaligned paged_blob->lba 0x%llx \n", __FUNCTION__, paged_blob->lba);
    }



    fbe_payload_block_build_operation(block_operation,
                                      block_opcode,
                                      metadata_element->paged_record_start_lba + paged_blob->lba,    /* LBA */
                                      paged_blob->block_count,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    fbe_payload_block_set_flag(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP);

    if (!(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET)) {

        /* set the pdo_service_time_ms to 0 (default value) in the subpacket */
        fbe_transport_modify_physical_drive_service_time(new_packet, pdo_service_time_ms);

        status = fbe_transport_add_subpacket(packet, new_packet);

        if (status != FBE_STATUS_OK) { /* We did not added subpacket ( Most likely packet was cancelled ) */
            metadata_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s failed add subpacket status %d", __FUNCTION__, status);

            fbe_transport_destroy_packet(new_packet);

            fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
            metadata_paged_release_blob(paged_blob);
            fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        
            fbe_payload_metadata_set_status(mdo, FBE_PAYLOAD_METADATA_STATUS_CANCELLED);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        fbe_payload_ex_set_verify_error_count_ptr(sep_payload, verify_errors_p);
    }


    /* We already have a write lock , let's check if we another writes we can "bundle" together */
    /* The current operation of packet should be a stripe lock */

    /* We will "bundle" paged operations if repeat_count is 1 and stripe lock is acquired */
    if ((mdo->u.metadata_callback.repeat_count == 1) && (mdo->u.metadata_callback.stripe_count != 0)){
        metadata_paged_mark_bundle_start(packet, mdo, paged_blob);
    }

    paged_blob->flags |= METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT;

   /* Set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_paged_write_blob_completion, paged_blob);

    fbe_transport_set_address(new_packet, FBE_PACKAGE_ID_SEP_0, FBE_SERVICE_ID_TOPOLOGY, FBE_CLASS_ID_INVALID, object_id);

    new_packet->base_edge = NULL;
    fbe_payload_ex_increment_block_operation_level(sep_payload);

    //fbe_transport_set_packet_attr(new_packet, FBE_PACKET_FLAG_CONSUMED);
    status = fbe_topology_send_io_packet(new_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t 
fbe_metadata_paged_write_blob_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t *  master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_ex_t * master_payload = NULL;
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_status_t status;
    fbe_payload_metadata_status_t metadata_status;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_payload_block_operation_opcode_t        block_opcode;
    metadata_paged_blob_t * paged_blob = NULL;  
    fbe_object_id_t object_id;

    paged_blob = (metadata_paged_blob_t *)context;

    paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_FLIGHT;

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);
    
    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation,&block_operation_status);
    fbe_payload_block_get_qualifier(block_operation,&block_operation_qualifier);
    fbe_payload_block_get_opcode(block_operation,&block_opcode);
    fbe_payload_ex_release_block_operation(sep_payload, block_operation);


    if (!(paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET)) {
        master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
        master_payload = fbe_transport_get_payload_ex(master_packet);
        mdo = metadata_paged_get_metadata_operation(master_payload);
        metadata_element = mdo->metadata_element;
        object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

        /* remove the subpacket */
        fbe_transport_remove_subpacket(packet);

        /* destroy the packet */
        fbe_transport_destroy_packet(packet);
    } else {
        mdo = metadata_paged_get_metadata_operation(sep_payload);
        metadata_element = mdo->metadata_element;
        object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
        /* We do not have a master packet, so set master to be itself */
        master_packet = packet;
    }

    if (block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS){
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s object_id: 0x%X lba 0x%llX bc 0x%llX BOS %d\n", __FUNCTION__, object_id,
                       metadata_element->paged_record_start_lba + paged_blob->lba,
                       paged_blob->block_count,
                       block_operation_status);
    }

    metadata_status = fbe_metadata_determine_status(status, block_operation_status, block_operation_qualifier);
    fbe_payload_metadata_set_status(mdo, metadata_status);

    if (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK) {
        /* We can not do the write !!! This is bad! We done with this operation */
        if (mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_MASTER){
            metadata_paged_clear_bundle(master_packet, mdo, paged_blob, metadata_status);
        }

        fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
        metadata_paged_release_blob(paged_blob);
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s object_id: 0x%X lba 0x%llX bc 0x%llX MDS %d\n", __FUNCTION__, object_id,
                       metadata_element->paged_record_start_lba + paged_blob->lba,
                       paged_blob->block_count, metadata_status);

        fbe_payload_metadata_set_status(mdo, metadata_status);
        fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
        return fbe_metadata_paged_complete_packet(master_packet, (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET));
    }
    
    /* Update object cache */
    fbe_metadata_paged_call_object_cache_function(FBE_METADATA_PAGED_CACHE_ACTION_POST_WRITE, mdo->metadata_element, mdo, 0, 0, paged_blob);

    /* Let's see if we need to continue (check the repeat count) */
    if((mdo->u.metadata_callback.repeat_count > 1) && (mdo->u.metadata_callback.repeat_count > mdo->u.metadata_callback.current_count_private)){
        /* OK let's do it again */
        fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
        metadata_paged_release_blob(paged_blob);
        fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);
        metadata_paged_switch(master_packet, (fbe_packet_completion_context_t *)mdo);
        return FBE_STATUS_OK;
    }

#if 0
    /* If we succesfully completed small change bits operation we may update the record_data of mdo
        to avoid additional read.
     */
    if ((metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK) && (mdo->u.metadata_callback.repeat_count == 1)){
        metadata_paged_blob_to_mdo(paged_blob, mdo);
        mdo->u.metadata_callback.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_VALID_DATA;
    }   
#endif

    if(mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_MASTER){
        metadata_paged_mark_bundle_done(master_packet, mdo, paged_blob);
    }


    fbe_spinlock_lock(&metadata_element->paged_record_queue_lock);
    paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_IO_IN_PROGRESS;
    if ((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY) 
        && (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) {
        /* clear the write_verify flag*/
        paged_blob->flags &= ~METADATA_PAGED_BLOB_FLAG_WRITE_VERIFY_NEEDED;
    }

    if (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_PRIVATE){
        metadata_paged_release_blob(paged_blob);
    } else {
        paged_blob->time_stamp = fbe_get_time(); 
    }

    fbe_spinlock_unlock(&metadata_element->paged_record_queue_lock);

    if ((metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK) && (metadata_status != FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE)) {
        /* all the other errors */
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "mdata_paged_wt_blob_compl I/O fail,stat:0x%x bl_stat:0x%x bl_qualifier:0x%x\n",
                       status, block_operation_status, block_operation_qualifier);
    }


    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    return fbe_metadata_paged_complete_packet(master_packet, (paged_blob->flags & METADATA_PAGED_BLOB_FLAG_CLIENT_PACKET));
}


