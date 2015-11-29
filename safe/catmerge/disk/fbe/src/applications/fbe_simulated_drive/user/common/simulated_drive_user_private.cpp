#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ctime>

#define I_AM_NATIVE_CODE
#include <windows.h>

#include "simulated_drive_private_cpp.h"

using namespace std;

// Global variables
simulated_drive_bool_t simulated_drive_is_init;
simulated_drive_file_handle_t simulated_drive_file_handle;
simulated_drive_header_t simulated_drive_header;
simulated_drive_handle_index_t handles[SIMULATED_DRIVE_MAX_NUM * SIMULATED_DRIVE_AVERAGE_OPEN];
simulated_drive_mutext_t handles_mutex;
simulated_drive_mutext_t header_mutex;
simulated_drive_capacity_t simulated_drive_total_capacity_mb;
simulated_drive_mode_e mode = SIMULATED_DRIVE_MODE_PERMANENT;
simulated_drive_port_t server_port = SIMULATED_DRIVE_DEFAULT_PORT_NUMBER;
simulated_drive_backend_type_e backend_type = SIMULATED_DRIVE_BACKEND_TYPE_MEMORY;
simulated_drive_bool_t fake_zeroing_read = TRUE;

/*
 * begin of static function declarations
 */

static void set_up_chunk_linkage(simulated_drive_handle_entry_t *handle, affected_chunks_t *affected_chunks, simulated_drive_chunk_t *previous_chunk,
        simulated_drive_chunk_t *start_chunk, simulated_drive_chunk_t *end_chunk, simulated_drive_chunk_t *next_chunk, simulated_drive_s32_t *overwrite_drive_header);
static void link_with_previous_chunk(simulated_drive_handle_entry_t *handle, simulated_drive_chunk_t *current_chunk,
        simulated_drive_chunk_t *previous_chunk, simulated_drive_s32_t *overwrite_drive_header, simulated_drive_bool_t new_chunk);
static simulated_drive_bool_t allocate_new_chunks(simulated_drive_handle_entry_t *handle, affected_chunks_t *affected_chunks,
        simulated_drive_chunk_t *start_chunk, simulated_drive_chunk_t *end_chunk);
static void release_chunk(simulated_drive_chunk_t * chunk);
static void init_affected_chunks(affected_chunks_t * affected_chunks);
static void destroy_affected_chunks(affected_chunks_t * affected_chunks);

/*
 * end of static function declarations
 */

simulated_drive_bool_t set_file_pointer(simulated_drive_file_handle_t file_handle, simulated_drive_offset_t offset, simulated_drive_offset_t *new_file_location, simulated_drive_u32_t method)
{
    simulated_drive_bool_t result;
    LARGE_INTEGER quadOffset = {0};
    LARGE_INTEGER newOffset = {0};

    quadOffset.QuadPart = offset;

    result = SetFilePointerEx(file_handle, quadOffset, &newOffset, method);
    if(new_file_location != NULL)
    {
        *new_file_location = newOffset.QuadPart;
    }
    return result;
}

simulated_drive_bool_t write_file(simulated_drive_file_handle_t file_handle, void *buffer, simulated_drive_u32_t bytes_to_write, simulated_drive_u32_t *bytes_written)
{
    return WriteFile(file_handle, buffer, bytes_to_write, bytes_written, 0);
}

simulated_drive_bool_t read_file(simulated_drive_file_handle_t file_handle, void *buffer, simulated_drive_u32_t bytes_to_read, simulated_drive_u32_t *bytes_read)
{
    return ReadFile(file_handle, buffer, bytes_to_read, bytes_read, 0);
}

simulated_drive_bool_t close_handle(simulated_drive_file_handle_t file_handle)
{
    return CloseHandle(file_handle);
}

void memory_set(void *mem, simulated_drive_s32_t c, simulated_drive_size_t size)
{
    memset(mem, c, (size_t)size);
}

simulated_drive_s32_t memory_cmp(const void *mem1, const void *mem2, simulated_drive_size_t size)
{
    return memcmp(mem1, mem2, (size_t)size);
}

void *memory_cpy(void *dest, const void *sour, simulated_drive_size_t size)
{
    return memcpy(dest, sour, (size_t)size);
}

void* allocate_memory(simulated_drive_size_t size)
{
    return malloc((size_t)size);
}

void release_memory(void *buffer)
{
    free(buffer);
}

void create_mutex(simulated_drive_mutext_t *mutex)
{
    mutex->mutex = CreateSemaphore(NULL, 1, 1, NULL);
}

void destroy_mutex(simulated_drive_mutext_t *mutex)
{
    CloseHandle(mutex->mutex);
}

void wait_mutex(simulated_drive_mutext_t *mutex)
{
    WaitForSingleObject(mutex->mutex, INFINITE);
}

void release_mutex(simulated_drive_mutext_t *mutex)
{
    LONG retval;
    ReleaseSemaphore(mutex->mutex, 1, &retval);
}

void print_time(void)
{    
    simulated_drive_byte_t tmpbuf[128];

#ifdef ALAMOSA_WINDOWS_ENV
    _tzset();
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

    _strdate(tmpbuf );
    cout << tmpbuf << " ";
    _strtime(tmpbuf );
    cout << tmpbuf << " ";
}

void numeric_to_bytes(simulated_drive_size_t num, simulated_drive_byte_t *output)
{
    simulated_drive_s32_t i, mov = 8 * simulated_drive_size_t_BYTE / simulated_drive_size_t_LEN, mask = 0;
    for (i = 0; i < mov; ++i)
    {
        mask = mask << 1;
        mask |= 1;
    }
    for (i = 0; i < simulated_drive_size_t_LEN; ++i)
    {
        output[i] = (simulated_drive_byte_t)((num >> (mov * i)) & mask);
    }
}

void bytes_to_numeric(simulated_drive_size_t *output, simulated_drive_byte_t *str)
{
    simulated_drive_s32_t i, mov = 8 * simulated_drive_size_t_BYTE / simulated_drive_size_t_LEN;
    *output = 0;
    for (i = simulated_drive_size_t_LEN - 1; i >= 0 ; --i)
    {
        *output = ((*output) << mov);
        *output |= str[i];
    }
}

void set_mode(simulated_drive_mode_e server_mode)
{
    mode = server_mode;
}

void set_server_port(simulated_drive_port_t new_server_port)
{
    server_port = new_server_port;
    return;
}

simulated_drive_port_t get_server_port(void)
{
    return server_port;
}

void set_backend_type(simulated_drive_backend_type_e type)
{
    backend_type = type;
}

void set_fake_zeroing_read(simulated_drive_bool_t flag)
{
    fake_zeroing_read = flag;
}

simulated_drive_mode_e get_mode()
{
    return mode;
}

simulated_drive_bool_t delete_simulated_drive_item(simulated_drive_header_t *header, simulated_drive_u64_t index)
{
    simulated_drive_u64_t i;
    simulated_drive_chunk_t *temp, *temp2;

    wait_mutex(&header_mutex);
    if (index < (simulated_drive_s32_t)header->drive_num)
    {
        temp = header->index[index].chunk_list_head;
        while (temp != NULL)
        {
            temp2 = temp->next_chunk;
            release_chunk(temp);
            temp = temp2;
        }
        destroy_mutex(&(header->index[index].io_mutext));
        for (i = index; i < header->drive_num - 1; ++i)
        {
            memory_cpy(header->index[i].drive_identity, header->index[i + 1].drive_identity, sizeof(simulated_drive_identity_t));
            header->index[i].data_size = header->index[i + 1].data_size;
            header->index[i].first_chunk_location = header->index[i + 1].first_chunk_location;
            header->index[i].drive_block_size = header->index[i + 1].drive_block_size;
            header->index[i].max_lba = header->index[i + 1].max_lba;
            header->index[i].chunk_list_head = header->index[i + 1].chunk_list_head;
            header->index[i].chunk_list_end = header->index[i + 1].chunk_list_end;
        }
        --header->drive_num;
        memory_set(header->index[header->drive_num].drive_identity, '\0', sizeof(simulated_drive_identity_t));
        header->index[header->drive_num].data_size = 0;
        header->index[header->drive_num].first_chunk_location = 0;
        header->index[header->drive_num].drive_block_size = 0;
        header->index[header->drive_num].max_lba = -1;
        header->index[header->drive_num].chunk_list_head = 0;
        header->index[header->drive_num].chunk_list_end = 0;
        header->index[header->drive_num].io_mutext.mutex = NULL;
        release_mutex(&header_mutex);
        return TRUE;
    }
    release_mutex(&header_mutex);
    return FALSE;
}

simulated_drive_bool_t delete_all_simulated_drive(simulated_drive_header_t *header)
{
    simulated_drive_s32_t i;
    simulated_drive_chunk_t *temp, *temp2;

    wait_mutex(&header_mutex);
    /* clear all drive handles */
    for (i = 0; i < (simulated_drive_s32_t)header->drive_num; ++i)
    {
        memory_set(header->index[i].drive_identity, '\0', sizeof(simulated_drive_identity_t));
        header->index[i].data_size = 0;
        header->index[i].first_chunk_location = 0;
        header->index[i].drive_block_size = 0;
        header->index[i].max_lba = -1;
        temp = header->index[i].chunk_list_head;
        while (temp != NULL)
        {
            temp2 = temp->next_chunk;
            release_chunk(temp);
            temp = temp2;
        }
        header->index[i].chunk_list_head = 0;
        header->index[i].chunk_list_end = 0;
        destroy_mutex(&(header->index[i].io_mutext));
        header->index[i].io_mutext.mutex = NULL;
    }
    header->drive_num = 0;
    release_mutex(&header_mutex);

    if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
    {
        /* recreate the real drive file */
        close_handle(simulated_drive_file_handle);

        simulated_drive_file_handle = CreateFile(drive_file_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                                                 0, NULL);

        if (write_simulated_drive_header(simulated_drive_file_handle, &simulated_drive_header) == SIMULATED_DRIVE_INVALID_SIZE)
        {
            REPORT_DETAIL_AND_RETURN_VALUE("write drive header failed", FALSE)
        }
    }
    return TRUE;
}

simulated_drive_bool_t add_simulated_drive_item(simulated_drive_header_t *header, simulated_drive_identity_t drive_identity, simulated_drive_size_t drive_block_size, simulated_drive_size_t max_lba)
{
    simulated_drive_size_t i;

    wait_mutex(&header_mutex);
    if (header->drive_num >= SIMULATED_DRIVE_MAX_NUM)
    {
        release_mutex(&header_mutex);
        return FALSE;
    }

    for (i = 0; i < simulated_drive_header.drive_num; ++i)
    {
        if (!strncmp(header->index[i].drive_identity, drive_identity, sizeof(simulated_drive_identity_t)))
        {
            break;
        }
    }
    if (i >= simulated_drive_header.drive_num)
    {
        memory_set(header->index[header->drive_num].drive_identity, '\0', sizeof(simulated_drive_identity_t));
        memory_cpy(header->index[header->drive_num].drive_identity, drive_identity, sizeof(simulated_drive_identity_t));
        header->index[header->drive_num].data_size = 0;
        header->index[header->drive_num].first_chunk_location = 0;
        header->index[header->drive_num].drive_block_size = drive_block_size;
        header->index[header->drive_num].max_lba = max_lba;
        header->index[header->drive_num].chunk_list_head = 0;
        header->index[header->drive_num].chunk_list_end = 0;
        create_mutex(&(header->index[header->drive_num].io_mutext));
        ++header->drive_num;
        release_mutex(&header_mutex);
        return TRUE;
    }
    release_mutex(&header_mutex);
    return FALSE;
}

simulated_drive_bool_t find_simulated_drive_item(simulated_drive_header_t *header, simulated_drive_identity_t drive_identity, simulated_drive_u64_t *index)
{
    simulated_drive_u64_t i;
    wait_mutex(&header_mutex);
    for (i = 0; i < header->drive_num; ++i)
    {
        if (!strncmp(header->index[i].drive_identity, drive_identity, sizeof(simulated_drive_identity_t)))
        {
            *index = i;
            release_mutex(&header_mutex);
            return TRUE;
        }
    }
    release_mutex(&header_mutex);
    return FALSE;
}

void fill_in_blank(void *container, simulated_drive_size_t capability, void *data, simulated_drive_size_t size)
{
    memory_set(container, 0, capability);
    memory_cpy(container, data, size);
}

// generate zeroing pattern to buffer according to the address
void generate_valid_zero_buffer(simulated_drive_byte_t * data_buffer_pointer, simulated_drive_lba_t lba, simulated_drive_size_t block_count, simulated_drive_size_t block_size)
{
    simulated_drive_u16_t byte_offset_520 = 0, metadata_offset;
    simulated_drive_u64_t last_lba, zero_metadata = 0x7fff5eed;

    /* Fill out the data buffer with valid zero buffer. */
    last_lba = lba + block_count;
    while(lba < last_lba)
    {
        memory_set(data_buffer_pointer, 0, block_size);
        byte_offset_520 = (simulated_drive_u16_t) (((simulated_drive_u64_t)lba * block_size) % 520);
        if(byte_offset_520 != 0)
        {
            metadata_offset = (simulated_drive_s16_t)block_size - byte_offset_520;
            data_buffer_pointer += metadata_offset;
            memory_cpy(data_buffer_pointer, (void *) &zero_metadata, 8);
            data_buffer_pointer += block_size - metadata_offset;
        }
        else
        {
            data_buffer_pointer += 512;
            if(!(block_size % 520))
            {
                memory_cpy(data_buffer_pointer, (void *) &zero_metadata, 8);
                data_buffer_pointer += 8;
            }
        }
        lba++;
    }
}

simulated_drive_size_t get_file_size(simulated_drive_file_handle_t handle)
{
    LARGE_INTEGER size = {0};

    if (!GetFileSizeEx(handle, &size))
    {
        REPORT_GENERAL_AND_RETURN_VALUE(SIMULATED_DRIVE_INVALID_SIZE)
    }
    return size.QuadPart;
}

simulated_drive_size_t write_simulated_drive_chunk(simulated_drive_file_handle_t handle, simulated_drive_chunk_t *chunk, simulated_drive_lba_t begin_location)
{
    simulated_drive_u32_t bytesWritten;
    simulated_drive_byte_t temp_string[simulated_drive_size_t_LEN], *container;


    if (set_file_pointer(handle, begin_location, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    numeric_to_bytes(chunk->header.drive_address, temp_string);
    if (!write_file(handle, temp_string, simulated_drive_size_t_LEN, &bytesWritten))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    numeric_to_bytes(chunk->header.previous_location, temp_string);
    if (!write_file(handle, temp_string, simulated_drive_size_t_LEN, &bytesWritten))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    numeric_to_bytes(chunk->header.next_location, temp_string);
    if (!write_file(handle, temp_string, simulated_drive_size_t_LEN, &bytesWritten))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    numeric_to_bytes(chunk->header.data_size, temp_string);
    if (!write_file(handle, temp_string, simulated_drive_size_t_LEN, &bytesWritten))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    numeric_to_bytes(chunk->header.chunk_size, temp_string);
    if (!write_file(handle, temp_string, simulated_drive_size_t_LEN, &bytesWritten))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    numeric_to_bytes(chunk->header.repeat_count, temp_string);
    if (!write_file(handle, temp_string, simulated_drive_size_t_LEN, &bytesWritten))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }

    container = (simulated_drive_byte_t *)allocate_memory(chunk->header.chunk_size);
    if (container == NULL)
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    fill_in_blank(container, chunk->header.data_size, chunk->content, chunk->header.data_size);
    if (!write_file(handle, container, (simulated_drive_u32_t)chunk->header.data_size, &bytesWritten))
    {
        release_memory(container);
        return SIMULATED_DRIVE_INVALID_SIZE;
    }

    release_memory(container);
    return bytesWritten;
}

simulated_drive_size_t read_simulated_drive_chunk_header(simulated_drive_file_handle_t handle, simulated_drive_chunk_header_t *chunk_header, simulated_drive_lba_t begin_location)
{
    simulated_drive_u32_t nbytesRead;
    simulated_drive_byte_t temp_string[simulated_drive_size_t_LEN];

    if (set_file_pointer(handle, begin_location, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    if (!read_file(handle, temp_string, simulated_drive_size_t_LEN, &nbytesRead))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    bytes_to_numeric(&(chunk_header->drive_address), temp_string);
    if (!read_file(handle, temp_string, simulated_drive_size_t_LEN, &nbytesRead))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    bytes_to_numeric(&(chunk_header->previous_location), temp_string);
    if (!read_file(handle, temp_string, simulated_drive_size_t_LEN, &nbytesRead))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    bytes_to_numeric(&(chunk_header->next_location), temp_string);
    if (!read_file(handle, temp_string, simulated_drive_size_t_LEN, &nbytesRead))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    bytes_to_numeric(&(chunk_header->data_size), temp_string);
    if (!read_file(handle, temp_string, simulated_drive_size_t_LEN, &nbytesRead))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    bytes_to_numeric(&(chunk_header->chunk_size), temp_string);
    if (!read_file(handle, temp_string, simulated_drive_size_t_LEN, &nbytesRead))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    bytes_to_numeric(&(chunk_header->repeat_count), temp_string);

    return nbytesRead;
}

simulated_drive_size_t write_simulated_drive_header(simulated_drive_file_handle_t handle, simulated_drive_header_t *header)
{
    simulated_drive_size_t nbytesWritten = 0, i;
    simulated_drive_u32_t tempBytes = 0;
    simulated_drive_byte_t temp_num[simulated_drive_size_t_LEN], fill[SIMULATED_DRIVE_HEADER_ITEM_SIZE];

    if (set_file_pointer(handle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    fill_in_blank(fill, SIMULATED_DRIVE_HEADER_ITEM_SIZE, fill, 0);
    wait_mutex(&header_mutex);
    numeric_to_bytes(header->drive_num, temp_num);
    if (!write_file(handle, temp_num, simulated_drive_size_t_LEN, &tempBytes))
    {
        release_mutex(&header_mutex);
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    nbytesWritten += tempBytes;
    for (i = 0; i < SIMULATED_DRIVE_MAX_NUM; ++i)
    {
        if (i < header->drive_num)
        {
            if (!write_file(handle, header->index[i].drive_identity, sizeof(simulated_drive_identity_t), &tempBytes))
            {
                release_mutex(&header_mutex);
                return SIMULATED_DRIVE_INVALID_SIZE;
            }
            nbytesWritten += tempBytes;
            numeric_to_bytes(header->index[i].max_lba, temp_num);
            if (!write_file(handle, temp_num, simulated_drive_size_t_LEN, &tempBytes))
            {
                release_mutex(&header_mutex);
                return SIMULATED_DRIVE_INVALID_SIZE;
            }
            nbytesWritten += tempBytes;
            numeric_to_bytes(header->index[i].drive_block_size, temp_num);
            if (!write_file(handle, temp_num, simulated_drive_size_t_LEN, &tempBytes))
            {
                release_mutex(&header_mutex);
                return SIMULATED_DRIVE_INVALID_SIZE;
            }
            nbytesWritten += tempBytes;
            numeric_to_bytes(header->index[i].data_size, temp_num);
            if (!write_file(handle, temp_num, simulated_drive_size_t_LEN, &tempBytes))
            {
                release_mutex(&header_mutex);
                return SIMULATED_DRIVE_INVALID_SIZE;
            }
            nbytesWritten += tempBytes;
            numeric_to_bytes(header->index[i].first_chunk_location, temp_num);
            if (!write_file(handle, temp_num, simulated_drive_size_t_LEN, &tempBytes))
            {
                release_mutex(&header_mutex);
                return SIMULATED_DRIVE_INVALID_SIZE;
            }
            nbytesWritten += tempBytes;
        }
        else
        {
            if (!write_file(handle, fill, SIMULATED_DRIVE_HEADER_ITEM_SIZE, &tempBytes))
            {
                release_mutex(&header_mutex);
                return SIMULATED_DRIVE_INVALID_SIZE;
            }
        }
    }
    release_mutex(&header_mutex);
    return nbytesWritten;
}

simulated_drive_size_t read_simulated_drive_header(simulated_drive_file_handle_t handle, simulated_drive_header_t *header)
{
    simulated_drive_u32_t nbytesRead = 0, tempBytes, i;
    simulated_drive_byte_t temp_num[simulated_drive_size_t_LEN];

    if (set_file_pointer(handle, FILE_BEGIN, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    wait_mutex(&header_mutex);
    if (!read_file(handle, temp_num, simulated_drive_size_t_LEN, &tempBytes))
    {
        release_mutex(&header_mutex);
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    else if (tempBytes == 0)
    {
        header->drive_num = 0;
        nbytesRead = 0;
    }
    else
    {
        nbytesRead += tempBytes;
        bytes_to_numeric(&(header->drive_num), temp_num);
        for (i = 0; i < header->drive_num; ++i)
        {
            if (!read_file(handle, header->index[i].drive_identity, sizeof(simulated_drive_identity_t), &tempBytes))
            {
                release_mutex(&header_mutex);
                return SIMULATED_DRIVE_INVALID_SIZE;
            }
            nbytesRead += tempBytes;
            if (!read_file(handle, temp_num, simulated_drive_size_t_LEN, &tempBytes))
            {
                release_mutex(&header_mutex);
                return SIMULATED_DRIVE_INVALID_SIZE;
            }
            nbytesRead += tempBytes;
            bytes_to_numeric(&(header->index[i].max_lba), temp_num);
            if (!read_file(handle, temp_num, simulated_drive_size_t_LEN, &tempBytes))
            {
                release_mutex(&header_mutex);
                return SIMULATED_DRIVE_INVALID_SIZE;
            }
            nbytesRead += tempBytes;
            bytes_to_numeric(&(header->index[i].drive_block_size), temp_num);
            if (!read_file(handle, temp_num, simulated_drive_size_t_LEN, &tempBytes))
            {
                release_mutex(&header_mutex);
                return SIMULATED_DRIVE_INVALID_SIZE;
            }
            nbytesRead += tempBytes;
            bytes_to_numeric(&(header->index[i].data_size), temp_num);
            if (!read_file(handle, temp_num, simulated_drive_size_t_LEN, &tempBytes))
            {
                release_mutex(&header_mutex);
                return SIMULATED_DRIVE_INVALID_SIZE;
            }
            nbytesRead += tempBytes;
            bytes_to_numeric(&(header->index[i].first_chunk_location), temp_num);

            create_mutex(&(header->index[i].io_mutext));
        }
    }
    release_mutex(&header_mutex);
    return nbytesRead;
}

simulated_drive_size_t read_simulated_drive_chunk_content(simulated_drive_file_handle_t handle, void *buffer, simulated_drive_lba_t begin_location, simulated_drive_size_t nbytes_to_read)
{
    simulated_drive_u32_t nbytesRead;

    if (set_file_pointer(handle, begin_location + SIMULATED_DRIVE_CHUNK_CONTENT_OFFSET, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }
    if (!read_file(handle, buffer, (simulated_drive_u32_t)nbytes_to_read, &nbytesRead))
    {
        return SIMULATED_DRIVE_INVALID_SIZE;
    }

    return nbytesRead;
}

simulated_drive_bool_t write_simulated_drive_chunk_previous(simulated_drive_file_handle_t handle, simulated_drive_lba_t begin_location, simulated_drive_lba_t previous)
{
    simulated_drive_u32_t nbytesWrite;
    simulated_drive_byte_t temp_num[simulated_drive_size_t_LEN];

    if (set_file_pointer(handle, begin_location + SIMULATED_DRIVE_CHUNK_PREVIOUS_OFFSET, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return FALSE;
    }
    numeric_to_bytes(previous, temp_num);
    if (!write_file(handle, temp_num, SIMULATED_DRIVE_LOCATION_LEN, &nbytesWrite))
    {
        return FALSE;
    }

    return TRUE;
}

simulated_drive_bool_t write_simulated_drive_chunk_next(simulated_drive_file_handle_t handle, simulated_drive_lba_t begin_location, simulated_drive_lba_t next)
{
    simulated_drive_u32_t nbytesWrite;
    simulated_drive_byte_t temp_num[simulated_drive_size_t_LEN];

    if (set_file_pointer(handle, begin_location + SIMULATED_DRIVE_CHUNK_NEXT_OFFSET, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return FALSE;
    }
    numeric_to_bytes(next, temp_num);
    if (!write_file(handle, temp_num, SIMULATED_DRIVE_LOCATION_LEN, &nbytesWrite))
    {
        return FALSE;
    }

    return TRUE;
}

simulated_drive_bool_t write_simulated_drive_chunk_data_size(simulated_drive_file_handle_t handle, simulated_drive_lba_t begin_location, simulated_drive_size_t size)
{
    simulated_drive_u32_t nbytesWrite;
    simulated_drive_byte_t temp_num[simulated_drive_size_t_LEN];

    if (set_file_pointer(handle, begin_location + SIMULATED_DRIVE_CHUNK_DATA_SIZE_OFFSET, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return FALSE;
    }
    numeric_to_bytes(size, temp_num);
    if (!write_file(handle, temp_num, SIMULATED_DRIVE_LOCATION_LEN, &nbytesWrite))
    {
        return FALSE;
    }

    return TRUE;
}

simulated_drive_bool_t write_simulated_drive_chunk_chunk_size(simulated_drive_file_handle_t handle, simulated_drive_lba_t begin_location, simulated_drive_size_t size)
{
    simulated_drive_u32_t nbytesWrite;
    simulated_drive_byte_t temp_num[simulated_drive_size_t_LEN];

    if (set_file_pointer(handle, begin_location + SIMULATED_DRIVE_CHUNK_CHUNK_SIZE_OFFSET, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return FALSE;
    }
    numeric_to_bytes(size, temp_num);
    if (!write_file(handle, temp_num, SIMULATED_DRIVE_LOCATION_LEN, &nbytesWrite))
    {
        return FALSE;
    }

    return TRUE;
}

simulated_drive_bool_t write_simulated_drive_chunk_repeat_count(simulated_drive_file_handle_t handle, simulated_drive_lba_t begin_location, simulated_drive_size_t repeat_count)
{
    simulated_drive_u32_t nbytesWrite;
    simulated_drive_byte_t temp_num[simulated_drive_size_t_LEN];

    if (set_file_pointer(handle, begin_location + SIMULATED_DRIVE_CHUNK_REPEAT_COUNT_OFFSET, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return FALSE;
    }
    numeric_to_bytes(repeat_count, temp_num);
    if (!write_file(handle, temp_num, SIMULATED_DRIVE_LOCATION_LEN, &nbytesWrite))
    {
        return FALSE;
    }

    return TRUE;
}

simulated_drive_bool_t write_simulated_drive_chunk_content(simulated_drive_file_handle_t handle, simulated_drive_lba_t begin_location, void *buffer, simulated_drive_size_t length)
{
    simulated_drive_u32_t nbytesWrite;

    if (set_file_pointer(handle, begin_location + SIMULATED_DRIVE_CHUNK_CONTENT_OFFSET, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return FALSE;
    }
    if (!write_file(handle, buffer, (simulated_drive_u32_t)length, &nbytesWrite))
    {
        return FALSE;
    }

    return TRUE;
}

simulated_drive_bool_t write_simulated_drive_chunk_drive_address(simulated_drive_file_handle_t handle, simulated_drive_lba_t begin_location, simulated_drive_lba_t address)
{
    simulated_drive_u32_t nbytesWrite;
    simulated_drive_byte_t temp_num[simulated_drive_size_t_LEN];

    if (set_file_pointer(handle, begin_location + SIMULATED_DRIVE_CHUNK_DRIVE_ADDRESS_OFFSET, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        return FALSE;
    }
    numeric_to_bytes(address, temp_num);
    if (!write_file(handle, temp_num, SIMULATED_DRIVE_LOCATION_LEN, &nbytesWrite))
    {
        return FALSE;
    }

    return TRUE;
}

// read information from simulated drive header and the drive file, build drive chunk index with chunk linked list
simulated_drive_bool_t build_index(simulated_drive_file_handle_t handle, simulated_drive_header_t *header)
{
    simulated_drive_size_t i;
    simulated_drive_chunk_t *chunk;
    simulated_drive_lba_t file_location;

    wait_mutex(&header_mutex);
    for (i = 0; i < header->drive_num; ++i)
    {
        if (header->index[i].first_chunk_location <= 0)
        {
            header->index[i].chunk_list_head = NULL;
            header->index[i].chunk_list_end = NULL;
        }
        else
        {
            file_location = header->index[i].first_chunk_location;
            do
            {
                chunk = (simulated_drive_chunk_t *)allocate_memory(sizeof(simulated_drive_chunk_t));
                if (chunk == NULL)
                {
                    release_mutex(&header_mutex);
                    return FALSE;
                }
                if (read_simulated_drive_chunk_header(handle, &chunk->header, file_location) ==  SIMULATED_DRIVE_INVALID_SIZE)
                {
                    release_chunk(chunk);
                    release_mutex(&header_mutex);
                    return FALSE;
                }
                if (header->index[i].chunk_list_head <= 0)
                {
                    chunk->previous_chunk = 0;
                    chunk->next_chunk = 0;
                    header->index[i].chunk_list_head = chunk;
                    header->index[i].chunk_list_end = chunk;
                }
                else
                {
                    header->index[i].chunk_list_end->next_chunk = chunk;
                    chunk->previous_chunk = header->index[i].chunk_list_end;
                    chunk->next_chunk = 0;
                    header->index[i].chunk_list_end = chunk;
                }
                chunk->header.current_location = file_location;
                file_location = chunk->header.next_location;
                chunk->content = NULL;
            } while (file_location > 0);
        }
    }
    release_mutex(&header_mutex);
    return TRUE;
}

simulated_drive_bool_t destory_index(simulated_drive_header_t *header)
{
    simulated_drive_size_t i;
    simulated_drive_chunk_t *chunk, *temp;

    wait_mutex(&header_mutex);
    for (i = 0; i < header->drive_num; ++i)
    {
        if (header->index[i].chunk_list_head != NULL)
        {
            for (chunk = header->index[i].chunk_list_head; chunk != header->index[i].chunk_list_end;)
            {
                temp = chunk;
                chunk = chunk->next_chunk;
                release_chunk(temp);
            }
            release_chunk(chunk);
        }
        header->index[i].chunk_list_head = NULL;
        header->index[i].chunk_list_end = NULL;
    }
    release_mutex(&header_mutex);

    return TRUE;
}


simulated_drive_handle_t find_valid_handle(void)
{
    simulated_drive_s32_t i;

    for (i = 0; i< SIMULATED_DRIVE_MAX_NUM * SIMULATED_DRIVE_AVERAGE_OPEN; ++i)
    {
        if (!handles[i].used)
        {
            return i;
        }
    }

    return SIMULATED_DRIVE_INVALID_HANDLE;
}

simulated_drive_bool_t is_drive_opened(simulated_drive_identity_t drive_identity)
{
    simulated_drive_s32_t i;

    wait_mutex(&handles_mutex);
    for (i = 0; i< SIMULATED_DRIVE_MAX_NUM * SIMULATED_DRIVE_AVERAGE_OPEN; ++i)
    {
        if (handles[i].used && !strncmp(handles[i].handle.drive_identity, drive_identity, sizeof(simulated_drive_identity_t)))
        {
            release_mutex(&handles_mutex);
            return TRUE;
        }
    }
    release_mutex(&handles_mutex);

    return FALSE;
}

simulated_drive_bool_t is_need_compress(simulated_drive_u64_t repeat_count)
{
    return (repeat_count >= SIMULATED_DRIVE_WRITE_SAME_COMPRESS_THRESHOLD);
}

// set the ptr of the simulated drive handle to the location indicates by offset and whence.
// offset can be negative, and whence is: 0 - file begin; 1 - current; 2 - file end.
simulated_drive_bool_t lseek_simulated_drive(simulated_drive_handle_t file_handle, simulated_drive_offset_t offset, simulated_drive_u32_t whence)
{
    simulated_drive_chunk_t *chunk, *previous_chunk;
    simulated_drive_handle_entry_t *handle = &(handles[file_handle].handle);
    simulated_drive_bool_t find = FALSE;

    // set ptr the start location according to whence first
    if (whence == 0)
    {
        handle->drive_address = 0;
    }
    else if (whence == 2)
    {
        if (simulated_drive_header.index[handle->header_index].chunk_list_end > 0)
        {
            /* TODO: using block_size here is not accurate. */
            handle->drive_address = simulated_drive_header.index[handle->header_index].chunk_list_end->header.drive_address + simulated_drive_header.index[handle->header_index].chunk_list_end->header.chunk_size;
        }
        else
        {
            handle->drive_address = 0;
        }
    }
    handle->drive_address += offset * simulated_drive_header.index[handle->header_index].drive_block_size;
    if (handle->drive_address < 0 || handle->drive_address / simulated_drive_header.index[handle->header_index].drive_block_size> simulated_drive_header.index[handle->header_index].max_lba)
    {
        handle->current_chunk = NULL;
        REPORT_DETAIL_AND_RETURN_VALUE("seeking over the boundary", FALSE)
    }

    chunk = simulated_drive_header.index[handle->header_index].chunk_list_head;
    previous_chunk = NULL;
    while (chunk != NULL)
    {
        if (chunk->header.drive_address <= handle->drive_address)
        {
            if (chunk->header.drive_address + chunk->header.chunk_size > handle->drive_address)
            {
                find = TRUE;
                break;
            }
        }
        else
        {
            break;
        }
        previous_chunk = chunk;
        chunk = chunk->next_chunk;
    }
    if (find)
    {
        handle->current_chunk = chunk;
    }
    else
    {
        handle->current_chunk = previous_chunk;
    }
    return TRUE;
}

simulated_drive_lba_t locate_file_end(simulated_drive_handle_entry_t *handle)
{
    simulated_drive_lba_t file_end;

    // set file ptr
    if ((file_end = get_file_size(handle->file_handle)) == SIMULATED_DRIVE_INVALID_SIZE)
    {
        REPORT_DETAIL_AND_RETURN_VALUE("get file size failed", SIMULATED_DRIVE_INVALID_SIZE)
    }
    if (set_file_pointer(handle->file_handle, file_end, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        REPORT_DETAIL_AND_RETURN_VALUE("set file ptr failed", SIMULATED_DRIVE_INVALID_SIZE)
    }
    return file_end;
}

simulated_drive_bool_t check_simulated_drive_capacity(simulated_drive_lba_t location)
{
    if ((simulated_drive_capacity_t)location / (1024 * 1024) > simulated_drive_total_capacity_mb)
    {
        cout << (simulated_drive_capacity_t)location / (1024 * 1024) <<  " MB exceeds the max file size: " << simulated_drive_total_capacity_mb << " MB." << endl;
        REPORT_GENERAL_AND_RETURN_VALUE(FALSE)
    }
    return TRUE;
}


simulated_drive_bool_t update_simulated_drive_chunk_repeat_count(simulated_drive_file_handle_t handle, simulated_drive_chunk_t *chunk, simulated_drive_size_t repeat_count)
{
    if(chunk->header.repeat_count != repeat_count)
    {
        chunk->header.repeat_count = repeat_count;
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            return write_simulated_drive_chunk_repeat_count(handle, chunk->header.current_location, repeat_count);
        }
    }
    return TRUE;
}

simulated_drive_bool_t update_simulated_drive_chunk_chunk_size(simulated_drive_file_handle_t handle, simulated_drive_chunk_t *chunk, simulated_drive_size_t size)
{
    if(chunk->header.chunk_size != size)
    {
        chunk->header.chunk_size = size;
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            return write_simulated_drive_chunk_chunk_size(handle, chunk->header.current_location, size);
        }
    }
    return TRUE;
}

simulated_drive_bool_t update_simulated_drive_chunk_drive_address(simulated_drive_file_handle_t handle, simulated_drive_chunk_t *chunk, simulated_drive_lba_t address)
{
    if(chunk->header.drive_address != address)
    {
        chunk->header.drive_address = address;
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            return write_simulated_drive_chunk_drive_address(handle, chunk->header.current_location, address);
        }
    }
    return TRUE;
}

simulated_drive_bool_t update_simulated_drive_chunk_previous(simulated_drive_file_handle_t handle, simulated_drive_chunk_t *current_chunk, simulated_drive_chunk_t *previous_chunk)
{
    simulated_drive_lba_t loc = (previous_chunk == NULL) ? 0 : previous_chunk->header.current_location;
    if(current_chunk->previous_chunk != previous_chunk || current_chunk->header.previous_location != loc)
    {
        current_chunk->previous_chunk = previous_chunk;
        current_chunk->header.previous_location = loc;
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            return write_simulated_drive_chunk_previous(handle, current_chunk->header.current_location, current_chunk->header.previous_location);
        }
    }
    return TRUE;
}

simulated_drive_bool_t update_simulated_drive_chunk_next(simulated_drive_file_handle_t handle, simulated_drive_chunk_t *current_chunk, simulated_drive_chunk_t *next_chunk)
{
    simulated_drive_lba_t loc = (next_chunk == NULL) ? 0 : next_chunk->header.current_location;
    if(current_chunk->next_chunk != next_chunk || current_chunk->header.next_location != loc)
    {
        current_chunk->next_chunk = next_chunk;
        current_chunk->header.next_location = loc;
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            return write_simulated_drive_chunk_next(handle, current_chunk->header.current_location, current_chunk->header.next_location);
        }
    }
    return TRUE;
}

simulated_drive_bool_t update_chunk_list_head(simulated_drive_u64_t header_index, simulated_drive_chunk_t *chunk)
{
    wait_mutex(&header_mutex);
    if(simulated_drive_header.index[header_index].chunk_list_end == NULL)
    {
        simulated_drive_header.index[header_index].chunk_list_end = chunk;
        if(simulated_drive_header.index[header_index].chunk_list_end != NULL)
        {
            simulated_drive_header.index[header_index].chunk_list_end->next_chunk = NULL;
            simulated_drive_header.index[header_index].chunk_list_end->header.next_location = 0;
        }
    }
    simulated_drive_header.index[header_index].chunk_list_head = chunk;
    simulated_drive_header.index[header_index].first_chunk_location =
            simulated_drive_header.index[header_index].chunk_list_head == NULL ? 0 : simulated_drive_header.index[header_index].chunk_list_head->header.current_location;
    if(simulated_drive_header.index[header_index].chunk_list_head != NULL)
    {
        simulated_drive_header.index[header_index].chunk_list_head->previous_chunk = NULL;
        simulated_drive_header.index[header_index].chunk_list_head->header.previous_location = 0;
    }
    release_mutex(&header_mutex);
    return TRUE;
}

simulated_drive_bool_t update_chunk_list_end(simulated_drive_u64_t header_index, simulated_drive_chunk_t *chunk)
{
    wait_mutex(&header_mutex);
    if(simulated_drive_header.index[header_index].chunk_list_head == NULL)
    {
        simulated_drive_header.index[header_index].chunk_list_head = chunk;
        simulated_drive_header.index[header_index].first_chunk_location =
                simulated_drive_header.index[header_index].chunk_list_head == NULL ? 0 : simulated_drive_header.index[header_index].chunk_list_head->header.current_location;
        if(simulated_drive_header.index[header_index].chunk_list_head != NULL)
        {
            simulated_drive_header.index[header_index].chunk_list_head->previous_chunk = NULL;
            simulated_drive_header.index[header_index].chunk_list_head->header.previous_location = 0;
        }
    }
    simulated_drive_header.index[header_index].chunk_list_end = chunk;
    if(simulated_drive_header.index[header_index].chunk_list_end != NULL)
    {
        simulated_drive_header.index[header_index].chunk_list_end->next_chunk = NULL;
        simulated_drive_header.index[header_index].chunk_list_end->header.next_location = 0;
    }
    release_mutex(&header_mutex);
    return TRUE;
}

simulated_drive_bool_t update_chunk_list_head_end_for_deleted_chunk(simulated_drive_file_handle_t handle, simulated_drive_u64_t header_index, simulated_drive_chunk_t *deleted_chunk)
{
    simulated_drive_bool_t overwrite_drive_header = FALSE;

    if(deleted_chunk == NULL)
    {
        return FALSE;
    }

    wait_mutex(&header_mutex);
    if(simulated_drive_header.index[header_index].chunk_list_head == deleted_chunk)
    {
        simulated_drive_header.index[header_index].chunk_list_head = deleted_chunk->next_chunk;
        simulated_drive_header.index[header_index].first_chunk_location =
                simulated_drive_header.index[header_index].chunk_list_head == NULL ? 0 : simulated_drive_header.index[header_index].chunk_list_head->header.current_location;
        overwrite_drive_header = TRUE;
        if(simulated_drive_header.index[header_index].chunk_list_head != NULL)
        {
            update_simulated_drive_chunk_previous(handle, simulated_drive_header.index[header_index].chunk_list_head, NULL);
        }
    }
    if(simulated_drive_header.index[header_index].chunk_list_end == deleted_chunk)
    {
        simulated_drive_header.index[header_index].chunk_list_end = deleted_chunk->previous_chunk;
        overwrite_drive_header = TRUE;
        if(simulated_drive_header.index[header_index].chunk_list_end != NULL)
        {
            update_simulated_drive_chunk_next(handle, simulated_drive_header.index[header_index].chunk_list_end, NULL);
        }
    }
    release_mutex(&header_mutex);
    return overwrite_drive_header;
}

static void init_affected_chunks(affected_chunks_t * affected_chunks)
{
    affected_chunks->start_chunk_repeat.buffer = NULL;
    affected_chunks->start_chunk_repeat.start_address = SIMULATED_DRIVE_INVALID_SIZE;
    affected_chunks->start_chunk_repeat.length = 0;
    affected_chunks->start_chunk_repeat.repeat_count = 0;
    affected_chunks->start_chunk_repeat.chunk_pointer = NULL;

    affected_chunks->start_chunk_piece.buffer = NULL;
    affected_chunks->start_chunk_piece.start_address = SIMULATED_DRIVE_INVALID_SIZE;
    affected_chunks->start_chunk_piece.length = 0;
    affected_chunks->start_chunk_piece.repeat_count = 0;
    affected_chunks->start_chunk_piece.chunk_pointer = NULL;

    affected_chunks->new_data.buffer = NULL;
    affected_chunks->new_data.start_address = SIMULATED_DRIVE_INVALID_SIZE;
    affected_chunks->new_data.length = 0;
    affected_chunks->new_data.repeat_count = 0;
    affected_chunks->new_data.chunk_pointer = NULL;

    affected_chunks->end_chunk_piece.buffer = NULL;
    affected_chunks->end_chunk_piece.start_address = SIMULATED_DRIVE_INVALID_SIZE;
    affected_chunks->end_chunk_piece.length = 0;
    affected_chunks->end_chunk_piece.repeat_count = 0;
    affected_chunks->end_chunk_piece.chunk_pointer = NULL;

    affected_chunks->end_chunk_repeat.buffer = NULL;
    affected_chunks->end_chunk_repeat.start_address = SIMULATED_DRIVE_INVALID_SIZE;
    affected_chunks->end_chunk_repeat.length = 0;
    affected_chunks->end_chunk_repeat.repeat_count = 0;
    affected_chunks->end_chunk_repeat.chunk_pointer = NULL;
}

static void destroy_affected_chunks(affected_chunks_t * affected_chunks)
{
    release_memory(affected_chunks->start_chunk_repeat.buffer);
    affected_chunks->start_chunk_repeat.buffer = NULL;
    affected_chunks->start_chunk_repeat.start_address = SIMULATED_DRIVE_INVALID_SIZE;
    affected_chunks->start_chunk_repeat.length = 0;
    affected_chunks->start_chunk_repeat.repeat_count = 0;
    affected_chunks->start_chunk_repeat.chunk_pointer = NULL;

    release_memory(affected_chunks->start_chunk_piece.buffer);
    affected_chunks->start_chunk_piece.buffer = NULL;
    affected_chunks->start_chunk_piece.start_address = SIMULATED_DRIVE_INVALID_SIZE;
    affected_chunks->start_chunk_piece.length = 0;
    affected_chunks->start_chunk_piece.repeat_count = 0;
    affected_chunks->start_chunk_piece.chunk_pointer = NULL;

    release_memory(affected_chunks->new_data.buffer);
    affected_chunks->new_data.buffer = NULL;
    affected_chunks->new_data.start_address = SIMULATED_DRIVE_INVALID_SIZE;
    affected_chunks->new_data.length = 0;
    affected_chunks->new_data.repeat_count = 0;
    affected_chunks->new_data.chunk_pointer = NULL;

    release_memory(affected_chunks->end_chunk_piece.buffer);
    affected_chunks->end_chunk_piece.buffer = NULL;
    affected_chunks->end_chunk_piece.start_address = SIMULATED_DRIVE_INVALID_SIZE;
    affected_chunks->end_chunk_piece.length = 0;
    affected_chunks->end_chunk_piece.repeat_count = 0;
    affected_chunks->end_chunk_piece.chunk_pointer = NULL;

    release_memory(affected_chunks->end_chunk_repeat.buffer);
    affected_chunks->end_chunk_repeat.buffer = NULL;
    affected_chunks->end_chunk_repeat.start_address = SIMULATED_DRIVE_INVALID_SIZE;
    affected_chunks->end_chunk_repeat.length = 0;
    affected_chunks->end_chunk_repeat.repeat_count = 0;
    affected_chunks->end_chunk_repeat.chunk_pointer = NULL;
}

/*
 * This function get the chunk context for the new data.
 *
 * handle
 * end_lba: end lba address, we don't need start lba explicitely because we can get it from handle
 * previous_chunk: chunk right before the new data, it is NULL when new data is before all existing chunks
 * start_chunk: starting address of new chunk falls into start_chunk, if new data starts at unallocated address it is set to NULL
 * end_chunk: ending address of new chunk falls into end_chunk, if new data ends at unallocated address it is set to NULL
 * next_chunk: chunk right after the new data, it is NULL when new data is after all existing chunks
 */
void get_new_data_context(simulated_drive_handle_entry_t *handle, simulated_drive_lba_t end_lba, simulated_drive_chunk_t **previous_chunk,
        simulated_drive_chunk_t **start_chunk, simulated_drive_chunk_t **end_chunk, simulated_drive_chunk_t **next_chunk)
{
    /*
     * current_chunk == NULL means the data should be in a new chunk and inserted before all existing chunks.
     * end_chunk is initialized to head of the header list.
     */
    if (handle->current_chunk == NULL)
    {
        *start_chunk = NULL;
        *previous_chunk = NULL;
        *end_chunk = simulated_drive_header.index[handle->header_index].chunk_list_head;
    }
    /*
     * else if starting address of new data does not fall into existing chunks and should be inserted after the current chunk,
     * set start_chunk to NULL and previous_chunk to current chunk in handle. end_chunk is also initialized to current_chunk.
     */
    else if(handle->drive_address >= handle->current_chunk->header.drive_address + handle->current_chunk->header.chunk_size)
    {
        *start_chunk = NULL;
        *previous_chunk = handle->current_chunk;
        *end_chunk = handle->current_chunk;
    }
    /*
     * else it falls into an existing chunk.
     */
    else
    {
        *start_chunk = handle->current_chunk;
        *previous_chunk = handle->current_chunk->previous_chunk;
        *end_chunk = handle->current_chunk;
    }

    /* initialize next_chunk to NULL. */
    *next_chunk = NULL;

    /*
     * start to go through the chunk header list to locate correct end_chunk and next_chunk
     */
    while(*end_chunk)
    {
        /*
         * if the end address of new data is smaller than the current end_chunk, means it will not fall in existing chunks.
         * set next_chunk to end_chunk and end_chunk to NULL and break.
         */
        if(end_lba * simulated_drive_header.index[handle->header_index].drive_block_size < (*end_chunk)->header.drive_address)
        {
            *next_chunk = *end_chunk;
            *end_chunk = NULL;
            break;
        }
        /*
         * else if end address of new data if bigger than the end address of end_chunk, mean we should look at the next chunk.
         * set end_chunk to its successor and continue.
         */
        else if(end_lba * simulated_drive_header.index[handle->header_index].drive_block_size >= (*end_chunk)->header.drive_address + (*end_chunk)->header.chunk_size)
        {
            *end_chunk = (*end_chunk)->next_chunk;
        }
        /*
         * else end_lba falls in end_chunk. set next_chunk to the successor of end_chunk.
         */
        else
        {
            *next_chunk = (*end_chunk)->next_chunk;
            break;
        }
    }
}

// link current chunk with previous one, and set up related information
static void link_with_previous_chunk(simulated_drive_handle_entry_t *handle, simulated_drive_chunk_t *current_chunk,
        simulated_drive_chunk_t *previous_chunk, simulated_drive_s32_t *overwrite_drive_header, simulated_drive_bool_t new_chunk)
{
    if(current_chunk != NULL)
    {
        if(new_chunk)
        {
            if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
            {
                /* write to the file end */
                current_chunk->header.current_location = locate_file_end(handle);
            }
            /* if previous_header_pointer is not NULL, link with it */
            if(previous_chunk != NULL)
            {
                update_simulated_drive_chunk_next(handle->file_handle, previous_chunk, current_chunk);
                current_chunk->previous_chunk = previous_chunk;
                current_chunk->header.previous_location = previous_chunk->header.current_location;
            }
            /* else it should be the head of all chunks */
            else
            {
                current_chunk->previous_chunk = 0;
                current_chunk->header.previous_location = 0;
                if(update_chunk_list_head(handle->header_index, current_chunk))
                {
                    ++(*overwrite_drive_header);
                }
            }
        }
        /* if it is existing chunk */
        else
        {
            if(previous_chunk != NULL)
            {
                update_simulated_drive_chunk_next(handle->file_handle, previous_chunk, current_chunk);
                update_simulated_drive_chunk_previous(handle->file_handle, current_chunk, previous_chunk);
            }
            /* previous_header is NULL means current chunk should be the first chunk in the linked list */
            else
            {
                current_chunk->previous_chunk = 0;
                current_chunk->header.previous_location = 0;
                /* update the list head */
                if(update_chunk_list_head(handle->header_index, current_chunk))
                {
                    ++(*overwrite_drive_header);
                }
            }
        }
    }
    /* if current_header is NULL, previous_header should be the last one in the linked list */
    else
    {
        if(previous_chunk != NULL)
        {
            update_simulated_drive_chunk_next(handle->file_handle, previous_chunk, NULL);
            if(update_chunk_list_end(handle->header_index, previous_chunk))
            {
                ++(*overwrite_drive_header);
            }
        }
    }
}

/*
 *  Description: set up chunk linkage for existing chunks and their neighborings.
 *
 *  Inputs: handle - simulated drive handle.
 *             affected_chunks - chunk headers need to be inserted or modified.
 *             previous_chunk - the chunk header which is before the starting chunk header, could be NULL.
 *             start_chunk - the starting chunk header which new data write to, could be NULL.
 *             end_chunk - the ending chunk header which new data write to, could be NULL.
 *             next_chunk - the chunk header which is after the starting chunk header, could be NULL.
 *             overwrite_drive_header - indicates whether the chunk list head or end need to be changed in the simulated drive header.
 */
static void set_up_chunk_linkage(simulated_drive_handle_entry_t *handle, affected_chunks_t *affected_chunks, simulated_drive_chunk_t *previous_chunk,
        simulated_drive_chunk_t *start_chunk, simulated_drive_chunk_t *end_chunk, simulated_drive_chunk_t *next_chunk, simulated_drive_s32_t *overwrite_drive_header)
{
    /* ptr which point to the previous chunk header */
    simulated_drive_chunk_t *previous_chunk_pointer = NULL;
    simulated_drive_bool_t free_start_chunk = FALSE;

    /* if the remaining repeat count in start_chunk is bigger than 0, update the repeat_count and chunk_size fields */
    if(affected_chunks->start_chunk_repeat.repeat_count > 0)
    {
        update_simulated_drive_chunk_repeat_count(handle->file_handle, start_chunk, affected_chunks->start_chunk_repeat.repeat_count);
        update_simulated_drive_chunk_chunk_size(handle->file_handle, start_chunk, start_chunk->header.repeat_count * start_chunk->header.data_size);
        previous_chunk_pointer = start_chunk;
    }
    /*
     * else means no or only part of the data pattern in start_chunk will remain, release start_chunk in memory
     * and update chunk list head/end accordingly.
     */
    else
    {
        if(start_chunk != NULL)
        {
            if(update_chunk_list_head_end_for_deleted_chunk(handle->file_handle, handle->header_index, start_chunk))
            {
                ++(*overwrite_drive_header);
            }
            free_start_chunk = TRUE;
        }
        previous_chunk_pointer = previous_chunk;
    }

    /* if there is part of the data pattern in start_chunk should remain before new data, create new chunk for it */
    if(affected_chunks->start_chunk_piece.repeat_count > 0)
    {
        link_with_previous_chunk(handle, affected_chunks->start_chunk_piece.chunk_pointer, previous_chunk_pointer, overwrite_drive_header, TRUE);
        /* write the new chunk to the drive file */
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            write_simulated_drive_chunk(handle->file_handle, affected_chunks->start_chunk_piece.chunk_pointer, affected_chunks->start_chunk_piece.chunk_pointer->header.current_location);
        }
        previous_chunk_pointer = affected_chunks->start_chunk_piece.chunk_pointer;
    }

    if(affected_chunks->new_data.repeat_count > 0)
    {
        link_with_previous_chunk(handle, affected_chunks->new_data.chunk_pointer, previous_chunk_pointer, overwrite_drive_header, TRUE);
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            write_simulated_drive_chunk(handle->file_handle, affected_chunks->new_data.chunk_pointer, affected_chunks->new_data.chunk_pointer->header.current_location);
        }
        previous_chunk_pointer = affected_chunks->new_data.chunk_pointer;
    }

    /* if there is part of the data pattern in end_chunk should remain after new data, create new chunk for it */
    if(affected_chunks->end_chunk_piece.repeat_count > 0)
    {
        link_with_previous_chunk(handle, affected_chunks->end_chunk_piece.chunk_pointer, previous_chunk_pointer, overwrite_drive_header, TRUE);
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            write_simulated_drive_chunk(handle->file_handle, affected_chunks->end_chunk_piece.chunk_pointer, affected_chunks->end_chunk_piece.chunk_pointer->header.current_location);
        }
        previous_chunk_pointer = affected_chunks->end_chunk_piece.chunk_pointer;
    }

    /* if the remaining repeat count in end_chunk is bigger than 0, update the repeat_count and chunk_size filed */
    if(affected_chunks->end_chunk_repeat.repeat_count > 0)
    {
        /* if new data falls into a single chunk, new chunk should be allocated */
        if(start_chunk == end_chunk)
        {
            link_with_previous_chunk(handle, affected_chunks->end_chunk_repeat.chunk_pointer, previous_chunk_pointer, overwrite_drive_header, TRUE);
            /* now connect all previous chunks with next_chunk */
            if(next_chunk != NULL)
            {
                affected_chunks->end_chunk_repeat.chunk_pointer->next_chunk = next_chunk;
                affected_chunks->end_chunk_repeat.chunk_pointer->header.next_location = next_chunk->header.current_location;
                update_simulated_drive_chunk_previous(handle->file_handle, next_chunk, affected_chunks->end_chunk_repeat.chunk_pointer);
            }
            else
            {
                if(update_chunk_list_end(handle->header_index, affected_chunks->end_chunk_repeat.chunk_pointer))
                {
                    ++(*overwrite_drive_header);
                }
            }
            if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
            {
                write_simulated_drive_chunk(handle->file_handle, affected_chunks->end_chunk_repeat.chunk_pointer,
                        affected_chunks->end_chunk_repeat.chunk_pointer->header.current_location);
            }
        }
        /* else just update end_chunk fields */
        else
        {
            update_simulated_drive_chunk_repeat_count(handle->file_handle, end_chunk, affected_chunks->end_chunk_repeat.repeat_count);
            update_simulated_drive_chunk_chunk_size(handle->file_handle, end_chunk, end_chunk->header.repeat_count * end_chunk->header.data_size);
            update_simulated_drive_chunk_drive_address(handle->file_handle, end_chunk, affected_chunks->end_chunk_repeat.start_address);
            update_simulated_drive_chunk_previous(handle->file_handle, end_chunk, previous_chunk_pointer);
            update_simulated_drive_chunk_next(handle->file_handle, previous_chunk_pointer, end_chunk);
            if(simulated_drive_header.index[handle->header_index].chunk_list_end == NULL || simulated_drive_header.index[handle->header_index].chunk_list_end->header.drive_address <= end_chunk->header.drive_address)
            {
                if(update_chunk_list_end(handle->header_index, end_chunk))
                {
                    ++(*overwrite_drive_header);
                }
            }
        }
    }
    /* else the remaining repeat count in end_chunk is 0, abandon end_chunk */
    else
    {
        /* delete end_chunk only if it is not a single chunk, otherwise the chunk is also used by affected_chunks->start_chunk_repeat */
        if(end_chunk != NULL && end_chunk != start_chunk)
        {
            if(update_chunk_list_head_end_for_deleted_chunk(handle->file_handle, handle->header_index, end_chunk))
            {
                ++(*overwrite_drive_header);
            }
            release_chunk(end_chunk);
        }
        /* now connect all previous chunks with next_chunk */
        if(next_chunk != NULL)
        {
            update_simulated_drive_chunk_next(handle->file_handle, previous_chunk_pointer, next_chunk);
            update_simulated_drive_chunk_previous(handle->file_handle, next_chunk, previous_chunk_pointer);
        }
        else
        {
            update_simulated_drive_chunk_next(handle->file_handle, previous_chunk_pointer, NULL);
            if(update_chunk_list_end(handle->header_index, previous_chunk_pointer))
            {
                ++(*overwrite_drive_header);
            }
        }
    }
    if(free_start_chunk)
    {
        release_chunk(start_chunk);
    }
}

/* this function reads information in affected_chunks and allocates necessary chunks. */
static simulated_drive_bool_t allocate_new_chunks(simulated_drive_handle_entry_t *handle, affected_chunks_t *affected_chunks,
        simulated_drive_chunk_t *start_chunk, simulated_drive_chunk_t *end_chunk)
{
    /* we don't need to allocate content buffer for it because it can use the content buffer of start_chunk_repeat */
    if(affected_chunks->start_chunk_piece.repeat_count > 0)
    {
        affected_chunks->start_chunk_piece.chunk_pointer = (simulated_drive_chunk_t *)allocate_memory(sizeof(simulated_drive_chunk_t));
        if (affected_chunks->start_chunk_piece.chunk_pointer == NULL)
        {
            destroy_affected_chunks(affected_chunks);
            REPORT_DETAIL_AND_RETURN_VALUE("memory allocation for chunk failed", FALSE)
        }
        affected_chunks->start_chunk_piece.chunk_pointer->header.chunk_size = affected_chunks->start_chunk_piece.length;
        affected_chunks->start_chunk_piece.chunk_pointer->header.data_size = affected_chunks->start_chunk_piece.length;
        /* because it's part of data pattern, repeat_count must be 1 */
        affected_chunks->start_chunk_piece.chunk_pointer->header.repeat_count = 1;
        affected_chunks->start_chunk_piece.chunk_pointer->header.drive_address = affected_chunks->start_chunk_piece.start_address;
        affected_chunks->start_chunk_piece.chunk_pointer->content = affected_chunks->start_chunk_piece.buffer;
    }
    if(affected_chunks->new_data.repeat_count > 0)
    {
        affected_chunks->new_data.chunk_pointer = (simulated_drive_chunk_t *)allocate_memory(sizeof(simulated_drive_chunk_t));
        if (affected_chunks->new_data.chunk_pointer == NULL)
        {
            destroy_affected_chunks(affected_chunks);
            REPORT_DETAIL_AND_RETURN_VALUE("memory allocation for chunk failed", FALSE)
        }
        affected_chunks->new_data.chunk_pointer->header.chunk_size = affected_chunks->new_data.length * affected_chunks->new_data.repeat_count;
        affected_chunks->new_data.chunk_pointer->header.data_size = affected_chunks->new_data.length;
        affected_chunks->new_data.chunk_pointer->header.repeat_count = affected_chunks->new_data.repeat_count;
        affected_chunks->new_data.chunk_pointer->header.drive_address = affected_chunks->new_data.start_address;
        affected_chunks->new_data.chunk_pointer->content = affected_chunks->new_data.buffer;
    }
    /* we don't need to allocate content buffer for it because it can use the content buffer of end_chunk_repeat */
    if(affected_chunks->end_chunk_piece.repeat_count > 0)
    {
        affected_chunks->end_chunk_piece.chunk_pointer = (simulated_drive_chunk_t *)allocate_memory(sizeof(simulated_drive_chunk_t));
        if (affected_chunks->end_chunk_piece.chunk_pointer == NULL)
        {
            destroy_affected_chunks(affected_chunks);
            REPORT_DETAIL_AND_RETURN_VALUE("memory allocation for chunk failed", FALSE)
        }
        affected_chunks->end_chunk_piece.chunk_pointer->header.chunk_size = affected_chunks->end_chunk_piece.length;
        affected_chunks->end_chunk_piece.chunk_pointer->header.data_size = affected_chunks->end_chunk_piece.length;
        /* because it's part of data pattern, repeat_count must be 1 */
        affected_chunks->end_chunk_piece.chunk_pointer->header.repeat_count = 1;
        affected_chunks->end_chunk_piece.chunk_pointer->header.drive_address = affected_chunks->end_chunk_piece.start_address;
        affected_chunks->end_chunk_piece.chunk_pointer->content = affected_chunks->end_chunk_piece.buffer;
    }
    /* we only need to allocate for it when new data falls in a single chunk */
    if(affected_chunks->end_chunk_repeat.repeat_count > 0 && start_chunk == end_chunk)
    {
        affected_chunks->end_chunk_repeat.chunk_pointer = (simulated_drive_chunk_t *)allocate_memory(sizeof(simulated_drive_chunk_t));
        if (affected_chunks->end_chunk_repeat.chunk_pointer == NULL)
        {
            destroy_affected_chunks(affected_chunks);
            REPORT_DETAIL_AND_RETURN_VALUE("memory allocation for chunk failed", FALSE)
        }
        affected_chunks->end_chunk_repeat.chunk_pointer->header.repeat_count = affected_chunks->end_chunk_repeat.repeat_count;
        affected_chunks->end_chunk_repeat.chunk_pointer->header.data_size = end_chunk->header.data_size;
        affected_chunks->end_chunk_repeat.chunk_pointer->header.chunk_size = end_chunk->header.data_size * affected_chunks->end_chunk_repeat.repeat_count;
        affected_chunks->end_chunk_repeat.chunk_pointer->header.drive_address = affected_chunks->end_chunk_repeat.start_address;
        /* we don't allocate content buffer for it before because it's a special case, allocate now */
        if(affected_chunks->end_chunk_repeat.buffer == NULL)
        {
            affected_chunks->end_chunk_repeat.buffer = (simulated_drive_byte_t *)allocate_memory(end_chunk->header.data_size);
            if (affected_chunks->end_chunk_repeat.buffer == NULL)
            {
                destroy_affected_chunks(affected_chunks);
                REPORT_DETAIL_AND_RETURN_VALUE("memory allocation for end_chunk buffer failed", FALSE)
            }
            if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
            {
                if (read_simulated_drive_chunk_content(handle->file_handle, affected_chunks->end_chunk_repeat.buffer, end_chunk->header.current_location, end_chunk->header.data_size) == SIMULATED_DRIVE_INVALID_SIZE)
                {
                    destroy_affected_chunks(affected_chunks);
                    REPORT_DETAIL_AND_RETURN_VALUE("read block content failed", FALSE)
                }
            }
            else if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_MEMORY)
            {
                if(end_chunk->content == NULL)
                {
                    destroy_affected_chunks(affected_chunks);
                                    REPORT_DETAIL_AND_RETURN_VALUE("read block content failed", FALSE)
                }
                memory_cpy(affected_chunks->end_chunk_repeat.buffer, end_chunk->content, end_chunk->header.data_size);
            }
        }
        affected_chunks->end_chunk_repeat.chunk_pointer->content = affected_chunks->end_chunk_repeat.buffer;
    }

    return TRUE;
}

/* this function reads information in affected_chunks and merges new data with existing data to save space if possible. */
static simulated_drive_bool_t check_and_merge_new_data(affected_chunks_t *affected_chunks)
{
    if(!is_need_compress(affected_chunks->new_data.repeat_count)
            && (affected_chunks->start_chunk_piece.repeat_count == 1 || affected_chunks->end_chunk_piece.repeat_count == 1))
    {
        simulated_drive_byte_t *temp_pointer = affected_chunks->new_data.buffer;
        affected_chunks->new_data.buffer = (simulated_drive_byte_t *)allocate_memory(affected_chunks->start_chunk_piece.length
                + affected_chunks->end_chunk_piece.length + affected_chunks->new_data.length);
        if (affected_chunks->new_data.buffer == NULL)
        {
            destroy_affected_chunks(affected_chunks);
            release_memory(temp_pointer);
            REPORT_DETAIL_AND_RETURN_VALUE("memory allocation for new chunk buffer failed", FALSE)
        }
        if(affected_chunks->start_chunk_piece.length > 0)
        {
            memory_cpy(affected_chunks->new_data.buffer, affected_chunks->start_chunk_piece.buffer, affected_chunks->start_chunk_piece.length);
        }
        memory_cpy(affected_chunks->new_data.buffer + affected_chunks->start_chunk_piece.length, temp_pointer, affected_chunks->new_data.length);
        if(affected_chunks->end_chunk_piece.length > 0)
        {
            memory_cpy(affected_chunks->new_data.buffer + affected_chunks->start_chunk_piece.length + affected_chunks->new_data.length,
                    affected_chunks->end_chunk_piece.buffer, affected_chunks->end_chunk_piece.length);
        }
        /* set the length and address */
        affected_chunks->new_data.length = affected_chunks->start_chunk_piece.length + affected_chunks->end_chunk_piece.length
                + affected_chunks->new_data.length;
        if(affected_chunks->start_chunk_piece.repeat_count == 1)
        {
            affected_chunks->new_data.start_address = affected_chunks->start_chunk_piece.start_address;
        }
        /* abandon two pieces */
        affected_chunks->start_chunk_piece.repeat_count = 0;
        affected_chunks->end_chunk_piece.repeat_count = 0;
        release_memory(affected_chunks->start_chunk_piece.buffer);
        affected_chunks->start_chunk_piece.buffer = NULL;
        release_memory(affected_chunks->end_chunk_piece.buffer);
        affected_chunks->end_chunk_piece.buffer = NULL;
        /* release original new_data buffer */
        release_memory(temp_pointer);
    }

    return TRUE;
}

/* destroy all chunks between start_chunk and end_chunk */
static void destroy_chunks_between_start_end(simulated_drive_handle_entry_t *handle, simulated_drive_chunk_t *previous_chunk,
        simulated_drive_chunk_t *start_chunk, simulated_drive_chunk_t *end_chunk, simulated_drive_chunk_t *next_chunk,
        simulated_drive_s32_t *overwrite_drive_header)
{
    simulated_drive_chunk_t *destroy_head = NULL, *destroy_end = NULL;
    if(start_chunk == NULL)
    {
        /* new chunk is before all chunks */
        if(previous_chunk == NULL)
        {
            destroy_head = simulated_drive_header.index[handle->header_index].chunk_list_head;
        }
        else
        {
            destroy_head = previous_chunk->next_chunk;
        }
    }
    else
    {
        destroy_head = start_chunk->next_chunk;
    }
    if(end_chunk == NULL)
    {
        /* new chunk is after all chunks */
        if(next_chunk == NULL)
        {
            destroy_end = simulated_drive_header.index[handle->header_index].chunk_list_end;
        }
        else
        {
            destroy_end = next_chunk->previous_chunk;
        }
    }
    else
    {
        destroy_end = end_chunk->previous_chunk;
    }
    if(destroy_head != NULL && ((destroy_end == NULL) || ((destroy_end != NULL) && destroy_end->header.drive_address < destroy_head->header.drive_address)))
    {
        destroy_head = NULL;
    }

    if(destroy_head != NULL && destroy_end != NULL && destroy_end->header.drive_address > destroy_head->header.drive_address)
    {
        for(simulated_drive_chunk_t *destroy_item = destroy_head; destroy_item != destroy_end && destroy_item != NULL;)
        {
            simulated_drive_chunk_t *temp_item = destroy_item;
            destroy_item = destroy_item->next_chunk;
            release_chunk(temp_item);
        }
        if(destroy_end != NULL)
        {
            release_chunk(destroy_end);
        }

        if(start_chunk == NULL)
        {
            /* new chunk is before all chunks */
            if(previous_chunk == NULL)
            {
                if(update_chunk_list_head(handle->header_index, (end_chunk == NULL) ? end_chunk : end_chunk))
                {
                    ++(*overwrite_drive_header);
                }
            }
            else
            {
                update_simulated_drive_chunk_next(handle->file_handle, previous_chunk, (end_chunk == NULL) ? end_chunk : end_chunk);
            }
        }
        else
        {
            update_simulated_drive_chunk_next(handle->file_handle, start_chunk, (end_chunk == NULL) ? end_chunk : end_chunk);
        }
        if(end_chunk == NULL)
        {
            /* new chunk is after all chunks */
            if(next_chunk == NULL)
            {
                if(update_chunk_list_end(handle->header_index, (start_chunk == NULL) ? start_chunk : start_chunk))
                {
                    ++(*overwrite_drive_header);
                }
            }
            else
            {
                update_simulated_drive_chunk_previous(handle->file_handle, next_chunk, (start_chunk == NULL) ? start_chunk : start_chunk);
            }
        }
        else
        {
            update_simulated_drive_chunk_previous(handle->file_handle, end_chunk, (start_chunk == NULL) ? start_chunk : start_chunk);
        }
    }
}

simulated_drive_size_t insert_new_data(simulated_drive_handle_entry_t *handle, void *buffer, simulated_drive_u64_t nblocks_to_write, simulated_drive_u64_t repeat_count,
        simulated_drive_chunk_t *previous_chunk, simulated_drive_chunk_t *start_chunk, simulated_drive_chunk_t *end_chunk, simulated_drive_chunk_t *next_chunk)
{
    simulated_drive_u64_t start_remaining_repeat = 0; // how many repeat count in start chunk will remain after new data overwriting
    simulated_drive_u64_t start_remaining_piece = 0;   // how long in start chunk will remain besides repeat parts
    simulated_drive_u64_t end_remaining_repeat = 0;   // how many repeat count in end chunk will remain after new data overwriting
    simulated_drive_u64_t end_remaining_piece = 0;     // how long in end chunk will remain besides repeat parts
    simulated_drive_size_t drive_block_size = simulated_drive_header.index[handle->header_index].drive_block_size;
    simulated_drive_s32_t overwrite_drive_header = 0;

    /* set start_remaining_repeat and start_remaining_piece */
    if(start_chunk == NULL)
    {
        start_remaining_repeat = 0;
        start_remaining_piece = 0;
    }
    else
    {
        start_remaining_repeat = (handle->drive_address - start_chunk->header.drive_address) / start_chunk->header.data_size;
        start_remaining_piece = (handle->drive_address - start_chunk->header.drive_address) % start_chunk->header.data_size;
    }

    /* set end_remaining_repeat and end_remaining_piece */
    if(end_chunk == NULL)
    {
        end_remaining_repeat = 0;
        end_remaining_piece = 0;
    }
    else
    {
        end_remaining_repeat = (end_chunk->header.drive_address + end_chunk->header.chunk_size - handle->drive_address - repeat_count * nblocks_to_write * drive_block_size) / end_chunk->header.data_size;
        end_remaining_piece = (end_chunk->header.drive_address + end_chunk->header.chunk_size - handle->drive_address - repeat_count * nblocks_to_write * drive_block_size) % end_chunk->header.data_size;
    }

    /* destroy all chunks between start_chunk and end_chunk, they will not survive anyway */
    destroy_chunks_between_start_end(handle, previous_chunk, start_chunk, end_chunk, next_chunk, &overwrite_drive_header);

    /* insertion of new data will affect at most 5 chunks with their content. */
    affected_chunks_t affected_chunks;

    /* initialize affected_chunks first */
    init_affected_chunks(&affected_chunks);

    /*
     * the only case we don't need to allocate a new chunk is that [new data fall into a single existing chunk]
     * && [new data does not need to be compressed] && [the repeat count of start(end)_chunk is 1].
     */
    if(start_chunk == end_chunk && start_chunk != NULL && !is_need_compress(repeat_count) && start_chunk->header.repeat_count == 1)
    {
        affected_chunks.start_chunk_repeat.buffer = (simulated_drive_byte_t *)allocate_memory(start_chunk->header.data_size);
        if (affected_chunks.start_chunk_repeat.buffer == NULL)
        {
            destroy_affected_chunks(&affected_chunks);
            REPORT_DETAIL_AND_RETURN_VALUE("memory allocation for start_chunk buffer failed", SIMULATED_DRIVE_INVALID_SIZE)
        }
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            if (read_simulated_drive_chunk_content(handle->file_handle, affected_chunks.start_chunk_repeat.buffer, start_chunk->header.current_location, start_chunk->header.data_size) == SIMULATED_DRIVE_INVALID_SIZE)
            {
                destroy_affected_chunks(&affected_chunks);
                REPORT_DETAIL_AND_RETURN_VALUE("read block content failed", SIMULATED_DRIVE_INVALID_SIZE)
            }
        }
        else if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_MEMORY)
        {
            if(start_chunk->content == NULL)
            {
                destroy_affected_chunks(&affected_chunks);
                                REPORT_DETAIL_AND_RETURN_VALUE("read block content failed", SIMULATED_DRIVE_INVALID_SIZE)
            }
            memory_cpy(affected_chunks.start_chunk_repeat.buffer, start_chunk->content, start_chunk->header.data_size);
        }

        /* overwrite new data to the chunk buffer */
        simulated_drive_byte_t *temp_pointer = affected_chunks.start_chunk_repeat.buffer + start_remaining_piece;
        for(simulated_drive_s32_t i = 0; i < repeat_count; ++i)
        {
            memory_cpy(temp_pointer, buffer, nblocks_to_write * drive_block_size);
            temp_pointer += nblocks_to_write * drive_block_size;
        }
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            write_simulated_drive_chunk_content(handle->file_handle, start_chunk->header.current_location, affected_chunks.start_chunk_repeat.buffer, start_chunk->header.data_size);
        }
        affected_chunks.start_chunk_repeat.length = start_chunk->header.data_size;
        affected_chunks.start_chunk_repeat.repeat_count = 1;
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_MEMORY && start_chunk->content != NULL)
        {
            /* free abandoned data and assign the new one */
            release_memory(start_chunk->content);
            start_chunk->content = affected_chunks.start_chunk_repeat.buffer;
        }
    }
    /* a new chunk is needed */
    else
    {
        //TODO: how to check in memory version
        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            /* capacity checking */
            if (!check_simulated_drive_capacity( locate_file_end(handle)
                    + (is_need_compress(repeat_count) ? 1 : repeat_count) * nblocks_to_write * drive_block_size + SIMULATED_DRIVE_CHUNK_CONTENT_OFFSET))
            {
                REPORT_DETAIL_AND_RETURN_VALUE("exceed max file size", SIMULATED_DRIVE_INVALID_SIZE)
            }
        }
        /*
         * set affected_chunks.start_chunk_repeat if start_remaining_repeat is greater than 0.
         * we don't need to read its content because it does not change.
         */
        if(start_remaining_repeat > 0)
        {
            affected_chunks.start_chunk_repeat.repeat_count = start_remaining_repeat;
        }

        /* set affected_chunks.start_chunk_piece if start_remaining_piece is greater than 0. */
        if(start_remaining_piece > 0)
        {
            affected_chunks.start_chunk_piece.buffer = (simulated_drive_byte_t *)allocate_memory(start_chunk->header.data_size);
            if (affected_chunks.start_chunk_piece.buffer == NULL)
            {
                destroy_affected_chunks(&affected_chunks);
                REPORT_DETAIL_AND_RETURN_VALUE("memory allocation for start_chunk buffer failed", SIMULATED_DRIVE_INVALID_SIZE)
            }
            if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
            {
                if (read_simulated_drive_chunk_content(handle->file_handle, affected_chunks.start_chunk_piece.buffer, start_chunk->header.current_location, start_chunk->header.data_size) == SIMULATED_DRIVE_INVALID_SIZE)
                {
                    destroy_affected_chunks(&affected_chunks);
                    REPORT_DETAIL_AND_RETURN_VALUE("read block content failed", SIMULATED_DRIVE_INVALID_SIZE)
                }
            }
            else if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_MEMORY)
            {
                if(start_chunk->content == NULL)
                {
                    destroy_affected_chunks(&affected_chunks);
                                    REPORT_DETAIL_AND_RETURN_VALUE("read block content failed", SIMULATED_DRIVE_INVALID_SIZE)
                }
                memory_cpy(affected_chunks.start_chunk_piece.buffer, start_chunk->content, start_chunk->header.data_size);
            }
            affected_chunks.start_chunk_piece.length = start_remaining_piece;
            affected_chunks.start_chunk_piece.repeat_count = 1;
            affected_chunks.start_chunk_piece.start_address = start_chunk->header.drive_address + start_remaining_repeat * start_chunk->header.data_size;
        }

        /* set affected_chunks.end_chunk_piece if end_remaining_piece is greater than 0. */
        if(end_remaining_piece > 0)
        {
            /*
             * because it's a part of end_chunk content we will store the buffer in affected_chunks.end_chunk_repeat.buffer.
             */
            simulated_drive_byte_t *temp_content = (simulated_drive_byte_t *)allocate_memory(end_chunk->header.data_size);
            affected_chunks.end_chunk_piece.buffer = (simulated_drive_byte_t *)allocate_memory(end_remaining_piece);
            if (affected_chunks.end_chunk_piece.buffer == NULL)
            {
                destroy_affected_chunks(&affected_chunks);
                REPORT_DETAIL_AND_RETURN_VALUE("memory allocation for end_chunk buffer failed", SIMULATED_DRIVE_INVALID_SIZE)
            }
            if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
            {
                if (read_simulated_drive_chunk_content(handle->file_handle, temp_content, end_chunk->header.current_location, end_chunk->header.data_size) == SIMULATED_DRIVE_INVALID_SIZE)
                {
                    destroy_affected_chunks(&affected_chunks);
                    REPORT_DETAIL_AND_RETURN_VALUE("read block content failed", SIMULATED_DRIVE_INVALID_SIZE)
                }
            }
            else if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_MEMORY)
            {
                if(end_chunk->content == NULL)
                {
                    destroy_affected_chunks(&affected_chunks);
                                    REPORT_DETAIL_AND_RETURN_VALUE("read block content failed", SIMULATED_DRIVE_INVALID_SIZE)
                }
                memory_cpy(temp_content, end_chunk->content, end_chunk->header.data_size);
            }
            memory_cpy(affected_chunks.end_chunk_piece.buffer, temp_content + end_chunk->header.data_size - end_remaining_piece, end_remaining_piece);
            release_memory(temp_content);
            affected_chunks.end_chunk_piece.length = end_remaining_piece;
            affected_chunks.end_chunk_piece.repeat_count = 1;
            affected_chunks.end_chunk_piece.start_address = end_chunk->header.drive_address + end_chunk->header.chunk_size - end_remaining_repeat * end_chunk->header.data_size - end_remaining_piece;
        }

        /*
         * set affected_chunks.end_chunk_repeat if end_remaining_repeat is greater than 0.
         * we don't need to read its content because it does not change.
         */
        if(end_remaining_repeat > 0)
        {
            affected_chunks.end_chunk_repeat.start_address = end_chunk->header.drive_address + end_chunk->header.chunk_size - end_remaining_repeat * end_chunk->header.data_size;
            affected_chunks.end_chunk_repeat.repeat_count = end_remaining_repeat;
        }

        /* set affected_chunks.new_data */
        simulated_drive_u64_t new_chunk_data_size = is_need_compress(repeat_count) ? 1 : repeat_count;
        affected_chunks.new_data.buffer = (simulated_drive_byte_t *)allocate_memory(new_chunk_data_size * nblocks_to_write * drive_block_size);
        simulated_drive_byte_t *temp_pointer = affected_chunks.new_data.buffer;
        if (affected_chunks.new_data.buffer == NULL)
        {
            destroy_affected_chunks(&affected_chunks);
            REPORT_DETAIL_AND_RETURN_VALUE("memory allocation for new chunk buffer failed", SIMULATED_DRIVE_INVALID_SIZE)
        }
        for(simulated_drive_s32_t i = 0; i < new_chunk_data_size; ++i)
        {
            memory_cpy(temp_pointer, buffer, nblocks_to_write * drive_block_size);
            temp_pointer += nblocks_to_write * drive_block_size;
        }
        affected_chunks.new_data.length = new_chunk_data_size * nblocks_to_write * drive_block_size;
        affected_chunks.new_data.repeat_count = !is_need_compress(repeat_count) ? 1 : repeat_count;
        affected_chunks.new_data.start_address = handle->drive_address;
    }

    /* now begin to perform real writing according to affected_chunks */

    /*
     * first check if we can merge new data with pieces from start_chunk and end_chunk.
     * it could happy when new data does not need to compress. this is optimization for regular
     * non-compress writing.
     */
    if(!check_and_merge_new_data(&affected_chunks))
    {
        REPORT_DETAIL_AND_RETURN_VALUE("merge new data with existing data failed", SIMULATED_DRIVE_INVALID_SIZE)
    }

    /* parse the affected_chunks and create necessary chunks */
    if(!allocate_new_chunks(handle, &affected_chunks, start_chunk, end_chunk))
    {
        REPORT_DETAIL_AND_RETURN_VALUE("allocated new chunks failed", SIMULATED_DRIVE_INVALID_SIZE)
    }

    /* set up linkage and write */
    set_up_chunk_linkage(handle, &affected_chunks, previous_chunk, start_chunk, end_chunk, next_chunk, &overwrite_drive_header);

    if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
    {
        if(overwrite_drive_header > 0)
        {
            write_simulated_drive_header(handle->file_handle, &simulated_drive_header);
        }
        destroy_affected_chunks(&affected_chunks);
    }

    return repeat_count * nblocks_to_write * drive_block_size;
}

static void release_chunk(simulated_drive_chunk_t * chunk)
{
     if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_MEMORY && chunk->content != NULL)
     {
         release_memory(chunk->content);
     }
    release_memory(chunk);
}
