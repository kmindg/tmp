/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_homewrecker_db_interface.c
***************************************************************************
*
* @brief
*  This file contains homewrecker accessing raw_mirror code.
*  
* @version
*  04/16/2012 - Created. 
*
***************************************************************************/
#include "fbe_raid_library.h"
#include "fbe_database_private.h"
#include "fbe_database_system_db_interface.h"
#include "fbe_database_homewrecker_db_interface.h"
#include "fbe/fbe_notification_lib.h"


#define ARRAY_SIZE(arr)	(sizeof(arr)/sizeof(arr[0]))

#define C4_MIRROR_RG_NUMBER  (4)
#define C4_MIRROR_SYS_DRIVES (4)
#define C4_MIRROR_SLOT_INVALID   (0xffffffff)
#define C4_MIRROR_FRU_NUMBER (2)
/* 'NTMR' */
#define C4_MIRROR_MAGIC_NUMBER   (0x4e544d52)
#define C4_MIRROR_RG_START   (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_UTILITY_BOOT_VOLUME_SPA)
#define C4_MIRROR_RG_END (C4_MIRROR_RG_START + C4_MIRROR_RG_NUMBER)

#pragma pack(1)
typedef struct c4_mirror_pdoinfo_s {
    fbe_u8_t sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1];
    fbe_u32_t slot;
} c4_mirror_pdoinfo_t;

typedef struct c4_mirror_rginfo_s {
    fbe_u64_t          seq_number;
    c4_mirror_pdoinfo_t pdo_info[C4_MIRROR_FRU_NUMBER];
} c4_mirror_rginfo_t;

/* Data struct on disk for pdo info */
typedef struct c4_mirror_rginfo_disk_s {
    fbe_u32_t               magic;
    c4_mirror_rginfo_t      rginfo;
} c4_mirror_rginfo_disk_t;
#pragma pack()

typedef struct c4_mirror_rginfo_cache_s {
    c4_mirror_rginfo_t   rginfo;
    fbe_object_id_t     pvd_id[C4_MIRROR_FRU_NUMBER];
    fbe_bool_t          is_loaded;
} c4_mirror_rginfo_cache_t;

typedef struct c4_mirror_cache_s {
    fbe_spinlock_t          lock;
    c4_mirror_rginfo_cache_t rg[C4_MIRROR_RG_NUMBER];
    c4_mirror_pdoinfo_t      current_pdo[C4_MIRROR_SYS_DRIVES];
} c4_mirror_cache_t;



fbe_raw_mirror_t fbe_database_homewrecker_db;
fbe_u64_t homewrecker_db_block_seq_number;

static c4_mirror_cache_t c4_mirror_cache;

static fbe_status_t c4_mirror_cache_init(c4_mirror_cache_t *cache);
static fbe_status_t c4_mirror_cache_destroy(c4_mirror_cache_t *mirror_cache);
/*!***************************************************************
 * @fn database_homewrecker_get_system_pvd_edge_state
 *****************************************************************
 * @brief
 *   
 *
 * @param 
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    4/20/2012: 
 *
 ****************************************************************/
fbe_status_t database_homewrecker_get_system_pvd_edge_state(fbe_u32_t pvd_index,
                                                                                 fbe_bool_t* is_path_enable)
{
    fbe_path_state_t path_state;
    fbe_object_id_t pvd_object_id;
    fbe_block_transport_control_get_edge_info_t block_edge_info_p;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE; 

    block_edge_info_p.base_edge_info.client_index = 0;
    
    status = fbe_database_get_system_pvd(pvd_index, &pvd_object_id);
    if (status != FBE_STATUS_OK) 
    {
        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Get pvd id 0x%x error\n",
                        __FUNCTION__, pvd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* send the control packet to get the block edge information. */
    status = fbe_database_send_packet_to_object(FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO,
                                                 &block_edge_info_p,
                                                 sizeof(block_edge_info_p),
                                                 pvd_object_id,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Get pvd 0x%x edge info error\n",
                        __FUNCTION__, pvd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    path_state = block_edge_info_p.base_edge_info.path_state;
    
    if (path_state == FBE_PATH_STATE_ENABLED)
        *is_path_enable = FBE_TRUE;
    else
        *is_path_enable = FBE_FALSE;

    return FBE_STATUS_OK;
}




/*!***************************************************************
 * @fn database_homewrecker_db_interface_init
 *****************************************************************
 * @brief
 *   Control function to initialize the homewrecker db and raw-mirror IO
 *   interface.
 *
 * @param no parameter
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    4/20/2012: Gao Jian - copied from system db init
 *
 ****************************************************************/
fbe_status_t database_homewrecker_db_interface_init(fbe_database_service_t* db_service)
{
    fbe_lba_t offset;
    fbe_block_count_t capacity;
    fbe_status_t status;

    /* need code review to confirm removing it */
    FBE_ASSERT_AT_COMPILE_TIME(DATABASE_HOMEWRECKER_DB_BLOCK_DATA_SIZE <= FBE_RAW_MIRROR_DATA_SIZE);

    status = c4_mirror_cache_init(&c4_mirror_cache);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to init c4mirror cache\n", __FUNCTION__);
        return status;
    }

    database_system_objects_get_homewrecker_db_offset_capacity(&offset, &capacity);
    status = fbe_raw_mirror_init(&fbe_database_homewrecker_db, offset, 0, capacity);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to init homewrecker raw_mirror\n", __FUNCTION__);
        return status;
    }

    fbe_spinlock_lock(&db_service->pvd_operation.system_fru_descriptor_lock);
    fbe_zero_memory(&db_service->pvd_operation.system_fru_descriptor, sizeof(fbe_homewrecker_fru_descriptor_t));
    fbe_spinlock_unlock(&db_service->pvd_operation.system_fru_descriptor_lock);

    return status;
}

/*!***************************************************************
 * @fn database_homewrecker_db_interface_init_raw_mirror_block_seq
 *****************************************************************
 * @brief
 *   Load current homewrecker db raw_mirror block seq number from drives, if there
 *   is no seq number, set the seq number as HOMEWRECKER_DB_RAW_MIRROR_START_SEQ
 *
 * @param no parameter
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    4/20/2012: Gao Jian - created
 *
 ****************************************************************/
fbe_status_t database_homewrecker_db_interface_init_raw_mirror_block_seq()
{
    fbe_status_t status;
    fbe_u8_t buffer[512];
    fbe_u64_t done_count;
    fbe_u64_t seq_num;
 
    database_homewrecker_db_raw_mirror_operation_status_t operation_status;
        
    status = database_homewrecker_db_interface_general_read(buffer, 
                                                                                                                               0, 
                                                                                                                               512,
                                                                                                                               &done_count, 
                                                                                                                               7,
                                                                                                                               &operation_status); 
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to init homewrecker db raw_mirror block seq\n", __FUNCTION__);
        return status;
    }
    else { 
        if(operation_status.block_operation_successful)
        {
            fbe_homewrecker_db_util_get_raw_mirror_block_seq(buffer, &seq_num);
            homewrecker_db_block_seq_number = seq_num;
        }
        else
        {
            homewrecker_db_block_seq_number = HOMEWRECKER_DB_RAW_MIRROR_START_SEQ;
        }
    }

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
 *    4/20/2012: Jian Gao - copied
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
 * @fn database_homewrecker_db_interface_general_read
 *****************************************************************
 * @brief
 *   general interface to read data from a contingous region in database raw-mirror,
 *   supporting one kind of block size: DATABASE_HOMEWRECKER_DB_BLOCK_SIZE.
 *
 * @param data_ptr   - buffer to store the data read
 * @param start_lba  - the start lba of the raw-mirror to be read
 * @param count      - the bytes to be read from raw-mirror
 * @param done_count - return how many bytes actually read
 * @param drive_mask - indicate to access which drive or drives
 *                                           -001: 1st drive only, 011: 1st and 2nd drive, 111: all of three drives
 * @param raw_mirror_error_report - return raw_mirror status with it
 * 
 * @return status - return FBE_STATUS_GENERIC_FAILURE if issue encountered
 *
 * @version
 *    4/20/2012: Jian Gao - created
 *
 ****************************************************************/


fbe_status_t database_homewrecker_db_interface_general_read
                                            (fbe_u8_t *data_ptr, fbe_lba_t start_lba, 
                                             fbe_u64_t count, fbe_u64_t *done_count, 
                                             fbe_u16_t drive_mask,
                                             database_homewrecker_db_raw_mirror_operation_status_t * operation_status)
{
    fbe_status_t                                                          status;
    fbe_semaphore_t                                               mem_alloc_semaphore;
    fbe_semaphore_t                                               sync_read_semaphore;
    fbe_memory_request_t                                    memory_request;
    
    fbe_block_count_t                                             block_count;
    fbe_memory_chunk_size_t                             chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    fbe_memory_number_of_objects_t            chunk_number = 1;
    fbe_packet_t *                                                     packet = NULL;

    fbe_payload_block_operation_t *               block_operation = NULL;
    fbe_sg_element_t *                                           sg_list_p = NULL;
    fbe_memory_header_t *                                 master_memory_header = NULL;
    fbe_memory_header_t *                                 current_memory_header = NULL;
    fbe_memory_chunk_size_t                             memory_chunk_size;
   
    fbe_u32_t                                                              buffer_count = 0;
    fbe_s64_t                                                               left_count = count;
    fbe_u32_t                                                              copied_count = 0;
    fbe_u8_t *                                                             buffer = NULL;
    fbe_u32_t                                                              sg_list_size = 0;
    fbe_u32_t                                                              sg_list_count = 0;
    fbe_u32_t                                                              i = 0;
    fbe_payload_ex_t *                                           payload = NULL;

    fbe_cpu_id_t                                                        cpu_id;
    fbe_raid_verify_raw_mirror_errors_t         internal_raw_mirror_error_report;
                                          
        
    /*if buffer pointer is NULL, fail*/
    if (data_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid arguments\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* round up */
    block_count = (count + DATABASE_HOMEWRECKER_DB_RAW_BLOCK_DATA_SIZE -1) / 
                                    DATABASE_HOMEWRECKER_DB_RAW_BLOCK_DATA_SIZE; 

    if (block_count < FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET) {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    } else {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    }
    chunk_number = (fbe_memory_number_of_objects_t)((block_count + block_count_chunk_size(chunk_size) - 1) / block_count_chunk_size(chunk_size));
    chunk_number ++;  /* +1 for packet and sg_list */

    fbe_semaphore_init(&mem_alloc_semaphore, 0, 1);
    fbe_semaphore_init(&sync_read_semaphore, 0, 1);
    fbe_zero_memory(&memory_request, sizeof(fbe_memory_request_t));
    fbe_memory_request_init(&memory_request);
    /* Allocate memroy for data */
    fbe_memory_build_chunk_request(&memory_request,
                                   chunk_size,
                                   chunk_number,
                                   DATABASE_HOMEWRECKER_DB_MEM_REQ_PRIORITY,
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                   (fbe_memory_completion_function_t)database_homewrecker_db_interface_mem_allocation_completion, /* SAFEBUG */
                                   &mem_alloc_semaphore);

    status = fbe_memory_request_entry(&memory_request);
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, &memory_request, memory_request.request_state);
        fbe_semaphore_destroy(&mem_alloc_semaphore);
        fbe_semaphore_destroy(&sync_read_semaphore);

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
        fbe_semaphore_destroy(&sync_read_semaphore);
        
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
        fbe_semaphore_destroy(&sync_read_semaphore);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* carve the packet */
    buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);
    packet = (fbe_packet_t *)buffer;
    fbe_zero_memory(packet, sizeof(fbe_packet_t));
    buffer += sizeof(fbe_packet_t);

    /* carve the sg list */
    sg_list_count = master_memory_header->number_of_chunks - 1; /* exclude the first chunk(for packet and sg_list itself)*/
    sg_list_p = (fbe_sg_element_t *)buffer;
    fbe_zero_memory(sg_list_p, sg_list_size);

    /* Form SG lists from remaing memory chunk*/
    for (i = 0; i < sg_list_count; i++) {
        /* move to the next chunk */
        fbe_memory_get_next_memory_header(master_memory_header, &current_memory_header);
        master_memory_header = current_memory_header;

        sg_list_p[i].address = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header);
        sg_list_p[i].count = DATABASE_HOMEWRECKER_DB_BLOCK_SIZE * block_count_chunk_size(memory_chunk_size);
        
        fbe_zero_memory(sg_list_p[i].address, sg_list_p[i].count);
    }
    sg_list_p[sg_list_count].address = NULL;
    sg_list_p[sg_list_count].count = 0;

    fbe_transport_initialize_sep_packet(packet);    
    payload = fbe_transport_get_payload_ex(packet);
	fbe_payload_ex_allocate_memory_operation(payload);
    block_operation =  fbe_payload_ex_allocate_block_operation(payload);

    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);
    fbe_payload_ex_set_sg_list(payload, sg_list_p, sg_list_count);

    /*set raw-mirror error verify report to record the IO errors on 
      raw-mirror, which is passed by caller, if there it is */
    if(&(operation_status->error_verify_report) != NULL) {
        fbe_zero_memory(&(operation_status->error_verify_report), sizeof (fbe_raid_verify_raw_mirror_errors_t));
        fbe_payload_ex_set_verify_error_count_ptr(payload, &(operation_status->error_verify_report));
    }
    /* else set an internal error report into packet */
    else {
        fbe_zero_memory(&internal_raw_mirror_error_report, sizeof (fbe_raid_verify_raw_mirror_errors_t));
        fbe_payload_ex_set_verify_error_count_ptr(payload, &internal_raw_mirror_error_report);
    }

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      start_lba,    /* LBA */
                                      block_count,
                                      DATABASE_HOMEWRECKER_DB_BLOCK_SIZE,    /* block size: 520  */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Set completion function */
    /* sync_read_semaphore is defined and initialized at this function beginning */
    fbe_transport_set_completion_function(packet, database_homewrecker_db_raw_mirror_blocks_read_completion, &sync_read_semaphore);
    fbe_payload_ex_increment_block_operation_level(payload);

    /* The drive mask is passed by caller, grant the ability of choose target drive to caller */
    status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(&fbe_database_homewrecker_db, packet, drive_mask);
    
    if (status != FBE_STATUS_OK) {
        fbe_semaphore_destroy(&sync_read_semaphore);
    } else {
        fbe_semaphore_wait(&sync_read_semaphore, NULL);
        fbe_semaphore_destroy(&sync_read_semaphore);
    }


    /*read complete and copy data */
    if (status == FBE_STATUS_OK && 
         block_operation->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {

        operation_status->block_operation_successful = FBE_TRUE;

        operation_status->error_disk_bitmap = operation_status->error_verify_report.raw_mirror_magic_bitmap | 
                                                                                        operation_status->error_verify_report.raw_mirror_seq_bitmap ;

        /* Copy the data */
        for (i = 0; i < sg_list_count; i++) {
            buffer = sg_list_p[i].address;
            buffer_count = sg_list_p[i].count;
            while ((buffer_count >= DATABASE_HOMEWRECKER_DB_BLOCK_SIZE) && (left_count > 0)) {
                copied_count = DATABASE_HOMEWRECKER_DB_BLOCK_SIZE;
                if (left_count <= DATABASE_HOMEWRECKER_DB_BLOCK_SIZE) {
                    copied_count = (fbe_u32_t)left_count;
                }

                fbe_copy_memory(data_ptr, buffer, copied_count);
                data_ptr += copied_count;
                left_count -= copied_count;

                /* move forward the sg_list's pointer (520) */
                buffer += DATABASE_HOMEWRECKER_DB_BLOCK_SIZE;                    
                buffer_count -= DATABASE_HOMEWRECKER_DB_BLOCK_SIZE;
            }
            if (left_count <= 0) {
                /* we have get the data */
                break;
            }
        }

    }
    else if(block_operation->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        operation_status->block_operation_successful = FBE_FALSE;
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: read raw_mirror blocks failed \n", __FUNCTION__);
            
    }

    //fbe_memory_request_get_entry_mark_free(&memory_request, &memory_ptr);
    /* If allocation failed, the memory_ptr may be NULL pointer */    
    //fbe_memory_free_entry(memory_ptr);
    fbe_memory_free_request_entry(&memory_request);

    /* destroy semaphores */
    fbe_semaphore_destroy(&mem_alloc_semaphore);

    if (done_count != NULL)
        *done_count = count - left_count;

    return status;
}

/*!***************************************************************
 * @fn database_homewrecker_db_raw_mirror_blocks_read_completion
 *****************************************************************
 * @brief
 *   complete function for database_homewrecker_db_interface_general_read,
 *   it will check the error report structure and block operation status to determine 
 *   how to fix the errors reported by raw-mirror raid library. 
 *
 * @param packet   - the IO packet
 * @param context  - context of the complete function run
 * 
 * @return status - indicating if the complete function success
 *
 * @version
 *    4/20/2012: Jian Gao - created
 *
 ****************************************************************/
static fbe_status_t 
database_homewrecker_db_raw_mirror_blocks_read_completion
                                                            (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    
    fbe_payload_ex_t *                                                                      payload = NULL;
    fbe_payload_block_operation_t *                                          block_operation = NULL;
    fbe_payload_block_operation_status_t                               block_operation_status;
    fbe_payload_block_operation_qualifier_t                          block_operation_qualifier;

    fbe_semaphore_t*                                                                        sync_read_semaphore = NULL;
    fbe_raid_verify_raw_mirror_errors_t *                                 error_verify_report = NULL;

    /*retrieve the context info*/
    sync_read_semaphore= (fbe_semaphore_t *)context;
    if (!sync_read_semaphore) {
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

    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: read failed: status =%d, block_operation_status = %d\n",
                       __FUNCTION__, status, block_operation_status);
        fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_destroy_packet(packet); /* This will not release the memory */
        fbe_semaphore_release(sync_read_semaphore, 0, 1, FALSE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
 
        /*1. magic and sequence errors*/
        if (error_verify_report != NULL) {
            if (error_verify_report->raw_mirror_seq_bitmap != 0 ||
                error_verify_report->raw_mirror_magic_bitmap != 0) {
                database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "database_raw_mirr_read error report:, seq_bitmap %x, magic_bitmap %x\n",
                       error_verify_report->raw_mirror_seq_bitmap,
                       error_verify_report->raw_mirror_magic_bitmap);
            }
        } 

        if (block_operation_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH) {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "database_raw_mirr_read sequence not match");
        }    
    } else {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "DB raw-mirr read operation failed, status: %d, qualifier %d \n", 
                       block_operation_status, block_operation_qualifier);
    }

    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_destroy_packet(packet);
    fbe_semaphore_release(sync_read_semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

                                                    
/*!***************************************************************
 * @fn database_homewrecker_db_interface_general_write
 *****************************************************************
 * @brief
 *   general interface to write data to a contingous region in homewrecke raw-mirror,
 *   the supporting  block size is DATABASE_SYSTEM_DB_BLOCK_SIZE. It also support clear
 *   a contingous region of homewrecker raw-mirror, which resets all the data including magic
 *   number and sequence number of each block.
 *
 * @param data_ptr   - buffer to store the data wrote
 * @param start_lba  - the start lba of the homewrecker raw-mirror to be wrote
 * @param count      - the bytes to be wrote to homewrecker raw-mirror
 * @param done_count - return how many bytes actually wrote
 * @param is_clear_write - indicating if it is clear operation
 * @param raw_mirror_error_report - raw_mirror and raid library would fill this structure to 
 * indicate what's error occur during IO, is a return structure
 * 
 * @return status - return FBE_STATUS_GENERIC_FAILURE if issue encountered
 *
 * @version
 *    4/16/2012: Jian Gao - created
 *
 ****************************************************************/
fbe_status_t database_homewrecker_db_interface_general_write
                                                        (fbe_u8_t *data_ptr, fbe_lba_t start_lba, fbe_u64_t count, 
                                                        fbe_u64_t *done_count, fbe_bool_t is_clear_write, 
                                                        database_homewrecker_db_raw_mirror_operation_status_t * operation_status)
{
    fbe_status_t                              status;

    fbe_semaphore_t                           mem_alloc_semaphore;
    fbe_semaphore_t                           sync_write_semaphore;
    fbe_memory_request_t                      memory_request;
    
    fbe_block_count_t                         block_count;
    fbe_memory_chunk_size_t                   chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    fbe_memory_number_of_objects_t            chunk_number = 1;
    fbe_packet_t                              *packet = NULL;
    fbe_sg_element_t                          *sg_list_p = NULL;
    fbe_u32_t                                 blocks_number = 0, blocks_number_internal = 0;
    fbe_memory_header_t                       *master_memory_header = NULL;
    fbe_memory_header_t                       *current_memory_header = NULL;
    fbe_memory_chunk_size_t                   memory_chunk_size;

    fbe_s64_t                                 left_count = 0;
    fbe_u32_t                                 copied_count = 0;
    fbe_u8_t                                  *buffer = NULL;
    fbe_u32_t                                 buffer_count = 0;
    fbe_u32_t                                 sg_list_size = 0;
    fbe_u32_t                                 sg_list_count = 0;

    fbe_payload_ex_t                          *payload = NULL;
    fbe_payload_block_operation_t             *block_operation = NULL;
    fbe_cpu_id_t                              cpu_id; 
    fbe_raid_verify_raw_mirror_errors_t       internal_raw_mirror_error_report;
    
    fbe_u32_t                                 i = 0;
   
    if (data_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid arguments, write buffer is NULL\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Caculate to be wrote block count */
    block_count = (count + DATABASE_HOMEWRECKER_DB_BLOCK_DATA_SIZE -1) / DATABASE_HOMEWRECKER_DB_BLOCK_DATA_SIZE;

    if (block_count < FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET) {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    } else {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    }
    chunk_number = (fbe_memory_number_of_objects_t)((block_count + block_count_chunk_size(chunk_size) - 1) / block_count_chunk_size(chunk_size));
    chunk_number ++; /* +1 for packet and sg_list */

    fbe_semaphore_init(&mem_alloc_semaphore, 0, 1);
    fbe_semaphore_init(&sync_write_semaphore, 0, 1);
    fbe_zero_memory(&memory_request, sizeof(fbe_memory_request_t));
    fbe_memory_request_init(&memory_request);

    /* Allocate memroy for data */
    fbe_memory_build_chunk_request(&memory_request,
                                   chunk_size,
                                   chunk_number,
                                   DATABASE_HOMEWRECKER_DB_MEM_REQ_PRIORITY,
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                   (fbe_memory_completion_function_t)database_homewrecker_db_interface_mem_allocation_completion, /* SAFEBUG */
                                   &mem_alloc_semaphore);

    status = fbe_memory_request_entry(&memory_request);
    /* If fail to allocate the memory */
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {
        fbe_semaphore_destroy(&mem_alloc_semaphore);
        fbe_semaphore_destroy(&sync_write_semaphore);

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
        fbe_semaphore_destroy(&sync_write_semaphore);

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
        fbe_semaphore_destroy(&sync_write_semaphore);

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

        buffer = sg_list_p[i].address = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header);
        buffer_count = DATABASE_HOMEWRECKER_DB_BLOCK_SIZE * block_count_chunk_size(memory_chunk_size);
        sg_list_p[i].count = 0;

        fbe_zero_memory(buffer, buffer_count);
        blocks_number_internal = 0;
        while ((buffer_count >= DATABASE_HOMEWRECKER_DB_BLOCK_SIZE) && (left_count > 0)) {
            copied_count = DATABASE_HOMEWRECKER_DB_BLOCK_DATA_SIZE;
            if (left_count <= DATABASE_HOMEWRECKER_DB_BLOCK_DATA_SIZE) {
                copied_count = (fbe_u32_t)left_count;
            }
            
            fbe_copy_memory(buffer, data_ptr, copied_count);
            
            data_ptr += copied_count;
            left_count -= copied_count;

            /* move forward the sg_list's pointer (520) */
            buffer += DATABASE_HOMEWRECKER_DB_BLOCK_SIZE;
            buffer_count -= DATABASE_HOMEWRECKER_DB_BLOCK_SIZE;
            sg_list_p[i].count += DATABASE_HOMEWRECKER_DB_BLOCK_SIZE;
            database_homewrecker_db_interface_db_calculate_checksum(
                sg_list_p[i].address + blocks_number_internal * DATABASE_HOMEWRECKER_DB_BLOCK_SIZE, is_clear_write);
            blocks_number_internal++;
        }

        blocks_number += blocks_number_internal;

        if (left_count <= 0) {
            /* we have get the data */
            i++;
            break;
        }
    }
    sg_list_p[i].address = NULL;
    sg_list_p[i].count = 0;

    fbe_transport_initialize_sep_packet(packet);    
    payload = fbe_transport_get_payload_ex(packet);
	fbe_payload_ex_allocate_memory_operation(payload);
    block_operation =  fbe_payload_ex_allocate_block_operation(payload);

    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);
    fbe_payload_ex_set_sg_list(payload, sg_list_p, sg_list_count);

    /* set raw-mirror error verify report to record the IO errors on raw_mirror
      * If the error_report passed from caller is NULL, would use the internal 
      * error_report structure. Else use the structure passed from caller
      */
    if(NULL == &(operation_status->error_verify_report))
    {
        fbe_zero_memory(&internal_raw_mirror_error_report, sizeof (fbe_raid_verify_raw_mirror_errors_t));
        fbe_payload_ex_set_verify_error_count_ptr(payload, &internal_raw_mirror_error_report);
    }
    else
    {
        fbe_zero_memory(&(operation_status->error_verify_report), sizeof (fbe_raid_verify_raw_mirror_errors_t));
        fbe_payload_ex_set_verify_error_count_ptr(payload, &(operation_status->error_verify_report));
    }
        
    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      start_lba,    /* LBA */
                                      block_count,
                                      DATABASE_HOMEWRECKER_DB_BLOCK_SIZE,    /* block size: 520  */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    fbe_transport_set_completion_function(packet, database_homewrecker_db_raw_mirror_blocks_write_completion, &sync_write_semaphore);
    fbe_payload_ex_increment_block_operation_level(payload);

    status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(&fbe_database_homewrecker_db, packet,FBE_SEND_IO_TO_RAW_MIRROR_BITMASK);

    if (status != FBE_STATUS_OK) {
        fbe_semaphore_destroy(&sync_write_semaphore);
    } else {
        fbe_semaphore_wait(&sync_write_semaphore, NULL);
        fbe_semaphore_destroy(&sync_write_semaphore);
    }

   if (status == FBE_STATUS_OK && 
         block_operation->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
         operation_status->block_operation_successful = FBE_TRUE;
    }
   else if (block_operation->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        operation_status->block_operation_successful = FBE_FALSE;
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
 * @fn database_homewrecker_db_interface_mem_allocation_completion
 *****************************************************************
 * @brief
 *   complete rountine of the memory request for homewrecker raw-mirror read/write
 *
 * @param memory_request  - request to allocate memory
 * @param context  - context of the complete function
 * 
 * @return status - return FBE_STATUS_GENERIC_FAILURE if issue encountered
 *
 * @version
 *    04/16/2012: Jian Gao - copied
 *
 ****************************************************************/
static fbe_status_t
database_homewrecker_db_interface_mem_allocation_completion
                                                                                                                                            (fbe_memory_request_t * memory_request, 
                                                                                                                                             fbe_memory_completion_context_t context)
{
    fbe_semaphore_t *mem_alloc_semaphore = (fbe_semaphore_t *)context;

    if (mem_alloc_semaphore != NULL) {
        fbe_semaphore_release(mem_alloc_semaphore, 0, 1, FALSE); 
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_homewrecker_db_interface_db_calculate_checksum
 *****************************************************************
 * @brief
 *   interface to calculate the checksum, set magic number and sequence
 *   number for homewrecker raw-mirror blocks. 
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
 *    4/17/2012: Jian Gao - modified
 *
 ****************************************************************/
static fbe_status_t
database_homewrecker_db_interface_db_calculate_checksum
                                                                    (void * block_p, fbe_bool_t seq_reset)
{
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
   if (!seq_reset) {
        fbe_homewrecker_db_util_raw_mirror_io_get_seq(FBE_FALSE, &seq);
   } else
        fbe_homewrecker_db_util_raw_mirror_io_get_seq(FBE_TRUE, &seq);

    fbe_xor_lib_raw_mirror_sector_set_seq_num(block_p, seq);
    /* skipping by 512 so skips data and raw mirror metadata; revisit in general for best overall solution.
      */
    reference_ptr = ( fbe_u32_t * ) ((char *)block_p + DATABASE_HOMEWRECKER_DB_RAW_BLOCK_DATA_SIZE);
    *reference_ptr = fbe_xor_lib_calculate_checksum((fbe_u32_t*)block_p);
    reference_ptr++;

    *reference_ptr = 0;

    block_p = (char *) block_p + DATABASE_HOMEWRECKER_DB_BLOCK_SIZE;

    return FBE_STATUS_OK;
}



fbe_status_t fbe_homewrecker_db_util_raw_mirror_io_get_seq(fbe_bool_t reset, fbe_u64_t *seq)
{
    if (reset)
        homewrecker_db_block_seq_number = HOMEWRECKER_DB_RAW_MIRROR_START_SEQ;
    else
        if (homewrecker_db_block_seq_number >= HOMEWRECKER_DB_RAW_MIRROR_END_SEQ)
            homewrecker_db_block_seq_number = HOMEWRECKER_DB_RAW_MIRROR_START_SEQ;
        else
            homewrecker_db_block_seq_number++;
 
    *seq = homewrecker_db_block_seq_number;

    return FBE_STATUS_OK;
}

fbe_status_t fbe_homewrecker_db_util_get_raw_mirror_block_seq(fbe_u8_t * block, fbe_u64_t * seq)
{
    *seq = fbe_xor_lib_raw_mirror_sector_get_seq_num((fbe_sector_t *)block);
    return FBE_STATUS_OK;
}

static fbe_status_t 
database_homewrecker_db_raw_mirror_blocks_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;

    fbe_raid_verify_raw_mirror_errors_t *error_verify_report = NULL;

    fbe_semaphore_t *sync_write_semaphore;

    /*retrieve the context info*/
    sync_write_semaphore = (fbe_semaphore_t *)context;
    if (NULL == sync_write_semaphore) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed, no context\n",
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
                       "%s: write failed: status =%d, block_operation_status = %d\n",
                       __FUNCTION__, status, block_operation_status);
        fbe_payload_ex_release_block_operation(payload, block_operation);
        fbe_transport_destroy_packet(packet); /* This will not release the memory */
        fbe_semaphore_release(sync_write_semaphore, 0, 1, FALSE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*2. check error report, which applied to both block operation success or failure 
      cases*/

    if (error_verify_report != NULL) {
        ;
    }

    if (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        if (block_operation_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH) {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "database_raw_mirr_write detected sequence not match");
        }
    } else {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "DB raw-mirr write operation failed, status: %d, qualifier %d \n", 
                       block_operation_status, block_operation_qualifier);
    }

    fbe_payload_ex_release_block_operation(payload, block_operation);
    fbe_transport_destroy_packet(packet);
    fbe_semaphore_release(sync_write_semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_homewrecker_db_destroy
 *****************************************************************
 * @brief
 *   interface to release the resource allocated for database homewrecker
 *   db 
 *
 * @param no
 * 
 * @return no
 *
 * @version
 *    4/25/2012: Jian Gao - created
 *
 ****************************************************************/
void fbe_database_homewrecker_db_destroy(void)
{
    fbe_raw_mirror_destroy(&fbe_database_homewrecker_db);
    c4_mirror_cache_destroy(&c4_mirror_cache);
}

/*!***************************************************************
 * @fn database_homewrecker_db_interface_clear
 *****************************************************************
 * @brief
 *   clear the raw-mirror homewrecker db region and set all data to zero.
 *
 * @param no 
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    8/15/2011: Gao Hongpo - created
 *
 ****************************************************************/
fbe_status_t database_homewrecker_db_interface_clear(void)
{
    fbe_u8_t *          zero_buffer;
    fbe_u64_t           chunk_nums, write_count = 0;
    fbe_u32_t           i, remain_blocks;
    fbe_lba_t           offset;
    fbe_block_count_t   capacity;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    database_homewrecker_db_raw_mirror_operation_status_t operation_status;

    /* Use chunk size which has 64 blocks */
    zero_buffer = (fbe_u8_t *)fbe_memory_ex_allocate(FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 
                                                                                                        * DATABASE_HOMEWRECKER_DB_BLOCK_DATA_SIZE);
    if (zero_buffer == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: Can't allocate buffer\n",
                      __FUNCTION__);
        return status;
    }

    fbe_zero_memory(zero_buffer, FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE);
    /* get the capacity of the raw-mirror */
    database_system_objects_get_homewrecker_db_offset_capacity(&offset, &capacity);
    if (capacity > HOMEWRECKER_DB_C4_MIRROR_OFFSET) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Shrink clear range from 0x%llx to 0x%x for c4mirror\n",
                       __FUNCTION__,
                       (unsigned long long)capacity, HOMEWRECKER_DB_C4_MIRROR_OFFSET);
        capacity = HOMEWRECKER_DB_C4_MIRROR_OFFSET;
    }
    chunk_nums = capacity / FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64; 
    for (i = 0; i < chunk_nums; i++) {
        status = database_homewrecker_db_interface_general_write(zero_buffer,
                                                    i * FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64,
                                                    FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * DATABASE_HOMEWRECKER_DB_BLOCK_DATA_SIZE,
                                                    &write_count, FBE_TRUE, &operation_status);
        if (status != FBE_STATUS_OK || (write_count != FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE )) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: zero failed, status = 0x%x, write_count = 0x%x\n",
                    __FUNCTION__,
                    status,
                    (unsigned int)write_count);
            fbe_memory_ex_release(zero_buffer);
            if (status == FBE_STATUS_OK) {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            return status;
        }
    }
    remain_blocks = capacity % FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64;
    if (remain_blocks > 0) {
        status = database_homewrecker_db_interface_general_write(zero_buffer,
                                                    chunk_nums * FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64,
                                                    remain_blocks * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE,
                                                    &write_count, FBE_TRUE, &operation_status);
        if (status != FBE_STATUS_OK || (write_count != remain_blocks * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE )) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: zero failed, status = 0x%x, write_count = 0x%x\n",
                           __FUNCTION__,
                           status,
                           (unsigned int)write_count);
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


/**************************************************************************
 * c4mirror cache interface
 **************************************************************************/

static fbe_status_t c4_mirror_pdoinfo_init(c4_mirror_pdoinfo_t *pdoinfo)
{
    pdoinfo->slot = C4_MIRROR_SLOT_INVALID;
    return FBE_STATUS_OK;
}

static fbe_status_t c4_mirror_rginfo_init(c4_mirror_rginfo_t *rginfo)
{
    fbe_u32_t i;

    for (i = 0; i < ARRAY_SIZE(rginfo->pdo_info); i +=1 ) {
        c4_mirror_pdoinfo_init(&rginfo->pdo_info[i]);
    }
    rginfo->seq_number = HOMEWRECKER_DB_RAW_MIRROR_START_SEQ;
    return FBE_STATUS_OK;
}

static fbe_status_t c4_mirror_get_rg_pvds(fbe_object_id_t rg_obj, fbe_object_id_t pvds[C4_MIRROR_FRU_NUMBER])
{
    fbe_status_t status;
    fbe_private_space_layout_region_t region;
    fbe_u32_t width;
    fbe_u32_t fru_index;

    status = fbe_database_layout_get_export_lun_region_by_raid_object_id(rg_obj, &region);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get region for rg 0x%x\n",
                       __FUNCTION__, rg_obj);
        return status;
    }
    width = region.number_of_frus;
    if (width != C4_MIRROR_FRU_NUMBER) {
        /* The Geometry should not be change. */
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Unsupported width %u\n", __FUNCTION__, width);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    for (fru_index = 0; fru_index < width; fru_index += 1) {
        pvds[fru_index] = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + region.fru_id[fru_index];
    }
    return FBE_STATUS_OK;
}


static fbe_status_t c4_mirror_cache_init(c4_mirror_cache_t *cache)
{
    fbe_u32_t i;

    fbe_zero_memory(cache, sizeof(*cache));
    fbe_spinlock_init(&cache->lock);
    for (i = 0; i < C4_MIRROR_RG_NUMBER; i += 1) {
        fbe_u32_t rg_obj = C4_MIRROR_RG_START + i;
        fbe_status_t status;

        cache->rg[i].is_loaded = FBE_FALSE;
        c4_mirror_rginfo_init(&cache->rg[i].rginfo);
        status = c4_mirror_get_rg_pvds(rg_obj, &cache->rg[i].pvd_id[0]);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: RG 0x%x: [ 0x%x, 0x%x ]\n",
                       __FUNCTION__,
                       rg_obj,
                       cache->rg[i].pvd_id[0], cache->rg[i].pvd_id[1]);
    }
    for (i = 0; i < C4_MIRROR_SYS_DRIVES; i += 1) {
        c4_mirror_pdoinfo_init(&cache->current_pdo[i]);
    }

    return FBE_STATUS_OK;
}

static fbe_status_t c4_mirror_cache_destroy(c4_mirror_cache_t *mirror_cache)
{
    fbe_spinlock_destroy(&mirror_cache->lock);
    return FBE_STATUS_OK;
}

static fbe_status_t c4_mirror_cache_get_offset(fbe_object_id_t object_id, fbe_lba_t *lba)
{
    *lba = FBE_LBA_INVALID;
    if (object_id < C4_MIRROR_RG_START || object_id >= C4_MIRROR_RG_END) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *lba = HOMEWRECKER_DB_C4_MIRROR_OFFSET + (object_id - C4_MIRROR_RG_START) * HOMEWRECKER_DB_C4_MIRROR_OBJ_SIZE;
    return FBE_STATUS_OK;
}

static fbe_status_t c4_mirror_cache_get_pdoinfo_ptr(fbe_object_id_t pvd_object,
                                                   c4_mirror_pdoinfo_t **pdoinfo)
{
    *pdoinfo = NULL;
    if (pvd_object < 1 || pvd_object > 4) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *pdoinfo = &c4_mirror_cache.current_pdo[pvd_object - 1];
    return FBE_STATUS_OK;
}

static fbe_status_t c4_mirror_cache_get_rginfo_ptr(fbe_object_id_t object_id,
                                                  c4_mirror_rginfo_cache_t **rginfo)
{
    *rginfo = NULL;
    if (object_id < C4_MIRROR_RG_START || object_id >= C4_MIRROR_RG_END) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *rginfo = &c4_mirror_cache.rg[object_id - C4_MIRROR_RG_START];
    return FBE_STATUS_OK;
}


static fbe_status_t c4_mirror_cache_get_pvd_edge(fbe_object_id_t rg_obj,
                                                fbe_object_id_t pvd_obj,
                                                fbe_u32_t *edge_idx)
{
    fbe_u32_t i;
    c4_mirror_rginfo_cache_t *rginfo;

    c4_mirror_cache_get_rginfo_ptr(rg_obj, &rginfo);
    if (rginfo == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get ptr for rg 0x%x\n",
                       __FUNCTION__, rg_obj);
        return FBE_STATUS_GENERIC_FAILURE;

    }
    for (i = 0; i < ARRAY_SIZE(rginfo->pvd_id); i += 1) {
        if (rginfo->pvd_id[i] == pvd_obj) {
            *edge_idx = i;
            return FBE_STATUS_OK;
        }
    }
    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: failed to find edge for rg 0x%x, pvd: 0x%x\n",
                   __FUNCTION__, rg_obj, pvd_obj);
    return FBE_STATUS_GENERIC_FAILURE;
}

static fbe_status_t c4_mirror_cache_verify_pdoinfo(c4_mirror_pdoinfo_t *pdoinfo)
{
    pdoinfo->sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] = '\0';
    if (pdoinfo->slot != C4_MIRROR_SLOT_INVALID && pdoinfo->slot >= 4) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid slot %u\n", __FUNCTION__, pdoinfo->slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

static fbe_status_t c4_mirror_cache_verify_rginfo(c4_mirror_rginfo_disk_t *rginfo_disk)
{
    fbe_u32_t i;
    c4_mirror_rginfo_t *rginfo = &rginfo_disk->rginfo;

    if (rginfo_disk->magic != C4_MIRROR_MAGIC_NUMBER) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid magic: 0x%x, expected: 0x%x\n",
                       __FUNCTION__, rginfo_disk->magic, C4_MIRROR_MAGIC_NUMBER);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (i = 0; i < ARRAY_SIZE(rginfo->pdo_info); i += 1) {
        fbe_status_t status = c4_mirror_cache_verify_pdoinfo(&rginfo->pdo_info[i]);
        if (status != FBE_STATUS_OK) {
            return status;
        }
    }
    return FBE_STATUS_OK;
}

static fbe_status_t c4_mirror_cache_calculate_checksum(void *block_p, c4_mirror_rginfo_t *rginfo, fbe_u32_t count)
{
    fbe_u32_t *reference_ptr;
    fbe_u32_t block_index;

    for (block_index = 0; block_index < count; block_index++)
    {
        fbe_xor_lib_raw_mirror_sector_set_seq_num(block_p, rginfo->seq_number);
        reference_ptr = ( fbe_u32_t * ) ((char *)block_p + DATABASE_HOMEWRECKER_DB_RAW_BLOCK_DATA_SIZE);
        *reference_ptr = fbe_xor_lib_calculate_checksum((fbe_u32_t*)block_p);
        reference_ptr++;

        *reference_ptr = 0;

        block_p = (char *) block_p + DATABASE_SYSTEM_DB_BLOCK_SIZE;

    }
    return FBE_STATUS_OK;
}

static fbe_status_t c4_mirror_cache_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_object_id_t rg_obj = (fbe_object_id_t)(csx_ptrhld_t)context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_sg_element_t *sg;
    c4_mirror_rginfo_disk_t *read_info;
    c4_mirror_rginfo_cache_t *cache_ptr = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_ex_release_block_operation(payload, block_operation);


    if (status != FBE_STATUS_OK || block_operation->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: read failed: status =%d, block_operation_status = %d\n",
                       __FUNCTION__, status, block_operation_status);
        fbe_memory_free_request_entry(&packet->memory_request);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    fbe_payload_ex_get_sg_list(payload, &sg, NULL);
    if(sg == NULL || sg->address == NULL || sg->count == 0) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: sg is NULL: %p\n", __FUNCTION__, sg);
        fbe_memory_free_request_entry(&packet->memory_request);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    read_info = (c4_mirror_rginfo_disk_t *)fbe_sg_element_address(sg);
    status = c4_mirror_cache_verify_rginfo(read_info);

    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid rginfo format for RG 0x%x\n",
                       __FUNCTION__, rg_obj);
        fbe_memory_free_request_entry(&packet->memory_request);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    } else {
        fbe_u32_t edge_idx;
        c4_mirror_rginfo_t *info = &read_info->rginfo;

        for (edge_idx = 0; edge_idx < ARRAY_SIZE(info->pdo_info); edge_idx += 1) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: load rg 0x%x: (%s %u)\n",
                           __FUNCTION__, rg_obj,
                           info->pdo_info[edge_idx].sn,
                           info->pdo_info[edge_idx].slot);
        }
    }

    c4_mirror_cache_get_rginfo_ptr(rg_obj, &cache_ptr);
    if (cache_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get ptr for object: 0x%x\n",
                       __FUNCTION__, rg_obj);
        fbe_memory_free_request_entry(&packet->memory_request);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&c4_mirror_cache.lock);
    fbe_copy_memory(&cache_ptr->rginfo, &read_info->rginfo, sizeof(c4_mirror_rginfo_t));
    cache_ptr->is_loaded = FBE_TRUE;
    fbe_spinlock_unlock(&c4_mirror_cache.lock);

    fbe_memory_free_request_entry(&packet->memory_request);

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Load 0x%x: %u: (%s %u), %u: (%s %u)\n",
                   __FUNCTION__, rg_obj,
                   cache_ptr->pvd_id[0], cache_ptr->rginfo.pdo_info[0].sn, cache_ptr->rginfo.pdo_info[0].slot,
                   cache_ptr->pvd_id[1], cache_ptr->rginfo.pdo_info[1].sn, cache_ptr->rginfo.pdo_info[1].slot);

    return FBE_STATUS_OK;
}

static fbe_status_t c4_mirror_cache_read_allocate_completion(fbe_memory_request_t * memory_request_p,
                                                            fbe_memory_completion_context_t in_context)
{
    fbe_object_id_t rg_obj = (fbe_object_id_t)(csx_ptrhld_t)in_context;
    fbe_packet_t *packet_p;
    fbe_sg_element_t *sg_list_p;
    fbe_u32_t sg_element_count;
    fbe_u8_t *buff;
    fbe_memory_header_t *memory_header_p;
    fbe_memory_header_t *next_memory_header_p = NULL;
    fbe_u64_t start_lba;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_block_operation_t *block_operation = NULL;

    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
        /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this doesn't expect any aborted requests */
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    sg_list_p = (fbe_sg_element_t *)fbe_memory_header_to_data(memory_header_p);
    sg_element_count = 1;

    fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
    if (next_memory_header_p == NULL) {
        /* Currently this doesn't expect any aborted requests */
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: error accessing memory for buffer.\n",
                       __FUNCTION__);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    buff = (fbe_u8_t *)fbe_memory_header_to_data(next_memory_header_p);
    fbe_sg_element_init(&sg_list_p[0], 1 * DATABASE_HOMEWRECKER_DB_BLOCK_SIZE, buff);
    fbe_sg_element_terminate(&sg_list_p[1]);

    c4_mirror_cache_get_offset(rg_obj, &start_lba);

    payload = fbe_transport_get_payload_ex(packet_p);
    block_operation =  fbe_payload_ex_allocate_block_operation(payload);
    fbe_payload_ex_set_sg_list(payload, sg_list_p, sg_element_count);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      start_lba,    /* LBA */
                                      1,
                                      DATABASE_HOMEWRECKER_DB_BLOCK_SIZE,    /* block size: 520  */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    fbe_transport_set_completion_function(packet_p, c4_mirror_cache_read_completion, (void *)(csx_ptrhld_t)rg_obj);
    fbe_payload_ex_increment_block_operation_level(payload);

    fbe_raw_mirror_ex_send_io_packet_to_raid_library(&fbe_database_homewrecker_db,
                                                     packet_p,
                                                     FBE_SEND_IO_TO_RAW_MIRROR_BITMASK);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t c4_mirror_cache_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_object_id_t rg_obj = (fbe_object_id_t)(csx_ptrhld_t)context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;

    fbe_memory_free_request_entry(&packet->memory_request);

    payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);
    fbe_payload_ex_release_block_operation(payload, block_operation);


    /*1. check packet status*/
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: write failed: status =%d, block_operation_status = %d\n",
                       __FUNCTION__, status, block_operation_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*2. check error report, which applied to both block operation success or failure
      cases*/
    if (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: pvd metadata persist done for rg 0x%x\n",
                       __FUNCTION__, rg_obj);
        if (block_operation_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH) {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: detected sequence not match\n", __FUNCTION__);
        }
    } else {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: write failed for rg 0x%x, status: %d, qualifier %d \n",
                       __FUNCTION__, rg_obj,
                       block_operation_status, block_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}


static fbe_status_t c4_mirror_cache_persist_allocate_completion(fbe_memory_request_t * memory_request_p,
                                                               fbe_memory_completion_context_t in_context)
{
    fbe_object_id_t rg_obj = (fbe_object_id_t)(csx_ptrhld_t)in_context;
    fbe_packet_t *packet_p;
    fbe_sg_element_t *sg_list_p;
    fbe_u32_t sg_element_count;
    fbe_u8_t *buff;
    fbe_memory_header_t *memory_header_p;
    fbe_memory_header_t *next_memory_header_p = NULL;
    c4_mirror_rginfo_cache_t *rginfo_ptr = NULL;
    c4_mirror_rginfo_disk_t *write_info = NULL;
    fbe_u64_t start_lba;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_block_operation_t *block_operation = NULL;

    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
        /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this doesn't expect any aborted requests */
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    c4_mirror_cache_get_rginfo_ptr(rg_obj, &rginfo_ptr);
    if (rginfo_ptr == NULL) {
        /* Currently this doesn't expect any aborted requests */
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get ptr for rg 0x%x\n",
                       __FUNCTION__, rg_obj);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    sg_list_p = (fbe_sg_element_t *)fbe_memory_header_to_data(memory_header_p);
    sg_element_count = 1;

    fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
    if (next_memory_header_p == NULL) {
        /* Currently this doesn't expect any aborted requests */
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: error accessing memory for buffer.\n",
                       __FUNCTION__);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    buff = (fbe_u8_t *)fbe_memory_header_to_data(next_memory_header_p);
    sg_list_p[0].address = buff;
    sg_list_p[0].count = 8 * DATABASE_HOMEWRECKER_DB_BLOCK_SIZE;
    /* Terminal */
    sg_list_p[1].address = NULL;
    sg_list_p[1].count = 0;

    fbe_zero_memory(buff, 8 * DATABASE_HOMEWRECKER_DB_BLOCK_SIZE);
    write_info = (c4_mirror_rginfo_disk_t *)buff;
    write_info->magic = C4_MIRROR_MAGIC_NUMBER;

    fbe_spinlock_lock(&c4_mirror_cache.lock);
    fbe_copy_memory(&write_info->rginfo, &rginfo_ptr->rginfo, sizeof(write_info->rginfo));
    c4_mirror_cache_calculate_checksum(buff, &rginfo_ptr->rginfo, 8);
    fbe_spinlock_unlock(&c4_mirror_cache.lock);

    c4_mirror_cache_get_offset(rg_obj, &start_lba);

    payload = fbe_transport_get_payload_ex(packet_p);
    block_operation =  fbe_payload_ex_allocate_block_operation(payload);
    fbe_payload_ex_set_sg_list(payload, sg_list_p, sg_element_count);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      start_lba,    /* LBA */
                                      8,
                                      DATABASE_HOMEWRECKER_DB_BLOCK_SIZE,    /* block size: 520  */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    fbe_transport_set_completion_function(packet_p, c4_mirror_cache_persist_completion, (void *)(csx_ptrhld_t)rg_obj);
    fbe_payload_ex_increment_block_operation_level(payload);

    fbe_raw_mirror_ex_send_io_packet_to_raid_library(&fbe_database_homewrecker_db,
                                                     packet_p,
                                                     FBE_SEND_IO_TO_RAW_MIRROR_BITMASK);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


static fbe_status_t c4_mirror_cache_start_io(fbe_packet_t *packet, fbe_object_id_t rg_obj,
                                            fbe_memory_completion_function_t completion_func)
{
    fbe_memory_request_t*               memory_request_p = NULL;
    fbe_memory_request_priority_t       memory_request_priority = 0;
    void *context = (void *)(csx_ptrhld_t)rg_obj;


    memory_request_p = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   2, /*1 for sg list */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet),
                                   (fbe_memory_completion_function_t) completion_func,
                                   context /* completion_context */);
    fbe_transport_memory_request_set_io_master(memory_request_p,  packet);
    fbe_memory_request_entry(memory_request_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t c4_mirror_rginfo_update_memory(fbe_object_id_t obj, fbe_u32_t mask, fbe_bool_t is_init)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    c4_mirror_rginfo_cache_t *cache_rg_ptr;
    c4_mirror_rginfo_t new_rginfo;
    fbe_u32_t edge_idx;

    c4_mirror_cache_get_rginfo_ptr(obj, &cache_rg_ptr);
    if (cache_rg_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get ptr for rg 0x%x\n",
                       __FUNCTION__, obj);
        return status;
    }
    if (!is_init && !cache_rg_ptr->is_loaded) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: rg 0x%x wasn't loaded\n", __FUNCTION__, obj);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&c4_mirror_cache.lock);
    fbe_copy_memory(&new_rginfo, &cache_rg_ptr->rginfo, sizeof(new_rginfo));
    if (is_init) {
        new_rginfo.seq_number = HOMEWRECKER_DB_RAW_MIRROR_START_SEQ;
    } else {
        new_rginfo.seq_number += 1;
    }
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: update 0x%x seq: 0x%llx\n",
                   __FUNCTION__, obj,
                   (unsigned long long)new_rginfo.seq_number);
    for (edge_idx = 0; edge_idx < ARRAY_SIZE(cache_rg_ptr->pvd_id); edge_idx += 1) {
        c4_mirror_pdoinfo_t *pdoinfo_ptr;
        fbe_object_id_t pvd_obj;

        if ((mask & (1 << edge_idx)) == 0) {
            continue;
        }
        pvd_obj = cache_rg_ptr->pvd_id[edge_idx];
        c4_mirror_cache_get_pdoinfo_ptr(pvd_obj, &pdoinfo_ptr);
        if (pdoinfo_ptr == NULL) {
            fbe_spinlock_unlock(&c4_mirror_cache.lock);
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get ptr for object: rg: 0x%x, pvd: 0x%x\n",
                           __FUNCTION__, obj, pvd_obj);
            return status;
        }
        fbe_copy_memory(&new_rginfo.pdo_info[edge_idx], pdoinfo_ptr, sizeof(c4_mirror_pdoinfo_t));
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: update 0x%x pvd 0x%x: (%s %u) -> (%s %u)\n",
                       __FUNCTION__, obj, pvd_obj,
                       cache_rg_ptr->rginfo.pdo_info[edge_idx].sn,
                       cache_rg_ptr->rginfo.pdo_info[edge_idx].slot,
                       new_rginfo.pdo_info[edge_idx].sn,
                       new_rginfo.pdo_info[edge_idx].slot);
    }
    fbe_copy_memory(&cache_rg_ptr->rginfo, &new_rginfo, sizeof(new_rginfo));
    fbe_spinlock_unlock(&c4_mirror_cache.lock);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_c4_mirror_load_rginfo(fbe_object_id_t obj, fbe_packet_t *packet_p)
{
    fbe_status_t status;
    c4_mirror_rginfo_cache_t *cache_ptr;

    /* Verify it is a valid raid group */
    c4_mirror_cache_get_rginfo_ptr(obj, &cache_ptr);
    if (cache_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get ptr for object: 0x%x\n",
                       __FUNCTION__, obj);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = c4_mirror_cache_start_io(packet_p, obj,
                                     (fbe_memory_completion_function_t)c4_mirror_cache_read_allocate_completion);
    return status;
}

fbe_status_t fbe_c4_mirror_load_rginfo_during_boot_path(fbe_object_id_t obj)
{
    fbe_packet_t    *packet = NULL;
    fbe_status_t    status;
    fbe_payload_ex_t    *payload = NULL;
    fbe_cpu_id_t    cpu_id;

    if (obj == FBE_OBJECT_ID_INVALID)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: invalid object_id \n", 
                       __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    packet = fbe_transport_allocate_packet();
    if(packet == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: fbe_transport_allocate_packet failed\n", 
                       __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    fbe_payload_ex_allocate_memory_operation(payload);
    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    
    fbe_c4_mirror_load_rginfo(obj, packet);

    fbe_transport_wait_completion(packet);
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: load rginfo packet return error:%d\n", __FUNCTION__, status);
        fbe_transport_release_packet(packet);
        return status;
    }
    fbe_transport_release_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_c4_mirror_rginfo_update(fbe_packet_t *packet, fbe_object_id_t rg_obj, fbe_u32_t edge)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t max_edge = ARRAY_SIZE(((c4_mirror_rginfo_t *)0)->pdo_info);

    if (edge >= max_edge) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid edge %u for rg: 0x%x\n",
                       __FUNCTION__, edge, rg_obj);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = c4_mirror_rginfo_update_memory(rg_obj, 1 << edge, FBE_FALSE);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: rg 0x%x failed to update, edge: %u\n", __FUNCTION__, rg_obj, edge);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = c4_mirror_cache_start_io(packet, rg_obj,
                                     (fbe_memory_completion_function_t)c4_mirror_cache_persist_allocate_completion);
    return status;
}

fbe_status_t fbe_c4_mirror_check_new_pvd(fbe_object_id_t rg_obj, fbe_u32_t *bitmask)
{
    c4_mirror_rginfo_cache_t *cache_rg_ptr;
    c4_mirror_rginfo_t *rginfo;
    fbe_u32_t i;

    *bitmask = 0;
    c4_mirror_cache_get_rginfo_ptr(rg_obj, &cache_rg_ptr);
    if (cache_rg_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get ptr for object: rg: 0x%x\n",
                       __FUNCTION__, rg_obj);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (!cache_rg_ptr->is_loaded) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: rg 0x%x wasn't loaded\n",
                       __FUNCTION__, rg_obj);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    rginfo = &cache_rg_ptr->rginfo;
    fbe_spinlock_lock(&c4_mirror_cache.lock);
    for (i = 0; i < sizeof(rginfo->pdo_info) / sizeof(rginfo->pdo_info[0]); i += 1) {
        fbe_u32_t slot;
        c4_mirror_pdoinfo_t *cur_pdo;

        slot = rginfo->pdo_info[i].slot;
        if (slot == C4_MIRROR_SLOT_INVALID) {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: slot is empty for edge %u rg 0x%x\n",
                           __FUNCTION__, i, rg_obj);
            *bitmask |= 1 << i;
            continue;
        }
        if (slot >= C4_MIRROR_SYS_DRIVES) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: invalid slot %u for edge %u rg 0x%x\n",
                           __FUNCTION__, slot, i, rg_obj);
            /* Mark as invalid */
            *bitmask |= 1 << i;
            continue;
        }
        cur_pdo = &c4_mirror_cache.current_pdo[slot];
        if (memcmp(rginfo->pdo_info[i].sn, cur_pdo->sn,
                   FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE) != 0) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: new pvd for edge %u rg 0x%x\n",
                           __FUNCTION__, i, rg_obj);
            *bitmask |= 1 << i;
            continue;
        }
    }
    fbe_spinlock_unlock(&c4_mirror_cache.lock);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_c4_mirror_update_pvd(fbe_u32_t slot, fbe_u8_t *sn, fbe_u32_t sn_len)
{
    c4_mirror_pdoinfo_t *pdo;
    fbe_u32_t copy_len;

    if (slot >= C4_MIRROR_SYS_DRIVES) {
        return FBE_STATUS_OK;
    }


    pdo = &c4_mirror_cache.current_pdo[slot];
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: update sn for slot %u: %s -> %s\n",
                   __FUNCTION__, slot,
                   pdo->sn, sn);

    fbe_spinlock_lock(&c4_mirror_cache.lock);
    copy_len = sn_len < FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE? sn_len : FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE;
    fbe_zero_memory(pdo, sizeof(pdo));
    pdo->slot = slot;
    fbe_copy_memory(pdo->sn, sn, copy_len);
    fbe_spinlock_unlock(&c4_mirror_cache.lock);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_c4_mirror_update_pvd_by_object_id(fbe_object_id_t pvd_obj, fbe_u8_t *sn, fbe_u32_t sn_len)
{
    fbe_u32_t slot;

    if (pvd_obj > FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Invalid pvd object: 0x%x\n", __FUNCTION__, pvd_obj);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    slot = pvd_obj - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST;
    return fbe_c4_mirror_update_pvd(slot, sn, sn_len);
}

fbe_status_t fbe_c4_mirror_is_load(fbe_object_id_t rg_obj, fbe_bool_t *is_loaded)
{
    c4_mirror_rginfo_cache_t *cache_rg_ptr;

    *is_loaded = FBE_FALSE;
    c4_mirror_cache_get_rginfo_ptr(rg_obj, &cache_rg_ptr);
    if (cache_rg_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get ptr for object: rg: 0x%x\n",
                       __FUNCTION__, rg_obj);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *is_loaded = cache_rg_ptr->is_loaded;

    return FBE_STATUS_OK;
}

fbe_status_t fbe_c4_mirror_write_default(fbe_packet_t *packet_p, fbe_object_id_t rg_obj)
{
    fbe_status_t status;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Writing default map for RG 0x%x\n",
                   __FUNCTION__, rg_obj);
    status = c4_mirror_rginfo_update_memory(rg_obj, 0xff, FBE_TRUE);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Failed to update cache for RG 0x%x\n",
                       __FUNCTION__, rg_obj);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    status = c4_mirror_cache_start_io(packet_p, rg_obj,
                                     (fbe_memory_completion_function_t)c4_mirror_cache_persist_allocate_completion);
    return status;
}

fbe_status_t fbe_c4_mirror_update_default(fbe_object_id_t rg_obj)
{
    fbe_packet_t *packet;
    fbe_payload_ex_t *payload = NULL;
    fbe_status_t status;
    fbe_cpu_id_t cpu_id;

    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to allocate packet\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    fbe_payload_ex_allocate_memory_operation(payload);
    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_c4_mirror_write_default(packet, rg_obj);
    fbe_transport_wait_completion(packet);

    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: persit failed\n", __FUNCTION__);
    }
    fbe_transport_release_packet(packet);
    return status;
}

fbe_status_t fbe_c4_mirror_rginfo_modify_drive_SN(fbe_object_id_t rg_obj_id, 
                                               fbe_u16_t edge_index, fbe_u8_t *sn, fbe_u32_t size)
{
    fbe_packet_t *packet;
    fbe_payload_ex_t *payload = NULL;
    fbe_status_t status;
    fbe_cpu_id_t cpu_id;
    c4_mirror_rginfo_cache_t *rginfo_cache;

    c4_mirror_cache_get_rginfo_ptr(rg_obj_id, &rginfo_cache);
    if (rginfo_cache == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get ptr for rg 0x%x\n",
                       __FUNCTION__, rg_obj_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!rginfo_cache->is_loaded)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: rginfo_cache has not been loaded yet\n",
                       __FUNCTION__);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    /* modify the rginfo_cache rginfo's drive serial number with input value */
    fbe_spinlock_lock(&c4_mirror_cache.lock);
    fbe_copy_memory(rginfo_cache->rginfo.pdo_info[edge_index].sn, sn, size);
    fbe_spinlock_unlock(&c4_mirror_cache.lock);
    
    /* Persist the new rginfo synchronously */
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to allocate packet\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    fbe_payload_ex_allocate_memory_operation(payload);
    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    status = c4_mirror_cache_start_io(packet, rg_obj_id,
                                     (fbe_memory_completion_function_t)c4_mirror_cache_persist_allocate_completion);
    
    fbe_transport_wait_completion(packet);

    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: persit failed\n", __FUNCTION__);
    }
    fbe_transport_release_packet(packet);
    return status;
}
                                               
fbe_status_t fbe_c4_mirror_rginfo_get_pvd_mapping(fbe_object_id_t rg_obj_id, 
                                               fbe_u16_t edge_index, fbe_u8_t *sn, fbe_u32_t size)
{
    c4_mirror_rginfo_cache_t *rginfo_cache;

    c4_mirror_cache_get_rginfo_ptr(rg_obj_id, &rginfo_cache);
    if (rginfo_cache == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get ptr for rg 0x%x\n",
                       __FUNCTION__, rg_obj_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!rginfo_cache->is_loaded)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: rginfo_cache has not been loaded yet\n",
                       __FUNCTION__);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    /* modify the rginfo_cache rginfo's drive serial number with input value */
    fbe_spinlock_lock(&c4_mirror_cache.lock);
    fbe_copy_memory(sn, rginfo_cache->rginfo.pdo_info[edge_index].sn, size);
    fbe_spinlock_unlock(&c4_mirror_cache.lock);
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_c4_mirror_get_nonpaged_metadata_config()
 *****************************************************************
 * @brief
 *   This function get the raw mirror region of the c4mirror's nonpaged metadata.
 *
 * @param raw_mirror_p
 * @param start_lba_p
 * @param capacity_p
 *
 * @return fbe_status_t - FBE_STATUS_OK
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_c4_mirror_get_nonpaged_metadata_config(void **raw_mirror_pp,
                                                            fbe_lba_t *start_lba_p,
                                                            fbe_block_count_t *capacity_p)
{
    /* The nonpaged metadata of the c4mirror objects persist in the homewrecker_db raw mirror
     */
    if (raw_mirror_pp != NULL)
    {
        *raw_mirror_pp = &fbe_database_homewrecker_db;
    }
    /* The lba offset of the c4mirror nonpaged region
     */
    if (start_lba_p != NULL)
    {
        *start_lba_p = HOMEWRECKER_DB_C4_MIRROR_NONPAGED_START_LBA; /* should be 4k aligned */
    }
    /* The capacity of the c4mirror nonpaged region
     */
    if (capacity_p != NULL)
    {
        *capacity_p =  HOMEWRECKER_DB_C4_MIRROR_NONPAGED_BLOCK_COUNT; /* should be 4k aligned */
    }

    return FBE_STATUS_OK;
}
