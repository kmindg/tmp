#include <winsock2.h>
#include <stdio.h>
#include <winioctl.h>

#include "fbe/fbe_types.h"
#include "fbe/fbe_time.h"

#include "terminator_drive.h"
#include "terminator_simulated_disk_remote_file_simple.h"

#include "emcutil_exe_include_c_include.h"
#include "EmcUTIL.h"

#include "csx_ext.h"

#define DEFAULT_LISTEN_PORT     10002

#define MAX_DISK_FILE_NAME_LEN  64
#define MAX_DATA_DIR_NAME_LEN   16

#define LOG_IO_MARK_INVALID     -1

/* Default I/O mark value, in microseconds */
#define LOG_IO_MARK_LOW_DEFAULT   10000
#define LOG_IO_MARK_HIGH_DEFAULT  1000000

#define LOG_IO_BACKUP_FILE_QUANTITY     10
#define LOG_IO_BACKUP_FILE_SIZE_LIMIT   (50 * 1024 * 1024)

static fbe_bool_t g_log_io_enabled   = FBE_FALSE;
static int        g_log_io_mark_low  = LOG_IO_MARK_INVALID;
static int        g_log_io_mark_high = LOG_IO_MARK_INVALID;

#define ZERO_PATTERN_WRITE_BUFF_BLOCKS   2048
char zero_pattern_write_buf[ZERO_PATTERN_SIZE * ZERO_PATTERN_WRITE_BUFF_BLOCKS]; /* 520 bytes/block * 2048 blocks = 1MB */

/* AR495100: BZR stuck issue, Windows can't handle frequent read/write of big files well,
 *  it needs too much memory to cache for the big files, thus we split big files into
 *  multiple small ones.
 *
 * BLOCKS_PER_FILE_DEFAULT should be larger than the max possible block count in request
 *  since we only support 2 files. The most common block count is 2048 (1MB), so we just
 *  need to ensure BLOCKS_PER_FILE_DEFAULT is larger than 2048.
 */
#define BLOCKS_PER_FILE_DEFAULT         2000000 /* 520 bytes/block * 2,000,000 blocks = 1GB */
#define BLOCK_SIZE_DEFAULT              520

#define DISK_SERVER_REGISTRY_KEY        EMCUTIL_T("\\Registry\\Machine\\SOFTWARE\\DiskServer")
#define DISK_SERVER_BLOCKS_PER_FILE     EMCUTIL_T("BlocksPerFile")

#define MAX_FILES_PER_REQUEST           2       /* We support 2 files at most. */

/* DAE versioning */
#define _GET_STRING(expr)       #expr
#define GET_STRING(expr)        _GET_STRING(expr)

#define DISK_SERVER_MAJOR_VERSION_NUM      0   /* This major version number should only be updated when there is a significant change. */
#define DISK_SERVER_MINOR_VERSION_NUM      6   /* This minor version number MUST be updated whenever there is a change. */

#define DISK_SERVER_MAJOR_VERSION          GET_STRING(DISK_SERVER_MAJOR_VERSION_NUM)
#define DISK_SERVER_MINOR_VERSION          GET_STRING(DISK_SERVER_MINOR_VERSION_NUM)

#define DISK_SERVER_VERSION_DOT            "."

#define DISK_SERVER_VERSION                                 \
        DISK_SERVER_MAJOR_VERSION   DISK_SERVER_VERSION_DOT \
        DISK_SERVER_MINOR_VERSION

/* DAE tracing */
#define __PRINT_TO_FILE__(format, ...)          \
    do                                          \
    {                                           \
        fprintf(stderr, format, ##__VA_ARGS__); \
        fflush(stderr);                         \
    } while (0);

/* Add "##" before "__VA_ARGS__" to avoid the compiling errors in the case like
 *  "__PRINT_TO_CONSOLE__("A message without any format.");" on GCC, however, there
 *  is not such a problem on VC or other Windows C/C++ compilers.
 */
#define __PRINT_TO_CONSOLE__(format, ...)   printf(format, ##__VA_ARGS__);

#define __PRINT_TO_FILE_AND_CONSOLE__(format, ...)   \
    do                                               \
    {                                                \
        __PRINT_TO_FILE__   (format, ##__VA_ARGS__); \
        __PRINT_TO_CONSOLE__(format, ##__VA_ARGS__); \
    } while (0);

#define __DISK_SERVER_TRACE_COMMON__(__PRINT_TYPE__, format, ...) \
    do                                                            \
    {                                                             \
        EMCUTIL_TIME_FIELDS system_time;                          \
        EmcutilGetSystemTimeFields(&system_time);                 \
        __PRINT_TYPE__("%02u:%02u:%02u.%03u: T%p "format,         \
                       system_time.Hour,                          \
                       system_time.Minute,                        \
                       system_time.Second,                        \
                       system_time.Milliseconds,                  \
                       (csx_pvoid_t)csx_p_get_thread_id(),        \
                       ##__VA_ARGS__);                            \
    } while (0);

#define DISK_SERVER_ERROR_TAG   "DAE ERROR "

#define __DISK_SERVER_TRACE_ERROR_TO_FILE__(format, ...) \
        __DISK_SERVER_TRACE_COMMON__(__PRINT_TO_FILE__, DISK_SERVER_ERROR_TAG format, ##__VA_ARGS__)

#define __DISK_SERVER_TRACE_ERROR_TO_CONSOLE__(format, ...) \
        __DISK_SERVER_TRACE_COMMON__(__PRINT_TO_CONSOLE__, DISK_SERVER_ERROR_TAG format, ##__VA_ARGS__)

#define __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__(format, ...) \
        __DISK_SERVER_TRACE_COMMON__(__PRINT_TO_FILE_AND_CONSOLE__, format, ##__VA_ARGS__)

/* CSX file wrappers */
#define __CSX_P_FILE_CLOSE__(file_handle, file_name)                                                         \
    do                                                                                                       \
    {                                                                                                        \
        csx_status_e status = csx_p_file_close(file_handle);                                                 \
        if (!CSX_SUCCESS(status))                                                                            \
        {                                                                                                    \
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("T%llx error message: failed to close %s at line %d: %s\n", \
                                                (csx_u64_t)csx_p_get_thread_id(),                                       \
                                                file_name,                                                   \
                                                __LINE__,                                                    \
                                                csx_p_cvt_csx_status_to_string(status));                     \
        }                                                                                                    \
    } while (0);

#define __CSX_P_FILE_SYNC__(file_handle, file_name)                                                         \
    do                                                                                                      \
    {                                                                                                       \
        csx_status_e status = csx_p_file_sync(file_handle);                                                 \
        if (!CSX_SUCCESS(status))                                                                           \
        {                                                                                                   \
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("T%llx error message: failed to sync %s at line %d: %s\n", \
                                                (csx_u64_t)csx_p_get_thread_id(),                                      \
                                                file_name,                                                  \
                                                __LINE__,                                                   \
                                                csx_p_cvt_csx_status_to_string(status));                    \
        }                                                                                                   \
    } while (0);

static fbe_u32_t blocks_per_file;

typedef struct disk_server_one_read_write_s
{
    char      file_name[MAX_DISK_FILE_NAME_LEN];
    fbe_u64_t offset_in_file;
    fbe_u64_t length_in_file;
    fbe_u64_t offset_in_buff;
} disk_server_one_read_write_t;

typedef struct disk_server_read_write_s
{
    fbe_u64_t                    num_of_files;
    disk_server_one_read_write_t read_writes[MAX_FILES_PER_REQUEST];
} disk_server_read_write_t;

static void generate_disk_file_name_prefix(char *disk_file_name_prefix, const remote_file_simple_request_t *request)
{
    _snprintf(disk_file_name_prefix,
              MAX_DISK_FILE_NAME_LEN - 1,
#ifdef ALAMOSA_WINDOWS_ENV
              "%u\\disk%u_%u_%u",
#else
              "%u/disk%u_%u_%u",
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
              request->array_id,
              request->port,
              request->enclosure,
              request->slot);
}

static void generate_disk_file_name_for_splitting_file(char *disk_file_name, const char *disk_file_name_prefix, fbe_u64_t file_index)
{
    _snprintf(disk_file_name,
              MAX_DISK_FILE_NAME_LEN - 1,
              "%s_%.3llu",
              disk_file_name_prefix,
              file_index);
}

static void generate_disk_file_name_for_logging(char *disk_file_name, const remote_file_simple_request_t *request)
{
    _snprintf(disk_file_name,
              MAX_DISK_FILE_NAME_LEN - 1,
              "%u_%u_%u",
              request->port,
              request->enclosure,
              request->slot);
}

static fbe_status_t generate_disk_file_request_for_read_write(disk_server_read_write_t *read_write, const remote_file_simple_request_t *request)
{
    disk_server_one_read_write_t *file_one         = &read_write->read_writes[0];
    disk_server_one_read_write_t *file_two         = &read_write->read_writes[1];
    fbe_u64_t                     start_file_index = 0;
    fbe_u64_t                     end_file_index   = 0;
    fbe_u32_t                     total_data_len   = (fbe_u32_t)(request->block_size * request->block_count);
    char                          disk_file_name_prefix[MAX_DISK_FILE_NAME_LEN] = {0};

    if (0 == request->lba && 0 == request->block_count)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: request->lba %llu, request->block_count %llu, invalid!\n",
                                            request->lba,
                                            request->block_count);

        return FBE_STATUS_INVALID;
    }

    generate_disk_file_name_prefix(disk_file_name_prefix, request);

    start_file_index =  request->lba / blocks_per_file;
    end_file_index   = (request->lba + request->block_count - 1) / blocks_per_file;

    if (end_file_index == start_file_index) /* 1 file */
    {
        read_write->num_of_files = 1;

        generate_disk_file_name_for_splitting_file(file_one->file_name, disk_file_name_prefix, start_file_index);

        file_one->offset_in_file = request->block_size * (request->lba % blocks_per_file);
        file_one->length_in_file = total_data_len;
        file_one->offset_in_buff = 0;
    }
    else if (end_file_index == start_file_index + 1) /* 2 files */
    {
        read_write->num_of_files = 2;

        generate_disk_file_name_for_splitting_file(file_one->file_name, disk_file_name_prefix, start_file_index);

        file_one->offset_in_file = request->block_size * (request->lba % blocks_per_file);
        file_one->length_in_file = request->block_size * blocks_per_file - file_one->offset_in_file;
        file_one->offset_in_buff = 0;

        generate_disk_file_name_for_splitting_file(file_two->file_name, disk_file_name_prefix, end_file_index);

        file_two->offset_in_file = 0;
        file_two->length_in_file = total_data_len - file_one->length_in_file;
        file_two->offset_in_buff = file_one->length_in_file; /* offset_in_buff will not be used in handle_write_zero_pattern_request() */
    }
    else
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: start_file_index %llu, end_file_index %llu, invalid!\n",
                                            start_file_index,
                                            end_file_index);

        read_write->num_of_files = 0;

        return FBE_STATUS_INVALID;
    }

    return FBE_STATUS_OK;
}

static const char * remote_file_request_type_to_str(remote_file_request_type_t req_type)
{
    switch (req_type)
    {
    case CREATE_FILE_REQ:
        return "CR";
    case READ_FILE_REQ:
        return "RD";
    case WRITE_FILE_REQ:
        return "WR";
    case WRITE_FILE_ZERO_PATTERN_REQ:
        return "WZ";
    case HEART_BEAT_REQ:
        return "HB";
    default:
        return "Unknown request type";
    }
}

static fbe_u32_t get_actual_data_len(const remote_file_simple_request_t *request)
{
    fbe_u32_t data_len = 0;

    switch (request->op)
    {
    case CREATE_FILE_REQ:
        data_len = sizeof(char);
        break;
    case READ_FILE_REQ:
        data_len = (fbe_u32_t)(request->block_size * request->block_count);
        break;
    case WRITE_FILE_REQ:
        data_len = (fbe_u32_t)(request->block_size * request->block_count);
        break;
    case WRITE_FILE_ZERO_PATTERN_REQ:
        data_len = (fbe_u32_t)(ZERO_PATTERN_SIZE * request->block_count);
        break;
    case HEART_BEAT_REQ:
        data_len = 0;
        break;
    default:
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: unknown request type.\n");
    }

    return data_len;
}

static fbe_status_t check_send_result(int return_value_of_send, int bytes_should_send, SOCKET conn_sock)
{
    if (return_value_of_send == bytes_should_send)
    {
        return FBE_STATUS_OK;
    }
    else if (return_value_of_send == SOCKET_ERROR)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: send data error on socket %u.\n", conn_sock);

        __DISK_SERVER_TRACE_ERROR_TO_FILE__("System error: %s.\n", EmcutilStatusToString((EMCUTIL_STATUS)EmcutilLastNetworkErrorGet()));

        return FBE_STATUS_INVALID;
    }
    else if (return_value_of_send < bytes_should_send)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: send data error on socket %u, %d bytes should be sent, only %d bytes was sent.\n",
                                            conn_sock,
                                            bytes_should_send,
                                            return_value_of_send);

        return FBE_STATUS_INVALID;
    }
    else
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: send data error on socket %u, %d bytes should be sent, return value of send %d.\n",
                                            conn_sock,
                                            bytes_should_send,
                                            return_value_of_send);

        return FBE_STATUS_INVALID;
    }
}

static fbe_status_t check_recv_result(int return_value_of_recv, int bytes_should_recv, SOCKET conn_sock)
{
    if (return_value_of_recv == bytes_should_recv)
    {
        return FBE_STATUS_OK;
    }
    else if (return_value_of_recv == 0) /* if the connection has been gracefully closed, the return value is zero */
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: socket %u has been closed.\n", conn_sock);

        return FBE_STATUS_INVALID;
    }
    else if (return_value_of_recv == SOCKET_ERROR)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: error occurred on socket %u when receiving.\n", conn_sock);

        __DISK_SERVER_TRACE_ERROR_TO_FILE__("System error: %s.\n", EmcutilStatusToString((EMCUTIL_STATUS)EmcutilLastNetworkErrorGet()));

        return FBE_STATUS_INVALID;
    }
    else if (return_value_of_recv < bytes_should_recv)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: failed to receive request on socket %u, %d bytes to receive, %d bytes received.\n",
                                            conn_sock,
                                            bytes_should_recv,
                                            return_value_of_recv);

        return FBE_STATUS_INVALID;
    }
    else
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: failed to receive request on socket %u, %d bytes to receive, return value of recv %d.\n",
                                            conn_sock,
                                            bytes_should_recv,
                                            return_value_of_recv);

        return FBE_STATUS_INVALID;
    }
}

static void log_request_info_before_handling(const remote_file_simple_request_t *request, csx_timestamp_value_t *p_start_time)
{
    char disk_file_name[MAX_DISK_FILE_NAME_LEN] = {0};

    generate_disk_file_name_for_logging(disk_file_name, request);

    /* Log will be like this: 06:50:28.460: T9f4 0_0_1 RD-S LBA 6550547 520by
     */
    __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("%s %s-S LBA %llu %uby\n",
                                                   disk_file_name,
                                                   remote_file_request_type_to_str(request->op),
                                                   request->lba,
                                                   get_actual_data_len(request));

    csx_p_get_timestamp_value(p_start_time);
}

static LONGLONG get_time_diff_in_microsecond(csx_timestamp_value_t start_time, csx_timestamp_value_t end_time)
{
    csx_timestamp_frequency_t freq;

    csx_p_get_timestamp_frequency(&freq);

    return ((end_time - start_time) * 1000 * 1000 / freq);
}

static void log_request_info_after_handling(const remote_file_simple_request_t *request, csx_timestamp_value_t start_time)
{
    csx_timestamp_value_t end_time;
    LONGLONG              time_diff;
    char                  disk_file_name[MAX_DISK_FILE_NAME_LEN] = {0};

    csx_p_get_timestamp_value(&end_time);
    time_diff = get_time_diff_in_microsecond(start_time, end_time);

    generate_disk_file_name_for_logging(disk_file_name, request);

#define LOG_AFTER_HANDLING_FORMAT "%s %s-C LBA %llu %uby %lldus\n"

#define LOG_AFTER_HANDLING_ARG_LIST                          \
               disk_file_name,                               \
               remote_file_request_type_to_str(request->op), \
               request->lba,                                 \
               get_actual_data_len(request),                 \
               time_diff

    /* Log will be like this: 06:50:28.460: T780 0_0_2 RD-C LBA 6854543 1040by 68us
     */
    if (g_log_io_mark_low <= time_diff && time_diff < g_log_io_mark_high)
    {
        __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__(LOG_AFTER_HANDLING_FORMAT, LOG_AFTER_HANDLING_ARG_LIST);
    }
    else if (g_log_io_mark_high <= time_diff)
    {
        __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__(LOG_AFTER_HANDLING_FORMAT, LOG_AFTER_HANDLING_ARG_LIST);
    }

#undef LOG_AFTER_HANDLING_ARG_LIST
#undef LOG_AFTER_HANDLING_FORMAT
}

static LONGLONG get_file_size(const char *file_name)
{
    csx_status_e            status      = CSX_STATUS_GENERAL_FAILURE;
    csx_p_file_t            file_handle = CSX_NULL;
    csx_p_file_query_info_t query_info;
    const LONGLONG          ERROR_CODE  = -1;

    if (!file_name)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: %s() parameter error.\n", __FUNCTION__);

        return ERROR_CODE;
    }

    status = csx_p_file_open(CSX_MY_MODULE_CONTEXT(),
                             &file_handle,
                             file_name,
                             "rb");
    if (!CSX_SUCCESS(status))
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: csx_p_file_open() failed in %s(): %s.\n",
                                            __FUNCTION__,
                                            csx_p_cvt_csx_status_to_string(status));

        return ERROR_CODE;
    }

    status = csx_p_file_query(&file_handle, &query_info);
    if (!CSX_SUCCESS(status))
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: csx_p_file_query() failed in %s(): %s.\n",
                                            __FUNCTION__,
                                            csx_p_cvt_csx_status_to_string(status));

        __CSX_P_FILE_CLOSE__(&file_handle, file_name);

        return ERROR_CODE;
    }

    __CSX_P_FILE_CLOSE__(&file_handle, file_name);

    return query_info.file_size;
}

static void move_file(const char *from_file, const char *to_file)
{
    csx_status_e status = CSX_STATUS_GENERAL_FAILURE;

    if (!from_file || !to_file)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: %s() parameters error.\n", __FUNCTION__);

        return;
    }

    status = csx_p_file_rename(from_file, to_file);
    if (!CSX_SUCCESS(status))
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to rename %s to %s with csx_p_file_rename() in %s(): %s.\n",
                                            from_file,
                                            to_file,
                                            __FUNCTION__,
                                            csx_p_cvt_csx_status_to_string(status));

        return;
    }

    __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("Backup %s to %s.\n", from_file, to_file);
}

static void delete_and_backup(const char *delete_file_name, const char *new_file_name)
{
    csx_status_e status = CSX_STATUS_GENERAL_FAILURE;

    if (!delete_file_name || !new_file_name)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: %s() parameters error.\n", __FUNCTION__);

        return;
    }

    __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("Trying to delete file: %s.\n", delete_file_name);

    status = csx_p_file_delete(delete_file_name);
    if (!CSX_SUCCESS(status))
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to delete file %s with csx_p_file_delete() in %s(): %s.\n",
                                            delete_file_name,
                                            __FUNCTION__,
                                            csx_p_cvt_csx_status_to_string(status));

        return;
    }

    __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("Delete %s successfully\n", delete_file_name);

    move_file(new_file_name, delete_file_name);
}

static void backup_if_too_large(const char *file_name)
{
    char backup_file_name[MAX_DISK_FILE_NAME_LEN] = {0},
         oldest_file_name[MAX_DISK_FILE_NAME_LEN] = {0};

    int       file_index         = 0;
    csx_u64_t oldest_access_time = -1;

    if (!file_name)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: %s() parameter error.\n", __FUNCTION__);

        return;
    }

    if (get_file_size(file_name) < LOG_IO_BACKUP_FILE_SIZE_LIMIT)
    {
        return;
    }

    for (file_index = 1; file_index <= LOG_IO_BACKUP_FILE_QUANTITY; ++file_index)
    {
        csx_status_e            status             = CSX_STATUS_GENERAL_FAILURE;
        csx_p_file_t            backup_file_handle = CSX_NULL;
        csx_p_file_query_info_t query_info;

        _snprintf(backup_file_name,
                  MAX_DISK_FILE_NAME_LEN - 1,
                  "%s_%d.bak",
                  file_name,
                  file_index);

        status = csx_p_file_open(CSX_MY_MODULE_CONTEXT(),
                                 &backup_file_handle,
                                 backup_file_name,
                                 "rb");
        if (!CSX_SUCCESS(status))
        {
            move_file(file_name, backup_file_name);

            return;
        }

        status = csx_p_file_query(&backup_file_handle, &query_info);
        if (!CSX_SUCCESS(status))
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to query %s with csx_p_file_query() in %s(): %s.\n",
                                                backup_file_name,
                                                __FUNCTION__,
                                                csx_p_cvt_csx_status_to_string(status));

            __CSX_P_FILE_CLOSE__(&backup_file_handle, backup_file_name);

            continue;
        }

        if (query_info.file_atime < oldest_access_time)
        {
            oldest_access_time = query_info.file_atime;
            strncpy(oldest_file_name, backup_file_name, MAX_DISK_FILE_NAME_LEN);
        }

        __CSX_P_FILE_CLOSE__(&backup_file_handle, backup_file_name);
    }

    delete_and_backup(oldest_file_name, file_name);
}

static void prepare_zero_pattern(void)
{
    char       zero_pattern[ZERO_PATTERN_SIZE] = {0};
    fbe_u64_t *zero_pattern_checksum           = (fbe_u64_t *)&zero_pattern[ZERO_PATTERN_SIZE - sizeof(fbe_u64_t)];
    int        i = 0;

    *zero_pattern_checksum = ZERO_PATTERN_CHECKSUM;

    for (i = 0; i < ZERO_PATTERN_WRITE_BUFF_BLOCKS; ++i)
    {
        memcpy(zero_pattern_write_buf + i * ZERO_PATTERN_SIZE, zero_pattern, ZERO_PATTERN_SIZE);
    }
}

static BOOL handle_create_request(SOCKET conn_sock, remote_file_simple_request_t *request)
{
    csx_status_e status = CSX_STATUS_GENERAL_FAILURE;

    char data_dir_name        [MAX_DATA_DIR_NAME_LEN]  = {0},
         disk_file_name_prefix[MAX_DISK_FILE_NAME_LEN] = {0},
         disk_file_name       [MAX_DISK_FILE_NAME_LEN] = {0};

    fbe_u64_t file_index = 0,
              file_num   = (request->lba + blocks_per_file - 1) / blocks_per_file;

    int          result      = 0;
    fbe_status_t send_result = FBE_STATUS_GENERIC_FAILURE;

    remote_file_simple_response_t response;

    init_remote_file_simple_response(&response);

    /* Create the data directory.
     */
    _snprintf(data_dir_name,
              MAX_DATA_DIR_NAME_LEN - 1,
              "%u",
              request->array_id);

    status = csx_p_dir_create(data_dir_name);
    if (!CSX_SUCCESS(status) && status != CSX_STATUS_OBJECT_ALREADY_EXISTS)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to create data directory %s with csx_p_dir_create() in %s(): %s.\n",
                                            data_dir_name,
                                            __FUNCTION__,
                                            csx_p_cvt_csx_status_to_string(status));
    }

    generate_disk_file_name_prefix(disk_file_name_prefix, request);

    for (file_index = 0; file_index < file_num; ++file_index)
    {
        csx_p_file_t file_handle   = CSX_NULL;
        csx_size_t   bytes_written = 0;
        char         fill_data     = 0;
        csx_offset_t end_off       = (csx_offset_t)request->block_size * blocks_per_file - 1;

        response.status = FBE_STATUS_GENERIC_FAILURE;

        generate_disk_file_name_for_splitting_file(disk_file_name, disk_file_name_prefix, file_index);

        status = csx_p_file_open(CSX_MY_MODULE_CONTEXT(),
                                 &file_handle,
                                 disk_file_name,
                                 "wb");
        if (CSX_SUCCESS(status))
        {
            response.status = FBE_STATUS_OK;

            __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

            break;
        }

        status = csx_p_file_open(CSX_MY_MODULE_CONTEXT(),
                                 &file_handle,
                                 disk_file_name,
                                 "wbcs");
        if (!CSX_SUCCESS(status))
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to open file %s, socket=%u at line %d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            break;
        }

        /* This is a newly created sparse file, write the last byte.  Create
         * files with same size, we don't need accurate size for the last file.
         */
        status = csx_p_file_seek(&file_handle,
                                 CSX_P_FILE_SEEK_TYPE_SET,
                                 end_off,
                                 NULL);
        if (!CSX_SUCCESS(status))
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to set file pointer to %s, socket=%u at line %d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

            break;
        }

        status = csx_p_file_write(&file_handle,
                                  &fill_data,
                                  sizeof(fill_data),
                                  &bytes_written);
        if (!CSX_SUCCESS(status) || bytes_written != sizeof(fill_data))
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to write data to %s, socket=%u at line %d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

            break;
        }

        __CSX_P_FILE_SYNC__(&file_handle, disk_file_name);
        __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

        response.status = FBE_STATUS_OK;
    }

    result      = send(conn_sock, (const char *)&response, sizeof(response), 0);
    send_result = check_send_result(result, sizeof(response), conn_sock);

    return (FBE_SUCCESS(send_result) ? TRUE : FALSE);
}

static BOOL handle_write_request(SOCKET conn_sock, remote_file_simple_request_t *request)
{
    int       rcvd_bytes = 0;
    fbe_u64_t file_index = 0;

    int          result      = 0;
    fbe_status_t send_result = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t data_len = (fbe_u32_t)(request->block_size * request->block_count);
    char     *data     = NULL;

    disk_server_read_write_t      read_write;
    remote_file_simple_response_t response;

    fbe_zero_memory(&read_write, sizeof(read_write));
    init_remote_file_simple_response(&response);

    data = (char *)malloc(data_len);
    if (!data)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: failed to allocate memory.\n");

        return FALSE;
    }

    rcvd_bytes = recv(conn_sock, data, data_len, MSG_WAITALL);
    if (FBE_STATUS_INVALID == check_recv_result(rcvd_bytes, (int)data_len, conn_sock))
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: failed to receive data.\n");

        free(data);

        return FALSE;
    }

    if (request->checksum != get_checksum_for_eight_bytes_aligned_memory(data, data_len))
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: checksum does not match, socket=%u.\n", conn_sock);

        free(data);

        return FALSE;
    }

    if (FBE_STATUS_OK != generate_disk_file_request_for_read_write(&read_write, request))
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: failed to generate disk file request.\n");

        free(data);

        return FALSE;
    }

    for (file_index = 0; file_index < read_write.num_of_files; ++file_index)
    {
        csx_status_e status         = CSX_STATUS_GENERAL_FAILURE;
        csx_p_file_t file_handle    = CSX_NULL;
        csx_size_t   bytes_written  = 0;
        csx_size_t   bytes_to_write = (csx_size_t)read_write.read_writes[file_index].length_in_file;
        csx_offset_t offset         = read_write.read_writes[file_index].offset_in_file;
        const char  *disk_file_name = read_write.read_writes[file_index].file_name;

        response.status = FBE_STATUS_GENERIC_FAILURE;

        status = csx_p_file_open(CSX_MY_MODULE_CONTEXT(),
                                 &file_handle,
                                 disk_file_name,
                                 "wb");
        if (!CSX_SUCCESS(status))
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to open file %s, socket=%u at line %d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            break;
        }

        status = csx_p_file_seek(&file_handle,
                                 CSX_P_FILE_SEEK_TYPE_SET,
                                 offset,
                                 NULL);
        if (!CSX_SUCCESS(status))
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to set file pointer to %s, socket=%u at line%d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

            break;
        }

        status = csx_p_file_write(&file_handle,
                                  data + read_write.read_writes[file_index].offset_in_buff,
                                  bytes_to_write,
                                  &bytes_written);
        if (!CSX_SUCCESS(status) || bytes_written != bytes_to_write)
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to write data to %s, socket=%u at line %d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

            break;
        }

        __CSX_P_FILE_SYNC__(&file_handle, disk_file_name);
        __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

        response.status = FBE_STATUS_OK;
    }

    free(data);

    result      = send(conn_sock, (const char *)&response, sizeof(response), 0);
    send_result = check_send_result(result, sizeof(response), conn_sock);

    return (FBE_SUCCESS(send_result) ? TRUE : FALSE);
}

static BOOL handle_write_zero_pattern_request(SOCKET conn_sock, remote_file_simple_request_t *request)
{
    fbe_u64_t file_index = 0;

    int          result      = 0;
    fbe_status_t send_result = FBE_STATUS_GENERIC_FAILURE;

    disk_server_read_write_t      read_write;
    remote_file_simple_response_t response;

    fbe_zero_memory(&read_write, sizeof(read_write));
    init_remote_file_simple_response(&response);

    if (FBE_STATUS_OK != generate_disk_file_request_for_read_write(&read_write, request))
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: failed to generate disk file request.\n");

        return FALSE;
    }

    for (file_index = 0; file_index < read_write.num_of_files; ++file_index)
    {
        csx_status_e status         = CSX_STATUS_GENERAL_FAILURE;
        csx_p_file_t file_handle    = CSX_NULL;
        csx_size_t   bytes_written  = 0;
        csx_size_t   bytes_to_write = (csx_size_t)read_write.read_writes[file_index].length_in_file;
        csx_offset_t offset         = read_write.read_writes[file_index].offset_in_file;
        const char  *disk_file_name = read_write.read_writes[file_index].file_name;

        response.status = FBE_STATUS_GENERIC_FAILURE;

        status = csx_p_file_open(CSX_MY_MODULE_CONTEXT(),
                                 &file_handle,
                                 disk_file_name,
                                 "wb");
        if (!CSX_SUCCESS(status))
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to open file %s, socket=%u at line %d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            break;
        }

        status = csx_p_file_seek(&file_handle,
                                 CSX_P_FILE_SEEK_TYPE_SET,
                                 offset,
                                 NULL);
        if (!CSX_SUCCESS(status))
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to set file pointer to %s, socket=%u at line %d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

            break;
        }

        status = csx_p_file_write(&file_handle,
                                  zero_pattern_write_buf,
                                  bytes_to_write,
                                  &bytes_written);
        if (!CSX_SUCCESS(status) || bytes_written != bytes_to_write)
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to write zero pattern data to %s, socket=%u at line %d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

            break;
        }

        __CSX_P_FILE_SYNC__(&file_handle, disk_file_name);
        __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

        response.status = FBE_STATUS_OK;
    }

    result      = send(conn_sock, (const char *)&response, sizeof(response), 0);
    send_result = check_send_result(result, sizeof(response), conn_sock);

    return (FBE_SUCCESS(send_result) ? TRUE : FALSE);
}

static BOOL handle_read_request(SOCKET conn_sock, remote_file_simple_request_t *request)
{
    fbe_u64_t file_index = 0;

    int          result      = 0;
    fbe_status_t send_result = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t bytes_to_read = (fbe_u32_t)(request->block_size * request->block_count);
    char     *data_buffer   = NULL;
    char     *buffer        = NULL;

    disk_server_read_write_t       read_write;
    remote_file_simple_response_t *response = NULL;

    fbe_zero_memory(&read_write, sizeof(read_write));

    buffer = (char *)malloc(sizeof(remote_file_simple_response_t) + bytes_to_read);
    if (!buffer)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: failed to allocate memory.\n");

        return FALSE;
    }

    response = (remote_file_simple_response_t *)buffer;

    init_remote_file_simple_response(response);

    data_buffer = buffer + sizeof(remote_file_simple_response_t);

    if (FBE_STATUS_OK != generate_disk_file_request_for_read_write(&read_write, request))
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: failed to generate disk file request.\n");

        free(buffer);

        return FALSE;
    }

    for (file_index = 0; file_index < read_write.num_of_files; ++file_index)
    {
        csx_status_e status         = CSX_STATUS_GENERAL_FAILURE;
        csx_p_file_t file_handle    = CSX_NULL;
        csx_size_t   bytes_read     = 0;
        csx_size_t   bytes_to_read  = (csx_size_t)read_write.read_writes[file_index].length_in_file;
        csx_offset_t offset         = read_write.read_writes[file_index].offset_in_file;
        const char  *disk_file_name = read_write.read_writes[file_index].file_name;

        response->status = FBE_STATUS_GENERIC_FAILURE;

        status = csx_p_file_open(CSX_MY_MODULE_CONTEXT(),
                                 &file_handle,
                                 disk_file_name,
                                 "rb");
        if (!CSX_SUCCESS(status))
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to open file %s, socket=%u at line %d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            break;
        }

        status = csx_p_file_seek(&file_handle,
                                 CSX_P_FILE_SEEK_TYPE_SET,
                                 offset,
                                 NULL);
        if (!CSX_SUCCESS(status))
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to set file pointer to %s, socket=%u at line %d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

            break;
        }

        status = csx_p_file_read(&file_handle,
                                 data_buffer + read_write.read_writes[file_index].offset_in_buff,
                                 bytes_to_read,
                                 &bytes_read);
        if (!CSX_SUCCESS(status) || bytes_read != bytes_to_read)
         {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: fail to read data from %s, socket=%u at line %d: %s.\n",
                                                disk_file_name,
                                                conn_sock,
                                                __LINE__,
                                                csx_p_cvt_csx_status_to_string(status));

            __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

            break;
        }

        __CSX_P_FILE_CLOSE__(&file_handle, disk_file_name);

        response->status = FBE_STATUS_OK;
     }

    if (response->status != FBE_STATUS_OK)
    {
        result      = send(conn_sock, (const char *)response, sizeof(remote_file_simple_response_t), 0);
        send_result = check_send_result(result, (int)sizeof(remote_file_simple_response_t), conn_sock);
    }
    else
    {
        response->data_len = bytes_to_read;
        response->checksum = get_checksum_for_eight_bytes_aligned_memory(data_buffer, bytes_to_read);

        result      = send(conn_sock, buffer,         sizeof(remote_file_simple_response_t) + bytes_to_read, 0);
        send_result = check_send_result(result, (int)(sizeof(remote_file_simple_response_t) + bytes_to_read), conn_sock);
    }

    free(buffer);

    return (FBE_SUCCESS(send_result) ? TRUE : FALSE);
}

static void set_socket_recv_timeout_option(SOCKET conn_sock)
{
    /* If we have missed 3 heartbeat messages, it is reasonable to say that the client is dead.
     */
    DWORD recv_timeout = TERMINATOR_HEARTBEAT_MAX_INTERVAL_SEC * FBE_TIME_MILLISECONDS_PER_SECOND * 3;

    if (setsockopt(conn_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&recv_timeout, sizeof(recv_timeout)))
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: failed to set socket receive timeout option.\n");
    }
}

csx_void_t worker_thread_func(csx_p_thr_context_t param)
{
    SOCKET conn_sock = (SOCKET)param;

    CSX_P_THR_ENTER();

    set_socket_recv_timeout_option(conn_sock);

    for (;;)
    {
        BOOL succeed = FALSE;

        csx_timestamp_value_t        start_time;
        remote_file_simple_request_t request;

        int rcvd_bytes = recv(conn_sock, (char *)&request, sizeof(request), MSG_WAITALL);

        if (FBE_STATUS_INVALID == check_recv_result(rcvd_bytes, (int)sizeof(request) , conn_sock))
        {
            closesocket(conn_sock);

            break;
        }

        if (g_log_io_enabled)
        {
            log_request_info_before_handling(&request, &start_time);
        }

        switch (request.op)
        {
        case CREATE_FILE_REQ:
            succeed = handle_create_request(conn_sock, &request);
            break;
        case READ_FILE_REQ:
            succeed = handle_read_request(conn_sock, &request);
            break;
        case WRITE_FILE_REQ:
            succeed = handle_write_request(conn_sock, &request);
            break;
        case WRITE_FILE_ZERO_PATTERN_REQ:
            succeed = handle_write_zero_pattern_request(conn_sock, &request);
            break;
        case HEART_BEAT_REQ:
            __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("HEART_BEAT_REQ received...\n");
            succeed = TRUE;
            break;
        default:
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: unknow request received, request operation=%d, socket=%u.\n",
                                                request.op,
                                                conn_sock);

            succeed = FALSE;
            break;
        }

        if (g_log_io_enabled)
        {
            log_request_info_after_handling(&request, start_time);
        }

        if (!succeed)
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("Normal error: failed to handle request in %s, so close socket %u.\n",
                                                __FUNCTION__,
                                                conn_sock);

            closesocket(conn_sock);

            break;
        }
    }

    CSX_P_THR_EXIT();
}

static fbe_status_t check_log_io_parameters(fbe_bool_t g_log_io_enabled, int *p_log_io_mark_low, int *p_log_io_mark_high)
{
    if (!g_log_io_enabled)
    {
        return FBE_STATUS_OK;
    }

    if (LOG_IO_MARK_INVALID == *p_log_io_mark_low)
    {
        __PRINT_TO_FILE_AND_CONSOLE__("Set --logiolowmark=%d\n", LOG_IO_MARK_LOW_DEFAULT);
        *p_log_io_mark_low = LOG_IO_MARK_LOW_DEFAULT;
    }

    if (LOG_IO_MARK_INVALID == *p_log_io_mark_high)
    {
        __PRINT_TO_FILE_AND_CONSOLE__("Set --logiohighmark=%d\n", LOG_IO_MARK_HIGH_DEFAULT);
        *p_log_io_mark_high = LOG_IO_MARK_HIGH_DEFAULT;
    }

    if (*p_log_io_mark_low > *p_log_io_mark_high)
    {
        __PRINT_TO_FILE_AND_CONSOLE__("--logiolowmark=%d, --logiohighmark=%d. --logiolowmark must be smaller or equal than --logiohighmark\n",
                                      *p_log_io_mark_low,
                                      *p_log_io_mark_high);

        return FBE_STATUS_INVALID;
    }

    return FBE_STATUS_OK;
}

static void print_help(void)
{
    __PRINT_TO_CONSOLE__("Usage:  rmt_file_simple_svr [--port <listen_port>] [--datadir <data_directory>] [--help]\n"
                         "Parameters:\n"
                         "        --port           Specify the port that server will listen to\n"
                         "        --datadir        Specify the directory where the server will store disk files\n"
                         "        --logio          Log I/O infomation\n"
                         "        --logiolowmark   Specify the low water mark of logio\n"
                         "        --logiohighmark  Specify the high water mark of logio\n"
                         "        --help           Print this help\n");
}

static void set_blocks_per_file(void)
{
    fbe_u32_t blocks_per_file_default = BLOCKS_PER_FILE_DEFAULT;

    EMCUTIL_STATUS status = EmcutilRegQueryValue32Direct(DISK_SERVER_REGISTRY_KEY,
                                                         DISK_SERVER_BLOCKS_PER_FILE,
                                                         &blocks_per_file_default,
                                                         &blocks_per_file);

    if (EMCUTIL_FAILURE(status))
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("EmcUTIL error: failed to read blocks per file from registry, set to default value %d.\n", BLOCKS_PER_FILE_DEFAULT);

        blocks_per_file = BLOCKS_PER_FILE_DEFAULT;

        return;
    }

    if (blocks_per_file < BLOCKS_PER_FILE_DEFAULT)
    {
        __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("Blocks per file from registry is too small, set to default value %d.\n", BLOCKS_PER_FILE_DEFAULT);
        
        blocks_per_file = BLOCKS_PER_FILE_DEFAULT;

        return;
    }

    __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("Blocks per file successfully set to %d from registry, each file is %lluMB.\n",
                                                   blocks_per_file,
                                                   ((fbe_u64_t)blocks_per_file * BLOCK_SIZE_DEFAULT / 1024 / 1024));
}

int __cdecl main(int argc, char **argv)
{
    int     listen_port = DEFAULT_LISTEN_PORT;
    SOCKET  listen_socket;
    struct  sockaddr_in server;
    int     i     = 0,
            reuse = 1;

    const int MAX_PENDING_CONNECTIONS = 50;

    /* Back up the log file and open it again.
     */
    const char * const log_file_name   = "C:/rmt_file_simple_svr.out";
    FILE              *log_file_stream = NULL;

#include "emcutil_exe_include_c_code.h"

    backup_if_too_large(log_file_name);

    log_file_stream = freopen(log_file_name, "a+", stderr);
    if (!log_file_stream)
    {
        __DISK_SERVER_TRACE_ERROR_TO_CONSOLE__("Normal error: failed to create the log file %s.\n", log_file_name);

        return EXIT_FAILURE;
    }

    __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("Succeeded to create the log file %s.\n", log_file_name);

    /* Log DAE version
     */
    __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("DAE version: %s.\n", DISK_SERVER_VERSION);

    /* Set blocks per file from registry
     */
    set_blocks_per_file();

    /* Parse all the arguments.
     */
    for (i = 1; i < argc; ++i)
    {
        if (!strcmp("--port", argv[i]))
        {
            if (argc - 1 <= i)
            {
                __PRINT_TO_FILE_AND_CONSOLE__("No value was specified for the --port option.\n");

                return EXIT_FAILURE;
            }

            listen_port = atoi(argv[++i]);
        }
        else if (!strcmp("--datadir", argv[i]))
        {
            csx_status_e status = CSX_STATUS_GENERAL_FAILURE;

            if (argc - 1 <= i)
            {
                __PRINT_TO_FILE_AND_CONSOLE__("No value was specified for the --datadir option.\n");

                return EXIT_FAILURE;
            }

            status = csx_p_native_working_directory_set(argv[++i]);
            if (!CSX_SUCCESS(status))
            {
                __PRINT_TO_FILE_AND_CONSOLE__("Failed to set data directory to %s: %s.\n",
                                              argv[i],
                                              csx_p_cvt_csx_status_to_string(status));

                return EXIT_FAILURE;
            }
        }
        else if (!strcmp("--help", argv[i]))
        {
            print_help();

            return EXIT_SUCCESS;
        }
        else if (!strcmp("--logio", argv[i]))
        {
            g_log_io_enabled = FBE_TRUE;
        }
        else if (!strcmp("--logiolowmark", argv[i]))
        {
            if (argc - 1 <= i)
            {
                __PRINT_TO_FILE_AND_CONSOLE__("No value was specified for the --logiolowmark option.\n");

                return EXIT_FAILURE;
            }

            g_log_io_mark_low = atoi(argv[++i]);

            if (g_log_io_mark_low < 0)
            {
               __PRINT_TO_FILE_AND_CONSOLE__("--logiolowmark parameter must be larger than 0\n");

               return EXIT_FAILURE;
            }
        }
        else if (!strcmp("--logiohighmark", argv[i]))
        {
            if (argc - 1 <= i)
            {
                __PRINT_TO_FILE_AND_CONSOLE__("No value was specified for the --logiohighmark option.\n");

                return EXIT_FAILURE;
            }

            g_log_io_mark_high = atoi(argv[++i]);

            if (g_log_io_mark_high < 0)
            {
               __PRINT_TO_FILE_AND_CONSOLE__("--logiohighmark parameter must be larger than 0\n");

               return EXIT_FAILURE;
            }
        }
        else
        {
            __PRINT_TO_FILE_AND_CONSOLE__("Unknown option: %s.\n", argv[i]);

            print_help();

            return EXIT_FAILURE;
        }
    }

    if (FBE_STATUS_INVALID == check_log_io_parameters(g_log_io_enabled, &g_log_io_mark_low, &g_log_io_mark_high))
    {
        return EXIT_FAILURE;
    }

    prepare_zero_pattern();

    /* Setup for Windows socket.
     */
    if (EmcutilNetworkWorldInitializeMaybe() != 0)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: failed to init WSA library.\n");

        return EXIT_FAILURE;
    }

    server.sin_family      = AF_INET;
    server.sin_port        = htons(listen_port);
    server.sin_addr.s_addr = INADDR_ANY;

    /* Initialize the listen socket.
     */
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == listen_socket)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: failed to create listen socket.\n");

        return EXIT_FAILURE;
    }

    /* Bind the listen socket.
     */
    if (bind(listen_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: failed to bind listen socket.\n");

        return EXIT_FAILURE;
    }

    /* Set reuse for the listen socket
     */
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, (int)sizeof(reuse)) == SOCKET_ERROR)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: failed to set option for listen socket.\n");

        return EXIT_FAILURE;
    }

    /* Start to listen
     */
    if (listen(listen_socket, MAX_PENDING_CONNECTIONS) == SOCKET_ERROR)
    {
        __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: failed to start listenning.\n");

        return EXIT_FAILURE;
    }

    __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("Listening at port %d.\n", listen_port);
    __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("Server: READY. Press Ctrl+C to exit...\n");

    for (;;)
    {
        BOOL   no_delay = TRUE;
        SOCKET conn_sock;

        csx_p_thr_handle_t thread_handle = CSX_NULL;
        csx_status_e       status        = CSX_STATUS_GENERAL_FAILURE;

        __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("Waiting on accept().\n");

        /* Accept the incoming socket.
         */
        conn_sock = accept(listen_socket, 0, 0);
        if (INVALID_SOCKET == conn_sock)
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: failed to accept the incoming socket.\n");

            continue;
        }

        __DISK_SERVER_TRACE_INFO_TO_FILE_AND_CONSOLE__("Get a new connection, socket = %u.\n", conn_sock);

        /* Set TCP_NODELAY to minimize latency.
         */
        if (setsockopt(conn_sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&no_delay, sizeof(no_delay)) == SOCKET_ERROR)
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("Socket error: failed to set TCP_NODELAY option.\n");
        }

        status = csx_p_thr_create_maybe(CSX_MY_MODULE_CONTEXT(),
                                        &thread_handle,
                                        CSX_P_THR_TYPE_DETACHED,
                                        "DiskServerWorkerThread",
                                        0,
                                        worker_thread_func,
                                        CSX_NULL,
                                        (csx_p_thr_context_t)(long)conn_sock);
        if (!CSX_SUCCESS(status) || !thread_handle)
        {
            __DISK_SERVER_TRACE_ERROR_TO_FILE__("CSX error: failed to create worker thread for socket %u: %s.\n",
                                                conn_sock,
                                                csx_p_cvt_csx_status_to_string(status));

            closesocket(conn_sock);
        }
    }

    /* Should never run to here since we don't support clean exit now...
     */
    //fclose(log_file_stream);

    return EXIT_SUCCESS;
}
