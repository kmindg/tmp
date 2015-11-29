/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_system_db_interface.c
***************************************************************************
*
* @brief
*  This file contains system_db code.
*  
* @version
*  08/03/2011 - Created. 
*
***************************************************************************/
#include "fbe_raid_library.h"
#include "fbe_database_private.h"
#include "fbe_database_system_db_interface.h"
#include "fbe/fbe_notification_lib.h"
#include "fbe_metadata.h"

fbe_raw_mirror_t fbe_database_system_db;
static fbe_database_raw_mirror_io_cb_t fbe_database_raw_mirror_io_cb;
static fbe_database_raw_mirror_drive_state_t fbe_database_raw_mirror_drive_state;


static fbe_status_t database_system_db_interface_mem_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t database_system_db_interface_db_calculate_checksum(void * block_p, fbe_lba_t start_lba,
                                                          fbe_u32_t size,
                                                          fbe_bool_t seq_reset);
static fbe_status_t database_raw_mirror_blocks_write_verify_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t database_raw_mirror_blocks_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t database_raw_mirror_blocks_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t database_util_handle_drive_lifecycle_events(fbe_database_raw_mirror_io_cb_t *  raw_mirr_io_cb,
                                                                fbe_u32_t pos, 
                                                                fbe_database_raw_mirror_drive_events_t drive_event);

fbe_status_t database_util_raw_mirror_check_error_counts(fbe_raid_verify_raw_mirror_errors_t *error_verify_report,
                                                         fbe_u64_t *c_errors,
                                                         fbe_u64_t *u_errors,
                                                         fbe_u64_t *shutdown_errors,
                                                         fbe_u64_t *retryable_errors,
                                                         fbe_u64_t *non_retryable_errors);

static fbe_bool_t raw_mirror_util_evaluate_rebuild_condition(fbe_database_raw_mirror_io_cb_t *raw_mirror_io_cb);

void fbe_database_raw_mirror_background_rebuild_thread(void *context);
fbe_status_t database_util_notify_raw_mirror_background_thread(void);
fbe_status_t fbe_database_raw_mirror_rebuild_init(fbe_database_raw_mirror_io_cb_t *rw_mirr_cb,
                                                  fbe_u32_t valid_db_drive_mask);
fbe_status_t database_util_raw_mirror_calculate_rebuild_checkpoint(fbe_database_raw_mirror_io_cb_t *raw_mirr_io_cb);
fbe_status_t database_util_raw_mirror_set_error_retry(database_raw_mirror_operation_cb_t *rw_mirr_op_cb,
                                                            fbe_bool_t flush_err,
                                                            fbe_u32_t flush_retry_count);
void database_utils_raw_mirror_init_operation_cb(database_raw_mirror_operation_cb_t *raw_mirr_op_cb, fbe_lba_t lba,
                                                 fbe_block_count_t block_count, fbe_sg_element_t *sg_list,
                                                 fbe_u32_t sg_list_count);
fbe_status_t database_system_db_interface_general_read(fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t count, 
                                             fbe_u32_t data_block_size, fbe_u64_t *done_count, fbe_u32_t read_mask,
                                             fbe_u32_t write_mask,
                                             fbe_bool_t is_read_flush);
void database_utils_raw_mirror_operation_cb_set_drive_mask(database_raw_mirror_operation_cb_t *raw_mirr_op_cb, 
                                                           fbe_u32_t  read_drive_mask,
                                                           fbe_u32_t  write_drive_mask,
                                                           fbe_bool_t force_flush);


/*!***************************************************************
 * @fn fbe_database_system_get_all_raw_mirror_drive_mask
 *****************************************************************
 * @brief
 *   return the drive mask cover all raw-mirror drives.
 *
 * @param no parameter
 *
 * @return fbe_u32_t   mask cover all drives
 *
 * @version
 *    5/24/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_u32_t fbe_database_system_get_all_raw_mirror_drive_mask(void)
{
    return DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK;
}



/*!***************************************************************
 * @fn database_system_db_interface_init
 *****************************************************************
 * @brief
 *   Control function to initialize the system db and raw-mirror IO
 *   interface.
 *
 * @param no parameter
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    8/15/2011: Gao Hongpo - created
 *
 ****************************************************************/
fbe_status_t database_system_db_interface_init()
{
    fbe_lba_t offset;
    fbe_lba_t rg_offset;
    fbe_block_count_t capacity;
    fbe_status_t status;

    FBE_ASSERT_AT_COMPILE_TIME(DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE <= FBE_RAW_MIRROR_DATA_SIZE);
    database_system_objects_get_system_db_offset_capacity(&offset, &rg_offset, &capacity);
    status = fbe_raw_mirror_init(&fbe_database_system_db, offset, rg_offset, capacity);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to init raw_mirror interface\n", __FUNCTION__);
        return status;
    }

    /*initialize raw-mirror io control block fields*/
    status = fbe_database_raw_mirror_io_cb_init(&fbe_database_raw_mirror_io_cb);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to init raw_mirror io control block\n", __FUNCTION__);
        return status;
    }

    return status;
}

/*!***************************************************************
 * @fn database_system_db_persist_init
 *****************************************************************
 * @brief
 *   Control function to initialize journal header of system db.
 *
 * @param no parameter
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    8/15/2011: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t database_system_db_persist_init(fbe_u32_t valid_db_drive_mask)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t system_db_journal_header_lba;
    fbe_u32_t   retry_count = 10;
    
    status = database_system_db_get_lba_by_region(FBE_DATABASE_SYSTEM_DB_LAYOUT_JOURNAL, &system_db_journal_header_lba);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get lba for system db journal\n", __FUNCTION__);
        return status;
    }
    /*check the journal header of system db and replay the journal log if there exist data 
      of last transaction not committed sucessfully*/
    while(retry_count--)
    {
        status = fbe_database_system_db_persist_initialize(system_db_journal_header_lba, 
                                                           system_db_journal_header_lba + 1,
                                                           valid_db_drive_mask);
        if(status == FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: succeed to init system db persist.\n", __FUNCTION__);
            break;
        }

        /*failure here may mean can't init the system db persist transaction and check journal 
          header sucessfully*/
        database_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to init system db persist. retry again\n", __FUNCTION__);


        /*need to retry as read journal maybe fail due to downstream drives not ready*/        

        fbe_thread_delay(3000);
    }

    return status;
}

/*!***************************************************************
 * @fn database_system_db_get_lba_by_region
 *****************************************************************
 * @brief
  *  return the LBA offset of a specific region:
  * @Layout
   *  system db header: 4 blocks
   *  shared emv info: 2 blocks
   *  system obj flags: 2 blocks
   *  raw mirror state: 1 block
   *  reserved regions: 16 blocks
   *  journal regions: 256 blocks
   *  system spare entry: 48 blocks
   *  object/user/edge entry: system_object_num * max_entry_num_per_object * 4 blocks 
   *  As show belows:
   *  -----------------------------------------------------------------------------------
   *  (1)4 blks | (2)2 blks | (3) reserve 16 blks | (4) journal 256 blocks | (5)system spare | (6)obj.user.edge
   *  -----------------------------------------------------------------------------------
 *
 * @param region id - region to store the system db
 * @param lba - lba to return the block address got
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    8/15/2011: Jingcheng Zhang - created
 *    5/5/2012: Hongpo Gao - modified : 4 blocks per entry
 *    5/29/2012: Zhipeng Hu - modified : change the calculation method
 *
 ****************************************************************/
fbe_status_t database_system_db_get_lba_by_region(fbe_database_system_db_layout_region_id_t region_id, fbe_lba_t *lba)
{
    
    switch (region_id) {

        /* This region is system db header starting with offset 0 with 4 blks*/
        case FBE_DATABASE_SYSTEM_DB_LAYOUT_HEADER:
            *lba = FBE_DATABASE_SYSTEM_DB_LAYOUT_HEADER_OFFSET;
            return FBE_STATUS_OK;

        /* This region is shared emv info with 2 blks*/
        case FBE_DATABASE_SYSTEM_DB_LAYOUT_SEMV_INFO:
            *lba = FBE_DATABASE_SYSTEM_DB_LAYOUT_SEMV_INFO_OFFSET;
            return FBE_STATUS_OK;

        /* This region is Recreate system objects flags */
        case FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_OBJECT_OP_FLAGS:
            *lba = FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_OBJECT_OP_FLAGS_OFFSET;
            return FBE_STATUS_OK;
            
        /* This region is database raw-mirror drive state*/
        case FBE_DATABASE_SYSTEM_DB_RAW_MIRROR_STATE:
            *lba = FBE_DATABASE_SYSTEM_DB_LAYOUT_RAW_MIRROR_STATE_OFFSET;
            return FBE_STATUS_OK;
        /*This regions is reserved regions for future usage
         * size is FBE_DATABASE_SYSTEM_ENTRY_RESERVE_BLOCKS*/

        /* This region is the journal region*/
        case FBE_DATABASE_SYSTEM_DB_LAYOUT_JOURNAL:
            *lba = FBE_DATABASE_SYSTEM_DB_LAYOUT_JOURNAL_OFFSET;
            return FBE_STATUS_OK;

        /* This region is system spare table*/
        case FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_SPARE_TABLE:
            *lba = FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_SPARE_TABLE_OFFSET;
            return FBE_STATUS_OK;    

        /* This region is system object/edge/user entries*/
        case FBE_DATABASE_SYSTEM_DB_LAYOUT_ENTRY:
            *lba = FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_ENTRY_OFFSET;
            return FBE_STATUS_OK;
            
        default:
            return FBE_STATUS_INVALID;
    }
    
}

/*!***************************************************************
 * @fn database_system_db_interface_clear
 *****************************************************************
 * @brief
 *   clear the raw-mirror system db region and set all data to zero.
 *
 * @param no 
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    8/15/2011: Gao Hongpo - created
 *
 ****************************************************************/
fbe_status_t database_system_db_interface_clear(void)
{
    fbe_u8_t *          zero_buffer;
    fbe_u64_t           chunk_nums, write_count = 0;
    fbe_u32_t           i, remain_blocks;
    fbe_lba_t           offset;
    fbe_block_count_t   capacity;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    /* Use chunk size which has 64 blocks */
    zero_buffer = (fbe_u8_t *)fbe_memory_ex_allocate(FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE);
    if (zero_buffer == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: Can't allocate buffer\n",
                      __FUNCTION__);
        return status;
    }

    fbe_zero_memory(zero_buffer, FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE);
    /* get the capacity of the raw-mirror */
    database_system_objects_get_system_db_offset_capacity(&offset, NULL, &capacity);
    chunk_nums = capacity / FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64; 
    for (i = 0; i < chunk_nums; i++) {
        status = database_system_db_interface_general_write(zero_buffer,
                                                    i * FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64,
                                                    FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE,
                                                    &write_count, FBE_TRUE,
                                                    DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK);
        if (status != FBE_STATUS_OK || (write_count != FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE )) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: zero failed, status = 0x%x, write_count = 0x%llx\n",
                    __FUNCTION__,
                    status,
                    (unsigned long long)write_count);
            fbe_memory_ex_release(zero_buffer);
            if (status == FBE_STATUS_OK) {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            return status;
        }
    }
    remain_blocks = capacity % FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64;
    if (remain_blocks > 0) {
        status = database_system_db_interface_general_write(zero_buffer,
                                                    chunk_nums * FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64,
                                                    remain_blocks * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE,
                                                    &write_count, FBE_TRUE, DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK);
        if (status != FBE_STATUS_OK || (write_count != remain_blocks * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE )) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: zero failed, status = 0x%x, write_count = 0x%llx\n",
                           __FUNCTION__,
                           status,
                           (unsigned long long)write_count);
            if (status == FBE_STATUS_OK) {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            fbe_memory_ex_release(zero_buffer);
            return status;
        }
    }

    fbe_memory_ex_release(zero_buffer);
    return status;
}

/*!***************************************************************
 * @fn block_count_chunk_size
 *****************************************************************
 * @brief
 *   map the chunk_size to the block count per chunk.
 *
 * @param chunk_size  - size of the memory chunk 
 * 
 * @return block_count - the block count of a memory chunk
 *
 * @version
 *    8/15/2011: Gao Hongpo - created
 *
 ****************************************************************/
static fbe_memory_chunk_size_block_count_t
block_count_chunk_size(fbe_memory_chunk_size_t chunk_size)
{
    switch (chunk_size) {
    case FBE_MEMORY_CHUNK_SIZE_FOR_PACKET:
        return FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET;
    case FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO:
        return FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64;
    default:
        return FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET;
    }
}

/*!***************************************************************
 * @fn database_system_db_interface_read
 *****************************************************************
 * @brief
 *   read data from raw-mirror. the read region can cover several
 *   blocks with block size DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE.
 *
 * @param data_ptr   - buffer to store the data read
 * @param start_lba  - the start lba of the raw-mirror to be read
 * @param count      - the bytes to be read from raw-mirror
 * @param done_count - return how many bytes actually read
 * 
 * @return status - return FBE_STATUS_GENERIC_FAILURE if issue encountered
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t database_system_db_interface_read(fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t count, 
                                               fbe_u32_t read_drive_mask, fbe_u64_t *done_count)
{
    /*if read detects sequence problem and flush, should write/verify all raw-mirror drives
      to ensure consistency
    */
    return database_system_db_interface_general_read(data_ptr, start_lba, count, 
                                                     DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE,
                                                     done_count, FBE_FALSE, read_drive_mask,
                                                     DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK);
}

/*!***************************************************************
 * @fn database_system_db_interface_general_read
 *****************************************************************
 * @brief
 *   general interface to read data from a contingous region in database raw-mirror,
 *   supporting two kind of block size: DATABASE_SYSTEM_DB_BLOCK_SIZE and
 *   DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE. It will flush to correct errors reported in
 *   the read operation.
 *
 * @param data_ptr   - buffer to store the data read
 * @param start_lba  - the start lba of the raw-mirror to be read
 * @param count      - the bytes to be read from raw-mirror
 * @param data_block_size - the block size to be read, only two option:
 *                          DATABASE_SYSTEM_DB_BLOCK_SIZE
 *                          DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE
 * @param done_count - return how many bytes actually read
 * @param is_read_flush - indicating if it is a read flush and needn't return data
 * 
 * @return status - return FBE_STATUS_GENERIC_FAILURE if issue encountered
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t database_system_db_interface_general_read(fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t count, 
                                             fbe_u32_t data_block_size, fbe_u64_t *done_count,
                                             fbe_bool_t is_read_flush, fbe_u32_t read_mask,
                                             fbe_u32_t  write_mask)
{
    fbe_semaphore_t                 mem_alloc_semaphore;
    fbe_memory_request_t            memory_request;
    
    fbe_block_count_t               block_count;
    fbe_memory_chunk_size_t         chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    fbe_memory_number_of_objects_t  chunk_number = 1;
    fbe_status_t                    status;
    fbe_packet_t * packet = NULL;
    fbe_sg_element_t * sg_list_p = NULL;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_memory_header_t * current_memory_header = NULL;
    fbe_memory_chunk_size_t memory_chunk_size;
    database_raw_mirror_operation_cb_t *raw_mirr_op_cb = NULL;
    fbe_u32_t   buffer_count = 0;
    fbe_s64_t   left_count = count;
    fbe_u32_t   copied_count = 0;
    fbe_u8_t        *buffer = NULL;
    fbe_u32_t       sg_list_size = 0;
    fbe_u32_t       sg_list_count = 0;
    fbe_u32_t       i = 0;
    fbe_u32_t       aligned_rw_io_cb_size = 0;
    fbe_u32_t       retry_count = 0;


    /*if it is not read flush request but buffer pointer NULL, fail*/
    if (data_ptr == NULL && !is_read_flush) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid arguments\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*current only support two kind of data block size: 520 bytes to read whole block including 
      seqence number and checksum, DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE to read data without raw-mirror
      metadata and checksum*/
    if (data_block_size != DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE && 
        data_block_size != DATABASE_SYSTEM_DB_BLOCK_SIZE) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* round up */
    block_count = (count + data_block_size -1) / data_block_size; 

    /*add extra blocks for raw mirror IO control block, assuming the size 
      smaller than 3 blocks*/
    aligned_rw_io_cb_size = (sizeof (database_raw_mirror_operation_cb_t) + 
                             DATABASE_SYSTEM_DB_BLOCK_SIZE - 1)/DATABASE_SYSTEM_DB_BLOCK_SIZE;
    block_count += aligned_rw_io_cb_size;

    if (block_count < FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET) {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    } else {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    }
    chunk_number = (fbe_memory_number_of_objects_t)((block_count + block_count_chunk_size(chunk_size) - 1) / block_count_chunk_size(chunk_size));

    chunk_number ++;  /* +1 for packet and sg_list */


    fbe_semaphore_init(&mem_alloc_semaphore, 0, 1);
    fbe_zero_memory(&memory_request, sizeof(fbe_memory_request_t));
    fbe_memory_request_init(&memory_request);
    /* Allocate memroy for data */
    fbe_memory_build_chunk_request(&memory_request,
                                   chunk_size,
                                   chunk_number,
                                   DATABASE_SYSTEM_DB_MEM_REQ_PRIORITY,
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                   (fbe_memory_completion_function_t)database_system_db_interface_mem_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   &mem_alloc_semaphore);

    status = fbe_memory_request_entry(&memory_request);
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, &memory_request, memory_request.request_state);
        fbe_semaphore_destroy(&mem_alloc_semaphore);

        return status;
    }

    fbe_semaphore_wait(&mem_alloc_semaphore, NULL);
    /* Check memory allocation status */
    if (fbe_memory_request_is_allocation_complete(&memory_request) == FBE_FALSE)
    {
        /* Currently doesn't expect any aborted requests */
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, &memory_request, memory_request.request_state);
        fbe_semaphore_destroy(&mem_alloc_semaphore);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*memory request done, issue IO now*/
    master_memory_header = fbe_memory_get_first_memory_header(&memory_request);
    memory_chunk_size = master_memory_header->memory_chunk_size; /* valid in master memory header only */

    sg_list_size = (master_memory_header->number_of_chunks) * sizeof(fbe_sg_element_t);
    /* round up the sg list size to align with 64-bit boundary */
    sg_list_size = (sg_list_size + sizeof(fbe_u64_t) - 1) & 0xfffffff8; 

    if ((master_memory_header->memory_chunk_size) < (sizeof(fbe_packet_t)+sg_list_size)) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: chunk_size < packet+sg_lists: %d < %d \n \n",
                       __FUNCTION__,
                       memory_request.chunk_size,
                       (int)(sizeof(fbe_packet_t) + sg_list_size));

        //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
        /* If allocation failed, the memory_ptr may be NULL pointer */        
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(&memory_request);
        
        fbe_semaphore_destroy(&mem_alloc_semaphore);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* carve the packet */
    buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);
    packet = (fbe_packet_t *)buffer;
    fbe_zero_memory(packet, sizeof(fbe_packet_t));
    buffer += sizeof(fbe_packet_t);

    /* carve the sg list */
    sg_list_count = master_memory_header->number_of_chunks - 1; /* minus the first 1 chunk*/
    sg_list_p = (fbe_sg_element_t *)buffer;
    fbe_zero_memory(sg_list_p, sg_list_size);

    /*
      form raw-mirror IO control block. The second memory chunk will divided into two
      parts: raw-mirror IO cb placed at the begin, the remaining memory used for IO
    */
    fbe_memory_get_next_memory_header(master_memory_header, &current_memory_header);
    master_memory_header = current_memory_header;

    /*the beginning part used for raw-mirror IO block*/
    raw_mirr_op_cb = (database_raw_mirror_operation_cb_t *)fbe_memory_header_to_data(current_memory_header);

    /*ramining memory of second memory chunk used for IO*/
    sg_list_p[0].address = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header) + DATABASE_SYSTEM_DB_BLOCK_SIZE * aligned_rw_io_cb_size;
    sg_list_p[0].count = DATABASE_SYSTEM_DB_BLOCK_SIZE * (block_count_chunk_size(memory_chunk_size) - aligned_rw_io_cb_size);

    /* Form SG lists from remaing memory chunk*/
    for (i = 1; i < sg_list_count; i++) {
        /* move to the next chunk */
        fbe_memory_get_next_memory_header(master_memory_header, &current_memory_header);
        master_memory_header = current_memory_header;

        sg_list_p[i].address = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header);
        sg_list_p[i].count = DATABASE_SYSTEM_DB_BLOCK_SIZE * block_count_chunk_size(memory_chunk_size);
        
        fbe_zero_memory(sg_list_p[i].address, sg_list_p[i].count);
    }
    sg_list_p[sg_list_count].address = NULL;
    sg_list_p[sg_list_count].count = 0;

    /*A read operation may overlap with disk failure and the caller need
      re-issue the operation by checking the block operation qualifier.
      retry at most 2 times.*/
    retry_count = DATABASE_RAW_MIRROR_READ_RETRY_COUNT;
    while (retry_count > 0) {
        retry_count --;
        fbe_zero_memory(packet, sizeof (fbe_packet_t));
        /*initialize the raw-mirror io control block*/
        database_utils_raw_mirror_init_operation_cb(raw_mirr_op_cb, start_lba,
                                                    (count + data_block_size - 1) / data_block_size,
                                                    sg_list_p, sg_list_count);

        database_utils_raw_mirror_operation_cb_set_drive_mask(raw_mirr_op_cb, read_mask, write_mask,
                                                              is_read_flush);

        database_util_raw_mirror_set_error_retry(raw_mirr_op_cb,
                                                 FBE_TRUE,
                                                 2);

        /*read data from raw-mirror with 520-bytes sized block, 
          it is synchronous call*/
        status = database_util_raw_mirror_read_blocks(packet, raw_mirr_op_cb);
        /*handle cases:
          1) if read successful, needn't retry;
          2) if read failure, require retry flag set by checking block opeartion qualifier
             in complete rountine, retry it;
        */
        if (status == FBE_STATUS_OK && raw_mirr_op_cb->block_operation_successful || 
            !raw_mirr_op_cb->operation_need_retry) {
            break;
        } 
    }

    /*read complete and copy data if required, handle two cases as follows: 
      1) read for flush, we don't need to copy data back because database_util_raw_mirror_read_blocks
         already did the flush
      2) normal read, copy back data
     */
    if (!is_read_flush) {
        if (status == FBE_STATUS_OK && raw_mirr_op_cb->block_operation_successful) {
            /* Copy the data */
            left_count = count;
            for (i = 0; i < sg_list_count; i++) {
                buffer = sg_list_p[i].address;
                buffer_count = sg_list_p[i].count;
                while ((buffer_count >= data_block_size) && (left_count > 0)) {
                    copied_count = data_block_size;
                    if (left_count <= data_block_size) {
                        copied_count = (fbe_u32_t)left_count;
                    }
                    fbe_copy_memory(data_ptr, buffer, copied_count);
                    data_ptr += copied_count;
                    left_count -= copied_count;

                    /* move forward the sg_list's pointer (520) */
                    buffer += DATABASE_SYSTEM_DB_BLOCK_SIZE;
                    buffer_count -= DATABASE_SYSTEM_DB_BLOCK_SIZE;
                }
                if (left_count <= 0) {
                    /* we have get the data */
                    break;
                }
            }

        } else {
            status = FBE_STATUS_GENERIC_FAILURE;
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: read raw_mirror blocks failed \n", __FUNCTION__);
        }
    } else {
        /*for read/flush opeartion, check if flush success*/
        if (status == FBE_STATUS_OK && raw_mirr_op_cb->need_flush && raw_mirr_op_cb->flush_error &&
            !raw_mirr_op_cb->flush_done) {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
    /* If allocation failed, the memory_ptr may be NULL pointer */
    //fbe_memory_free_entry(memory_ptr);
	fbe_memory_free_request_entry(&memory_request);

	fbe_semaphore_destroy(&mem_alloc_semaphore);

    if (done_count != NULL)
        *done_count = count - left_count;

    return status;
}

/*!***************************************************************
 * @fn database_system_db_interface_mem_allocation_completion
 *****************************************************************
 * @brief
 *   complete rountine of the memory request for raw-mirror read/write
 *
 * @param memory_request  - request to allocate memory
 * @param context  - context of the complete function
 * 
 * @return status - return FBE_STATUS_GENERIC_FAILURE if issue encountered
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
static fbe_status_t
database_system_db_interface_mem_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_semaphore_t *mem_alloc_semaphore = (fbe_semaphore_t *)context;

    if (mem_alloc_semaphore != NULL) {
        fbe_semaphore_release(mem_alloc_semaphore, 0, 1, FALSE); 
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_system_db_interface_write
 *****************************************************************
 * @brief
 *   interface to write data to a contingous region in database raw-mirror,
 *   the supporting  block size is DATABASE_SYSTEM_DB_BLOCK_SIZE. 
 *
 * @param data_ptr   - buffer to store the data wrote
 * @param start_lba  - the start lba of the raw-mirror to be wrote
 * @param count      - the bytes to be wrote to raw-mirror
 * @param done_count - return how many bytes actually wrote
 * 
 * @return status - return FBE_STATUS_GENERIC_FAILURE if issue encountered
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t database_system_db_interface_write(fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t count, fbe_u64_t *done_count)
{
    /*each write should write all the raw-mirror drives to ensure consistency*/
    return database_system_db_interface_general_write(data_ptr, start_lba, count, done_count, FBE_FALSE, DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK);
}


/*!***************************************************************
 * @fn database_system_db_interface_general_write
 *****************************************************************
 * @brief
 *   general interface to write data to a contingous region in database raw-mirror,
 *   the supporting  block size is DATABASE_SYSTEM_DB_BLOCK_SIZE. It also support clear
 *   a contingous region of database raw-mirror, which resets all the data including magic
 *   number and sequence number of each block.
 *
 * @param data_ptr   - buffer to store the data wrote
 * @param start_lba  - the start lba of the raw-mirror to be wrote
 * @param count      - the bytes to be wrote to raw-mirror
 * @param done_count - return how many bytes actually wrote
 * @param is_clear_write - indicating if it is clear operation
 * 
 * @return status - return FBE_STATUS_GENERIC_FAILURE if issue encountered
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t database_system_db_interface_general_write(fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t count, 
                                                        fbe_u64_t *done_count,
                                                        fbe_bool_t is_clear_write,
                                                        fbe_u32_t  write_mask)
{
    fbe_semaphore_t                 mem_alloc_semaphore;
    fbe_memory_request_t            memory_request;
    
    fbe_block_count_t               block_count;
    fbe_memory_chunk_size_t         chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    fbe_memory_number_of_objects_t  chunk_number = 1;
    fbe_status_t                    status;
    fbe_packet_t * packet = NULL;
    fbe_sg_element_t * sg_list_p = NULL;
    fbe_u32_t blocks_number = 0, blocks_number_internal = 0;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_memory_header_t * current_memory_header = NULL;
    fbe_memory_chunk_size_t memory_chunk_size;
    database_raw_mirror_operation_cb_t *raw_mirr_op_cb = NULL;

    fbe_s64_t       left_count = 0;
    fbe_u32_t       copied_count = 0;
    fbe_u8_t        *buffer;
    fbe_u32_t       buffer_count = 0;
    fbe_u32_t       sg_list_size = 0;
    fbe_u32_t       sg_list_count = 0;
    fbe_u32_t       i = 0;
    fbe_u32_t       aligned_rw_io_cb_size = 0;

    if (data_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid arguments\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* round up */
    block_count = (count + DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE -1) / DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE; 

    /*add extra blocks for raw mirror IO control block*/
    aligned_rw_io_cb_size = (sizeof (database_raw_mirror_operation_cb_t) + 
                             DATABASE_SYSTEM_DB_BLOCK_SIZE - 1)/DATABASE_SYSTEM_DB_BLOCK_SIZE;
    block_count += aligned_rw_io_cb_size;

    if (block_count < FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET) {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    }  else {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    }
    chunk_number = (fbe_memory_number_of_objects_t)((block_count + block_count_chunk_size(chunk_size) - 1) / block_count_chunk_size(chunk_size));

    chunk_number ++; /* +1 for packet and sg_list */

    fbe_semaphore_init(&mem_alloc_semaphore, 0, 1);
    fbe_zero_memory(&memory_request, sizeof(fbe_memory_request_t));
    fbe_memory_request_init(&memory_request);
    /* Allocate memroy for data */
    fbe_memory_build_chunk_request(&memory_request,
                                   chunk_size,
                                   chunk_number,
                                   DATABASE_SYSTEM_DB_MEM_REQ_PRIORITY,
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                   (fbe_memory_completion_function_t)database_system_db_interface_mem_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   &mem_alloc_semaphore);
					   

    status = fbe_memory_request_entry(&memory_request);
    /* If fail to allocate the memory */
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {
        fbe_semaphore_destroy(&mem_alloc_semaphore);

        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, &memory_request, memory_request.request_state);
        return status;
    }

    fbe_semaphore_wait(&mem_alloc_semaphore, NULL);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(&memory_request) == FBE_FALSE)
    {
        /* Currently this doesn't expect any aborted requests */
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, &memory_request, memory_request.request_state);
        fbe_semaphore_destroy(&mem_alloc_semaphore);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    master_memory_header = fbe_memory_get_first_memory_header(&memory_request);
    memory_chunk_size = master_memory_header->memory_chunk_size; /* valid in master header only */

    sg_list_size = (master_memory_header->number_of_chunks) * sizeof(fbe_sg_element_t);
    /* round up the sg list size to align with 64-bit boundary */
    sg_list_size = (sg_list_size + sizeof(fbe_u64_t) - 1) & 0xfffffff8; 

    if ((master_memory_header->memory_chunk_size) < (sizeof(fbe_packet_t) + sg_list_size)) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: chunk_size < packet+sg_lists: %d < %d \n",
                       __FUNCTION__,
                       memory_request.chunk_size,
                       (int)(sizeof(fbe_packet_t) + sg_list_size));

        //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
        /* If allocation failed, the memory_ptr may be NULL pointer */        
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(&memory_request);

        fbe_semaphore_destroy(&mem_alloc_semaphore);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* carve the packet */
    buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);
    packet = (fbe_packet_t *)buffer;
    fbe_zero_memory(packet, sizeof(fbe_packet_t));
    buffer += sizeof(fbe_packet_t);

    /* carve the sg list */
    sg_list_count = master_memory_header->number_of_chunks - 1;
    sg_list_p = (fbe_sg_element_t *)buffer;
    fbe_zero_memory(sg_list_p, sg_list_size);

    /* Form SG lists and carve raw-mirror io control block from the second memory header*/
    left_count = count;
    for (i = 0; i < sg_list_count; i++) {
        /* move to the next chunk */
        fbe_memory_get_next_memory_header(master_memory_header, &current_memory_header);
        master_memory_header = current_memory_header;

        /*tricky part: the raw-mirror io control block will be allocated from 
          the second memory chunk*/
        if (i == 0) {
            raw_mirr_op_cb = (database_raw_mirror_operation_cb_t *)fbe_memory_header_to_data(current_memory_header);
            buffer = sg_list_p[i].address = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header) + aligned_rw_io_cb_size * DATABASE_SYSTEM_DB_BLOCK_SIZE;
            buffer_count = DATABASE_SYSTEM_DB_BLOCK_SIZE * (block_count_chunk_size(memory_chunk_size) - aligned_rw_io_cb_size);
            sg_list_p[i].count = 0;
        } else {
            buffer = sg_list_p[i].address = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header);
            buffer_count = DATABASE_SYSTEM_DB_BLOCK_SIZE * block_count_chunk_size(memory_chunk_size);
            sg_list_p[i].count = 0;
        }

        fbe_zero_memory(buffer, buffer_count);
        blocks_number_internal = 0;
        while ((buffer_count >= DATABASE_SYSTEM_DB_BLOCK_SIZE) && (left_count > 0)) {
            copied_count = DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE;
            if (left_count <= DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE) {
                copied_count = (fbe_u32_t)left_count;
            }
            fbe_copy_memory(buffer, data_ptr, copied_count);
            data_ptr += copied_count;
            left_count -= copied_count;

            /* move forward the sg_list's pointer (520) */
            buffer += DATABASE_SYSTEM_DB_BLOCK_SIZE;
            buffer_count -= DATABASE_SYSTEM_DB_BLOCK_SIZE;
            sg_list_p[i].count += DATABASE_SYSTEM_DB_BLOCK_SIZE;
            blocks_number_internal++;
        }
        database_system_db_interface_db_calculate_checksum(sg_list_p[i].address, start_lba + blocks_number, 
                                                           blocks_number_internal,
                                                           is_clear_write);
        blocks_number += blocks_number_internal;

        if (left_count <= 0) {
            /* we have get the data */
            i++;
            break;
        }
    }
    sg_list_p[i].address = NULL;
    sg_list_p[i].count = 0;

    /*initialize the raw-mirror IO control block*/
    database_utils_raw_mirror_init_operation_cb(raw_mirr_op_cb, start_lba,
                                                blocks_number,
                                                sg_list_p,
                                                sg_list_count);

    database_utils_raw_mirror_operation_cb_set_drive_mask(raw_mirr_op_cb, DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK, write_mask,
                                                          FBE_FALSE);

    database_util_raw_mirror_set_error_retry(raw_mirr_op_cb,
                                             FBE_TRUE,
                                             2);

    /*write data to raw-mirror with error check and flush to correct 
      the correctable errors. it is synchronous call*/
    status = database_util_raw_mirror_write_blocks(packet, raw_mirr_op_cb);

    if (status == FBE_STATUS_OK && raw_mirr_op_cb->block_operation_successful) {
        status = FBE_STATUS_OK;
    } else {
        status = FBE_STATUS_GENERIC_FAILURE;
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s failed to write", __FUNCTION__);
    }

    //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
    /* If allocation failed, the memory_ptr may be NULL pointer */    
    //fbe_memory_free_entry(memory_ptr);    
	fbe_memory_free_request_entry(&memory_request);

    fbe_semaphore_destroy(&mem_alloc_semaphore);

    if (done_count != NULL)
        *done_count = count - left_count;

    return status;
}


/*!***************************************************************
 * @fn database_system_db_interface_db_calculate_checksum
 *****************************************************************
 * @brief
 *   interface to calculate the checksum, set magic number and sequence
 *   number for database raw-mirror blocks. 
 *
 * @param block_p   - buffer to store the blocks to be written
 * @param start_lba  - the start lba of the raw-mirror to be caculated checksum
 * @param size      - the block count need to be calculated checksum
 * @param seq_reset - flag indicating if required to reset the sequence number
 *                    to be start sequence
 * 
 * @return status - return FBE_STATUS_GENERIC_FAILURE if issue encountered
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
static fbe_status_t
database_system_db_interface_db_calculate_checksum(void * block_p, fbe_lba_t start_lba,
                                                   fbe_u32_t size, fbe_bool_t seq_reset)
{
   fbe_u32_t block_index;
   fbe_u32_t * reference_ptr;
   fbe_u64_t seq = 0;

   /* If input pointer is null then return error. */
   if(block_p == NULL)
   {
       database_trace(FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: NULL pointer\n",
                      __FUNCTION__);
       return FBE_STATUS_GENERIC_FAILURE;
   }
   
   /* Calculate the checksum and place it as a metadata on block. */
   for(block_index = 0; block_index < size; block_index++) {
       if (!seq_reset) {
            fbe_database_util_raw_mirror_io_get_seq(start_lba + block_index,
                                                             FBE_TRUE, &seq);
       } else
           seq = DATABASE_RAW_MIRROR_START_SEQ;

       fbe_xor_lib_raw_mirror_sector_set_seq_num(block_p, seq);

       /* skipping by 512 so skips data and raw mirror metadata; revisit in general for best overall solution.
        */
       reference_ptr = ( fbe_u32_t * ) ((char *)block_p + DATABASE_SYSTEM_DB_RAW_BLOCK_DATA_SIZE);
       *reference_ptr = fbe_xor_lib_calculate_checksum((fbe_u32_t*)block_p);
       reference_ptr++;

       *reference_ptr = 0;

       block_p = (char *) block_p + DATABASE_SYSTEM_DB_BLOCK_SIZE;
   }

   return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_utils_raw_mirror_init_operation_cb
 *****************************************************************
 * @brief
 *   interface to calculate the checksum, set magic number and sequence
 *   number for database raw-mirror blocks. 
 *
 * @param raw_mirr_op_cb   - raw-mirror io control block to be initialized
 * @param lba  - the start lba of the raw-mirror IO
 * @param block_count   - the block count the raw-mirror IO invloves
 * @param sg_list - well formed scatter-gather list of the raw-mirror IO
 * @param sg_list_count - size of the scatter-gather list
 * 
 * @return no
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
void database_utils_raw_mirror_init_operation_cb(database_raw_mirror_operation_cb_t *raw_mirr_op_cb, fbe_lba_t lba,
                                                 fbe_block_count_t block_count, fbe_sg_element_t *sg_list,
                                                 fbe_u32_t sg_list_count)
{
    if (raw_mirr_op_cb != NULL) {
        fbe_zero_memory(raw_mirr_op_cb, sizeof (database_raw_mirror_operation_cb_t));
        raw_mirr_op_cb->lba = lba;
        raw_mirr_op_cb->block_count = block_count;
        raw_mirr_op_cb->sg_list = sg_list;
        raw_mirr_op_cb->sg_list_count = sg_list_count;
        raw_mirr_op_cb->flush_error = FBE_FALSE;
        raw_mirr_op_cb->flush_retry_count = 0;

        /*initialy no disk error in read/write*/
        raw_mirr_op_cb->error_disk_bitmap = 0;
        raw_mirr_op_cb->flush_done = FBE_FALSE;
        raw_mirr_op_cb->need_flush = FBE_FALSE;
        raw_mirr_op_cb->block_operation_successful = FBE_FALSE;
        raw_mirr_op_cb->operation_need_retry = FBE_FALSE;

        raw_mirr_op_cb->force_flush = FBE_FALSE;

        /*set the default read/write mask*/
        raw_mirr_op_cb->read_drive_mask = raw_mirr_op_cb->write_drive_mask = DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK;

        fbe_zero_memory(&raw_mirr_op_cb->error_verify_report,sizeof (fbe_raid_verify_raw_mirror_errors_t));
    }

}


/*!***************************************************************
 * @fn database_utils_raw_mirror_operation_cb_set_drive_mask
 *****************************************************************
 * @brief
 *   set the read/write drive mask for this raw-mirror IO. 
 *
 * @param raw_mirr_op_cb   - raw-mirror io control block to be initialized
 * @param read_drive_mask  - the drive set want to read from
 * @param write_drive_mask  - the drive set want to write to
 * 
 * @return no
 *
 * @version
 *    5/24/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
void database_utils_raw_mirror_operation_cb_set_drive_mask(database_raw_mirror_operation_cb_t *raw_mirr_op_cb, 
                                                           fbe_u32_t  read_drive_mask,
                                                           fbe_u32_t  write_drive_mask,
                                                           fbe_bool_t force_flush)
{
    if (raw_mirr_op_cb != NULL) {
        raw_mirr_op_cb->read_drive_mask = read_drive_mask;
        raw_mirr_op_cb->write_drive_mask = write_drive_mask;
        raw_mirr_op_cb->force_flush = force_flush;
    }
}


/*!***************************************************************
 * @fn database_util_raw_mirror_set_error_retry
 *****************************************************************
 * @brief
 *   interface to calculate the checksum, set magic number and sequence
 *   number for database raw-mirror blocks. 
 *
 * @param raw_mirr_op_cb   - raw-mirror io control block to be initialized
 * @param flush_err  - indicate if we check and flush error
 * @param flush_retry_count   - how many times we retry flush
 * 
 * @return no
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t database_util_raw_mirror_set_error_retry(database_raw_mirror_operation_cb_t *rw_mirr_op_cb,
                                                            fbe_bool_t flush_err,
                                                            fbe_u32_t flush_retry_count)
{
    if (!rw_mirr_op_cb) 
        return FBE_STATUS_GENERIC_FAILURE;

    rw_mirr_op_cb->flush_error = flush_err;
    rw_mirr_op_cb->flush_retry_count = flush_retry_count;

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_raw_mirror_io_cb_init
 *****************************************************************
 * @brief
 *   initialize the global database raw-mirror IO state 
 *   tracking structure. 
 *
 * @param rw_mirr_cb   - global database raw-mirror tracking structure
 * 
 * @return status - indicating if the initialization success
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t fbe_database_raw_mirror_io_cb_init(fbe_database_raw_mirror_io_cb_t *rw_mirr_cb)
{
    fbe_lba_t offset;
    fbe_block_count_t capacity;


    if (!rw_mirr_cb) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*initialize raw-mirror io control block fields*/
    fbe_zero_memory(rw_mirr_cb, sizeof (fbe_database_raw_mirror_io_cb_t));
    database_system_objects_get_system_db_offset_capacity(&offset, NULL, &capacity);
    rw_mirr_cb->block_count = capacity;
    rw_mirr_cb->seq_numbers = (fbe_u64_t *)fbe_memory_ex_allocate((fbe_u32_t)capacity * sizeof (fbe_u64_t));
    if (!rw_mirr_cb->seq_numbers) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(rw_mirr_cb->seq_numbers, (fbe_u32_t)capacity * sizeof (fbe_u64_t));


    return FBE_STATUS_OK;
}
 
/*!***************************************************************
 * @fn fbe_database_raw_mirror_io_cb_destroy
 *****************************************************************
 * @brief
 *   interface to release the resource of global database raw-mirror
 *   IO state tracking structure. 
 *
 * @param rw_mirr_cb   - global database raw-mirror IO state tracking
 *                       structure
 * 
 * @return status - indicating if the resource released successfully
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t fbe_database_raw_mirror_io_cb_destroy(fbe_database_raw_mirror_io_cb_t *rw_mirr_cb)
{

    if (!rw_mirr_cb) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*the database raw-mirror rebuild thread should already be destroyed before*/

    //fbe_spinlock_destroy(&rw_mirr_cb->lock);

    if (rw_mirr_cb->seq_numbers) {
        fbe_memory_ex_release(rw_mirr_cb->seq_numbers);
    }


    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_system_db_destroy
 *****************************************************************
 * @brief
 *   interface to release the resource allocated for database system
 *   db 
 *
 * @param no
 * 
 * @return no
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
void fbe_database_system_db_destroy(void)
{
	fbe_raw_mirror_destroy(&fbe_database_system_db);
	fbe_database_raw_mirror_io_cb_destroy(&fbe_database_raw_mirror_io_cb);
}

/*!***************************************************************
 * @fn database_util_raw_mirror_read_blocks
 *****************************************************************
 * @brief
 *   Util function to read 520-byte sized blocks from raw-mirror 
 *   into a buffer (SG list) with error check and correction. It 
 *   is a synchronous call. 
 *
 * @param packet   - packet memory allocated by the caller
 * @param raw_mirr_op_cb  - the database raw-mirror IO control block:
 *        1. sg_list and sg_list_count contains read buffer
 *        2. lba and block_count specify the block address and size
 *        3. flush_retry_count:  how many times to retry flush if first
             flush failed
 * 
 * @return status - return FBE_STATUS_OK if read successful, 
 *                  FBE_STATUS_GENERIC_FAILURE otherwise
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
/*
  NOTES:
  1) packet memory allocated by caller and caller responsible for releasing it. The
     function will intialize and destroy it;
  2) read error will be checked and flush operation will be issued with write/verify
  3) block_operation_success field in database_raw_mirror_operation_cb_t will
     indicate if the read itself succeed;
 
 
  The function will handle the following error cases:
  1) read operation successful, magic number un-match, means one drive is not DB drive;
  2) read operation successful, sequence number un-match, means some drive has old data;
  3) read operation itself failed, failure will return to caller;
*/

fbe_status_t database_util_raw_mirror_read_blocks(fbe_packet_t *packet, 
                                                  database_raw_mirror_operation_cb_t *raw_mirr_op_cb)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_block_operation_t *block_operation = NULL;
    fbe_cpu_id_t cpu_id; 

    if (!packet || !raw_mirr_op_cb || !raw_mirr_op_cb->sg_list) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);    
    payload = fbe_transport_get_payload_ex(packet);
	fbe_payload_ex_allocate_memory_operation(payload);
    block_operation =  fbe_payload_ex_allocate_block_operation(payload);

    cpu_id = fbe_get_cpu_id();
	fbe_transport_set_cpu_id(packet, cpu_id);
    fbe_payload_ex_set_sg_list(payload, raw_mirr_op_cb->sg_list, raw_mirr_op_cb->sg_list_count);

    /*set raw-mirror error verify report to record the IO errors on 
      raw-mirror*/
    fbe_zero_memory(&raw_mirr_op_cb->error_verify_report, sizeof (fbe_raid_verify_raw_mirror_errors_t));
    fbe_payload_ex_set_verify_error_count_ptr(payload, &raw_mirr_op_cb->error_verify_report);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      raw_mirr_op_cb->lba,    /* LBA */
                                      raw_mirr_op_cb->block_count,
                                      DATABASE_SYSTEM_DB_BLOCK_SIZE,    /* block size: 520  */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Set completion function */
    fbe_semaphore_init(&raw_mirr_op_cb->semaphore,0,1);
    fbe_transport_set_completion_function(packet, database_raw_mirror_blocks_read_completion, raw_mirr_op_cb);
    fbe_payload_ex_increment_block_operation_level(payload);


    /*temporary the in_bitmask is 7, it will be the pass in parameter of the caller function
           when raw mirror common utils are finished
      */
    status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(&fbe_database_system_db, packet, raw_mirr_op_cb->read_drive_mask);
    if (status != FBE_STATUS_OK) {
        fbe_semaphore_destroy(&raw_mirr_op_cb->semaphore);
    } else {
        fbe_semaphore_wait(&raw_mirr_op_cb->semaphore, NULL);
        fbe_semaphore_destroy(&raw_mirr_op_cb->semaphore);
    }
    return status;
    
}


/*!***************************************************************
 * @fn database_raw_mirror_blocks_read_completion
 *****************************************************************
 * @brief
 *   complete function for database_util_raw_mirror_read_blocks, it
 *   will check the error report structure and block operation status
 *   to determine how to fix the errors reported by raw-mirror raid
 *   library. 
 *
 * @param packet   - the IO packet
 * @param context  - context of the complete function run
 * 
 * @return status - indicating if the complete function success
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
static fbe_status_t 
database_raw_mirror_blocks_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_status_t status;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    database_raw_mirror_operation_cb_t *raw_mirror_op_cb = NULL;
    fbe_raid_verify_raw_mirror_errors_t *error_verify_report = NULL;
    fbe_bool_t  flush_needed = FBE_FALSE;

    /*retrieve the context info*/
    raw_mirror_op_cb = (database_raw_mirror_operation_cb_t *)context;
    if (!raw_mirror_op_cb) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed by no context\n",
                       __FUNCTION__);
        fbe_transport_destroy_packet(packet); /* This will not release the memory */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);
    fbe_payload_ex_get_verify_error_count_ptr(payload, (void **)&error_verify_report);

    /*now starts the read error handling: 
      1. check packet status, if not ok, return failure directly
      2. check block operation status, if failure, return failure directly;
         if FBE_STATUS_OK:
         1) copy the data to application buffer, because the block operation succeed;
         2) check the block operation status qualifier and error report, if error exist,
            synchrously issue a write-verify to flush
      */
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: read failed: status =%d, block_operation_status = %d\n",
                       __FUNCTION__, status, block_operation_status);
        fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_destroy_packet(packet); /* This will not release the memory */
        fbe_semaphore_release(&raw_mirror_op_cb->semaphore, 0, 1, FALSE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        raw_mirror_op_cb->block_operation_successful = FBE_TRUE;
        /* We are done copying read data and start check errors */
        flush_needed = FBE_FALSE;

        /*1. magic and sequence errors*/
        if (error_verify_report != NULL) {
            if (error_verify_report->raw_mirror_seq_bitmap != 0 ||
                error_verify_report->raw_mirror_magic_bitmap != 0) {
                database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "database_raw_mirr_read error report:, seq_bitmap %x, magic_bitmap %x\n",
                       error_verify_report->raw_mirror_seq_bitmap,
                       error_verify_report->raw_mirror_magic_bitmap);
                    /*magic number or sequence numbers don't match, need flush*/
                    flush_needed = FBE_TRUE;
                raw_mirror_op_cb->error_disk_bitmap = 
                    (error_verify_report->raw_mirror_seq_bitmap | error_verify_report->raw_mirror_magic_bitmap);
            }
        } 

        if (block_operation_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH) {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "database_raw_mirr_read sequence not match");
                flush_needed = FBE_TRUE;
        }

        /*in the read error handling case,  
          we can flush to correct errors only after we have read good data
          successfully
          */
        raw_mirror_op_cb->need_flush = flush_needed;
    } else {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "DB raw-mirr read operation failed, status: %d, qualifier %d \n", 
                       block_operation_status, block_operation_qualifier);
        raw_mirror_op_cb->block_operation_successful = FBE_FALSE;
        if (block_operation_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE || 
            block_operation_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED) {
            raw_mirror_op_cb->operation_need_retry = FBE_TRUE;
        }
        /*currently retry any failed read because detailed retryable block operation 
          qualifier need to be figured out.  must change if got the detailed qualifier*/
        raw_mirror_op_cb->operation_need_retry = FBE_TRUE;
    }


    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_destroy_packet(packet);
    fbe_semaphore_release(&raw_mirror_op_cb->semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_util_raw_mirror_write_verify_blocks
 *****************************************************************
 * @brief
 *   Util functions for write/verify a buffer into the raw-3way-mirror with 
 *   op code FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY.  Data buffer
 *   SG list and block address will be set by caller in the structure
 *   database_raw_mirror_operation_cb_t. 
 *
 * @param raw_mirr_op_cb   - raw-mirror io control block for the write/verify
 *                           operation
 * 
 * @return status  - FBE_STATUS_OK:  write/verify successful
 *                 - other:          write/verify failed
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
/*
  NOTES that: 
  1) Caller should be responsible for initializing sg_list,
     lba, count, semaphore in database_raw_mirror_operation_cb_t;
  2) packet will be allcoated and released inside the function
  3) If error detected in the operation, it will NOT flush it and
     just set flush_done to FALSE in database_raw_mirror_operation_cb_t; The
     caller should determine the action;
*/

fbe_status_t database_util_raw_mirror_write_verify_blocks(database_raw_mirror_operation_cb_t *raw_mirr_op_cb)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_block_operation_t *block_operation = NULL;
    fbe_cpu_id_t cpu_id; 
    fbe_packet_t *packet = NULL;

    packet = fbe_transport_allocate_packet();
    if (!packet || !raw_mirr_op_cb || !raw_mirr_op_cb->sg_list) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);    
    payload = fbe_transport_get_payload_ex(packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(payload);

    cpu_id = fbe_get_cpu_id();
	fbe_transport_set_cpu_id(packet, cpu_id);
    /* Set sg list */
    fbe_payload_ex_set_sg_list(payload, raw_mirr_op_cb->sg_list, 
                               raw_mirr_op_cb->sg_list_count);

    /*set raw-mirror error verify report to record the IO errors on 
      raw-way-mirror*/
    fbe_zero_memory(&raw_mirr_op_cb->error_verify_report, sizeof (fbe_raid_verify_raw_mirror_errors_t));
    fbe_payload_ex_set_verify_error_count_ptr(payload, &raw_mirr_op_cb->error_verify_report);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY,
                                      raw_mirr_op_cb->lba,    /* LBA */
                                      raw_mirr_op_cb->block_count,
                                      DATABASE_SYSTEM_DB_BLOCK_SIZE,    /* block size: 520  */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Set completion function */
    fbe_semaphore_init(&raw_mirr_op_cb->semaphore,0,1);
    fbe_transport_set_completion_function(packet, database_raw_mirror_blocks_write_verify_completion, raw_mirr_op_cb);

    fbe_payload_ex_increment_block_operation_level(payload);
    if (fbe_raw_mirror_is_enabled(&fbe_database_system_db)){

        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "system write verify raw mirror library lba: 0x%llx bl: 0x%llx\n", 
                       (unsigned long long)raw_mirr_op_cb->lba, (unsigned long long)raw_mirr_op_cb->block_count);
        status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(&fbe_database_system_db, packet, raw_mirr_op_cb->write_drive_mask);
    }
    else {
        /* After the system boots, we disable the raw mirror and send I/Os directly to the LUN on the raw mirror RG.
         */
        fbe_object_id_t sys_config_lun_id;
        fbe_database_get_raw_mirror_config_lun_id(&sys_config_lun_id);
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "system sb write verify raw mirror lun object_id 0x%x lba: 0x%llx bl: 0x%llx\n", 
                       sys_config_lun_id, (unsigned long long)raw_mirr_op_cb->lba, (unsigned long long)raw_mirr_op_cb->block_count);
    
        fbe_transport_set_address(packet, FBE_PACKAGE_ID_SEP_0, FBE_SERVICE_ID_TOPOLOGY, 
                                  FBE_CLASS_ID_INVALID, sys_config_lun_id);
        packet->base_edge = NULL;
        status = fbe_topology_send_io_packet(packet);
    }
    if (status != FBE_STATUS_OK) {
        /*send packet failed, we needn't wait for the callback*/
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "database_raw_mirr_write_verify failed to send packet ");

    } else {
        fbe_semaphore_wait(&raw_mirr_op_cb->semaphore, NULL);
    }

    fbe_semaphore_destroy(&raw_mirr_op_cb->semaphore);

    return status;
}

/*!***************************************************************
 * @fn database_raw_mirror_blocks_write_verify_completion
 *****************************************************************
 * @brief
 *   complete function for database_util_raw_mirror_write_verify_blocks, it
 *   will check the error report structure and block operation status
 *   to determine how to fix the errors reported by raw-mirror raid
 *   library. 
 *
 * @param packet   - the IO packet
 * @param context  - context of the complete function run
 * 
 * @return status - indicating if the complete function success
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
static fbe_status_t 
database_raw_mirror_blocks_write_verify_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_status_t status;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    database_raw_mirror_operation_cb_t *raw_mirror_op_cb = NULL;
    fbe_raid_verify_raw_mirror_errors_t *error_verify_report = NULL;
    fbe_bool_t  flush_needed = FBE_FALSE;

    /*retrieve the context info*/
    raw_mirror_op_cb = (database_raw_mirror_operation_cb_t *)context;
    if (!raw_mirror_op_cb) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed by no context\n",
                       __FUNCTION__);
        fbe_transport_release_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);
    fbe_payload_ex_get_verify_error_count_ptr(payload, (void **)&error_verify_report);

    /*now starts the write_verify error handling: 
      1. check packet status, if not ok, return failure directly
      */
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: read failed: status =%d, block_operation_status = %d\n",
                       __FUNCTION__, status, block_operation_status);
        fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_release_packet(packet);
        fbe_semaphore_release(&raw_mirror_op_cb->semaphore, 0, 1, FALSE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    flush_needed = FBE_FALSE;

    /*2. check block operation status*/
    if (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        raw_mirror_op_cb->block_operation_successful = FBE_TRUE;
        if (block_operation_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH) {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "database_raw_mirr_read detected sequence not match");
                flush_needed = FBE_TRUE;
        }
    } else {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "DB raw-mirr write/verify operation failed, status: %d, qualifier %d \n", 
                       block_operation_status, block_operation_qualifier);
        flush_needed = FBE_TRUE;
        raw_mirror_op_cb->block_operation_successful = FBE_FALSE;
    }

    /*flush done only when no errors detected in this write/verify operation*/
    raw_mirror_op_cb->flush_done = !flush_needed;

    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_release_packet(packet);
    fbe_semaphore_release(&raw_mirror_op_cb->semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn database_utils_raw_mirror_general_read
 *****************************************************************
 * @brief
 *   Read 520 bytes size blocks from raw-mirror into a buffer. The 
 *   buffer should be allocated by user and large enough to hold 
 *   the whole blocks 
 *
 * @param data_ptr   - buffer to store the read data
 * @param start_lba  - the start lba of the read
 * @param count   - the block count the raw-mirror IO invloves
 * 
 *  @return status  - FBE_STATUS_OK:  write/verify successful
 *                 - other:          write/verify failed
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************
*/
fbe_status_t database_utils_raw_mirror_general_read(fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u32_t count, fbe_u32_t read_mask)
{

    if (!data_ptr) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*read and return 520-byte block as data*/
    return database_system_db_interface_general_read(data_ptr, start_lba, count, 
                                                     DATABASE_SYSTEM_DB_BLOCK_SIZE,
                                                     NULL, FBE_FALSE, read_mask, DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK);
}


/*!***************************************************************
 * @fn database_util_raw_mirror_write_blocks
 *****************************************************************
 * @brief
 *   Util function to write 520-byte sized blocks into raw-mirror with error
 *   check and correction. It is a synchronous call. 
 *
 * @param packet   - packet memory allocated by the caller
 * @param raw_mirr_op_cb  - the database raw-mirror IO control block:
 *        1. sg_list and sg_list_count contains read buffer
 *        2. lba and block_count specify the block address and size
 *        3. flush_retry_count:  how many times to retry flush if first
             flush failed
 * 
 *  @return status  - FBE_STATUS_OK:  write/verify successful
 *                 - other:          write/verify failed
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
/*
  NOTES:
  1) packet memory allocated by caller and caller responsible for releasing it. The
     function will intialize and destroy it;
  2) read error will be checked and flush operation will be issued with write/verify
  3) block_operation_success field in database_raw_mirror_operation_cb_t will
     indicate if the write itself succeed;
 
 
  The function will handle the following error cases:
  1) write operation failure, a write/verify will be issued to retry the write
  3) if write/verify failed, return failure to caller;

*/

fbe_status_t database_util_raw_mirror_write_blocks(fbe_packet_t *packet, 
                                                  database_raw_mirror_operation_cb_t *raw_mirr_op_cb)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_block_operation_t *block_operation = NULL;
    fbe_cpu_id_t cpu_id; 
    fbe_u32_t retry_num = 0;

    if (!packet || !raw_mirr_op_cb || !raw_mirr_op_cb->sg_list) {
        return FBE_STATUS_GENERIC_FAILURE;
    }


    fbe_transport_initialize_sep_packet(packet);    
    payload = fbe_transport_get_payload_ex(packet);
	fbe_payload_ex_allocate_memory_operation(payload);
    block_operation =  fbe_payload_ex_allocate_block_operation(payload);

    cpu_id = fbe_get_cpu_id();
	fbe_transport_set_cpu_id(packet, cpu_id);
    fbe_payload_ex_set_sg_list(payload, raw_mirr_op_cb->sg_list, raw_mirr_op_cb->sg_list_count);

    /*set raw-mirror error verify report to record the IO errors on 
      raw-way-mirror*/
    fbe_zero_memory(&raw_mirr_op_cb->error_verify_report, sizeof (fbe_raid_verify_raw_mirror_errors_t));
    fbe_payload_ex_set_verify_error_count_ptr(payload, &raw_mirr_op_cb->error_verify_report);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      raw_mirr_op_cb->lba,    /* LBA */
                                      raw_mirr_op_cb->block_count,
                                      DATABASE_SYSTEM_DB_BLOCK_SIZE,    /* block size: 520  */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Set completion function */
    fbe_semaphore_init(&raw_mirr_op_cb->semaphore,0,1);
    fbe_transport_set_completion_function(packet, database_raw_mirror_blocks_write_completion, raw_mirr_op_cb);
    fbe_payload_ex_increment_block_operation_level(payload);


    if (fbe_raw_mirror_is_enabled(&fbe_database_system_db)){

        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "system persist raw mirror library lba: 0x%llx bl: 0x%llx\n", 
                       (unsigned long long)raw_mirr_op_cb->lba, (unsigned long long)raw_mirr_op_cb->block_count);
        status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(&fbe_database_system_db, packet, raw_mirr_op_cb->write_drive_mask);
    }
    else {
        /* After the system boots, we disable the raw mirror and send I/Os directly to the LUN on the raw mirror RG.
         */
        fbe_object_id_t sys_config_lun_id;
        fbe_database_get_raw_mirror_config_lun_id(&sys_config_lun_id);
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "system sb persist raw mirror lun object_id 0x%x lba: 0x%llx bl: 0x%llx\n", 
                       sys_config_lun_id, (unsigned long long)raw_mirr_op_cb->lba, (unsigned long long)raw_mirr_op_cb->block_count);
    
        fbe_transport_set_address(packet, FBE_PACKAGE_ID_SEP_0, FBE_SERVICE_ID_TOPOLOGY, 
                                  FBE_CLASS_ID_INVALID, sys_config_lun_id);
        packet->base_edge = NULL;
        fbe_topology_send_io_packet(packet);
        /* The packet is always completed.  The submission status is not important.
         */
        status = FBE_STATUS_OK;
    }
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {
        fbe_semaphore_destroy(&raw_mirr_op_cb->semaphore);
    } else {
        fbe_semaphore_wait(&raw_mirr_op_cb->semaphore, NULL);
        fbe_semaphore_destroy(&raw_mirr_op_cb->semaphore);

        /*currently we will retry any kind of write errors with write-verify; 
          need_flush flag set means errors detected*/
        if (raw_mirr_op_cb->need_flush && raw_mirr_op_cb->flush_error) {
            for (retry_num = 0; retry_num < raw_mirr_op_cb->flush_retry_count + 1; retry_num ++) {
                /*issue write-verify with the data read to correct the errors*/
                status = database_util_raw_mirror_write_verify_blocks(raw_mirr_op_cb);
                if (status != FBE_STATUS_OK) {
                    /*send packet failed, we needn't wait for the callback*/
                    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                   "database_raw_mirr_write failed to send write-verify packet ");
                    break;
        
                } else {
                    if (raw_mirr_op_cb->flush_done == FBE_TRUE) {
                        break;
                    }
                }
            }

            /*if original write operation failed but write-verify succeed, 
              set original operation successful*/
            if (raw_mirr_op_cb->flush_done && !raw_mirr_op_cb->block_operation_successful) {
                raw_mirr_op_cb->block_operation_successful = FBE_TRUE;
            }
        }

    }



    return status;
}


/*!***************************************************************
 * @fn database_raw_mirror_blocks_write_completion
 *****************************************************************
 * @brief
 *   complete function for database_util_raw_mirror_write_blocks, it
 *   will check the error report structure and block operation status
 *   to determine how to fix the errors reported by raw-mirror raid
 *   library. 
 *
 * @param packet   - the IO packet
 * @param context  - context of the complete function run
 * 
 * @return status - indicating if the complete function success
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
static fbe_status_t 
database_raw_mirror_blocks_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_status_t status;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    database_raw_mirror_operation_cb_t *raw_mirror_op_cb = NULL;
    fbe_raid_verify_raw_mirror_errors_t *error_verify_report = NULL;
    fbe_bool_t  flush_needed = FBE_FALSE;

    /*retrieve the context info*/
    raw_mirror_op_cb = (database_raw_mirror_operation_cb_t *)context;
    if (!raw_mirror_op_cb) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed by no context\n",
                       __FUNCTION__);
        fbe_transport_destroy_packet(packet); /* This will not release the memory */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);
    fbe_payload_ex_get_verify_error_count_ptr(payload, (void **)&error_verify_report);

    /*now starts the read error detection */

    /*1. check packet status*/
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: read failed: status =%d, block_operation_status = %d\n",
                       __FUNCTION__, status, block_operation_status);
        fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_destroy_packet(packet); /* This will not release the memory */
        fbe_semaphore_release(&raw_mirror_op_cb->semaphore, 0, 1, FALSE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*2. The write block operation failure will cause set need_flush flag and 
      re-issue a write/verify*/
    flush_needed = FBE_FALSE;

    if (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        raw_mirror_op_cb->block_operation_successful = FBE_TRUE;
    } else {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "DB raw-mirr write operation failed, status: %d, qualifier %d \n", 
                       block_operation_status, block_operation_qualifier);
        raw_mirror_op_cb->block_operation_successful = FBE_FALSE;
        flush_needed = FBE_TRUE;
    }

    raw_mirror_op_cb->need_flush = flush_needed;

    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_destroy_packet(packet);
    fbe_semaphore_release(&raw_mirror_op_cb->semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_util_raw_mirror_io_get_seq
 *****************************************************************
 * @brief
 *   get the sequence number of a raw-mirror block, it will check and ensure
 *   the sequence number is in a specified range
 *
 * @param lba  - the block address to get the sequence number
 * @param inc   - indicating if increase the sequence number before returning it
 * @param seq - return the got sequence number
 * 
 * @return status - indicating if we can get the sequence number successfully
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
/**/
fbe_status_t fbe_database_util_raw_mirror_io_get_seq(fbe_lba_t lba, fbe_bool_t inc,
                                                  fbe_u64_t *seq)
{

    if (!seq || lba >= fbe_database_raw_mirror_io_cb.block_count) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (inc) {
        if (fbe_database_raw_mirror_io_cb.seq_numbers[lba] >= DATABASE_RAW_MIRROR_END_SEQ) {
            fbe_database_raw_mirror_io_cb.seq_numbers[lba] = DATABASE_RAW_MIRROR_START_SEQ;
        } else
            fbe_database_raw_mirror_io_cb.seq_numbers[lba] ++;
    }
    *seq = fbe_database_raw_mirror_io_cb.seq_numbers[lba];

    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_database_util_raw_mirror_get_error_report
 *****************************************************************
 * @brief
 *   util function to retrieve the error handling report for database
 *   raw-mirror. 
 *
 * @param report   - structure to record the database raw-mirror IO
                     error handling
 * 
 * @return status - indicating if the report can be got successfully
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
/*it is an interface to return database raw-mirror error report to test or clients with a FBE API*/
fbe_status_t fbe_database_util_raw_mirror_get_error_report(fbe_database_control_raw_mirror_err_report_t *report)
{
    if (!report) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(report, sizeof (fbe_database_control_raw_mirror_err_report_t));

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_util_raw_mirror_check_error_counts
 *****************************************************************
 * @brief
 *   sum the total errors of error verify report and return the num
 *   according to the error category. 
 *
 * @param error_verify_report   - error report structure returned by
 *                                database raw-mirror raid library
 * @param c_errors  - total correctable errors count
 * @param u_errors   - total un-correctable errors count
 * @param retryable_errors - total retryable errors count
 * @param non_retryable_errors - total non-retryable errors count
 * 
 * @return status  - indicating if the function success
 *
 * @version
 *    1/5/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
/**/
fbe_status_t database_util_raw_mirror_check_error_counts(fbe_raid_verify_raw_mirror_errors_t *error_verify_report,
                                                         fbe_u64_t *c_errors,
                                                         fbe_u64_t *u_errors,
                                                         fbe_u64_t *shutdown_errors,
                                                         fbe_u64_t *retryable_errors,
                                                         fbe_u64_t *non_retryable_errors)
{
    if (!error_verify_report) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*sum all the correctable errors*/
    if (c_errors != NULL) {
        *c_errors = 0;
        *c_errors += error_verify_report->verify_errors_counts.c_coh_count;
        *c_errors += error_verify_report->verify_errors_counts.c_crc_count;
        *c_errors += error_verify_report->verify_errors_counts.c_crc_multi_count;
        *c_errors += error_verify_report->verify_errors_counts.c_crc_single_count;
        *c_errors += error_verify_report->verify_errors_counts.c_media_count;
        *c_errors += error_verify_report->verify_errors_counts.c_rm_magic_count;
        *c_errors += error_verify_report->verify_errors_counts.c_rm_seq_count;
        *c_errors += error_verify_report->verify_errors_counts.c_soft_media_count;
        *c_errors += error_verify_report->verify_errors_counts.c_ts_count;
        *c_errors += error_verify_report->verify_errors_counts.c_ws_count;
    }

    /*sum all the non-correctable errors*/
    if (u_errors != NULL) {
        *u_errors = 0;
        *u_errors += error_verify_report->verify_errors_counts.u_coh_count;
        *u_errors += error_verify_report->verify_errors_counts.u_crc_count;
        *u_errors += error_verify_report->verify_errors_counts.u_crc_multi_count;
        *u_errors += error_verify_report->verify_errors_counts.u_crc_single_count;
        *u_errors += error_verify_report->verify_errors_counts.u_media_count;
        *u_errors += error_verify_report->verify_errors_counts.u_rm_magic_count;
        *u_errors += error_verify_report->verify_errors_counts.u_ss_count;
        *u_errors += error_verify_report->verify_errors_counts.u_ts_count;
        *u_errors += error_verify_report->verify_errors_counts.u_ws_count;
    }

    if (shutdown_errors != NULL) {
        *shutdown_errors = error_verify_report->verify_errors_counts.shutdown_errors;
    }

    if (retryable_errors != NULL) {
        *retryable_errors = error_verify_report->verify_errors_counts.retryable_errors;
    }

    if (non_retryable_errors != NULL) {
        *non_retryable_errors = error_verify_report->verify_errors_counts.non_retryable_errors;
    }

    return FBE_STATUS_OK;
}

fbe_status_t database_wait_system_object_ready(fbe_object_id_t system_object_id,fbe_u16_t time_count, fbe_bool_t *is_ready_p)
{
    fbe_u16_t count = 0;
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_object_mgmt_get_lifecycle_state_t  get_lifecycle;
	while (count < time_count)
	{
        
		status = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
													&get_lifecycle,
													sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
													system_object_id,
													NULL,
													0,
													FBE_PACKET_FLAG_NO_ATTRIB,
													FBE_PACKAGE_ID_SEP_0);
		if (status != FBE_STATUS_OK && status != FBE_STATUS_NO_OBJECT)
		{
			*is_ready_p = FBE_FALSE;
			database_trace (FBE_TRACE_LEVEL_ERROR,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s: unable to get object lifecycle state\n", 
							__FUNCTION__); 
			return FBE_STATUS_GENERIC_FAILURE;
		} 
		if(status == FBE_STATUS_NO_OBJECT){
			*is_ready_p = FBE_FALSE;
			return FBE_STATUS_NO_OBJECT;

		}
        if(status == FBE_STATUS_OK && get_lifecycle.lifecycle_state == FBE_LIFECYCLE_STATE_READY)
        {
              *is_ready_p = FBE_TRUE;
			  break;
	    }else{
               count++;
		}
		
			
	}
	if(count == time_count)
		*is_ready_p = FBE_FALSE;
	return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_raw_mirror_drive_state_initialize
 *****************************************************************
 * @brief
 *   load and initialize the database raw-mirror drive state
 *
 * @param no
 * 
 * @return fbe_status_t  return whether the load success or not
 *
 * @version
 *    05/28/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t fbe_database_raw_mirror_drive_state_initialize(void)
{   
    /*initialize and load the state from disk*/
    fbe_zero_memory(&fbe_database_raw_mirror_drive_state, sizeof (fbe_database_raw_mirror_drive_state_t));

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_raw_mirror_set_new_drive
 *****************************************************************
 * @brief
 *   set the new drive of the database raw-mirror. It is supposed to
 *   be called by Homewrecker
 *
 * @param new_drives     a bitmap to indicate which drive is new
 * 
 * @return fbe_status_t  return whether the set success or not
 *
 * @version
 *    05/28/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t fbe_database_raw_mirror_set_new_drive(fbe_u32_t new_drives)
{
    fbe_database_raw_mirror_drive_state.raw_mirror_drive_nr_bitmap |= new_drives;
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_database_raw_mirror_rebuild_complete
 *****************************************************************
 * @brief
 *   set which drives already complete rebuild and persist to disk
 *
 * @param rb_complete_drives     a bitmap to indicate which drive rebuild done
 * 
 * @return fbe_status_t  return whether the set success or not
 *
 * @version
 *    05/28/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t fbe_database_raw_mirror_rebuild_complete(fbe_u32_t rb_complete_drives)
{
    fbe_database_raw_mirror_drive_state.raw_mirror_drive_nr_bitmap &= ~rb_complete_drives;

    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_database_raw_mirror_get_valid_drives
 *****************************************************************
 * @brief
 *   return a bitmap of drives to indicate which drives are valid to load
 *   configuration from it
 *
 * @param no     
 * 
 * @return fbe_status_t  return whether the set success or not
 *
 * @version
 *    05/28/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t fbe_database_raw_mirror_get_valid_drives(fbe_u32_t *valid_drives)
{
    if (valid_drives != NULL) {
        *valid_drives = (~fbe_database_raw_mirror_drive_state.raw_mirror_drive_nr_bitmap) & DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK;
        return FBE_STATUS_OK;
    } else
        return FBE_STATUS_GENERIC_FAILURE;
}

/*!***************************************************************
 * @fn fbe_database_raw_mirror_init_block_seq_number
 *****************************************************************
 * @brief
 *   Load current system db raw_mirror block seq number from drives, if there
 *   is no seq number, set the seq number as DATABASE_RAW_MIRROR_START_SEQ
 *
 * @param no 
 * 
 * @return fbe_status_t  return whether the set success or not
 *
 * @version
 *    12/30/2012: Jibing Dong - created
 *
 ****************************************************************/
fbe_status_t fbe_database_raw_mirror_init_block_seq_number(fbe_u32_t valid_drives)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lba_t           lba = 0;
    fbe_block_count_t   blocks_count;
    fbe_u8_t            *buffer, *block_p;
    fbe_u32_t           read_blocks;

    /* allocate a buffer which size of 64 blocks */
    buffer = (fbe_u8_t *)fbe_memory_ex_allocate(FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * DATABASE_SYSTEM_DB_BLOCK_SIZE);
    if (buffer == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Can't allocate read buffer\n",
            __FUNCTION__);
        return status;
    }

    /* get the capacity of the raw-mirror */
    database_system_objects_get_system_db_offset_capacity(NULL, NULL, &blocks_count);

    while (blocks_count > 0)
    {
        /* get the number of read blocks */ 
        if (blocks_count > FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64)
        {
            read_blocks = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64;
        }
        else
        {
            read_blocks = (fbe_u32_t)blocks_count;
        }

        blocks_count -= read_blocks;
        
        /* read a chunk size from system db raw mirror */ 
        status = database_utils_raw_mirror_general_read(buffer, 
                                                        lba, 
                                                        read_blocks * DATABASE_SYSTEM_DB_BLOCK_SIZE,
                                                        valid_drives);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: read failed, status = 0x%x\n",
                    __FUNCTION__,
                    status);
            database_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: try to read block one by one\n",
                    __FUNCTION__);
           
        }

        block_p = buffer;
        while (read_blocks-- > 0)
        {
            fbe_status_t retry_status = FBE_STATUS_OK;
                
            /* if read fail, retry to read current block */ 
            if (status != FBE_STATUS_OK)
            {
                retry_status = database_utils_raw_mirror_general_read(block_p,
                                                                      lba,
                                                                      DATABASE_SYSTEM_DB_BLOCK_SIZE, /* read 1 block */
                                                                      valid_drives);
            }

            if (retry_status == FBE_STATUS_OK)
            {
                /* initialize the seq num from disk */
                fbe_database_raw_mirror_io_cb.seq_numbers[lba] = fbe_xor_lib_raw_mirror_sector_get_seq_num((fbe_sector_t*)block_p);
            }
            else
            {
                /* if we can't read out data, set the seq number to default value */
                fbe_database_raw_mirror_io_cb.seq_numbers[lba] = DATABASE_RAW_MIRROR_START_SEQ;
            }

            /* move to next */
            lba++;
            block_p += DATABASE_SYSTEM_DB_BLOCK_SIZE;
        }
    }

    fbe_memory_ex_release(buffer);

    return FBE_STATUS_OK;
}
