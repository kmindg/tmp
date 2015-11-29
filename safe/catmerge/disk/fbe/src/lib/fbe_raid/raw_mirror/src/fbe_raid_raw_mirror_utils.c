/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_raw_mirror_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains code for the generic raw-mirror error handling logic
 ***************************************************/
#include "fbe_raid_library.h"
#include "fbe_raid_raw_mirror_utils.h"
extern fbe_raw_mirror_t fbe_database_system_db;
extern fbe_raw_mirror_t fbe_metadata_nonpaged_raw_mirror;
extern fbe_raw_mirror_t fbe_database_homewrecker_db;
fbe_raw_mirror_t* fbe_raw_mirror_table[]={&fbe_database_system_db,
	                                      &fbe_metadata_nonpaged_raw_mirror,
	                                      &fbe_database_homewrecker_db,
										  NULL };
/*!***************************************************************
 * @fn raw_mirror_init_system_load_context
 *****************************************************************
 * @brief
 *   interface to calculate the checksum, set magic number and sequence
 *   number for  raw-mirror blocks. 
 *
 * @param raw_mirror_system_load_context   - raw-mirror system load context to be initialized
 * @param lba  - the start lba of the raw-mirror IO
 * @param block_count   - the block count the raw-mirror IO invloves
 * @param sg_list - well formed scatter-gather list of the raw-mirror IO
 * @param sg_list_count - size of the scatter-gather list
 * @param raw_mirr_op_cb_p  - the pionter to raw-mirror io control block
 * @return no
 *
 * @version
 *    3/21/2012: zhangy - created
 *
 ****************************************************************/
void raw_mirror_init_system_load_context(raw_mirror_system_load_context_t *raw_mirror_system_load_context_p, fbe_lba_t lba,
                                                    fbe_block_count_t block_count, fbe_sg_element_t *sg_list,
                                                    fbe_u32_t sg_list_count,
                                                    raw_mirror_operation_cb_t* raw_mirror_cb_p,
                                                    fbe_u16_t disk_bitmask)
{
    if (raw_mirror_system_load_context_p != NULL) {
        fbe_zero_memory(raw_mirror_system_load_context_p, sizeof (raw_mirror_system_load_context_t));
        raw_mirror_system_load_context_p->start_lba = lba;
        raw_mirror_system_load_context_p->range = block_count;
        raw_mirror_system_load_context_p->sg_list = sg_list;
        raw_mirror_system_load_context_p->sg_list_count = sg_list_count;
        raw_mirror_system_load_context_p->is_single_block_mode_read = FBE_FALSE;
        raw_mirror_system_load_context_p->current_lba = 0;
        raw_mirror_system_load_context_p->disk_bitmask = disk_bitmask;
		raw_mirror_system_load_context_p->raw_mirror_cb_p = raw_mirror_cb_p;
		fbe_zero_memory(&raw_mirror_system_load_context_p->error_verify_report,sizeof (fbe_raid_verify_raw_mirror_errors_t));
    }

}

/*!***************************************************************
 * @fn raw_mirror_init_operation_cb
 *****************************************************************
 * @brief
 *   interface to calculate the checksum, set magic number and sequence
 *   number for  raw-mirror blocks. 
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
 *    3/21/2012: zhangy - created
 *
 ****************************************************************/
void raw_mirror_init_operation_cb(raw_mirror_operation_cb_t *raw_mirror_op_cb, fbe_lba_t lba,
                                          fbe_block_count_t block_count, fbe_sg_element_t *sg_list,
                                          fbe_u32_t sg_list_count,
                                          fbe_raw_mirror_t* raw_mirror_p,
                                          fbe_u16_t disk_bitmask)
{
    if (raw_mirror_op_cb != NULL) {
        fbe_zero_memory(raw_mirror_op_cb, sizeof (raw_mirror_operation_cb_t));
        raw_mirror_op_cb->lba = lba;
        raw_mirror_op_cb->block_count = block_count;
        raw_mirror_op_cb->sg_list = sg_list;
        raw_mirror_op_cb->sg_list_count = sg_list_count;
        raw_mirror_op_cb->is_read_retry = FBE_FALSE;
        raw_mirror_op_cb->raw_mirror_p = raw_mirror_p;
        fbe_zero_memory(&raw_mirror_op_cb->error_verify_report,sizeof (fbe_raid_verify_raw_mirror_errors_t));
        raw_mirror_op_cb->disk_bitmask = disk_bitmask;
	}

}
/*!***************************************************************
 * @fn raw_mirror_check_error_counts
 *****************************************************************
 * @brief
 *   sum the total errors of error verify report and return the num
 *   according to the error category. 
 *
 * @param error_verify_report   - error report structure returned by
 *                                 raw-mirror raid library
 * @param c_errors  - total correctable errors count
 * @param u_errors   - total un-correctable errors count
 * @param retryable_errors - total retryable errors count
 * @param non_retryable_errors - total non-retryable errors count
 * 
 * @return status  - indicating if the function success
 *
 * @version
 *    3/21/2012: zhangy - created
 *
 ****************************************************************/
fbe_status_t raw_mirror_check_error_counts(fbe_raid_verify_raw_mirror_errors_t *error_verify_report,
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

/*!***************************************************************
 * @fn raw_mirror_set_error_retry
 *****************************************************************
 * @brief
 *   interface to calculate the checksum, set magic number and sequence
 *   number for  raw-mirror blocks. 
 *
 * @param raw_mirr_op_cb   - raw-mirror io control block to be initialized
 * @param flush_err  - indicate if we check and flush error
 * @param flush_retry_count   - how many times we retry flush
 * 
 * @return no
 *
 * @version
 *    3/21/2012: zhangy - created
 *
 ****************************************************************/
fbe_status_t raw_mirror_set_error_retry(raw_mirror_operation_cb_t *rw_mirr_op_cb,
                                               fbe_bool_t retry_err)
{
    if (!rw_mirr_op_cb) 
        return FBE_STATUS_GENERIC_FAILURE;

    rw_mirr_op_cb->is_read_retry = retry_err;

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_raw_mirror_mask_down_disk_in_all()
 ****************************************************************
 * @brief
 *  mask the down disk  in all raw mirrors' down disk bitmask. 
 * 
 * @param  fbe_u32_t disk_index  - the down disk index
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_mask_down_disk_in_all(fbe_u32_t disk_index)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_u32_t    index = 0;
	fbe_raw_mirror_t* next_raw_mirror = NULL;
	next_raw_mirror = fbe_raw_mirror_table[index];
	while(next_raw_mirror != NULL)
	{
		status = fbe_raw_mirror_mask_down_disk(next_raw_mirror,disk_index);
		if(status == FBE_STATUS_OK)
		{
          index++;
		  next_raw_mirror = fbe_raw_mirror_table[index];
		}else{
		 fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: fail to mask the bit mask in raw mirror 0x%x\n", 
						__FUNCTION__, status);
		 break;
		}
	}
	return status;
}
/*!**************************************************************
 * fbe_raw_mirror_unmask_down_disk_in_all()
 ****************************************************************
 * @brief
 *  unmask the down disk  in all raw mirrors' down disk bitmask. 
 * 
 * @param  fbe_u32_t disk_index  - the down disk index
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_unmask_down_disk_in_all(fbe_u32_t disk_index)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_u32_t    index = 0;
	fbe_raw_mirror_t* next_raw_mirror = NULL;
	next_raw_mirror = fbe_raw_mirror_table[index];
	while(next_raw_mirror != NULL)
	{
		status = fbe_raw_mirror_unmask_down_disk(next_raw_mirror,disk_index);
		if(status == FBE_STATUS_OK)
		{
          index++;
		  next_raw_mirror = fbe_raw_mirror_table[index];
		}else{
		 fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: fail to unmask the bit mask in raw mirror 0x%x\n", 
						__FUNCTION__, status);
		 break;
		}
	}
	return status;
}
/*!**************************************************************
 * fbe_raw_mirror_unset_initialized_in_all()
 ****************************************************************
 * @brief
 *  unset the initialized  in all raw mirrors' b_initialized. 
 * 
 * @param  
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_unset_initialized_in_all(void)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_u32_t    index = 0;
	fbe_raw_mirror_t* next_raw_mirror = NULL;
	next_raw_mirror = fbe_raw_mirror_table[index];
	while(next_raw_mirror != NULL)
	{
		status = fbe_raw_mirror_unset_initialized(next_raw_mirror);
		if(status == FBE_STATUS_OK)
		{
          index++;
		  next_raw_mirror = fbe_raw_mirror_table[index];
		}else{
		 fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: fail to unset b_initialized in raw mirror 0x%x\n", 
						__FUNCTION__, status);
		 break;
		}
	}
	return status;
}
	
/*!**************************************************************
 * fbe_raw_mirror_set_quiesce_flags_in_all()
 ****************************************************************
 * @brief
 *  set the quiesce flags in all raw mirrors. 
 * 
 * @param  
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_set_quiesce_flags_in_all(void)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_u32_t    index = 0;
	fbe_raw_mirror_t* next_raw_mirror = NULL;
	next_raw_mirror = fbe_raw_mirror_table[index];
	while(next_raw_mirror != NULL)
	{
		status = fbe_raw_mirror_set_quiesce_flag(next_raw_mirror);
		if(status == FBE_STATUS_OK)
		{
          index++;
		  next_raw_mirror = fbe_raw_mirror_table[index];
		}else{
		 fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: fail to set quiesce flag in raw mirror 0x%x\n", 
						__FUNCTION__, status);
		 break;
		}
	}
	return status;
}
fbe_status_t fbe_raw_mirror_unset_quiesce_flags_in_all(void)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_u32_t    index = 0;
	fbe_raw_mirror_t* next_raw_mirror = NULL;
	next_raw_mirror = fbe_raw_mirror_table[index];
	while(next_raw_mirror != NULL)
	{
		status = fbe_raw_mirror_unset_quiesce_flag(next_raw_mirror);
		if(status == FBE_STATUS_OK)
		{
          index++;
		  next_raw_mirror = fbe_raw_mirror_table[index];
		}else{
		 fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: fail to unset quiesce flag in raw mirror 0x%x\n", 
						__FUNCTION__, status);
		 break;
		}
	}
	return status;
}
fbe_status_t fbe_raw_mirror_wait_all_io_quiesced(void)
{
	fbe_u32_t	 index = 0;
	fbe_raw_mirror_t* next_raw_mirror = NULL;
	next_raw_mirror = fbe_raw_mirror_table[index];
	while(next_raw_mirror != NULL)
	{
		fbe_semaphore_wait(&next_raw_mirror->quiesce_io_semaphore, NULL);
		index++;
		next_raw_mirror = fbe_raw_mirror_table[index];
		
	}
	return FBE_STATUS_OK;
}
#define FBE_RAW_MIRROR_TIMEOUT_MS 120000
fbe_status_t fbe_raw_mirror_wait_quiesced(fbe_raw_mirror_t *raw_mirror_p)
{
    fbe_status_t status;
    status = fbe_semaphore_wait_ms(&raw_mirror_p->quiesce_io_semaphore, FBE_RAW_MIRROR_TIMEOUT_MS);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed wait for quiesce Status: 0x%x\n", 
                               __FUNCTION__, status);
    }
    return status;
}

fbe_status_t fbe_raw_mirror_init_edges_in_all(void)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_u32_t    index = 0;
	fbe_raw_mirror_t* next_raw_mirror = NULL;
	next_raw_mirror = fbe_raw_mirror_table[index];
	while(next_raw_mirror != NULL)
	{
		status = fbe_raw_mirror_init_edges(next_raw_mirror);
		if(status == FBE_STATUS_OK)
		{
          index++;
		  next_raw_mirror = fbe_raw_mirror_table[index];
		}else{
		 fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: fail to init edges in raw mirror 0x%x\n", 
						__FUNCTION__, status);
		 break;
		}
	}
	return status;
}
