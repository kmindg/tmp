/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  terminator_simulated_disk_remote_file_simple.c
 ***************************************************************************
 *
 *  Description
 *      This is another remote file implementation. It's very similar to the local
 *      file mode except file operations are over TCP.
 *
 *
 *  History:
 *      05/23/11    Zhifeng    Created
 *      11/23/11    Allen      AR457943
 *      02/04/12    Tommy      AR537089
 ***************************************************************************/

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define I_AM_NATIVE_CODE
#include <windows.h>
#endif // UMODE_ENV || SIMMODE_ENV

#include "csx_ext.h"

#include "fbe_terminator_common.h"
#include "fbe_terminator_file_api.h"
#include "fbe_panic_sp.h"

#include "terminator_port.h"
#include "terminator_drive.h"
#include "terminator_simulated_disk_remote_file_simple.h"

static csx_module_context_t g_module_context;

#define MAX_ADDR_LEN 32

static char        g_address[MAX_ADDR_LEN];
static csx_nuint_t g_port               = 0;
static csx_nuint_t g_array_id           = 0;
static csx_bool_t  g_client_initialized = CSX_FALSE;

typedef struct thread_socket_map_s
{
    csx_thread_id_t             thread_id;
    csx_p_tcp_connect_socket_t  socket;
    csx_bool_t                  online;
    struct thread_socket_map_s *next;
} thread_socket_map_t;

static thread_socket_map_t *thread_socket_map_head = CSX_NULL;
static csx_p_mut_t          thread_socket_map_mutex;

#define __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_REQUEST_INFO__(request)        \
        terminator_trace(FBE_TRACE_LEVEL_INFO,                                  \
                         FBE_TRACE_MESSAGE_ID_INFO,                             \
                         "Terminator request type = %d, array ID = %u "         \
                         "port = %u, enclosure  = %u, slot = %u\n",             \
                         request.op,   request.array_id,                        \
                         request.port, request.enclosure,  request.slot);       \
        terminator_trace(FBE_TRACE_LEVEL_INFO,                                  \
                         FBE_TRACE_MESSAGE_ID_INFO,                             \
                         "lba  = %llu, block size = %u, block count = %llu ", \
                         request.lba,  request.block_size, request.block_count);

#define __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_RESPONSE_INFO__(response) \
        terminator_trace(FBE_TRACE_LEVEL_INFO,                                 \
                         FBE_TRACE_MESSAGE_ID_INFO,                            \
                         "Terminator response status: %X "                     \
                         "data len = %u\n",                                    \
                         response.status,                                      \
                         response.data_len);

#define __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FUNC_FAILURE__                           \
        terminator_trace(FBE_TRACE_LEVEL_ERROR,                                           \
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                            \
                         "Terminator %s failed from simulated disk client at line %d.\n", \
                         __FUNCTION__,                                                    \
                         __LINE__);

#define __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_REQUEST__(request) \
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_REQUEST_INFO__(request)       \
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FUNC_FAILURE__

#define __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_RESPONSE__(request, response) \
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_REQUEST_INFO__(request)                  \
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_RESPONSE_INFO__(response)                \
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FUNC_FAILURE__

#define __TERMINATOR_SIMULATED_DISK_CLIENT_PANIC_ON_REQUEST__(request)         \
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_REQUEST__(request) \
        FBE_PANIC_SP(FBE_PANIC_SP_BASE, FBE_STATUS_OK);

#define __TERMINATOR_SIMULATED_DISK_CLIENT_PANIC_ON_RESPONSE__(request, response)         \
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_RESPONSE__(request, response) \
        FBE_PANIC_SP(FBE_PANIC_SP_BASE, FBE_STATUS_OK);

static thread_socket_map_t * find_thread_socket_map_element(csx_thread_id_t thread_id)
{
    thread_socket_map_t *elem = CSX_NULL;

    csx_p_mut_lock(&thread_socket_map_mutex);

    for (elem = thread_socket_map_head; elem; elem = elem->next)
    {
        if (elem->thread_id == thread_id)
        {
            csx_p_mut_unlock(&thread_socket_map_mutex);

            return elem;
        }
    }

    csx_p_mut_unlock(&thread_socket_map_mutex);

    return CSX_NULL;
}

static csx_p_tcp_connect_socket_t find_thread_socket(csx_thread_id_t thread_id)
{
    thread_socket_map_t *elem = find_thread_socket_map_element(thread_id);

    if (elem && elem->online)
    {
        return elem->socket;
    }

    return CSX_NULL;
}

static void add_thread_socket_map(csx_thread_id_t thread_id, csx_p_tcp_connect_socket_t socket)
{
    thread_socket_map_t *elem = find_thread_socket_map_element(thread_id);

    if (!elem)
    {
        elem = (thread_socket_map_t *)csx_p_util_mem_alloc_always(sizeof(thread_socket_map_t));

        csx_p_mut_lock(&thread_socket_map_mutex);

        elem->thread_id = thread_id;
        elem->socket    = socket;
        elem->next      = thread_socket_map_head;
        elem->online    = CSX_TRUE;

        thread_socket_map_head = elem;

        csx_p_mut_unlock(&thread_socket_map_mutex);
    }
    else
    {
        CSX_ASSERT_H_DC(!elem->online);

        elem->socket = socket;
        elem->online = CSX_TRUE;
    }
}

static void offline_broken_socket(csx_thread_id_t thread_id)
{
    csx_status_e         csx_status = CSX_STATUS_GENERAL_FAILURE;
    thread_socket_map_t *elem       = find_thread_socket_map_element(thread_id);

    CSX_ASSERT_H_DC(elem && elem->online);

    csx_status = csx_p_tcp_connect_close(&elem->socket);
    if (!CSX_SUCCESS(csx_status))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "Terminator %s failed to close socket, status=%X.\n",
                         __FUNCTION__,
                         csx_status);
    }

    elem->online = CSX_FALSE;
}

static void remove_all_thread_socket_map(void)
{
    thread_socket_map_t *elem = CSX_NULL,
                        *temp = CSX_NULL;

    csx_p_mut_lock(&thread_socket_map_mutex);

    for (elem = thread_socket_map_head; elem;)
    {
        temp = elem->next;
        csx_p_util_mem_free(elem);
        elem = temp;
    }

    thread_socket_map_head = CSX_NULL;

    csx_p_mut_unlock(&thread_socket_map_mutex);
}

static csx_p_tcp_connect_socket_t start_client(void)
{
    csx_status_e csx_status = CSX_STATUS_GENERAL_FAILURE;

    csx_p_tcp_connect_socket_t tcp_connect_socket_object = CSX_NULL;
    csx_p_tcp_options_info_t   tcp_options_info;

    int i = 0;
    int retry_count = terminator_port_io_thread_count * terminator_port_count + 1;

    CSX_ASSERT_H_DC(g_client_initialized);

    /* AR486987: add retry in TCP connection setup from Terminator client to disk server.
     */
    for (i = 0; i < retry_count; ++i)
    {
        csx_status = csx_p_tcp_socket_open_to_peer(g_module_context,
                                                   &tcp_connect_socket_object,
                                                   g_address,
                                                   g_port,
                                                   TERMINATOR_TCP_CONNECT_TIMEOUT_MS);
        if (CSX_SUCCESS(csx_status))
        {
            break;
        }

        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "Terminator retry connect to disk server, i=%d, address=%s, port=%u status=%X.\n",
                         i, g_address, g_port, csx_status);
    }

    if (!CSX_SUCCESS(csx_status))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "Terminator failed to connect to disk server, address=%s, port=%u status=%X.\n",
                         g_address, g_port, csx_status);

        return CSX_NULL;
    }

    /* set TCP_NODELAY option to send data ASAP
     */
    csx_status = csx_p_tcp_options_get(&tcp_connect_socket_object, &tcp_options_info);
    if (!CSX_SUCCESS(csx_status))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "Terminator %s failed to get TCP options info, status=%X.\n",
                         __FUNCTION__, csx_status);
    }

    tcp_options_info.tcp_nodelay.is_valid = CSX_TRUE;
    tcp_options_info.tcp_nodelay.value    = 1;

    csx_status = csx_p_tcp_options_set(&tcp_connect_socket_object, &tcp_options_info);
    if (!CSX_SUCCESS(csx_status))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "Terminator %s failed to set TCP_NODELAY option, status=%X.\n",
                         __FUNCTION__, csx_status);
    }

    add_thread_socket_map(csx_p_get_thread_id(), tcp_connect_socket_object);

    return tcp_connect_socket_object;
}

static csx_bool_t end_all_clients(void)
{
    csx_status_e         csx_status = CSX_STATUS_GENERAL_FAILURE;
    thread_socket_map_t *elem       = thread_socket_map_head;

    CSX_ASSERT_H_DC(g_client_initialized);

    for (; elem; elem = elem->next)
    {
        if (!elem->online)
        {
            continue;
        }

        csx_status = csx_p_tcp_connect_close(&elem->socket);
        if (!CSX_SUCCESS(csx_status))
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "Terminator %s failed to close socket, status=%X.\n",
                             __FUNCTION__, csx_status);
        }

        elem->online = CSX_FALSE;
    }

    remove_all_thread_socket_map();

    return CSX_TRUE;
}

static csx_size_t send_buffer(csx_pvoid_t buffer, csx_size_t buffer_size, csx_bool_t allow_reconnect)
{
    csx_status_e    csx_status  = CSX_STATUS_GENERAL_FAILURE;
    csx_thread_id_t thread_id   = csx_p_get_thread_id();
    csx_size_t      bytes_sent  = 0;
    csx_nuint_t     retry_count = 0;

    csx_p_tcp_connect_socket_t tcp_connect_socket_object = find_thread_socket(thread_id);
    if (tcp_connect_socket_object == CSX_NULL)
    {
        if (!allow_reconnect)
        {
            return 0;
        }

        tcp_connect_socket_object = start_client();
        if (tcp_connect_socket_object == CSX_NULL)
        {
            return 0;
        }
    }

    for (;;)
    {
        csx_status = csx_p_tcp_socket_send(&tcp_connect_socket_object,
                                           buffer,
                                           buffer_size,
                                           &bytes_sent);
        if (CSX_SUCCESS(csx_status))
        {
            break;
        }

        bytes_sent = 0;

        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "Terminator %s failed to send buffer through socket, status=%X.\n", 
                         __FUNCTION__, csx_status);

        /* bring down the socket, so it would retry connecting next time
         */
        offline_broken_socket(thread_id);

        /* give it one more chance to be more robust
         */
        if (!allow_reconnect || retry_count > 0)
        {
            break;
        }

        ++retry_count;

        tcp_connect_socket_object = start_client();
        if (tcp_connect_socket_object == CSX_NULL)
        {
            break;
        }
    }

    return bytes_sent;
}

static csx_bool_t send_request(csx_pvoid_t request, csx_size_t request_size)
{
    csx_size_t bytes_sent = 0;

    CSX_CALL_CONTEXT;
    CSX_CALL_ENTER();

    bytes_sent = send_buffer(request, request_size, CSX_TRUE);

    CSX_CALL_LEAVE();

    return (bytes_sent > 0 ? CSX_TRUE : CSX_FALSE);
}

static csx_size_t recv_buffer(csx_pvoid_t buffer, csx_size_t buffer_size)
{
    csx_status_e    csx_status = CSX_STATUS_GENERAL_FAILURE;
    csx_thread_id_t thread_id  = csx_p_get_thread_id();
    csx_size_t      bytes_rcvd = 0;

    csx_p_tcp_connect_socket_t tcp_connect_socket_object = find_thread_socket(thread_id);
    if (tcp_connect_socket_object == CSX_NULL)
    {
        return 0;
    }

    csx_status = csx_p_tcp_socket_recv(&tcp_connect_socket_object,
                                       buffer,
                                       buffer_size,
                                       &bytes_rcvd);
    if (!CSX_SUCCESS(csx_status))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "Terminator %s failed to receive buffer through socket, status=%X.\n", 
                         __FUNCTION__, csx_status);

        /* if any error, bring down the socket, so it would retry connecting next time but when??
         */
        offline_broken_socket(thread_id);

        return 0;
    }

    return bytes_rcvd;
}

static csx_bool_t recv_response(csx_pvoid_t response, csx_size_t response_size)
{
    csx_size_t bytes_rcvd = 0;

    CSX_CALL_CONTEXT;
    CSX_CALL_ENTER();

    bytes_rcvd = recv_buffer(response, response_size);

    CSX_CALL_LEAVE();

    return (bytes_rcvd > 0 ? CSX_TRUE: CSX_FALSE);
}

static fbe_status_t retrieve_drive_location(terminator_drive_t *drive_handle,
                                            fbe_u32_t          *port,
                                            fbe_u32_t          *enclosure,
                                            fbe_u32_t          *slot)
{
    fbe_status_t status = terminator_get_port_index(drive_handle, port);
    if (!FBE_SUCCESS(status))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "Terminator %s can not get port number thru drive handle, status=%X.\n",
                         __FUNCTION__, status);

        return status;
    }

    status = terminator_get_drive_enclosure_number(drive_handle, enclosure);
    if (!FBE_SUCCESS(status))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "Terminator %s can not get enclosure_number thru drive handle, status=%X.\n", 
                         __FUNCTION__, status);

        return status;
    }

    status = terminator_get_drive_slot_number(drive_handle, slot);
    if (!FBE_SUCCESS(status))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "Terminator %s can not get slot_number thru drive handle, status=%X.\n", 
                         __FUNCTION__, status);

        return status;
    }

    return status;
}

fbe_status_t terminator_simulated_disk_remote_file_simple_create(fbe_u32_t        port,
                                                                 fbe_u32_t        enclosure,
                                                                 fbe_u32_t        slot,
                                                                 fbe_block_size_t block_size,
                                                                 fbe_lba_t        max_lba)
{
    remote_file_simple_response_t response;
    remote_file_simple_request_t  request = {CREATE_FILE_REQ,
                                             g_array_id,
                                             port,
                                             enclosure,
                                             slot,
                                             max_lba,
                                             block_size,
                                             0,
                                             0};

    if (!send_request((csx_pvoid_t)&request, sizeof(request)))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!recv_response((csx_pvoid_t)&response, sizeof(response)))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!FBE_SUCCESS(response.status))
    {
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_RESPONSE__(request, response);

        return response.status;
    }

    CSX_ASSERT_H_DC(response.data_len == 0);

    return response.status;
}

fbe_status_t terminator_simulated_disk_remote_file_simple_write(terminator_drive_t *drive_handle,
                                                                fbe_lba_t           lba,
                                                                fbe_block_count_t   block_count,
                                                                fbe_block_size_t    block_size,
                                                                fbe_u8_t           *data_buffer,
                                                                void               *context)
{
    fbe_status_t                  status      = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                     write_bytes = (fbe_u32_t)(block_size * block_count);
    remote_file_simple_response_t response;
    remote_file_simple_request_t *request;
        
    FBE_UNREFERENCED_PARAMETER(context);

    /* it seems the TCP_NODELAY has no effect in CSX kernel, so we have to pack all data together here for
     *  better performance
     */
    request = (remote_file_simple_request_t *)csx_p_util_mem_alloc_always(sizeof(remote_file_simple_request_t) + write_bytes);
    if (!request)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = retrieve_drive_location(drive_handle, &request->port, &request->enclosure, &request->slot);
    if (!FBE_SUCCESS(status))
    {
        csx_p_util_mem_free(request);
        return status;
    }

    request->op          = WRITE_FILE_REQ;
    request->array_id    = g_array_id;
    request->lba         = lba;
    request->block_size  = block_size;
    request->block_count = block_count;
    request->checksum    = get_checksum_for_eight_bytes_aligned_memory(data_buffer, write_bytes);

    csx_p_memcpy((csx_pvoid_t)(request + 1), data_buffer, write_bytes);

    if (!send_request((csx_pvoid_t)request, sizeof(remote_file_simple_request_t) + write_bytes))
    {
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_REQUEST__((*request));

        csx_p_util_mem_free(request);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!recv_response((csx_pvoid_t)&response, sizeof(response)))
    {
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_RESPONSE__((*request), response);

        csx_p_util_mem_free(request);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!FBE_SUCCESS(response.status))
    {
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_RESPONSE__((*request), response);

        csx_p_util_mem_free(request);

        return response.status;
    }

    csx_p_util_mem_free(request);

    CSX_ASSERT_H_DC(response.data_len == 0);

    return response.status;
}

fbe_status_t terminator_simulated_disk_remote_file_simple_write_zero_pattern(terminator_drive_t *drive_handle,
                                                                             fbe_lba_t           lba,
                                                                             fbe_block_count_t   block_count,
                                                                             fbe_block_size_t    block_size,
                                                                             fbe_u8_t           *data_buffer,
                                                                             void               *context)
{
    remote_file_simple_response_t response;
    remote_file_simple_request_t  request;
    fbe_status_t                  status = FBE_STATUS_GENERIC_FAILURE;

    FBE_UNREFERENCED_PARAMETER(context);

    status = retrieve_drive_location(drive_handle, &request.port, &request.enclosure, &request.slot);
    if (!FBE_SUCCESS(status))
    {
        return status;
    }

    request.op          = WRITE_FILE_ZERO_PATTERN_REQ;
    request.array_id    = g_array_id;
    request.lba         = lba;
    request.block_size  = block_size;
    request.block_count = block_count;
    request.checksum    = 0;

    if (!send_request((csx_pvoid_t)&request, sizeof(request)))
    {
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_REQUEST__(request);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!recv_response((csx_pvoid_t)&response, sizeof(response)))
    {
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_RESPONSE__(request, response);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!FBE_SUCCESS(response.status))
    {
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_RESPONSE__(request, response);

        return response.status;
    }

    CSX_ASSERT_H_DC(response.data_len == 0);

    return response.status;
}

fbe_status_t terminator_simulated_disk_remote_file_simple_read(terminator_drive_t *drive_handle,
                                                               fbe_lba_t           lba,
                                                               fbe_block_count_t   block_count,
                                                               fbe_block_size_t    block_size,
                                                               fbe_u8_t           *data_buffer,
                                                               void               *context)
{
    fbe_u32_t                     read_bytes = (fbe_u32_t)(block_size * block_count);
    remote_file_simple_response_t response;
    remote_file_simple_request_t  request;
    fbe_status_t                  status     = FBE_STATUS_GENERIC_FAILURE;

    FBE_UNREFERENCED_PARAMETER(context);

    status = retrieve_drive_location(drive_handle, &request.port, &request.enclosure, &request.slot);
    if (!FBE_SUCCESS(status))
    {
        return status;
    }

    request.op          = READ_FILE_REQ;
    request.array_id    = g_array_id;
    request.lba         = lba;
    request.block_size  = block_size;
    request.block_count = block_count;
    request.checksum    = 0;

    if (!send_request((csx_pvoid_t)&request, sizeof(request)))
    {
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_REQUEST__(request);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!recv_response((csx_pvoid_t)&response, sizeof(response)))
    {
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_RESPONSE__(request, response);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!FBE_SUCCESS(response.status))
    {
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_RESPONSE__(request, response);

        return response.status;
    }

    CSX_ASSERT_H_DC(response.data_len == read_bytes);

    if (!recv_response(data_buffer, read_bytes))
    {
        __TERMINATOR_SIMULATED_DISK_CLIENT_TRACE_FAILURE_ON_RESPONSE__(request, response);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    CSX_ASSERT_H_DC(response.checksum == get_checksum_for_eight_bytes_aligned_memory(data_buffer, read_bytes));

    return response.status;
}

void terminator_simulated_disk_remote_file_simple_init(csx_module_context_t context,
                                                       const char          *server_address,
                                                       csx_nuint_t          server_port,
                                                       csx_nuint_t          array_id)
{
    g_module_context     = context;
    csx_p_strcpy(g_address, server_address);
    g_port               = server_port;
    g_array_id           = array_id;
    g_client_initialized = CSX_TRUE;

    csx_p_mut_create_always(g_module_context, &thread_socket_map_mutex, "thread_sock_map_mutex");
}

void terminator_simulated_disk_remote_file_simple_destroy(void)
{
    end_all_clients();

    csx_p_mut_destroy(&thread_socket_map_mutex);
}

fbe_status_t send_heartbeat_message()
{
    remote_file_simple_request_t request;
    fbe_zero_memory(&request, sizeof(request));

    request.op = HEART_BEAT_REQ;

    if (!send_request((csx_pvoid_t)&request, sizeof(request)))
    {
        terminator_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "Sent heartbeat failed in %s() at line%d\n",
                         __FUNCTION__,
                         __LINE__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
