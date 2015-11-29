/***************************************************************************
*  terminator_simulated_disk_main.c
***************************************************************************
*
*  Description
*      APIs to emulate the disk drives
*
*
*  History:
*      08/12/09    guov    Created (Move Peter's memory_drive)
*
***************************************************************************/

#include "terminator_simulated_disk_private.h"
#include "terminator_drive.h"
#include "fbe_terminator_common.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_time.h"
#include "fbe_terminator_file_api.h"

/**********************************/
/*        local variables         */
/**********************************/
#ifdef C4_INTEGRATED
#define OPEN_FILE_DISK_COUNT 128
struct _openDiskArray 
{
    terminator_drive_t *drive_handle;
    fbe_file_handle_t open_file;
} openDiskArray[OPEN_FILE_DISK_COUNT] = { 0 };

fbe_file_handle_t findOpenFile(terminator_drive_t *drive_handle)
{
    int i;
    for (i=0;i<OPEN_FILE_DISK_COUNT;i++)
    {
        if (drive_handle == openDiskArray[i].drive_handle)
        {
            return openDiskArray[i].open_file;
        }
    }
    return FBE_FILE_INVALID_HANDLE;
}
bool enterOpenFileIntoTable(terminator_drive_t *drive_handle, fbe_file_handle_t open_file)
{
    int i;
    for (i=0;i<OPEN_FILE_DISK_COUNT;i++)
    {
        if (openDiskArray[i].drive_handle == 0)
        {
            openDiskArray[i].drive_handle = drive_handle;
            openDiskArray[i].open_file    = open_file;
            return true;
        }
    }
    return false;
}
#endif /* C4_INTEGRATED - C4ARCH - simulator */

/******************************/
/*     local function         */
/*****************************/
static fbe_status_t generate_local_file_name (terminator_drive_t * drive_handle, fbe_u8_t * drive_name, int bufSize);

/* Zero pattern write buffer management */
#define ZERO_PATTERN_WRITE_BUF_BLOCKS   2048            /*2048 blocks equal to 1M bytes */
static fbe_u8_t *zero_pattern_write_buf = NULL;

fbe_status_t terminator_simulated_disk_local_file_init(void)
{
    fbe_status_t status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    char zero_pattern[ZERO_PATTERN_SIZE] = {0};
    fbe_u64_t* zero_pattern_checksum = (fbe_u64_t*)&zero_pattern[ZERO_PATTERN_SIZE - sizeof(fbe_u64_t)];
    int i = 0;

    // Set checksum pattern
    *zero_pattern_checksum = ZERO_PATTERN_CHECKSUM;

    if (zero_pattern_write_buf == NULL)
    {
        zero_pattern_write_buf = (fbe_u8_t *) fbe_terminator_allocate_memory(
        (fbe_u32_t) (ZERO_PATTERN_WRITE_BUF_BLOCKS * ZERO_PATTERN_SIZE));
        if (zero_pattern_write_buf == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s failed to allocate memory for zero pattern write buffer at line %d.\n",
            __FUNCTION__,
            __LINE__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (zero_pattern_write_buf)
        {
            for (i = 0; i < ZERO_PATTERN_WRITE_BUF_BLOCKS; ++i)
            {
                fbe_copy_memory(zero_pattern_write_buf + i * ZERO_PATTERN_SIZE, zero_pattern, ZERO_PATTERN_SIZE);
            }

            status = FBE_STATUS_OK;
        }
    }

    return status;
}

fbe_status_t terminator_simulated_disk_local_file_destroy(void)
{
    if (zero_pattern_write_buf)
    {
        fbe_terminator_free_memory(zero_pattern_write_buf);
        zero_pattern_write_buf = NULL;
    }
    return FBE_STATUS_OK;
}

fbe_status_t terminator_simulated_disk_local_file_write_zero_pattern(terminator_drive_t * drive_handle,
fbe_lba_t lba, 
fbe_block_count_t block_count, 
fbe_block_size_t block_size, 
fbe_u8_t * data_buffer,
void * context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t drive_name[TERMINATOR_DISK_FILE_FULL_PATH_LEN];
    fbe_file_handle_t file_handle = FBE_FILE_INVALID_HANDLE;
    fbe_u64_t device_byte = lba * block_size;

#ifdef C4_INTEGRATED
    bool lookup = true;
    file_handle = findOpenFile(drive_handle);
    if (file_handle == FBE_FILE_INVALID_HANDLE)
    {
#endif /* C4_INTEGRATED - C4ARCH - simulator */

        status = generate_local_file_name(drive_handle, drive_name, TERMINATOR_DISK_FILE_FULL_PATH_LEN);
        
        file_handle = fbe_file_open(drive_name, terminator_file_api_get_access_mode(), 0, NULL);
        if (file_handle == FBE_FILE_INVALID_HANDLE)
        {
            /* The target file doesn't exist.  Return hard error.
        * This should really be a selection timeout error.
        */
            status = FBE_STATUS_GENERIC_FAILURE;
            terminator_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s drive %s not found\n", __FUNCTION__, drive_name);
            terminator_drive_increment_error_count(drive_handle);
            return status;
        }
#ifdef C4_INTEGRATED
        lookup = enterOpenFileIntoTable(drive_handle, file_handle);
    }
#endif /* C4_INTEGRATED - C4ARCH - simulator */
    /* Try to seek to the correct location.*/
    if (fbe_file_lseek(file_handle, device_byte, 0) == FBE_FILE_ERROR)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
        "%s drive %s attempt to seek past end of disk (0x%llx).\n",
        __FUNCTION__, drive_name, 
        (unsigned long long)device_byte);
        terminator_drive_increment_error_count(drive_handle);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_block_count_t left_blocks = block_count;
        if (zero_pattern_write_buf == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s local file write not initialized\n", __FUNCTION__);
#ifdef C4_INTEGRATED
            if (!lookup)
#endif /* C4_INTEGRATED - C4ARCH - simulator */
            fbe_file_close(file_handle);
            return FBE_STATUS_NOT_INITIALIZED;
        }

        /* write all blocks */ 
        while (left_blocks > 0) 
        {
            fbe_block_count_t current_write_blocks =
            left_blocks > ZERO_PATTERN_WRITE_BUF_BLOCKS ? ZERO_PATTERN_WRITE_BUF_BLOCKS : left_blocks;
            fbe_u32_t bytes_to_xfer = (fbe_u32_t) (current_write_blocks * block_size);
            fbe_u32_t byte_written = 0;

            while(byte_written < bytes_to_xfer)
            {
                fbe_u32_t bytes = fbe_file_write(file_handle, zero_pattern_write_buf + byte_written, bytes_to_xfer - byte_written, NULL);
                if (bytes == FBE_FILE_ERROR) 
                break;
                byte_written += bytes;
            }

            if (byte_written != bytes_to_xfer)
            break;

            left_blocks -= current_write_blocks;
        }

        /* Check if we transferred enough.
            */
        if (left_blocks > 0)
        {
            /* Error, we didn't transfer what we expected.
                */
            status = FBE_STATUS_GENERIC_FAILURE;
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s drive %s unexpected transfer, left_blocks 0x%x != 0.\n",
            __FUNCTION__, drive_name, (fbe_u32_t)left_blocks);
            terminator_drive_increment_error_count(drive_handle);
        }
    }

    /* Close the file since we are done with it.
        */
#ifdef C4_INTEGRATED
    if (!lookup)
    {
        fbe_file_sync_close(file_handle);
    }
#else
    fbe_file_close(file_handle);
#endif /* C4_INTEGRATED - C4ARCH - simulator */

    return FBE_STATUS_OK;
}

fbe_status_t terminator_simulated_disk_local_file_write(terminator_drive_t * drive_handle,
fbe_lba_t lba, 
fbe_block_count_t block_count, 
fbe_block_size_t block_size, 
fbe_u8_t * data_buffer,
void * context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t drive_name[TERMINATOR_DISK_FILE_FULL_PATH_LEN];
    fbe_file_handle_t file_handle = FBE_FILE_INVALID_HANDLE;
    fbe_u64_t device_byte = lba * block_size;
    fbe_u32_t bytes_to_xfer = (fbe_u32_t) (block_count * block_size);
    fbe_u32_t bytes_received;
    fbe_time_t io_start_time;
    fbe_u32_t io_time = 0;

#ifdef C4_INTEGRATED
    bool lookup = true;
    file_handle = findOpenFile(drive_handle);
    if (file_handle == FBE_FILE_INVALID_HANDLE)
    {
#endif /* C4_INTEGRATED - C4ARCH - simulator */
        status = generate_local_file_name(drive_handle, drive_name, TERMINATOR_DISK_FILE_FULL_PATH_LEN);

        file_handle = fbe_file_open(drive_name, terminator_file_api_get_access_mode(), 0, NULL);
        if (file_handle == FBE_FILE_INVALID_HANDLE)
        {
            /* The target file doesn't exist.  Return hard error.
        * This should really be a selection timeout error.
        */
            status = FBE_STATUS_GENERIC_FAILURE;
            terminator_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s drive %s not found\n", __FUNCTION__, drive_name);
            terminator_drive_increment_error_count(drive_handle);
            return status;
        }
#ifdef C4_INTEGRATED
        lookup = enterOpenFileIntoTable(drive_handle, file_handle);
    }
#endif /* C4_INTEGRATED - C4ARCH - simulator */
    /* Try to seek to the correct location.*/
    if (fbe_file_lseek(file_handle, device_byte, 0) == FBE_FILE_ERROR)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
        "%s drive %s attempt to seek past end of disk (0x%llx).\n",
        __FUNCTION__, drive_name, 
        (unsigned long long)device_byte);
        terminator_drive_increment_error_count(drive_handle);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Get start time of IO
        */
        io_start_time = fbe_get_time_in_us();
        
        bytes_received = fbe_file_write(file_handle,
        data_buffer,
        bytes_to_xfer,
        NULL);
        
        io_time = fbe_get_elapsed_microseconds(io_start_time);
        if (io_time > TERMINATOR_IO_WARNING_TIME)
        {
            terminator_trace(FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s drive %s write took %d microseconds to complete.\n",
            __FUNCTION__, drive_name, io_time);
        }
        
        /* Check if we transferred enough.
            */
        if (bytes_received != bytes_to_xfer)
        {
            /* Error, we didn't transfer what we expected.
                */
            status = FBE_STATUS_GENERIC_FAILURE;
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s drive %s unexpected transfer.\n",
            __FUNCTION__, drive_name);
            terminator_drive_increment_error_count(drive_handle);
        }
    }

    /* Close the file since we are done with it.
        */
#ifdef C4_INTEGRATED
    if (!lookup)
    {
        fbe_file_sync_close(file_handle);
    }
#else
    fbe_file_close(file_handle);
#endif /* C4_INTEGRATED - C4ARCH - simulator */

    return status;
}

fbe_status_t terminator_simulated_disk_local_file_read(terminator_drive_t * drive_handle,
fbe_lba_t lba, 
fbe_block_count_t block_count, 
fbe_block_size_t block_size, 
fbe_u8_t * data_buffer,
void * context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t drive_name[TERMINATOR_DISK_FILE_FULL_PATH_LEN];
    fbe_file_handle_t file_handle = FBE_FILE_INVALID_HANDLE;
    fbe_u64_t device_byte = lba * block_size;
    fbe_u32_t bytes_to_xfer = (fbe_u32_t) (block_count * block_size);
    fbe_u32_t bytes_received;
    fbe_time_t io_start_time;
    fbe_u32_t io_time = 0;


    /*!@todo Fill the data buffer with valid zero buffer, we need to take out this
    * code changes when we have zero on demand working.
    */
    //status = terminator_simulated_disk_generate_valid_zero_buffer(data_buffer, lba, block_count, block_size);

#ifdef C4_INTEGRATED
    bool lookup = true;
    file_handle = findOpenFile(drive_handle);
    if (file_handle == FBE_FILE_INVALID_HANDLE)
    {
#endif /* C4_INTEGRATED - C4ARCH - simulator */
        status = generate_local_file_name(drive_handle, drive_name, TERMINATOR_DISK_FILE_FULL_PATH_LEN);

        file_handle = fbe_file_open(drive_name, terminator_file_api_get_access_mode(), 0, NULL);
        if (file_handle == FBE_FILE_INVALID_HANDLE)
        {
            /* The target file doesn't exist.  Return hard error.
        * This should really be a selection timeout error.
        */
            status = FBE_STATUS_GENERIC_FAILURE;
            terminator_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s drive %s not found\n", __FUNCTION__, drive_name);
            terminator_drive_increment_error_count(drive_handle);
            return status;
        }
#ifdef C4_INTEGRATED
        lookup = enterOpenFileIntoTable(drive_handle, file_handle);
    }
#endif /* C4_INTEGRATED - C4ARCH - simulator */
    /* Try to seek to the correct location.*/
    if (fbe_file_lseek(file_handle, device_byte, 0) == FBE_FILE_ERROR)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
        "%s drive %s attempt to seek past end of disk (0x%llx).\n",
        __FUNCTION__, drive_name, 
        (unsigned long long) device_byte);
        terminator_drive_increment_error_count(drive_handle);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        io_start_time = fbe_get_time_in_us();
        
        bytes_received = fbe_file_read(file_handle,
        data_buffer,
        bytes_to_xfer,
        NULL);
        
        io_time = fbe_get_elapsed_microseconds(io_start_time);
        if (io_time > TERMINATOR_IO_WARNING_TIME)
        {
            terminator_trace(FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s drive %s read took %d microseconds to complete.\n",
            __FUNCTION__, drive_name, io_time);
        }

        /* Check if we transferred enough.
            */
        if (bytes_received != bytes_to_xfer)
        {
            /* Error, we didn't transfer what we expected.
                */
            status = FBE_STATUS_GENERIC_FAILURE;
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s drive %s unexpected transfer.\n",
            __FUNCTION__, drive_name);
            terminator_drive_increment_error_count(drive_handle);
        }
    }
    /* Close the file since we are done with it.
        */
#ifdef C4_INTEGRATED
    if (!lookup)
    {
        fbe_file_close(file_handle);
    }
#else
    fbe_file_close(file_handle);
#endif /* C4_INTEGRATED - C4ARCH - simulator */
    return status;
}

static fbe_status_t generate_local_file_name (terminator_drive_t * drive_handle, fbe_u8_t * drive_name, int bufSize)
{
    fbe_status_t status;
    fbe_u32_t port_number, enclosure_number, slot_number;
    fbe_s32_t count;
    status = terminator_get_port_index(drive_handle, &port_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! can not get port number thru drive handle\n", __FUNCTION__);
        return status;
    }
    status = terminator_get_drive_enclosure_number(drive_handle, &enclosure_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! can not get enclosure_number thru drive handle\n", __FUNCTION__);
        return status;
    }
    status = terminator_get_drive_slot_number(drive_handle, &slot_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! can not get slot_number thru drive handle\n", __FUNCTION__);
        return status;
    }

    count = _snprintf(drive_name, bufSize, TERMINATOR_DISK_FILE_FULL_PATH_FORMAT,
    terminator_file_api_get_disk_storage_dir(), port_number, enclosure_number, slot_number);
    if (( count < 0 ) || (bufSize == count )) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! can not generate disk file name\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
