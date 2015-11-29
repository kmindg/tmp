/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#ifndef TERMINATOR_SIMULATED_DISK_REMOTE_FILE_SIMPLE_H
#define TERMINATOR_SIMULATED_DISK_REMOTE_FILE_SIMPLE_H

/* AR484231: TERMINATOR_TCP_CONNECT_TIMEOUT_MS = 5000 (5s) is based on test, usually it takes 3s
 *  for SP to reboot and Terminator to reconnect to disk server with the same client port.
 */
#define TERMINATOR_TCP_CONNECT_TIMEOUT_MS       5000

#define TERMINATOR_PORT_IO_TIMEOUT_SEC          3

/* The max interval of two successive heartbeat messages is (TERMINATOR_PORT_IO_TIMEOUT_SEC * 2).
 *  Refer to the comments in port_io_thread_func() in fbe_terminator_port.c.
 */
#define TERMINATOR_HEARTBEAT_MAX_INTERVAL_SEC   (TERMINATOR_PORT_IO_TIMEOUT_SEC * 2)

#define FBE_SUCCESS(fbe_status) ((fbe_status) == FBE_STATUS_OK)

fbe_status_t send_heartbeat_message(void);

typedef enum remote_file_request_type_e
{
    CREATE_FILE_REQ,
    READ_FILE_REQ,
    WRITE_FILE_REQ,
    WRITE_FILE_ZERO_PATTERN_REQ,
    HEART_BEAT_REQ,
} remote_file_request_type_t;

#pragma pack(push, TERM_RMT_FILE_SIMPLE, 1)

typedef struct remote_file_simple_request_s
{
    remote_file_request_type_t op;
    fbe_u32_t                  array_id;
    fbe_u32_t                  port;
    fbe_u32_t                  enclosure;
    fbe_u32_t                  slot;
    fbe_u64_t                  lba;
    fbe_u32_t                  block_size;
    fbe_u64_t                  block_count;
    fbe_u64_t                  checksum;
} remote_file_simple_request_t;

typedef struct remote_file_simple_response_s
{
    fbe_status_t status;
    fbe_u32_t    data_len;
    fbe_u64_t    checksum;
} remote_file_simple_response_t;

#pragma pack(pop, TERM_RMT_FILE_SIMPLE)

__forceinline static void init_remote_file_simple_response(remote_file_simple_response_t *response)
{
    response->status   = FBE_STATUS_GENERIC_FAILURE;
    response->data_len = 0;
    response->checksum = 0;
}

__forceinline static fbe_u64_t get_checksum_for_eight_bytes_aligned_memory(const fbe_u8_t *data_buff, fbe_u32_t bytes)
{
    const fbe_u64_t *buff       = (const fbe_u64_t *)data_buff;
    fbe_u64_t        checksum   = 0;
    fbe_u32_t        calc_times = bytes / sizeof(fbe_u64_t);

    while (calc_times--)
    {
        checksum += *buff++;
    }

    return checksum;
}

#endif /* TERMINATOR_SIMULATED_DISK_REMOTE_FILE_SIMPLE_H */
