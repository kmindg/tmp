#ifndef FBE_PERSIST_INTERFACE_H
#define FBE_PERSIST_INTERFACE_H

#include "fbe/fbe_types.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"

/* Transaction elements in a transaction: one header element + N operation elements. */
#if defined(SIMMODE_ENV)
#define FBE_PERSIST_TRAN_ENTRY_MAX 256
#else
#define FBE_PERSIST_TRAN_ENTRY_MAX 112
#endif
#define FBE_PERSIST_START_LBA 0x0 
#define FBE_PERSIST_DATA_BYTES_PER_BLOCK 512
#define FBE_PERSIST_HEADER_BYTES_PER_BLOCK 8
#define FBE_PERSIST_BYTES_PER_BLOCK (FBE_PERSIST_DATA_BYTES_PER_BLOCK + FBE_PERSIST_HEADER_BYTES_PER_BLOCK)
#define FBE_PERSIST_BLOCKS_PER_ENTRY 4
#define FBE_PERSIST_BYTES_PER_ENTRY (FBE_PERSIST_BYTES_PER_BLOCK * FBE_PERSIST_BLOCKS_PER_ENTRY)
#define FBE_PERSIST_DATA_BYTES_PER_ENTRY (FBE_PERSIST_BYTES_PER_ENTRY - (FBE_PERSIST_HEADER_BYTES_PER_BLOCK * FBE_PERSIST_BLOCKS_PER_ENTRY))
#define FBE_PERSIST_STATUS_LUN_ALREADY_SET 1
/* 5 blocks per transaction entry. 1 header adn 4 data */
#define FBE_PERSIST_BLOCKS_PER_TRAN_ENTRY (FBE_PERSIST_BLOCKS_PER_ENTRY + 1)

extern struct fbe_persist_service_s fbe_persist_service;

typedef fbe_u64_t fbe_persist_transaction_handle_t;
typedef fbe_u64_t fbe_persist_entry_id_t;

typedef void * fbe_persist_completion_context_t;
typedef fbe_status_t (* fbe_persist_completion_function_t) (fbe_status_t commit_status, fbe_persist_completion_context_t context);
typedef fbe_status_t (* fbe_persist_single_read_completion_function_t) (fbe_status_t commit_status, fbe_persist_completion_context_t context, fbe_persist_entry_id_t next_entry);
typedef fbe_status_t (* fbe_persist_read_multiple_entries_completion_function_t) (fbe_status_t read_status,
																				  fbe_persist_entry_id_t next_entry,
																				  fbe_u32_t entries_read,
																				  fbe_persist_completion_context_t context);
typedef fbe_status_t (* fbe_persist_single_operation_completion_function_t) (fbe_status_t read_status,
																			 fbe_persist_entry_id_t,
																			 fbe_persist_completion_context_t context);

typedef enum fbe_persist_control_code_e {
    FBE_PERSIST_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_PERSIST),
    FBE_PERSIST_CONTROL_CODE_SET_LUN,
    FBE_PERSIST_CONTROL_CODE_READ_ENTRY,
    FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY,/*for new entries*/
    FBE_PERSIST_CONTROL_CODE_TRANSACTION,
	FBE_PERSIST_CONTROL_CODE_MODIFY_ENTRY,/*for existing entries*/
	FBE_PERSIST_CONTROL_CODE_DELETE_ENTRY,
	FBE_PERSIST_CONTROL_CODE_READ_SECTOR,
	FBE_PERSIST_CONTROL_CODE_GET_LAYOUT_INFO,
	FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY_WITH_AUTO_ENTRY_ID,/*for new entries when first 64 bits are populated with entry ID by service*/
	FBE_PERSIST_CONTROL_CODE_WRITE_SINGLE_ENTRY,/*single operation, asynchronous and takses care of transactions internaly*/
	FBE_PERSIST_CONTROL_CODE_MODIFY_SINGLE_ENTRY,/*single operation, asynchronous and takses care of transactions internaly*/
	FBE_PERSIST_CONTROL_CODE_DELETE_SINGLE_ENTRY,/*single operation, asynchronous and takses care of transactions internaly*/
	FBE_PERSIST_CONTROL_CODE_GET_REQUEIRED_LUN_SIZE,
    FBE_PERSIST_CONTROL_CODE_UNSET_LUN,
    FBE_PERSIST_CONTROL_CODE_HOOK, /*set/unset hook in persist service*/
    FBE_PERSIST_CONTROL_CODE_GET_ENTRY_INFO,
    FBE_PERSIST_CONTROL_CODE_VALIDATE_ENTRY,
    FBE_PERSIST_CONTROL_CODE_LAST
} fbe_persist_control_code_t;

/* FBE_PERSIST_CONTROL_CODE_SET_LUN */
typedef struct fbe_persist_control_set_lun {
    fbe_object_id_t lun_object_id;
	fbe_persist_completion_context_t completion_context;
	fbe_persist_completion_function_t completion_function;
    FBE_TRI_STATE journal_replayed;
} fbe_persist_control_set_lun_t;

typedef enum fbe_persist_sector_type_e{
    FBE_PERSIST_SECTOR_TYPE_INVALID,
    FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS,
    FBE_PERSIST_SECTOR_TYPE_SEP_EDGES,
	FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION,
	FBE_PERSIST_SECTOR_TYPE_ESP_OBJECTS,
	FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA,
	FBE_PERSIST_SECTOR_TYPE_SCRATCH_PAD,
	FBE_PERSIST_SECTOR_TYPE_DIEH_RECORD,
	FBE_PERSIST_SECTOR_TYPE_LAST
} fbe_persist_sector_type_t;

/* FBE_PERSIST_CONTROL_CODE_READ_ENTRY */
typedef struct fbe_persist_control_read_entry_s {
    fbe_persist_entry_id_t entry_id;
    fbe_persist_sector_type_t persist_sector_type;
	fbe_persist_completion_context_t completion_context;/*in*/
	fbe_persist_single_read_completion_function_t completion_function;/*in*/
    fbe_persist_entry_id_t next_entry_id;/*out*/
} fbe_persist_control_read_entry_t;

/* FBE_PERSIST_CONTROL_CODE_WRITE_ENTRY */
typedef struct fbe_persist_control_write_entry_s {
    fbe_persist_transaction_handle_t tran_handle;
	fbe_persist_sector_type_t target_sector;/*which sector to write to*/
    fbe_persist_entry_id_t entry_id;/*out*/
} fbe_persist_control_write_entry_t;

/* FBE_PERSIST_CONTROL_CODE_MODIFY_ENTRY */
typedef struct fbe_persist_control_modify_entry_s {
    fbe_persist_transaction_handle_t tran_handle;
    fbe_persist_entry_id_t entry_id;/*in*/
} fbe_persist_control_modify_entry_t;

/* FBE_PERSIST_CONTROL_CODE_DELETE_ENTRY */
typedef struct fbe_persist_control_delete_entry_s {
    fbe_persist_transaction_handle_t tran_handle;
    fbe_persist_entry_id_t entry_id;/*in*/
} fbe_persist_control_delete_entry_t;

/* FBE_PERSIST_CONTROL_CODE_GET_ENTRY_INFO */
typedef struct fbe_persist_control_get_entry_info_s {
    fbe_persist_entry_id_t  entry_id;
    fbe_bool_t              b_does_entry_exist;
} fbe_persist_control_get_entry_info_t;

/* FBE_PERSIST_CONTROL_CODE_VALIDATE_ENTRY */
typedef struct fbe_persist_control_validate_entry_s {
    fbe_persist_transaction_handle_t tran_handle;
    fbe_persist_entry_id_t entry_id;/*in*/
} fbe_persist_control_validate_entry_t;

typedef enum {
    FBE_PERSIST_TRANSACTION_OP_START,
    FBE_PERSIST_TRANSACTION_OP_COMMIT,
    FBE_PERSIST_TRANSACTION_OP_ABORT,
} fbe_persist_transaction_op_type_t;

/* FBE_PERSIST_CONTROL_CODE_TRANSACTION */
typedef struct fbe_persist_control_transaction_s {
    fbe_persist_transaction_op_type_t tran_op_type;
    fbe_persist_transaction_handle_t tran_handle;
	fbe_persist_completion_context_t completion_context;
	fbe_persist_completion_function_t completion_function;
    fbe_package_id_t    caller_package;     /*used for persist service test and debug*/
} fbe_persist_control_transaction_t;

/*this header should be used by anyone who wish to use persistance as part of the memory it persist.
It is added to the 2K the user has per each peristed memory*/
typedef struct fbe_persist_user_header_s{
	fbe_persist_entry_id_t	entry_id;
}fbe_persist_user_header_t;

#define FBE_PERSIST_USER_DATA_BYTES_PER_ENTRY (FBE_PERSIST_DATA_BYTES_PER_ENTRY + sizeof(fbe_persist_user_header_t))

/*each buffer will have FBE_PERSIST_NUMBER_OF_SECTOR_READ_ENTRIES elements read at once when we do a read of a sector*/
/*we use 400 since it gives us a full utilization of a 1MB read in the miniport. The persistance service will read
400 * 5 * 520 bytes which is almost 1MB, the user will need a bit less since he has less headers and CRC*/
#define FBE_PERSIST_NUMBER_OF_SECTOR_READ_ENTRIES 400
#define FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE (FBE_PERSIST_USER_DATA_BYTES_PER_ENTRY * FBE_PERSIST_NUMBER_OF_SECTOR_READ_ENTRIES)
#define FBE_PERSIST_SECTOR_START_ENTRY_ID 0xFFFFFFFFFFFFFFFD
#define FBE_PERSIST_NO_MORE_ENTRIES_TO_READ 0xFFFFFFFFFFFFFFFE

/*FBE_PERSIST_CONTROL_CODE_READ_SECTOR*/
typedef struct fbe_persist_control_read_sector_s {
    fbe_persist_completion_context_t completion_context;/*in*/
	fbe_persist_read_multiple_entries_completion_function_t completion_function;/*in*/
	fbe_persist_entry_id_t start_entry_id;/*in*/
	fbe_persist_entry_id_t next_entry_id;/*out*/
	fbe_persist_sector_type_t sector_type;/*in*/
	fbe_u32_t entries_read;/*out*/
} fbe_persist_control_read_sector_t;


/*FBE_PERSIST_CONTROL_CODE_GET_LAYOUT_INFO*/
typedef struct fbe_persist_control_get_layout_info_s {
	fbe_object_id_t lun_object_id;
    fbe_lba_t	journal_start_lba;
	fbe_lba_t	sep_object_start_lba;
	fbe_lba_t	sep_edges_start_lba;
	fbe_lba_t	sep_admin_conversion_start_lba;
	fbe_lba_t	esp_objects_start_lba;
    fbe_lba_t	system_data_start_lba;
} fbe_persist_control_get_layout_info_t;

/* FBE_PERSIST_CONTROL_CODE_WRITE_SINGLE_ENTRY */
typedef struct fbe_persist_control_write_single_entry_s {
	fbe_persist_sector_type_t target_sector;/*which sector to write to*/
    fbe_persist_entry_id_t entry_id;/*out*/
	fbe_persist_single_operation_completion_function_t completion_function;
	fbe_persist_completion_context_t completion_context;
} fbe_persist_control_write_single_entry_t;

/* FBE_PERSIST_CONTROL_CODE_MODIFY_SINGLE_ENTRY */
typedef struct fbe_persist_control_modify_single_entry_s {
	fbe_persist_entry_id_t entry_id;/*in*/
	fbe_persist_single_operation_completion_function_t completion_function;
	fbe_persist_completion_context_t completion_context;
} fbe_persist_control_modify_single_entry_t;

/* FBE_PERSIST_CONTROL_CODE_DELETE_SINGLE_ENTRY */
typedef struct fbe_persist_control_delete_single_entry_s {
	fbe_persist_entry_id_t entry_id;/*in*/
	fbe_persist_single_operation_completion_function_t completion_function;
	fbe_persist_completion_context_t completion_context;
} fbe_persist_control_delete_single_entry_t;

/*FBE_PERSIST_CONTROL_CODE_GET_REQUEIRED_LUN_SIZE*/
typedef struct fbe_persist_control_get_required_lun_size_s {
	fbe_lba_t total_block_needed;/*out*/
} fbe_persist_control_get_required_lun_size_t;

/*all hooks that user can set in persist service*/
typedef enum fbe_persist_hook_type_e
{
    FBE_PERSIST_HOOK_TYPE_INVALID = 0,
    FBE_PERSIST_HOOK_TYPE_SHRINK_JOURNAL_AND_WAIT = 1,
    FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_MARK_JOURNAL_HEADER_VALID = 2,
    FBE_PERSIST_HOOK_TYPE_SHRINK_LIVE_TRANSACTION_AND_WAIT = 3,
    FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_INVALIDATE_JOURNAL_HEADER = 4,
    FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_JOURNAL_REGION = 5,
    FBE_PERSIST_HOOK_TYPE_PANIC_WHEN_WRITING_LIVE_DATA_REGION = 6,
    FBE_PERSIST_HOOK_TYPE_RETURN_FAILED_TRANSACTION = 7,
    FBE_PERSIST_HOOK_TYPE_LAST
}fbe_persist_hook_type_t;

/*all operations that user can perform to a specific hook*/
typedef enum fbe_persist_hook_opt_e
{
    FBE_PERSIST_HOOK_OPERATION_INVALID = 0,
    FBE_PERSIST_HOOK_OPERATION_GET_STATE = 1,
    FBE_PERSIST_HOOK_OPERATION_SET_HOOK = 2,
    FBE_PERSIST_HOOK_OPERATION_REMOVE_HOOK = 3,
    FBE_PERSIST_HOOK_OPERATION_LAST
}fbe_persist_hook_opt_t;

/*control structure that user can use to operate a hook*/
typedef struct fbe_persist_control_hook_s {
    fbe_persist_hook_type_t hook_type;
    fbe_persist_hook_opt_t  hook_op_code;
    fbe_bool_t  is_triggered; /*output for FBE_PERSIST_HOOK_OPERATION_GET_STATE*/
    fbe_bool_t  is_set_sep;
    fbe_bool_t  is_set_esp;
}fbe_persist_control_hook_t;

#endif /* FBE_PERSIST_INTERFACE_H */
