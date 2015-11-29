#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#define I_AM_NATIVE_CODE
#include <windows.h>

#include "simulated_drive_private_cpp.h"

#include "csx_ext.h"

using namespace std;

simulated_drive_s32_t init_count = 0;
simulated_drive_byte_t drive_file_name[64] = "drive_file";
extern simulated_drive_bool_t fake_zeroing_read;
extern simulated_drive_backend_type_e backend_type;
extern simulated_drive_mutext_t handles_mutex;
extern simulated_drive_mutext_t header_mutex;

/*
 * Initialize the simulated drive subsystem. Once initialized, subsequent initialization will only increase the init_count. 
 * It is also required that init and cleanup should be paired.
 */
simulated_drive_bool_t _simulated_drive_init(void)
{
    // avoid multiple initializing
    if (simulated_drive_is_init)
    {
        ++init_count;
        return TRUE;
    }

    create_mutex(&header_mutex);
    create_mutex(&handles_mutex);

    if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
    {
        simulated_drive_s32_t    i;
        csx_status_e             status;
        csx_p_file_find_handle_t temp_handle;
        csx_p_file_find_info_t   FindFileData;

        /*get process id and generate unique drive file name*/
        sprintf(drive_file_name, DRIVE_FILE_NAME_FORMAT, csx_p_get_process_id());

        status = csx_p_file_find_first(&temp_handle, drive_file_name,
                                       CSX_P_FILE_FIND_FLAGS_NONE,
                                       &FindFileData);
        if (!CSX_SUCCESS(status))
        {
            status = csx_p_file_open(MY_CSX_MODULE_CONTEXT(),
                                     &simulated_drive_file_handle,
                                     drive_file_name, "Wc");
        }
        else
        {
            csx_p_file_find_close(temp_handle);

            status = csx_p_file_open(MY_CSX_MODULE_CONTEXT(),
                                     &simulated_drive_file_handle,
                                     drive_file_name, "W");
        }

        // prepare the header
        if (CSX_SUCCESS(status))
        {
            if ((status = read_simulated_drive_header(simulated_drive_file_handle, &simulated_drive_header)) == SIMULATED_DRIVE_INVALID_SIZE)
            {
                simulated_drive_is_init = FALSE;
                return FALSE;
            }
            // status == 0 means it's an empty simulated_drive file. Write hearder to the file with no drive.
            else if (status == 0)
            {
                wait_mutex(&header_mutex);
                simulated_drive_header.drive_num = 0;
                release_mutex(&header_mutex);
                write_simulated_drive_header(simulated_drive_file_handle, &simulated_drive_header);
            }

            // build the in-memory index for each drive.
            if (!build_index(simulated_drive_file_handle, &simulated_drive_header))
            {
                return FALSE;
            }

            simulated_drive_total_capacity_mb = SIMULATED_DRIVE_DEFAULT_TOTAL_CAPACITY;

            wait_mutex(&handles_mutex);
            // initialize drive handle repository.
            for (i = 0; i < SIMULATED_DRIVE_MAX_NUM * SIMULATED_DRIVE_AVERAGE_OPEN; ++i)
            {
                handles[i].used = FALSE;
            }
            release_mutex(&handles_mutex);
            simulated_drive_is_init = TRUE;
            ++init_count;

            /* clear all drive if it's in temporary mode */
            if (SIMULATED_DRIVE_MODE_TEMPORARY == get_mode())
            {
                if (!delete_all_simulated_drive(&simulated_drive_header))
                    return FALSE;
            }
            return TRUE;
        }
    }
    else if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_MEMORY)
    {
        wait_mutex(&header_mutex);
        simulated_drive_header.drive_num = 0;
        release_mutex(&header_mutex);
        simulated_drive_is_init = TRUE;
        ++init_count;
        return TRUE;
    }
    simulated_drive_is_init = FALSE;
    return FALSE;
}

/*
 * Clean up the simulated drive subsystem. Should be in pair with init.
 */
simulated_drive_bool_t _simulated_drive_cleanup(void)
{
    // do the real clean only if no client uses it.
    if (--init_count <= 0)
    {
        init_count = 0;
        simulated_drive_is_init = FALSE;
        wait_mutex(&handles_mutex);
        for (simulated_drive_s32_t i = 0; i < SIMULATED_DRIVE_MAX_NUM * SIMULATED_DRIVE_AVERAGE_OPEN; ++i)
        {
            handles[i].used = FALSE;
            handles[i].handle.current_chunk = NULL;
            handles[i].handle.drive_address = -1;
            handles[i].handle.drive_identity[0] = '\0';
            handles[i].handle.file_handle = NULL;
            handles[i].handle.header_index = -1;
        }
        release_mutex(&handles_mutex);
        if (!destory_index(&simulated_drive_header))
        {
            REPORT_DETAIL_AND_RETURN_VALUE("destroy index failed", FALSE)
        }

        if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
        {
            close_handle(simulated_drive_file_handle);
        }

        /* clear all drive if it's in temporary mode */
        if (SIMULATED_DRIVE_MODE_TEMPORARY == get_mode())
        {
            if (!delete_all_simulated_drive(&simulated_drive_header))
            {
                REPORT_DETAIL_AND_RETURN_VALUE("clear all drives failed", FALSE)
            }
        }
        destroy_mutex(&handles_mutex);
        destroy_mutex(&header_mutex);
    }
    return TRUE;
}

/*
 * Read drive list from in-memory index.
 */
simulated_drive_list_t _simulated_drive_get_drive_list(void)
{
    simulated_drive_list_t drive_list;

    wait_mutex(&header_mutex);
    drive_list.size = simulated_drive_header.drive_num;
    for (simulated_drive_s32_t i = 0; i < (simulated_drive_s32_t)simulated_drive_header.drive_num; ++i)
    {
        drive_list.drives[i].drive_block_size = simulated_drive_header.index[i].drive_block_size;
        memory_set(drive_list.drives[i].drive_identity, '\0', sizeof(simulated_drive_identity_t));
        memory_cpy(drive_list.drives[i].drive_identity, simulated_drive_header.index[i].drive_identity, sizeof(simulated_drive_identity_t));
        drive_list.drives[i].max_lba = simulated_drive_header.index[i].max_lba;
    }
    release_mutex(&header_mutex);
    return drive_list;
}

/*
 * Read Simulated Drive server process ID.
 */
simulated_drive_u32_t _simulated_drive_get_server_pid(void)
{
    return csx_p_get_process_id();
}

/*
 * Open the drive and return a simulated_drive_handle. File point will be put to the beginning of the content.
 * The handle is allocated from a repository. Should be in pair with close. 
 */
simulated_drive_handle_t _simulated_drive_open(simulated_drive_identity_t drive_identity)
{
    long seek_status;
    simulated_drive_u64_t item_index;
    simulated_drive_handle_t handle;

    if (!simulated_drive_is_init)
    {
        REPORT_DETAIL_AND_RETURN_VALUE("simulated drive has not been initialized", SIMULATED_DRIVE_INVALID_HANDLE)
    }

    // match the identity.
    if (!find_simulated_drive_item(&simulated_drive_header, drive_identity, &item_index))
    {
//        REPORT_DETAIL_AND_RETURN_VALUE("invalid drive identity", SIMULATED_DRIVE_INVALID_HANDLE)
        return SIMULATED_DRIVE_INVALID_HANDLE;
    }
    else
    {
        wait_mutex(&handles_mutex);
        // obtain a valid(unused) handle from the drive repository.
        if ((handle = find_valid_handle()) == SIMULATED_DRIVE_INVALID_HANDLE)
        {
            release_mutex(&handles_mutex);
            REPORT_DETAIL_AND_RETURN_VALUE("no more available simulated drive handle can be assigned", SIMULATED_DRIVE_INVALID_HANDLE)
        }
        handles[handle].handle.header_index = item_index;

        memory_set(handles[handle].handle.drive_identity, '\0', sizeof(simulated_drive_identity_t));
        memory_cpy(handles[handle].handle.drive_identity, drive_identity, sizeof(simulated_drive_identity_t));
        handles[handle].handle.file_handle = simulated_drive_file_handle;
        handles[handle].used = TRUE;
        release_mutex(&handles_mutex);
    }

    // set the file pointer inside the simulated_drive_handle to the beginning of its content.
    if ((seek_status = lseek_simulated_drive(handle, 0, FILE_BEGIN)) == SIMULATED_DRIVE_SEEK_ERR)
    {
        REPORT_DETAIL_AND_RETURN_VALUE("seeking error", SIMULATED_DRIVE_INVALID_HANDLE)
    }

    return handle;
}

/*
 * Create new simulated drive. If the drive with drive_identity is opened, raise an error. 
 * If the drive exists but is not opened, its content will be abandoned. 
 */
simulated_drive_handle_t _simulated_drive_create(simulated_drive_identity_t drive_identity, simulated_drive_size_t drive_block_size, simulated_drive_size_t max_lba)
{
    long seek_status;
    simulated_drive_u64_t item_index;
    simulated_drive_handle_t handle;

    if (!simulated_drive_is_init)
    {
        REPORT_DETAIL_AND_RETURN_VALUE("simulated drive has not been initialized", SIMULATED_DRIVE_INVALID_HANDLE)
    }
    
    // if the drive is already opened, raise an error.
    if (is_drive_opened(drive_identity))
    {
        cout << "Error: drive has been opened, can not create with the same identity: [" << drive_identity << "]." << endl;
        REPORT_GENERAL_AND_RETURN_VALUE(SIMULATED_DRIVE_INVALID_HANDLE)
    }

    wait_mutex(&handles_mutex);
    // obtain a valid(unused) handle from the drive repository.
    if ((handle = find_valid_handle()) == SIMULATED_DRIVE_INVALID_HANDLE)
    {
        release_mutex(&handles_mutex);
        REPORT_DETAIL_AND_RETURN_VALUE( "no more available simulated drive handle can be assigned.", SIMULATED_DRIVE_INVALID_HANDLE)
    }

    if (find_simulated_drive_item(&simulated_drive_header, drive_identity, &item_index))
    {
        wait_mutex(&header_mutex);
        // abandon all the content if the drive exists.
        memory_set(simulated_drive_header.index[item_index].drive_identity, '\0', sizeof(simulated_drive_identity_t));
        memory_cpy(simulated_drive_header.index[item_index].drive_identity, drive_identity, sizeof(simulated_drive_identity_t));
        simulated_drive_header.index[item_index].data_size = 0;
        simulated_drive_header.index[item_index].first_chunk_location = 0;
        simulated_drive_header.index[item_index].drive_block_size = drive_block_size;
        simulated_drive_header.index[item_index].max_lba = max_lba;
        simulated_drive_header.index[item_index].chunk_list_head = 0;
        simulated_drive_header.index[item_index].chunk_list_end = 0;
        destroy_mutex(&(simulated_drive_header.index[item_index].io_mutext));
        create_mutex(&(simulated_drive_header.index[item_index].io_mutext));
        release_mutex(&header_mutex);
        handles[handle].handle.header_index = item_index;
    }
    else
    {
        // add new drive to the header if it's a new drive.
        if (!add_simulated_drive_item(&simulated_drive_header, drive_identity, drive_block_size, max_lba))
        {
            release_mutex(&handles_mutex);
            REPORT_DETAIL_AND_RETURN_VALUE("add drive failed in the internal index", SIMULATED_DRIVE_INVALID_HANDLE)
        }
        handles[handle].handle.header_index = simulated_drive_header.drive_num - 1;
    }
    memset(handles[handle].handle.drive_identity, '\0', sizeof(simulated_drive_identity_t));
    memory_cpy(handles[handle].handle.drive_identity, drive_identity, sizeof(simulated_drive_identity_t));
    handles[handle].handle.file_handle = simulated_drive_file_handle;
    handles[handle].used = TRUE;
    release_mutex(&handles_mutex);

    // set the file pointer inside the simulated_drive_handle to the beginning of its content.
    if ((seek_status = lseek_simulated_drive(handle, 0, FILE_BEGIN)) == SIMULATED_DRIVE_SEEK_ERR)
    {
        REPORT_DETAIL_AND_RETURN_VALUE("seeking failed", SIMULATED_DRIVE_INVALID_HANDLE)
    }
    if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
    {
        if (write_simulated_drive_header(simulated_drive_file_handle, &simulated_drive_header) == SIMULATED_DRIVE_INVALID_SIZE)
        {
            REPORT_DETAIL_AND_RETURN_VALUE("write drive header failed", SIMULATED_DRIVE_INVALID_HANDLE)
        }
    }

    return handle;
}

/*
 * Remove an existing simulated drive. If the drive with drive_identity is opened, raise an error.
 * All content will be abandoned.
 */
simulated_drive_bool_t _simulated_drive_remove(simulated_drive_identity_t drive_identity)
{
    simulated_drive_u64_t item_index;

    if (!simulated_drive_is_init)
    {
        REPORT_DETAIL_AND_RETURN_VALUE("simulated drive has not been initialized", FALSE)
    }

    // if the drive is already opened, raise an error.
    if (is_drive_opened(drive_identity))
    {
        REPORT_DETAIL_AND_RETURN_VALUE("drive has been opened, can not remove it", FALSE)
    }

    if (find_simulated_drive_item(&simulated_drive_header, drive_identity, &item_index))
    {
        // remove the drive from the header.
        if (!delete_simulated_drive_item(&simulated_drive_header, item_index))
        {
            REPORT_DETAIL_AND_RETURN_VALUE("remove drive failed in the internal index", FALSE)
        }
    }
    if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
    {
        if (write_simulated_drive_header(simulated_drive_file_handle, &simulated_drive_header) == SIMULATED_DRIVE_INVALID_SIZE)
        {
            REPORT_DETAIL_AND_RETURN_VALUE("write drive header failed", FALSE)
        }
    }

    return TRUE;
}

/*
 * Remove all simulated drives.
 */
simulated_drive_bool_t _simulated_drive_remove_all(void)
{
    simulated_drive_bool_t result;
    result = delete_all_simulated_drive(&simulated_drive_header);
    return result;
}

/*
 * Set the total capacity of the entire simulated drive file.
 */
simulated_drive_bool_t _simulated_drive_set_total_capacity(simulated_drive_capacity_t capacity)
{
    if (capacity <= 0)
    {
        return FALSE;
    }
    else
    {
        simulated_drive_total_capacity_mb = capacity;
        return TRUE;
    }
}

/*
 * Set the mode of simulated drive.
 */
simulated_drive_bool_t _simulated_drive_set_mode(simulated_drive_mode_e mode)
{
    set_mode(mode);
    return TRUE;
}

/*
 * Set the server port
 */
simulated_drive_bool_t _simulated_drive_set_server_port(simulated_drive_port_t new_server_port)
{
    set_server_port(new_server_port);
    return TRUE;
}

/*
 * Get the server port
 */
simulated_drive_port_t _simulated_drive_get_server_port(void)
{
    return get_server_port();
}

/*
 * Set the mode of simulated drive.
 */
simulated_drive_bool_t _simulated_drive_set_backend_type(simulated_drive_backend_type_e type)
{
    set_backend_type(type);
    return TRUE;
}

/*
 * Set whether need to fake zeroing pattern when reading.
 */
simulated_drive_bool_t _simulated_drive_set_fake_zeroing_read(simulated_drive_bool_t flag)
{
    set_fake_zeroing_read(flag);
    return TRUE;
}

/*
 * Close the simulated_drive_handle(clear and mark it as unused).
 */
simulated_drive_bool_t _simulated_drive_close(simulated_drive_handle_t handle)
{
    wait_mutex(&handles_mutex);
    handles[handle].used = FALSE;
    handles[handle].handle.current_chunk = NULL;
    handles[handle].handle.drive_address = -1;
    handles[handle].handle.drive_identity[0] = '\0';
    handles[handle].handle.file_handle = NULL;
    handles[handle].handle.header_index = -1;
    release_mutex(&handles_mutex);
    return TRUE;
}

/*
 * Read [nblocks_to_read] blocks to the buffer. Return read bytes if succeeds, SIMULATED_DRIVE_INVALID_SIZE if fails.
 */
simulated_drive_size_t _simulated_drive_read(simulated_drive_handle_t file_handle, void *buffer, simulated_drive_lba_t lba, simulated_drive_u64_t nblocks_to_read)
{
    simulated_drive_size_t each_read, temp_count, offset, temp_size, result;
    simulated_drive_handle_entry_t *handle;
    simulated_drive_chunk_t *chunk;
    simulated_drive_byte_t *data, *temp_buffer;
    simulated_drive_lba_t temp_location;
    simulated_drive_s64_t cnt;

    wait_mutex(&handles_mutex);
    SIMULATED_DRIVE_HANDLE_VALIDATE_WITH_LOCK(file_handle, SIMULATED_DRIVE_INVALID_SIZE)

    handle = &(handles[file_handle].handle);
    release_mutex(&handles_mutex);

    wait_mutex(&(simulated_drive_header.index[handle->header_index].io_mutext));
    cnt = nblocks_to_read * simulated_drive_header.index[handle->header_index].drive_block_size;
    data = (simulated_drive_byte_t *)buffer;

    // boundary checking.
    if (simulated_drive_header.index[handle->header_index].max_lba + 1 < lba + nblocks_to_read)
    {
        cout << "Error: driver ["
                << simulated_drive_header.index[handle->header_index].drive_identity
                << "] - required blocks [" << lba << " - " << lba + nblocks_to_read << "] exceed max_lba ["
                << simulated_drive_header.index[handle->header_index].max_lba
                << "]." << endl;
        release_mutex(&(simulated_drive_header.index[handle->header_index].io_mutext));
        REPORT_GENERAL_AND_RETURN_VALUE(SIMULATED_DRIVE_INVALID_SIZE)
    }

    /*!@todo Fill the data buffer with valid zero buffer, we need to take out this
     * code changes when we have zero on demand working.
     */
    if(fake_zeroing_read)
    {
        generate_valid_zero_buffer(data, lba, nblocks_to_read, simulated_drive_header.index[handle->header_index].drive_block_size);
    }
    else
    {
        fill_in_blank(data, nblocks_to_read * simulated_drive_header.index[handle->header_index].drive_block_size, data, 0);
    }

    // seek to the beginning lba.
    if (!lseek_simulated_drive(file_handle, lba, FILE_BEGIN))
    {
        release_mutex(&(simulated_drive_header.index[handle->header_index].io_mutext));
        REPORT_DETAIL_AND_RETURN_VALUE("seeking failed", SIMULATED_DRIVE_INVALID_SIZE)
    }

    chunk = handle->current_chunk;

    // chunk_header(handle->current_chunk) equals NULL only when the lba is smaller than the smallest lba among allocated lba.
    if (chunk == NULL && simulated_drive_header.index[handle->header_index].chunk_list_head != NULL)
    {
        temp_location = simulated_drive_header.index[handle->header_index].chunk_list_head->header.drive_address - handle->drive_address;
        if (temp_location < (simulated_drive_lba_t)cnt)
        {
            // if there is overwriting, write the non-overwriting part and leave the rest to the loop below.
            data += temp_location;
            handle->drive_address += temp_location;
            cnt -= temp_location;
            chunk = simulated_drive_header.index[handle->header_index].chunk_list_head;
        }
    }

    // start reading from existing chunks
    while (chunk != NULL && cnt > 0)
    {
        // current address bigger than the starting address of current chunk means we don't need to append empty space first
        if (handle->drive_address >= chunk->header.drive_address)
        {
            // current address falls into current chunk
            if (handle->drive_address <= chunk->header.drive_address + chunk->header.chunk_size)
            {
                // set the reading bytes number to the smaller between remaining bytes and chunk bytes after current address
                each_read = cnt < (long)(chunk->header.chunk_size - (handle->drive_address - chunk->header.drive_address)) ? cnt : chunk->header.chunk_size - (handle->drive_address - chunk->header.drive_address);
                temp_count = each_read;
                temp_buffer = (simulated_drive_byte_t *)allocate_memory(chunk->header.data_size);
                if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_FILE)
                {
                    // read the chunk content and copy the correct part to return buffer
                    if (read_simulated_drive_chunk_content(handle->file_handle, temp_buffer, chunk->header.current_location, chunk->header.data_size) == SIMULATED_DRIVE_INVALID_SIZE)
                    {
                        release_memory(temp_buffer);
                        release_mutex(&(simulated_drive_header.index[handle->header_index].io_mutext));
                        REPORT_DETAIL_AND_RETURN_VALUE("read block content failed", SIMULATED_DRIVE_INVALID_SIZE)
                    }
                }
                else if(backend_type == SIMULATED_DRIVE_BACKEND_TYPE_MEMORY)
                {
                    if(((simulated_drive_chunk_t *)chunk)->content == NULL)
                    {
                        release_memory(temp_buffer);
                        release_mutex(&(simulated_drive_header.index[handle->header_index].io_mutext));
                        REPORT_DETAIL_AND_RETURN_VALUE("read block content failed", SIMULATED_DRIVE_INVALID_SIZE)
                    }
                    memory_cpy(temp_buffer, ((simulated_drive_chunk_t *)chunk)->content, chunk->header.data_size);
                }
                offset = (handle->drive_address - chunk->header.drive_address) % chunk->header.data_size;
                temp_size = chunk->header.data_size - offset < temp_count ? chunk->header.data_size - offset : temp_count;
                memory_cpy(data, temp_buffer + offset, temp_size);
                data += temp_size;
                temp_count -= temp_size;
                // if it is a repeat count we may copy multiple times to the return buffer
                while(temp_count > 0)
                {
                    temp_size = chunk->header.data_size < temp_count ? chunk->header.data_size : temp_count;
                    memory_cpy(data, temp_buffer, temp_size);
                    data += temp_size;
                    temp_count -= temp_size;
                }
                release_memory(temp_buffer);
            }
            // else current address is bigger than max address in current chunk, skip it
            else
            {
                each_read = 0;
            }
            chunk = chunk->next_chunk;
        }
        // else we need to append some empty space
        else
        {
            each_read = chunk->header.drive_address - handle->drive_address;
            if((simulated_drive_size_t)cnt < each_read)
            {
                each_read = (simulated_drive_size_t)cnt;
            }
            data += each_read;
        }
        handle->drive_address += each_read;
        cnt -= each_read;
    }

    handle->drive_address += cnt;

    if (nblocks_to_read * simulated_drive_header.index[handle->header_index].drive_block_size <= 0)
    {
        release_mutex(&(simulated_drive_header.index[handle->header_index].io_mutext));
        REPORT_DETAIL_AND_RETURN_VALUE("read failed", nblocks_to_read * simulated_drive_header.index[handle->header_index].drive_block_size)
    }

    result = nblocks_to_read * simulated_drive_header.index[handle->header_index].drive_block_size;

    release_mutex(&(simulated_drive_header.index[handle->header_index].io_mutext));
    return result;
}

simulated_drive_size_t _simulated_drive_write(simulated_drive_handle_t file_handle, void *buffer, simulated_drive_lba_t lba, simulated_drive_u64_t nblocks_to_write)
{
    return _simulated_drive_write_same(file_handle, buffer, lba, nblocks_to_write, 1);
}


/*
 * It is the underlying function of interface simulated_drive_write_same().
 */
simulated_drive_size_t _simulated_drive_write_same(simulated_drive_handle_t file_handle, void *buffer, simulated_drive_lba_t lba, simulated_drive_u64_t nblocks_to_write, simulated_drive_u64_t repeat_count)
{
    simulated_drive_chunk_t *previous_chunk = NULL, *start_chunk = NULL, *end_chunk = NULL, *next_chunk = NULL;
    simulated_drive_handle_entry_t *handle;
    simulated_drive_size_t result;

    // check the buffer
    if(buffer == NULL)
    {
        REPORT_DETAIL_AND_RETURN_VALUE("invalid buffer", SIMULATED_DRIVE_INVALID_SIZE)
    }

    if(nblocks_to_write < 0)
    {
        REPORT_DETAIL_AND_RETURN_VALUE("negative nblocks_to_write", SIMULATED_DRIVE_INVALID_SIZE)
    }

    if(repeat_count <= 0)
    {
        REPORT_DETAIL_AND_RETURN_VALUE("non-positive repeat_count", SIMULATED_DRIVE_INVALID_SIZE)
    }

    wait_mutex(&handles_mutex);
    // validate the input file handle
    SIMULATED_DRIVE_HANDLE_VALIDATE_WITH_LOCK(file_handle, SIMULATED_DRIVE_INVALID_SIZE)
    handle = &(handles[file_handle].handle);
    release_mutex(&handles_mutex);

    // boundary checking
    wait_mutex(&simulated_drive_header.index[handle->header_index].io_mutext);
    if (simulated_drive_header.index[handle->header_index].max_lba + 1 < lba + nblocks_to_write * repeat_count)
    {
        cout << lba << " + " << nblocks_to_write << " * " << repeat_count << " = " << lba + nblocks_to_write * repeat_count
                << " > " << simulated_drive_header.index[handle->header_index].max_lba << endl;
        release_mutex(&(simulated_drive_header.index[handle->header_index].io_mutext));
        REPORT_DETAIL_AND_RETURN_VALUE("required blocks exceed max_lba", SIMULATED_DRIVE_INVALID_SIZE)
    }

    // seek to the right chunk
    if (!lseek_simulated_drive(file_handle, lba, FILE_BEGIN))
    {
        release_mutex(&(simulated_drive_header.index[handle->header_index].io_mutext));
        REPORT_DETAIL_AND_RETURN_VALUE("seeking failed", SIMULATED_DRIVE_INVALID_SIZE)
    }

    /* retrieving the context of new data */
    get_new_data_context(handle, lba + nblocks_to_write * repeat_count - 1, &previous_chunk, &start_chunk, &end_chunk, &next_chunk);

    result = insert_new_data(handle, buffer, nblocks_to_write, repeat_count, previous_chunk, start_chunk, end_chunk, next_chunk);

    release_mutex(&simulated_drive_header.index[handle->header_index].io_mutext);
    return result;
}
