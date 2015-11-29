#ifndef FBE_RAID_RAW_MIRROR_UTILS_H
#define FBE_RAID_RAW_MIRROR_UTILS_H

 /* All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_raw_mirror_utils.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the raw mirror utils.
 *
 * @version
 *   03/26/2012:  Created. 
 *
 ***************************************************************************/
#define RAW_MIRROR_BLOCK_SIZE  520
#define RAW_MIRROR_BLOCK_DATA_SIZE  FBE_RAW_MIRROR_DATA_SIZE
typedef enum{
    FBE_RAW_MIRROR_OP_STATE_INVALID = 0,
    FBE_RAW_MIRROR_OP_STATE_READ,          /*Read operation*/
    FBE_RAW_MIRROR_OP_STATE_READ_RETRY,         /*Retry read oparetion*/
    FBE_RAW_MIRROR_OP_STATE_WRITE_VERIFY,  /*Read/Write Flush oparetion*/
    FBE_RAW_MIRROR_OP_STATE_WRITE,         /*Write oparetion*/
    FBE_RAW_MIRROR_OP_STATE_LAST
} fbe_raw_mirror_op_state_t;
typedef enum{
    FBE_RAW_MIRROR_DATABASE = 0,          
    FBE_RAW_MIRROR_METADATA,         
    FBE_RAW_MIRROR_INDEX_LAST
} fbe_raw_mirror_index_t;

/*!*******************************************************************
 * @struct raw_mirror_operation_cb_t
 *********************************************************************
 * @brief use for read/write interface as the arguments 
 *
 *********************************************************************/
typedef struct raw_mirror_operation_cb_s {
    /*The raw-3way-mirror LBA and count need read/write*/
    fbe_lba_t           lba; 
    fbe_block_count_t   block_count;  

    /*the sg list IO is for*/
    fbe_sg_element_t    *sg_list;
    fbe_u32_t           sg_list_count;

    /*error verify report structure passed to raid library to report errors*/
    fbe_raid_verify_raw_mirror_errors_t error_verify_report;
	
    /*the flage indicate whether we need retry the read when one read failed or not*/
    fbe_bool_t           is_read_retry;
	fbe_raw_mirror_t*    raw_mirror_p;
	/*indicate which disk the caller want to read from*/
    fbe_u16_t           disk_bitmask;
} raw_mirror_operation_cb_t;
/*!*******************************************************************
 * @struct raw_mirror_operation_cb_t
 *********************************************************************
 * @brief use for read/write interface as the arguments 
 *
 *********************************************************************/
typedef struct raw_mirror_system_load_context_s {
    /*The raw-3way-mirror LBA and count need read/write*/
    fbe_lba_t           start_lba; 
    fbe_block_count_t   range;  
    /*indicate where we have finished read already*/
    fbe_lba_t           current_lba; 

	/*the sg list IO is for*/
    fbe_sg_element_t    *sg_list;
    fbe_u32_t           sg_list_count;
	
    /*set this flag be TRUE if we switch to single block read*/
	fbe_bool_t          is_single_block_mode_read;
    /*error verify report structure passed to raid library to report errors*/
    fbe_raid_verify_raw_mirror_errors_t error_verify_report;
	/*indicate which disk the caller want to read from*/
    fbe_u16_t           disk_bitmask;
	/*the pionter to raw_mirror_control_block */
	raw_mirror_operation_cb_t* raw_mirror_cb_p;

} raw_mirror_system_load_context_t;

fbe_status_t 
fbe_raw_mirror_utils_error_handling_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
fbe_status_t raw_mirror_retry_read_blocks(fbe_packet_t *packet, 
                                            raw_mirror_operation_cb_t *raw_mirr_op_cb);
void raw_mirror_init_operation_cb(raw_mirror_operation_cb_t *raw_mirror_op_cb, fbe_lba_t lba,
                                          fbe_block_count_t block_count, fbe_sg_element_t *sg_list,
                                          fbe_u32_t sg_list_count,
                                          fbe_raw_mirror_t* raw_mirror_p,
                                          fbe_u16_t disk_bitmask);

void raw_mirror_init_system_load_context(raw_mirror_system_load_context_t *raw_mirror_system_load_context_p, fbe_lba_t lba,
											        fbe_block_count_t block_count, fbe_sg_element_t *sg_list,
											        fbe_u32_t sg_list_count,
											        raw_mirror_operation_cb_t *raw_mirror_op_cb,
											        fbe_u16_t disk_bitmask);

fbe_status_t 
raw_mirror_set_error_retry(raw_mirror_operation_cb_t *rw_mirr_op_cb,
                                               fbe_bool_t retry_err);
fbe_status_t 
raw_mirror_check_error_counts(fbe_raid_verify_raw_mirror_errors_t *error_verify_report,
                                                         fbe_u64_t *c_errors,
                                                         fbe_u64_t *u_errors,
                                                         fbe_u64_t *shutdown_errors,
                                                         fbe_u64_t *retryable_errors,
                                                         fbe_u64_t *non_retryable_errors);

#endif /*FBE_RAID_RAW_MIRROR_UTILS_H*/
