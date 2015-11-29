#ifndef FBE_PAYLOAD_STRIPE_LOCK_OPERATION_H
#define FBE_PAYLOAD_STRIPE_LOCK_OPERATION_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_payload_operation_header.h"
#include "fbe/fbe_queue.h"


typedef enum fbe_metadata_cmi_message_type_e {
	FBE_METADATA_CMI_MESSAGE_TYPE_INVALID,
	FBE_METADATA_CMI_MESSAGE_TYPE_MEMORY_UPDATE, /* Object metadata memory update */
	FBE_METADATA_CMI_MESSAGE_TYPE_NONPAGED_WRITE,
	FBE_METADATA_CMI_MESSAGE_TYPE_NONPAGED_POST_PERSIST,
	FBE_METADATA_CMI_MESSAGE_TYPE_SET_BITS,
	FBE_METADATA_CMI_MESSAGE_TYPE_CLEAR_BITS,
	FBE_METADATA_CMI_MESSAGE_TYPE_SET_CHECKPOINT,
	FBE_METADATA_CMI_MESSAGE_TYPE_FORCE_SET_CHECKPOINT,
	FBE_METADATA_CMI_MESSAGE_TYPE_INCR_CHECKPOINT,
	
	FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_LOCK_START,
	FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_LOCK_STOP,
	
	FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_LOCK,
	FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_LOCK,

	FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_GRANT,
	FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_GRANT,

	FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_RELEASE, /* Release peer stripe lock */
	FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_ABORTED, /* Peer request was aborted */

}fbe_metadata_cmi_message_type_t;

typedef struct fbe_metadata_stripe_lock_region_s{
    fbe_lba_t	first;  /* First stripe to lock */
    fbe_lba_t	last;   /* Last stripe to lock can be equal to first if block count eq. to 1*/
}fbe_metadata_stripe_lock_region_t;

typedef struct fbe_metadata_cmi_message_header_s {
	fbe_object_id_t object_id;

	fbe_u64_t metadata_element_receiver; /* If not NULL should point to the memory of the receiver */
	fbe_u64_t metadata_element_sender; /* Should point to the memory of the sender */

	fbe_metadata_cmi_message_type_t			metadata_cmi_message_type;
	fbe_payload_metadata_operation_flags_t  metadata_operation_flags; /* This mirrors the metadata operation flags and should not be used for any CMI related stuff */
}fbe_metadata_cmi_message_header_t;

enum fbe_metadata_cmi_stripe_lock_flags_e{
	FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE = 0x00000001, /* This grant need to be released */

	FBE_METADATA_CMI_STRIPE_LOCK_FLAG_ABORT_DESTROY = 0x00000010, /* This SL is aborted due to destroy */
	FBE_METADATA_CMI_STRIPE_LOCK_FLAG_MONITOR_OP = 0x00000020, /* This is a monitor op */
};

typedef fbe_u32_t fbe_metadata_cmi_stripe_lock_flags_t;

typedef struct fbe_metadata_cmi_stripe_lock_s {
	fbe_metadata_cmi_message_header_t header;

	fbe_metadata_stripe_lock_region_t	write_region;
	fbe_metadata_stripe_lock_region_t	read_region;
	struct fbe_packet_s				   * packet; 

	/* Version 1. (Nice NDU example for MR1) */
	void * request_sl_ptr;
	void * grant_sl_ptr; /* Valid if NEED_RELEASE is set */
	fbe_metadata_cmi_stripe_lock_flags_t flags;
}fbe_metadata_cmi_stripe_lock_t;

enum fbe_metadata_stripe_lock_constants_e{
    METADATA_STRIPE_LOCK_MAX_BLOBS			= 500, /* 500 Raid's to cover 1000 drives  */
	METADATA_STRIPE_LOCK_SLOTS				= 514, /* in bytes */
#if defined(SIMMODE_ENV)
	METADATA_STRIPE_LOCK_MAX_PEER_SL		= 2000, /* 1024 peer stripe lock requests  */
#else
	METADATA_STRIPE_LOCK_MAX_PEER_SL		= 10240, /* 1024 peer stripe lock requests  */
#endif
	METADATA_STRIPE_LOCK_BITS_PER_SLOT		= 2,   /* bits per slot */
	METADATA_STRIPE_LOCK_SLOTS_PER_BYTE		= 4,   /* slots per byte */

	METADATA_STRIPE_LOCK_HASH_TABLE_SIZE		= 16,	/* Stripe lock hash table size */
	METADATA_STRIPE_LOCK_HASH_TABLE_MASK		= 0x0F, /* Stripe lock hash mask for the table size above */
};

typedef enum fbe_metadata_lock_slot_state_e{
    METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL = 0x000,
    METADATA_STRIPE_LOCK_SLOT_STATE_SHARED			= 0x001,
    METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_PEER	= 0x003,
	METADATA_STRIPE_LOCK_SLOT_STATE_MASK			= 0x003,
}fbe_metadata_lock_slot_state_t;

enum fbe_metadata_lock_blob_flags_e {
	METADATA_STRIPE_LOCK_BLOB_FLAG_STARTED		= 0x00000001,
	METADATA_STRIPE_LOCK_BLOB_FLAG_ENABLE_HASH	= 0x00000010,

};

typedef fbe_u32_t fbe_metadata_lock_blob_flags_t;

typedef struct fbe_metadata_lock_blob_byte_s{
    fbe_u8_t subslot_0    : 2;         // bit 0-1;
    fbe_u8_t subslot_1    : 2;         // bit 2-3;
    fbe_u8_t subslot_2    : 2;         // bit 4-5;
    fbe_u8_t subslot_3    : 2;         // bit 6-7;
} fbe_metadata_lock_blob_byte_t;

typedef struct fbe_metadata_stripe_lock_blob_s{
	fbe_queue_element_t queue_element; /* This will help to enqueue the blob */
	fbe_queue_head_t	peer_sl_queue; /* Queue of peer related sl's */
	fbe_spinlock_t		peer_sl_queue_lock; /* Spinlock to protect peer_sl_queue */
	struct fbe_metadata_element_s * mde;
	fbe_u32_t size; /* in bytes; 4 slots per byte */
	fbe_u32_t write_count; /* Number of write locks in progress */
	fbe_u64_t user_slot_size;
	fbe_u64_t private_slot;
	fbe_u64_t nonpaged_slot;
	fbe_metadata_lock_blob_flags_t flags;
	FBE_ALIGN(8) fbe_u8_t  slot[METADATA_STRIPE_LOCK_SLOTS]; /* MUST be LAST */
}fbe_metadata_stripe_lock_blob_t;

typedef struct fbe_metadata_stripe_lock_hash_table_entry_s {
	void *				lock;
	fbe_queue_head_t	head;
}fbe_metadata_stripe_lock_hash_table_entry_t;

typedef struct fbe_metadata_stripe_lock_hash_s {
    fbe_u64_t          count; /* Number of I/O's which cross default boundaries */ 
    fbe_u64_t          table_size;
    fbe_u64_t          divisor;
    fbe_u64_t          default_divisor;
    fbe_metadata_stripe_lock_hash_table_entry_t   * table;
} fbe_metadata_stripe_lock_hash_t;

typedef struct fbe_ext_pool_lock_slice_entry_s {
	fbe_u64_t			slice_lock_state;  /* This is the sticky lock for the slice */
	void *				lock;
	fbe_queue_head_t	head;
}fbe_ext_pool_lock_slice_entry_t;


typedef enum fbe_payload_stripe_lock_status_e{
	FBE_PAYLOAD_STRIPE_LOCK_STATUS_INVALID,

	FBE_PAYLOAD_STRIPE_LOCK_STATUS_INITIALIZED, /* Just build */

	FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING, /* In progress */

    FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK,
    FBE_PAYLOAD_STRIPE_LOCK_STATUS_ILLEGAL_REQUEST,
     
    FBE_PAYLOAD_STRIPE_LOCK_STATUS_DROPPED, /*! User decided not to hold these requests. */
    FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED,   /*! User decided to abort this request. */
    FBE_PAYLOAD_STRIPE_LOCK_STATUS_CANCELLED, /*! Packet was cancelled, so MDS returning with error. */

    FBE_PAYLOAD_STRIPE_LOCK_STATUS_LAST
}fbe_payload_stripe_lock_status_t;

enum fbe_payload_stripe_lock_flags_e{
	FBE_PAYLOAD_STRIPE_LOCK_FLAG_ALLOW_HOLD			= 0x00000001,
	FBE_PAYLOAD_STRIPE_LOCK_FLAG_SYNC_MODE			= 0x00000002,
	FBE_PAYLOAD_STRIPE_LOCK_FLAG_DO_NOT_LOCK		= 0x00000004, /* Do not get a stripe lock. Just complete packet. */

	FBE_PAYLOAD_STRIPE_LOCK_FLAG_NO_ABORT			= 0x00000010,

	FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER	= 0x00010000, /* This operation waiting for peer */
	FBE_PAYLOAD_STRIPE_LOCK_FLAG_RETRY				= 0x00020000, /* This needs to be retried */

	FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST		= 0x00040000, /* This operation is peer request */
	FBE_PAYLOAD_STRIPE_LOCK_FLAG_FULL_SLOT			= 0x00080000, /* Peer asked to grant the full slot (Version 0) */

	FBE_PAYLOAD_STRIPE_LOCK_FLAG_HOLDING_PEER		= 0x00100000, /* Peer request queued on this operation */
	FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANT				= 0x00200000, /* Peer request ready to be granted */
	FBE_PAYLOAD_STRIPE_LOCK_FLAG_GRANTED			= 0x00400000, /* Peer request granted */

	FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_COLLISION		= 0x00800000, /* This sl has a peer collision */
	FBE_PAYLOAD_STRIPE_LOCK_FLAG_LOCAL_REQUEST		= 0x01000000, /* This operation is peer request */

	FBE_PAYLOAD_STRIPE_LOCK_FLAG_DEAD_LOCK			= 0x02000000, /* Dead lock was detected */

	FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_LOST      	= 0x04000000, /* peer lost was detected */
};

typedef fbe_u32_t fbe_payload_stripe_lock_flags_t;

enum fbe_payload_stripe_lock_priv_flags_e{
	FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_PENDING		= 0x00000001, /* If set - the SL is pending */
	FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_READ			= 0x00000002, /* If set - the SL is read lock, otherwise - write lock */
	FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LOCAL_GRANT	= 0x00000004, /* The stripe lock was granted localy */
	FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_NP			= 0x00000008, /* Non paged SL */
	FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_PAGED			= 0x00000010, /* Paged SL */
	FBE_PAYLOAD_STRIPE_LOCK_PRIV_FLAG_LARGE			= 0x00000020, /* This large SL crosses the hash boundaries */
};

typedef fbe_u32_t fbe_payload_stripe_lock_priv_flags_t;


typedef enum fbe_payload_stripe_lock_operation_opcode_e{
    FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_INVALID,
    FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK,
    FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_UNLOCK,

    FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK,
    FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK,

    FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_START,
    FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_STOP,


    FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_LAST
}fbe_payload_stripe_lock_operation_opcode_t;

//enum fbe_payload_stripe_lock_constants_e{};


typedef struct fbe_payload_stripe_lock_operation_s{
    fbe_payload_operation_header_t				operation_header; /* Must be first */

    fbe_payload_stripe_lock_operation_opcode_t  opcode;     
    fbe_payload_stripe_lock_status_t            status;

	fbe_payload_stripe_lock_flags_t				flags;
	fbe_payload_stripe_lock_priv_flags_t		priv_flags; /* Tis flags will be used by SL service ONLY */

    fbe_metadata_stripe_lock_region_t	stripe;
            
    fbe_queue_element_t					queue_element; /* This operation will be queued */
	fbe_metadata_cmi_stripe_lock_t      cmi_stripe_lock;
	fbe_queue_head_t					wait_queue;	
}fbe_payload_stripe_lock_operation_t;

/* Stripe lock operation functions */

static __forceinline fbe_payload_stripe_lock_operation_t * 
fbe_metadata_stripe_lock_queue_element_to_operation(fbe_queue_element_t * queue_element)
{
    fbe_payload_stripe_lock_operation_t * operation;
    operation = (fbe_payload_stripe_lock_operation_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_payload_stripe_lock_operation_t  *)0)->queue_element));
    return operation;
}

static __forceinline fbe_status_t 
fbe_payload_stripe_lock_get_status(fbe_payload_stripe_lock_operation_t * stripe_lock_operation, fbe_payload_stripe_lock_status_t * status)
{
	*status = stripe_lock_operation->status;
	return FBE_STATUS_OK;
}

static __forceinline fbe_status_t 
fbe_payload_stripe_lock_set_status(fbe_payload_stripe_lock_operation_t * stripe_lock_operation, fbe_payload_stripe_lock_status_t status)
{
	stripe_lock_operation->status = status;
	return FBE_STATUS_OK;
}

static __forceinline fbe_status_t 
fbe_payload_stripe_lock_get_opcode(fbe_payload_stripe_lock_operation_t * stripe_lock_operation, fbe_payload_stripe_lock_operation_opcode_t * opcode)
{
	* opcode = stripe_lock_operation->opcode;
	return FBE_STATUS_OK;
}

fbe_status_t fbe_payload_stripe_lock_build_read_lock(fbe_payload_stripe_lock_operation_t * stripe_lock_operation,
                                                     struct fbe_metadata_element_s * metadata_element,
                                                     fbe_lba_t  stripe_number,
                                                     fbe_block_count_t  stripe_count);

fbe_status_t fbe_payload_stripe_lock_build_read_unlock(fbe_payload_stripe_lock_operation_t * stripe_lock_operation);

fbe_status_t fbe_payload_stripe_lock_build_write_lock(fbe_payload_stripe_lock_operation_t * stripe_lock_operation,
                                                      struct fbe_metadata_element_s * metadata_element,
                                                      fbe_lba_t   stripe_number,
                                                      fbe_block_count_t   stripe_count);


fbe_status_t fbe_payload_stripe_lock_build_write_unlock(fbe_payload_stripe_lock_operation_t * stripe_lock_operation);


fbe_status_t fbe_payload_stripe_lock_build_start(fbe_payload_stripe_lock_operation_t * stripe_lock_operation,
                                                        struct fbe_metadata_element_s * metadata_element);

fbe_status_t fbe_payload_stripe_lock_build_stop(fbe_payload_stripe_lock_operation_t * stripe_lock_operation,
                                                        struct fbe_metadata_element_s * metadata_element);



fbe_status_t fbe_payload_stripe_lock_set_sync_mode(fbe_payload_stripe_lock_operation_t * stripe_lock_operation);


#endif /* FBE_PAYLOAD_STRIPE_LOCK_OPERATION_H */


  
