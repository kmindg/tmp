#ifndef FBE_PERSIST_TRANSACTION_H
#define FBE_PERSIST_TRANSACTION_H

#include "fbe/fbe_persist_interface.h"

#define FBE_PERSIST_TRAN_HANDLE_INVALID ((fbe_persist_transaction_handle_t)0)

/* 520 byte blocks per transaction buffer. */
#define FBE_PERSIST_BLOCKS_PER_TRAN_BUF (FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY * 2)

/* Transaction buffer size in bytes. */
#define FBE_PERSIST_TRAN_BUF_SIZE (FBE_PERSIST_BLOCKS_PER_TRAN_BUF * FBE_PERSIST_BYTES_PER_BLOCK)

/* Transaction entry size in bytes. */
#define FBE_PERSIST_TRAN_ENTRY_SIZE (FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY * FBE_PERSIST_BYTES_PER_BLOCK)

/* Transaction elements per transaction buffer. */
#define FBE_PERSIST_TRAN_ENTRY_PER_TRAN_BUF (FBE_PERSIST_BLOCKS_PER_TRAN_BUF / FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY)

/* FBE_PERSIST_TRAN_ENTRY_MAX entries in each transaction, so the const below is the size of the journal. */
#define FBE_PERSIST_BLOCKS_PER_TRAN (FBE_PERSIST_TRAN_ENTRY_MAX * FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY)

/* The number of transaction buffers. */
#define FBE_PERSIST_TRAN_BUF_MAX (FBE_PERSIST_BLOCKS_PER_TRAN / FBE_PERSIST_BLOCKS_PER_TRAN_BUF)

/* SGL entries needed for the transaction journal */
#define FBE_PERSIST_SGL_ELEM_PER_TRAN_JOURNAL (FBE_PERSIST_TRAN_BUF_MAX + 1)

/* SGL entries needed for the transaction commit */
#define FBE_PERSIST_SGL_ELEM_PER_TRAN_COMMIT (FBE_PERSIST_TRAN_ENTRY_MAX)

/*how many transactions we keep in the journal area, today we use only the first one with 3 spare one for future use*/
#define FBE_PERSIST_TRANSACTION_COUNT_IN_JOURNAL_AREA 4 /*If there is any change to this variable,
										                you also need to change object_entry_start_lba calculation in
                                                        the sailor_moon_test which uses this number.*/

/* This is a constant for journal area. Don't change it unless you want to change database layout */
#define FBE_JOURNAL_AREA_SIZE        (112 * 5 * 4)

/* blocks at start of lun before the journal*/
#define FBE_PERSIST_BLOCKS_PER_HEADER 1


/* These are the states of a transaction. */
typedef enum {
    FBE_PERSIST_TRAN_STATE_INVALID = 0,
    FBE_PERSIST_TRAN_STATE_PENDING,
    FBE_PERSIST_TRAN_STATE_READ_HEADER,
    FBE_PERSIST_TRAN_STATE_WRITE_HEADER,
    FBE_PERSIST_TRAN_STATE_JOURNAL,
    FBE_PERSIST_TRAN_STATE_INVALIDATE_HEADER,
    FBE_PERSIST_TRAN_STATE_COMMIT,
	FBE_PERSIST_TRAN_STATE_INVALIDATE_JOURNAL,
	FBE_PERSIST_TRAN_STATE_REPLAY_JOURNAL,
    FBE_PERSIST_TRAN_STATE_COMPLETE,
	FBE_PERSIST_TRAN_STATE_REBUILD_BITMAP,
    FBE_PERSIST_TRAN_STATE_ABORT,
} fbe_persist_tran_state_t;

/* These are the operations that can be performed on an element in a transaction. */
typedef enum {
    FBE_PERSIST_TRAN_OP_INVALID = 0,
    FBE_PERSIST_TRAN_OP_WRITE_ENTRY,
	FBE_PERSIST_TRAN_OP_MODIFY_ENTRY,/*just like write, but for existing records*/
	FBE_PERSIST_TRAN_OP_DELETE_ENTRY,
    FBE_PERSIST_TRAN_OP_MAX
} fbe_persist_tran_op_t;

/* This is the internals of the header of an element in a transaction.
(seperated from the header itself so we can use sizeof safely in fbe_persist_tran_elem_header_t and not count length of members*/
typedef struct fbe_persist_header_internal_s{
	fbe_u64_t tran_elem_op;
    fbe_persist_entry_id_t entry_id;
    fbe_u64_t data_length;
	fbe_bool_t	valid;
}fbe_persist_header_internal_t;

/* This is the header of an element in a transaction. */
typedef struct fbe_persist_tran_elem_header_s {
	fbe_persist_header_internal_t header;
    fbe_u8_t  unused[FBE_PERSIST_BYTES_PER_BLOCK - (sizeof(fbe_persist_header_internal_t))];
} fbe_persist_tran_elem_header_t;

/* This an element in a transaction. */
typedef struct fbe_persist_tran_elem_s {
    fbe_persist_tran_elem_header_t tran_elem_header;
    fbe_u8_t tran_elem_data1[FBE_PERSIST_BYTES_PER_ENTRY];
} fbe_persist_tran_elem_t;

/* This is the header of a transaction. */
typedef struct fbe_persist_tran_header_s {
    fbe_persist_transaction_handle_t tran_handle;
    fbe_u32_t tran_state;
    fbe_u32_t tran_elem_count;
    fbe_u32_t tran_elem_committed_count;
} fbe_persist_tran_header_t;

/* These are the prototypes for invoking persist transaction operations. */
fbe_status_t fbe_persist_transaction_init(void);
void fbe_persist_transaction_destroy(void);

fbe_status_t fbe_persist_transaction_write_entry(fbe_persist_transaction_handle_t tran_handle,
												 fbe_persist_sector_type_t target_sector,
                                                 fbe_persist_entry_id_t * entry_id,
                                                 fbe_u8_t * buffer,
                                                 fbe_u32_t byte_count,
												 fbe_bool_t auto_insert_entry_id);

fbe_status_t fbe_persist_transaction_modify_entry(fbe_persist_transaction_handle_t tran_handle,
                                                 fbe_persist_entry_id_t entry_id,
                                                 fbe_u8_t * buffer,
                                                 fbe_u32_t byte_count);

fbe_status_t fbe_persist_transaction_start(fbe_packet_t * packet,
                                           fbe_persist_transaction_handle_t * tran_handle);
fbe_status_t fbe_persist_transaction_abort(fbe_persist_transaction_handle_t tran_handle);
fbe_status_t fbe_persist_transaction_commit(fbe_packet_t * packet, fbe_persist_transaction_handle_t tran_handle);

fbe_status_t fbe_persist_transaction_delete_entry(fbe_persist_transaction_handle_t tran_handle,
												  fbe_persist_entry_id_t entry_id);

fbe_status_t fbe_persist_transaction_is_entry_deleted(fbe_persist_entry_id_t entry_id, fbe_bool_t *deleted);
fbe_status_t fbe_persist_transaction_read_sectors_and_update_bitmap(fbe_packet_t *packet);
fbe_status_t fbe_persist_transaction_reset(void);
fbe_status_t fbe_persist_read_persist_db_header_and_replay_journal(fbe_packet_t *packet);
fbe_status_t persist_set_database_lun_completion(fbe_packet_t * request_packet, fbe_packet_completion_context_t notused);
fbe_status_t fbe_persist_transaction_validate_entry(fbe_persist_transaction_handle_t callers_tran_handle,
                                                    fbe_persist_entry_id_t entry_id);


#endif /* FBE_PERSIST_TRANSACTION_H */
