#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_ex.h"

fbe_status_t 
fbe_payload_persistent_memory_build_read_and_pin(fbe_payload_persistent_memory_operation_t * persistent_memory_operation, 
                                                 fbe_lba_t lba,
                                                 fbe_block_count_t blocks,
                                                 fbe_lun_index_t index,
                                                 fbe_object_id_t object_id,
                                                 fbe_sg_element_t *sg_p,
                                                 fbe_u32_t max_sg_entries,
                                                 fbe_u8_t *chunk_p,
                                                 fbe_bool_t b_beyond_capacity)
{
	persistent_memory_operation->opcode = FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_OPCODE_READ_AND_PIN;

	persistent_memory_operation->lba = lba;
	persistent_memory_operation->blocks = blocks;
    persistent_memory_operation->object_index = index;
    persistent_memory_operation->object_id = object_id;
    persistent_memory_operation->sg_p = sg_p;
    persistent_memory_operation->chunk_p = chunk_p;
    persistent_memory_operation->max_sg_entries = max_sg_entries;
    persistent_memory_operation->b_beyond_capacity = b_beyond_capacity;
	return FBE_STATUS_OK;
}
fbe_status_t 
fbe_payload_persistent_memory_build_unpin(fbe_payload_persistent_memory_operation_t * persistent_memory_operation, 
                                          fbe_lba_t lba,
                                          fbe_block_count_t blocks,
                                          fbe_lun_index_t index,
                                          fbe_object_id_t object_id,
                                          void *opaque,
                                          fbe_payload_persistent_memory_unpin_mode_t mode,
                                          fbe_u8_t *chunk_p)
{
	persistent_memory_operation->opcode = FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_OPCODE_PERSIST_UNPIN;

	persistent_memory_operation->lba = lba;
	persistent_memory_operation->blocks = blocks;
    persistent_memory_operation->object_index = index;
    persistent_memory_operation->object_id = object_id;
    persistent_memory_operation->mode = mode;
    persistent_memory_operation->opaque = opaque;
    persistent_memory_operation->chunk_p = chunk_p;
	return FBE_STATUS_OK;
}
fbe_status_t 
fbe_payload_persistent_memory_build_check_vault(fbe_payload_persistent_memory_operation_t * persistent_memory_operation,
                                                fbe_u8_t *chunk_p)
{
	persistent_memory_operation->opcode = FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_OPCODE_CHECK_VAULT_BUSY;
    persistent_memory_operation->chunk_p = chunk_p;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_persistent_memory_get_opcode(fbe_payload_persistent_memory_operation_t * persistent_memory_operation, 
                                         fbe_payload_persistent_memory_operation_opcode_t * opcode)
{
	* opcode = 0;

	* opcode = persistent_memory_operation->opcode;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_persistent_memory_get_status(fbe_payload_persistent_memory_operation_t * persistent_memory_operation, 
                                         fbe_payload_persistent_memory_status_t * persistent_memory_status)
{
	*persistent_memory_status = persistent_memory_operation->persistent_memory_status;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_persistent_memory_set_status(fbe_payload_persistent_memory_operation_t * persistent_memory_operation, 
                                         fbe_payload_persistent_memory_status_t persistent_memory_status)
{
	persistent_memory_operation->persistent_memory_status = persistent_memory_status;
	return FBE_STATUS_OK;
}

