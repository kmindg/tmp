#ifndef FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_H
#define FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_H
 
#include "fbe/fbe_types.h"
#include "fbe/fbe_payload_operation_header.h"
#include "flare_ioctls.h"
typedef LUN_INDEX fbe_lun_index_t;

typedef enum fbe_payload_persistent_memory_status_e{
    FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_INVALID,
    FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_OK,

    /*! The pin was successful but we did not pin persistently. 
     */
    FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_PINNED_NOT_PERSISTENT,
    FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_FAILURE,  /* user input wrong*/
    FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_BUSY,     /* service not ready */
    FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_INSUFFICIENT_RESOURCES,

    FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_LAST
}fbe_payload_persistent_memory_status_t;

typedef enum fbe_payload_persistent_memory_operation_opcode_e{
    FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_OPCODE_INVALID,
    FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_OPCODE_READ_AND_PIN,
    FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_OPCODE_PERSIST_UNPIN,
    FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_OPCODE_CHECK_VAULT_BUSY,
    FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_OPCODE_LAST
}fbe_payload_persistent_memory_operation_opcode_t;

typedef enum fbe_payload_persistent_memory_unpin_mode_e{
    FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_INVALID,
    FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_DATA_WRITTEN,
    FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_FLUSH_REQUIRED,
    FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_VERIFY_REQUIRED,
    FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_LAST
}fbe_payload_persistent_memory_unpin_mode_t;

enum fbe_payload_persistent_memory_operation_flags_e{
    FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_FLAGS_NONE			= 0x00000000,
};

typedef fbe_u32_t fbe_payload_persistent_memory_operation_flags_t;


enum fbe_payload_persistent_memory_constants_e {
    FBE_PAYLOAD_PERSISTENT_MEMORY_MAX_SG_LENGTH_SIZE = 192,
};

typedef struct fbe_payload_persistent_memory_operation_s{
    fbe_payload_operation_header_t          operation_header; /* Must be first */

    fbe_payload_persistent_memory_operation_opcode_t opcode;     

    fbe_payload_persistent_memory_status_t persistent_memory_status;

    fbe_sg_element_t *sg_p; /*!< Allocated sg for read and pin. */
    fbe_u32_t max_sg_entries; /*!< Max length of our SGL. */

    fbe_u8_t *chunk_p; /*! Memory allocated for use by persistent memory svc. */
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_lun_index_t object_index; /*! Index of the LUN from the perspective of MCC. */
    fbe_object_id_t object_id;
    fbe_payload_persistent_memory_unpin_mode_t mode; /*! Way to unlock*/
    fbe_bool_t b_was_dirty; /*!< Indicates if SP Cache pages were dirty or not. */
    fbe_bool_t b_beyond_capacity; /*!< Indicates if we go beyond the expected capacity. */
    void *opaque; /*!< Opaque information from SP Cache. */
}fbe_payload_persistent_memory_operation_t;

/*!*******************************************************************
 * @struct fbe_persistent_memory_set_params_t
 *********************************************************************
 * @brief Used for the memory service's usurper:
 *        FBE_MEMORY_SERVICE_CONTROL_CODE_PERSIST_SET_PARAMS
 *
 *********************************************************************/
typedef struct fbe_persistent_memory_set_params_s
{
    /*! Force this status.  Ignore if invalid.
     */
    fbe_payload_persistent_memory_status_t force_status;

    /*! Force the pages to appear dirty on pin.
     */
    fbe_bool_t b_force_dirty;

    fbe_bool_t b_always_unlock; /* Tells us to always unpin from cache.*/

    fbe_bool_t b_vault_busy; /* Tells us to mock that cache is busy.*/
}
fbe_persistent_memory_set_params_t;
fbe_status_t fbe_api_persistent_memory_init_params(fbe_persistent_memory_set_params_t *params_p);
fbe_status_t 
fbe_payload_persistent_memory_get_opcode(fbe_payload_persistent_memory_operation_t * persistent_memory_operation, 
                                         fbe_payload_persistent_memory_operation_opcode_t * opcode);
fbe_status_t 
fbe_payload_persistent_memory_get_status(fbe_payload_persistent_memory_operation_t * persistent_memory_operation, 
                                         fbe_payload_persistent_memory_status_t * persistent_memory_status);
fbe_status_t 
fbe_payload_persistent_memory_set_status(fbe_payload_persistent_memory_operation_t * persistent_memory_operation, 
                                         fbe_payload_persistent_memory_status_t persistent_memory_status);
fbe_status_t 
fbe_payload_persistent_memory_build_read_and_pin(fbe_payload_persistent_memory_operation_t * persistent_memory_operation, 
                                                 fbe_lba_t lba,
                                                 fbe_block_count_t blocks,
                                                 fbe_lun_index_t index,
                                                 fbe_object_id_t object_id,
                                                 fbe_sg_element_t *sg_p,
                                                 fbe_u32_t max_sg_entries,
                                                 fbe_u8_t *chunk_p,
                                                 fbe_bool_t b_beyond_capacity);
fbe_status_t 
fbe_payload_persistent_memory_build_check_vault(fbe_payload_persistent_memory_operation_t * persistent_memory_operation,
                                                fbe_u8_t *chunk_p);

fbe_status_t 
fbe_payload_persistent_memory_build_unpin(fbe_payload_persistent_memory_operation_t * persistent_memory_operation, 
                                          fbe_lba_t lba,
                                          fbe_block_count_t blocks,
                                          fbe_lun_index_t index,
                                          fbe_object_id_t object_id,
                                          void *opaque,
                                          fbe_payload_persistent_memory_unpin_mode_t mode,
                                          fbe_u8_t *chunk_p);
#endif // FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_H
