#include "fbe/fbe_transport.h"
#include "fbe_traffic_trace.h"
#include "fbe/fbe_payload_ex.h"
#include "sas_physical_drive_private.h"
#include "fbe_scsi.h"


#define ZERO_SENSE_BUFFER 0

/*************************
 *   FORWARD FUNCTION DEFINITIONS
 *************************/
static __forceinline fbe_status_t sas_physical_drive_cdb_build_10_byte(fbe_payload_cdb_operation_t* cdb_operation,
                                                  fbe_time_t timeout,
                                                  fbe_lba_t lba,
                                                  fbe_block_count_t block_count);

static __forceinline fbe_status_t sas_physical_drive_cdb_build_16_byte(fbe_payload_cdb_operation_t* cdb_operation,
                                                  fbe_time_t timeout,
                                                  fbe_lba_t lba,
                                                  fbe_block_count_t block_count);

static __forceinline fbe_status_t sas_physical_drive_handle_write_same_16(fbe_sas_physical_drive_t* sas_physical_drive_p,
                                        fbe_payload_ex_t* payload,
                                        fbe_payload_cdb_operation_t* cdb_operation,
                                        fbe_time_t timeout,
                                        fbe_bool_t unmap);


/* Update lba and block to take into account physical block size */
static fbe_status_t sas_physical_drive_cdb_convert_to_physical_block_size(fbe_sas_physical_drive_t *const sas_physical_drive_p,
                                                                          const fbe_block_size_t client_block_size,
                                                                          const fbe_block_size_t physical_block_size,
                                                                          fbe_lba_t *const lba_p, 
                                                                          fbe_block_count_t *const blocks_p)
{
    fbe_block_count_t   aligned_client_blocks;
    
    if (client_block_size == physical_block_size)
    {
        return FBE_STATUS_OK;
    }

    /*! @note Block Size Virtual is not longer supported.  Therefore the 
     *        following are require if the physical block size:
     *          o Must be equal or greater than client block size
     *          o Must be a multiple of block size
     */
    if ((physical_block_size < client_block_size)        ||
        ((physical_block_size % client_block_size) != 0)    )
    {
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive_p,
                              FBE_TRACE_LEVEL_CRITICAL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s client block size: %d not a multiple of physical: %d\n", 
                              __FUNCTION__, client_block_size, physical_block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the start and end of the command is aligned to the
     * optimal block size, then we are considered aligned.
     */
    aligned_client_blocks = (physical_block_size / client_block_size);
    if (((*lba_p % aligned_client_blocks) == 0)               &&
        (((*lba_p + *blocks_p) % aligned_client_blocks) == 0)    )
    {
        /* Simply update the lba and blocks */
#if 0
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "physical_drive: lba/bl: %llx/%llx -> %llx/%llx\n",
                              *lba_p, *blocks_p, (*lba_p / aligned_client_blocks), (*blocks_p / aligned_client_blocks));
#endif
        *lba_p = *lba_p / aligned_client_blocks;
        *blocks_p = *blocks_p / aligned_client_blocks;
        return FBE_STATUS_OK;
    }

    if (physical_block_size == FBE_4K_BYTES_PER_BLOCK) {
        fbe_lba_t lba = *lba_p;
        fbe_block_count_t blocks = *blocks_p;
        fbe_lba_t end_lba = lba + blocks - 1;

        /* Align the lba and block count (if needed).
         */
        lba = (lba / FBE_520_BLOCKS_PER_4K);
        end_lba = (end_lba / FBE_520_BLOCKS_PER_4K);
        blocks = end_lba - lba + 1;
#if 0
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "physical_drive: lba/bl: %llx/%llx -> %llx/%llx\n",
                              *lba_p, *blocks_p, lba, blocks);
#endif
        *lba_p = lba;
        *blocks_p = blocks;
        return FBE_STATUS_OK;
    } else {
        /* Not aligned
         */
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s lba: 0x%llx blks: 0x%llx not aligned to block size: %d\n", 
                              __FUNCTION__, *lba_p, *blocks_p, 
                              physical_block_size);   
        return FBE_STATUS_GENERIC_FAILURE;
    }
}


/* Update physical lba and block to take into account logical block size */
fbe_status_t sas_physical_drive_cdb_convert_to_logical_block_size(fbe_sas_physical_drive_t *const sas_physical_drive_p,
                                                                          const fbe_block_size_t client_block_size,
                                                                          const fbe_block_size_t physical_block_size,
                                                                          fbe_lba_t *const lba_p, 
                                                                          fbe_block_count_t *const blocks_p)
{
    fbe_block_count_t   aligned_client_blocks;

    if (client_block_size == physical_block_size)
    {
        return FBE_STATUS_OK;
    }

    /*! @note Block Size Virtual is not longer supported.  Therefore the 
     *        following are require if the physical block size:
     *          o Must be equal or greater than client block size
     *          o Must be a multiple of block size
     */
    if ((physical_block_size < client_block_size)        ||
        ((physical_block_size % client_block_size) != 0)    )
    {
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive_p,
                              FBE_TRACE_LEVEL_CRITICAL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s client block size: %d not a multiple of physical: %d\n", 
                              __FUNCTION__, client_block_size, physical_block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    aligned_client_blocks = (physical_block_size / client_block_size);
    *lba_p = *lba_p * aligned_client_blocks;

    if (blocks_p != NULL)
    {
        *blocks_p = *blocks_p * aligned_client_blocks;
    }

    return FBE_STATUS_OK;
}


fbe_status_t 
sas_physical_drive_cdb_build_reserve(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER;
    
    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 10;
    /* OPERATION CODE (00h)*/
    cdb_operation->cdb[0] = FBE_SCSI_RESERVE_10;

    return FBE_STATUS_OK;
}

fbe_status_t 
sas_physical_drive_cdb_build_reassign(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_bool_t longlba)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;

    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 6;
    /* OPERATION CODE (07h)*/
    cdb_operation->cdb[0] = FBE_SCSI_REASSIGN_BLOCKS;
    if (longlba) {
        cdb_operation->cdb[1] = 2;
    }

    return FBE_STATUS_OK;
}
fbe_status_t 
sas_physical_drive_cdb_build_release(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER;
    
    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 10;
    /* OPERATION CODE (00h)*/
    cdb_operation->cdb[0] = FBE_SCSI_RELEASE_10;

    return FBE_STATUS_OK;
}

fbe_status_t 
sas_physical_drive_cdb_build_download(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u32_t image_size)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT | FBE_PAYLOAD_CDB_FLAGS_DRIVE_FW_UPGRADE;
    
    cdb_operation->timeout = timeout;

    /* construct cdb */
    cdb_operation->cdb_length = 10;

    cdb_operation->cdb[0] = FBE_SCSI_WRITE_BUFFER;
    cdb_operation->cdb[1] = 0x05;

    cdb_operation->cdb[6] = ((image_size & 0x00ff0000) >> 16);
    cdb_operation->cdb[7] = ((image_size & 0x0000ff00) >> 8);
    cdb_operation->cdb[8] = (image_size & 0x000000ff);

    return FBE_STATUS_OK;
}

/*!*************************************************************
 * @fn sas_physical_drive_cdb_build_send_diagnostics()
 ***************************************************************
 * @brief
 * This function builds the Send Diagnostic CDB, including any
 * self test bits that are specified by the caller.
 *
 * @param cdb_operation = Pointer to the cdb payload struct.
 * @param timeout - When to timeout the CDB completion
 * @param test_bits - If this CDB enables a drive self test.
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  04/24/2012 - Created. Darren Insko
 *
 ***************************************************************/
fbe_status_t 
sas_physical_drive_cdb_build_send_diagnostics(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u8_t test_bits)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER;

    cdb_operation->timeout = timeout;

    /* construct cdb */
    cdb_operation->cdb_length = 6;
    /* OPERATION CODE (1Dh)*/
    cdb_operation->cdb[0] = FBE_SCSI_SEND_DIAGNOSTIC;
    cdb_operation->cdb[1] = test_bits;
    cdb_operation->cdb[2] = 0;
    cdb_operation->cdb[3] = 0;
    cdb_operation->cdb[4] = 0;
    cdb_operation->cdb[5] = 0;

    return FBE_STATUS_OK;
}
/******************************************************
 * end sas_physical_drive_cdb_build_send_diagnostics()
 ******************************************************/

fbe_status_t 
sas_physical_drive_cdb_build_receive_diag(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, 
                                          fbe_u8_t page_code, fbe_u16_t resp_size)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;

    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 6;
    /* OPERATION CODE (1Ch)*/
    cdb_operation->cdb[0] = FBE_SCSI_RECEIVE_DIAGNOSTIC;
    /* PCV = 1: the contents of the Page Code field shall define the data returned. */
    cdb_operation->cdb[1] = 0x01;
    cdb_operation->cdb[2] = page_code;

    /* ALLOCATION LENGTH */
    cdb_operation->cdb[3] = ((resp_size & 0xff00) >> 8); /* MSB */
    cdb_operation->cdb[4] = (resp_size & 0x00ff); /* LSB */

    return FBE_STATUS_OK;
}

fbe_status_t 
sas_physical_drive_cdb_build_log_sense(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, 
                                          fbe_u8_t page_code, fbe_u16_t size)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;

    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 10;
    /* OPERATION CODE (4Dh)*/
    cdb_operation->cdb[0] = FBE_SCSI_LOG_SENSE;
    cdb_operation->cdb[2] = page_code;
    
    /* This is required for Hitachi HDD drive to get the log page 0x15 (BMS). */
    cdb_operation->cdb[2] |= FBE_LOG_SENSE_SPF;

    /* ALLOCATION LENGTH */
    cdb_operation->cdb[7] = ((size & 0xff00) >> 8); /* MSB */
    cdb_operation->cdb[8] = (size & 0x00ff); /* LSB */

    return FBE_STATUS_OK;
}

fbe_status_t 
sas_physical_drive_cdb_build_smart_dump(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, 
                                        fbe_u32_t size)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;

    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 10;
    /* OPERATION CODE (E6h)*/
    cdb_operation->cdb[0] = FBE_SCSI_SMART_DUMP;

    /* ALLOCATION LENGTH */
    cdb_operation->cdb[6] = ((size & 0x00ff0000) >> 16); /* MSB */
    cdb_operation->cdb[7] = ((size & 0x0000ff00) >> 8); 
    cdb_operation->cdb[8] = (size & 0x000000ff); /* LSB */

    return FBE_STATUS_OK;
}


/*!*************************************************************
 * @fn sas_physical_drive_convert_read()
 ***************************************************************
 * @brief
 * This will adjust the lba and block count to be based on the physical block size.  Also,
 * if the read is unaligned, the payload is modified to make IO aligned by adding a bit bucket to 
 * the front and/or back of the sg_list.  
 *
 * @param  sas_physical_drive       - ptr to pdo
 * @param  payload                  - ptr to packet payload
 * @param  physical_block_size      - formated block size of drive
 * @param  client_block_size        - exported block size
 * @param  lba                      - client lba passed in, lba based on physical returned
 * @param  block_count              - client block count passed in,  block count based on physical returned.
*
 * @return fbe_status_t - success or failure
 *
 * @version
 *  10/07/2014 - Reworked.  Wayne Garrett
 *
 ***************************************************************/
static __forceinline fbe_status_t 
sas_physical_drive_convert_read(fbe_sas_physical_drive_t* sas_physical_drive,
                                fbe_payload_ex_t* payload,
                                fbe_block_size_t physical_block_size,
                                fbe_block_size_t client_block_size,
                                fbe_lba_t *lba_p,
                                fbe_block_count_t *block_count_p)                                          
{
    fbe_u32_t pre_bytes = 0;
    fbe_u32_t post_bytes = 0;
    fbe_u8_t *bitbucket_addr = fbe_memory_get_bit_bucket_addr();
    fbe_lba_t orig_end_lba = *lba_p + *block_count_p - 1;
    fbe_lba_t orig_lba = *lba_p;
    fbe_lba_t orig_block_count = *block_count_p;
    fbe_status_t status;

    status = sas_physical_drive_cdb_convert_to_physical_block_size(sas_physical_drive, client_block_size, physical_block_size, lba_p, block_count_p);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* If the packet comes from PPFD with FBE_PAYLOAD_FLAG_USE_PHYS_OFFSET set, 
     * we have to set the bitbucket_addr as physical address.
     */
#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
    if (payload->payload_attr & FBE_PAYLOAD_FLAG_USE_PHYS_OFFSET)
    {
        fbe_u64_t addr = fbe_get_contigmem_physical_address(bitbucket_addr);
        bitbucket_addr = (fbe_u8_t *)(addr);
    }
#endif

    /* If we are unaligned at the beginning, add in our bitbucket.
     */
    if ((physical_block_size == FBE_4K_BYTES_PER_BLOCK)){
        fbe_lba_t aligned_520_lba;
        fbe_block_count_t aligned_520_block_count;

        aligned_520_lba = *lba_p * FBE_520_BLOCKS_PER_4K;
        aligned_520_block_count = *block_count_p * FBE_520_BLOCKS_PER_4K;

        if (orig_lba % FBE_520_BLOCKS_PER_4K) {
            fbe_sg_element_t *pre_sg_list = NULL;
            if (aligned_520_lba > orig_lba) {
                fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                                      FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "physical_drive: lba %llx > orig_lba %llx\n",
                                      aligned_520_lba, orig_lba);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            fbe_payload_ex_get_pre_sg_list(payload, &pre_sg_list);
            pre_sg_list[0].address = bitbucket_addr;
            pre_bytes = (fbe_u32_t)(FBE_BE_BYTES_PER_BLOCK * (orig_lba - aligned_520_lba));

            if (pre_bytes > (FBE_520_BLOCKS_PER_4K - 1) * FBE_BE_BYTES_PER_BLOCK){
                fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                                      FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "physical_drive: post bytes: 0x%x too large end_lba: %llx aligned_lba: %llx bc: %llx\n",
                                      pre_bytes, orig_end_lba, aligned_520_lba, aligned_520_block_count );
            }
            pre_sg_list[0].count = pre_bytes;
            fbe_sg_element_terminate(&pre_sg_list[1]);
        }

        /* If we are unaligned at the end, add in our bitbucket.
         */
        if ((orig_lba + orig_block_count) % FBE_520_BLOCKS_PER_4K) {
            fbe_sg_element_t *post_sg_list = NULL;
            if ((aligned_520_lba + aligned_520_block_count - 1) < orig_end_lba) {
                fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                                      FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "physical_drive: block_count %llx > orig_block_count %llx\n",
                                      aligned_520_block_count, orig_block_count);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            fbe_payload_ex_get_post_sg_list(payload, &post_sg_list);
            post_sg_list[0].address = bitbucket_addr;
            post_bytes = (fbe_u32_t)(FBE_BE_BYTES_PER_BLOCK * ((aligned_520_lba + aligned_520_block_count - 1) - orig_end_lba));
            if (post_bytes > (FBE_520_BLOCKS_PER_4K - 1) * FBE_BE_BYTES_PER_BLOCK){
                fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
                                      FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "physical_drive: post bytes: 0x%x too large end_lba: %llx aligned_lba: %llx bc: %llx\n",
                                      post_bytes, orig_end_lba, aligned_520_lba, aligned_520_block_count );
            }
            post_sg_list[0].count = post_bytes;
            fbe_sg_element_terminate(&post_sg_list[1]);
        }
    }
    return FBE_STATUS_OK;
}



/* Function to convert a block read operation to a read_10 */
static __forceinline fbe_status_t 
sas_physical_drive_convert_block_to_cdb_read_10(fbe_sas_physical_drive_t* sas_physical_drive,
                                                             fbe_payload_ex_t* payload,
                                                             fbe_payload_cdb_operation_t* cdb_operation,
                                                             fbe_time_t timeout)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_t* block_operation = NULL;
    fbe_block_count_t block_count= 0;
    fbe_block_size_t block_size = 0;
    fbe_lba_t lba = 0;
    fbe_block_size_t physical_block_size = 0;
    
    block_operation = fbe_payload_ex_get_block_operation(payload);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);    
    fbe_payload_block_get_lba(block_operation, &lba);

    physical_block_size = sas_physical_drive->base_physical_drive.block_size;
    /* If needed convert the lba and blocks based on physical block size. */
    if ( physical_block_size != block_size) {
        status = sas_physical_drive_convert_read(sas_physical_drive, payload, physical_block_size, block_size, &lba, &block_count);
        if (status != FBE_STATUS_OK){
            return status;
        }
    }


#if ZERO_SENSE_BUFFER
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
#endif

    fbe_payload_cdb_set_transfer_count(cdb_operation, (fbe_u32_t)(block_count * physical_block_size));    
      
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;
    cdb_operation->cdb[0] = FBE_SCSI_READ_10;
    cdb_operation->cdb[1] = 0;
    if (fbe_payload_block_is_flag_set(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCE_UNIT_ACCESS))
    {
        cdb_operation->cdb[1] |= FBE_SCSI_FUA;
    }
    
    status = sas_physical_drive_cdb_build_10_byte(cdb_operation, timeout, lba, block_count);

    /*Enhanced Queuing*/
    if(fbe_sas_physical_drive_is_enhanced_queuing_supported(sas_physical_drive)
        && cdb_operation->payload_cdb_task_attribute == FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE
        && cdb_operation->payload_cdb_priority > FBE_PAYLOAD_CDB_PRIORITY_NORMAL)
    {
        /*Set low priority*/
        cdb_operation->cdb[1] |= FBE_SCSI_LOW_PRIORITY;
    }

    /* Allow reading of unmapped sectors so that zero on demand reads of chunks with 
     * data and non data would succeed 
     */
    if (fbe_payload_block_is_flag_set(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_UNMAP_READ))
    {
        cdb_operation->payload_cdb_flags |= FBE_PAYLOAD_CDB_FLAGS_ALLOW_UNMAP_READ;
    }

    if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
    {
        fbe_u16_t extra = fbe_traffic_trace_get_priority_from_cdb(cdb_operation->payload_cdb_priority);
        fbe_sas_physical_drive_trace_rba(sas_physical_drive, cdb_operation, KT_TRAFFIC_READ_START | extra);
    }
    
    return status;
}


/* Function to convert a block read operation to a read_16 */
static __forceinline fbe_status_t 
sas_physical_drive_convert_block_to_cdb_read_16(fbe_sas_physical_drive_t* sas_physical_drive,
                                                             fbe_payload_ex_t* payload,
                                                             fbe_payload_cdb_operation_t* cdb_operation,
                                                             fbe_time_t timeout)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_t* block_operation = NULL;
    fbe_block_count_t block_count= 0;
    fbe_block_size_t block_size = 0;
    fbe_lba_t lba = 0;
    fbe_block_size_t physical_block_size;

    block_operation = fbe_payload_ex_get_block_operation(payload);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);  
    fbe_payload_block_get_lba(block_operation, &lba);

    physical_block_size = sas_physical_drive->base_physical_drive.block_size;


    /* If needed convert the lba and blocks based on physical block size. */
    if (physical_block_size != block_size)
    {
        status = sas_physical_drive_convert_read(sas_physical_drive, payload, physical_block_size, block_size, &lba, &block_count);
        if (status != FBE_STATUS_OK){
            return status;
        }
    }    
     
#if ZERO_SENSE_BUFFER
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
#endif

    fbe_payload_cdb_set_transfer_count(cdb_operation, (fbe_u32_t)(block_count * physical_block_size));
    
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;
    cdb_operation->cdb[0] = FBE_SCSI_READ_16;
    cdb_operation->cdb[1] = 0;
    if (fbe_payload_block_is_flag_set(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCE_UNIT_ACCESS))
    {
        cdb_operation->cdb[1] |= FBE_SCSI_FUA;
    }
    
    status = sas_physical_drive_cdb_build_16_byte(cdb_operation, timeout, lba, block_count);

    /*Enhanced Queuing*/
    if(fbe_sas_physical_drive_is_enhanced_queuing_supported(sas_physical_drive)
        && cdb_operation->payload_cdb_task_attribute == FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE
        && cdb_operation->payload_cdb_priority > FBE_PAYLOAD_CDB_PRIORITY_NORMAL)
    {
        /*Set low priority*/
        cdb_operation->cdb[15] |= FBE_SCSI_LOW_PRIORITY;
    }

    if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
    {
        fbe_u16_t extra = fbe_traffic_trace_get_priority_from_cdb(cdb_operation->payload_cdb_priority);
        fbe_sas_physical_drive_trace_rba(sas_physical_drive, cdb_operation, KT_TRAFFIC_READ_START | extra);
    }
    
    return status;
}


/* This function contains common code to build a 10 byte cdb */
static __forceinline fbe_status_t sas_physical_drive_cdb_build_10_byte(fbe_payload_cdb_operation_t* cdb_operation,
                                                  fbe_time_t timeout,
                                                  fbe_lba_t lba,
                                                  fbe_block_count_t block_count)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    /* Set common parameters */    
    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->timeout = timeout;    
    cdb_operation->payload_sg_descriptor.repeat_count = 1;
    cdb_operation->cdb_length = 10;  
    
    /* Set the lba and blockcount */   
    cdb_operation->cdb[2] = (fbe_u8_t)((lba >> 24) & 0xFF); /* MSB */
    cdb_operation->cdb[3] = (fbe_u8_t)((lba >> 16) & 0xFF); 
    cdb_operation->cdb[4] = (fbe_u8_t)((lba >> 8) & 0xFF);
    cdb_operation->cdb[5] = (fbe_u8_t)(lba & 0xFF); /* LSB */
    cdb_operation->cdb[6] = 0;
    cdb_operation->cdb[7] = (fbe_u8_t)((block_count >> 8) & 0xFF); /* MSB */
    cdb_operation->cdb[8] = (fbe_u8_t)(block_count & 0xFF); /* LSB */
    cdb_operation->cdb[9] = 0;
    
    return status;    
}

/* This function contains common code to build a 16 byte cdb */
static __forceinline fbe_status_t sas_physical_drive_cdb_build_16_byte(fbe_payload_cdb_operation_t* cdb_operation,
                                                  fbe_time_t timeout,
                                                  fbe_lba_t lba,
                                                  fbe_block_count_t block_count)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    /* Set common parameters */    
    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->timeout = timeout;    
    cdb_operation->payload_sg_descriptor.repeat_count = 1;
    cdb_operation->cdb_length = 16;      
    
    /* Set the lba and blockcount */       
    cdb_operation->cdb[2] = (fbe_u8_t)((lba >> 56) & 0xFF); /* MSB */
    cdb_operation->cdb[3] = (fbe_u8_t)((lba >> 48) & 0xFF); 
    cdb_operation->cdb[4] = (fbe_u8_t)((lba >> 40) & 0xFF);
    cdb_operation->cdb[5] = (fbe_u8_t)((lba >> 32) & 0xFF); 
    cdb_operation->cdb[6] = (fbe_u8_t)((lba >> 24) & 0xFF); 
    cdb_operation->cdb[7] = (fbe_u8_t)((lba >> 16) & 0xFF); 
    cdb_operation->cdb[8] = (fbe_u8_t)((lba >> 8) & 0xFF); 
    cdb_operation->cdb[9] = (fbe_u8_t)(lba & 0xFF); /* LSB */
    cdb_operation->cdb[10] = (fbe_u8_t)((block_count >> 24) & 0xFF); /* MSB */
    cdb_operation->cdb[11] = (fbe_u8_t)((block_count >> 16) & 0xFF); 
    cdb_operation->cdb[12] = (fbe_u8_t)((block_count >> 8) & 0xFF);
    cdb_operation->cdb[13] = (fbe_u8_t)(block_count & 0xFF); /* LSB */       
    cdb_operation->cdb[14] = 0;
    cdb_operation->cdb[15] = 0;
    
    return status;
}


/* Function to convert a block write operation to a write_10 */
static __forceinline fbe_status_t 
sas_physical_drive_convert_block_to_cdb_write_10(fbe_sas_physical_drive_t* sas_physical_drive,
                                                              fbe_payload_ex_t* payload,
                                                              fbe_payload_cdb_operation_t* cdb_operation,
                                                              fbe_time_t timeout)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_t* block_operation = NULL;
    fbe_block_count_t block_count= 0;
    fbe_block_size_t block_size = 0;
    fbe_lba_t lba = 0;
    fbe_block_size_t physical_block_size = 0;

    block_operation = fbe_payload_ex_get_block_operation(payload);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);    
    fbe_payload_block_get_lba(block_operation, &lba);

    physical_block_size = sas_physical_drive->base_physical_drive.block_size;

    /* If needed convert the lba and blocks based on physical block size. */
    if (physical_block_size != block_size)
    {
        status = sas_physical_drive_cdb_convert_to_physical_block_size(sas_physical_drive, block_size, physical_block_size, &lba, &block_count);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

#if ZERO_SENSE_BUFFER
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
#endif


    fbe_payload_cdb_set_transfer_count(cdb_operation, (fbe_u32_t)(block_count * physical_block_size));    
  
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    cdb_operation->cdb[0] = FBE_SCSI_WRITE_10;
    cdb_operation->cdb[1] = 0;
    if (fbe_payload_block_is_flag_set(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCE_UNIT_ACCESS))
    {
        cdb_operation->cdb[1] |= FBE_SCSI_FUA;
    }
    
    status = sas_physical_drive_cdb_build_10_byte(cdb_operation, timeout, lba, block_count);

    /*Enhanced Queuing*/
    if(fbe_sas_physical_drive_is_enhanced_queuing_supported(sas_physical_drive)
        && cdb_operation->payload_cdb_task_attribute == FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE
        && cdb_operation->payload_cdb_priority > FBE_PAYLOAD_CDB_PRIORITY_NORMAL)
    {
        /*Set low priority*/
        cdb_operation->cdb[1] |= FBE_SCSI_LOW_PRIORITY;
    }

    if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
    {
        fbe_u16_t extra = fbe_traffic_trace_get_priority_from_cdb(cdb_operation->payload_cdb_priority);
        fbe_sas_physical_drive_trace_rba(sas_physical_drive, cdb_operation, KT_TRAFFIC_WRITE_START | extra);
    }
    
    return status;
}


/* Function to convert a block write operation to a write_16 */
static __forceinline fbe_status_t 
sas_physical_drive_convert_block_to_cdb_write_16(fbe_sas_physical_drive_t* sas_physical_drive,
                                                              fbe_payload_ex_t* payload,
                                                              fbe_payload_cdb_operation_t* cdb_operation,
                                                              fbe_time_t timeout)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_t* block_operation = NULL;
    fbe_block_count_t block_count= 0;
    fbe_block_size_t block_size = 0;
    fbe_lba_t lba = 0;
    fbe_block_size_t physical_block_size = 0;

    block_operation = fbe_payload_ex_get_block_operation(payload);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);  
    fbe_payload_block_get_lba(block_operation, &lba);

    physical_block_size = sas_physical_drive->base_physical_drive.block_size;
    /* If needed convert the lba and blocks based on physical block size. */
    if (physical_block_size != block_size)
    {
        status = sas_physical_drive_cdb_convert_to_physical_block_size(sas_physical_drive, block_size, physical_block_size, &lba, &block_count);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

#if ZERO_SENSE_BUFFER
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
#endif


    fbe_payload_cdb_set_transfer_count(cdb_operation, (fbe_u32_t)(block_count * physical_block_size));
    
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    cdb_operation->cdb[0] = FBE_SCSI_WRITE_16;
    cdb_operation->cdb[1] = 0;
    if (fbe_payload_block_is_flag_set(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCE_UNIT_ACCESS))
    {
        cdb_operation->cdb[1] |= FBE_SCSI_FUA;
    }
    
    status = sas_physical_drive_cdb_build_16_byte(cdb_operation, timeout, lba, block_count);

    /*Enhanced Queuing*/
    if(fbe_sas_physical_drive_is_enhanced_queuing_supported(sas_physical_drive)
        && cdb_operation->payload_cdb_task_attribute == FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE
        && cdb_operation->payload_cdb_priority > FBE_PAYLOAD_CDB_PRIORITY_NORMAL)
    {
        /*Set low priority*/
        cdb_operation->cdb[15] |= FBE_SCSI_LOW_PRIORITY;
    }

    if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
    {
        fbe_u16_t extra = fbe_traffic_trace_get_priority_from_cdb(cdb_operation->payload_cdb_priority);
        fbe_sas_physical_drive_trace_rba(sas_physical_drive, cdb_operation, KT_TRAFFIC_WRITE_START | extra);
    }
 
    return status;
}


/* Function to convert a block verify operation to a verify_10 */
static __forceinline fbe_status_t 
sas_physical_drive_convert_block_to_cdb_verify_10(fbe_sas_physical_drive_t* sas_physical_drive,
                                                               fbe_payload_ex_t* payload,
                                                               fbe_payload_cdb_operation_t* cdb_operation,
                                                               fbe_time_t timeout)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_t* block_operation = NULL;
    fbe_block_count_t block_count= 0;
    fbe_block_size_t block_size = 0;
    fbe_lba_t lba = 0;
    fbe_block_size_t physical_block_size;
    
    block_operation = fbe_payload_ex_get_block_operation(payload);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);    
    fbe_payload_block_get_lba(block_operation, &lba);
    
    physical_block_size = sas_physical_drive->base_physical_drive.block_size;
   /* If needed convert the lba and blocks based on physical block size. */
   if ( physical_block_size != block_size) {
       status = sas_physical_drive_cdb_convert_to_physical_block_size(sas_physical_drive, block_size, physical_block_size, &lba, &block_count);
       if (status != FBE_STATUS_OK)
       {
           return status;
       }
   }


#if ZERO_SENSE_BUFFER
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
#endif
  
    fbe_payload_cdb_set_transfer_count(cdb_operation, 0);    
  
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER;
    cdb_operation->cdb[0] = FBE_SCSI_VERIFY_10;
    cdb_operation->cdb[1] = 0;   

    status = sas_physical_drive_cdb_build_10_byte(cdb_operation, timeout, lba, block_count);

#if 0 // Too verbose for now.
    if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
    {
        // We map the verify command to the same bit as CRPT_CRC
        fbe_u16_t extra = fbe_traffic_trace_get_priority_from_cdb(cdb_operation->payload_cdb_priority);
        fbe_sas_physical_drive_trace_rba(sas_physical_drive_p, cdb_operation, KT_TRAFFIC_CRPT_CRC_START | extra);
    }
#endif
    return status;
}


/* Function to convert a block verify operation to a verify_16 */
static __forceinline fbe_status_t 
sas_physical_drive_convert_block_to_cdb_verify_16(fbe_sas_physical_drive_t* sas_physical_drive,
                                                               fbe_payload_ex_t* payload,
                                                               fbe_payload_cdb_operation_t* cdb_operation,
                                                               fbe_time_t timeout)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_t* block_operation = NULL;
    fbe_block_count_t block_count= 0;
    fbe_block_size_t block_size = 0;
    fbe_lba_t lba = 0;
    fbe_block_size_t physical_block_size;

    block_operation = fbe_payload_ex_get_block_operation(payload);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);  
    fbe_payload_block_get_lba(block_operation, &lba);
    
     physical_block_size = sas_physical_drive->base_physical_drive.block_size;
    /* If needed convert the lba and blocks based on physical block size. */
    if ( physical_block_size != block_size) {
        status = sas_physical_drive_cdb_convert_to_physical_block_size(sas_physical_drive, block_size, physical_block_size, &lba, &block_count);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

#if ZERO_SENSE_BUFFER
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
#endif
      
    fbe_payload_cdb_set_transfer_count(cdb_operation, 0);
    
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER;
    cdb_operation->cdb[0] = FBE_SCSI_VERIFY_16;
    cdb_operation->cdb[1] = 0;

    status = sas_physical_drive_cdb_build_16_byte(cdb_operation, timeout, lba, block_count);

#if 0 // Too verbose for now.    
    if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
    {
        // We map the verify command to the same bit as CRPT_CRC
        fbe_u16_t extra = fbe_traffic_trace_get_priority_from_cdb(cdb_operation->payload_cdb_priority);
        fbe_sas_physical_drive_trace_rba(sas_physical_drive_p, cdb_operation, KT_TRAFFIC_CRPT_CRC_START | extra);
    }
#endif
    return status;
}


/* Function to convert a block write verify operation to a write_verify_10 */
static __forceinline fbe_status_t 
sas_physical_drive_convert_block_to_cdb_write_verify_10(fbe_sas_physical_drive_t* sas_physical_drive,
                                                                     fbe_payload_ex_t* payload,
                                                                     fbe_payload_cdb_operation_t* cdb_operation,
                                                                     fbe_time_t timeout)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_t* block_operation = NULL;
    fbe_block_count_t block_count= 0;
    fbe_block_size_t block_size = 0;
    fbe_lba_t lba = 0;
    fbe_block_size_t physical_block_size = 0;

    block_operation = fbe_payload_ex_get_block_operation(payload);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);    
    fbe_payload_block_get_lba(block_operation, &lba);

    physical_block_size = sas_physical_drive->base_physical_drive.block_size;

    /* If needed convert the lba and blocks based on physical block size. */
    if (physical_block_size != block_size)
    {
        status = sas_physical_drive_cdb_convert_to_physical_block_size(sas_physical_drive, block_size, physical_block_size, &lba, &block_count);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

#if ZERO_SENSE_BUFFER
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
#endif

    fbe_payload_cdb_set_transfer_count(cdb_operation, (fbe_u32_t)(block_count * physical_block_size));  
  
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    cdb_operation->cdb[0] = FBE_SCSI_WRITE_VERIFY_10;
    cdb_operation->cdb[1] = 0;

    status = sas_physical_drive_cdb_build_10_byte(cdb_operation, timeout, lba, block_count);
    if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
    {
        fbe_u16_t extra = fbe_traffic_trace_get_priority_from_cdb(cdb_operation->payload_cdb_priority);
        fbe_sas_physical_drive_trace_rba(sas_physical_drive, cdb_operation, KT_TRAFFIC_WRITE_START | extra);
    }
    return status;
}


/* Function to convert a block write verify operation to a write_verify_16 */
static __forceinline fbe_status_t 
sas_physical_drive_convert_block_to_cdb_write_verify_16(fbe_sas_physical_drive_t* sas_physical_drive,
                                                                     fbe_payload_ex_t* payload,
                                                                     fbe_payload_cdb_operation_t* cdb_operation,
                                                                     fbe_time_t timeout)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_t* block_operation = NULL;
    fbe_block_count_t block_count= 0;
    fbe_block_size_t block_size = 0;
    fbe_lba_t lba = 0;
    fbe_block_size_t physical_block_size = 0;

    block_operation = fbe_payload_ex_get_block_operation(payload);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_payload_block_get_block_size(block_operation, &block_size);  
    fbe_payload_block_get_lba(block_operation, &lba);
    
    physical_block_size = sas_physical_drive->base_physical_drive.block_size;
    /* If needed convert the lba and blocks based on physical block size. */
    if ( physical_block_size != block_size) {
        status = sas_physical_drive_cdb_convert_to_physical_block_size(sas_physical_drive, block_size, physical_block_size, &lba, &block_count);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

#if ZERO_SENSE_BUFFER
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
#endif

    fbe_payload_cdb_set_transfer_count(cdb_operation, (fbe_u32_t)(block_count * physical_block_size));
    
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    cdb_operation->cdb[0] = FBE_SCSI_WRITE_VERIFY_16;
    cdb_operation->cdb[1] = 0;
    
    status = sas_physical_drive_cdb_build_16_byte(cdb_operation, timeout, lba, block_count);
    
    if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
    {
        fbe_u16_t extra = fbe_traffic_trace_get_priority_from_cdb(cdb_operation->payload_cdb_priority);
        fbe_sas_physical_drive_trace_rba(sas_physical_drive, cdb_operation, KT_TRAFFIC_WRITE_START | extra);
    }
    return status;
}


/* Function to convert a block write same operation to a write_same_10 */
static __forceinline fbe_status_t 
sas_physical_drive_handle_write_same_10(fbe_sas_physical_drive_t* sas_physical_drive_p,
                                        fbe_payload_ex_t* payload,
                                        fbe_payload_cdb_operation_t* cdb_operation,
                                        fbe_time_t timeout,
                                        fbe_bool_t unmap)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u64_t total_bytes = 0;
    fbe_u32_t repeat_count = 0;
    fbe_u32_t sg_byte_count = 0;
    fbe_u32_t new_repeat_count = 0;
    fbe_u32_t optimum_blocks = 0;
    fbe_block_size_t physical_block_size = 0;
    
    fbe_block_size_t optimum_block_size = 0;
    fbe_block_size_t block_size = 0;
    fbe_block_count_t block_count = 0;
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload);

    fbe_payload_sg_descriptor_t *payload_sg_desc_p = NULL;
    fbe_payload_sg_descriptor_t *cdb_sg_desc_p = NULL;
    fbe_sg_element_t *sg_p = NULL;
    fbe_lba_t lba = 0;
 
    fbe_payload_block_get_sg_descriptor(block_operation_p, &payload_sg_desc_p);
    fbe_payload_sg_descriptor_get_repeat_count(payload_sg_desc_p, &repeat_count);
    fbe_payload_cdb_get_sg_descriptor(cdb_operation, &cdb_sg_desc_p);

    fbe_payload_block_get_optimum_block_size(block_operation_p, &optimum_block_size);
    fbe_payload_block_get_block_size(block_operation_p, &block_size);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    if (block_count > 0xFFFF) {
        return sas_physical_drive_handle_write_same_16(sas_physical_drive_p, payload, cdb_operation, 30000, unmap);
    }
    fbe_payload_ex_get_sg_list(payload, &sg_p, NULL);
    fbe_payload_block_get_lba(block_operation_p, &lba);
    
     physical_block_size = sas_physical_drive_p->base_physical_drive.block_size;
    /* If needed convert the lba and blocks based on physical block size. */
    if ( physical_block_size != block_size) {
        status = sas_physical_drive_cdb_convert_to_physical_block_size(sas_physical_drive_p, block_size, physical_block_size, &lba, &block_count);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

#if ZERO_SENSE_BUFFER
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
#endif
    
    fbe_payload_cdb_set_transfer_count(cdb_operation, physical_block_size); 
    
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    cdb_operation->cdb[0] = FBE_SCSI_WRITE_SAME_10;
    if (unmap) {
        cdb_operation->cdb[1] = 0x08;

        if ((sas_physical_drive_p->base_physical_drive.drive_info.drive_parameter_flags & FBE_PDO_FLAG_SUPPORTS_WS_UNMAP) == 0)
        {
            fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, write same: unmap not supported\n",  __FUNCTION__);
        }
    } else {
        cdb_operation->cdb[1] = 0;
    }

    status = sas_physical_drive_cdb_build_10_byte(cdb_operation, timeout, lba, block_count);

    /* Peter Puhov HACK */
    optimum_block_size = 1;
    /* End of HACK */
    if (optimum_block_size == 1)
    {
        /* We are done translating this write same into a cdb.
         */
        if (repeat_count > 1)
        {
            fbe_payload_sg_descriptor_set_repeat_count(cdb_sg_desc_p, repeat_count);
        }

        fbe_payload_cdb_set_transfer_count(cdb_operation, physical_block_size);

        if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
        {
            fbe_u16_t extra = fbe_traffic_trace_get_priority_from_cdb(cdb_operation->payload_cdb_priority);
            fbe_sas_physical_drive_trace_rba(sas_physical_drive_p, cdb_operation, KT_TRAFFIC_ZERO_START | extra);
        }
        return FBE_STATUS_OK;
    }

    if (optimum_block_size == 0)
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, write same: optimum_block_size is 0\n", 
                                 __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the optimum block size is not 1, then we need to adjust the sg
     * descriptor repeat count and turn this into a write.
     */
    cdb_operation->cdb[0] = FBE_SCSI_WRITE_10;
    cdb_operation->cdb[1] = 0;

    /* In this case we have an entire sg list.
     */
    sg_byte_count = fbe_sg_element_count_list_bytes(sg_p);

    total_bytes = sg_byte_count * repeat_count;

    /* If the total data represented by the sg is not a multiple of the optimum 
     * block size, something is wrong. 
     */
    if (total_bytes != (optimum_block_size * physical_block_size))
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, write same, sg bytes not optimum block size 0x%llx 0x%x\n", 
                                 __FUNCTION__, (unsigned long long)total_bytes,
				 optimum_block_size);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Divide the block count in the transfer by the optimum block size to determine 
     * how many optimum blocks are in the transfer. 
     */
    if ((block_count / optimum_block_size) >FBE_U32_MAX)
    {
       /* we do not expect the optimum blocks to go beyond 32bit
         */
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, Invalid optimum blocks 0x%llx \n", 
                                 __FUNCTION__, (unsigned long long)(block_count / optimum_block_size));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    optimum_blocks = (fbe_u32_t)(block_count / optimum_block_size);

    /* Since the data in the sg represents one optimum block, just 
     * scale it up to represent the entire transfer by multiplying by the number 
     * of optimum blocks in the transfer. 
     */
    new_repeat_count = repeat_count * optimum_blocks;
    fbe_payload_sg_descriptor_set_repeat_count(cdb_sg_desc_p, new_repeat_count);
    return FBE_STATUS_OK;
}


/* Function to convert a block write same operation to a write_same_16 */
static __forceinline fbe_status_t 
sas_physical_drive_handle_write_same_16(fbe_sas_physical_drive_t* sas_physical_drive_p,
                                        fbe_payload_ex_t* payload,
                                        fbe_payload_cdb_operation_t* cdb_operation,
                                        fbe_time_t timeout,
                                        fbe_bool_t unmap)
{
    fbe_status_t status = 0;
    fbe_u64_t total_bytes = 0;
    fbe_u32_t repeat_count = 0;
    fbe_u32_t sg_byte_count = 0;
    fbe_u32_t new_repeat_count = 0;
    fbe_u32_t optimum_blocks = 0;
    fbe_block_size_t physical_block_size = 0;

    fbe_block_size_t optimum_block_size = 0;
    fbe_block_size_t block_size = 0;
    fbe_block_count_t block_count = 0;
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload);

    fbe_payload_sg_descriptor_t *payload_sg_desc_p = NULL;
    fbe_payload_sg_descriptor_t *cdb_sg_desc_p = NULL;
    fbe_sg_element_t *sg_p = NULL;
    fbe_lba_t lba = 0;
 
    fbe_payload_block_get_sg_descriptor(block_operation_p, &payload_sg_desc_p);
    fbe_payload_sg_descriptor_get_repeat_count(payload_sg_desc_p, &repeat_count);
    fbe_payload_cdb_get_sg_descriptor(cdb_operation, &cdb_sg_desc_p);

    fbe_payload_block_get_optimum_block_size(block_operation_p, &optimum_block_size);
    fbe_payload_block_get_block_size(block_operation_p, &block_size);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_ex_get_sg_list(payload, &sg_p, NULL);
    fbe_payload_block_get_lba(block_operation_p, &lba);
    
     physical_block_size = sas_physical_drive_p->base_physical_drive.block_size;
    /* If needed convert the lba and blocks based on physical block size. */
    if ( physical_block_size != block_size) {
        status = sas_physical_drive_cdb_convert_to_physical_block_size(sas_physical_drive_p, block_size, physical_block_size, &lba, &block_count);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

#if ZERO_SENSE_BUFFER
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
#endif

    fbe_payload_cdb_set_transfer_count(cdb_operation, physical_block_size); 
 
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    cdb_operation->cdb[0] = FBE_SCSI_WRITE_SAME_16;
    if (unmap) {
        cdb_operation->cdb[1] = 0x08;

        if ((sas_physical_drive_p->base_physical_drive.drive_info.drive_parameter_flags & FBE_PDO_FLAG_SUPPORTS_WS_UNMAP) == 0)
        {
            fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, write same: unmap not supported\n",  __FUNCTION__);
        }
    } else {
        cdb_operation->cdb[1] = 0;
    }
    
    status = sas_physical_drive_cdb_build_16_byte(cdb_operation, timeout, lba, block_count);
    
    /* Peter Puhov HACK */
    optimum_block_size = 1;
    /* End of HACK */
    if (optimum_block_size == 1)
    {
        /* We are done translating this write same into a cdb.
         */
        if (repeat_count > 1)
        {
            fbe_payload_sg_descriptor_set_repeat_count(cdb_sg_desc_p, repeat_count);
        }

        fbe_payload_cdb_set_transfer_count(cdb_operation, physical_block_size);

        if (fbe_traffic_trace_is_enabled (KTRC_TPDO))
        {
            fbe_u16_t extra = fbe_traffic_trace_get_priority_from_cdb(cdb_operation->payload_cdb_priority);
            fbe_sas_physical_drive_trace_rba(sas_physical_drive_p, cdb_operation, KT_TRAFFIC_ZERO_START | extra);
        }
        return FBE_STATUS_OK;
    }

    if (optimum_block_size == 0)
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, write same: optimum_block_size is 0\n", 
                                 __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the optimum block size is not 1, then we need to adjust the sg
     * descriptor repeat count and turn this into a write.
     */
    cdb_operation->cdb[0] = FBE_SCSI_WRITE_16;
    cdb_operation->cdb[1] = 0;

    /* In this case we have an entire sg list.
     */
    sg_byte_count = fbe_sg_element_count_list_bytes(sg_p);

    total_bytes = sg_byte_count * repeat_count;

    /* If the total data represented by the sg is not a multiple of the optimum 
     * block size, something is wrong. 
     */
    if (total_bytes != (optimum_block_size * physical_block_size))
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sas_physical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, write same, sg bytes not optimum block size 0x%llx 0x%x\n", 
                                 __FUNCTION__, (unsigned long long)total_bytes,
				 optimum_block_size);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Divide the block count in the transfer by the optimum block size to determine 
     * how many optimum blocks are in the transfer. 
     */
    if ((block_count / optimum_block_size) >FBE_U32_MAX)
    {
       /* we do not expect the optimum blocks to go beyond 32bit
         */
        fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, Invalid optimum blocks 0x%llx \n", 
                                 __FUNCTION__, (unsigned long long)(block_count / optimum_block_size));
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    optimum_blocks = (fbe_u32_t)(block_count / optimum_block_size);

    /* Since the data in the sg represents one optimum block, just 
     * scale it up to represent the entire transfer by multiplying by the number 
     * of optimum blocks in the transfer. 
     */
    new_repeat_count = repeat_count * optimum_blocks;
    fbe_payload_sg_descriptor_set_repeat_count(cdb_sg_desc_p, new_repeat_count);
    return FBE_STATUS_OK;
}


fbe_status_t sas_physical_drive_convert_block_to_cdb_return_error(fbe_sas_physical_drive_t* sas_physical_drive,
                                                                  fbe_payload_ex_t* payload,
                                                                  fbe_payload_cdb_operation_t* cdb_operation,
                                                                  fbe_time_t timeout)
{
    return FBE_STATUS_GENERIC_FAILURE;
}                                                                  

fbe_status_t
fbe_payload_cdb_build_disk_collect_write(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, 
                                         fbe_block_size_t block_size, fbe_lba_t lba, fbe_block_count_t block_count)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    if((block_count * block_size) > FBE_U32_MAX)
    {
        /* unexpected transfer count
          */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;

    fbe_payload_cdb_set_transfer_count(cdb_operation, 1 * block_size);

    cdb_operation->cdb[0] = lba > FBE_U32_MAX ? FBE_SCSI_WRITE_16 : FBE_SCSI_WRITE_10;
    cdb_operation->cdb[1] = 0;

    /* Generate the CDB.  We will fill in a 10 byte or a 16 byte CDB, depending lba.
     */
    if (lba > FBE_U32_MAX)
    {
        /* 16 byte CDB */
        status = sas_physical_drive_cdb_build_16_byte(cdb_operation, timeout, lba, block_count);
    }
    else 
    {
        /* 10 byte CDB */
       status = sas_physical_drive_cdb_build_10_byte(cdb_operation, timeout, lba, block_count);
    }
    
    return status;
}

/* "F7 04 xx xx xx xx xx 00 00 00" command manually trigger the drive to dump internal log to disk. */
fbe_status_t 
sas_physical_drive_cdb_build_dump_disk_log(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER;
    
    cdb_operation->timeout = timeout;
    
    /* constract cdb */
    cdb_operation->cdb_length = 10;
    /* OPERATION CODE (00h)*/
    cdb_operation->cdb[0] = 0xF7;
    cdb_operation->cdb[1] = 0x04;
    
    /* Seagate require to set the bit 6 to 0xFE. */
    cdb_operation->cdb[6] = 0xFE;
    cdb_operation->cdb[7] = 0x00;
    cdb_operation->cdb[8] = 0x00;
    cdb_operation->cdb[9] = 0x00;
    
    return FBE_STATUS_OK;
}

fbe_status_t 
sas_physical_drive_cdb_build_read_disk_blocks(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, 
                                          fbe_lba_t lba )
{
    fbe_status_t status = FBE_STATUS_OK;
        
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
    
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;
    cdb_operation->cdb[0] = FBE_SCSI_READ_10;
    cdb_operation->cdb[1] = 0;
    
    status = sas_physical_drive_cdb_build_10_byte(cdb_operation, timeout, lba, 1);
    
    return status;
}

/*!**************************************************************
 * @fn sas_physical_drive_cdb_build_read_long_16(fbe_payload_cdb_operation_t * cdb_operation,
 *                                               fbe_time_t timeout,
                                                 fbe_lba_t lba,
                                                 fbe_u32_t long_block_size)
 ****************************************************************
 * @brief
 *  This function build the read long command in format of 16 byte.
 *
 * @param cdb_operation - Pointer to the CDB 
 * @param timeout - Timer.
 * @param lba - Lba to be read.
 * @param long_block_size - Size of the block
 *
 * @return status - success of failure.
 *
 * HISTORY:
 **08/21/2013: Dipak Patel - Ported from Joe B.'s code on R31 code base
 ****************************************************************/
fbe_status_t 
sas_physical_drive_cdb_build_read_long_16(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_lba_t lba, fbe_u32_t long_block_size )
{
    fbe_status_t status = FBE_STATUS_OK;
    
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
    
    /* The read long command can only transfer one long block of data */
    fbe_payload_cdb_set_transfer_count(cdb_operation, long_block_size);    
    
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;
           
    /* Set common parameters */    
    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->timeout = timeout;    
    cdb_operation->payload_sg_descriptor.repeat_count = 1;
    cdb_operation->cdb_length = 16;  
    
    /* Set the command */
    cdb_operation->cdb[0] = FBE_SCSI_READ_LONG_16;  
        
    /* For 16 byte Read Long need to set the Service Action to 0x11 */  
    cdb_operation->cdb[1] = 0x11;
    
    /* Set the lba */   
    cdb_operation->cdb[2] = (fbe_u8_t)((lba >> 56) & 0xFF); /* MSB */
    cdb_operation->cdb[3] |= (fbe_u8_t)((lba >> 48) & 0xFF); 
    cdb_operation->cdb[4] |= (fbe_u8_t)((lba >> 40) & 0xFF);
    cdb_operation->cdb[5] |= (fbe_u8_t)((lba >> 32) & 0xFF);    
    cdb_operation->cdb[6] |= (fbe_u8_t)((lba >> 24) & 0xFF);
    cdb_operation->cdb[7] |= (fbe_u8_t)((lba >> 16) & 0xFF);
    cdb_operation->cdb[8] |= (fbe_u8_t)((lba >> 8) & 0xFF); 
    cdb_operation->cdb[9] |= (fbe_u8_t)(lba & 0xFF); /* LSB */
    
    /* Set the byte Transfer Length */
    cdb_operation->cdb[12] = (fbe_u8_t)((long_block_size >> 8) & 0xFF); /* MSB */
    cdb_operation->cdb[13] |= (fbe_u8_t)(long_block_size & 0xFF); /* LSB */  
   
    return status;  
}

/*!**************************************************************
 * @fn sas_physical_drive_cdb_build_read_long_10(fbe_payload_cdb_operation_t * cdb_operation,
 *                                               fbe_time_t timeout,
                                                 fbe_lba_t lba,
                                                 fbe_u32_t long_block_size)
 ****************************************************************
 * @brief
 *  This function build the read long command in format of 10 byte.
 *
 * @param cdb_operation - Pointer to the CDB 
 * @param timeout - Timer.
 * @param lba - Lba to be read.
 * @param long_block_size - Size of the block
 *
 * @return status - success of failure.
 *
 * HISTORY:
 **08/21/2013: Dipak Patel - Ported from Joe B.'s code on R31 code base
 ****************************************************************/
fbe_status_t 
    sas_physical_drive_cdb_build_read_long_10(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_lba_t lba, fbe_u32_t long_block_size )
{
    fbe_status_t status = FBE_STATUS_OK;
    
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
    
    /* The read long command can only transfer one long block of data */
    fbe_payload_cdb_set_transfer_count(cdb_operation, long_block_size);    
    
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;
    
    /* Set common parameters */    
    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->timeout = timeout;    
    cdb_operation->payload_sg_descriptor.repeat_count = 1;
    cdb_operation->cdb_length = 10;  
    
    /* Set the command */
    cdb_operation->cdb[0] = FBE_SCSI_READ_LONG_10;  
    
    /* Set the lba */   
    cdb_operation->cdb[2] = (fbe_u8_t)((lba >> 24) & 0xFF); /* MSB */
    cdb_operation->cdb[3] |= (fbe_u8_t)((lba >> 16) & 0xFF); 
    cdb_operation->cdb[4] |= (fbe_u8_t)((lba >> 8) & 0xFF);
    cdb_operation->cdb[5] |= (fbe_u8_t)(lba & 0xFF); /* LSB */
    
    /* Set the byte Transfer Length */
    cdb_operation->cdb[7] = (fbe_u8_t)((long_block_size >> 8) & 0xFF); /* MSB */
    cdb_operation->cdb[8] |= (fbe_u8_t)(long_block_size & 0xFF); /* LSB */  
    
    return status;  
}

/*!**************************************************************
 * @fn sas_physical_drive_cdb_build_write_long_16(fbe_payload_cdb_operation_t * cdb_operation,
 *                                                fbe_time_t timeout,
                                                  fbe_lba_t lba,
                                                  fbe_u32_t long_block_size)
 ****************************************************************
 * @brief
 *  This function build the write long command in format of 16 byte.
 *
 * @param cdb_operation - Pointer to the CDB 
 * @param timeout - Timer.
 * @param lba - Lba to be read.
 * @param long_block_size - Size of the block
 *
 * @return status - success of failure.
 *
 * HISTORY:
 **08/21/2013: Dipak Patel - Ported from Joe B.'s code on R31 code base
 ****************************************************************/
fbe_status_t
    sas_physical_drive_cdb_build_write_long_16(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_lba_t lba, fbe_u32_t long_block_size )
{
    fbe_status_t status = FBE_STATUS_OK;
    
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
    
    /* The write long command can only transfer one long block of data */
    fbe_payload_cdb_set_transfer_count(cdb_operation, long_block_size);    
    
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    
    /* Set common parameters */    
    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->timeout = timeout;    
    cdb_operation->payload_sg_descriptor.repeat_count = 1;
    cdb_operation->cdb_length = 16;  
    
    /* Set the command */
    cdb_operation->cdb[0] = FBE_SCSI_WRITE_LONG_16; 

    /* For 16 byte Write Long need to set the Service Action to 0x11 */ 
    cdb_operation->cdb[1] = 0x11;   
    
    /* Set the lba */   
    cdb_operation->cdb[2] = (fbe_u8_t)((lba >> 56) & 0xFF); /* MSB */
    cdb_operation->cdb[3] |= (fbe_u8_t)((lba >> 48) & 0xFF); 
    cdb_operation->cdb[4] |= (fbe_u8_t)((lba >> 40) & 0xFF);
    cdb_operation->cdb[5] |= (fbe_u8_t)((lba >> 32) & 0xFF);    
    cdb_operation->cdb[6] |= (fbe_u8_t)((lba >> 24) & 0xFF);
    cdb_operation->cdb[7] |= (fbe_u8_t)((lba >> 16) & 0xFF);
    cdb_operation->cdb[8] |= (fbe_u8_t)((lba >> 8) & 0xFF); 
    cdb_operation->cdb[9] |= (fbe_u8_t)(lba & 0xFF); /* LSB */
    
    /* Set the byte Transfer Length */
    cdb_operation->cdb[12] = (fbe_u8_t)((long_block_size >> 8) & 0xFF); /* MSB */
    cdb_operation->cdb[13] |= (fbe_u8_t)(long_block_size & 0xFF); /* LSB */  
    
    return status;  
}

/*!**************************************************************
 * @fn sas_physical_drive_cdb_build_write_long_10(fbe_payload_cdb_operation_t * cdb_operation,
 *                                               fbe_time_t timeout,
                                                 fbe_lba_t lba,
                                                 fbe_u32_t long_block_size)
 ****************************************************************
 * @brief
 *  This function build the write long command in format of 10 byte.
 *
 * @param cdb_operation - Pointer to the CDB 
 * @param timeout - Timer.
 * @param lba - Lba to be read.
 * @param long_block_size - Size of the block
 *
 * @return status - success of failure.
 *
 * HISTORY:
 **08/21/2013: Dipak Patel - Ported from Joe B.'s code on R31 code base
 ****************************************************************/
fbe_status_t
sas_physical_drive_cdb_build_write_long_10(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_lba_t lba, fbe_u32_t long_block_size )
{
    fbe_status_t status = FBE_STATUS_OK;
    
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
    
    /* The write long command can only transfer one long block of data */
    fbe_payload_cdb_set_transfer_count(cdb_operation, long_block_size);    
    
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    
    /* Set common parameters */    
    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->timeout = timeout;    
    cdb_operation->payload_sg_descriptor.repeat_count = 1;
    cdb_operation->cdb_length = 10;  
    
    /* Set the command */
    cdb_operation->cdb[0] = FBE_SCSI_WRITE_LONG_10; 
    
    /* Set the lba */   
    cdb_operation->cdb[2] = (fbe_u8_t)((lba >> 24) & 0xFF); /* MSB */
    cdb_operation->cdb[3] |= (fbe_u8_t)((lba >> 16) & 0xFF); 
    cdb_operation->cdb[4] |= (fbe_u8_t)((lba >> 8) & 0xFF);
    cdb_operation->cdb[5] |= (fbe_u8_t)(lba & 0xFF); /* LSB */
    
    /* Set the byte Transfer Length */
    cdb_operation->cdb[7] = (fbe_u8_t)((long_block_size >> 8) & 0xFF); /* MSB */
    cdb_operation->cdb[8] |= (fbe_u8_t)(long_block_size & 0xFF); /* LSB */  
    
    return status;  
}


fbe_status_t sas_physical_drive_cdb_build_sanitize(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u8_t * sanitize_header, fbe_scsi_sanitize_pattern_t pattern)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(sanitize_header, FBE_SCSI_FORMAT_UNIT_SANITIZE_HEADER_LENGTH);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;

    cdb_operation->timeout = timeout;

    /* construct cdb */
    cdb_operation->cdb_length = 6;

    cdb_operation->cdb[0] = FBE_SCSI_FORMAT_UNIT;
    cdb_operation->cdb[1] = 0x10;

    /* construct header */
    sanitize_header[1] = 0x9A;
    sanitize_header[4] = 0x20;
    sanitize_header[5] = pattern;

    return FBE_STATUS_OK;
}

fbe_status_t sas_physical_drive_cdb_build_format(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u8_t * parameter_list, fbe_u32_t parameter_list_size)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(parameter_list, parameter_list_size);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;

    cdb_operation->timeout = timeout;

    /* construct cdb */
    cdb_operation->cdb_length = 6;

    cdb_operation->cdb[0] = FBE_SCSI_FORMAT_UNIT;
    cdb_operation->cdb[1] = 0x10;   /* use parameter list */

    /* construct header */
    parameter_list[1] = 0x02;  /* immediate response */

    return FBE_STATUS_OK;
}


fbe_status_t sas_physical_drive_block_to_cdb(fbe_sas_physical_drive_t * sas_physical_drive, 
											 fbe_payload_ex_t* payload, 
											 fbe_payload_cdb_operation_t * cdb_operation)
{
    fbe_payload_block_operation_t * block_operation;
	fbe_payload_block_operation_opcode_t block_operation_opcode;
	fbe_lba_t lba;
	fbe_status_t status;
	fbe_time_t  timeout;

    block_operation = fbe_payload_ex_get_block_operation(payload);

	fbe_payload_block_get_lba(block_operation, &lba);
    fbe_payload_block_get_opcode(block_operation, &block_operation_opcode);    
    
    fbe_base_physical_drive_get_default_timeout((fbe_base_physical_drive_t *) sas_physical_drive, &timeout);

    if (lba  < 0x00000000FFFFFFFF) { /* Drive does NOT use 16 byte CDB's */         
		switch(block_operation_opcode){
		case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
			status = sas_physical_drive_convert_block_to_cdb_read_10(sas_physical_drive, payload, cdb_operation, timeout);
			break;
		case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
			status = sas_physical_drive_convert_block_to_cdb_write_10(sas_physical_drive, payload, cdb_operation, timeout);
			break;
		case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
			status = sas_physical_drive_convert_block_to_cdb_verify_10(sas_physical_drive, payload, cdb_operation, timeout);
			break;
		case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
			status = sas_physical_drive_convert_block_to_cdb_write_verify_10(sas_physical_drive, payload, cdb_operation, timeout);
			break;
		case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
			status = sas_physical_drive_handle_write_same_10(sas_physical_drive, payload, cdb_operation, timeout,
                     fbe_payload_block_is_flag_set(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_UNMAP));
			break;
#if 0
		case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMAP:
			status = sas_physical_drive_handle_write_same_10(sas_physical_drive, payload, cdb_operation, timeout, FBE_TRUE);
			break;
#endif
		default:
			status = FBE_STATUS_GENERIC_FAILURE;
			fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
								FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s, Invalid block_operation_opcode %d\n", __FUNCTION__, block_operation_opcode);

		}
		return status;
    }

	switch(block_operation_opcode){
	case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
		status = sas_physical_drive_convert_block_to_cdb_read_16(sas_physical_drive, payload, cdb_operation, timeout);
		break;
	case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
		status = sas_physical_drive_convert_block_to_cdb_write_16(sas_physical_drive, payload, cdb_operation, timeout);
		break;
	case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
		status = sas_physical_drive_convert_block_to_cdb_verify_16(sas_physical_drive, payload, cdb_operation, timeout);
		break;
	case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
		status = sas_physical_drive_convert_block_to_cdb_write_verify_16(sas_physical_drive, payload, cdb_operation, timeout);
		break;
	case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
		status = sas_physical_drive_handle_write_same_16(sas_physical_drive, payload, cdb_operation, timeout, 
                 fbe_payload_block_is_flag_set(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_UNMAP));
		break;
#if 0
	case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMAP:
		status = sas_physical_drive_handle_write_same_16(sas_physical_drive, payload, cdb_operation, timeout, FBE_TRUE);
		break;
#endif
	default:
		status = FBE_STATUS_GENERIC_FAILURE;
		fbe_base_object_trace((fbe_base_object_t*)sas_physical_drive,
							FBE_TRACE_LEVEL_CRITICAL_ERROR,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s, Invalid block_operation_opcode %d\n", __FUNCTION__, block_operation_opcode);

	}
	return status;
}
