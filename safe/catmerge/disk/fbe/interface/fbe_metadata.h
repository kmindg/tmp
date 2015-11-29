#ifndef FBE_METADATA_H
#define FBE_METADATA_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe_stripe_lock.h"
#include "fbe/fbe_metadata_interface.h"
#include "fbe/fbe_topology_interface.h"

#if defined(SIMMODE_ENV)
#define MAX_NONPAGED_ENTRIES FBE_MAX_SEP_OBJECTS
#else
#define MAX_NONPAGED_ENTRIES (FBE_MAX_SEP_OBJECTS / 8)
#endif

enum fbe_metadata_constants_e{
    FBE_METADATA_BLOCK_SIZE = 520,
    FBE_METADATA_BLOCK_DATA_SIZE = 512,
    FBE_METADATA_NONPAGED_MAX_OBJECTS = MAX_NONPAGED_ENTRIES,
};

typedef struct fbe_metadata_memory_s{
    fbe_u8_t * memory_ptr;
    fbe_u32_t memory_size;
}fbe_metadata_memory_t;

typedef struct fbe_metadata_record_s{
    fbe_lba_t                   lba;            /* The first lba for this record */
    fbe_u8_t                  * data_ptr;       /* Data memory */
    fbe_u32_t                   data_size;      /* Size of data memory in bytes*/
}fbe_metadata_record_t;

typedef struct fbe_metadata_paged_record_s{
    fbe_queue_element_t         queue_element; /* Will be used to queue record on paged queue */
    fbe_metadata_record_t       metadata_record;
}fbe_metadata_paged_record_t;

typedef enum fbe_metadata_element_attributes_e {
    FBE_METADATA_ELEMENT_ATTRIBUTES_INVALID,
    FBE_METADATA_ELEMENT_ATTRIBUTES_SL_HASH_DISABLED	 = 0x00000001, /* There are large I/O's in progress. SL hash table is disabled */
    //FBE_METADATA_ELEMENT_ATTRIBUTES_HOLD_STRIPE_LOCKS	 = 0x00000002, /* Don't start any new stripe locks. */
    FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_STRIPE_LOCKS	 = 0x00000004, /* Abort any new stripe locks except non paged. */
    FBE_METADATA_ELEMENT_ATTRIBUTES_NONPAGED_REQUEST     = 0x00000008, /* Set by active SP and cleared at the completion of non-paged metadata request from Peer. */
    FBE_METADATA_ELEMENT_ATTRIBUTES_ABORTED_REQUESTS	 = 0x00000010, /*!< We have packets that were aborted.  Abort thread will clear this. */
    FBE_METADATA_ELEMENT_ATTRIBUTES_FAIL_PAGED       	 = 0x00000020, /*!< Fail any new paged requests. */
    FBE_METADATA_ELEMENT_ATTRIBUTES_HOLD_PEER_DATA_STRIPE_LOCKS = 0x00000040, /*!< When releasing stripe_locks on peer contact lost, this attribute     
                                                                                   indicates if we should keep peer's data stripe locks as held. */
    FBE_METADATA_ELEMENT_ATTRIBUTES_DESTROY_ABORT_STRIPE_LOCKS = 0x00000080, /*!< Abort any new stripe locks including non paged. */
    FBE_METADATA_ELEMENT_ATTRIBUTES_ELEMENT_REINITED     = 0x00000100, /**/
    FBE_METADATA_ELEMENT_ATTRIBUTES_PAGED_LOCAL_CACHE    = 0x00000200, /* Use local cache for paged metadata. */
    FBE_METADATA_ELEMENT_ATTRIBUTES_PEER_DEAD       	 = 0x01000000, /*!< Set when memory update bring DESTROY state */ 
    FBE_METADATA_ELEMENT_ATTRIBUTES_DISBALE_CMI          = 0x02000000, /*!< We are about to die so let's stop send/recive CMI */ 
    FBE_METADATA_ELEMENT_ATTRIBUTES_PEER_PERSIST_PENDING = 0x04000000, /*!< Peer is in the process of persisting the data */

    FBE_METADATA_ELEMENT_ATTRIBUTES_SL_PEER_LOST         = 0x08000000, /*!< Set when stripe lock get peer lost event, clear when all SL slots cleared */

    FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_MONITOR_OPS    = 0x10000000, /*!< Set to abort monitor ops when peer lost. */
    FBE_METADATA_ELEMENT_ATTRIBUTES_CMI_MSG_PROCESSING   = 0x20000000, /*!< Set to indicate processing received CMI messages */

}fbe_metadata_element_attributes_t;

typedef enum fbe_metadata_paged_cache_action_e {
    FBE_METADATA_PAGED_CACHE_ACTION_INVALID,
    FBE_METADATA_PAGED_CACHE_ACTION_PRE_READ   = 0x00000001,
    FBE_METADATA_PAGED_CACHE_ACTION_POST_READ  = 0x00000002,
    FBE_METADATA_PAGED_CACHE_ACTION_POST_WRITE = 0x00000003,
    FBE_METADATA_PAGED_CACHE_ACTION_INVALIDATE = 0x00000004,
    FBE_METADATA_PAGED_CACHE_ACTION_PRE_UPDATE = 0x00000005,
    FBE_METADATA_PAGED_CACHE_ACTION_NONPAGE_CHANGED = 0x00000006,
}fbe_metadata_paged_cache_action_t;

typedef void * fbe_metadata_paged_cache_callback_context_t;
typedef fbe_status_t (* fbe_metadata_paged_cache_callback_t) (fbe_metadata_paged_cache_action_t action,
                                                              struct fbe_metadata_element_s * metadata_element,
                                                              fbe_payload_metadata_operation_t * mdo,
                                                              fbe_lba_t start_lba, fbe_block_count_t block_count);

typedef struct fbe_metadata_element_s{
    fbe_queue_element_t queue_element; /* Will be used to queue object with metadata service */
    
    fbe_metadata_memory_t metadata_memory;/* in order to be able to deal with the correct information, we need a SPA/SPB split at this level*/
    fbe_metadata_memory_t metadata_memory_peer;

    fbe_u64_t peer_metadata_element_ptr; /* Memory address of peer metadata element */

    fbe_metadata_record_t nonpaged_record; /* Nonpaged record of the leaf class */

    fbe_lba_t           paged_record_start_lba;
    fbe_lba_t           paged_mirror_metadata_offset;
    fbe_lba_t           paged_metadata_capacity;  /* paged metadata capacity in block*/
    fbe_metadata_element_state_t metadata_element_state;

    fbe_u32_t           paged_record_request_count;  /* Number of requests currently in flight. */
    fbe_queue_head_t    paged_record_queue_head;     /* Paged record queue */
    fbe_queue_head_t	paged_blob_queue_head;
    fbe_spinlock_t      paged_record_queue_lock;    /*  Lock to protect the paged queues. */


    fbe_queue_head_t    stripe_lock_queue_head;	/* Stripe lock queue */
	fbe_atomic_t		stripe_lock_large_io_count; /* Number of large I/O's in progress */
	fbe_u64_t			stripe_lock_number_of_stripes; /* Overall number of stripes */
    fbe_u64_t			stripe_lock_number_of_private_stripes; /* number of private stripes */    
    fbe_metadata_stripe_lock_blob_t * stripe_lock_blob;
	fbe_metadata_stripe_lock_hash_t * stripe_lock_hash;

    /* Stripe lock Statistic counters */
    fbe_u64_t			stripe_lock_count;
    fbe_u64_t			local_collision_count;
    fbe_u64_t			peer_collision_count;
    fbe_u64_t			cmi_message_count;

    fbe_spinlock_t      stripe_lock_spinlock;    /*  Lock to protect the stripe lock queues + stats. */	

    FBE_ALIGN(4) fbe_atomic_32_t   attributes;

    fbe_spinlock_t      metadata_element_lock;			/* Lock to protect attributes and nonpaged. */

    fbe_metadata_paged_cache_callback_t cache_callback;

}fbe_metadata_element_t;

fbe_status_t fbe_metadata_register(fbe_metadata_element_t * metadata_element);
fbe_status_t fbe_metadata_unregister(fbe_metadata_element_t * metadata_element);
fbe_status_t fbe_metadata_element_set_state(fbe_metadata_element_t *metadata_element,
                                            fbe_metadata_element_state_t state);
fbe_status_t fbe_metadata_element_init_memory(struct fbe_packet_s * packet);
fbe_status_t fbe_metadata_element_reinit_memory(struct fbe_packet_s * packet);

fbe_status_t fbe_metadata_element_init_records(struct fbe_packet_s * packet);
 
/* fbe_status_t fbe_metadata_element_zero_records(struct fbe_packet_s * packet); */

fbe_status_t fbe_metadata_element_destroy(fbe_metadata_element_t * metadata_element);
fbe_status_t fbe_metadata_element_outstanding_stripe_lock_request(fbe_metadata_element_t * metadata_element);

fbe_status_t fbe_metadata_element_get_state(fbe_metadata_element_t * metadata_element, fbe_metadata_element_state_t * metadata_element_state);
fbe_bool_t fbe_metadata_element_is_active(fbe_metadata_element_t * metadata_element);

fbe_status_t fbe_metadata_element_set_paged_record_start_lba(fbe_metadata_element_t * metadata_element_p, fbe_lba_t paged_record_start_lba);
fbe_status_t fbe_metadata_element_get_paged_record_start_lba(fbe_metadata_element_t * metadata_element_p, fbe_lba_t * paged_record_start_lba_p);

fbe_status_t fbe_metadata_element_set_paged_mirror_metadata_offset(fbe_metadata_element_t * metadata_element_p, fbe_lba_t paged_mirror_metadata_offset);
fbe_status_t fbe_metadata_element_get_paged_mirror_metadata_offset(fbe_metadata_element_t * metadata_element_p, fbe_lba_t * paged_mirror_metadata_offset_p);

fbe_status_t fbe_metadata_element_set_paged_metadata_capacity(fbe_metadata_element_t * metadata_element_p, fbe_lba_t paged_metadata_capacity);
fbe_status_t fbe_metadata_element_get_paged_metadata_capacity(fbe_metadata_element_t * metadata_element_p, fbe_lba_t * paged_metadata_capacity_p);

fbe_status_t fbe_metadata_element_set_number_of_stripes(fbe_metadata_element_t * metadata_element_p, fbe_u64_t number_of_stripes);
fbe_status_t fbe_metadata_element_get_number_of_stripes(fbe_metadata_element_t * metadata_element_p, fbe_u64_t * number_of_stripes);

fbe_status_t fbe_metadata_element_set_number_of_private_stripes(fbe_metadata_element_t * metadata_element_p, fbe_u64_t number_of_stripes);
fbe_status_t fbe_metadata_element_get_number_of_private_stripes(fbe_metadata_element_t * metadata_element_p, fbe_u64_t * number_of_stripes);

fbe_status_t fbe_metadata_element_is_quiesced(fbe_metadata_element_t* metadata_element, fbe_bool_t *b_quiesced_p);

fbe_status_t fbe_metadata_element_restart_io(fbe_metadata_element_t * metadata_element);
fbe_status_t fbe_metadata_element_abort_io(fbe_metadata_element_t* metadata_element_p);
fbe_status_t fbe_metadata_element_destroy_abort_io(fbe_metadata_element_t* metadata_element_p);
fbe_status_t fbe_metadata_stripe_lock_element_abort(fbe_metadata_element_t * metadata_element);
fbe_status_t fbe_metadata_stripe_lock_element_destroy_abort(fbe_metadata_element_t * metadata_element);
fbe_status_t fbe_metadata_element_abort_paged(fbe_metadata_element_t* metadata_element);
fbe_status_t fbe_metadata_stripe_lock_element_abort_monitor_ops(fbe_metadata_element_t * metadata_element, fbe_bool_t abort_peer);
fbe_status_t metadata_stripe_lock_update(fbe_metadata_element_t * mde);

fbe_status_t fbe_metadata_element_clear_abort(fbe_metadata_element_t* metadata_element_p);
fbe_status_t fbe_metadata_element_get_metadata_memory(fbe_metadata_element_t * metadata_element_p,  void **memory_ptr);
fbe_status_t fbe_metadata_element_get_metadata_memory_size(fbe_metadata_element_t * metadata_element_p, fbe_u32_t * memory_size);

fbe_status_t fbe_metadata_element_get_peer_metadata_memory(fbe_metadata_element_t * metadata_element_p,  void **memory_ptr);
fbe_status_t fbe_metadata_element_get_peer_metadata_memory_size(fbe_metadata_element_t * metadata_element_p, fbe_u32_t *memory_size);

fbe_status_t fbe_metadata_element_set_nonpaged_request(fbe_metadata_element_t* metadata_element_p);
fbe_status_t fbe_metadata_element_clear_nonpaged_request(fbe_metadata_element_t* metadata_element_p);
fbe_bool_t fbe_metadata_element_is_nonpaged_request_set(fbe_metadata_element_t* metadata_element_p);

fbe_status_t fbe_metadata_element_hold_peer_data_stripe_locks_on_contact_loss(fbe_metadata_element_t* metadata_element_p);

/* The following control codes should be sent by objects ONLY */

/* FBE_METADATA_CONTROL_CODE_ELEMENT_ZERO_RECORDS */
typedef fbe_metadata_element_t fbe_metadata_control_element_zero_records_t; /* This should NOT be modified !!! */

fbe_status_t fbe_metadata_operation_entry(struct fbe_packet_s *  packet);
fbe_status_t fbe_metadata_paged_clear_cache(fbe_metadata_element_t * metadata_element);

static __forceinline fbe_lba_t fbe_metadata_paged_get_lba_offset(fbe_payload_metadata_operation_t * mdo)
{
    fbe_lba_t start_lba;
    fbe_u64_t offset;

    offset = mdo->u.metadata_callback.offset + mdo->u.metadata_callback.current_count_private * mdo->u.metadata_callback.record_data_size;
    start_lba = offset / FBE_METADATA_BLOCK_DATA_SIZE;

    return (start_lba);
}
static __forceinline fbe_u64_t fbe_metadata_paged_get_slot_offset(fbe_payload_metadata_operation_t * mdo)
{
    fbe_u64_t offset;

    offset = mdo->u.metadata_callback.offset + mdo->u.metadata_callback.current_count_private * mdo->u.metadata_callback.record_data_size;
    return (offset % FBE_METADATA_BLOCK_DATA_SIZE);
}

static __forceinline fbe_u64_t fbe_metadata_paged_get_record_offset(fbe_payload_metadata_operation_t * mdo)
{
    fbe_u64_t offset;

    offset = (mdo->u.metadata_callback.offset / mdo->u.metadata_callback.record_data_size) + mdo->u.metadata_callback.current_count_private;
    return offset;
}

/* This function should be set on packet stack to call fbe_metadata_operation_entry */
fbe_status_t fbe_metadata_operation_entry_completion(struct fbe_packet_s *  packet, void * context);

/* This function should be set on packet stack to call fbe_metadata_operation_entry */
fbe_status_t fbe_metadata_operation_entry_completion(struct fbe_packet_s *  packet, void * context);

fbe_bool_t fbe_metadata_stripe_lock_is_started(fbe_metadata_element_t * metadata_element);

fbe_status_t fbe_metadata_paged_get_lock_range(fbe_payload_metadata_operation_t * metadata_operation, 
                                               fbe_lba_t * lba, 
                                               fbe_block_count_t * block_count);

/*we have the pointer to the metadata queue element from the peer*/
fbe_bool_t fbe_metadata_is_peer_object_alive(fbe_metadata_element_t * metadata_element);

/*we beleive the peer is at a DESTROYED state*/
fbe_status_t fbe_metadata_element_set_peer_dead(fbe_metadata_element_t* metadata_element_p);

/*we beleive the peer is not at a DESTROYED state*/
fbe_status_t fbe_metadata_element_clear_peer_dead(fbe_metadata_element_t* metadata_element_p);

fbe_status_t fbe_metadata_element_clear_sl_peer_lost(fbe_metadata_element_t* metadata_element_p);

fbe_status_t fbe_metadata_element_clear_abort_monitor_ops(fbe_metadata_element_t* metadata_element_p);

/*this function checks if the peer (which may have been alive once) is at a DESTRYED state*/
fbe_bool_t fbe_metadata_element_is_peer_dead(fbe_metadata_element_t* metadata_element_p);

/*we want_to_make sure we stop CMI in preperation of killing ourselvs*/
fbe_status_t fbe_metadata_element_disable_cmi(fbe_metadata_element_t* metadata_element_p);

fbe_bool_t fbe_metadata_element_is_cmi_disabled(fbe_metadata_element_t* metadata_element_p);

fbe_bool_t fbe_stripe_lock_was_paged_slot_held_by_peer(fbe_metadata_element_t *mde);

fbe_status_t fbe_metadata_element_set_peer_persist_pending(fbe_metadata_element_t* metadata_element_p);

fbe_status_t fbe_metadata_element_clear_peer_persist_pending(fbe_metadata_element_t* metadata_element_p);

fbe_bool_t fbe_metadata_element_is_peer_persist_pending(fbe_metadata_element_t* metadata_element_p);

fbe_status_t fbe_stripe_lock_release_peer_data_stripe_locks(fbe_metadata_element_t* metadata_element_p);
fbe_status_t fbe_metadata_element_set_element_reinited(fbe_metadata_element_t* metadata_element_p);
fbe_status_t fbe_metadata_element_clear_element_reinited(fbe_metadata_element_t* metadata_element_p);
fbe_bool_t fbe_metadata_element_is_element_reinited(fbe_metadata_element_t* metadata_element_p);

fbe_status_t fbe_metadata_element_set_paged_object_cache_flag(fbe_metadata_element_t* metadata_element_p);
fbe_bool_t fbe_metadata_element_is_paged_object_cache(fbe_metadata_element_t* metadata_element_p);
fbe_status_t fbe_metadata_element_set_paged_object_cache_function(fbe_metadata_element_t* metadata_element_p, 
																  fbe_metadata_paged_cache_callback_t callback_function);
fbe_status_t fbe_metadata_element_clear_paged_object_cache_flag(fbe_metadata_element_t* metadata_element_p);


fbe_status_t fbe_metadata_nonpaged_disable_raw_mirror(void);
void fbe_metadata_get_control_mem_use_mb(fbe_u32_t *memory_use_mb_p);

/*functions and structure related with metadata raw mirror rebuild*/

typedef struct fbe_metadata_raw_mirror_io_cb_s{

    /*spinlock used to protect the system_object_need_rebuild_counts array,
         because it can be accessed both in database thread context and 
         metadata raw mirror rebuild context.
      */
    fbe_spinlock_t      lock;


    /*total block numbers in database raw-3way-mirror*/
    fbe_block_count_t block_count;          

    /*it is an array to store the sequence number of each block of database raw-mirror*/
    fbe_u64_t* seq_numbers;
} fbe_metadata_raw_mirror_io_cb_t;

fbe_status_t fbe_metadata_translate_metadata_status_to_block_status(
                                    fbe_payload_metadata_status_t         in_metadata_status,                   
                                    fbe_payload_block_operation_status_t* out_block_status_p,
                                    fbe_payload_block_operation_qualifier_t *out_block_qualifier_p);

fbe_status_t fbe_metadata_translate_metadata_status_to_packet_status(fbe_payload_metadata_status_t in_metadata_status,
                                                                     fbe_status_t *out_packet_status_p);



fbe_status_t fbe_metadata_raw_mirror_rebuild_init(void);
fbe_status_t fbe_metadata_wake_up_raw_mirror_rebuild(void);
fbe_bool_t fbe_metadata_check_raw_mirror_rebuild_running(void);
fbe_status_t fbe_metadata_notify_raw_mirror_rebuild(fbe_lba_t start_lba, fbe_block_count_t block_counts);
fbe_status_t fbe_metadata_get_system_object_need_rebuild(fbe_object_id_t object_id, fbe_u32_t *out_need_rebuild_count);
fbe_status_t fbe_metadata_clear_system_object_need_rebuild(fbe_object_id_t object_id);
fbe_status_t fbe_metadata_decrease_system_object_need_rebuild(fbe_object_id_t object_id,fbe_u32_t need_rebild_count);
void fbe_metadata_raw_mirror_background_rebuild_thread(void *context);
fbe_status_t fbe_metadata_raw_mirror_rebuild_destroy(void);

fbe_status_t fbe_metadata_paged_init_client_blob(fbe_memory_request_t * memory_request_p, fbe_payload_metadata_operation_t * mdo, 
                                                 fbe_u32_t skip_bytes, fbe_bool_t use_client_packet);
fbe_status_t fbe_metadata_paged_init_client_blob_with_sg_list(fbe_sg_element_t * sg_p, fbe_payload_metadata_operation_t * mdo);

fbe_status_t fbe_metadata_stripe_lock_all(fbe_metadata_element_t * mde);
fbe_status_t fbe_metadata_stripe_unlock_all(fbe_metadata_element_t * mde);

fbe_status_t fbe_metadata_stripe_lock_hash(fbe_metadata_element_t * mde, fbe_payload_stripe_lock_operation_t * sl);
fbe_status_t fbe_metadata_stripe_unlock_hash(fbe_metadata_element_t * mde, fbe_payload_stripe_lock_operation_t * sl);
fbe_bool_t fbe_metadata_is_abort_set(fbe_metadata_element_t * mde);

fbe_status_t fbe_ext_pool_lock_all(fbe_metadata_element_t * mde);
fbe_status_t fbe_ext_pool_unlock_all(fbe_metadata_element_t * mde);
fbe_bool_t fbe_ext_pool_lock_is_started(fbe_metadata_element_t * mde);
fbe_status_t fbe_ext_pool_lock_hash(fbe_metadata_element_t * mde, fbe_payload_stripe_lock_operation_t * sl);
fbe_status_t fbe_ext_pool_unlock_hash(fbe_metadata_element_t * mde, fbe_payload_stripe_lock_operation_t * sl);
fbe_status_t fbe_ext_pool_lock_release_peer_data_stripe_locks(fbe_metadata_element_t* metadata_element_p);
fbe_status_t fbe_ext_pool_lock_operation_entry(fbe_packet_t *  packet);

#endif /* FBE_METADATA_H */
 
