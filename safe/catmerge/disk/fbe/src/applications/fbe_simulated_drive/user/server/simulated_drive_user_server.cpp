#include <iostream>

#define I_AM_NATIVE_CODE
#include <windows.h>

#include "simulated_drive_private_cpp.h"

extern "C"
{
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_sector.h"
}

#include "EmcUTIL_Time.h"

using namespace std;

#define PRINT_MSG(_msg) \
    print_time(); \
    cout << __FUNCTION__ << "(" <<  __LINE__ << "): " << _msg << endl;

//---------Server Code--------
typedef enum request_queue_thread_flag_e{
    REQUEST_QUEUE_THREAD_RUN,
    REQUEST_QUEUE_THREAD_STOP,
    REQUEST_QUEUE_THREAD_DONE
}request_queue_thread_flag_t;

static fbe_queue_head_t request_queue;
static fbe_mutex_t	request_queue_lock;
static fbe_semaphore_t	request_queue_semaphore;
static fbe_thread_t		request_queue_thread;
static request_queue_thread_flag_t request_queue_thread_flag;
static simulated_drive_u32_t request_queue_delay;


static fbe_thread_t Hthread;

static simulated_drive_bool_t gQuitFlag = SIMULATED_DRIVE_FALSE;

static void worker_thread_func(fbe_thread_user_context_t client_socket_ptr);
static void check_collisions(simulated_drive_request_t *request);
static void print_collision_info(simulated_drive_request_t *request, simulated_drive_request_t *current_request);

typedef struct stats_s{
	simulated_drive_u64_t io_count;
	simulated_drive_u64_t read_count;
	simulated_drive_u64_t write_count;
	simulated_drive_s32_t is_updated; /* 0 - not updated, 1 - updated */
}stats_t;

static simulated_drive_mutext_t sp_stats_mutex;

static simulated_drive_u32_t read_collisions = 0;
static fbe_mutex_t socket_lock[2];
static simulated_drive_u32_t socket_index[2];

static stats_t sp_stats[2]; /* 0 - SPA, 1 - SPB */

/*! @note Should be checkin as false */
static simulated_drive_bool_t simulated_drive_write_same_debug = SIMULATED_DRIVE_FALSE;

static simulated_drive_s32_t send_buff(simulated_drive_socket_t socket, void *buffer, simulated_drive_u32_t buffer_size)
{
    simulated_drive_u32_t nBytes = 0;
    simulated_drive_byte_t *buffer_ptr = (simulated_drive_byte_t *)buffer;

    while(nBytes < buffer_size)
    {
        simulated_drive_u32_t res = send(socket, buffer_ptr, buffer_size - nBytes, 0);
        if(res < 0)
        {
            return SOCKET_ERROR;
        }
        else if(res == 0)
        {
            break;
        }
        nBytes += res;
        buffer_ptr += res;
    }

    return nBytes;
}

static simulated_drive_s32_t recv_buff(simulated_drive_socket_t socket, void *buffer, simulated_drive_u32_t buffer_size)
{
    simulated_drive_u32_t nBytes = 0;
    simulated_drive_byte_t *buffer_ptr = (simulated_drive_byte_t *)buffer;

    while(nBytes < buffer_size)
    {
        simulated_drive_u32_t res = recv(socket, buffer_ptr, buffer_size - nBytes, 0);
        if(res < 0)
        {
            return SOCKET_ERROR;
        }
        else if(res == 0)
        {
            break;
        }
        nBytes += res;
        buffer_ptr += res;
    }

    return nBytes;
}

void accept_thread(fbe_thread_user_context_t server_socket)
{
    simulated_drive_socket_t mySocket = *(simulated_drive_socket_t *)server_socket;

    while (1)
    {
        simulated_drive_socket_t client_socket = (simulated_drive_socket_t)accept(mySocket, 0, 0);

        if (client_socket == SOCKET_ERROR)
        {
            PRINT_SOCKET_ERR_MSG("FBE Server", "accept()")
            closesocket(client_socket);
            continue;
        }
        else
        {
            print_time();
            cout << __FUNCTION__ << "(" <<  __LINE__ << "): " << "FBE Server: Client on [" << client_socket << "] has connected." << endl;
        }
        if(socket_index[0] == 0){
            socket_index[0] = client_socket;
        } else {
            socket_index[1] = client_socket;
        }

        fbe_thread_t  worker_thread;
        EMCPAL_STATUS status;

        status = fbe_thread_init(&worker_thread, "fbe_srv_worker",
                                 worker_thread_func,
                                 (fbe_thread_user_context_t)&(socket_index[0] == client_socket ? socket_index[0] : socket_index[1]));

        if (EMCPAL_STATUS_SUCCESS != status)
        {
            PRINT_SOCKET_ERR_MSG("Server", "create worker thread")
        }
    }
}

simulated_drive_s32_t start_listening(simulated_drive_port_t port)
{
    simulated_drive_s32_t error;
    WSAData wsaData;
    if ((error = EmcutilNetworkWorldInitializeMaybe()) != 0)
    {
        PRINT_SOCKET_ERR_MSG("Server", "WSAStartup()")
        return -1;
    }
    simulated_drive_s32_t result = (simulated_drive_s32_t)socket(AF_INET, SOCK_STREAM, 0);
    if (result == SOCKET_ERROR)
    {
        PRINT_SOCKET_ERR_MSG("Server", "socket()")
        return -1;
    }
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    if (bind(result, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        PRINT_SOCKET_ERR_MSG("Server", "bind()")
        closesocket(result);
        return -1;
    }

    simulated_drive_bool_t bOptVal = TRUE;
    simulated_drive_u32_t bOptLen = sizeof(simulated_drive_bool_t);
    setsockopt(result, SOL_SOCKET, SO_REUSEADDR, (simulated_drive_byte_t*)&bOptVal, bOptLen);

    if (listen(result, 5) == SOCKET_ERROR)
    {
        PRINT_SOCKET_ERR_MSG("Server", "listen()")
        closesocket(result);
        return -1;
    }
    PRINT_MSG("Server: READY.")
    return result;
}

void end_server(simulated_drive_socket_t socket)
{
    closesocket(socket);
    EmcutilNetworkWorldDeinitializeMaybe();
    PRINT_MSG("Server: Shutted down.")
}

simulated_drive_s32_t handle_init(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_INIT == request->type)
    {
        request->return_int = _simulated_drive_init();
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_cleanup(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_CLEANUP == request->type)
    {
        request->return_int = _simulated_drive_cleanup();
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_get_drive_list(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_GET_DRIVE_LIST == request->type)
    {
        simulated_drive_list_t drive_list = _simulated_drive_get_drive_list();

        simulated_drive_s32_t result;
        request->return_int = drive_list.size;

        result = send_buff(client_socket, request, sizeof(simulated_drive_request_t));
        if (SOCKET_ERROR == result)
        {
            return result;
        }

        result = send_buff(client_socket, (&drive_list), sizeof(simulated_drive_list_t));
        return result;
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_get_server_pid(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_GET_SERVER_PID == request->type)
    {
        request->return_int = _simulated_drive_get_server_pid();
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_open(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_OPEN == request->type)
    {
        request->handle = _simulated_drive_open(request->identity);
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_close(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_CLOSE == request->type)
    {
        request->return_int = _simulated_drive_close(request->handle);
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_create(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_CREATE == request->type)
    {
        request->handle = _simulated_drive_create(request->identity, request->size, request->lba);
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_remove(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_REMOVE == request->type)
    {
        request->return_int = _simulated_drive_remove(request->identity);
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_remove_all(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_REMOVE_ALL == request->type)
    {
        request->return_int = _simulated_drive_remove_all();
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_set_total_capacity(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_SET_TOTAL_CAPACITY == request->type)
    {
        request->return_int = _simulated_drive_set_total_capacity(request->capacity);
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_set_mode(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_SET_MODE== request->type)
    {
        request->return_int = _simulated_drive_set_mode((simulated_drive_mode_e)request->int_val);
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_set_backend_type(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_SET_MODE== request->type)
    {
        request->return_int = _simulated_drive_set_backend_type((simulated_drive_backend_type_e)request->int_val);
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_read(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    simulated_drive_size_t nBytes, nblocks;

	request->handle = _simulated_drive_open(request->identity);
	if(request->handle == SIMULATED_DRIVE_INVALID_HANDLE){
		request->handle = _simulated_drive_create(request->identity, request->block_size, 0x0FFFFFFFFFFFFFFF);
	}

    nblocks = request->int_val / SIMULATED_DRIVE_GET_BLOCK_SIZE(request->handle);
    /* in case the bytes user requests is not a multiple of the block size */
    if(request->int_val % SIMULATED_DRIVE_GET_BLOCK_SIZE(request->handle) != 0)
    {
		PRINT_SOCKET_ERR_MSG(client_socket, "this should never ever happen")
        ++nblocks;
    }

    simulated_drive_byte_t *buffer = (simulated_drive_byte_t*)allocate_memory(nblocks * SIMULATED_DRIVE_GET_BLOCK_SIZE(request->handle));
    request->return_size = _simulated_drive_read(request->handle, (void *)buffer, request->lba, nblocks);

    if (request->return_size != SIMULATED_DRIVE_INVALID_SIZE) {
        request->return_size = request->int_val;
    }

    if ((nBytes = send_buff(client_socket, request, sizeof(simulated_drive_request_t))) == SOCKET_ERROR)
    {
        PRINT_SOCKET_ERR_MSG(client_socket, "Socket error on read")
        release_memory(buffer);
		_simulated_drive_close(request->handle);
        return (simulated_drive_s32_t)nBytes;
    }

    if (request->return_size != SIMULATED_DRIVE_INVALID_SIZE) {
        nBytes = (simulated_drive_size_t)send_buff(client_socket, buffer, (simulated_drive_s32_t)request->return_size);

        if (nBytes != request->return_size)
        {
            PRINT_SOCKET_ERR_MSG(client_socket, "Socket error on read, return size not expected")
        }
        release_memory(buffer);
		_simulated_drive_close(request->handle);
        return (simulated_drive_s32_t)nBytes;
    }

    release_memory(buffer);
	_simulated_drive_close(request->handle);

    return 1;
}

simulated_drive_s32_t handle_get_file_size(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    if (SIMULATED_DRIVE_REQUEST_GET_FILE_SIZE == request->type)
    {
        request->return_size = get_file_size(simulated_drive_file_handle);
        return send_buff(client_socket, request, sizeof(simulated_drive_request_t));
    }
    return SOCKET_ERROR;
}

simulated_drive_s32_t handle_write(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    simulated_drive_size_t nBytes, cnt = request->int_val, nblocks;
    simulated_drive_byte_t *buffer = NULL;
    simulated_drive_byte_t *temp = NULL;

	request->handle = _simulated_drive_open(request->identity);
	if(request->handle == SIMULATED_DRIVE_INVALID_HANDLE){
		request->handle = _simulated_drive_create(request->identity, request->block_size, 0x0FFFFFFFFFFFFFFF);
	}

    nblocks = request->int_val / SIMULATED_DRIVE_GET_BLOCK_SIZE(request->handle);
    /* in case the bytes user requests is not a multiple of the block size */
    if(request->int_val % SIMULATED_DRIVE_GET_BLOCK_SIZE(request->handle) != 0)
    {
		PRINT_SOCKET_ERR_MSG(client_socket, "this should never ever happen")
        ++nblocks;
    }

	buffer = (simulated_drive_byte_t*)allocate_memory(nblocks * SIMULATED_DRIVE_GET_BLOCK_SIZE(request->handle));
    if (buffer == NULL)
    {
        PRINT_SOCKET_ERR_MSG(client_socket, "failed to allocate memory for write")
        return (simulated_drive_s32_t)0;
    }
    memory_set(buffer, 0, nblocks * SIMULATED_DRIVE_GET_BLOCK_SIZE(request->handle));
    temp = buffer;
	request->memory_ptr = buffer;

    nBytes = recv_buff(client_socket, temp, (simulated_drive_u32_t)request->int_val);
    if (nBytes == SOCKET_ERROR)
    {
        PRINT_SOCKET_ERR_MSG(client_socket, "recv()")
        release_memory(buffer);
        return (simulated_drive_s32_t)nBytes;
    }
    request->return_size = _simulated_drive_write(request->handle, (void *)buffer, request->lba, nblocks);

	_simulated_drive_close(request->handle);
    return 0;
}

simulated_drive_s32_t handle_write_same(simulated_drive_socket_t client_socket, simulated_drive_request_t *request)
{
    simulated_drive_size_t nBytes, nBytes_in_buffer, nblocks_in_buffer;
    simulated_drive_byte_t *buffer = NULL;
    simulated_drive_byte_t *temp = NULL;

    /*! Description of request fields (some are specific to WRITE SAME):
     *  o lba - The starting drive lba for the SCSI WRITE SAME request
     *  o size - The number of `disk size' blocks to write with the
     *           buffer supplied
     *  o int_val - The total number of bytes that will be written to disk
     *              (i.e. NOT the number of bytes in the write buffer!!)
     */

	request->handle = _simulated_drive_open(request->identity);

	if(request->handle == SIMULATED_DRIVE_INVALID_HANDLE){
		request->handle = _simulated_drive_create(request->identity, request->block_size, 0x0FFFFFFFFFFFFFFF);
	}
    
    nBytes_in_buffer = request->int_val / request->size;
    nblocks_in_buffer = nBytes_in_buffer / SIMULATED_DRIVE_GET_BLOCK_SIZE(request->handle);

    /*! @todo Currently this interface only supports multiples of FBE_BE_BYTES_PER_BLOCK buffers */
    if((nBytes_in_buffer % FBE_BE_BYTES_PER_BLOCK) != 0)
    {
		PRINT_SOCKET_ERR_MSG(client_socket, "nBytes_in_buffer not a multiple of FBE_BE_BYTES_PER_BLOCK")
        return (simulated_drive_s32_t)0;
    }

    /* in case the bytes user requests is not a multiple of the block size */
    if((nBytes_in_buffer % SIMULATED_DRIVE_GET_BLOCK_SIZE(request->handle)) != 0)
    {
		PRINT_SOCKET_ERR_MSG(client_socket, "nBytes_in_buffer not a multiple of drive block size")
        /* Mark this request as a collision */
        request->collision_found++;
        return (simulated_drive_s32_t)0;
    }

	buffer = (simulated_drive_byte_t*)allocate_memory(nblocks_in_buffer * SIMULATED_DRIVE_GET_BLOCK_SIZE(request->handle));
    if (buffer == NULL)
    {
        PRINT_SOCKET_ERR_MSG(client_socket, "failed to allocate memory for write same")
        /* Mark this request as a collision */
        request->collision_found++;
        return (simulated_drive_s32_t)0;
    }
    memory_set(buffer, 0, nblocks_in_buffer * SIMULATED_DRIVE_GET_BLOCK_SIZE(request->handle));
    temp = buffer;
	request->memory_ptr = buffer;

    /* Number of bytes received is a single block */
    nBytes = recv_buff(client_socket, temp, (simulated_drive_u32_t)nBytes_in_buffer);
    if (nBytes == SOCKET_ERROR)
    {
        PRINT_SOCKET_ERR_MSG(client_socket, "recv()")
        release_memory(buffer);
        return (simulated_drive_s32_t)nBytes;
    }
    /*! @note Debug information for WRITE SAME requests
     */
    if ((simulated_drive_write_same_debug == SIMULATED_DRIVE_TRUE) &&
        (request->lba >= 0x10000ULL)                  )
    {
        simulated_drive_byte_t       *metadata_p;
        simulated_drive_u64_t   default_pattern = (FBE_SECTOR_INVALID_TSTAMP << 16 | FBE_SECTOR_DEFAULT_CHECKSUM);
        
        metadata_p = buffer + (FBE_BE_BYTES_PER_BLOCK - 8);
        print_time();
        cout << "Info: drive ["
             << request->identity
             << "] write same to lba " << request->lba << " blocks " << request->size 
             << " metadata " << *((simulated_drive_u64_t *)metadata_p)
             << "." << endl;
        
        /*! @note Metadata should always be default pattern */
        if (*((simulated_drive_u64_t *)metadata_p) != default_pattern)
        {
            /* Mark this request as a collision */
            request->collision_found++;
        }
    }

    request->return_size = _simulated_drive_write_same(request->handle, (void *)buffer, request->lba, nblocks_in_buffer, (int)request->size);
    _simulated_drive_close(request->handle);
    return 0;
}

static void 
simulated_drive_dispatch_request(simulated_drive_request_t *request)
{
    simulated_drive_s32_t nBytes;

    // Dispatch to corresponding handler
	switch (request->type){
		/* Here we will take care of statistics */
    case SIMULATED_DRIVE_REQUEST_READ:
		if(request->side < 2){
			wait_mutex(&sp_stats_mutex);
			sp_stats[request->side].io_count++;
			sp_stats[request->side].read_count++;
			sp_stats[request->side].is_updated = 1;
			release_mutex(&sp_stats_mutex);
		}
        nBytes = handle_read(request->socket_id, request);
        break;
    case SIMULATED_DRIVE_REQUEST_WRITE:
		if(request->side < 2){
			wait_mutex(&sp_stats_mutex);
			sp_stats[request->side].io_count++;
			sp_stats[request->side].write_count++;
			sp_stats[request->side].is_updated = 1;
			release_mutex(&sp_stats_mutex);
		}
		nBytes = send_buff(request->socket_id, request, sizeof(simulated_drive_request_t));
        if (nBytes != sizeof(simulated_drive_request_t))
        {
            cout << "Simulated drive: write status bytes " << nBytes << " != " << sizeof(simulated_drive_request_t);
        }
		release_memory(request->memory_ptr);
        //nBytes = handle_write(request->socket_id, request);
        break;
    case SIMULATED_DRIVE_REQUEST_WRITE_SAME:
		if(request->side < 2){
			wait_mutex(&sp_stats_mutex);
			sp_stats[request->side].io_count++;
			sp_stats[request->side].write_count++;
			sp_stats[request->side].is_updated = 1;				
			release_mutex(&sp_stats_mutex);
		}
		nBytes = send_buff(request->socket_id, request, sizeof(simulated_drive_request_t));

        if (nBytes != sizeof(simulated_drive_request_t))
        {
            cout << "Simulated drive: write same status bytes " << nBytes << " != " << sizeof(simulated_drive_request_t);
        }
		release_memory(request->memory_ptr);
        //nBytes = handle_write_same(request->socket_id, request);
        break;
    }
}

static void worker_thread_func(fbe_thread_user_context_t client_socket_ptr)
{
    simulated_drive_s32_t nBytes;
    simulated_drive_s32_t rcv_size = 0;

    if(client_socket_ptr == NULL)
    {
        PRINT_SOCKET_ERR_MSG("Server", "get client socket")
        return;
    }
    simulated_drive_socket_t client_socket = *(simulated_drive_socket_t *)client_socket_ptr;

    while (1)
    {
        if (gQuitFlag){
            break;
        }        
		
        simulated_drive_byte_t * buffer = (simulated_drive_byte_t *)allocate_memory(sizeof(simulated_drive_request_t));

		nBytes = 0;
		rcv_size = 0;

#if 0
		if(socket_index[0] == client_socket){
			fbe_mutex_lock(&socket_lock[0]);
		} else {
			fbe_mutex_lock(&socket_lock[1]);
		}
#endif

		rcv_size = recv_buff(client_socket, buffer, sizeof(simulated_drive_request_t));

        /* This means the client has closed this socket and we break out so that
         * we cleanup (i.e. exit the `per-socket' worker thread).
         */
		if(rcv_size < sizeof(simulated_drive_request_t)){
			break;
		}

		simulated_drive_request_t *request = (simulated_drive_request_t *)buffer;
//          cout << __FUNCTION__ << "(" <<  __LINE__ << "): " << "Server: Client [" <<  client_socket << "]: " << request->type << endl;

		request->socket_id = client_socket;
        // Dispatch to corresponding handler
        switch (request->type)
        {
        case(SIMULATED_DRIVE_REQUEST_INIT):
            nBytes = handle_init(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_CLEANUP):
            nBytes = handle_cleanup(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_GET_SERVER_PID):
            nBytes = handle_get_server_pid(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_GET_DRIVE_LIST):
            nBytes = handle_get_drive_list(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_OPEN):
            nBytes = handle_open(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_CLOSE):
            nBytes = handle_close(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_CREATE):
            nBytes = handle_create(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_REMOVE):
            nBytes = handle_remove(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_REMOVE_ALL):
            nBytes = handle_remove_all(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_SET_TOTAL_CAPACITY):
            nBytes = handle_set_total_capacity(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_SET_MODE):
            nBytes = handle_set_mode(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_SET_BACKEND_TYPE):
            nBytes = handle_set_backend_type(client_socket, request);
			release_memory(request);
            break;

		case SIMULATED_DRIVE_REQUEST_READ:
			fbe_mutex_lock(&request_queue_lock);
			check_collisions(request);
			fbe_queue_element_init(&request->queue_element);
			fbe_queue_push(&request_queue, &request->queue_element);
			fbe_semaphore_release(&request_queue_semaphore, 0, 1, FALSE);
			fbe_mutex_unlock(&request_queue_lock);
			break;

        case SIMULATED_DRIVE_REQUEST_WRITE:
            /* we need to get data for that request */
			handle_write(request->socket_id, request);

			fbe_mutex_lock(&request_queue_lock);
			check_collisions(request);
			fbe_queue_element_init(&request->queue_element);
			fbe_queue_push(&request_queue, &request->queue_element);
			fbe_semaphore_release(&request_queue_semaphore, 0, 1, FALSE);
			fbe_mutex_unlock(&request_queue_lock);
			break;

		case SIMULATED_DRIVE_REQUEST_WRITE_SAME:
			/* we need to get data for that request */
			handle_write_same(request->socket_id, request);

			fbe_mutex_lock(&request_queue_lock);
			check_collisions(request);
			fbe_queue_element_init(&request->queue_element);
			fbe_queue_push(&request_queue, &request->queue_element);
			fbe_semaphore_release(&request_queue_semaphore, 0, 1, FALSE);
			fbe_mutex_unlock(&request_queue_lock);
			break;

        case(SIMULATED_DRIVE_REQUEST_GET_FILE_SIZE):
            nBytes = handle_get_file_size(client_socket, request);
			release_memory(request);
            break;
        case(SIMULATED_DRIVE_REQUEST_EXIT):
            gQuitFlag = SIMULATED_DRIVE_TRUE;
			release_memory(request);
            break;
        default:
            release_memory(request);
            break;
        }
#if 0
		if(socket_index[0] == client_socket){
			fbe_mutex_unlock(&socket_lock[0]);
		} else {
			fbe_mutex_unlock(&socket_lock[1]);
		}		
#endif
    }

	if(socket_index[0] == client_socket){
		socket_index[0] = 0;
	} else {
		socket_index[1] = 0;
	}

}

static simulated_drive_s32_t stats_exit;
void stats_thread(fbe_thread_user_context_t unref)
{
	cout << "stats_thread Started ..." << endl;

	while(!stats_exit){
		EmcutilSleep(10000);
		wait_mutex(&sp_stats_mutex);
		if(sp_stats[0].is_updated || sp_stats[1].is_updated){
			print_time();
			cout << "SPA " << ": io " << sp_stats[0].io_count << "; w " << sp_stats[0].write_count << "; r " << sp_stats[0].read_count;
			cout << " SPB " << ": io " << sp_stats[1].io_count << "; w " << sp_stats[1].write_count << "; r " << sp_stats[1].read_count << endl;
			cout << "Read collisions: " << read_collisions << endl;
			sp_stats[0].is_updated = 0;
			sp_stats[1].is_updated = 0;
		}
		release_mutex(&sp_stats_mutex);

	}
}

static void 
request_queue_thread_func(void * context)
{
	simulated_drive_request_t * request;

    cout << __FUNCTION__ << " entry ..." << endl;

    while(1) {
		fbe_semaphore_wait(&request_queue_semaphore, NULL);		

		if(request_queue_thread_flag == REQUEST_QUEUE_THREAD_RUN) {
			fbe_mutex_lock(&request_queue_lock);
			request = (simulated_drive_request_t *)fbe_queue_pop(&request_queue);
			fbe_mutex_unlock(&request_queue_lock);
			if(request != NULL){
				//fbe_mutex_lock(&socket_lock);
				//cout << request->socket_id << endl;
				simulated_drive_dispatch_request(request);
				//fbe_mutex_unlock(&socket_lock);
				release_memory(request);
			}
		} else {
			break;
		}
    }

    cout << __FUNCTION__ << " exit" << endl;

    request_queue_thread_flag = REQUEST_QUEUE_THREAD_DONE;
	fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

int
__cdecl
main(int argc, char **argv)
{
    simulated_drive_socket_t server_socket;
    simulated_drive_port_t server_port = SIMULATED_DRIVE_DEFAULT_PORT_NUMBER;
    simulated_drive_s32_t value;
    EMCPAL_STATUS status;

    // set mode
    if(argc > 1)
    {
        for(simulated_drive_s32_t i = 1; i < argc; ++i)
        {
            if(!strcmp("-port", argv[i]))
            {
                i++;
                PRINT_SOCKET_MSG("Server -port ",argv[i])
                value = atoi(argv[i]);
                server_port = (simulated_drive_port_t)value;
                _simulated_drive_set_server_port(server_port);
            }
            else if(!strcmp("perm", argv[i]))
            {
                PRINT_SOCKET_MSG("Server","set mode to permanent")
                _simulated_drive_set_mode(SIMULATED_DRIVE_MODE_PERMANENT);
            }
            else if(!strcmp("-temp", argv[i]))
            {
                PRINT_SOCKET_MSG("Server","set mode to temporary")
                _simulated_drive_set_mode(SIMULATED_DRIVE_MODE_TEMPORARY);
            }
            else if(!strcmp("memory", argv[i]))
            {
                PRINT_SOCKET_MSG("Server","set backup type to memory")
                _simulated_drive_set_backend_type(SIMULATED_DRIVE_BACKEND_TYPE_MEMORY);
            }
            else if(!strcmp("file", argv[i]))
            {
                PRINT_SOCKET_MSG("Server","set backup type to file")
                _simulated_drive_set_backend_type(SIMULATED_DRIVE_BACKEND_TYPE_FILE);
            }
            else if(!strcmp("fake_zeroing_off", argv[i]))
            {
                PRINT_SOCKET_MSG("Server","fake zeroing off")
                _simulated_drive_set_fake_zeroing_read(0);
            }
        }
    }
    /* Start request queue thread's */
    fbe_mutex_init(&socket_lock[0]);
    fbe_mutex_init(&socket_lock[1]);
    fbe_mutex_init(&request_queue_lock);
    fbe_queue_init(&request_queue);
    fbe_semaphore_init(&request_queue_semaphore, 0, 0x0FFFFFFF);
    request_queue_thread_flag = REQUEST_QUEUE_THREAD_RUN;
    fbe_thread_init(&request_queue_thread, "fbe_srv_req", request_queue_thread_func, NULL);
	
    memset(sp_stats,0, 2 * sizeof(stats_t));
    create_mutex(&sp_stats_mutex);

    socket_index[0] = 0;
    socket_index[1] = 0;

    fbe_thread_t ht;
    stats_exit = 0;

    // Connect to the port specified
    server_port = (simulated_drive_s32_t)_simulated_drive_get_server_port();
    server_socket = start_listening(server_port);

    if (server_socket == -1)
    {
        PRINT_SOCKET_ERR_MSG("Server", "listening")
        return 1;
    }

    status = fbe_thread_init(&ht, "fbe_srv_stats", stats_thread, NULL);

    if (EMCPAL_STATUS_SUCCESS != status)
    {
        PRINT_SOCKET_ERR_MSG("Server", "create stats_thread")
        end_server(server_socket);
        return 1;
    }

    status = fbe_thread_init(&Hthread, "fbe_srv_accept", accept_thread,
                             (fbe_thread_user_context_t)&server_socket);

    if (EMCPAL_STATUS_SUCCESS != status)
    {
        PRINT_SOCKET_ERR_MSG("Server", "create accept_thread")
        end_server(server_socket);
        return 1;
    }

    fbe_thread_wait(&Hthread);
    fbe_thread_destroy(&Hthread);

    stats_exit = 1;
    fbe_thread_wait(&ht);
    fbe_thread_destroy(&ht);

    end_server(server_socket);
    PRINT_MSG("Server: Press any key to shut down....")
    getchar();
    destroy_mutex(&sp_stats_mutex);

    request_queue_thread_flag = REQUEST_QUEUE_THREAD_STOP;  
    fbe_semaphore_release(&request_queue_semaphore, 0, 1, FALSE);
    fbe_thread_wait(&request_queue_thread);
    fbe_thread_destroy(&request_queue_thread);

    fbe_semaphore_destroy(&request_queue_semaphore);
    fbe_queue_destroy(&request_queue);
    fbe_mutex_destroy(&request_queue_lock);

    fbe_mutex_destroy(&socket_lock[0]);
    fbe_mutex_destroy(&socket_lock[1]);
	
    return 0;
}

static void check_collisions(simulated_drive_request_t *request)
{
	fbe_queue_element_t * queue_element = NULL;
	fbe_queue_element_t * next_queue_element = NULL;
	fbe_queue_head_t * queue;
	simulated_drive_request_t * current_request;
	simulated_drive_u32_t i = 0;
	simulated_drive_u32_t block_size;

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
			PRINT_MSG("Server: Critical error looped queue....")
			break;
		}

		if(strncmp(current_request->identity, request->identity, sizeof(simulated_drive_identity_t))){
			continue;
		}

		block_size = request->block_size;

		if((request->lba <= current_request->lba) && ((request->lba + (request->int_val / block_size)) > current_request->lba)){
			if((request->type == SIMULATED_DRIVE_REQUEST_READ) && (current_request->type == SIMULATED_DRIVE_REQUEST_READ)){
				read_collisions ++;
			} else {
				request->collision_found++;
				current_request->collision_found++;
				print_collision_info(request, current_request);
			}
			continue;
		}

		if((current_request->lba <= request->lba) && ((current_request->lba + (current_request->int_val / block_size)) > request->lba)){
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
	cout << "Collision " << request->identity;
	if(request->side == 0){
		cout << " SPA ";
	} else {
		cout << " SPB ";
	}
	cout << " LBA " << request->lba << " BC " <<  (request->int_val / request->block_size);
	cout << " Context " << request->context << endl;

	cout << " \t\t With ";
	if(current_request->side == 0){
		cout << " SPA ";
	} else {
		cout << " SPB ";
	}
	
	cout << " LBA " << current_request->lba << " BC " <<  (current_request->int_val / current_request->block_size);
	cout << " Context " << current_request->context << endl << endl;
}
