#ifndef FBE_PAYLOAD_METADATA_OPERATION_H
#define FBE_PAYLOAD_METADATA_OPERATION_H
 
#include "fbe/fbe_types.h"
#include "fbe/fbe_payload_operation_header.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_sg_element.h"

/* #include "fbe_metadata.h" */

typedef enum fbe_payload_metadata_status_e{
    FBE_PAYLOAD_METADATA_STATUS_OK,
    FBE_PAYLOAD_METADATA_STATUS_FAILURE,  /* user input wrong*/
    FBE_PAYLOAD_METADATA_STATUS_BUSY,     /* service not ready */
    FBE_PAYLOAD_METADATA_STATUS_TIMEOUT,  /*! The I/O expired. */
    FBE_PAYLOAD_METADATA_STATUS_ABORTED,
    FBE_PAYLOAD_METADATA_STATUS_CANCELLED,

    FBE_PAYLOAD_METADATA_STATUS_IO_RETRYABLE,  /* used in RG and pvd can't get lock, edge not ready, */
    FBE_PAYLOAD_METADATA_STATUS_IO_NOT_RETRYABLE, /* used when quiescing I/O and for FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE I/O qualifier */
    FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE,/* Like hard media, crc, lba stamp errors, metadata is invalidated and will be reconstructed by the client*/
    FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE, /* Correctable/recoverable errors - Like soft media errors.
                                                 * It is possible for the read to succeed but for an error to get returned.
                                                 * This corresponds to block status of FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS
                                                 * with a qualifier of FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED.
                                                 * These blocks can be read but need to be remapped by writing them with a write-verify opcode. */

    FBE_PAYLOAD_METADATA_STATUS_NO_MORE_ENTRIES,/* used by write_log and journal only*/
    FBE_PAYLOAD_METADATA_STATUS_ENTRY_NOT_VALID,/* used by write_log and journal only*/
    FBE_PAYLOAD_METADATA_STATUS_ENCRYPTION_NOT_ENABLED, /*! Miniport found encryption is not enabled. */
    FBE_PAYLOAD_METADATA_STATUS_BAD_KEY_HANDLE, /*! Key handle incorrect or bogus. */
    FBE_PAYLOAD_METADATA_STATUS_KEY_WRAP_ERROR, /*! Key unwrap failed. */

    FBE_PAYLOAD_METADATA_STATUS_PAGED_CACHE_HIT, /* Paged cache hit, read is not performed */

    FBE_PAYLOAD_METADATA_STATUS_LAST
}fbe_payload_metadata_status_t;

typedef enum fbe_payload_metadata_operation_opcode_e{
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_INVALID,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_INIT_MEMORY,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_REINIT_MEMORY,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INIT, /* Ussued only once on init records condition */
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_WRITE,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_BITS,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_CLEAR_BITS,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_CHECKPOINT, /* Called in I/O context to move checkpoint backward */
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_FORCE_SET_CHECKPOINT, /* Move checkpoint with no checks. */
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INCR_CHECKPOINT, /* Used in monitor context ONLY to advance checkpoint */
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INCR_CHECKPOINT_NO_PEER, /* advance checkpoint on active side only */
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PERSIST, /* object may decide to persist it's nonpaged metadata */
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_READ_PERSIST, /* object may decide to read its persisted nonpaged metadata */
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PRESET,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_WRITE_VERIFY,/*object may decide to write/verify its non paged metadata
													                    this is used in metadata raw mirror read/write/rebuild*/
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_POST_PERSIST, /* Actions to be taken after the persist completes */
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE, /* Used only once to set up default values*/
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY, /* Used when client reconstruct the paged */
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE, /* Used for reconstruction in case of RAID glitch scenarion */
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_CLEAR_BITS,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_XOR_BITS,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_BITS,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_NEXT_MARKED_BITS,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_READ,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_UNREGISTER_ELEMENT,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_MEMORY_UPDATE,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_SET_DEFAULT_NONPAGED,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY_UPDATE,
    FBE_PAYLOAD_METADATA_OPERATION_OPCODE_LAST


}fbe_payload_metadata_operation_opcode_t;

enum fbe_payload_metadata_operation_flags_e{
    FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NONE			= 0x00000000,
    FBE_PAYLOAD_METADATA_OPERATION_FLAGS_PERSIST		= 0x00000001, /* Indicates the operation specified is a persist kind of Operation */
    FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NO_PERSIST		= 0x00000002, /* Indicates the operation specified is a non persist kind of Operation */
    FBE_PAYLOAD_METADATA_OPERATION_FLAGS_FUA_READ       = 0x00000004, /* Indicates that we must clear cache for the block before reading from disk*/
    FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NEED_VERIFY	= 0x00001000, /* Internal: indicates the need to verify */
    FBE_PAYLOAD_METADATA_OPERATION_FLAGS_VALID_DATA		= 0x00010000, /* Internal: that record_data is valid */
    FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE			= 0x00040000, /* Internal: this is a write operation in update */
    FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY	= 0x00080000, /* Internal: this is a write verify operation in update */
	FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_MASTER  = 0x00100000, /* Internal: Marks the first operation of the bundle */
    FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_START	= 0x00200000, /* Internal: indicates the the bundle write in progress */
    FBE_PAYLOAD_METADATA_OPERATION_PAGED_BUNDLE_DONE	= 0x00400000, /* Internal: indicates the the bundle write completed successfully */
    FBE_PAYLOAD_METADATA_OPERATION_FLAGS_REPEAT_TO_END  = 0x00800000, /* Apply the operation to the end of the blob */
    FBE_PAYLOAD_METADATA_OPERATION_PAGED_USE_BGZ_CACHE  = 0x01000000, /* PVD only: use BGZ cache for this IO */
    FBE_PAYLOAD_METADATA_OPERATION_FLAGS_SEQ_NUM        = 0x10000000, /* Indicates that the cmi message has a valid seq number associated with it */
};

typedef fbe_u32_t fbe_payload_metadata_operation_flags_t;


enum fbe_payload_metadata_constants_e {
    FBE_PAYLOAD_METADATA_MAX_DATA_SIZE = 192,
};

typedef void*   context_t;
typedef fbe_bool_t (*metadata_search_fn_t)(fbe_u8_t* search_data, fbe_u32_t search_size, context_t context);

typedef struct fbe_payload_metadata_operation_metadata_s{
    fbe_u64_t           offset; 
    fbe_u8_t            record_data[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE]; 
    fbe_u32_t           record_data_size; /* Should be less than FBE_PAYLOAD_METADATA_MAX_DATA_SIZE */
    fbe_u32_t           current_count_private; /* Keep state between I/O's */
    fbe_u64_t           repeat_count;       
    void    *           client_blob;  

    fbe_lba_t           stripe_offset; /* We will get a lock for that region */
    fbe_block_count_t   stripe_count; /* 0 - means we will not get a lock */

    fbe_payload_metadata_operation_flags_t    operation_flags;
    fbe_u64_t           second_offset; 
}fbe_payload_metadata_operation_metadata_t;

typedef struct fbe_payload_metadata_operation_get_next_marked_chunk_s{
    
    fbe_payload_metadata_operation_metadata_t metadata;
    
    fbe_u64_t                          current_offset; /* This is the offset that needs to be acted upon */
    metadata_search_fn_t               search_fn;  /* function to check whether the specified bit is set */
    //fbe_u8_t                          * search_data_ptr;//[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE]; /* It should be same as record data in metadata */
    fbe_u32_t                          search_data_size;
    context_t                          context;

}fbe_payload_metadata_operation_get_next_marked_chunk_t;

typedef void * fbe_metadata_callback_context_t;
typedef fbe_status_t (* fbe_metadata_callback_function_t) (struct fbe_packet_s * packet, fbe_metadata_callback_context_t context);
typedef struct fbe_payload_metadata_operation_metadata_callback_s{
    fbe_u64_t           offset; 
    fbe_u8_t            record_data[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE]; 
    fbe_u32_t           record_data_size; /* Should be less than FBE_PAYLOAD_METADATA_MAX_DATA_SIZE */
    fbe_u32_t           current_count_private; /* Keep state between I/O's */
    fbe_u64_t           repeat_count;       
    void    *           client_blob;  

    fbe_lba_t           stripe_offset; /* We will get a lock for that region */
    fbe_block_count_t   stripe_count; /* 0 - means we will not get a lock */

    fbe_payload_metadata_operation_flags_t    operation_flags;
    fbe_u64_t           second_offset; 

    fbe_u64_t           current_offset; /* This is the offset that needs to be acted upon */
    fbe_sg_element_t *  sg_list;
    fbe_lba_t           start_lba;
    fbe_metadata_callback_function_t callback_function;
    fbe_metadata_callback_context_t  callback_context;
}fbe_payload_metadata_operation_metadata_callback_t;

typedef struct fbe_payload_metadata_operation_s{
    fbe_payload_operation_header_t          operation_header; /* Must be first */

    fbe_payload_metadata_operation_opcode_t opcode;     

    fbe_payload_metadata_status_t           metadata_status;
    FBE_ALIGN(8)struct fbe_metadata_element_s   * metadata_element;

    union{
        fbe_payload_metadata_operation_metadata_t metadata;
		fbe_payload_metadata_operation_get_next_marked_chunk_t  next_marked_chunk;
        fbe_payload_metadata_operation_metadata_callback_t metadata_callback;
    }u;
	void * paged_blob; /* Private blob */

}fbe_payload_metadata_operation_t;

/* Metadata operation functions */
fbe_status_t fbe_payload_metadata_build_init_memory(fbe_payload_metadata_operation_t * metadata_operation,
                                                        struct fbe_metadata_element_s   * metadata_element);

fbe_status_t fbe_payload_metadata_build_reinit_memory(fbe_payload_metadata_operation_t * metadata_operation,
                                                        struct fbe_metadata_element_s   * metadata_element);

fbe_status_t fbe_payload_metadata_build_nonpaged_write(fbe_payload_metadata_operation_t * metadata_operation,
                                                       struct fbe_metadata_element_s   * metadata_element,
                                                       fbe_payload_metadata_operation_flags_t flags);

fbe_status_t fbe_payload_metadata_build_nonpaged_read_persist(fbe_payload_metadata_operation_t * metadata_operation,
                                                       struct fbe_metadata_element_s   * metadata_element,
                                                       fbe_payload_metadata_operation_flags_t flags);

fbe_status_t fbe_payload_metadata_build_nonpaged_persist(fbe_payload_metadata_operation_t * metadata_operation,
                                                        struct fbe_metadata_element_s   * metadata_element);
fbe_status_t fbe_payload_metadata_build_nonpaged_post_persist(fbe_payload_metadata_operation_t * metadata_operation, 
                                                              struct fbe_metadata_element_s	* metadata_element);

fbe_status_t fbe_payload_metadata_build_nonpaged_preset(fbe_payload_metadata_operation_t * metadata_operation,
                                                        fbe_u8_t *   metadata_record_data, 
                                                        fbe_u32_t    metadata_record_data_size,
                                                        fbe_object_id_t object_id);

fbe_status_t fbe_payload_metadata_build_nonpaged_set_bits(fbe_payload_metadata_operation_t * metadata_operation, 
                                                        struct fbe_metadata_element_s   * metadata_element, 
                                                        fbe_u64_t   metadata_offset, 
                                                        fbe_u8_t *  metadata_record_data, 
                                                        fbe_u32_t   metadata_record_data_size,
                                                        fbe_u64_t   metadata_repeat_count,
                                                        fbe_payload_metadata_operation_flags_t flags);

fbe_status_t fbe_payload_metadata_build_nonpaged_clear_bits(fbe_payload_metadata_operation_t * metadata_operation, 
                                                        struct fbe_metadata_element_s   * metadata_element, 
                                                        fbe_u64_t   metadata_offset, 
                                                        fbe_u8_t *  metadata_record_data, 
                                                        fbe_u32_t   metadata_record_data_size,
                                                        fbe_u64_t   metadata_repeat_count);

fbe_status_t fbe_payload_metadata_build_nonpaged_set_checkpoint(fbe_payload_metadata_operation_t * metadata_operation, 
                                                                struct fbe_metadata_element_s * metadata_element, 
                                                                fbe_u64_t     metadata_offset, 
                                                                fbe_u64_t     second_metadata_offset,
                                                                fbe_lba_t     checkpoint,
                                                                fbe_payload_metadata_operation_flags_t flags);

fbe_status_t fbe_payload_metadata_build_nonpaged_force_set_checkpoint(fbe_payload_metadata_operation_t * metadata_operation, 
                                                                      struct fbe_metadata_element_s * metadata_element, 
                                                                      fbe_u64_t 	metadata_offset, 
                                                                      fbe_u64_t 	second_metadata_offset,      
                                                                      fbe_lba_t 	checkpoint);

fbe_status_t fbe_payload_metadata_build_nonpaged_incr_checkpoint(fbe_payload_metadata_operation_t * metadata_operation, 
                                                                  struct fbe_metadata_element_s * metadata_element, 
                                                                  fbe_u64_t     metadata_offset,
                                                                  fbe_u64_t     second_metadata_offset,
                                                                  fbe_lba_t     checkpoint,
                                                                  fbe_u64_t     repeat_count);

fbe_status_t fbe_payload_metadata_build_np_inc_chkpt_no_peer(fbe_payload_metadata_operation_t * metadata_operation,
                                                             struct fbe_metadata_element_s * metadata_element,
                                                             fbe_u64_t 	metadata_offset,
                                                             fbe_u64_t 	second_metadata_offset,
                                                             fbe_lba_t 	checkpoint,
                                                             fbe_u64_t	repeat_count);
fbe_status_t fbe_payload_metadata_reuse_metadata_operation(fbe_payload_metadata_operation_t * metadata_operation, 
                                                           fbe_u64_t 	metadata_offset, 
                                                           fbe_u64_t	    metadata_repeat_count);
fbe_status_t fbe_payload_metadata_build_paged_set_bits(fbe_payload_metadata_operation_t * metadata_operation, 
                                                        struct fbe_metadata_element_s   * metadata_element, 
                                                        fbe_u64_t   metadata_offset, 
                                                        fbe_u8_t *  metadata_record_data, 
                                                        fbe_u32_t   metadata_record_data_size,
                                                        fbe_u64_t   metadata_repeat_count);

fbe_status_t fbe_payload_metadata_build_paged_clear_bits(fbe_payload_metadata_operation_t * metadata_operation, 
                                                        struct fbe_metadata_element_s   * metadata_element, 
                                                        fbe_u64_t   metadata_offset, 
                                                        fbe_u8_t *  metadata_record_data, 
                                                        fbe_u32_t   metadata_record_data_size,
                                                        fbe_u64_t   metadata_repeat_count);

fbe_status_t fbe_payload_metadata_build_paged_write(fbe_payload_metadata_operation_t * metadata_operation, 
                                                        struct fbe_metadata_element_s   * metadata_element, 
                                                        fbe_u64_t   metadata_offset, 
                                                        fbe_u8_t *  metadata_record_data, 
                                                        fbe_u32_t   metadata_record_data_size,
                                                        fbe_u64_t   metadata_repeat_count);

fbe_status_t fbe_payload_metadata_build_paged_write_verify(fbe_payload_metadata_operation_t * metadata_operation, 
                                                           struct fbe_metadata_element_s    * metadata_element, 
                                                           fbe_u64_t    metadata_offset, 
                                                           fbe_u8_t *   metadata_record_data, 
                                                           fbe_u32_t    metadata_record_data_size,
                                                           fbe_u64_t    metadata_repeat_count);

fbe_status_t fbe_payload_metadata_build_paged_verify_write(fbe_payload_metadata_operation_t * metadata_operation, 
                                                           struct fbe_metadata_element_s    * metadata_element, 
                                                           fbe_u64_t    metadata_offset, 
                                                           fbe_u8_t *   metadata_record_data, 
                                                           fbe_u32_t    metadata_record_data_size,
                                                           fbe_u64_t    metadata_repeat_count);

fbe_status_t fbe_payload_metadata_build_paged_xor_bits(fbe_payload_metadata_operation_t * metadata_operation, 
                                                       struct fbe_metadata_element_s    * metadata_element, 
                                                       fbe_u64_t   metadata_offset, 
                                                       fbe_u8_t *  metadata_record_data, 
                                                       fbe_u32_t   metadata_record_data_size,
                                                       fbe_u64_t   metadata_repeat_count);

fbe_status_t  fbe_payload_metadata_build_paged_get_bits(fbe_payload_metadata_operation_t * metadata_operation, 
                                                        struct fbe_metadata_element_s   * metadata_element, 
                                                        fbe_u64_t       metadata_offset, 
                                                        fbe_u8_t *  metadata_record_data, 
                                                        fbe_u32_t metadata_record_data_size,
                                                        fbe_u32_t paged_data_size);

fbe_status_t  fbe_payload_metadata_build_paged_get_next_marked_bits(fbe_payload_metadata_operation_t * metadata_operation, 
                                                               struct fbe_metadata_element_s    * metadata_element, 
                                                               fbe_u64_t        metadata_offset, 
                                                               fbe_u8_t *   metadata_record_data, 
                                                               fbe_u32_t metadata_record_data_size,
                                                               fbe_u32_t paged_data_size,
                                                               metadata_search_fn_t search_fn,
                                                               fbe_u32_t  search_size,
                                                               context_t  context);

fbe_status_t fbe_payload_metadata_build_paged_read(fbe_payload_metadata_operation_t * metadata_operation, 
                                                   struct fbe_metadata_element_s    * metadata_element, 
                                                   fbe_u64_t     metadata_offset, 
                                                   //fbe_u8_t *    metadata_record_data, 
                                                   fbe_u32_t     metadata_record_data_size,
                                                   fbe_u64_t     metadata_repeat_count,
                                                   fbe_metadata_callback_function_t callback_func,
                                                   fbe_metadata_callback_context_t context);

fbe_status_t fbe_payload_metadata_build_paged_update(fbe_payload_metadata_operation_t * metadata_operation, 
                                                     struct fbe_metadata_element_s    * metadata_element, 
                                                     fbe_u64_t     metadata_offset, 
                                                     //fbe_u8_t *    metadata_record_data, 
                                                     fbe_u32_t     metadata_record_data_size,
                                                     fbe_u64_t     metadata_repeat_count,
                                                     fbe_metadata_callback_function_t callback_func,
                                                     fbe_metadata_callback_context_t context);
fbe_status_t fbe_payload_metadata_set_metadata_operation_flags(fbe_payload_metadata_operation_t * metadata_operation, 
                                                               fbe_payload_metadata_operation_flags_t flags);

fbe_status_t fbe_payload_metadata_build_paged_write_verify_update(fbe_payload_metadata_operation_t * metadata_operation,
                                                                  struct fbe_metadata_element_s    * metadata_element,
                                                                  fbe_u64_t     metadata_offset, 
                                                                  fbe_u32_t     metadata_record_data_size,
                                                                  fbe_u64_t     metadata_repeat_count,
                                                                  fbe_metadata_callback_function_t callback_func,
                                                                  fbe_metadata_callback_context_t context);

fbe_status_t 
fbe_payload_metadata_build_nonpaged_init(fbe_payload_metadata_operation_t * metadata_operation, 
                                          struct fbe_metadata_element_s * metadata_element);

fbe_status_t fbe_payload_metadata_get_status(fbe_payload_metadata_operation_t * metadata_operation, fbe_payload_metadata_status_t * metadata_status);
fbe_status_t fbe_payload_metadata_set_status(fbe_payload_metadata_operation_t * metadata_operation, fbe_payload_metadata_status_t metadata_status);

fbe_status_t fbe_payload_metadata_get_opcode(fbe_payload_metadata_operation_t * metadata_operation, fbe_payload_metadata_operation_opcode_t * opcode);
fbe_status_t fbe_payload_metadata_get_search_function(fbe_payload_metadata_operation_t * metadata_operation, metadata_search_fn_t *search_fn);

fbe_status_t fbe_payload_metadata_build_unregister_element(fbe_payload_metadata_operation_t * metadata_operation, 
                                                           struct fbe_metadata_element_s    * metadata_element);

fbe_status_t fbe_payload_metadata_get_metadata_offset(fbe_payload_metadata_operation_t * metadata_operation, fbe_u64_t * metadata_offset);
fbe_status_t fbe_payload_metadata_set_metadata_offset(fbe_payload_metadata_operation_t * metadata_operation, fbe_u64_t metadata_offset);

fbe_status_t fbe_payload_metadata_set_metadata_repeat_count(fbe_payload_metadata_operation_t * metadata_operation, fbe_u64_t metadata_repeat_count);
fbe_status_t fbe_payload_metadata_get_metadata_repeat_count(fbe_payload_metadata_operation_t * metadata_operation, fbe_u64_t * metadata_repeat_count);

fbe_status_t fbe_payload_metadata_get_metadata_record_data_ptr(fbe_payload_metadata_operation_t * metadata_operation, void ** metadata_record_data_p);

fbe_status_t fbe_payload_metadata_get_metadata_stripe_offset(fbe_payload_metadata_operation_t * metadata_operation, fbe_lba_t * stripe_offset);
fbe_status_t fbe_payload_metadata_get_metadata_stripe_count(fbe_payload_metadata_operation_t * metadata_operation, fbe_block_count_t * stripe_count);
fbe_status_t fbe_payload_metadata_set_metadata_stripe_offset(fbe_payload_metadata_operation_t * metadata_operation, fbe_lba_t stripe_offset);
fbe_status_t fbe_payload_metadata_set_metadata_stripe_count(fbe_payload_metadata_operation_t * metadata_operation, fbe_block_count_t stripe_count);

fbe_status_t fbe_payload_metadata_build_memory_update(fbe_payload_metadata_operation_t * metadata_operation, 
                                                           struct fbe_metadata_element_s    * metadata_element);
fbe_status_t fbe_payload_metadata_build_nonpaged_write_verify(fbe_payload_metadata_operation_t * metadata_operation, 
															   struct fbe_metadata_element_s	* metadata_element);



#endif /* FBE_PAYLOAD_METADATA_OPERATION_H */
