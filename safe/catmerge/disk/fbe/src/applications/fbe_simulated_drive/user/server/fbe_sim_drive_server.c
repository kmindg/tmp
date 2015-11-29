#include <time.h>

#include <winsock2.h>

#include "simulated_drive_private.h"

#include "terminator_drive.h"
#include "terminator_simulated_disk.h"

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_emcutil_shell_include.h"

#include "EmcUTIL_Time.h"

#include "csx_ext.h"

typedef enum request_queue_thread_flag_e{
    REQUEST_QUEUE_THREAD_RUN,
    REQUEST_QUEUE_THREAD_STOP,
    REQUEST_QUEUE_THREAD_DONE
}request_queue_thread_flag_t;

static fbe_queue_head_t request_queue;
static fbe_mutex_t      request_queue_lock;
static fbe_semaphore_t  request_queue_semaphore;
static fbe_thread_t     request_queue_thread;
static request_queue_thread_flag_t request_queue_thread_flag;
static fbe_u32_t client_count = 0;
static fbe_mutex_t client_count_lock;

static void recv_thread_func(void *context);

static void check_collisions(simulated_drive_request_t *request);
static void print_collision_info(simulated_drive_request_t *request, simulated_drive_request_t *current_request);

typedef struct stats_s{
    fbe_u64_t io_count;
    fbe_u64_t read_count;
    fbe_u64_t write_count;
    fbe_bool_t is_updated; /* 0 - not updated, 1 - updated */
}stats_t;

static fbe_mutex_t sim_drive_stats_lock;
static fbe_u32_t read_collisions = 0;
static stats_t sp_stats[2]; /* 0 - SPA, 1 - SPB */
static SOCKET socket_index[2];

enum sim_drive_constants_e{
    SIM_DRIVE_MAX_DRIVES = 1024,
};

static terminator_drive_t * drive_table[SIM_DRIVE_MAX_DRIVES];
static fbe_u32_t drive_reference_count[SIM_DRIVE_MAX_DRIVES];
static fbe_mutex_t      drive_table_lock;

/*! @todo start debug code 
 * We need this debug code to root cause issues where the sim drive server/client 
 * get out of sync. 
 */
/*!*******************************************************************
 * @struct fbe_sim_drive_log_t
 *********************************************************************
 * @brief This is a simple ring structure appropriate for logging where
 *        we do not want to affect the timing.
 *
 *********************************************************************/
typedef struct fbe_sim_drive_log_s
{
    fbe_u64_t arg0;
    fbe_u64_t arg1;
    fbe_u64_t arg2;
    fbe_u64_t arg3;
    fbe_u64_t arg4;
    fbe_u64_t arg5;
}
fbe_sim_drive_log_t;

static fbe_bool_t fbe_sim_drive_log_is_initialized = FBE_FALSE;
#define FBE_SIM_DRIVE_LOG_MAX_ENTRIES 50000
static fbe_sim_drive_log_t fbe_sim_drive_log_array[FBE_SIM_DRIVE_LOG_MAX_ENTRIES];
static fbe_u32_t fbe_sim_drive_log_index = 0;
static fbe_spinlock_t fbe_sim_drive_log_lock;

fbe_status_t fbe_sim_drive_log_record(fbe_u64_t arg0, fbe_u64_t arg1, fbe_u64_t arg2, fbe_u64_t arg3, fbe_u64_t arg4, fbe_u64_t arg5)
{
    if (fbe_sim_drive_log_is_initialized == FBE_FALSE)
    {
        fbe_sim_drive_log_is_initialized = FBE_TRUE;
        fbe_spinlock_init(&fbe_sim_drive_log_lock);
    }

    fbe_spinlock_lock(&fbe_sim_drive_log_lock);
    fbe_sim_drive_log_array[fbe_sim_drive_log_index].arg0 = arg0;
    fbe_sim_drive_log_array[fbe_sim_drive_log_index].arg1 = arg1;
    fbe_sim_drive_log_array[fbe_sim_drive_log_index].arg2 = arg2;
    fbe_sim_drive_log_array[fbe_sim_drive_log_index].arg3 = arg3;
    fbe_sim_drive_log_array[fbe_sim_drive_log_index].arg4 = arg4;
    fbe_sim_drive_log_array[fbe_sim_drive_log_index].arg5 = arg5;

    fbe_sim_drive_log_index++;
    if (fbe_sim_drive_log_index >= FBE_SIM_DRIVE_LOG_MAX_ENTRIES)
    {
        fbe_sim_drive_log_index = 0;
    }
    fbe_spinlock_unlock(&fbe_sim_drive_log_lock);
    return FBE_STATUS_OK;
}

#define MAX_TRACE_CHARS 256
static void get_time(char *time_p)
{    
    char d[MAX_TRACE_CHARS];
    char t[MAX_TRACE_CHARS];

#ifdef ALAMOSA_WINDOWS_ENV
    _tzset();
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

    _strdate(d);
    _strtime(t);

    _snprintf(time_p, MAX_TRACE_CHARS, "%s %s",d,t);    
}
static void sim_drive_trace(fbe_u8_t * string_p, ...)
{
    char time_str[MAX_TRACE_CHARS];
    char buffer[MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;

    if (string_p != NULL)
    {
        va_start(argList, string_p);
        num_chars_in_string = _vsnprintf(buffer, MAX_TRACE_CHARS, string_p, argList);
        buffer[MAX_TRACE_CHARS - 1] = '\0';
        va_end(argList);
        get_time(&time_str[0]);
        printf("%s: %s", time_str, buffer);
    }
}

static void sim_drive_server_clean_drive_table(void)
{
    fbe_u32_t i;
    journal_record_t * record = NULL;
    fbe_queue_element_t * queue_element;
    terminator_drive_t * drive;

    fbe_mutex_lock(&drive_table_lock);
    for(i = 0; i < SIM_DRIVE_MAX_DRIVES; i++){
        if(drive_table[i] != NULL){
            drive = drive_table[i];
            /* Walk trou the queue and release all journal_records */
            fbe_spinlock_lock(&drive->journal_lock);
            queue_element = fbe_queue_pop(&drive->journal_queue_head);
            /* printf("\n\n"); */
            while(queue_element != NULL){
                record = (journal_record_t *)queue_element;

                /* printf("free lba %d block_count %d \n", (int)(record->lba), (int)(record->block_count)); */

                terminator_simulated_disk_memory_free_record(drive, &record);
                queue_element = fbe_queue_pop(&drive->journal_queue_head);
            }
            fbe_spinlock_unlock(&drive->journal_lock);

            fbe_spinlock_destroy(&drive->journal_lock);
            fbe_queue_destroy(&drive->journal_queue_head);
            drive_table[i] = NULL;
            drive_reference_count[i] = 0;
        }
    }
    fbe_mutex_unlock(&drive_table_lock);
}

static fbe_u64_t sim_drive_server_get_handle(fbe_u8_t * drive_identity)
{
    fbe_u32_t i;

    fbe_mutex_lock(&drive_table_lock);
    for(i = 0; i < SIM_DRIVE_MAX_DRIVES; i++){
        if(drive_table[i] != NULL){
            if(!strncmp(drive_identity, drive_table[i]->drive_identity, FBE_TERMINATOR_DRIVE_IDENTITY_SIZE)){
                fbe_mutex_unlock(&drive_table_lock);
                return i;
            }
        }
    }
    /* We did not find drive - lets create it */
    for(i = 0; i < SIM_DRIVE_MAX_DRIVES; i++){
        if(drive_table[i] == NULL){
            drive_table[i] = malloc(sizeof(terminator_drive_t));
            if (drive_table[i] == NULL) {
                sim_drive_trace("FBE Server: malloc() failed in %s\n", __FUNCTION__);
                break;
            }
            fbe_zero_memory(drive_table[i], sizeof(terminator_drive_t));
            fbe_copy_memory(drive_table[i]->drive_identity, drive_identity, FBE_TERMINATOR_DRIVE_IDENTITY_SIZE);

            /* Initialize journal lock and queue */
            fbe_spinlock_init(&drive_table[i]->journal_lock);
            fbe_queue_init(&drive_table[i]->journal_queue_head);
            fbe_mutex_unlock(&drive_table_lock);
            return i;
        }
    }

    fbe_mutex_unlock(&drive_table_lock);
    return FBE_TERMINATOR_DRIVE_HANDLE_INVALID;
}


static void accept_thread_func(void *context)
{
    SOCKET mySocket = *((SOCKET *)context);
    SOCKET clientSocket;
    fbe_thread_t *thread;

    while (1) {
        clientSocket = accept(mySocket, 0, 0);
        if (clientSocket == SOCKET_ERROR) {
            sim_drive_trace("FBE Server accept error()\n");
            continue;
        } else {
            sim_drive_trace("FBE Server: Client has connected.\n");
        }

        if(socket_index[0] == 0){
            socket_index[0] = clientSocket;
        } else {
            socket_index[1] = clientSocket;
        }

        thread = malloc(sizeof(fbe_thread_t));
        if (thread == NULL) {
            sim_drive_trace("FBE Server accept fail to allocate resource()\n");
            continue;
        }

        fbe_mutex_lock(&client_count_lock);
        ++client_count;
        fbe_thread_init(thread, "fbe_srv_recv", recv_thread_func, &clientSocket);
        fbe_mutex_unlock(&client_count_lock);
    }
}

static SOCKET start_listening(unsigned short port)
{
    int error;
    int bOptVal;
    int bOptLen;
    int sock_buf_size;
    SOCKET s;
    struct sockaddr_in server;

    error = EmcutilNetworkWorldInitializeMaybe();
    if(error != 0) {
        sim_drive_trace("FBE Server WSAStartup() Failed \n");
        return error;
    }

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == SOCKET_ERROR) {
        sim_drive_trace("FBE Server socket() Failed\n");
        return error;
    }
    
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    bOptVal = FBE_TRUE;
    bOptLen = sizeof(bOptVal);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&bOptVal, bOptLen);

    error = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&bOptVal, bOptLen);

    bOptLen = sizeof(sock_buf_size);
    sock_buf_size = SOCKET_EACH_TRANSFER_MAX_SIZE *2;
    error = setsockopt(s, SOL_SOCKET, SO_SNDBUF,(char *)&sock_buf_size, bOptLen );
    error = getsockopt(s, SOL_SOCKET, SO_SNDBUF,(char *)&sock_buf_size, &bOptLen );
    sim_drive_trace("send buffer size %d, status %d\n", sock_buf_size, error );

    sock_buf_size = SOCKET_EACH_TRANSFER_MAX_SIZE *4;
    error = setsockopt(s, SOL_SOCKET, SO_RCVBUF,(char *)&sock_buf_size, bOptLen );
    error = getsockopt(s, SOL_SOCKET, SO_RCVBUF,(char *)&sock_buf_size, &bOptLen);
    sim_drive_trace("send buffer size %d, status %d\n", sock_buf_size, error );

    error = bind(s, (SOCKADDR*)&server, sizeof(server));
    if(error == SOCKET_ERROR) {
        sim_drive_trace("FBE Server bind() Failed\n");
        closesocket(s);
        return error;
    }

    if (error == SOCKET_ERROR) {
        sim_drive_trace("FBE Server setsockopt() Failed, bind() May Fail\n");
    }
    error = listen(s, 5);
    if (error == SOCKET_ERROR) {
        sim_drive_trace("FBE Server listen() Failed\n");
        closesocket(s);
        return error;
    }
    sim_drive_trace("FBE Server READY.\n");
    return s;
}

void end_server(SOCKET socket)
{
    closesocket(socket);
    EmcutilNetworkWorldDeinitializeMaybe();
    sim_drive_trace("FBE Server Shutted down.\n");
}

static fbe_status_t sim_drive_init(SOCKET clientSocket, simulated_drive_request_t *request)
{
    return FBE_STATUS_OK;
}

static fbe_status_t sim_drive_cleanup(SOCKET clientSocket, simulated_drive_request_t *request)
{
    fbe_mutex_lock(&client_count_lock);
    if(--client_count <= 0)
    {
        sim_drive_server_clean_drive_table();
    }
    fbe_mutex_unlock(&client_count_lock);
    return FBE_STATUS_OK;
}

static fbe_status_t sim_drive_create(SOCKET clientSocket, simulated_drive_request_t *request)
{
    sim_drive_trace("Drive %d created SP: %s reference_mask: %d\n", 
                    request->handle, request->side ? "B" : "A", drive_reference_count[request->handle]);
    drive_reference_count[request->handle] |= (1 << request->side);
    return FBE_STATUS_OK;
}

static fbe_status_t sim_drive_remove(SOCKET clientSocket, simulated_drive_request_t *request)
{
    terminator_drive_t *drive = NULL;

    drive = drive_table[request->handle];

    sim_drive_trace("Drive %d destroyed SP: %s reference_mask: %d\n", 
                    request->handle, request->side ? "B" : "A", drive_reference_count[request->handle]);

    /* Clear reference count for this SP.
     */
    if ((drive_reference_count[request->handle] & (1 << request->side)) != 0)
    {
        drive_reference_count[request->handle] &= ~(1 << request->side);
    }

    /* This remove only clears the journal out.  We'll allow the remove all later to 
     * clean up the spinlocks. 
     */
    if (drive_reference_count[request->handle] == 0)
    {
#if 0
        fbe_mutex_lock(&drive_table_lock);
        if (drive_table[request->handle] != NULL)
        {
            /* Walk through the queue and release all journal_records */
            fbe_spinlock_lock(&drive->journal_lock);
            queue_element = fbe_queue_pop(&drive->journal_queue_head);
            /* printf("\n\n"); */
            while (queue_element != NULL)
            {
                record = (journal_record_t *)queue_element;
    
                /* sim_drive_trace("free lba %d block_count %d \n", (int)(record->lba), (int)(record->block_count)); */
    
                terminator_simulated_disk_memory_free_record(drive, &record);
                queue_element = fbe_queue_pop(&drive->journal_queue_head);
            }
            fbe_spinlock_unlock(&drive->journal_lock);
        }
        fbe_mutex_unlock(&drive_table_lock);
#endif
    }
    return FBE_STATUS_OK;
}

static fbe_status_t sim_drive_remove_all(SOCKET clientSocket, simulated_drive_request_t *request)
{
    return FBE_STATUS_OK;
}

static fbe_status_t sim_drive_read(SOCKET client_socket, simulated_drive_request_t *request)
{
    fbe_u32_t send_size = 0;
    fbe_u32_t nBytes = 0;
    fbe_u8_t * buffer = NULL;
    fbe_terminator_io_t terminator_io;

    /* We need to allocate the memory and read the user data */
    buffer = malloc((fbe_u32_t)request->size);
    if (buffer == NULL) {
        sim_drive_trace("FBE Server: malloc() failed in %s\n", __FUNCTION__);
        closesocket(client_socket);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    memset(buffer, 0, (size_t)request->size);
    request->memory_ptr = buffer;

    terminator_simulated_disk_memory_read(drive_table[request->handle],
                                                    request->lba,
                                                    (fbe_block_count_t)( request->size / request->block_size),
                                                    request->block_size,
                                                    request->memory_ptr,
                                                    &terminator_io);

    request->return_size = terminator_io.return_size;
    nBytes = send(request->socket_id, (char *)request, sizeof(simulated_drive_request_t), 0);
    fbe_sim_drive_log_record(0xF0010 | request->side, nBytes, (fbe_u64_t)request->context, request->lba, request->size,
                             request->return_size);
    while(send_size < request->return_size){

        nBytes = send(client_socket, buffer + send_size, (int)(request->return_size - send_size), 0);
        fbe_sim_drive_log_record(0x80010 | request->side, nBytes, (fbe_u64_t)request->context, request->lba, request->size,
                                 request->return_size);
        if (nBytes == SOCKET_ERROR  || nBytes == 0) {
            /* This simply means the client has closed this socket*/
            sim_drive_trace("FBE Server: Client has disconnected\n");
            closesocket(client_socket);
            free(buffer);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        send_size += nBytes;
    }
    free(buffer);
    request->memory_ptr = NULL;
    return FBE_STATUS_OK;
}

static fbe_status_t sim_drive_write(SOCKET client_socket, simulated_drive_request_t *request)
{
    fbe_u32_t rcv_size = 0;
    fbe_u32_t nBytes = 0;
    fbe_u8_t * buffer = NULL;

    /* We need to allocate the memory and read the user data */
    buffer = malloc((fbe_u32_t)request->size);
    if (buffer == NULL) {
        sim_drive_trace("FBE Server: malloc() failed in %s\n", __FUNCTION__);
        closesocket(client_socket);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    request->memory_ptr = buffer;

    /* Read the data from the socket */
    while(rcv_size < request->size){
        nBytes = recv(client_socket, buffer + rcv_size, (int)(request->size - rcv_size), 0);

        fbe_sim_drive_log_record(0x80020 | request->side, nBytes, (fbe_u64_t)request->context, request->lba, request->size,
                                 request->return_size);
        if (nBytes == SOCKET_ERROR  || nBytes == 0) {
            /* This simply means the client has closed this socket*/
            sim_drive_trace("FBE Server: Client has disconnected\n");
            closesocket(client_socket);
            free(buffer);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        rcv_size += nBytes;
    }

    terminator_simulated_disk_memory_write(drive_table[request->handle],
                                                    request->lba,
                                                    (fbe_block_count_t)( request->size / request->block_size),
                                                    request->block_size,
                                                    request->memory_ptr,
                                                    NULL);

    free(request->memory_ptr);

    return FBE_STATUS_OK;
}

static fbe_status_t sim_drive_write_same(SOCKET client_socket, simulated_drive_request_t *request)
{
    fbe_u32_t rcv_size = 0;
    fbe_u32_t nBytes = 0;
    fbe_u8_t * buffer = NULL;
    fbe_terminator_io_t terminator_io;

    /* We need to allocate the memory and read the user data */
    buffer = malloc((fbe_u32_t)request->size);
    if (buffer == NULL) {
        sim_drive_trace("FBE Server: malloc() failed in %s\n", __FUNCTION__);
        closesocket(client_socket);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Read the data from the socket */
    while(rcv_size < request->size){
        nBytes = recv(client_socket, buffer + rcv_size, (int)(request->size - rcv_size), 0);
        if (nBytes == SOCKET_ERROR  || nBytes == 0) {
            /* This simply means the client has closed this socket*/
            sim_drive_trace("FBE Server: Client has disconnected\n");
            closesocket(client_socket);
            free(buffer);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        rcv_size += nBytes;
    }
#if 0
    big_buffer = malloc((fbe_u32_t)(request->size * request->repeat_count));
    if (big_buffer == NULL) {
        sim_drive_trace("FBE Server: malloc() failed in %s\n", __FUNCTION__);
        closesocket(client_socket);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    request->memory_ptr = big_buffer;
    for(i = 0; i < request->repeat_count; i++){
        fbe_copy_memory(big_buffer, buffer,  (fbe_u32_t)request->size);
        big_buffer += request->size;
    }

    free(buffer);

    terminator_simulated_disk_memory_write(drive_table[request->handle],
                                                    request->lba,
                                                    (fbe_block_count_t)( request->size / request->block_size),
                                                    request->block_size,
                                                    request->memory_ptr,
                                                    NULL);
#endif
    terminator_io.b_key_valid = request->b_key_valid;
    if (terminator_io.b_key_valid) {
        fbe_copy_memory(&terminator_io.keys[0], &request->keys[0], sizeof(fbe_u8_t) * FBE_ENCRYPTION_KEY_SIZE);
    }
    terminator_simulated_disk_memory_write_zero_pattern(drive_table[request->handle],
                                                        request->lba,
                                                        request->repeat_count,
                                                        (fbe_block_size_t)request->size,
                                                        buffer,
                                                        &terminator_io);
    free(buffer);
    return FBE_STATUS_OK;
}

static fbe_status_t sim_drive_get_server_pid(SOCKET client_socket, simulated_drive_request_t *request)
{
    request->return_val = csx_p_get_process_id();
    send(client_socket, (char *)request, sizeof(simulated_drive_request_t),0);

    return FBE_STATUS_OK;
}

static void 
simulated_drive_dispatch_request(simulated_drive_request_t *request)
{
    int nBytes;

    // Dispatch to corresponding handler
    switch (request->type){
    case SIMULATED_DRIVE_REQUEST_GET_SERVER_PID:
        sim_drive_get_server_pid(request->socket_id, request);
        break;
        /* Here we will take care of statistics */
    case SIMULATED_DRIVE_REQUEST_READ:
        if(request->side < 2){
            fbe_mutex_lock(&sim_drive_stats_lock);
            sp_stats[request->side].io_count++;
            sp_stats[request->side].read_count++;
            sp_stats[request->side].is_updated = 1;
            fbe_mutex_unlock(&sim_drive_stats_lock);
        }
        sim_drive_read(request->socket_id, request);
        break;
    case SIMULATED_DRIVE_REQUEST_WRITE:
        if(request->side < 2){
            fbe_mutex_lock(&sim_drive_stats_lock);
            sp_stats[request->side].io_count++;
            sp_stats[request->side].write_count++;
            sp_stats[request->side].is_updated = 1;
            fbe_mutex_unlock(&sim_drive_stats_lock);
        }
        nBytes = send(request->socket_id, (char *)request, sizeof(simulated_drive_request_t), 0);

        fbe_sim_drive_log_record(0xF0020 | request->side, nBytes, (fbe_u64_t)request->context, request->lba, request->size,
                                 request->return_size);
        if (nBytes != sizeof(simulated_drive_request_t)) {
            sim_drive_trace("Simulated drive: write status bytes %d != %d\n", nBytes, (int)sizeof(simulated_drive_request_t));
        }
        break;
    case SIMULATED_DRIVE_REQUEST_WRITE_SAME:
        if(request->side < 2){
            fbe_mutex_lock(&sim_drive_stats_lock);
            sp_stats[request->side].io_count++;
            sp_stats[request->side].write_count++;
            sp_stats[request->side].is_updated = 1;
            fbe_mutex_unlock(&sim_drive_stats_lock);
        }
        nBytes = send(request->socket_id, (char *)request, sizeof(simulated_drive_request_t), 0);
        if (nBytes != sizeof(simulated_drive_request_t)) {
            sim_drive_trace("Simulated drive: write same status bytes %d != %d\n", nBytes, (int)sizeof(simulated_drive_request_t));
        }
        break;
    }
}


static void recv_thread_func(void *context)
{
    int nBytes;
    int rcv_size = 0;
    SOCKET client_socket = *((SOCKET *)context);
    fbe_u8_t * buffer;
    simulated_drive_request_t *request;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    while (1) {    

        buffer = (fbe_u8_t *)malloc(sizeof(simulated_drive_request_t));
        if (buffer == NULL) {
            sim_drive_trace("FBE Server: malloc() failed in %s\n", __FUNCTION__);
            closesocket(client_socket);
            break;
        }

        nBytes = 0;
        rcv_size = 0;

        while(rcv_size < sizeof(simulated_drive_request_t)){
            nBytes = recv(client_socket, buffer + rcv_size, sizeof(simulated_drive_request_t) - rcv_size, 0);
            if (nBytes == SOCKET_ERROR  || nBytes == 0) {
                /* This simply means the client has closed this socket*/
                sim_drive_trace("FBE Server: Client has disconnected\n");
                closesocket(client_socket);
                free(buffer);
                break;
            }
            rcv_size += nBytes;
        }

        /* This means the client has closed this socket and we break out so that
         * we cleanup (i.e. exit the `per-socket' worker thread).
         */
        if(rcv_size < sizeof(simulated_drive_request_t))
        {
            break;
        }

        request = (simulated_drive_request_t *)buffer;

        request->socket_id = (fbe_u32_t)client_socket;
        if (request->handle == FBE_TERMINATOR_DRIVE_HANDLE_INVALID)
        {
            request->handle = sim_drive_server_get_handle(request->identity);
        }

        /* If handle is bad report an error.*/
        if (request->handle >= SIM_DRIVE_MAX_DRIVES) 
        {
            sim_drive_trace("%s: Invalid drive handle: %d SP: %s socket: %d request type: %d\n", 
                            __FUNCTION__, request->handle, 
                            (request->socket_id == socket_index[0]) ? "A" : "B", 
                            request->socket_id, request->type);
            request->return_val = -1;
            request->collision_found = 1;
            send(client_socket, (char *)request, sizeof(simulated_drive_request_t), 0);
            free(request);
            continue;
        }

        /* Inc reference count for this SP.
         */
        if ( (drive_reference_count[request->handle] & (1 << request->side)) == 0)
        {
            drive_reference_count[request->handle] |= (1 << request->side);
            sim_drive_trace("Drive %d attached SP: %s reference_mask: %d type: %d\n", 
                            request->handle, request->side ? "B" : "A", drive_reference_count[request->handle], request->type);
        }

        // Dispatch to corresponding handler
        switch (request->type)
        {
        case SIMULATED_DRIVE_REQUEST_GET_SERVER_PID:
            fbe_mutex_lock(&request_queue_lock);
            fbe_queue_element_init(&request->queue_element);
            fbe_queue_push(&request_queue, &request->queue_element);
            fbe_semaphore_release(&request_queue_semaphore, 0, 1, FALSE);
            fbe_mutex_unlock(&request_queue_lock);
            break;
        case SIMULATED_DRIVE_REQUEST_INIT:
            status = sim_drive_init(client_socket, request);
            request->return_val = (status == FBE_STATUS_OK ? 1 : 0);
            send(client_socket, (char *)request, sizeof(simulated_drive_request_t), 0);
            free(request);
            break;
        case SIMULATED_DRIVE_REQUEST_CLEANUP:
            status = sim_drive_cleanup(client_socket, request);
            request->return_val = (status == FBE_STATUS_OK ? 1 : 0);
            send(client_socket, (char *)request, sizeof(simulated_drive_request_t), 0);
            free(request);
            break;
        case SIMULATED_DRIVE_REQUEST_CREATE:
            status = sim_drive_create(client_socket, request);
            request->return_val = (status == FBE_STATUS_OK ? 1 : 0);
            send(client_socket, (char *)request, sizeof(simulated_drive_request_t), 0);
            free(request);
            break;
        case SIMULATED_DRIVE_REQUEST_REMOVE:
            status = sim_drive_remove(client_socket, request);
            request->return_val = (status == FBE_STATUS_OK ? 1 : 0);
            //Use send here is not multi-thread safe(request_queue_thread_func will also send in this socket),
            //and this will cause io failure. So I comment this send, after all, no one care about the return val.
            //If someone need to enable this, please rewrite it in a multi-thread safe way.
            //send(client_socket, request, NULL, 0);
            free(request);
            break;
        case SIMULATED_DRIVE_REQUEST_REMOVE_ALL:
            status = sim_drive_remove_all(client_socket, request);
            request->return_val = (status == FBE_STATUS_OK ? 1 : 0);
            send(client_socket, (char *)request, sizeof(simulated_drive_request_t), 0);
            free(request);
            break;
        case SIMULATED_DRIVE_REQUEST_READ:
            fbe_sim_drive_log_record(0x00010 | request->side, 0, (fbe_u64_t)request->context, request->lba, request->size, 0);
            fbe_mutex_lock(&request_queue_lock);
            check_collisions(request);
            fbe_queue_element_init(&request->queue_element);
            fbe_queue_push(&request_queue, &request->queue_element);
            fbe_semaphore_release(&request_queue_semaphore, 0, 1, FALSE);
            fbe_mutex_unlock(&request_queue_lock);
            break;

        case SIMULATED_DRIVE_REQUEST_WRITE:
            /* we need to get data for that request */
            fbe_sim_drive_log_record(0x00020 | request->side, 0, (fbe_u64_t)request->context, request->lba, request->size, 0);
            sim_drive_write(request->socket_id, request);
            fbe_mutex_lock(&request_queue_lock);
            check_collisions(request);
            fbe_queue_element_init(&request->queue_element);
            fbe_queue_push(&request_queue, &request->queue_element);
            fbe_semaphore_release(&request_queue_semaphore, 0, 1, FALSE);
            fbe_mutex_unlock(&request_queue_lock);
            break;

        case SIMULATED_DRIVE_REQUEST_WRITE_SAME:
            /* we need to get data for that request */
            fbe_sim_drive_log_record(0x000AA0 | request->side, 0, (fbe_u64_t)request->context, request->lba, request->size, 0);
            sim_drive_write_same(request->socket_id, request);
            fbe_mutex_lock(&request_queue_lock);
            check_collisions(request);
            fbe_queue_element_init(&request->queue_element);
            fbe_queue_push(&request_queue, &request->queue_element);
            fbe_semaphore_release(&request_queue_semaphore, 0, 1, FALSE);
            fbe_mutex_unlock(&request_queue_lock);
            break;
        default:
            sim_drive_trace("%s: Invalid request type: %d drive handle: %d SP: %s \n", 
                            __FUNCTION__, request->type, request->handle, 
                            (request->socket_id == socket_index[0]) ? "A" : "B");
            request->return_val = -1;
            request->collision_found = 1;
            send(client_socket, (char *)request, sizeof(simulated_drive_request_t), 0);
            free(request);
            break;
        }

    }

    if(socket_index[0] == client_socket){
        socket_index[0] = 0;
    } else {
        socket_index[1] = 0;
    }

}

static int stats_exit;
void stats_thread_func(void* unref)
{

    while(!stats_exit){
        EmcutilSleep(10000);
        fbe_mutex_lock(&sim_drive_stats_lock);
        if(sp_stats[0].is_updated || sp_stats[1].is_updated){
            sim_drive_trace("SPA io %lld; w %lld; r %lld SPB io %lld; w %lld; r %lld \n",sp_stats[0].io_count,
                                                                                        sp_stats[0].write_count,
                                                                                        sp_stats[0].read_count,
                                                                                        sp_stats[1].io_count,
                                                                                        sp_stats[1].write_count,
                                                                                        sp_stats[1].read_count);
            sim_drive_trace("Read collisions: %d \n", read_collisions);
            sp_stats[0].is_updated = 0;
            sp_stats[1].is_updated = 0;
        }
        fbe_mutex_unlock(&sim_drive_stats_lock);

    }
}

static void 
request_queue_thread_func(void * context)
{
    simulated_drive_request_t * request;

    while(1) {
        fbe_semaphore_wait(&request_queue_semaphore, NULL);

        if(request_queue_thread_flag == REQUEST_QUEUE_THREAD_RUN) {
            fbe_mutex_lock(&request_queue_lock);
            request = (simulated_drive_request_t *)fbe_queue_pop(&request_queue);
            fbe_mutex_unlock(&request_queue_lock);
            if(request != NULL){
                simulated_drive_dispatch_request(request);
                free(request);
            }
        } else {
            break;
        }
    }

    request_queue_thread_flag = REQUEST_QUEUE_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

void set_console_title(fbe_u32_t port, fbe_u32_t test_pid)
{
    fbe_char_t current_console_title[232] = {'\0'};
    fbe_char_t new_console_title[256] = {'\0'};
    fbe_u32_t current_fbe_pid = csx_p_get_process_id();
    fbe_bool_t fbe_test_list_cmd_flag = false;

    if(!fbe_test_list_cmd_flag)
    {
        csx_uap_native_console_title_get(current_console_title, 232);
        _snprintf(new_console_title, sizeof(new_console_title)-1, " PID: %d port: %d FbeTestPID: %d ", current_fbe_pid, port, test_pid);
        strncat(new_console_title, current_console_title,(sizeof(new_console_title)-1-strlen(new_console_title)));
        csx_uap_native_console_title_set(new_console_title);
     }
}

int __cdecl main(int argc, char *argv[])
{
    SOCKET serverSocket;
    unsigned short server_port = SIMULATED_DRIVE_DEFAULT_PORT_NUMBER;
    int value;
    fbe_u32_t i;
    fbe_thread_t stats_thread;
    fbe_thread_t accept_thread;
    fbe_u32_t test_pid = 0;
    csx_ic_id_t fbe_test_ic_id;

#include "fbe/fbe_emcutil_shell_maincode.h"
    // set mode
    if(argc > 1) {
        for(i = 1; i < argc; ++i) {
            if(!strcmp("-port", argv[i])) {
                i++;
                sim_drive_trace("FBE Server -port %s\n",argv[i]);
                value = atoi(argv[i]);
                server_port = (unsigned short)value;
            }
            if(!strcmp("-testpid", argv[i])) {
                i++;
                sim_drive_trace("FBE Server -port %s\n",argv[i]);
                value = atoi(argv[i]);
                test_pid = (unsigned short)value;
            }
            if(!strcmp("-fbe_test_ic_id", argv[i])) {
                i++;
                sim_drive_trace("FBE Server -fbe_test_ic_id %s\n",argv[i]);
                value = atoi(argv[i]);
                fbe_test_ic_id = (csx_ic_id_t)value;
            }
        }
    }
    {
        csx_status_e status;
        status = csx_p_grman_connection_establish(CSX_MY_MODULE_CONTEXT(), fbe_test_ic_id);
        if (status == CSX_STATUS_ALREADY_CONNECTED)
        {
            status = CSX_STATUS_SUCCESS;
        }
        CSX_ASSERT_SUCCESS_H_RDC(status);
    }
    set_console_title(server_port, test_pid);

    for(i = 0 ; i < SIM_DRIVE_MAX_DRIVES; i++){
        drive_table[i] = NULL;
    }
    fbe_mutex_init(&drive_table_lock);

    /* Start request queue thread's */
    fbe_mutex_init(&request_queue_lock);
    fbe_queue_init(&request_queue);
    fbe_mutex_init(&sim_drive_stats_lock);

    fbe_semaphore_init(&request_queue_semaphore, 0, 0x0FFFFFFF);
    request_queue_thread_flag = REQUEST_QUEUE_THREAD_RUN;
    fbe_thread_init(&request_queue_thread, "fbe_srv_req", request_queue_thread_func, NULL);

    memset(sp_stats,0, 2 * sizeof(stats_t));
   
    fbe_mutex_init(&client_count_lock);

    socket_index[0] = 0;
    socket_index[1] = 0;

    stats_exit = 0;

    // Connect to the port specified
    serverSocket = start_listening(server_port);
    if (serverSocket == SOCKET_ERROR) {
        return 1;
    }
    fbe_thread_init(&stats_thread, "fbe_srv_stat", stats_thread_func, NULL);
    fbe_thread_init(&accept_thread, "fbe_srv_accept", accept_thread_func, &serverSocket);
    fbe_thread_wait(&accept_thread);
    fbe_thread_destroy(&accept_thread);

    stats_exit = 1;
    fbe_thread_wait(&stats_thread);
    fbe_thread_destroy(&stats_thread);

    fbe_mutex_destroy(&sim_drive_stats_lock);

    request_queue_thread_flag = REQUEST_QUEUE_THREAD_STOP;  
    fbe_semaphore_release(&request_queue_semaphore, 0, 1, FALSE);
    fbe_thread_wait(&request_queue_thread);
    fbe_thread_destroy(&request_queue_thread);

    fbe_semaphore_destroy(&request_queue_semaphore);
    fbe_queue_destroy(&request_queue);
    fbe_mutex_destroy(&request_queue_lock);
    
    fbe_mutex_destroy(&client_count_lock);

    sim_drive_server_clean_drive_table();
    return 0;
}

static void check_collisions(simulated_drive_request_t *request)
{
    fbe_queue_element_t * queue_element = NULL;
    fbe_queue_element_t * next_queue_element = NULL;
    fbe_queue_head_t * queue;
    simulated_drive_request_t * current_request;
    fbe_u32_t i = 0;
    fbe_u32_t block_size;

    queue = &request_queue;
    /* Check if queue is empty */
    if(fbe_queue_is_empty(&request_queue)){
        return;
    }

    next_queue_element = fbe_queue_front(queue);
    while(next_queue_element){
        queue_element = next_queue_element;
        next_queue_element = fbe_queue_next(queue, queue_element);

        current_request = (simulated_drive_request_t *)queue_element;
        if(i++ > 0x00ffffff){
            sim_drive_trace("FBE Server: Critical error looped queue....\n");
            break;
        }
#if 0
        if(strncmp(current_request->identity, request->identity, sizeof(simulated_drive_identity_t))){
            continue;
        }
#endif
        if(current_request->handle != request->handle){
            continue;
        }

        block_size = request->block_size;

        if((request->lba <= current_request->lba) && ((request->lba + (request->size / request->block_size)) > current_request->lba)){
            if((request->type == SIMULATED_DRIVE_REQUEST_READ) && (current_request->type == SIMULATED_DRIVE_REQUEST_READ)){
                read_collisions ++;
            } else {
                request->collision_found++;
                current_request->collision_found++;
                print_collision_info(request, current_request);
            }
            continue;
        }

        if((current_request->lba <= request->lba) && ((current_request->lba + (current_request->size / current_request->block_size)) > request->lba)){
            if((request->type == SIMULATED_DRIVE_REQUEST_READ) && (current_request->type == SIMULATED_DRIVE_REQUEST_READ)){
                read_collisions ++;
            } else {
                request->collision_found++;
                current_request->collision_found++;
                print_collision_info(request, current_request);
            }
        }
    }

    return ;
}

static 
void print_collision_info(simulated_drive_request_t *request, simulated_drive_request_t *current_request)
{
    sim_drive_trace("Collision %s",request->identity);
    if(request->side == 0){
        sim_drive_trace(" SPA ");
    } else {
        sim_drive_trace(" SPB ");
    }
    sim_drive_trace(" LBA 0x%llX BC 0x%llX Context %p\n",request->lba,(request->size / request->block_size),request->context);

    sim_drive_trace(" \t\t With ");
    if(current_request->side == 0){
        sim_drive_trace(" SPA ");
    } else {
        sim_drive_trace(" SPB ");
    }

    sim_drive_trace(" LBA 0x%llX BC 0x%llX Context %p\n\n",current_request->lba,(current_request->size / current_request->block_size),current_request->context);
}

/*! @todo Would be nice to get some debug information. 
 */
void terminator_trace(fbe_trace_level_t trace_level, fbe_trace_message_id_t message_id, const char * fmt, ...)
{

}

void terminator_trace_report(fbe_trace_level_t trace_level,
                             fbe_u32_t message_id,
                             const fbe_u8_t * fmt, 
                             va_list args)
{

}
