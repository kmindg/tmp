#ifndef __SIMULATED_DRIVE_PRIVATE_H__
#define __SIMULATED_DRIVE_PRIVATE_H__

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe_simulated_drive.h"
#include "fbe/fbe_encryption.h"

/* Winsock has a problem with transferring a large packet, should break into small pieces. */
#define SOCKET_EACH_TRANSFER_MAX_SIZE 1069056  /* a little over 1M. */

typedef enum simulated_drive_request_type_e{
    SIMULATED_DRIVE_REQUEST_INVALID         = 0,
    SIMULATED_DRIVE_REQUEST_INIT,
    SIMULATED_DRIVE_REQUEST_CLEANUP,
    SIMULATED_DRIVE_REQUEST_GET_DRIVE_LIST,
    SIMULATED_DRIVE_REQUEST_GET_SERVER_PID,
    SIMULATED_DRIVE_REQUEST_OPEN,
    SIMULATED_DRIVE_REQUEST_CLOSE,
    SIMULATED_DRIVE_REQUEST_CREATE,
    SIMULATED_DRIVE_REQUEST_REMOVE,
    SIMULATED_DRIVE_REQUEST_REMOVE_ALL,
    SIMULATED_DRIVE_REQUEST_SET_TOTAL_CAPACITY,
    SIMULATED_DRIVE_REQUEST_SET_MODE,
    SIMULATED_DRIVE_REQUEST_SET_BACKEND_TYPE,
    SIMULATED_DRIVE_REQUEST_READ,
    SIMULATED_DRIVE_REQUEST_WRITE,
    SIMULATED_DRIVE_REQUEST_WRITE_SAME,
    SIMULATED_DRIVE_REQUEST_GET_FILE_SIZE,
    SIMULATED_DRIVE_REQUEST_EXIT
} simulated_drive_request_type_t;

#pragma pack(1)
typedef struct simulated_drive_request_s {
	fbe_queue_element_t queue_element;
    simulated_drive_request_type_t type;
    simulated_drive_identity_t identity;
    simulated_drive_handle_t handle;
    fbe_lba_t lba;
    fbe_u64_t size; /* transfer size in bytes */
	fbe_u32_t repeat_count; /* Used in write same */
    fbe_s64_t return_val;
    fbe_lba_t capacity;
    fbe_u64_t return_size; /* transfer size in bytes */
	fbe_u32_t side; /* 0 - SPA; 1 - SPB */
	fbe_u32_t socket_id; /* server private field */
	void * context; /* This will be the pointer to the fbe_terminator_io_t structure */
	void * memory_ptr; 
	fbe_u32_t block_size;
	fbe_u32_t collision_found;
    fbe_bool_t b_key_valid;
    fbe_u8_t  keys[FBE_ENCRYPTION_KEY_SIZE];
} simulated_drive_request_t;
#pragma pack()

#endif /* __SIMULATED_DRIVE_PRIVATE_H__ */
