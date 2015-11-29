#include <stdio.h>

#include <winsock2.h>

#include "simulated_drive_private.h"

#include "terminator_drive.h"

#include "EmcUTIL_Time.h"

/*!*******************************************************************
 * @def FBE_SIMULATED_DRIVE_CLEANUP_WAIT_MS
 *********************************************************************
 * @brief This is how long we will wait for cleanup of the thread.
 *        This is intended to be much longer than needed and if
 *        we ever do see this timeout we will know something is stuck.
 *********************************************************************/
#define FBE_SIMULATED_DRIVE_CLEANUP_WAIT_MS 60000

// ----begin of socket code----
static SOCKET server_socket;
//static fbe_mutex_t socket_lock;
static fbe_semaphore_t send_request_semaphore;

SOCKET start_client(const fbe_u8_t* serverName, fbe_u16_t port)
{
    fbe_s32_t error;
    SOCKET mySocket;
    struct hostent *host_entry;
    struct sockaddr_in server;
    int max_retries = 10;
    int bOptVal;
    int bOptLen;

    //fbe_mutex_init(&socket_lock); /* SAFEBUG - moved so would be cleaned up in error case */
    

    if ((error = EmcutilNetworkWorldInitializeMaybe()) != 0)
    {
        printf("%s(%d): Client: WSAStartup() failed: %d.\n", __FUNCTION__, __LINE__, EmcutilLastNetworkErrorGet());
        EmcutilNetworkWorldDeinitializeMaybe();
        return -1;
    }
    mySocket = (SOCKET)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mySocket == INVALID_SOCKET)
    {
        printf("%s(%d): Client: socket() failed: %d.\n", __FUNCTION__, __LINE__, EmcutilLastNetworkErrorGet());
        EmcutilNetworkWorldDeinitializeMaybe();
        return -1;
    }

    bOptVal = 1;
    bOptLen = sizeof(bOptVal);
    setsockopt(mySocket, IPPROTO_TCP, TCP_NODELAY, (char*)&bOptVal, bOptLen);

    if ((host_entry = gethostbyname(serverName)) == NULL)
    {
        printf("%s(%d): Client: gethostbyname() failed: %d.\n", __FUNCTION__, __LINE__, EmcutilLastNetworkErrorGet());
        EmcutilNetworkWorldDeinitializeMaybe();
        return -1;
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = *(unsigned long*) host_entry->h_addr;
retry: /* SAFEMESS - on Linux you may need to try a little harder here in case it takes time for the peer to start up */
    if (connect(mySocket, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
    {
        if (EmcutilLastNetworkErrorGet() == WSAECONNREFUSED && max_retries > 0) {
            max_retries--;
            EmcutilSleep(1000);
            goto retry;
        }
        printf("%s(%d): Client: socket() failed: %d.\n", __FUNCTION__, __LINE__, EmcutilLastNetworkErrorGet());
        EmcutilNetworkWorldDeinitializeMaybe();
        return -1;
    }
//  printf("%s(%d): Client: Started up (Server %s: %d).\n", __FUNCTION__, __LINE__, serverName, port);

    return mySocket;
}


void end_client(SOCKET clientSocket)
{
    if (shutdown(clientSocket, SD_SEND) == SOCKET_ERROR) {
        printf("%s(%d): Client: shutdown() failed: %d.\n", __FUNCTION__, __LINE__, EmcutilLastNetworkErrorGet());
    }
    closesocket(clientSocket);
    EmcutilNetworkWorldDeinitializeMaybe();
//    fbe_mutex_destroy(&socket_lock);
//  printf("%s(%d): Client: Shutted down.\n", __FUNCTION__, __LINE__);
}

static fbe_s32_t send_buff(void *buffer, fbe_u32_t buffer_size)
{
    fbe_s32_t nBytes = 0;
    fbe_u8_t *buffer_ptr = buffer;

    //fbe_mutex_lock(&socket_lock);

    while(nBytes < buffer_size)
    {
        fbe_s32_t res = send(server_socket, buffer_ptr, buffer_size - nBytes, 0);
        if(res < 0)
        {
            printf("%s(%d): Client: send() failed: %d.\n", __FUNCTION__, __LINE__, res);
            //fbe_mutex_unlock(&socket_lock);
            return SOCKET_ERROR;
        }
        else if(res == 0)
        {
            break;
        }
        nBytes += res;
        buffer_ptr += res;
    }

    //fbe_mutex_unlock(&socket_lock);

    return nBytes;
}

fbe_s32_t recv_buff(void *buffer, fbe_u32_t buffer_size)
{
    fbe_s32_t nBytes = 0;
    fbe_u8_t *buffer_ptr = buffer;

    //fbe_mutex_lock(&socket_lock);

    while(nBytes < buffer_size)
    {
        fbe_s32_t res = recv(server_socket, buffer_ptr, buffer_size - nBytes, 0);
        if(res < 0)
        {
            printf("%s(%d): Client: send() failed: %d.\n", __FUNCTION__, __LINE__, res);
            //fbe_mutex_unlock(&socket_lock);
            return SOCKET_ERROR;
        }
        else if(res == 0)
        {
            break;
        }
        nBytes += res;
        buffer_ptr += res;
    }

    //fbe_mutex_unlock(&socket_lock);

    return nBytes;
}

static fbe_s32_t send_request(simulated_drive_request_t *request, void *buffer, fbe_u32_t nbytes_to_write)
{
    fbe_s32_t nBytes = 0;

    if ((request->type < SIMULATED_DRIVE_REQUEST_INIT) ||
        (request->type > SIMULATED_DRIVE_REQUEST_EXIT)    )
    {
        printf("%s(%d): Client: send() invalid request type: %d\n", __FUNCTION__, __LINE__, request->type);
        return (-1);
    }
    if ((request->handle != FBE_TERMINATOR_DRIVE_HANDLE_INVALID) &&
        (request->handle > 2048)                                    )
    {
        printf("%s(%d): Client: send() invalid request handle: %d\n", __FUNCTION__, __LINE__, (int)request->handle);
        return (-1);
    }

    /* Wait until we own the `send' semaphore. */
    fbe_semaphore_wait(&send_request_semaphore, NULL);
    if ((nBytes = send_buff(request, sizeof(simulated_drive_request_t))) == SOCKET_ERROR)
    {
        printf("%s(%d): Client: send request failed: %d.\n", __FUNCTION__, __LINE__, EmcutilLastNetworkErrorGet());
    }
    if ((buffer != NULL) && (nbytes_to_write > 0) && (nBytes != SOCKET_ERROR))
    {
        if ((nBytes = send_buff(buffer, nbytes_to_write)) == SOCKET_ERROR)
        {
            printf("%s(%d): Client: send buff failed: %d.\n", __FUNCTION__, __LINE__, EmcutilLastNetworkErrorGet());
        }
    }
    fbe_semaphore_release(&send_request_semaphore, 0, 1, FALSE);
    return nBytes;
}

fbe_s32_t recv_and_update_request(simulated_drive_request_t *request)
{
    fbe_s32_t nBytes;
//    fbe_u8_t buff[sizeof(simulated_drive_request_t)];

    if ((nBytes = recv_buff((fbe_u8_t *)request, sizeof(simulated_drive_request_t))) == SOCKET_ERROR)
    {
        printf("%s(%d): Client: recv() failed: %d.\n", __FUNCTION__, __LINE__, EmcutilLastNetworkErrorGet());
    }
//    memset(request, 0, sizeof(simulated_drive_request_t));
//    memcpy(request, buff, sizeof(simulated_drive_request_t));
    return nBytes;
}

// ----end of socket code----

static fbe_s32_t SP; /* 0 - SPA; 1 - SPB */
static fbe_thread_t callback_thread;
static fbe_bool_t callback_thread_running = FBE_FALSE;
static fbe_semaphore_t callback_thread_semaphore;
static fbe_semaphore_t callback_semaphore;

extern fbe_status_t sas_drive_process_xfer_read_completion(void * context);
extern fbe_status_t sas_drive_process_xfer_write_completion(void * context);

static void
callback_thread_func(void * context)
{
    simulated_drive_request_t request;
    fbe_s32_t rc;
    fbe_u32_t nBytes, each;
    fbe_s64_t cnt;
    fbe_bool_t is_done = FBE_FALSE;
    fbe_terminator_io_t * terminator_io = NULL;
	terminator_drive_t * drive = NULL;

    while(!is_done) {
        memset(&request, 0, sizeof(simulated_drive_request_t));

        fbe_semaphore_wait(&callback_semaphore, NULL);
        rc = recv_and_update_request(&request);

        if (rc <= 0){
            is_done = FBE_TRUE;
            break;
        }

        if (request.context == NULL){
            continue;
        }

        terminator_io = (fbe_terminator_io_t *)request.context;

        switch(request.type) {
            case SIMULATED_DRIVE_REQUEST_READ:
                
            if (request.return_size > 0) {
                fbe_u8_t *temp = (fbe_u8_t *)terminator_io->memory_ptr;
                cnt = request.return_size;
                while (cnt > 0) {
                    each = (fbe_u32_t)__min(cnt, SOCKET_EACH_TRANSFER_MAX_SIZE);
                    nBytes = recv_buff(temp, (fbe_u32_t)each);
                    cnt -= nBytes;
                    temp += nBytes;
                }
            }
            terminator_io->collision_found = request.collision_found;
            terminator_io->return_size = (fbe_u32_t)request.return_size;
			drive = (terminator_drive_t *)terminator_io->device_ptr;
			drive->drive_handle = request.handle;
            sas_drive_process_xfer_read_completion(request.context);
            break;
        case SIMULATED_DRIVE_REQUEST_WRITE:
            case SIMULATED_DRIVE_REQUEST_WRITE_SAME:
            terminator_io->collision_found = request.collision_found;
			drive = (terminator_drive_t *)terminator_io->device_ptr;
			drive->drive_handle = request.handle;			
            sas_drive_process_xfer_write_completion(request.context);
            break;
        case SIMULATED_DRIVE_REQUEST_CLEANUP:
            is_done = FBE_TRUE;
            break;
        default:
            printf("%s: Invalid request type: %d drive handle: %d SP: %s return_val: %d\n", 
                   __FUNCTION__, request.type, (int)request.handle, request.side ? "B" : "A", (int)request.return_val);
            terminator_io->collision_found = FBE_TRUE;
			drive = (terminator_drive_t *)terminator_io->device_ptr;
			drive->drive_handle = request.handle;
            sas_drive_process_xfer_write_completion(request.context);
            break;
        }

    }
    if (callback_thread_running==FBE_TRUE)
    {
        callback_thread_running = FBE_FALSE;
        fbe_semaphore_release(&callback_thread_semaphore, 0, 1, FALSE);
    }
    else
    {
        /* if the clean up thread already exited, we need to release the semaphore here.
        */
        fbe_semaphore_destroy(&callback_thread_semaphore);
        fbe_semaphore_destroy(&callback_semaphore);
    }
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

// ----begin of user interfaces----
fbe_bool_t fbe_simulated_drive_init(const fbe_u8_t* server_name, fbe_u16_t port, fbe_u32_t sp)
{
    simulated_drive_request_t request;

    memset(&request, 0, sizeof(simulated_drive_request_t));

    server_socket =  start_client(server_name, port);
    if(server_socket == SOCKET_ERROR)
    {
        end_client(server_socket);
        return FBE_FALSE;
    }

    SP = sp;
    request.type = SIMULATED_DRIVE_REQUEST_INIT;

    /* Initialize as full */
    fbe_semaphore_init(&send_request_semaphore, 1, 0x0FFFFFFF);

    send_request(&request, NULL, 0);
    recv_and_update_request(&request);

    fbe_semaphore_init(&callback_semaphore, 0, 0x0FFFFFFF);
    fbe_semaphore_init(&callback_thread_semaphore, 0, 1);
    callback_thread_running = FBE_TRUE;
    fbe_thread_init(&callback_thread, "fbe_sim_drv", callback_thread_func, NULL);

    return FBE_TRUE;
}

fbe_u64_t fbe_simulated_drive_get_server_pid(void)
{
    simulated_drive_request_t request;
    fbe_bool_t b_status;

    memset(&request, 0, sizeof(simulated_drive_request_t));
    request.type = SIMULATED_DRIVE_REQUEST_GET_SERVER_PID;

    b_status = send_request(&request, NULL, 0);
    if (b_status == FBE_FALSE)
    {
        ////fbe_DbgBreakPoint();
        return 0;
    }

    b_status = recv_and_update_request(&request);
    if (b_status == FBE_FALSE)
    {
        ////fbe_DbgBreakPoint();
        return 0;
    }
    return (fbe_u64_t)(request.return_val);
}
fbe_bool_t fbe_simulated_drive_cleanup(void)
{
    fbe_bool_t b_status;
    simulated_drive_request_t request;

    memset(&request, 0, sizeof(simulated_drive_request_t));
    request.type = SIMULATED_DRIVE_REQUEST_CLEANUP;
    request.context = &request;

    b_status = send_request(&request, NULL, 0);
    if (b_status == FBE_FALSE)
    {
        //fbe_DbgBreakPoint();
        return FBE_FALSE;
    }

    fbe_semaphore_release(&send_request_semaphore, 0, 1, FALSE);

    fbe_semaphore_destroy(&send_request_semaphore);

    /* Kick the thread so it will run to get this request.
     */
    fbe_semaphore_release(&callback_semaphore, 0, 1, FALSE);

    /* Wait for the thread to get this message and exit.
     */
    fbe_semaphore_wait_ms(&callback_thread_semaphore, 
                          FBE_SIMULATED_DRIVE_CLEANUP_WAIT_MS);

    if (!callback_thread_running) {
        fbe_thread_wait(&callback_thread);
    } else {

        /* let's close the socket first, just in case the callback_thread was
         * still waiting on socket read.
         */
        end_client(server_socket);

        /* Kick the thread so it will run to get this request.
         */
        fbe_semaphore_release(&callback_semaphore, 0, 1, FALSE);

        /* Wait again for the thread to get this message and exit.
         */
        fbe_semaphore_wait_ms(&callback_thread_semaphore, 
                              FBE_SIMULATED_DRIVE_CLEANUP_WAIT_MS);

        /* if still stop the thread, just exiting */
        if (!callback_thread_running) {
            fbe_thread_wait(&callback_thread);
        } else {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                          "%s: callback thread (%p) did not exit!\n",
                          __FUNCTION__, callback_thread.thread);
            
            /* this tells the callback thread we exited here without
             * releasing the resource.
             */
            callback_thread_running = FBE_FALSE;
            return FBE_FALSE;
        }
    }
    fbe_semaphore_destroy(&callback_thread_semaphore);
    fbe_thread_destroy(&callback_thread);

    fbe_semaphore_destroy(&callback_semaphore);

    end_client(server_socket);

    return FBE_TRUE;
}

simulated_drive_handle_t simulated_drive_open(simulated_drive_identity_t drive_identity)
{
    simulated_drive_request_t request;

    memset(&request, 0, sizeof(simulated_drive_request_t));

    request.type = SIMULATED_DRIVE_REQUEST_OPEN;
    memcpy(request.identity, drive_identity, sizeof(simulated_drive_identity_t));
    send_request(&request, NULL, 0);
    recv_and_update_request(&request);
    return request.handle;
}

fbe_bool_t simulated_drive_close(simulated_drive_handle_t handle)
{
    simulated_drive_request_t request;

    memset(&request, 0, sizeof(simulated_drive_request_t));

    request.type = SIMULATED_DRIVE_REQUEST_CLOSE;
    request.handle = handle;
    send_request(&request, NULL, 0);
    recv_and_update_request(&request);
    return (fbe_bool_t)request.return_val;
}

simulated_drive_handle_t simulated_drive_create(simulated_drive_identity_t drive_identity, fbe_u32_t drive_block_size, fbe_lba_t max_lba)
{
    simulated_drive_request_t request;

    memset(&request, 0, sizeof(simulated_drive_request_t));

    request.type = SIMULATED_DRIVE_REQUEST_CREATE;
    memcpy(request.identity, drive_identity, sizeof(simulated_drive_identity_t));
    request.size = drive_block_size;
    request.lba = max_lba;
    send_request(&request, NULL, 0);
    recv_and_update_request(&request);
    return request.handle;
}

fbe_bool_t simulated_drive_remove(simulated_drive_identity_t drive_identity)
{
    simulated_drive_request_t request;

    memset(&request, 0, sizeof(simulated_drive_request_t));

    request.type = SIMULATED_DRIVE_REQUEST_REMOVE;
    request.side = SP;
    request.handle = FBE_TERMINATOR_DRIVE_HANDLE_INVALID;
    memcpy(request.identity, drive_identity, sizeof(simulated_drive_identity_t));
    send_request(&request, NULL, 0);
    //Use receive here is not multi-thread safe(callback_thread_func will also receive in this socket),
    //and this will cause io failure. So I comment this receive, after all, no one care about the return val.
    //If someone need to enable this, please rewrite it in a multi-thread safe way.
    //recv_and_update_request(&request);
    return (fbe_bool_t)1;
}

fbe_bool_t simulated_drive_remove_all(void)
{
    simulated_drive_request_t request;

    memset(&request, 0, sizeof(simulated_drive_request_t));

    request.type = SIMULATED_DRIVE_REQUEST_REMOVE_ALL;
    send_request(&request, NULL, 0);
    recv_and_update_request(&request);
    return (fbe_bool_t)request.return_val;
}

fbe_u32_t simulated_drive_read(simulated_drive_identity_t drive_identity,
                                          void *buffer,
                                          fbe_lba_t lba,
                                          fbe_u32_t nbytes_to_read,
                                          void * context)
{
    //fbe_u32_t nBytes, each;
    //fbe_s64_t cnt;
    simulated_drive_request_t request;
    fbe_terminator_io_t * terminator_io = (fbe_terminator_io_t *)context;
	terminator_drive_t * drive = (terminator_drive_t *)terminator_io->device_ptr;

    memset(&request, 0, sizeof(simulated_drive_request_t));

    request.type = SIMULATED_DRIVE_REQUEST_READ;
    request.lba = lba;
    request.size = nbytes_to_read;
    request.side = SP;
    request.context = context;
	request.handle = drive->drive_handle;
    memcpy(request.identity, drive_identity, sizeof(simulated_drive_identity_t));
    request.block_size = terminator_io->block_size;

    send_request(&request, NULL, 0);
    fbe_semaphore_release(&callback_semaphore, 0, 1, FALSE);

    return 0;
}

fbe_u32_t simulated_drive_write(simulated_drive_identity_t drive_identity,
                                void *buffer,
                                fbe_lba_t lba,
                                fbe_u32_t nbytes_to_write,
                                void * context)
{
    fbe_u32_t nBytes;
    simulated_drive_request_t request;
    fbe_terminator_io_t * terminator_io = (fbe_terminator_io_t *)context;
	terminator_drive_t * drive = (terminator_drive_t *)terminator_io->device_ptr;

    memset(&request, 0, sizeof(simulated_drive_request_t));

    request.type = SIMULATED_DRIVE_REQUEST_WRITE;
    request.lba = lba;
    request.size = nbytes_to_write;
    request.side = SP;
    request.context = context;
	request.handle = drive->drive_handle;
    memcpy(request.identity, drive_identity, sizeof(simulated_drive_identity_t));
    request.block_size = terminator_io->block_size;
    if (terminator_io->is_compressed) {
        if (terminator_io->block_size > FBE_BE_BYTES_PER_BLOCK) {
            request.block_size = sizeof(fbe_terminator_compressed_block_t) * (terminator_io->block_size / FBE_BE_BYTES_PER_BLOCK);
        } else {
            request.block_size = sizeof(fbe_terminator_compressed_block_t);
        }
    }
    nBytes = send_request(&request, buffer, nbytes_to_write);
    fbe_semaphore_release(&callback_semaphore, 0, 1, FALSE);
    return 0;
}
 
fbe_u32_t simulated_drive_write_same(simulated_drive_identity_t drive_identity,
                                                void *buffer,
                                                fbe_lba_t lba,
                                                fbe_u32_t nbytes_in_buffer,
                                                fbe_u32_t repeat_count,
                                                void * context)
{
    fbe_u32_t nBytes;
    simulated_drive_request_t request;
    fbe_terminator_io_t * terminator_io = (fbe_terminator_io_t *)context;
	terminator_drive_t * drive = (terminator_drive_t *)terminator_io->device_ptr;

    memset(&request, 0, sizeof(simulated_drive_request_t));

    request.b_key_valid = terminator_io->b_key_valid;
    if (terminator_io->b_key_valid) {
        fbe_copy_memory(&request.keys[0], &terminator_io->keys[0], (sizeof(fbe_u8_t) * FBE_ENCRYPTION_KEY_SIZE));
    }
    request.type = SIMULATED_DRIVE_REQUEST_WRITE_SAME;
    request.lba = lba;
    /* Total number of bytes to be written */
    request.repeat_count = repeat_count;
    request.size = nbytes_in_buffer;
    request.side = SP;
    request.context = context;
	request.handle = drive->drive_handle;
    memcpy(request.identity, drive_identity, sizeof(simulated_drive_identity_t));
    request.block_size = terminator_io->block_size;

    nBytes = send_request(&request, buffer, nbytes_in_buffer);
    fbe_semaphore_release(&callback_semaphore, 0, 1, FALSE);

    return (fbe_u32_t)request.return_size;
}
// ----end of user interfaces----
