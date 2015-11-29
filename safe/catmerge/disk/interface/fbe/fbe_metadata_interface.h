#ifndef FBE_METADATA_INTERFACE_H
#define FBE_METADATA_INTERFACE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
//#include "fbe/fbe_payload_stripe_lock_operation.h"

typedef enum fbe_metadata_control_code_e {
	FBE_METADATA_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_METADATA),
	FBE_METADATA_CONTROL_CODE_ELEMENT_ZERO_RECORDS,
	FBE_METADATA_CONTROL_CODE_SET_ELEMENTS_STATE,

	FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_CLEAR,
    FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_LOAD,
	FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_PERSIST,
	FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_ZERO_AND_PERSIST,

	FBE_METADATA_CONTROL_CODE_NONPAGED_LOAD,
	FBE_METADATA_CONTROL_CODE_NONPAGED_PERSIST,

	FBE_METADATA_CONTROL_CODE_DEBUG_LOCK,
    FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_LOAD_WITH_DISKMASK,
    FBE_METADATA_CONTROL_CODE_GET_SYSTEM_NONPAGED_NEED_REBUILD_INFO,
    FBE_METADATA_CONTROL_CODE_SET_DEFAULT_NONPAGED_METADATA,
    FBE_METADATA_CONTROL_CODE_NONPAGED_BACKUP_RESTORE,
	FBE_METADATA_CONTROL_CODE_SET_NDU_IN_PROGRESS,
	FBE_METADATA_CONTROL_CODE_CLEAR_NDU_IN_PROGRESS,
	FBE_METADATA_CONTROL_CODE_GET_STRIPE_LOCK_INFO,

	FBE_METADATA_CONTROL_CODE_DEBUG_EXT_POOL_LOCK,

	FBE_METADATA_CONTROL_CODE_NONPAGED_SET_BLOCKS_PER_OBJECT,
    FBE_METADATA_CONTROL_CODE_NONPAGED_GET_NONPAGED_METADATA_PTR,

	FBE_METADATA_CONTROL_CODE_LAST
} fbe_metadata_control_code_t;

typedef enum fbe_metadata_element_state_e{
	FBE_METADATA_ELEMENT_STATE_INVALID,
	FBE_METADATA_ELEMENT_STATE_ACTIVE,
	FBE_METADATA_ELEMENT_STATE_PASSIVE
}fbe_metadata_element_state_t;

typedef struct fbe_metadata_info_s{
    fbe_metadata_element_state_t metadata_element_state;
}fbe_metadata_info_t;

/*FBE_METADATA_CONTROL_CODE_SET_ELEMENTS_STATE*/
typedef struct fbe_metadata_set_elements_state_s{
	fbe_object_id_t					object_id;
	fbe_metadata_element_state_t	new_state;
}fbe_metadata_set_elements_state_t;

/* FBE_METADATA_CONTROL_CODE_DEBUG_LOCK */
typedef struct fbe_metadata_control_debug_lock_s{
	fbe_object_id_t			object_id;

	fbe_u32_t				opcode;		
	fbe_u32_t				status;

	fbe_u64_t				stripe_count;
	fbe_u64_t				stripe_number;	
	struct fbe_packet_s		* packet_ptr; /* Pointer to packet allocated for the lock operation */
}fbe_metadata_control_debug_lock_t;

#define FBE_METADATA_NONPAGED_MAX_SIZE 208 /* 8 check points. Note: Most likely we will increase it later */
                                              
typedef struct fbe_metadata_nonpaged_data_s{
    fbe_u8_t data[FBE_METADATA_NONPAGED_MAX_SIZE];
}fbe_metadata_nonpaged_data_t;

#define FBE_METADATA_MEMORY_MAX_SIZE 200 /* For raid group it is 72 bytes */

/*FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_LOAD_WITH_DISKMASK*/
typedef struct fbe_metadata_nonpaged_system_load_diskmask_s{
	fbe_u16_t disk_bitmask;
	fbe_object_id_t start_object_id;
	fbe_u32_t object_count;
}fbe_metadata_nonpaged_system_load_diskmask_t;

/*FBE_METADATA_CONTROL_CODE_GET_SYSTEM_NONPAGED_NEED_REBUILD_INFO*/
typedef struct fbe_metadata_nonpaged_get_need_rebuild_info_s{
	fbe_u32_t need_rebuild_count;
	fbe_object_id_t object_id;
}fbe_metadata_nonpaged_get_need_rebuild_info_t;

/*FBE_METADATA_CONTROL_CODE_SET_DEFAULT_NONPAGED_METADATA*/
typedef struct fbe_metadata_set_default_nonpaged_metadata_s{
	fbe_object_id_t					object_id;
}fbe_metadata_set_default_nonpaged_metadata_t;

typedef enum fbe_nonpaged_backup_restore_op_s{
    FBE_NONPAGED_METADATA_BACKUP_RESTORE_OP_INVALID,
    FBE_NONPAGED_METADATA_BACKUP_OBJECTS,
    FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_MEMORY,
    FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_DISK
} fbe_nonpaged_metadata_backup_restore_op_code_t;

#define FBE_MAX_METADATA_BACKUP_RESTORE_SIZE   16 * FBE_METADATA_NONPAGED_MAX_SIZE

/*data structure for backup or restore nonpaged metadata*/
typedef struct fbe_nonpaged_metadata_backup_restore_s{
    fbe_nonpaged_metadata_backup_restore_op_code_t opcode;
    fbe_object_id_t start_object_id;
    fbe_u32_t   object_number;
    fbe_u32_t   nonpaged_entry_size;
    fbe_u8_t    buffer[FBE_MAX_METADATA_BACKUP_RESTORE_SIZE];

    /*output result*/
    fbe_bool_t  operation_successful;
} fbe_nonpaged_metadata_backup_restore_t;

/* Data structure for persist nonpaged metadata by raw mirror write interface*/
typedef struct fbe_nonpaged_metadata_persist_system_object_context_s {
    fbe_object_id_t     start_object_id;
    fbe_block_count_t   block_count;
}fbe_nonpaged_metadata_persist_system_object_context_t;

#endif /*FBE_METADATA_INTERFACE_H*/
