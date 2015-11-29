/*******************************************************************
 * Copyright (C) EMC Corporation 1999 - 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*!*****************************************************************
 *  @file fbe_logical_error_injection_record_validate.c
 *******************************************************************
 *
 * @brief
 *  This file contains the logical error injection validation code.
 *  This code was ported from rg_error_validate.c
 * 
 * @version
 *  1/6/2010 - Created from rderr. Rob Foley
 *
 *******************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_logical_error_injection_private.h"
#include "fbe_logical_error_injection_proto.h"
#include "fbe/fbe_xor_api.h"

/* The search for a match is broken into four steps, with each step 
 * having its own function.
 */
static fbe_bool_t 
fbe_logical_error_injection_rec_find_match_lba(fbe_raid_siots_t*    const siots_p,
                          fbe_logical_error_injection_record_t* const rec_p,
                          fbe_xor_error_region_t*       const region_p,
                          fbe_logical_error_injection_validate_parms_t*  const parms_p,
                          fbe_bool_t* const b_first_record_p,
                          fbe_bool_t b_update_region,
                          fbe_s32_t idx_region);
static fbe_bool_t 
fbe_logical_error_injection_rec_find_match_position(fbe_raid_siots_t*    const siots_p,
                               fbe_logical_error_injection_record_t* const rec_p,
                               fbe_xor_error_region_t* const region_p,
                               fbe_logical_error_injection_validate_parms_t*  const parms_p);
static fbe_bool_t 
fbe_logical_error_injection_rec_find_match_error_type(fbe_raid_siots_t*    const siots_p,
                                 fbe_logical_error_injection_record_t* const rec_p,
                                 fbe_xor_error_region_t* const region_p,
                                 fbe_logical_error_injection_validate_parms_t*  const parms_p);
static fbe_bool_t 
fbe_logical_error_injection_rec_find_match_correctable(fbe_raid_siots_t*    const siots_p,
                                  fbe_logical_error_injection_record_t* const rec_p,
                                  fbe_xor_error_region_t* const region_p,
                                  fbe_logical_error_injection_validate_parms_t*  const parms_p);

/* Local helper functions.
 */
static fbe_bool_t fbe_logical_error_injection_opt_size_not_one(fbe_raid_siots_t* const siots_p,
                                             fbe_u16_t bitmap_phys);
static fbe_bool_t fbe_logical_error_injection_record_contain_region(const fbe_lba_t lba_rec, 
                     const fbe_block_count_t cnt_rec, const fbe_lba_t lba_reg, const fbe_s32_t cnt_reg);
static fbe_bool_t fbe_logical_error_injection_record_head_of_region(const fbe_lba_t lba_rec, 
                                    const fbe_block_count_t cnt_rec,
                                    const fbe_lba_t lba_reg, const fbe_s32_t cnt_reg,
                                    const fbe_bool_t b_first_record,
                                    fbe_s32_t* const blks_overlap_p);
static fbe_status_t fbe_logical_error_injection_set_up_validate_parms(fbe_raid_siots_t*    const siots_p,
                                    fbe_lba_t stripe_start_lba,
                                    fbe_u32_t blocks,
                                    fbe_s32_t idx_record,
                                    fbe_s32_t idx_region,
                                    fbe_bool_t b_print,
                                    fbe_logical_error_injection_validate_parms_t* const parms_p);
static fbe_status_t fbe_logical_error_injection_evaluate_err_adj(fbe_raid_siots_t* const siots_p,
                                       fbe_lba_t stripe_start_lba,
                                       fbe_block_count_t blocks,
                                       const fbe_s32_t idx_record,
                                       const fbe_s32_t idx_region,
                                       const fbe_u16_t bitmap_parity_phys,
                                       const fbe_bool_t b_print,
                                       fbe_u16_t *err_adj_phys_p);
static fbe_status_t fbe_logical_error_injection_find_matching_lba_range(const fbe_s32_t idx_record, 
                                          fbe_s32_t* const idx_first_p, 
                                          fbe_s32_t* const idx_last_p);
static void fbe_logical_error_injection_handle_validation_failure( fbe_raid_siots_t *const siots_p );
static fbe_raid_state_status_t fbe_logical_error_injection_retry_finished(fbe_raid_siots_t * siots_p);
static fbe_status_t fbe_logical_error_injection_display_fruts_blocks(fbe_raid_fruts_t * const input_fruts_p, const fbe_lba_t lba);
static fbe_bool_t fbe_logical_error_injection_is_ts_err_type( const fbe_logical_error_injection_record_t* const record_p );
static fbe_bool_t fbe_logical_error_injection_is_stamp_err_type( const fbe_logical_error_injection_record_t* const rec_p );
static fbe_bool_t fbe_logical_error_injection_is_crc_err_type( const fbe_logical_error_injection_record_t* const rec_p );
static fbe_bool_t fbe_logical_error_injection_is_extern_hard_media_error_present( fbe_raid_siots_t * siots_p);
static fbe_bool_t fbe_logical_error_injection_extend_error_region_up(fbe_xor_error_region_t* error_regions_array,
                                                                     fbe_lba_t * lba_p,  
                                                                     fbe_u16_t * block_count_p,
                                                                     fbe_s32_t index);
static fbe_bool_t fbe_logical_error_injection_extend_error_region_down(fbe_xor_error_regions_t * error_regions_p,
                                                                       fbe_lba_t   start_lba, 
                                                                       fbe_u16_t * block_count_p,
                                                                       fbe_s32_t index);
static void fbe_logical_error_injection_extend_error_region(fbe_raid_siots_t* const siots_p,
                                                            fbe_lba_t * lba_p,  
                                                            fbe_u16_t * block_count_p,
                                                            fbe_s32_t   idx_region);
static fbe_bool_t fbe_logical_error_injection_rec_find_downstream_match(fbe_raid_siots_t*    const siots_p,
                                                                 fbe_logical_error_injection_record_t* const rec_p,
                                                                 fbe_xor_error_region_t*       const region_p,
                                                                 fbe_logical_error_injection_validate_parms_t*  const parms_p,
                                                                 fbe_bool_t* const b_first_record_p,
                                                                 fbe_bool_t b_update_region,
                                                                 fbe_s32_t idx_region);

/****************************************************************
 * fbe_logical_error_injection_rec_find_match()
 ****************************************************************
 * @brief
 *  This function finds the matching error records to the given error
 *  region.
 *
 * @param siots_p - pointer to a fbe_raid_siots_t struct.
 * @param idx_region - index to an error region to match.
 * @param b_print - whether partially matching record is printed.
 *
 * @return
 *  FBE_TRUE  - If    match error record(s) is found.
 *  FBE_FALSE - If no match error record(s) is found.
 *
 * @author
 *  05/03/06 - Created. SL
 *
 ****************************************************************/
fbe_bool_t fbe_logical_error_injection_rec_find_match(fbe_raid_siots_t *const siots_p,
                                                      fbe_s32_t idx_region,
                                                      fbe_bool_t b_print)
{
    fbe_xor_error_region_t* const region_p = 
        &siots_p->vcts_ptr->error_regions.regions_array[idx_region];
    fbe_xor_error_region_t reg_local; /* Local copy to keep track progress. */
    fbe_bool_t b_first_record = FBE_TRUE; /* Matching record is the first one. */
    fbe_bool_t is_found = FBE_FALSE;      /* By default, we assume no match. */
    fbe_s32_t idx;
    fbe_bool_t b_ignore_corrupt_crc_data_errs = FBE_FALSE;

    fbe_logical_error_injection_get_ignore_corrupt_crc_data_errs(&b_ignore_corrupt_crc_data_errs);
    /* The error region is used to keep track the validation progress. 
     * Make a local copy and modify the local copy only. 
     */
    memcpy(&reg_local, region_p, sizeof(fbe_xor_error_region_t));

    if ( reg_local.lba + reg_local.blocks - 1 < siots_p->parity_start )
    {
        /* This region is below this parity region, which
         * means we already validated this as part of a
         * different strip mine pass.
         * Don't validate this region.
         */
        return FBE_TRUE;
    }
    else if ((((reg_local.error & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_CORRUPT_CRC)  ||
              ((reg_local.error & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_CORRUPT_DATA) ||
              ((reg_local.error & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_INVALIDATED)     ) &&
             (b_ignore_corrupt_crc_data_errs == FBE_TRUE)                                          )
    {
        /* for CORRUPT_CRC/DATA errors we will consider it as match
         * For now this is only used by turner test 
         */        
        return FBE_TRUE;
    }
    else if (((reg_local.error & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_RAID_CRC)    ||
             ((reg_local.error & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_CORRUPT_CRC) ||
             ((reg_local.error & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_COPY_CRC)       )       
    {
        /*! @note This is a very big hammer.  Basically when we inject hard 
         *        media errors it can result sectors being invalidated.  Thus 
         *        we always allow a error type if `raid - invalidated'.
         *
         *  @todo Not sure why we always allow `Corrupt CRC'.  The previous 
         *        comment stated that a client could have previously `Corrupt CRC' 
         *        (for instance to a pre-read range) but all extents should have 
         *        been put into a known valid state (i.e. zeroed) prior to 
         *        injecting errors. 
         */
        return FBE_TRUE;
    }
    else if ( reg_local.lba < siots_p->parity_start )
    {
        /* The error region starts before this parity region.
         * Just validate the portion of the error region
         * which overlaps with this parity region.
         */
        reg_local.blocks -= (fbe_u16_t)(siots_p->parity_start - reg_local.lba);
        reg_local.lba = siots_p->parity_start;
    }
    
    /* Normalize the region's lba to match the table.
     */
    reg_local.lba = fbe_logical_error_injection_get_table_lba(siots_p,
                                                              reg_local.lba, 
                                                              reg_local.blocks);

    /* Loop over all error records of the current error table.
     */
    for (idx = 0; idx < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; ++idx)
    {
        fbe_status_t status = FBE_STATUS_OK;
        fbe_logical_error_injection_record_t* rec_p = NULL;
        fbe_logical_error_injection_validate_parms_t parms; /* Parameters used by multiple steps. */
        fbe_logical_error_injection_get_error_info(&rec_p, idx);
        /* Set up the parameters used by multiple validation steps.
         */
        status = fbe_logical_error_injection_set_up_validate_parms(siots_p, reg_local.lba, reg_local.blocks,
                                 idx, idx_region, b_print, &parms);
        if(LEI_COND(status != FBE_STATUS_OK))
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Bad status from set_up_validate_parms siots_p %p reg_local.lba 0x%llx reg_local.blocks 0x%x\n",
                __FUNCTION__, siots_p, (unsigned long long)reg_local.lba,
		reg_local.blocks);
            return FBE_FALSE;
        }

        if (rec_p->err_type == FBE_XOR_ERR_TYPE_NONE)
        {
            /* FBE_XOR_ERR_TYPE_NONE means the end of valid error records.
             */
            break;
        }

        /* Step 1:
         * Check if a downstream object has injected the error.
         */
        if (fbe_logical_error_injection_rec_find_downstream_match(siots_p, rec_p, &reg_local, 
                                      &parms, &b_first_record, FBE_FALSE, idx_region) == FBE_TRUE)
        {
            /* Downstream object injected this error.
             */
            is_found = FBE_TRUE;
            break;
        }

        /* Step 2:
         * Check whether the LBA's of the record and the region overlap.
         * Skip the rest of the check if LBA's do not match.
         */
        if (fbe_logical_error_injection_rec_find_match_lba(siots_p, rec_p, &reg_local, 
                                      &parms, &b_first_record, FBE_FALSE, idx_region) == FBE_FALSE)
        {
            continue;
        }

        /* Print out partially matching error record to assist debugging.
         */
        if (b_print)
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,
                                                      FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: lba match: idx=%d, lba=%llX, pos=%x, err=%x, err_adj_phys=%x\n",
                    __FUNCTION__, idx, (unsigned long long)rec_p->lba,
		    rec_p->pos_bitmap, rec_p->err_type,parms.err_adj_phys);
        }
        
        /* Step 1:
         * Check whether the bitmaps of error record and error region overlap.
         * Skip the rest of the check if bitmaps do not match.
         */
        if (fbe_logical_error_injection_rec_find_match_position(siots_p, rec_p, &reg_local, 
                                           &parms) == FBE_FALSE)
        {
            continue;
        }

        /* Print out partially matching error record to assist debugging.
         */
        if (b_print)
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: lba/pos match: idx=0x%x, lba=0x%llX, pos=0x%x, err=0x%x\n",
                __FUNCTION__, idx, (unsigned long long)rec_p->lba,
		rec_p->pos_bitmap, rec_p->err_type);
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: lba/pos match: b_deg=0x%x, b_phys=0x%x, b_par=0x%x, err=0x%x\n", 
                __FUNCTION__, parms.bitmap_deg, parms.bitmap_phys, 
                parms.bitmap_parity_phys, parms.err_type);
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: lba/pos match: correctable_tbl=%d\n",
                __FUNCTION__, parms.correctable_tbl);
        }

        /* Step 3:
         * Check whether the error types of the record and the region match.
         * Skip the rest of the check if error types do not match.
         */
        if (fbe_logical_error_injection_rec_find_match_error_type(siots_p, rec_p, &reg_local,
                                             &parms) == FBE_FALSE)
        {
            continue;
        }

        /* Print out partially matching error record to assist debugging.
         */
        if (b_print)
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: lba/pos/type match: idx=0x%x, lba=0x%llX, pos=0x%x, err=0x%x\n",
                __FUNCTION__, idx, (unsigned long long)rec_p->lba,
		rec_p->pos_bitmap, rec_p->err_type);
        }
 
        /* Step 4:
         * Check whether error record and error region have the same 
         * correctable-ness.
         */
        if (fbe_logical_error_injection_rec_find_match_correctable(siots_p, rec_p, &reg_local,
                                              &parms) == FBE_FALSE)
        {
            continue;
        }

        /* We have found a match that satisfies all four conditions:
         * 1) LBA range,
         * 2) Position bitmap,
         * 3) Error type,
         * 4) Correctable-ness.
         */
        if (b_print)
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "Record idx: 0x%x matches region - lba: 0x%llx blks: 0x%x bitmask: 0x%x error: 0x%x\n",
                idx, (unsigned long long)reg_local.lba, reg_local.blocks,
		reg_local.pos_bitmask, reg_local.error);
        }

        /* Now that we have a match, update the error region.
         */
        fbe_logical_error_injection_rec_find_match_lba(siots_p, rec_p, &reg_local, 
                                  &parms, &b_first_record, FBE_TRUE /* yes update */, idx_region);
        
        if (reg_local.blocks == 0)
        {
            /* The entire error region has been processed and matched up. 
             * We are done. 
             */
            is_found = FBE_TRUE;
            break;
        }
        else
        {
            /* Only part of the error region has been processed and matched
             * up. We continue with the next error record.
             */
            if (b_print)
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_WARNING,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "Record idx: 0x%x partial lba match - lba: 0x%llx blks: 0x%llx err_type: 0x%x\n",
                    idx, (unsigned long long)rec_p->lba,
		    (unsigned long long)rec_p->blocks, rec_p->err_type);
            }
        }
    } /* end of for(idx...) */

    return is_found;
}
/**************************************************
 * end fbe_logical_error_injection_rec_find_match()
 **************************************************/

/*****************************************************************************
 *          fbe_logical_error_injection_get_valid_bitmask()
 *****************************************************************************
 *
 * @brief   Using the siots simply generate the bitmask of all valid positions.
 *
 * @param   siots_p - pointer to a fbe_raid_siots_t struct.
 *
 * @return  bitmask - Bitmask of valid positions
 *
 *****************************************************************************/
fbe_u16_t fbe_logical_error_injection_get_valid_bitmask(fbe_raid_siots_t *const siots_p)
{
    fbe_u32_t   index;
    fbe_u32_t   width = fbe_raid_siots_get_width(siots_p);
    fbe_u16_t   valid_bitmask = 0;

    for (index = 0; index < width; index++)
    {
        valid_bitmask |= (1 << index);
    }
    return valid_bitmask;
}
/*****************************************************
 * end fbe_logical_error_injection_get_valid_bitmask()
 *****************************************************/

/*****************************************************************************
 *          fbe_logical_error_injection_get_first_set_position()
 *****************************************************************************
 *
 * @brief   Using the bitmask simply generate the bitmask of all valid positions.
 *
 * @param   position_bitmask
 *
 * @return  position - First set position
 *
 *****************************************************************************/
fbe_u32_t fbe_logical_error_injection_get_first_set_position(fbe_u16_t position_bitmask)
{
    fbe_u32_t   bit_index = 0;

    /* Find first
     */
    while (bit_index < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH)
    {
        /* Check the current bit_index
         */
        if ((1 << bit_index) & position_bitmask)
        {
            return bit_index;
        }
        bit_index++;
    }

    return FBE_RAID_INVALID_DISK_POSITION;
}
/**********************************************************
 * end fbe_logical_error_injection_get_first_set_position()
 **********************************************************/

/*****************************************************************************
 *          fbe_logical_error_injection_get_bit_count()
 *****************************************************************************
 *
 * @brief   Using the bitmask simply generate the bitmask of all valid positions.
 *
 * @param   position_bitmask
 *
 * @return  count of bits set
 *
 *****************************************************************************/
fbe_u32_t fbe_logical_error_injection_get_bit_count(fbe_u16_t position_bitmask)
{
    fbe_u32_t   bit_index = 0;
    fbe_u32_t   bit_count = 0;

    /* Find first
     */
    while (bit_index < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH)
    {
        /* Check the current bit_index
         */
        if ((1 << bit_index) & position_bitmask)
        {
            bit_count++;
        }
        bit_index++;
    }

    return bit_count;
}
/**********************************************************
 * end fbe_logical_error_injection_get_bit_count()
 **********************************************************/

/*****************************************************************************
 *          fbe_logical_error_injection_rec_downstream_pos_match()
 *****************************************************************************
 *
 * @brief   This function checks if we have injected an error downstream for
 *          this record for the position indicated.
 *
 * @param   siots_p - pointer to a fbe_raid_siots_t struct.
 * @param   position_to_check - Raid group position to check downstream
 * @param   rec_p - pointer to an error record to examine.
 * @param   region_p - pointer to an error region to match.
 * @param   parms_p - pointer to paramters for multiple steps. 
 * @param   b_first_record_p - whether matching record is the first one.
 * @param   b_update_region - FBE_TRUE if we can modify the region.
 * @param   idx_region - index to an error region to match.
 *
 * @return
 *  FBE_TRUE  - If error injected by downstream object.
 *  FBE_FALSE - Either there was no downstream object or it did not inject it.
 *
 * @author
 *  05/08/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_bool_t fbe_logical_error_injection_rec_downstream_pos_match(fbe_raid_siots_t*    const siots_p,
                                                                 fbe_u32_t position_to_check,
                                                                 fbe_logical_error_injection_record_t* const rec_p,
                                                                 fbe_xor_error_region_t*       const region_p,
                                                                 fbe_logical_error_injection_validate_parms_t*  const parms_p,
                                                                 fbe_bool_t* const b_first_record_p,
                                                                 fbe_bool_t b_update_region,
                                                                 fbe_s32_t idx_region)
{
    fbe_bool_t                              b_injected_by_downstream = FBE_FALSE;   /* By default the downstream object didn't inject the error. */
    fbe_logical_error_injection_object_t   *object_p;
    fbe_u64_t                               num_errors_injected = 0;

    /* First get the downstream logical error injection object (if any).
     */
    object_p = fbe_logical_error_injection_get_downstream_object(siots_p, position_to_check);
    if (object_p == NULL)
    {
        /* Print a debug message and return False.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: downstream pos: %d siots_p: %p reg lba: 0x%llx blks: 0x%llx object not found.\n",
                    position_to_check, siots_p, (unsigned long long)region_p->lba, (unsigned long long)region_p->blocks);
        return FBE_FALSE;
    }
    
    /* If the object is no longer enabled return False.
     * If there is no object associated with this request something is wrong.
     */
    if (object_p->b_enabled == FBE_FALSE)
    {
        /* Print an debug message and return False.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: downstream pos: %d siots_p: %p reg lba: 0x%llx blks: 0x%llx object not enabled.\n",
                    position_to_check, siots_p, (unsigned long long)region_p->lba, (unsigned long long)region_p->blocks);
        return FBE_FALSE;
    }

    /* Get the number of errors injected for this object.
     */
    fbe_logical_error_injection_object_get_num_errors(object_p, &num_errors_injected);

    /* Trace some debug infomation.
     */
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: downstream pos: %d siots_p: %p reg lba: 0x%llx blks: 0x%llx injected: 0x%llx\n",
                    position_to_check, siots_p, (unsigned long long)region_p->lba, (unsigned long long)region_p->blocks, (unsigned long long)num_errors_injected);

    /*! @note Currently we simply validate that there were errors injected.
     */
    if (num_errors_injected == 0)
    {
        if (parms_p->b_print == FBE_TRUE)
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: downstream pos: %d siots_p: %p reg lba: 0x%llx blks: 0x%llx no errors injected.\n",
                    position_to_check, siots_p, (unsigned long long)region_p->lba, (unsigned long long)region_p->blocks);
        }
    }
    b_injected_by_downstream = (num_errors_injected > 0) ? FBE_TRUE : FBE_FALSE;

    /* Return if the error was injected by the downstream object or not.
     */
    return b_injected_by_downstream;
}
/*************************************************************
 * end fbe_logical_error_injection_rec_downstream_pos_match()
 *************************************************************/

/*****************************************************************************
 *          fbe_logical_error_injection_rec_find_downstream_match()
 *****************************************************************************
 *
 * @brief   This function checks if we have injected an error downstream for
 *          this record.  First is checks of error injection has been disabled
 *          for the position associated with the record.  If it has it invokes
 *          the method to check if injection has occurred for the associated
 *          downstream object.
 *
 * @param   siots_p - pointer to a fbe_raid_siots_t struct.
 * @param   rec_p - pointer to an error record to examine.
 * @param   region_p - pointer to an error region to match.
 * @param   parms_p - pointer to paramters for multiple steps. 
 * @param   b_first_record_p - whether matching record is the first one.
 * @param   b_update_region - FBE_TRUE if we can modify the region.
 * @param   idx_region - index to an error region to match.
 *
 * @return
 *  FBE_TRUE  - If error injected by downstream object.
 *  FBE_FALSE - Either there was no downstream object or it did not inject it.
 *
 * @author
 *  05/08/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_bool_t fbe_logical_error_injection_rec_find_downstream_match(fbe_raid_siots_t*    const siots_p,
                                                                 fbe_logical_error_injection_record_t* const rec_p,
                                                                 fbe_xor_error_region_t*       const region_p,
                                                                 fbe_logical_error_injection_validate_parms_t*  const parms_p,
                                                                 fbe_bool_t* const b_first_record_p,
                                                                 fbe_bool_t b_update_region,
                                                                 fbe_s32_t idx_region)
{
    fbe_bool_t                              b_injected_by_downstream = FBE_FALSE;   /* By default the downstream object didn't inject the error. */
    fbe_raid_geometry_t                    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t                         object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    fbe_logical_error_injection_object_t   *object_p;
    fbe_u16_t                               object_enabled_bitmask = 0;
    fbe_u16_t                               downstream_enabled_bitmask = 0;
    fbe_u32_t                               position = FBE_RAID_INVALID_DISK_POSITION;

    /* First get the object assocaited with this request.
     * If there is no object associated with this request something is wrong.
     */
    object_p = fbe_logical_error_injection_find_object(object_id);
    if (object_p == NULL)
    {
        /* Print an error message and return False.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: find downstream: siots_p: %p reg lba: 0x%llx blks: 0x%llx object not found.\n",
                    siots_p, (unsigned long long)region_p->lba, (unsigned long long)region_p->blocks);
        return FBE_FALSE;
    }

    /* If the object is not enabled or the record position mask indicates that
     * this position is enalbed, then the downstream object did not inject the
     * error.
     */
    object_enabled_bitmask = (fbe_u16_t)object_p->edge_hook_bitmask;
    if ((object_p->b_enabled == FBE_FALSE)                  ||
        ((object_enabled_bitmask & rec_p->pos_bitmap) != 0)    )
    {
        /* Normal case is error injection is enabled for all positions.
         */
        return FBE_FALSE;
    }

    /* If we are here it means that error injection has been disabled for one
     * or more positions for this object.  Check any downstream objects to
     * see if there is a match.
     */
    downstream_enabled_bitmask = fbe_logical_error_injection_get_valid_bitmask(siots_p);
    downstream_enabled_bitmask &= ~object_enabled_bitmask;
    while ((downstream_enabled_bitmask & rec_p->pos_bitmap) != 0)
    {
        /* Get the next position to check.
         */
        position = fbe_logical_error_injection_get_first_set_position((downstream_enabled_bitmask & rec_p->pos_bitmap));
        if (position == FBE_RAID_INVALID_DISK_POSITION)
        {
            /* No more positions to check. Return False.
             */
            return FBE_FALSE;
        }

        /* Invoke method to check if downstream object injected for this request.
         */
        b_injected_by_downstream = fbe_logical_error_injection_rec_downstream_pos_match(siots_p, 
                                                                                        position,
                                                                                        rec_p,
                                                                                        region_p,
                                                                                        parms_p,
                                                                                        b_first_record_p,
                                                                                        b_update_region,
                                                                                        idx_region);
        if (b_injected_by_downstream == FBE_TRUE)
        {
            break;
        }

        /* Goto next.
         */
        downstream_enabled_bitmask &= ~(1 << position);

    } /* while there are more positions to check */

    /* Return if the error was injected by the downstream object or not.
     */
    return b_injected_by_downstream;
}
/*************************************************************
 * end fbe_logical_error_injection_rec_find_downstream_match()
 *************************************************************/

/*****************************************************************************
 * fbe_logical_error_injection_rec_find_match_lba()
 *****************************************************************************
 * @brief
 *  This function checks whether the LBA's of the record and 
 *  the region overlap. Rewritten from fbe_logical_error_injection_rec_find_match.
 *

 * @param siots_p - pointer to a fbe_raid_siots_t struct.
 * @param rec_p - pointer to an error record to examine.
 * @param region_p - pointer to an error region to match.
 * @param parms_p - pointer to paramters for multiple steps. 
 * @param b_first_record_p - whether matching record is the first one.
 * @param b_update_region - FBE_TRUE if we can modify the region.
 * @param idx_region - index to an error region to match.
 *
 * @return
 *  FBE_TRUE  - If LBA's        match.
 *  FBE_FALSE - If LBA's do not match.
 *
 * @author
 *  07/26/06 - Created. SL
 *
 ****************************************************************/
static fbe_bool_t fbe_logical_error_injection_rec_find_match_lba(fbe_raid_siots_t*    const siots_p,
                                                                 fbe_logical_error_injection_record_t* const rec_p,
                                                                 fbe_xor_error_region_t*       const region_p,
                                                                 fbe_logical_error_injection_validate_parms_t*  const parms_p,
                                                                 fbe_bool_t* const b_first_record_p,
                                                                 fbe_bool_t b_update_region,
                                                                 fbe_s32_t idx_region)
{
    /* When validating LBA, degraded-ness and error type are irrelevant.
     * Thus use the physical bitmap that does not change thoroughout the
     * validation process.
     */
    const fbe_u16_t bitmap_phys = parms_p->bitmap_phys_const;
    fbe_lba_t lba_phys_tbl;        /* physical LBA based on err tbl */
    fbe_bool_t  b_lba_match = FBE_TRUE;  /* By default, we assume a match. */
    fbe_lba_t   extended_lba;          
    fbe_u16_t   extended_blocks; 

    /* If bitmap_phys is equal to 0 or greater than the maximum possible 
     * bitmap position, it is not valid. This validation is needed to avoid 
     * the asserts in fbe_logical_error_injection_bitmap_to_position. 
     */
    if (bitmap_phys == 0 || bitmap_phys >= (1 << fbe_raid_siots_get_width(siots_p)))
    {
        return FBE_FALSE;
    }

    /*! Convert unit relative physical address to physical address.
     *  fbe_logical_error_injection_record_t::lba: unit relative physical address.
     *  fbe_xor_error_region_t::lba:               physical address. 
     *  
     *  @note The value in region_p has already been normalize to the the table.
     */    
    lba_phys_tbl = rec_p->lba;

    /* Extend error region lba if needed for media errors
     */
    extended_lba = region_p->lba;
    extended_blocks = region_p->blocks;
    if (((region_p->error & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR)||
        ((region_p->error & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR))
    {
        if(siots_p->vcts_ptr->error_regions.next_region_index > 1)
            fbe_logical_error_injection_extend_error_region(siots_p,&extended_lba,&extended_blocks,idx_region);
    }

    /* If enabled it means we didn't match.
     */
    if (parms_p->b_print == FBE_TRUE)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: find match lba: rec: 0x%llx phys: 0x%llx blks: 0x%llx\n",
                    (unsigned long long)rec_p->lba, (unsigned long long)lba_phys_tbl, (unsigned long long)rec_p->blocks);
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: find match lba: region: 0x%llx blks: 0x%x \n",
                    (unsigned long long)extended_lba, extended_blocks);
    }
    /*
     * - Raid does not strip mine below the optimal block size. 
     *  
     * All of the above limits our ability to do a more precise check so we 
     * check based on the region size. 
     *  
     * In addition, if the error was a raid crc error, it might have been caused by an 
     * earlier error whose range was extended because of the above. 
     * 
     * if (fbe_logical_error_injection_opt_size_not_one(siots_p, bitmap_phys) == FBE_TRUE)
     *     ||
     *     ((region_p->error & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_RAID_CRC) )
     */
    if (1) /*! @todo, do we need the else case anymore? */
    {
        /* We need to use the max possible optimal block size 
         * since it is possible that in the past we had a hot spare with a 
         * different optimal block size swapped in, and thus uncorrectables 
         * could be left over in an area that would only make sense with 
         * this max optimal block size. 
         */
        fbe_block_count_t round_blocks = FBE_RAID_MAX_OPTIMAL_BLOCK_SIZE;

        /* lba and lba + blocks may span REMAP_ROUND_BLOCKS boundary.
         *
         *         0          64                 n * 64
         *         |----------| ... ... |----------|
         * Case 1:   <----->
         * Case 2:   <--------- ... ... -------->
         */
        fbe_block_count_t blks_start = (fbe_u32_t)(lba_phys_tbl % round_blocks);
        fbe_block_count_t blks_end   = (fbe_u32_t)(blks_start + rec_p->blocks);
        fbe_block_count_t no_of_regions = blks_end / round_blocks + 1;

        /* Round the blocks up to the number of regions.
         */
        fbe_block_count_t total_blocks = no_of_regions * round_blocks;
        
        /* Round the lba to a track boundary, since ATA drives invalidate
         * full tracks at a time.
         */
        if ( (lba_phys_tbl % round_blocks ) != 0 )
        {
            /* If the lba is not aligned, then align it.
             * Just go back to the prior boundary by determining
             * how far we are from the boundary and
             * subtracting from our lba.
             */
            lba_phys_tbl -= (lba_phys_tbl % round_blocks);
        }
        
        
        /* This FRU is an ATA drive. Require that the error record 
         * has some overlap with the error region.
         */
        if (fbe_logical_error_injection_overlap(extended_lba, extended_blocks,
                                                lba_phys_tbl, total_blocks) == FBE_TRUE)
        {
            /* The error record has some overlap with the error region.  
             * In this case, we handle the entire error region in one pass. 
             * Set blocks to zero for bookkeeping.
             */
            if (b_update_region)
            {
                region_p->blocks = 0;
            }
        }
        else
        {
            /* There is no match.
             */
            b_lba_match = FBE_FALSE;
            if (parms_p->b_print == FBE_TRUE)
            {
                fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: find match lba: region: 0x%llx blks: 0x%x rec: 0x%llx blks: 0x%llx\n",
                    (unsigned long long)extended_lba, extended_blocks, (unsigned long long)lba_phys_tbl, (unsigned long long)total_blocks);
            }
        }
    }
    else
    {
        fbe_s32_t blks_overlap; /* number of blocks overlapped */

        /* This FRU has optimal size of 1.
         */
        if (fbe_logical_error_injection_record_contain_region(lba_phys_tbl, 
                                                             rec_p->blocks, 
                                                             extended_lba, extended_blocks) == FBE_TRUE)
        {
            /* The error record contains the error region completely.
             * In this case, we handle the entire error region in one pass. 
             * Set blocks to zero for bookkeeping.
             */
            if (b_update_region)
            {
                region_p->blocks = 0;
            }
        }
        else if (fbe_logical_error_injection_record_head_of_region(lba_phys_tbl,  
                                          rec_p->blocks,
                                          region_p->lba, region_p->blocks,
                                          *b_first_record_p, 
                                          &blks_overlap) == FBE_TRUE)
        {
            /* The error record is at the head of the error region.
             * In this case, we handle the error region in multiple passes. 
             * Substract lba and blocks for bookkeeping.
             */
            if(LEI_COND(region_p->blocks < blks_overlap))
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: region_p->blocks 0x%x < blks_overlap 0x%x\n",
                    __FUNCTION__, region_p->blocks, blks_overlap);
                return b_lba_match;
            }
            if (b_update_region)
            {
                region_p->lba    += blks_overlap;
                region_p->blocks -= blks_overlap;
                *b_first_record_p = FBE_FALSE;
            }
        }
        else
        {
            /* There is no match. 
             */
            b_lba_match = FBE_FALSE;
            if (parms_p->b_print == FBE_TRUE)
            {
                fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: find match lba: region: 0x%llx blks: 0x%x phys: 0x%llx blks: 0x%llx\n",
                    (unsigned long long)region_p->lba, region_p->blocks, (unsigned long long)lba_phys_tbl, (unsigned long long)rec_p->blocks);
            }
        }
    }

    return b_lba_match;
}
/******************************************************
 * end fbe_logical_error_injection_rec_find_match_lba()
 ******************************************************/

/****************************************************************
 * fbe_logical_error_injection_rec_find_match_position()
 ****************************************************************
 * @brief
 *  This function checks whether the positions of the record and 
 *  the region overlap. Rewritten from fbe_logical_error_injection_rec_find_match.
 *

 * @param siots_p - pointer to a fbe_raid_siots_t struct.
 * @param rec_p - pointer to an error record to examine.
 * @param region_p - pointer to an error region to match.
 * @param parms_p - pointer to paramters for multiple steps. 
 *
 * @return
 *  FBE_TRUE  - If positions        match.
 *  FBE_FALSE - If positions do not match.
 *
 * @author
 *  07/26/06 - Created. SL
 *
 ****************************************************************/
static fbe_bool_t 
fbe_logical_error_injection_rec_find_match_position(fbe_raid_siots_t*    const siots_p,
                               fbe_logical_error_injection_record_t* const rec_p, 
                               fbe_xor_error_region_t* const region_p,
                               fbe_logical_error_injection_validate_parms_t*  const parms_p)
{
    const fbe_u16_t bitmap_deg         = parms_p->bitmap_deg;
    const fbe_u16_t bitmap_parity_phys = parms_p->bitmap_parity_phys;
    const fbe_u16_t err_adj_phys       = parms_p->err_adj_phys;
    const fbe_bool_t correctable_tbl        = parms_p->correctable_tbl;
    const fbe_xor_error_type_t err_type     = parms_p->err_type;
    fbe_bool_t  b_pos_match = FBE_TRUE;  /* By default, we assume a match. */

    /* For now we only perform postion match for RAID 6 LUNs. 
     * For other types of LUNS it will be added in the future.
     */
    if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) == FBE_FALSE)
    {
        return b_pos_match;
    }

    /* Remove degraded positions from the bitmap since we cannot inject 
     * error(s) to the degraded position(s).
     */
    parms_p->bitmap_phys &= ~bitmap_deg;

    if (err_type == FBE_XOR_ERR_TYPE_COH && bitmap_deg != 0 && 
        parms_p->bitmap_phys != 0 &&
        (region_p->error & FBE_XOR_ERR_TYPE_ZEROED) == 0)
    {
        /* For coherency error, the error(s) are assigned to the degraded 
         * position(s) only. Thus the error bitmap is identical to the 
         * degraded bitmap, not the OR of degraded and injection bitmaps. 
         * We don't want to set the bitmap to degraded if we took a
         * zeroed error, since in this case the errors will show up
         * on the drive we injected to.
         */
        parms_p->bitmap_phys |= bitmap_deg;
    }

    /* If we are degraded, and the error record is not correctable,
     * then we should expect uncorrectable errors to show up for degraded positions.
     */
    if ( bitmap_deg != 0 && correctable_tbl == FBE_FALSE)
    {
        parms_p->bitmap_phys |= bitmap_deg;
    }

    if ((err_type == FBE_XOR_ERR_TYPE_TS || 
         err_type == FBE_XOR_ERR_TYPE_WS) &&
        region_p->pos_bitmask & bitmap_parity_phys)
    {
        /* If a strip's data and crc's are valid a write stamp
         * error will cause the write stamp on the parity drives to be
         * updated.  This is the footprint of that valid state.
         */
    }
    /* We took an invalidated error here.  If we were degraded in the past,
     * then this error might be completely valid as long as this error
     * record shows some other error on a different position.
     */
    else if ( err_type == FBE_XOR_ERR_TYPE_RAID_CRC &&
              region_p->pos_bitmask != parms_p->bitmap_phys )
    {
    }
    /* A time stamp error can either show up on the drive it was injected
     * to or any drive that is not parity, not dead, or does not have
     * an error injected to it.
     */
    else if ( (err_type == FBE_XOR_ERR_TYPE_TS || err_type == FBE_XOR_ERR_TYPE_WS) &&
              (((bitmap_parity_phys | err_adj_phys | bitmap_deg) ^ 
                 parms_p->bitmap_width) != 0) )
    {
    }
    /* 1) We saw a coherency error
     * 2) The combination of error adjacency and degraded
     *    bits show that more than one error was injected.
     * 3) The error record contains, but does not necessarily match
     *    a) parity
     *    b) the error region
     * 4) A coherency error was injected on the same strip
     *    and thus might have been caused by a different drive.
     */
    else if ( (err_type == FBE_XOR_ERR_TYPE_COH || 
               err_type == FBE_XOR_ERR_TYPE_POC_COH ||
               err_type == FBE_XOR_ERR_TYPE_N_POC_COH) &&
              ( fbe_raid_get_bitcount(
                  bitmap_deg | err_adj_phys) > 1 ) &&
              ( (region_p->pos_bitmask & bitmap_parity_phys ) ||
                (region_p->pos_bitmask & parms_p->bitmap_phys) ) &&
              (fbe_logical_error_injection_coherency_adjacent(siots_p, rec_p,
                                                              fbe_raid_siots_get_width(siots_p), bitmap_deg) ||
               fbe_logical_error_injection_is_coherency_err_type(rec_p) ) )
    {
    }
    /* 1) We saw a crc error
     * 2) The combination of error adjacency and degraded
     *    bits show that more than one error was injected.
     * 3) The region contains a degraded position.
     * 4) This record is a coherency error.
     */
    else if ( (FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC(err_type)) &&
              fbe_logical_error_injection_is_coherency_err_type(rec_p) )
    {
    }
    /* 1) We saw a timestamp error.
     * 2) The combination of error adjacency and degraded
     *    bits show that more than one error was injected.
     * 3) The error record contains parity.
     * 4) A coherency error was injected on the same strip
     *    and thus might have been caused by a different drive.
     * On zeroed strips, coherency errors can make us think
     * the strip isn't zeroed.  We will see the initial
     * timestamp errors and will log errors against parity.
     */
    else if ( (err_type == FBE_XOR_ERR_TYPE_TS) &&
              ( fbe_raid_get_bitcount(
                  bitmap_deg | err_adj_phys) > 1 ) &&
              (region_p->pos_bitmask & bitmap_parity_phys) &&
              fbe_logical_error_injection_is_coherency_err_type(rec_p) )
    {
    }
    else if ( (err_type == FBE_XOR_ERR_TYPE_RB_FAILED || FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC(err_type)) &&
              fbe_logical_error_injection_is_coherency_err_type(rec_p) && 
              (region_p->pos_bitmask & bitmap_deg) )
    {
        /* We got a rebuild failed error or crc error
         * because of a degraded condition
         * and this coherency error.
         */
    }
    else if ( err_type  == FBE_XOR_ERR_TYPE_RB_FAILED &&
              (bitmap_parity_phys & err_adj_phys) &&
              (region_p->pos_bitmask & bitmap_deg) )
    {
        /* We got a rebuild failed error because there was an error
         * injected to a parity position.
         */
    }
    else if ( (err_type == FBE_XOR_ERR_TYPE_RB_FAILED) &&
              (fbe_raid_get_bitcount( bitmap_deg | err_adj_phys) > 1) )
    {
        /* We got a rebuild failed error since we injected errors to a degraded strip.
         */
    }

    else if ( err_type == FBE_XOR_ERR_TYPE_TS &&
              (region_p->error & FBE_XOR_ERR_TYPE_INITIAL_TS) &&
              (region_p->pos_bitmask & bitmap_parity_phys) &&
              ( fbe_raid_get_bitcount(
                  bitmap_deg | err_adj_phys) > 1 ) )
    {
        /* 1) We have a TS error on a parity drive.
         * 2) An initial timestamp was found.
         * 3) We have more than one error or faulted drive.
         * Then this is allowed since it can happen on zeroed stripes.
         */
    }
    else if (err_type == FBE_XOR_ERR_TYPE_RAID_CRC)
    {
        /* Invalidated errors may occur on any position, since
         * we could rebuild any position double degraded and
         * receive a single error on another drive.
         */
    }
    else if ((err_type == FBE_XOR_ERR_TYPE_INVALIDATED)  ||
             (err_type == FBE_XOR_ERR_TYPE_CORRUPT_CRC)  ||
             (err_type == FBE_XOR_ERR_TYPE_CORRUPT_DATA)    )
    {
        /* Corrupt CRC/DATA errors may occur on any position, since
         * we could rebuild any position double degraded and
         * receive a single error on another drive.
         */
    }
    /* Check whether the bitmaps of the record and the region overlap.
     * Skip the rest of the check if the two bitmaps do not overlap.
     */
    else if ((region_p->pos_bitmask & parms_p->bitmap_phys) == 0)
    {
        /* There is no match.
         */
        b_pos_match = FBE_FALSE;
    }

    return b_pos_match;
}
/****************************************
 * end fbe_logical_error_injection_rec_find_match_position()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_rec_find_match_error_type()
 ****************************************************************
 * @brief
 *  This function checks whether the error types of the record and 
 *  the region match. Rewritten from fbe_logical_error_injection_rec_find_match.
 *

 * @param siots_p - pointer to a fbe_raid_siots_t struct.
 * @param rec_p - pointer to an error record to examine.
 * @param region_p - pointer to an error region to match.
 * @param parms_p - pointer to paramters for multiple steps. 
 *
 * @return
 *  FBE_TRUE  - If error types        match.
 *  FBE_FALSE - If error types do not match.
 *
 * @author
 *  07/26/06 - Created. SL
 *
 ****************************************************************/
static fbe_bool_t 
fbe_logical_error_injection_rec_find_match_error_type(fbe_raid_siots_t*    const siots_p,
                                                      fbe_logical_error_injection_record_t* const rec_p,
                                                      fbe_xor_error_region_t* const region_p,
                                                      fbe_logical_error_injection_validate_parms_t*  const parms_p)
{
    const int b_print                  = parms_p->b_print;
    const fbe_u16_t bitmap_deg         = parms_p->bitmap_deg;
    const fbe_u16_t bitmap_parity_phys = parms_p->bitmap_parity_phys;
    const fbe_u16_t err_adj_phys       = parms_p->err_adj_phys;
    const fbe_bool_t correctable_tbl        = parms_p->correctable_tbl;
    const fbe_xor_error_type_t err_type     = parms_p->err_type;
    const fbe_u16_t bitmap_parity_log  = parms_p->bitmap_parity_log;
    const fbe_u16_t bitmap_phys        = parms_p->bitmap_phys;
    fbe_bool_t  b_err_match = FBE_TRUE;  /* By default, we assume a match. */

    if (       err_type == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR &&
        rec_p->err_type == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR)
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if (       err_type == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR &&
             rec_p->err_type == FBE_XOR_ERR_TYPE_RND_MEDIA_ERR)
    {
        /* FBE_XOR_ERR_TYPE_RND_MEDIA_ERR is converted to either 
         * FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR or FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR
         * at the injection time.
         */
    }
    else if ( err_type == FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM &&
              rec_p->err_type == FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM)
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if ( err_type == FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM &&
              rec_p->err_type == FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM)
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if ( (err_type == FBE_XOR_ERR_TYPE_RB_FAILED) &&
              ((rec_p->err_type == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR) ||
               (rec_p->err_type == FBE_XOR_ERR_TYPE_RND_MEDIA_ERR)) &&
              (region_p->pos_bitmask & bitmap_deg) )
    {
        /* A rebuild failed because of a media error on a different drive.
         */
    }
    /* If we are degraded, AND a degraded drive received an error 
     * AND we have an uncorrectable,
     * and the error received is a rebuild failed, then the type is ok.
     * This is because when we do a rebuild and it fails due to uncorrectables,
     * we log this to the error region as either  
     * rebuild failed error, or standard crc error.
     */
    else if ( bitmap_deg != 0 && 
              (region_p->pos_bitmask & bitmap_deg) &&
              correctable_tbl == FBE_FALSE &&
              (err_type == FBE_XOR_ERR_TYPE_RB_FAILED ||
              FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC(err_type)) )
    {
    }
    else if (FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC(err_type)) 
    {
        /* There are following possible causes of a checksum error:
         * 1) An FBE_XOR_ERR_TYPE_CRC error is injected. 
         * 2) An FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC is injected
         * 3) An FBE_XOR_ERR_TYPE_MULTI_BIT_CRC is injected
         * 4) An FBE_XOR_ERR_TYPE_[1NS, 1S, 1R, 1D] error is injected, 
         *    and the error is checksum detectable. 
         * 5) An FBE_XOR_ERR_TYPE_[1COD, 1COP] error is injected. 
         */
        if ((rec_p->err_type == FBE_XOR_ERR_TYPE_CRC) ||
            (rec_p->err_type == FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC) ||
            (rec_p->err_type == FBE_XOR_ERR_TYPE_MULTI_BIT_CRC) ||
            (rec_p->err_type == FBE_XOR_ERR_TYPE_MULTI_BIT_WITH_LBA_STAMP))
        {
            /* A match is found. Fall through to the next check.
             */
        }
        else if (rec_p->crc_det == RG_R6_ERR_PARM_YES &&
                 (rec_p->err_type == FBE_XOR_ERR_TYPE_1NS || 
                  rec_p->err_type == FBE_XOR_ERR_TYPE_1S  || 
                  rec_p->err_type == FBE_XOR_ERR_TYPE_1R  || 
                  rec_p->err_type == FBE_XOR_ERR_TYPE_1D))
        {
            /* A match is found. Fall through to the next check.
             */
        }
        else if (rec_p->crc_det == RG_R6_ERR_PARM_UNUSED &&
                 (rec_p->err_type == FBE_XOR_ERR_TYPE_1COD || 
                  rec_p->err_type == FBE_XOR_ERR_TYPE_1COP))
        {
            /* A match is found. Fall through to the next check.
             */
        }
        else if (rec_p->err_type == FBE_XOR_ERR_TYPE_INVALIDATED ||
                 rec_p->err_type == FBE_XOR_ERR_TYPE_RAID_CRC ||
                 rec_p->err_type == FBE_XOR_ERR_TYPE_CORRUPT_CRC ||
                 rec_p->err_type == FBE_XOR_ERR_TYPE_DH_CRC ||
                 rec_p->err_type == FBE_XOR_ERR_TYPE_KLOND_CRC)
        {
            /* A match is found if one of these errors is injected into a
             * RAID 6 it might be marked as a CRC error. Fall through to the 
             * next check.
             */
        }
        else
        {
            /* Skip the rest of the check if error types do not match. 
             */
            if (b_print)
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_WARNING,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s err reg's err type of _CRC has no match\n", __FUNCTION__);
            }

            b_err_match = FBE_FALSE;
        }
    }
    else if (       err_type == FBE_XOR_ERR_TYPE_KLOND_CRC && 
             rec_p->err_type == FBE_XOR_ERR_TYPE_KLOND_CRC)
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if (       err_type == FBE_XOR_ERR_TYPE_DH_CRC && 
             rec_p->err_type == FBE_XOR_ERR_TYPE_DH_CRC)
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if (       err_type == FBE_XOR_ERR_TYPE_DH_CRC && 
             ( rec_p->err_type == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR ||
               rec_p->err_type == FBE_XOR_ERR_TYPE_RND_MEDIA_ERR ) )
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if (err_type == FBE_XOR_ERR_TYPE_RAID_CRC && 
             (rec_p->err_type == FBE_XOR_ERR_TYPE_RAID_CRC || 
              correctable_tbl == FBE_FALSE))
    {
        /* A match is found. Fall through to the next check.
         * FBE_XOR_ERR_TYPE_RAID_CRC is detected under these scenarios:
         * 1) FBE_XOR_ERR_TYPE_RAID_CRC is injected,
         * 2) Errors injected are uncorrectable. 
         */
    }
    else if (err_type == FBE_XOR_ERR_TYPE_RAID_CRC)
    {
        /* Any "Invalidated" error was caused by an uncorrectable
         * that occurred in the past.
         * This would presumably have been validated by the
         * rg error validation code in the past, so we
         * assume that it is okay.
         */
        if (b_print)
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "lba/pos/type match raid crc: lba=0x%llX, pos=0x%x, err=0x%x\n",
                (unsigned long long)rec_p->lba, rec_p->pos_bitmap,
		rec_p->err_type);
        }
    }
    else if (err_type == FBE_XOR_ERR_TYPE_INVALIDATED &&
             rec_p->err_type == FBE_XOR_ERR_TYPE_INVALIDATED)
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if (err_type == FBE_XOR_ERR_TYPE_CORRUPT_CRC &&
             rec_p->err_type == FBE_XOR_ERR_TYPE_CORRUPT_CRC)
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if ((err_type == FBE_XOR_ERR_TYPE_CORRUPT_CRC) ||
             (err_type == FBE_XOR_ERR_TYPE_CORRUPT_DATA))
    {
        /* Any "Corrupt CRC/DATA" error was caused by an uncorrectable
         * that occurred in the past.This would presumably have been 
         * validated because of corrupt CRC operation. So, we
         * assume that it is okay.
         */
        if (b_print)
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "lba/pos/type match raid crc: lba=0x%llX, pos=0x%x, err=0x%x\n",
                (unsigned long long)rec_p->lba, rec_p->pos_bitmap,
		rec_p->err_type);
        }
    }
    else if (       err_type == FBE_XOR_ERR_TYPE_WS &&
            (rec_p->err_type == FBE_XOR_ERR_TYPE_TS ||
             rec_p->err_type == FBE_XOR_ERR_TYPE_WS ||
             rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_WS))
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if (       err_type == FBE_XOR_ERR_TYPE_TS &&
            (rec_p->err_type == FBE_XOR_ERR_TYPE_TS ||
             rec_p->err_type == FBE_XOR_ERR_TYPE_WS ||
             rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_TS))
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if ( err_type == FBE_XOR_ERR_TYPE_TS &&
              ( fbe_raid_get_bitcount(bitmap_deg | err_adj_phys) > 1 ) &&
              (region_p->pos_bitmask & bitmap_parity_phys) &&
              fbe_logical_error_injection_is_coherency_err_type(rec_p) )
    {
        /* Timestamp error on parity caused by a coherency 
         * error on a zeroed strip.
         */
    }
    else if ( err_type == FBE_XOR_ERR_TYPE_TS &&
              (region_p->error & FBE_XOR_ERR_TYPE_INITIAL_TS) &&
              (region_p->pos_bitmask & bitmap_parity_phys) &&
              ( fbe_raid_get_bitcount(
                  bitmap_deg | err_adj_phys) > 1 ) )
    {
        /* 1) We have a TS error on a parity drive.
         * 2) An initial timestamp was found.
         * 3) We have more than one error or faulted drive.
         * Then this is allowed since it can happen on zeroed stripes.
         */
    }
    else if (((err_type == FBE_XOR_ERR_TYPE_SS) &&
              (rec_p->err_type == FBE_XOR_ERR_TYPE_SS ||
               rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_SS)) &&
             (region_p->pos_bitmask & bitmap_parity_phys))
    {
        /* A shed stamp error was reported on a parity postion, which
         * is a match. Fall through to the next check.
         */
    }
    else if (((err_type == FBE_XOR_ERR_TYPE_LBA_STAMP) &&
              (rec_p->err_type == FBE_XOR_ERR_TYPE_SS ||
               rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_SS)) &&
             (!(region_p->pos_bitmask & bitmap_parity_phys) ||
              fbe_raid_geometry_is_mirror_type(fbe_raid_siots_get_raid_geometry(siots_p))||
              fbe_raid_geometry_is_raid0(fbe_raid_siots_get_raid_geometry(siots_p))))
    {
        /* A LBA stamp error was reported on a data postion, which
         * is a match. Fall through to the next check.
         */
    }
    else if ((err_type == FBE_XOR_ERR_TYPE_SS) &&
             (rec_p->err_type == FBE_XOR_ERR_TYPE_LBA_STAMP) &&
             (region_p->pos_bitmask & bitmap_parity_phys))
    {
        /* A Shed Stamp error was reported on a parity postion, which
         * is a match even if an LBA was injected, since the injection code 
         * treats LBA and SS the same, injecting LBA or SS based on the position. 
         * Fall through to the next check.
         */
    }
    else if (       err_type == FBE_XOR_ERR_TYPE_LBA_STAMP &&
             rec_p->err_type == FBE_XOR_ERR_TYPE_LBA_STAMP    )
    {
        /* A LBA stamp error was reported on a data postion, which
         * is a match. Fall through to the next check.
         */
    }
    else if (       err_type == FBE_XOR_ERR_TYPE_BOGUS_WS &&
             rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_WS)
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if (       err_type == FBE_XOR_ERR_TYPE_BOGUS_TS &&
             rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_TS)
    {
        /* A match is found. Fall through to the next check.
         */
    }
    else if (       err_type == FBE_XOR_ERR_TYPE_BOGUS_SS &&
             rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_SS &&
             (region_p->pos_bitmask & bitmap_parity_phys))
    {
        /* A shed stamp error was reported on a parity postion, which
         * is a match. Fall through to the next check.
         */
    }
    else if ((err_type == FBE_XOR_ERR_TYPE_POC_COH || 
              err_type == FBE_XOR_ERR_TYPE_N_POC_COH) &&
             (rec_p->err_type == FBE_XOR_ERR_TYPE_SS ||
              rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_SS ||
              rec_p->err_type == FBE_XOR_ERR_TYPE_TS ||
              rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_TS ||
              rec_p->err_type == FBE_XOR_ERR_TYPE_WS ||
              rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_WS))
    {
        /* We injected an error into one of the stamps and
         * on Raid 6 received a POC_COH error.
         */  
    }
    else if ( ( err_type == FBE_XOR_ERR_TYPE_POC_COH ||
                err_type == FBE_XOR_ERR_TYPE_N_POC_COH ||
                err_type == FBE_XOR_ERR_TYPE_COH ||
                err_type == FBE_XOR_ERR_TYPE_RB_FAILED ) &&
              ( fbe_raid_get_bitcount(bitmap_deg | err_adj_phys) > 1 ) &&
              fbe_logical_error_injection_coherency_adjacent(siots_p,
                                        rec_p, fbe_raid_siots_get_width(siots_p), bitmap_deg) )
    {
        /* We found a match since a coherency error was injected
         * and multiple errors were injected.
         */
    }
    else if ( ( err_type == FBE_XOR_ERR_TYPE_POC_COH ||
                err_type == FBE_XOR_ERR_TYPE_N_POC_COH ||
                err_type == FBE_XOR_ERR_TYPE_COH ) &&
              ( fbe_raid_get_bitcount(bitmap_deg | err_adj_phys) > 1 ) &&
              (rec_p->err_type == FBE_XOR_ERR_TYPE_1POC))
    {
        /* We found a match since a parity of checksum error was injected
         * and a coherency error may result.
         */
    }
    else if ( (err_type == FBE_XOR_ERR_TYPE_POC_COH ||
               err_type == FBE_XOR_ERR_TYPE_N_POC_COH) &&
              (rec_p->err_type == FBE_XOR_ERR_TYPE_1POC ||
               fbe_logical_error_injection_is_coherency_err_type(rec_p) ) )
    {
        /* We injected a POC coherency and saw one too.
         */
    }
    else if (err_type == FBE_XOR_ERR_TYPE_COH)
    {
        /* There are five possible causes of a coherency error:
         * 1) An FBE_XOR_ERR_TYPE_[1NS, 1S, 1R, 1D] error is injected, 
         *    and the error is NOT checksum detectable. 
         * 2) An FBE_XOR_ERR_TYPE_1POC error is injected. 
         * 3) An error is injected to the diagonal parity drive with two other
         *    errored/dead positions.
         * 4) COH is found on the data drive if one of parity drives has
         *    an errored/dead position and other parity drive is injected
         *    a Time Stamp error.
         * 5) A COH error was injected on a non-R6 parity unit 
         * 6) Raw mirror magic number mismatch 
         * 7) Raw mirror sequence number mismatch 
         */            
        if (rec_p->crc_det == RG_R6_ERR_PARM_NO &&
            (rec_p->err_type == FBE_XOR_ERR_TYPE_1NS  || 
             rec_p->err_type == FBE_XOR_ERR_TYPE_1S   || 
             rec_p->err_type == FBE_XOR_ERR_TYPE_1R   || 
             rec_p->err_type == FBE_XOR_ERR_TYPE_1D   ||
             rec_p->err_type == FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM ||
             rec_p->err_type == FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM))
        {
            /* A match is found. Fall through to the next check.
             */
        }
        else if (rec_p->crc_det  == RG_R6_ERR_PARM_UNUSED && 
                 (rec_p->err_type == FBE_XOR_ERR_TYPE_1POC ||
                  rec_p->err_type == FBE_XOR_ERR_TYPE_INVALIDATED ||
                  rec_p->err_type == FBE_XOR_ERR_TYPE_RAID_CRC ||
                  rec_p->err_type == FBE_XOR_ERR_TYPE_CORRUPT_CRC))
        {
            /* A match is found. Fall through to the next check.
             */
        }
        else if (((region_p->pos_bitmask & bitmap_parity_log) == 0) &&
                ((bitmap_phys & bitmap_parity_phys) == bitmap_parity_phys))     
        {
            /* A match is found. Fall through to the next check.
             */
        }
        else if (((region_p->pos_bitmask & bitmap_parity_log) == 0) && 
            ((bitmap_parity_phys & err_adj_phys) == bitmap_parity_phys) &&
            (fbe_logical_error_injection_has_adjacent_type(siots_p, rec_p, fbe_logical_error_injection_is_ts_err_type,
                                     ((1 << fbe_raid_siots_get_width(siots_p)) - 1))))
        {
            /* A match is found. Fall through to the next check.
             */
        }
        else if (  err_type == FBE_XOR_ERR_TYPE_COH)
        {

            /* A match is found. Fall through to the next check.
             */
        }
        else
        {
            /* Skip the rest of the check if error types do not match. 
             */
            if (b_print)
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_WARNING,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: err reg's err type of _COH has no match\n", __FUNCTION__);
            }

            b_err_match = FBE_FALSE;
        }
    }
    else if ( err_type == FBE_XOR_ERR_TYPE_RB_FAILED &&
              fbe_logical_error_injection_is_coherency_err_type(rec_p) && 
              (region_p->pos_bitmask & bitmap_deg) )
    {
        /* A rebuild failed because of a coherency error.
         */
    }
    else if ( err_type == FBE_XOR_ERR_TYPE_RB_FAILED &&
              (rec_p->err_type == FBE_XOR_ERR_TYPE_TS ||
               rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_TS ||
               rec_p->err_type == FBE_XOR_ERR_TYPE_WS ||
               rec_p->err_type == FBE_XOR_ERR_TYPE_BOGUS_WS) &&
             (region_p->pos_bitmask & bitmap_deg) &&
              ((err_adj_phys & bitmap_parity_phys) != 0) )
    {
        /* A rebuild failed because of a timestamp error on a parity drive.
         */
    }
    else if ( err_type == FBE_XOR_ERR_TYPE_TS &&
              (region_p->pos_bitmask & bitmap_parity_phys) &&
             ((fbe_raid_get_bitcount((bitmap_deg & ~bitmap_parity_phys))+
			   fbe_raid_get_bitcount(err_adj_phys & ~bitmap_parity_phys))>1) &&
             ((bitmap_deg & bitmap_parity_phys)|| (err_adj_phys & bitmap_parity_phys) ) )
    {
		/* R6:
		 * 1) We have a TS error on a parity drive.
         * 2) The other parity drive was faulted/injected with errors
         * 3) We have more than one error or faulted data drives
		 * => We didn't recognize it is a zeroed stripe, and found an unexpected error in an 
		 * unexpected position 
         */
    }
    else if ((      err_type == FBE_XOR_ERR_TYPE_WS  ||
                    err_type == FBE_XOR_ERR_TYPE_TS) &&
             rec_p->err_type == FBE_XOR_ERR_TYPE_CORRUPT_CRC &&
            !(fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p))))
    {
        /* A writestamp or timestamp error was report, which is match if a CORRUPT_CRC was injected.
         * We don't inject CORRUPT_CRC for RAID6, since it treats raid invalidated 
         * errors as "expected" and injecting one would cause a coherency error.
         */
    }    
    else
    {
        /* Skip the rest of the check if error types do not match. 
         */
        if (b_print)
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: region's err type 0x%x not valid with inject type: 0x%x\n", 
                __FUNCTION__, err_type, rec_p->err_type);
        }

        b_err_match = FBE_FALSE;
    }

    return b_err_match;
}
/****************************************
 * end fbe_logical_error_injection_rec_find_match_error_type()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_rec_find_match_correctable()
 ****************************************************************
 * @brief
 *  This function finds the matching error records to the given error
 *  region. Rewritten from fbe_logical_error_injection_rec_find_match. 
 * 

 * @param siots_p - pointer to a fbe_raid_siots_t struct
 * @param rec_p - pointer to an error record to examine.
 * @param region_p - pointer to an error region to match.
 * @param parms_p - pointer to paramters for multiple steps. 
 *
 * @return
 *  FBE_TRUE  - If    match error record(s) is found.
 *  FBE_FALSE - If no match error record(s) is found.
 *
 * @author
 *  05/03/06 - Created. SL
 *
 ****************************************************************/
static fbe_bool_t 
fbe_logical_error_injection_rec_find_match_correctable(fbe_raid_siots_t*    const siots_p,
                                  fbe_logical_error_injection_record_t* const rec_p,
                                  fbe_xor_error_region_t* const region_p,
                                  fbe_logical_error_injection_validate_parms_t*  const parms_p)
{
    const int b_print                  = parms_p->b_print;
    const fbe_u16_t bitmap_phys        = parms_p->bitmap_phys;
    const fbe_u16_t bitmap_deg         = parms_p->bitmap_deg;
    const fbe_u16_t bitmap_parity_log  = parms_p->bitmap_parity_log;
    const fbe_u16_t err_adj_phys       = parms_p->err_adj_phys;
    const fbe_bool_t correctable_tbl        = parms_p->correctable_tbl;
    const fbe_bool_t correctable_reg        = parms_p->correctable_reg;
    const fbe_xor_error_type_t err_type     = parms_p->err_type;
    const fbe_u16_t bitmap_parity_phys = parms_p->bitmap_parity_phys;
    fbe_raid_geometry_t *geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_bool_t  b_corr_match = FBE_TRUE;  /* By default, we assume a match. */

    if (b_print)
    {
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_WARNING,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: correctable_reg=0x%x, correctable_tbl=0x%x\n", __FUNCTION__, 
            correctable_reg, correctable_tbl);
    }


    if ( fbe_raid_geometry_is_raid6(geometry_p) &&
         correctable_reg == FBE_TRUE && 
         correctable_tbl == FBE_FALSE &&
         (region_p->error & FBE_XOR_ERR_TYPE_ZEROED) )
    {
        /* We are a RAID 6 and hit an error on a "zeroed"
         * region.  This error would normally be uncorrectable,
         * but in this case it is allowed to be correctable,
         * since we are just reconstructing zeros.
         */
    }
    else if ( correctable_reg == FBE_TRUE && 
              correctable_tbl == FBE_FALSE &&
             (((region_p->pos_bitmask & (~bitmap_parity_log)) == 0) || 
              ((region_p->pos_bitmask & (~bitmap_parity_phys)) == 0)))
    {
        /* We hit an error on a parity drive.
         * This error would normally be uncorrectable, 
         * but in this case it is allowed to be correctable,
         * since all errors on parity drives are correctable.
         */
    }
    else if ( (err_type == FBE_XOR_ERR_TYPE_TS) )
    {
        /* We hit a time stamp error, these can be both
         * correctable and uncorrectable so trust the checking we have done
         * up to now and let this one through as a match.
         */
    }
    else if ( fbe_raid_geometry_is_raid6(geometry_p) &&
              correctable_reg == FBE_FALSE && 
              correctable_tbl == FBE_TRUE && 
              err_type == FBE_XOR_ERR_TYPE_COH &&
              fbe_raid_get_bitcount(bitmap_deg | err_adj_phys) > 1 &&
              (region_p->pos_bitmask & bitmap_phys) )
    {
        /* We are raid 6 and injected a coherency error on this position.
         * If there is more than one error (bitmap_deg | rec_p->err_adj), 
         * then it's possible this coherency error is uncorrectable.
         */
    }
    else if ( fbe_raid_geometry_is_raid6(geometry_p) &&
              correctable_reg == FBE_FALSE && 
              correctable_tbl == FBE_TRUE && 
              (err_type == FBE_XOR_ERR_TYPE_COH || 
               err_type == FBE_XOR_ERR_TYPE_N_POC_COH || 
               err_type == FBE_XOR_ERR_TYPE_POC_COH || 
               err_type == FBE_XOR_ERR_TYPE_TS) &&
              fbe_raid_get_bitcount(bitmap_deg | err_adj_phys) > 1 &&
              fbe_logical_error_injection_is_coherency_err_type(rec_p) )
    {
        /* We are raid 6 and injected a coherency error on a different position.
         * If there is more than one error (bitmap_deg | rec_p->err_adj), 
         * then it's possible this coherency error is uncorrectable.
         */
    }
    else if ( fbe_raid_geometry_is_raid6(geometry_p) &&
              correctable_reg == FBE_FALSE && 
              correctable_tbl == FBE_TRUE && 
              fbe_raid_get_bitcount(bitmap_deg | err_adj_phys) > 1 &&
              fbe_logical_error_injection_coherency_adjacent(siots_p,
                                        rec_p, fbe_raid_siots_get_width(siots_p), 
                                        bitmap_deg) )
    {
        /* We are raid 6 and injected a coherency error on a different position.
         * If there is more than one error (bitmap_deg | rec_p->err_adj), 
         * then it's possible this error is uncorrectable.
         */
    }
    else if ( fbe_raid_geometry_is_raid6(geometry_p) &&
              correctable_reg == FBE_TRUE && 
              correctable_tbl == FBE_FALSE && 
              fbe_raid_get_bitcount(bitmap_deg | err_adj_phys) > 2 &&
              fbe_logical_error_injection_coherency_adjacent(siots_p,
                                        rec_p, fbe_raid_siots_get_width(siots_p), 
                                        bitmap_deg) )
    {
        /* We are raid 6 and injected a coherency error on a different position.
         * If there are more than 2 errors (bitmap_deg | err_adj_phys), 
         * and one of them is a coherency error, then we will not always
         * detect the coherency error so it will end up as correctable.
         */
    }
    else if ( fbe_raid_geometry_is_raid6(geometry_p) &&
              correctable_reg == FBE_TRUE && 
              correctable_tbl == FBE_FALSE && 
              rec_p->err_type == FBE_XOR_ERR_TYPE_1POC)
    {
        /* We are raid 6 and injected a parity of checksum error.  This can
         * manifest as a coherency error which can result in correctable errors
         * when uncorrectables are expected.
         */
    }
    else if ( fbe_raid_geometry_is_raid6(geometry_p) &&
              err_type == FBE_XOR_ERR_TYPE_RB_FAILED &&
              correctable_reg == FBE_FALSE && 
              correctable_tbl == FBE_TRUE )
    {
        /* All rebuild failed errors are uncorrectable,
         * even when there are fewer than 3 errors.
         */
    }
    else if (err_type == FBE_XOR_ERR_TYPE_INVALIDATED)
    {
        /* Invalidated errors may occur on any position, since
         * we could rebuild any position double degraded and
         * receive a single error on another drive.
         */
    }
    else if (err_type == FBE_XOR_ERR_TYPE_RAID_CRC)
    {
        /* RAID Invalidated errors may occur on any position, since
         * we could rebuild any position double degraded and
         * receive a single error on another drive.
         */
    }
    else if ((err_type == FBE_XOR_ERR_TYPE_CORRUPT_CRC) || 
             (err_type == FBE_XOR_ERR_TYPE_CORRUPT_DATA)) 
    {
        /* Corrupt CRC operation might have been execrised in past
         * and can be for any disk position.
         */
    }
    else if (correctable_reg == FBE_FALSE && 
             correctable_tbl == FBE_TRUE && 
             (region_p->error & FBE_XOR_ERR_TYPE_RB_INV_DATA) )
    {
        /* If we have an unexpected correctable, AND
         * there was ALSO a RAID error logged on this
         * same position, then it means we took a CRC error
         * and then reconstructed invalidated info on a position.
         * This most likely means that we took an uncorrectable in the
         * past due to degraded mode and later took a CRC error
         * on the invalidated data, reconstructing invalidated data.
         */
    }
    else if (correctable_reg == FBE_FALSE && 
             correctable_tbl == FBE_TRUE && 
             (region_p->error & FBE_XOR_ERR_TYPE_OTHERS_INVALIDATED) )
    {
        /* We have been conveyed that there is possibility of having
         * uncorrectable error although injected error was correctable
         * only in nature. So, we will pass the test.
         */
    }
    else if ( fbe_raid_geometry_is_raid6(geometry_p) &&
             correctable_reg == FBE_TRUE && 
             correctable_tbl == FBE_FALSE && 
              fbe_logical_error_injection_has_adjacent_type(siots_p, rec_p, fbe_logical_error_injection_is_stamp_err_type,
                                       ((1 << fbe_raid_siots_get_width(siots_p)) - 1)))
    {
        /* If we have a correctable region, but the table entry looks
         * uncorrectable, then we might have one or more adjacent
         * stamp errors, which were correctable.
         */
    }
    else if ( fbe_raid_geometry_is_triple_mirror(geometry_p) &&
             correctable_reg == FBE_TRUE && 
             correctable_tbl == FBE_FALSE && 
              fbe_logical_error_injection_has_adjacent_type(siots_p, rec_p, fbe_logical_error_injection_is_crc_err_type,
                                       ((1 << fbe_raid_siots_get_width(siots_p)) - 1)))
    {
        /* If we have a correctable region, but the table entry looks
         * uncorrectable.  A non-degraded 3-way mirror raid group can 
         * correct this error.  As long as there is an adjacent table 
         * entry, we allow a match against the uncorrectable table. 
         */
    }
    else if (correctable_reg != correctable_tbl)
    {
        b_corr_match = FBE_FALSE;

        if (b_print)
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: correctable-ness does not match\n", __FUNCTION__);
        }
    }

    return b_corr_match;
}
/****************************************
 * end fbe_logical_error_injection_rec_find_match_correctable()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_opt_size_not_one()
 ****************************************************************
 * @brief
 *  This function determines whether a FRU of interest is an ATA drive.
 *

 * @param siots_p - pointer to a fbe_raid_siots_t struct.
 * @param bitmap_phys - physical bitmap based on error table.
 *
 * @return
 *  FBE_TRUE  - If the FRU is     an ATA drive.
 *  FBE_FALSE - If the FRU is not an ATA drive.
 *
 * @author
 *  07/26/06 - Created. SL
 *
 ****************************************************************/
static fbe_bool_t fbe_logical_error_injection_opt_size_not_one(fbe_raid_siots_t* const siots_p,
                                             fbe_u16_t bitmap_phys)
{
    fbe_bool_t b_return;
    fbe_u32_t fru_position; /* fru physical position index */
    fbe_raid_geometry_t *raid_geometry_p = NULL; /* The raid group database for this SIOTS. */

    /* Get the raid group database for this SIOTS.
     */
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    
    /* Convert the fru's physical bitmap to a physical index.
     */ 
    fru_position = fbe_logical_error_injection_bitmap_to_position(bitmap_phys);

    /* If the optimla block size isn't (1) we assume that remap might have
     * extended the invalidation to the optimal block size.  These errors
     * should show up as RAID CRC errors.
     */
    if (raid_geometry_p->optimal_block_size != 1)
    {
        /* This FRU has an optimal block size of not one.
         */
        b_return = FBE_TRUE;
    }
    else
    {
        /* This FRU has an optimal block size of 1.
         */
        b_return = FBE_FALSE;
    }

    return b_return;
}
/****************************************
 * end fbe_logical_error_injection_opt_size_not_one()
 ****************************************/

/**********************************************************************
 * fbe_logical_error_injection_record_contain_region
 **********************************************************************
 * 
 * @brief
 *  This function checks whether the given error record contains the given 
 *  error region completely. The examples below show the possible cases.
 *
 *   Case1:    Case2:    Case3:    Case4:
 *   -   -     -   -     -         -     
 *   ^   ^     ^   ^     ^   -     ^   - 
 *   |   |     |   |     |   ^     |   ^
 *  Rec Reg   Rec Reg   Rec Reg   Rec Reg
 *   |   |     |   v     |   |     |   v
 *   v   v     v         v   v     v   - 
 *   -   -     -   -     -   -     -    
 *

 * @param lba_rec - Error record starting LBA.
 * @param cnt_rec - Error record size count.
 * @param lba_reg - Error region starting LBA.
 * @param cnt_reg - Error region size count.
 *
 * @return
 *  FBE_TRUE  - If the record          contains the region completely.
 *  FBE_FALSE - If the record does not contain  the region completely.
 *
 * @author
 *  07/26/06 - Created. SL
 *  
 *********************************************************************/
static fbe_bool_t fbe_logical_error_injection_record_contain_region(const fbe_lba_t lba_rec, 
                                    const fbe_block_count_t cnt_rec,
                                    const fbe_lba_t lba_reg, 
                                    const fbe_s32_t cnt_reg)
{
    /* for calculating end lba we need to adjust the block count 
     * by '1'.
     */
    const fbe_lba_t end_rec = lba_rec + cnt_rec - 1;
    const fbe_lba_t end_reg = lba_reg + cnt_reg - 1;
    fbe_bool_t b_contain;

    if (lba_rec <= lba_reg && end_rec >= end_reg)
    {
        b_contain = FBE_TRUE;
    }
    else
    {
        b_contain = FBE_FALSE;
    }

    return b_contain;
}
/****************************************
 * end fbe_logical_error_injection_record_contain_region()
 ****************************************/

/**********************************************************************
 * fbe_logical_error_injection_record_head_of_region
 **********************************************************************
 * 
 * @brief
 *  This function checks whether the given error record is at the head of 
 *  the given error region. The examples below show the possible cases.
 *
 *   Case1:             Case2:
 *   b_first == T       b_first = T || F
 *   -                  -   -      
 *   ^   -              ^   ^  
 *   |   ^              |   | 
 *  Rec Reg            Rec Reg
 *   v   |              v   | 
 *   -   v              -   V 
 *       -                  -     
 *

 * @param lba_rec - Error record starting LBA.
 * @param cnt_rec - Error record size count.
 * @param lba_reg - Error region starting LBA.
 * @param cnt_reg - Error region size count.
 * @param b_first_record - Whether should be treated as first record.
 * @param blks_overlap - The number of blocks that overlaps.
 *
 * @return
 *  FBE_TRUE  - If the record is     at the head of the region. 
 *  FBE_FALSE - If the record is not at the head of the region. 
 *
 * @author
 *  07/26/06 - Created. SL
 *  
 *********************************************************************/
static fbe_bool_t fbe_logical_error_injection_record_head_of_region(const fbe_lba_t lba_rec, 
                                    const fbe_block_count_t cnt_rec,
                                    const fbe_lba_t lba_reg, const fbe_s32_t cnt_reg,
                                    const fbe_bool_t b_first_record,
                                    fbe_s32_t* const blks_overlap_p)
{
    /* for calculating end lba we need to adjust the block count 
     * by '1'.
     */
    const fbe_lba_t end_rec = lba_rec + cnt_rec - 1;
    const fbe_lba_t end_reg = lba_reg + cnt_reg - 1;
    fbe_bool_t b_head_of_region = FBE_TRUE; /* By default, we assume a match. */

    if (b_first_record == FBE_TRUE)
    {
        if (lba_rec <= lba_reg && end_rec >= lba_reg && end_rec <= end_reg)
        {
            /* There is an overlap. Calculate the number of blocks overlapped.
             * also adjust the block count by '1' here.
             */
            *blks_overlap_p = (fbe_s32_t)(end_rec - lba_reg + 1);
        }
        else
        {
            b_head_of_region = FBE_FALSE;
        }
    }
    else
    {
        if (lba_rec == lba_reg && end_rec >= lba_reg && end_rec <= end_reg)
        {
            /* There is an overlap. Calculate the number of blocks overlapped.
             * also adjust the block count by '1' here.
             */
            *blks_overlap_p = (fbe_s32_t)(end_rec - lba_reg + 1);
        }
        else
        {
            b_head_of_region = FBE_FALSE;
        }
    }

    return b_head_of_region;
}
/****************************************
 * end fbe_logical_error_injection_record_head_of_region()
 ****************************************/


/**********************************************************************
 * fbe_logical_error_injection_set_up_validate_parms
 **********************************************************************
 * 
 * @brief
 *  This function sets up paramters used by multiple validation steps.
 *

 * @param siots_p - pointer to a fbe_raid_siots_t struct.
 * @param idx_record - index to an error record to examine.
 * @param idx_region - index to an error region to match.
 * @param b_print - whether partial results should be printed. 
 * @param parms_p - pointer to paramters for multiple steps. 
 *
 * @return
 *  NONE.
 *
 * @author
 *  07/26/06 - Created. SL
 *  
 *********************************************************************/
static fbe_status_t fbe_logical_error_injection_set_up_validate_parms(fbe_raid_siots_t*    const siots_p,
                                    fbe_lba_t stripe_start_lba,
                                    fbe_u32_t blocks,
                                    fbe_s32_t idx_record,
                                    fbe_s32_t idx_region,
                                    fbe_bool_t b_print,
                                    fbe_logical_error_injection_validate_parms_t* const parms_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    const fbe_xor_error_region_t* const region_p = 
        &siots_p->vcts_ptr->error_regions.regions_array[idx_region];
    fbe_logical_error_injection_record_t* rec_p = NULL;
    fbe_logical_error_injection_get_error_info(&rec_p, idx_record);

    /* Save the caller's KTRACE request. 
     */
    parms_p->b_print = b_print;
    
    /* pos_bitmap is logical for RAID 6 specific tables, and physical 
     * for the rest. Convert pos_bitmap to a physical bitmap if necessary.
     * The bitmap_phys_const is never altered during the entire validation 
     * process. On the contrary, bitmap_phys may be adjusted based on the
     * degraded-ness of the LUN, the error type, or other info.
     */    
    parms_p->bitmap_phys_const = fbe_logical_error_injection_is_table_for_all_types() ?
        rec_p->pos_bitmap :
        fbe_logical_error_injection_convert_bitmap_log2phys(siots_p, rec_p->pos_bitmap);
        
    /* pos_bitmap is logical for RAID 6 specific tables, and physical 
     * for the rest. Convert pos_bitmap to a physical bitmap if necessary.
     */    
    parms_p->bitmap_phys = parms_p->bitmap_phys_const;

    /* Get the physical bitmap of the degraded drive(s).
     */
    fbe_raid_siots_get_degraded_bitmask((fbe_raid_siots_t* const)siots_p, &parms_p->bitmap_deg);

    /* Setup the physical and logical parity bitmaps.
     */
    parms_p->bitmap_parity_phys = 1 << fbe_raid_siots_get_row_parity_pos(siots_p);

    if (fbe_logical_error_injection_is_table_for_all_types())
    {
        parms_p->bitmap_parity_log = 1 << fbe_raid_siots_get_row_parity_pos(siots_p);
    }
    else
    {
        parms_p->bitmap_parity_log = RG_R6_MASK_LOG_ROW;
    }

    /* If it is a RAID 6 LUN, we add the diagonal parity bitmap.
     */
    if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
    {
        parms_p->bitmap_parity_phys |= 1 << fbe_raid_siots_dp_pos(siots_p);

        if (fbe_logical_error_injection_is_table_for_all_types())
        {
            parms_p->bitmap_parity_log |= 1 << fbe_raid_siots_dp_pos(siots_p);
        }
        else
        {
            parms_p->bitmap_parity_log |= RG_R6_MASK_LOG_DIAG;
        }
    }

    /* Get the physical version of error adjacency. fbe_logical_error_injection_evaluate_err_adj
     * evaluates the given err_adj using the relevant error records and the 
     * error regions, and adjusts it if necessary. This is due to the fact 
     * that some errors are not always injected when their err_mode is 
     * random or transient. 
     */
     status = 
        fbe_logical_error_injection_evaluate_err_adj(siots_p, stripe_start_lba, blocks,
                                idx_record, idx_region,
                                parms_p->bitmap_parity_phys,
                                FBE_FALSE,
                                &(parms_p->err_adj_phys));
        if(LEI_COND(status != FBE_STATUS_OK))
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Bad status from evaluate_err_adj siots_p %p stripe_start_lba 0x%llx blocks 0x%x\n",
                __FUNCTION__, siots_p, (unsigned long long)stripe_start_lba,
		blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    /* Get the correctable-ness based on the error table and 
     * the degraded-ness of the LUN.
     */
    parms_p->correctable_tbl = 
        fbe_logical_error_injection_rec_evaluate(siots_p, idx_record, parms_p->err_adj_phys);

    /* Get the correctable-ness based on the error region.
     */
    parms_p->correctable_reg = 
        (region_p->error & FBE_XOR_ERR_TYPE_UNCORRECTABLE) == 0;

    /* Remove the FBE_XOR_ERR_TYPE_FLAG_MASK before comparison.
     */
    parms_p->err_type = region_p->error & ~FBE_XOR_ERR_TYPE_FLAG_MASK;

    /* Lets make a bitmask for the whole width of the strip so we can
     * use it below.
     */
    parms_p->bitmap_width = (1 << fbe_raid_siots_get_width(siots_p)) - 1;

    return status;
}
/****************************************
 * end fbe_logical_error_injection_set_up_validate_parms()
 ****************************************/

/**********************************************************************
 * fbe_logical_error_injection_evaluate_err_adj
 **********************************************************************
 * 
 * @brief
 *  This function evaluates the given err_adj using the relevant error 
 *  records and the error regions, and adjusts it if necessary. This is 
 *  due to the fact that some errors are not always injected when their 
 *  err_mode is random or transient. 
 *

 * @param siots_p - pointer to a fbe_raid_siots_t struct.
 * @param idx_record - index to an error record to examine.
 * @param idx_region - index to the error region being examined.
 * @param bitmap_parity_phys - physical parity bitmap.
 * @param b_print - FBE_TRUE to print diagnostic info.
 * @param err_adj_phys_p - err_adj using the relevant error .
 *
 * @return
 *  fbe_status_t.
 *
 * @author
 *  08/04/06 - Created. SL
 *  
 *********************************************************************/
static fbe_status_t fbe_logical_error_injection_evaluate_err_adj(fbe_raid_siots_t* const siots_p,
                                                                 fbe_lba_t stripe_start_lba,
                                                                 fbe_block_count_t blocks,
                                                                 const fbe_s32_t idx_record,
                                                                 const fbe_s32_t idx_region,
                                                                 const fbe_u16_t bitmap_parity_phys,
                                                                 const fbe_bool_t b_print,
                                                                 fbe_u16_t *err_adj_phys_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    const fbe_xor_error_regions_t* const regions_p = &siots_p->vcts_ptr->error_regions;
    const fbe_xor_error_region_t* const region_p = &regions_p->regions_array[idx_region];
    fbe_s32_t idx_first;
    fbe_s32_t idx_last;
    fbe_s32_t idx;
    fbe_s32_t width;
    fbe_lba_t error_region_lba = region_p->lba;
    fbe_block_count_t error_region_blocks = region_p->blocks;
    fbe_logical_error_injection_record_t* rec_p = NULL;
    fbe_logical_error_injection_get_error_info(&rec_p, idx_record);

    /* If we have an optimal block size, then round any media error that is
     * detected to the optimal block size, since strip mining does not occur below 
     * the optimal block size. 
     * During a rebuild also round if we see that a rebuild could have failed due 
     * to a media error. 
     */
    if ( ( ((region_p->error & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR) || 
           ((rec_p->err_type == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR) &&
            ((region_p->error & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_RB_FAILED)) ) )
    {
        
        status = fbe_raid_round_lba_blocks_to_optimal(&error_region_lba,
                                                      &error_region_blocks,
                                                      fbe_raid_siots_get_region_size(siots_p));
        if (status != FBE_STATUS_OK)
        {
            return status;
        }

        status = fbe_raid_round_lba_blocks_to_optimal(&stripe_start_lba,
                                                      &blocks,
                                                      fbe_raid_siots_get_region_size(siots_p));
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

    /* err_adj is logical for RAID 6 specific tables, and physical 
     * for the rest. Convert to a physical bitmap if necessary.
     */    
    *err_adj_phys_p = fbe_logical_error_injection_is_table_for_all_types() ?
        rec_p->err_adj :
        fbe_logical_error_injection_convert_bitmap_log2phys(siots_p, rec_p->err_adj);
    
    /* Remove the bitmap from the *err_adj_phys_p if disk is not available.
     */
    width = fbe_raid_siots_get_width(siots_p);
    *err_adj_phys_p = *err_adj_phys_p & ((1 << width) - 1);
   
    /* If only one bit in err_adj is set, there is no need for further 
     * evaluation and adjustment. 
     */
    if ((*err_adj_phys_p & (*err_adj_phys_p - 1)) == 0)
    {
        return status;
    }

    /* Get the range of the error records that match the given one.
     */
    status = fbe_logical_error_injection_find_matching_lba_range(idx_record, &idx_first, &idx_last);
    if(LEI_COND(status != FBE_STATUS_OK))
    {
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Bad status from display_fruts_blocks: idx_record 0x%x idx_first 0x%x idx_last 0x%x\n",
            __FUNCTION__, idx_record, idx_first, idx_last);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Loop over all relevant error records. 
     */
    for (idx = idx_first; idx <= idx_last; ++idx)
    {
        fbe_u16_t bitmap_phys;
        fbe_s32_t idx_reg;

        /* For this err_mode type, the error is always injected. 
         * Thus there is no need to make any adjustment to err_adj.
         * We always treat media errors as transient, since they
         * are not always injected for the entire range due to
         * the DH only injecting the first lba and randomly injecting
         * the rest of the lbas.
         */
        fbe_logical_error_injection_get_error_info(&rec_p, idx);
        if (rec_p->err_mode == FBE_LOGICAL_ERROR_INJECTION_MODE_ALWAYS &&
            rec_p->err_type != FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR &&
            rec_p->err_type != FBE_XOR_ERR_TYPE_RND_MEDIA_ERR &&
            rec_p->err_type != FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR)
        {
            continue;
        }

        /* pos_bitmap is logical for RAID 6 specific tables, and physical 
         * for the rest. Convert to a physical bitmap if necessary.
         */ 
        bitmap_phys = fbe_logical_error_injection_is_table_for_all_types() ?     
            rec_p->pos_bitmap :
            fbe_logical_error_injection_convert_bitmap_log2phys(siots_p, rec_p->pos_bitmap);

        /* For the rest of the err_mode types, use the error regions 
         * to determine whether an error has been injected. 
         */
        for (idx_reg = 0; idx_reg < regions_p->next_region_index; ++idx_reg)
        {
            const fbe_xor_error_region_t* const reg_p = &regions_p->regions_array[idx_reg];
            fbe_lba_t region_lba;
            fbe_xor_error_type_t region_error_type = reg_p->error & FBE_XOR_ERR_TYPE_MASK;
            fbe_lba_t record_rounded_lba = rec_p->lba;
            fbe_block_count_t record_rounded_blocks = rec_p->blocks;

            /* Skip the error region if it does not overlap with 
             * the error region being examined. 
             */
            if (fbe_logical_error_injection_overlap(reg_p->lba,    reg_p->blocks, 
                                                    error_region_lba, error_region_blocks) == FBE_FALSE)
            {
                continue;
            }

            /* Normalize this error region lba to fit within the table
             * so that we can compare the two values below.
             */
            region_lba = fbe_logical_error_injection_get_table_lba(siots_p, reg_p->lba, reg_p->blocks);
            
            /* Skip the error region if it does not overlap with 
             * the error region being examined. 
             */
            if (fbe_logical_error_injection_overlap(region_lba, reg_p->blocks, 
                                                    stripe_start_lba, blocks ) == FBE_FALSE)
            {
                continue;
            }

            /* CRC style errors always occur on the position they were
             * injected on.
             */
            if ( region_error_type == FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR ||
                 region_error_type == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR ||
                 region_error_type == FBE_XOR_ERR_TYPE_RND_MEDIA_ERR ||
                 region_error_type == FBE_XOR_ERR_TYPE_CRC ||
                 region_error_type == FBE_XOR_ERR_TYPE_KLOND_CRC ||
                 region_error_type == FBE_XOR_ERR_TYPE_DH_CRC ||
                 region_error_type == FBE_XOR_ERR_TYPE_RAID_CRC ||
                 region_error_type == FBE_XOR_ERR_TYPE_CORRUPT_CRC )
            {
                if ((reg_p->pos_bitmask & bitmap_phys ) == 0)
                {
                    continue;
                }
            }
            /* Because errors injected to data positions may be attributed to 
             * parity positions, in some cases,
             * it is acceptable if there is an overlap between 
             * the error region bitmap with either the error record bitmap or
             * the parity position bitmap. 
             */
            else if ((reg_p->pos_bitmask & (bitmap_phys | bitmap_parity_phys)) == 0)
            {
                continue;
            }

            if (b_print)
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_WARNING,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: idx=0x%x, lba=0x%llX, pos=0x%x, err=0x%x\n",
                    __FUNCTION__, idx, (unsigned long long)rec_p->lba,
		    rec_p->pos_bitmap, rec_p->err_type);
            }

            status = fbe_raid_round_lba_blocks_to_optimal(&record_rounded_lba,
                                                          &record_rounded_blocks,
                                                          fbe_raid_siots_get_region_size(siots_p));
            if (status != FBE_STATUS_OK)
            {
                return status;
            }
            
            /* Now we check whether the two LBA ranges also overlap.
             * If we're a raid 6, also check that this is not a RAID CRC error.
             * A Raid CRC error on Raid 6 means that previously an
             * uncorrectable was encountered here, but does not mean
             * an error was injected this time.
             */
            if (fbe_logical_error_injection_overlap(region_lba, reg_p->blocks, 
                                                    record_rounded_lba, record_rounded_blocks))
            {
                /* Both bitmaps and ranges overlap. We have a match.
                 */
                if (b_print)
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: match: reg_p->lba: 0x%llx reg_p->bl: 0x%x reg_p->bitm: 0x%x reg_p->error: 0x%x\n",
                        __FUNCTION__, (unsigned long long)reg_p->lba,
			reg_p->blocks, reg_p->pos_bitmask, reg_p->error);
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                        "%s: region_lba: 0x%llx\n", __FUNCTION__,
			(unsigned long long)region_lba);
                }
                break;
            }
            else
            {
                if (b_print)
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                            "%s no match: reg_p->lba: 0x%llx reg_p->bl: 0x%x reg_p->bitm: 0x%x reg_p->error: 0x%x\n",
                            __FUNCTION__, (unsigned long long)reg_p->lba,
			    reg_p->blocks, reg_p->pos_bitmask, reg_p->error);
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                        "%s: region_lba: 0x%llx\n",__FUNCTION__,
			(unsigned long long)region_lba);
                }
            }
        }

        /* When the two indices are equal, it means we did not find 
         * any matching error region. In this case, we assume no 
         * error was injected. Thus we remove that bit from err_adj. 
         */
        if (idx_reg == regions_p->next_region_index)
        {
            *err_adj_phys_p &= ~bitmap_phys;
        }
    }

    return status;
}
/****************************************
 * end fbe_logical_error_injection_evaluate_err_adj()
 ****************************************/

/**********************************************************************
 * fbe_logical_error_injection_find_matching_lba_range
 **********************************************************************
 * 
 * @brief
 *  This function searches for the range of error records that matches 
 *  the LBA of the given one. 
 *
 * @param idx_record - index to an error record to examine.
 * @param idx_first_p - pointer to the first index of the matching range.
 * @param idx_last_p - pointer to the last  index of the matching range. 
 *
 * @return
 *  fbe_status_t.
 *
 * @author
 *  08/04/06 - Created. SL
 *  
 *********************************************************************/
static fbe_status_t fbe_logical_error_injection_find_matching_lba_range(const fbe_s32_t idx_record, 
                                          fbe_s32_t* const idx_first_p, 
                                          fbe_s32_t* const idx_last_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_record_t* record_p = NULL;
    fbe_s32_t idx;
    fbe_logical_error_injection_get_error_info(&record_p, idx_record);

    /* Check the range of the given error record index.
     */
    if(LEI_COND((idx_record < 0) || 
                 (idx_record >= FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS)) )
    {
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "find_matching_lba_range: idx_record 0x%x is either 0 or >= 0x%x(MAX_RECORDS)\n",
            idx_record, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *idx_first_p = *idx_last_p = idx_record;

    /* Loop from the given record to the beginning of the error table.
     */
    for (idx = idx_record; idx >= 0; --idx)
    {
        fbe_logical_error_injection_record_t* rec_p = NULL;
        
        fbe_logical_error_injection_get_error_info(&rec_p, idx);
        /* If error records overlap, they must overlap completely. 
         */
        if (rec_p->lba == record_p->lba)
        {
            if(LEI_COND(rec_p->blocks  != record_p->blocks || 
                   rec_p->err_adj != record_p->err_adj) )
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "find_matching_lba_range: rec_p->blocks 0x%llx != record_p->blocks 0x%llx Or"
                    " rec_p->err_adj 0x%x != record_p->err_adj 0x%x\n",
                    (unsigned long long)rec_p->blocks,
		    (unsigned long long)record_p->blocks, 
                    rec_p->err_adj, record_p->err_adj);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            *idx_first_p = idx;
        }
        else
        {
            /* Found the first record that does not match. 
             */
            break;
        }
    }

    /* Loop from the given record to the end of the error table.
     */
    for (idx = idx_record; idx <= FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS - 1; ++idx)
    {
        fbe_logical_error_injection_record_t* rec_p = NULL;

        fbe_logical_error_injection_get_error_info(&rec_p, idx);
        /* FBE_XOR_ERR_TYPE_NONE means the end of valid error records.
         */
        if (rec_p->err_type != FBE_XOR_ERR_TYPE_NONE && 
            rec_p->lba      == record_p->lba)
        {
            if(LEI_COND(rec_p->blocks  != record_p->blocks || 
                   rec_p->err_adj != record_p->err_adj) )
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "find_matching_lba_range: rec_p->blocks 0x%llx != record_p->blocks 0x%llx Or"
                    " rec_p->err_adj 0x%x != record_p->err_adj 0x%x\n",
                    (unsigned long long)rec_p->blocks,
		    (unsigned long long)record_p->blocks,
                    rec_p->err_adj, record_p->err_adj);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            *idx_last_p = idx;
        }
        else
        {
            /* Found the first record that does not match. 
             */
            break;
        }
    }

    return status;
}
/****************************************
 * end fbe_logical_error_injection_find_matching_lba_range()
 ****************************************/

/**********************************************************************
 * fbe_logical_error_injection_correction_validate()
 **********************************************************************
 * 
 * @brief
 *  This function validates the results of error correction. 
 *
 * @param siots_p - pointer to a fbe_raid_siots_t struct 
 *                      
 * @return fbe_status_t
 *  FBE_STATUS_OK  - Results are   expected.
 *  FBE_STATUS_GENERIC_FAILURE - Results are unexpected.
 *
 * @author
 *  05/03/06 - Created. SL
 *  
 *********************************************************************/
fbe_status_t fbe_logical_error_injection_correction_validate(fbe_raid_siots_t* const siots_p,
                                                             fbe_xor_error_region_t **unmatched_error_region_p)
{
    fbe_xor_error_regions_t* regs_xor_p = &siots_p->vcts_ptr->error_regions;
    fbe_s32_t idx;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    fbe_logical_error_injection_object_t *object_p = NULL;
    fbe_bool_t b_enabled;

    /* Fetch the object ptr.
     */
    object_p = fbe_logical_error_injection_find_object(object_id);

    if (object_p == NULL)
    {
        /* We are not injecting for this object, so not sure why we are here.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                                  "%s: object_id: 0x%x was not found\n", __FUNCTION__, object_id);
        return FBE_STATUS_OK;
    }

    /* If error injection is not enabled, do not perform validation.
     */
    fbe_logical_error_injection_get_enable(&b_enabled);
    if (b_enabled == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }

    /* Loop over each of the fbe_xor_error_region_t from the XOR driver's 
     * fbe_xor_error_regions_t.
     */
    for (idx = 0; idx < regs_xor_p->next_region_index; ++idx)
    {
        fbe_xor_error_region_t* reg_xor_p = &regs_xor_p->regions_array[idx];
        /* Search through the current error table for a match.
         */
        if (fbe_logical_error_injection_rec_find_match(siots_p, idx, FBE_FALSE) == FBE_FALSE)
        {

            /* Determine where this region matches up to the table.
             * This allows us to easily determine which error table records (if any)
             * correspond to this error. 
             */
            fbe_lba_t table_lba = fbe_logical_error_injection_get_table_lba(siots_p, reg_xor_p->lba, reg_xor_p->blocks );

            fbe_logical_error_injection_lock();
            fbe_logical_error_injection_object_lock(object_p);
            fbe_logical_error_injection_inc_num_failed_validations();
            fbe_logical_error_injection_object_unlock(object_p);
            fbe_logical_error_injection_unlock();

            /* The results were unexpected.
             * First disable rderr, so we do not get any further failures.
             * This is necessary since we saved our context in the
             * "unmatched record" field of the RG_ERROR_SIM_GB and do not
             * want it to become overwritten.
             *
             * Now retry the reads so we will have something to 
             * display in the ktrace.
             */
            fbe_logical_error_injection_disable();
            if (fbe_logical_error_injection_is_extern_hard_media_error_present(siots_p))
            {
                /* There is a HARD MEDIA error and it is not injected by rderr.
                 * This might have caused the uncorrectable type mismatch here.
                 * We should ignore this as HARD MEDIA errors can occur on real
                 * disk -OR- it might have got injected at lower level driver,
                 * like Mirror Driver or Disk Driver.
                 */
                fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                         "%s: obj: 0x%x siots_p: %p Mismatch - HARD MEDIA ERROR. \n", 
                        __FUNCTION__, object_id, siots_p);
                fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                         " error region: lba=0x%llx, blk=0x%x, pos=0x%x, err=0x%x table_lba:0x%llx\n",
                         (unsigned long long)reg_xor_p->lba, reg_xor_p->blocks,
			 reg_xor_p->pos_bitmask, reg_xor_p->error,
			 (unsigned long long)table_lba );
            }

            /* There is no external hard media error, just proceed as usual 
             */
            else
            {
                /* Every error region must have at least one matching record.
                 * If no match is found, we print out the error region of 
                 * interest, then we search for and print out any partially 
                 * matching error record(s) to assist debugging.
                 */

                fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                         "error region: lba=0x%llx, blk=0x%x, pos=0x%x, err=0x%x table_lba:0x%llx\n",
                         (unsigned long long)reg_xor_p->lba, reg_xor_p->blocks,
			 reg_xor_p->pos_bitmask, reg_xor_p->error,
			 (unsigned long long)table_lba );

                fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                         "%s: object_id: 0x%x IOTS: %p\n", 
                        __FUNCTION__, object_id, fbe_raid_siots_get_iots(siots_p) );

                fbe_logical_error_injection_rec_find_match(siots_p, idx, FBE_TRUE);

                /* Save the unmatched record for use in the following states.
                 */
                fbe_logical_error_injection_set_unmatched_record( reg_xor_p );
                
                status = FBE_STATUS_GENERIC_FAILURE;
                *unmatched_error_region_p = reg_xor_p;
                /* Here instead of panicking, we will print the error.
                 */
                fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                        "%s: Unmatched record: %p and reg_xor_p : 0x%p.\n", __FUNCTION__,
                         unmatched_error_region_p, reg_xor_p);
                break;
            }/*end - else if(reg_xor_p->error & FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR)*/
        }/* end - if (fbe_logical_error_injection_rec_find_match(siots_p, idx, FBE_FALSE) == FBE_FALSE)*/
        else
        {
            /* Validation passed, increment counters.
             */
            fbe_logical_error_injection_lock();
            fbe_logical_error_injection_object_lock(object_p);
            fbe_logical_error_injection_inc_num_validations();
            fbe_logical_error_injection_object_inc_num_validations(object_p);
            if (reg_xor_p->error & FBE_XOR_ERR_TYPE_UNCORRECTABLE)
            {
                fbe_logical_error_injection_inc_uncorrectable_errors_detected();
            }
            else
            {
                fbe_logical_error_injection_inc_correctable_errors_detected();
            }
            fbe_logical_error_injection_object_unlock(object_p);
            fbe_logical_error_injection_unlock();
        }
    } /* end of for (idx...) */

    return status;
}
/****************************************
 * end fbe_logical_error_injection_correction_validate()
 ****************************************/
/************************************************************
 *  fbe_logical_error_injection_handle_validation_failure()
 ************************************************************
 *
 * @brief
 *  There has been a validation failure.
 *  This means that the error we encountered was unexpected
 *  based on the error table currently in use.
 *  Display information about the current read fruts and
 *  display all the blocks in the read fruts.
 *  Then retry the reads so we get a fresh picture of
 *  how the data appears on disk.
 *
 * PARAMTERS:
 * @param siots_p - Current siots.
 *
 * @return
 *  None.
 *
 * History:
 *  04/05/07:  Rob Foley -- Created.
 *
 ************************************************************/
static void fbe_logical_error_injection_handle_validation_failure( fbe_raid_siots_t *const siots_p )
{
    fbe_u16_t active_fruts_bitmap; /* Bitmap of non-NOP opcode fruts. */
    fbe_xor_error_region_t *unmatched_rec_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_logical_error_injection_get_unmatched_record(&unmatched_rec_p);

    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s:  Displaying sectors from pba 0x%llx "
                        "on object: 0x%x \n",
                        __FUNCTION__, (unsigned long long)unmatched_rec_p->lba,
			object_id);

    /* First display all the read blocks.
     */
    status = fbe_logical_error_injection_display_fruts_blocks( read_fruts_ptr,
                                                      unmatched_rec_p->lba );
    if(LEI_COND(status != FBE_STATUS_OK))
    {
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Bad status from display_fruts_blocks "
            "read_fruts_ptr %p unmatched_rec_p->lba 0x%llx\n",
            __FUNCTION__, read_fruts_ptr,
	    (unsigned long long)unmatched_rec_p->lba);
    }
    fbe_logical_error_injection_service_trace(
        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
        "%s: re-reading pba 0x%llx before panic\n",
        __FUNCTION__, (unsigned long long)unmatched_rec_p->lba );

    /* The results were unexpected.
     * First disable rderr, so we do not get any further failures.
     * This is necessary since we saved our context in the
     * "unmatched record" field of the RG_ERROR_SIM_GB and do not
     * want it to become overwritten.
     *
     * Now retry the reads so we will have something to 
     * display in the ktrace.
     */
    fbe_logical_error_injection_disable();
    fbe_raid_fruts_get_chain_bitmap( read_fruts_ptr, &active_fruts_bitmap );

    /* Update the wait count for all the reads we will be retrying.
     */
    siots_p->wait_count = fbe_raid_get_bitcount( active_fruts_bitmap );

    if ( siots_p->wait_count > 0 )
    {
        /* Send out retries for all reads.
         */
        (void) fbe_raid_fruts_for_each( 
            active_fruts_bitmap,  /* Retry for all non-nop fruts. */
            read_fruts_ptr,  /* Fruts chain to retry for. */
            fbe_raid_fruts_retry, /* Function to call for each fruts. */
            (fbe_u32_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* Opcode to retry as. */
            (fbe_u64_t) fbe_raid_fruts_evaluate); /* New evaluate fn. */
    }
    else
    {
        /* For some reason there was nothing to retry here, so
         * instead of retrying we will just fbe_panic now.
         */
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
            "RAID:  Log an Error due to rderr validation mismatch. "
            "see above ktrace for more details\n" );
    }
    
    /* Transition to our final state where we will display the information read.
     */
    fbe_raid_siots_transition(siots_p, fbe_logical_error_injection_retry_finished);

    return;
}
/*******************************************
 * end fbe_logical_error_injection_handle_validation_failure()
 *******************************************/
/************************************************************
 *  fbe_logical_error_injection_retry_finished()
 ************************************************************
 *
 * @brief
 *  The retry of errored blocks has finished.
 *  This allows us to display the current state of
 *  these blocks on disk.
 *
 * PARAMTERS:
 * @param siots_p - Current siots.
 *
 * @return
 *  FBE_RAID_STATE_STATUS_DONE
 *
 * History:
 *  04/04/07:  Rob Foley -- Created.
 *
 ************************************************************/

static fbe_raid_state_status_t fbe_logical_error_injection_retry_finished(fbe_raid_siots_t * siots_p)
{
    fbe_xor_error_region_t *unmatched_rec_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    fbe_logical_error_injection_get_unmatched_record(&unmatched_rec_p);
    /* The blocks we display are put in the traffic buffer since it is
     * unused and so that the standard buffer is not filled up.
     */
    fbe_logical_error_injection_service_trace(
        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
             "%s:  Displaying sectors newly read from pba 0x%llx "
             "on object_id: 0x%x\n",
             __FUNCTION__, (unsigned long long)unmatched_rec_p->lba, object_id);

    /* Display the set of blocks that we just retried the reads for.
     */
    status = fbe_logical_error_injection_display_fruts_blocks( read_fruts_ptr, unmatched_rec_p->lba );
    if(LEI_COND(status != FBE_STATUS_OK))
    {
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Bad status from display_fruts_blocks"
            "read_fruts_ptr %p unmatched_rec_p->lba 0x%llx\n",
            __FUNCTION__, read_fruts_ptr,
	    (unsigned long long)unmatched_rec_p->lba);
    }
    
    /* The validation retry is finished, we now fbe_panic and display the record that
     * was unmatched in validation.
     */
    fbe_logical_error_injection_service_trace(
        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s:  Validation retry "
            "lba: 0x%llx blocks: 0x%x pos_bitmask: 0x%x error: 0x%x\n",
            __FUNCTION__, (unsigned long long)unmatched_rec_p->lba,
	    unmatched_rec_p->blocks, unmatched_rec_p->pos_bitmask,
	    unmatched_rec_p->error );

    /* Panic - Something is wrong in logical Error injection.
     * Logging CRITICAL ERROR will take care of panic. 
     */
    fbe_logical_error_injection_service_trace(
        FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s:  Panic due to rderr validation mismatch. "
            "see above ktrace for more details\n", __FUNCTION__ );

    /* return is meaningless here since we are taking a PANIC. 
     * Disable or Remove this return when the PANIC is enabled in
     * trace_to_ring().
     */
    return FBE_RAID_STATE_STATUS_DONE;
}
/*************************************************
 * end of fbe_logical_error_injection_retry_finished()
 *************************************************/

/************************************************************
 *  fbe_logical_error_injection_display_fruts_blocks()
 ************************************************************
 *
 * @brief
 *   Display the fruts blocks for a given fruts chain
 *   and a given LBA.  All blocks in the chain for this
 *   lba will be displayed.
 *
 * PARAMTERS:
 * @param input_fruts_p - Fruts to display range for.
 * @param lba - Lba to display for.
 *
 * @return
 *  fbe_status_t.
 *
 * History:
 *  04/04/07:  Rob Foley -- Created.
 *
 ************************************************************/

static fbe_status_t fbe_logical_error_injection_display_fruts_blocks(fbe_raid_fruts_t * const input_fruts_p, 
                                                             const fbe_lba_t lba)
{
    fbe_raid_fruts_t *fruts_p;
    fbe_status_t status = FBE_STATUS_OK;
    /* Loop over all fruts, and display each block that matches the above LBA.
     */
    for (fruts_p = input_fruts_p;
         fruts_p != NULL;
         fruts_p = fbe_raid_fruts_get_next(fruts_p))
    {
        fbe_sg_element_t *fruts_sg_p = fbe_raid_fruts_return_sg_ptr(fruts_p);
        fbe_u32_t block_offset;
        
        /* Only display blocks that match the input lba.
         */
        if ( lba >= fruts_p->lba &&
             (lba - fruts_p->lba) <= fruts_p->blocks )
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: fruts lba: 0x%llx blocks: 0x%llx op: %d position: %d\n",
                __FUNCTION__, (unsigned long long)fruts_p->lba,
		(unsigned long long)fruts_p->blocks, fruts_p->opcode,
		fruts_p->position);

            /* Only display blocks that we read successfully.
             */
            if ( fruts_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS && fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID )
            {
                PVOID reference_p = NULL;
                fbe_sector_t *ref_sect_p = NULL;
                fbe_sector_t *sector_p;

                /* Adjust the sg ptr to point at the sg element with this sector.
                 */
                block_offset = fbe_raid_get_sg_ptr_offset( &fruts_sg_p,
                                                       (fbe_u32_t)(lba - fruts_p->lba) );

                /* Next adjust our sector ptr to point to the memory in this sg
                 * element with this sector.
                 */
                sector_p = (fbe_sector_t *)fruts_sg_p->address;
                if(LEI_COND( fruts_sg_p->count < (block_offset * FBE_BE_BYTES_PER_BLOCK) ) )
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: fruts_sg_p->count 0x%llx < 0xllx(block_offset 0x%llx * 0x%x )\n",
                        __FUNCTION__, (unsigned long long)fruts_sg_p->count,
			(unsigned long long)block_offset,
                        FBE_BE_BYTES_PER_BLOCK);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                sector_p += block_offset;
            
                /* Next, map in the memory
                 */
                reference_p = (FBE_LOGICAL_ERROR_INJECTION_VIRTUAL_ADDRESS_AVAIL(fruts_sg_p))
                    ?(PVOID)sector_p:
                    FBE_LOGICAL_ERROR_INJECTION_MAP_MEMORY(((UINT_PTR)sector_p), FBE_BE_BYTES_PER_BLOCK);
                ref_sect_p = (fbe_sector_t *) reference_p;

                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: mem: 0x%p mapped: 0x%p \n", 
                    __FUNCTION__, sector_p, ref_sect_p);
            
                /* Next display the sector.
                 */
                status = fbe_xor_lib_sector_trace_raid_sector(ref_sect_p);
                if (status != FBE_STATUS_OK)
                {
                    return status;
                }
                
                /* Finally, map out the sector.
                 */
                if( !FBE_LOGICAL_ERROR_INJECTION_VIRTUAL_ADDRESS_AVAIL(fruts_sg_p) )
                {
                    FBE_LOGICAL_ERROR_INJECTION_UNMAP_MEMORY(reference_p, FBE_BE_BYTES_PER_BLOCK);
                }
            }/* display successfully read blocks. */
        }
        else
        {
            /* We expected our error area to fall within the fruts, but it didn't
             * so we have no choice but to display this error.
             */
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Skip fruts 0x%p lba: 0x%llx blocks: 0x%llx unmatched lba: 0x%llx\n",
                __FUNCTION__, fruts_p, (unsigned long long)fruts_p->lba,
		(unsigned long long)fruts_p->blocks,
		(unsigned long long)lba);
        }
    }
    return status;
}
/****************************************
 * end of fbe_logical_error_injection_display_fruts_blocks() 
 ****************************************/
 
/****************************************************************
 * fbe_logical_error_injection_is_ts_err_type()
 ****************************************************************
 * @brief
 *  This function determines if the input error record is a
 *  time stamp error.
 * 

 * @param rec_p - Error record to check.
 *
 * @return
 *  FBE_TRUE  - If error rec is     a time stamp error.
 *  FBE_FALSE - If error rec is not a time stamp error.
 *
 * @author
 *  03/26/07 - Created. CLC
 *
 ****************************************************************/
static fbe_bool_t fbe_logical_error_injection_is_ts_err_type( const fbe_logical_error_injection_record_t* const rec_p )
{
    fbe_bool_t b_return = FBE_FALSE;
    
    if (rec_p->err_type == FBE_XOR_ERR_TYPE_TS)
    {
        /* Found TS.
         */
        b_return = FBE_TRUE;
    }
    return b_return;
}
/****************************************
 * end fbe_logical_error_injection_is_ts_err_type()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_is_stamp_err_type()
 ****************************************************************
 * @brief
 *  This function determines if the input error record is a
 *  stamp error.
 * 

 * @param rec_p - Error record to check.
 *
 * @return
 *  FBE_TRUE  - If error rec is     a stamp error.
 *  FBE_FALSE - If error rec is not a stamp error.
 *
 * @author
 *  08/03/07 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_bool_t fbe_logical_error_injection_is_stamp_err_type( const fbe_logical_error_injection_record_t* const rec_p )
{
    fbe_bool_t b_return = FBE_FALSE;
    
    if (rec_p->err_type == FBE_XOR_ERR_TYPE_TS ||
        rec_p->err_type == FBE_XOR_ERR_TYPE_WS )
    {
        /* Found Stamp Err.
         */
        b_return = FBE_TRUE;
    }
    return b_return;
}
/****************************************
 * end fbe_logical_error_injection_is_stamp_err_type()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_is_crc_err_type()
 ****************************************************************
 * @brief
 *  This function determines if the input error record is a
 *  crc error or not
 * 

 * @param rec_p - Error record to check.
 *
 * @return
 *  FBE_TRUE  - If error rec is     a crc error.
 *  FBE_FALSE - If error rec is not a crc error.
 *
 * @author
 *  08/19/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_bool_t fbe_logical_error_injection_is_crc_err_type( const fbe_logical_error_injection_record_t* const rec_p )
{
    fbe_bool_t b_return = FBE_FALSE;
    
    if ((rec_p->err_type == FBE_XOR_ERR_TYPE_CRC)            ||
        (rec_p->err_type == FBE_XOR_ERR_TYPE_KLOND_CRC)      ||
        (rec_p->err_type == FBE_XOR_ERR_TYPE_DH_CRC)         ||
        (rec_p->err_type == FBE_XOR_ERR_TYPE_INVALIDATED)    ||
        (rec_p->err_type == FBE_XOR_ERR_TYPE_RAID_CRC)       ||
        (rec_p->err_type == FBE_XOR_ERR_TYPE_CORRUPT_CRC)    ||
        (rec_p->err_type == FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC) ||
        (rec_p->err_type == FBE_XOR_ERR_TYPE_MULTI_BIT_CRC)  ||
        (rec_p->err_type == FBE_XOR_ERR_TYPE_MULTI_BIT_WITH_LBA_STAMP) )
    {
        /* Found CRC.
         */
        b_return = FBE_TRUE;
    }
    return b_return;
}
/***************************************************
 * end fbe_logical_error_injection_is_crc_err_type()
 ***************************************************/

/*****************************************************************************
 *          fbe_logical_error_injection_is_downstream_injection_enabled()
 ***************************************************************************** 
 * 
 * @brief   There is a case where a virtual drive detects previously
 *          invalidated sectors (`copy invalidated').  Normally the virtual
 *          drive is in `pass-thru' mode and therefore will not check and/or
 *          fail the request with `hard error.  But if the virtual drive is
 *          in mirror mode, it can detect and report `hard media error'.
 * 
 * @param   block_operation_p - Pointer to block operation to inject
 *
 * @return  bool - FBE_TRUE - The virtual drive has error injection enabled.
 * 
 * @note    Currently we don't checked the virtual drive mode we simply check
 *          if error injection is enabled or not.
 *
 * @author
 *  05/07/2012  Ron Proulx   - Created.
 *
 *****************************************************************************/
static fbe_bool_t fbe_logical_error_injection_is_downstream_injection_enabled(fbe_payload_block_operation_t *block_operation_p)
{
    fbe_block_edge_t   *block_edge_p = NULL;
    fbe_object_id_t     object_id = 0;
    fbe_logical_error_injection_object_t *object_p = NULL;

    /* Get the downstream object from the edge.
     */
    fbe_payload_block_get_block_edge(block_operation_p, &block_edge_p);
    object_id = block_edge_p->base_edge.server_id;

    /* Fetch the object ptr.
     */
    object_p = fbe_logical_error_injection_find_object(object_id);
    if (object_p == NULL)
    {
        /* This is the typically case where the downstream object doesn't have
         * error injection enabled.  Return False.
         */
        return FBE_FALSE;
    }

    /* Display some debug information.
     */
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                   "LEI: HARD MEDIA: obj: 0x%x enb: %d injected cnt: 0x%llx op: %d lba: 0x%llx blks: 0x%llx b/q:%d/%d\n",
                   object_id, object_p->b_enabled, (unsigned long long)object_p->num_errors_injected, block_operation_p->block_opcode, 
                   (unsigned long long)block_operation_p->lba, (unsigned long long)block_operation_p->block_count,
                   block_operation_p->status, block_operation_p->status_qualifier);

    /* Return if the object is enabled or not.
     */
    return fbe_logical_error_injection_object_is_enabled(object_p);
}
/*******************************************************************
 * end fbe_logical_error_injection_is_downstream_injection_enabled()
 *******************************************************************/

/**************************************************************************
 * fbe_logical_error_injection_is_extern_hard_media_error_present()
 **************************************************************************
 * @brief
 *  This function determines if there is any hard media error present 
 *  other than injected by rderr. This is called when we find rderr
 *  mismatch.
 * 

 * @param siots_p - fbe_raid_siots_t pointer 
 *
 * @return
 *  FBE_TRUE  - If error external hard media error is present
 *  FBE_FALSE - If error external hard media error is not present
 *
 * @author
 *  03/20/09 - Created. Mahesh Agarkar
 *
 *************************************************************************/
static fbe_bool_t fbe_logical_error_injection_is_extern_hard_media_error_present( fbe_raid_siots_t * siots_p)
{
    fbe_raid_fruts_t       *fruts_p;
    fbe_raid_fruts_t       *read_fruts_ptr = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_object_id_t         object_id;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    if(LEI_COND(read_fruts_ptr == NULL))
    {
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: read_fruts_ptr is NULL, siots_p->start_lba 0x%llx siots_p->xfer_count 0x%llx\n",
            __FUNCTION__, (unsigned long long)siots_p->start_lba,
	    (unsigned long long)siots_p->xfer_count);
        return FBE_FALSE;
    }

    /* we will check all reads to the disks done during this request
     */
    for (fruts_p = read_fruts_ptr;
         fruts_p != NULL;
         fruts_p = fbe_raid_fruts_get_next(fruts_p))
    {
        /* Only get the block operation if the opcode is not invalid since 
         * the fruts packet is not setup unless the opcode is valid. 
         */
        if (fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            block_operation_p = fbe_raid_fruts_to_block_operation(fruts_p);
            if ( (block_operation_p != NULL)                                                    &&
                 (block_operation_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)  &&
                !fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_ERROR_INJECTED)        &&
                !fbe_logical_error_injection_is_downstream_injection_enabled(block_operation_p)    )
            {
                /* There is a media error present at this position and it is not
                 * injected by rderr, return FBE_TRUE in this case
                 */
               fbe_logical_error_injection_service_trace(
                   FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                   "LEI: HARD MEDIA: obj: 0x%x fruts_p: %p pos: %d op: %d lba: 0x%llx blks: 0x%llx b/q:%d/%d\n",
                   object_id, fruts_p, fruts_p->position, fruts_p->opcode, (unsigned long long)fruts_p->lba, (unsigned long long)fruts_p->blocks,
                   block_operation_p->status, block_operation_p->status_qualifier);
               return FBE_TRUE;
            }
        }
    }
    /* There is no external or internal media external media error
     * which might be causing the rderr mismatch.
     */
    return FBE_FALSE;
}
/**************************************************************************
 * fbe_logical_error_injection_is_extern_hard_media_error_present()
 **************************************************************************/



/**************************************************************************
 * fbe_logical_error_injection_extend_error_region_up()
 **************************************************************************
 * @brief
 * Try to combine error region with another one with smaller lba
 *
 * @return
 *  FBE_TRUE - if the region was extended
 *  FBE_FALSE - otherwise
 *
 * @author
 *  06/15/11 - Created. Maya Dagon
 *
 *************************************************************************/
static fbe_bool_t fbe_logical_error_injection_extend_error_region_up(fbe_xor_error_region_t* error_regions_array,
                                                                     fbe_lba_t * lba_p,  
                                                                     fbe_u16_t * block_count_p,
                                                                     fbe_s32_t index)
{
    fbe_xor_error_region_t curr_reg;
    fbe_u16_t error = error_regions_array[index].error & FBE_XOR_ERR_TYPE_MASK; 
    fbe_u16_t pos_bitmask = error_regions_array[index].pos_bitmask ;
    fbe_bool_t was_extended = FBE_FALSE;

    index--;
    for(; index > -1; index--)
    {
        curr_reg = error_regions_array[index];
        if(((curr_reg.error & FBE_XOR_ERR_TYPE_MASK)!= error) || (curr_reg.pos_bitmask != pos_bitmask))
            continue;

        if((curr_reg.lba + curr_reg.blocks) == *lba_p)
        {
            *lba_p = curr_reg.lba;
            *block_count_p = *block_count_p + curr_reg.blocks;
            was_extended = FBE_TRUE;
        }
        else 
        {
            break;
        }
    }

    return was_extended;
}
/**************************************************************************
 * fbe_logical_error_injection_extend_error_region_up
 **************************************************************************/


/**************************************************************************
 * fbe_logical_error_injection_extend_error_region_down()
 **************************************************************************
 * @brief
 * Try to combine error region with another one with bigger lba
 *
 * @return
 *  FBE_TRUE - if the region was extended
 *  FBE_FALSE - otherwise
 *  
 *
 * @author
 *  06/15/11 - Created. Maya Dagon
 *
 *************************************************************************/
static fbe_bool_t fbe_logical_error_injection_extend_error_region_down(fbe_xor_error_regions_t * error_regions_p,
                                                                       fbe_lba_t   start_lba, 
                                                                       fbe_u16_t * block_count_p,
                                                                       fbe_s32_t index)
{
    fbe_xor_error_region_t * curr_reg_p = NULL;
    fbe_u16_t error = error_regions_p->regions_array[index].error & FBE_XOR_ERR_TYPE_MASK; 
    fbe_u16_t pos_bitmask = error_regions_p->regions_array[index].pos_bitmask; 
    fbe_bool_t was_extended = FBE_FALSE;



    index++;
    for(; index < error_regions_p->next_region_index ; index++)
    {
        curr_reg_p = &(error_regions_p->regions_array[index]);
        if(((curr_reg_p->error & FBE_XOR_ERR_TYPE_MASK)!= error) || (curr_reg_p->pos_bitmask != pos_bitmask))
            continue;

        if((start_lba + *block_count_p) == curr_reg_p->lba)
        {
            *block_count_p = *block_count_p + curr_reg_p->blocks;
            was_extended = FBE_TRUE;
        }
        else
        {
            break;
        }
    }

    return was_extended;
}
/**************************************************************************
 * fbe_logical_error_injection_extend_error_region_down()
 **************************************************************************/


/**************************************************************************
 * fbe_logical_error_injection_extend_error_region()
 **************************************************************************
 * @brief
 * Combine several error regions in case of media error with several consecutive error regions
 * seperated by correctable/uncorrectable/zeroed
 * 
 * @param siots_p - fbe_raid_siots_t pointer
 * @param lba_p   - pointer to error region lba start
 * @param block_count_p - poiter to error region block count
 * @param idx_region - index to an error region to match.
 *
 * @return
 *  void
 *
 * @author
 *  06/15/11 - Created. Maya Dagon
 *
 *************************************************************************/
static void fbe_logical_error_injection_extend_error_region(fbe_raid_siots_t* const siots_p,
                                                            fbe_lba_t * lba_p,  
                                                            fbe_u16_t * block_count_p,
                                                            fbe_s32_t   idx_region)
{
    fbe_bool_t was_extended_up = FBE_FALSE;
    fbe_bool_t was_extended_down = FBE_FALSE;
    fbe_lba_t  new_lba;  
    fbe_u16_t  new_block_count;

	/* initialize lba and block count */
	new_lba = siots_p->vcts_ptr->error_regions.regions_array[idx_region].lba;
	new_block_count =  siots_p->vcts_ptr->error_regions.regions_array[idx_region].blocks;

    was_extended_up = fbe_logical_error_injection_extend_error_region_up(siots_p->vcts_ptr->error_regions.regions_array,
                                                       &new_lba,&new_block_count,idx_region);
    was_extended_down = fbe_logical_error_injection_extend_error_region_down(&(siots_p->vcts_ptr->error_regions),
                                                                             new_lba,
                                                                             &new_block_count,
                                                                             idx_region);

    /* If the lba or block count was updated, we need to */
    if (was_extended_up || was_extended_down)
    {
        *lba_p = new_lba;
        *block_count_p = new_block_count;

        if ( *lba_p < siots_p->parity_start )
        {
            /* The error region starts before this parity region.
             * Just validate the portion of the error region
             * which overlaps with this parity region.
             */
            *block_count_p -= (fbe_u16_t)(siots_p->parity_start - *lba_p);
            *lba_p = (fbe_u32_t)(siots_p->parity_start);
        }
        
        /* Normalize the region's lba to match the table.
         */
        *lba_p = fbe_logical_error_injection_get_table_lba(siots_p,
                                                           *lba_p, 
                                                           *block_count_p);
    }

    return;
}
/**************************************************************************
 * fbe_logical_error_injection_extend_error_region()
 **************************************************************************/

/*****************************************
 * end of fbe_logical_error_injection_record_validate.c
 *****************************************/
