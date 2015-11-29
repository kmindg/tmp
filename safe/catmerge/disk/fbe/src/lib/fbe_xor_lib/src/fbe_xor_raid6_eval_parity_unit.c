/****************************************************************
 *  Copyright (C)  EMC Corporation 2006-2007
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ****************************************************************/

/***************************************************************************
 * xor_raid6_eval_parity_unit.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions to implement the EVENODD algorithm
 *  which is used for RAID 6 functionality.
 *
 * @notes
 *
 * @author
 *
 * 03/16/06 - Created. RCL
 *
 **************************************************************************/

#include "fbe_xor_raid6.h"
#include "fbe_xor_raid6_util.h"
#include "fbe_xor_sector_history.h"
#include "fbe_xor_epu_tracing.h"
#include "fbe/fbe_library_interface.h"


/*****************************************
 * Local Functions declared for Visibility
 *****************************************/
static fbe_status_t fbe_xor_epu_debug_checks_r6(const fbe_xor_scratch_r6_t * const scratch_p,
                                        const fbe_xor_error_t * const eboard_p,
                                        fbe_sector_t * const sector_p[],
                                        const fbe_u16_t bitkey[],
                                        const fbe_u16_t width,
                                        const fbe_u16_t parity_pos[],
                                        const fbe_u16_t rebuild_pos[],
                                        fbe_lba_t seed,
                                        const fbe_bool_t inv_parity_crc_error,
                                        const fbe_u16_t data_position[],
                                        fbe_xor_option_t option,
                                        const fbe_u16_t parity_bitmap );

static void fbe_xor_epu_convert_coherency_errors(fbe_xor_error_t * const eboard_p,
                                                 const fbe_u16_t parity_bitmap);
static void fbe_xor_setup_logical_rebuild_positions(fbe_u16_t m_rebuild_pos[],
                                                    fbe_u16_t m_parity_pos[],
                                                    fbe_u16_t m_logical_rebuild_position[],
                                                    fbe_u16_t m_data_position[]);

/* This macro is used to setup the logcial rebuild positions.
 */
static void fbe_xor_setup_logical_rebuild_positions(fbe_u16_t m_rebuild_pos[],
                                                    fbe_u16_t m_parity_pos[],
                                                    fbe_u16_t m_logical_rebuild_position[],
                                                    fbe_u16_t m_data_position[])
{
    if (m_rebuild_pos[0] != FBE_XOR_INV_DRV_POS)
    {
        /* Get the column index of the dead disks and save them for later use in
         * EVENODD algorithm.  If it is the parity disk then it does not have a
         * logical position but save something to let us know which parity disk
         * is being rebuilt.
         */
        if (m_rebuild_pos[0] == m_parity_pos[0])
        {
            m_logical_rebuild_position[0] = FBE_XOR_LOGICAL_ROW_PARITY_POSITION;
        }
        else if (m_rebuild_pos[0] == m_parity_pos[1])
        {
            m_logical_rebuild_position[0] = FBE_XOR_LOGICAL_DIAG_PARITY_POSITION;
        }
        else
        {
            m_logical_rebuild_position[0] = m_data_position[m_rebuild_pos[0]];
        }

        if (m_rebuild_pos[1] != FBE_XOR_INV_DRV_POS)
        {
            /* Get the column index of the dead disks and save them for 
             * later use in EVENODD algorithm.  If it is the parity disk 
             * then it does not have a logical position but save something
             * to let us know which parity disk is being rebuilt.
             */
            if (m_rebuild_pos[1] == m_parity_pos[0])
            {
                m_logical_rebuild_position[1] = FBE_XOR_LOGICAL_ROW_PARITY_POSITION;
            }
            else if (m_rebuild_pos[1] == m_parity_pos[1])
            {
                m_logical_rebuild_position[1] = FBE_XOR_LOGICAL_DIAG_PARITY_POSITION;
            }
            else
            {  
                m_logical_rebuild_position[1] = m_data_position[m_rebuild_pos[1]];
            }
        }
        else
        {
            m_logical_rebuild_position[1] = FBE_XOR_INV_DRV_POS;
        }
    } 
    else
    {
        m_logical_rebuild_position[0] = FBE_XOR_INV_DRV_POS;
        m_logical_rebuild_position[1] = FBE_XOR_INV_DRV_POS;
    }/* End if to setup logical rebuild positions. */
    return;
} /* end fbe_xor_setup_logical_rebuild_positions() */

/* This macro sets the scratch state based on the number of errors and error 
 * locations.
 */
static __forceinline fbe_status_t fbe_xor_set_scratch_state(fbe_xor_scratch_r6_t * m_Scratch,
                                                     fbe_u16_t m_parity_bitmap)
{
    /* Set the scratch state based on the number of errors and error locations.
     */
    if (FBE_XOR_IS_SCRATCH_VERIFY_STATE(m_Scratch, 
                                                m_parity_bitmap, 
                                                m_Scratch->fatal_key))
    {
        m_Scratch->scratch_state = FBE_XOR_SCRATCH_R6_STATE_VERIFY;
    }
    else if (FBE_XOR_IS_SCRATCH_RECONSTRUCT_1_STATE(m_Scratch, 
                                                            m_parity_bitmap, 
                                                            m_Scratch->fatal_key))
    {
        m_Scratch->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_1;
    }   
    else if (FBE_XOR_IS_SCRATCH_RECONSTRUCT_2_STATE(m_Scratch, 
                                                            m_parity_bitmap, 
                                                            m_Scratch->fatal_key))
    {
        m_Scratch->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_2;
    }   
    else if (FBE_XOR_IS_SCRATCH_RECONSTRUCT_P_STATE(m_Scratch, 
                                                            m_parity_bitmap, 
                                                            m_Scratch->fatal_key))
    {
        m_Scratch->scratch_state = FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P;
    }   
    else
    {
        /* There is no state for more than 2 errors, just check the 
         * checksums and we will invalidate the errored sectors later.
         */
        if (XOR_COND(m_Scratch->fatal_cnt <= 2))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "m_Scratch->fatal_cnt 0x%x <= 2\n",
                                  m_Scratch->fatal_cnt);

            return FBE_STATUS_GENERIC_FAILURE;
        }


        m_Scratch->scratch_state = FBE_XOR_SCRATCH_R6_STATE_DONE;
    } /* End if to set scratch state. */

    return FBE_STATUS_OK;
} /* end fbe_xor_set_scratch_state() */

/* This macro is used to handle a failure to rebuild and the strip_verified
 * flag being set.
 */
static __forceinline fbe_status_t fbe_xor_verify_rebuild_fail(fbe_xor_error_t * m_eboard_p,
                                                       fbe_u16_t m_parity_pos[],
                                                       const fbe_u16_t m_bitkey[],
                                                       fbe_xor_scratch_r6_t * m_scratch_p,
                                                       fbe_u16_t m_initial_rebuild_position[])
{
    fbe_status_t status;

    /* Clear out all errors we have found up to now.
     */
    fbe_xor_setup_eboard(m_eboard_p, 0);
    
    /* Make sure this error is logged, so we will log it against
     * the parity disks.
     */
    m_eboard_p->c_coh_bitmap = m_bitkey[m_parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]] |
                               m_bitkey[m_parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
    status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->c_coh_bitmap, 
                          FBE_XOR_ERR_TYPE_COH, 0, FBE_FALSE);
    if (status != FBE_STATUS_OK) { return status; }
    m_scratch_p->fatal_key = m_bitkey[m_parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]] |
                             m_bitkey[m_parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
    m_scratch_p->fatal_cnt = 2;
    
    if (m_initial_rebuild_position[0] != FBE_XOR_INV_DRV_POS)
    {
        m_eboard_p->u_crc_bitmap = m_bitkey[m_initial_rebuild_position[0]];

        status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_crc_bitmap, 
                              FBE_XOR_ERR_TYPE_CRC, 2, FBE_TRUE);
        if (status != FBE_STATUS_OK) { return status; }

        m_scratch_p->fatal_key |= m_bitkey[m_initial_rebuild_position[0]];
        m_scratch_p->fatal_cnt++;
        
        if (m_initial_rebuild_position[1] != FBE_XOR_INV_DRV_POS)
        {
            m_eboard_p->u_crc_bitmap |= m_bitkey[m_initial_rebuild_position[1]];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_eboard_p->u_crc_bitmap, 
                                  FBE_XOR_ERR_TYPE_CRC, 3, FBE_TRUE);
            if (status != FBE_STATUS_OK) { return status; }
            m_scratch_p->fatal_key |= m_bitkey[m_initial_rebuild_position[1]];
            m_scratch_p->fatal_cnt++;
        }
    }

    return FBE_STATUS_OK;
} /* end fbe_xor_verify_rebuild_fail() */

/***************************************************************************
 *  fbe_xor_raid6_finish_verify_state()
 ***************************************************************************
 * @brief
 *   This routine is called after each sector in a RAID 6 strip has been
 *   inspected and has had the verify precalculations performed on it.  This
 *   function will evaluate the data and determine if the strip is done being
 *   processed or not.
 *
 * @param scratch_p - ptr to the scratch database area.
 * @param sector_p - ptr to the strip.
 * @param bitkey - bitkey for stamps/error boards.
 *   width          [I] - width of strip being checked.
 *
 *  @return fbe_status_t
 *
 *  @author
 *    06/15/2006 - Created. RCL
 ***************************************************************************/
fbe_status_t fbe_xor_raid6_finish_verify_state(fbe_xor_scratch_r6_t * const scratch_p,
                                   fbe_xor_error_t * eboard_p,
                                   const fbe_u16_t bitkey[],
                                   fbe_u16_t data_position[],
                                   fbe_u16_t parity_pos[],
                                   const fbe_u16_t width,
                                   fbe_sector_t * const sector_p[],
                                   fbe_u16_t rebuild_pos[], 
                                   fbe_lba_t seed)
{
    fbe_status_t status;
    fbe_bool_t verify_result;

    status = fbe_xor_eval_verify( scratch_p, eboard_p, bitkey, data_position, parity_pos, width, &verify_result);
    
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    if (verify_result)
    {
        /* Verify found no errors so make the stamps consistent across the strip.
         */
        status = fbe_xor_rbld_parity_stamps_r6(scratch_p, eboard_p,
                                               sector_p, bitkey,
                                               width, rebuild_pos, parity_pos, seed,
                                               FBE_TRUE /* Check parity stamps. */ );
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
    }
    return FBE_STATUS_OK;
} /* fbe_xor_raid6_finish_verify_state() */

/***************************************************************************
 *  fbe_xor_raid6_finish_reconstruct_1_state()
 ***************************************************************************
 * @brief
 *   This routine is called after each sector in a RAID 6 strip has been
 *   inspected and has had the reconstruct 1 precalculations performed on it.
 *   This function will evaluate the data and determine if the strip is done 
 *   being processed or not.
 *
 * @param scratch_p - ptr to the scratch database area.
 * @param eboard_p - error board for marking different types of errors.
 * @param bitkey - bitkey for stamps/error boards.
 * @param data_position - vector of data positions
 *   parity_pos[2], [I] - indexes containing parity info.  Always 2 valid drives
 *                        for RAID 6.
 *   rebuild_pos[2], [I] - indexes to be rebuilt, FBE_XOR_INV_DRV_POS in first position if
 *                         no rebuild required.
 * @param width - number of sectors in array.
 * @param sector -  vector of ptrs to sector contents.
 * @param initial_rebuild_position - indexes to the rebuild positions passed
 *                                   into eval_parity_unit.
 * @param seed - seed value for checksum.
 * @param inv_parity_crc_error - FBE_TRUE to update the parity on CRC error,
 *                               FBE_FALSE to not update the parity on CRC error.
 * @param parity_bitmap - bitmap of the parity positions.
 *
 *  @return fbe_status_t
 *
 *  @author
 *    06/15/2006 - Created. RCL
 *    07/19/2012 - Vamsi V. Invalidate rebuild pos if stamp check fails.   
 ***************************************************************************/
fbe_status_t fbe_xor_raid6_finish_reconstruct_1_state(fbe_xor_scratch_r6_t * const scratch_p,
                                          fbe_xor_error_t * eboard_p,
                                          const fbe_u16_t bitkey[],
                                          fbe_u16_t data_position[],
                                          fbe_u16_t parity_pos[],
                                          const fbe_u16_t width,
                                          fbe_sector_t * const sector_p[],
                                          fbe_u16_t rebuild_pos[],
                                          fbe_u16_t initial_rebuild_position[],
                                          fbe_lba_t seed,
                                          fbe_bool_t inv_parity_crc_error,
                                          fbe_u16_t parity_bitmap)
{
    fbe_bool_t error_result;
    fbe_bool_t reconstruct_result;
    fbe_status_t status;
    fbe_u16_t invalid_bitmap;
    fbe_bool_t b_needs_inv;

    /* Check stamps across strip and rebuild the stamps on the disk that is
     * being rebuilt.  If there are stamp inconsistencies across the strip then
     * the rebuild can not proceed.
     */
    status = fbe_xor_rbld_data_stamps_r6(scratch_p,
                                         eboard_p,
                                         sector_p,
                                         bitkey,
                                         parity_bitmap,
                                         width,
                                         rebuild_pos,
                                         parity_pos, 
                                         FBE_TRUE /* Calling from reconstruct 1. */,
                                         &error_result,
                                         &b_needs_inv);
    if (status != FBE_STATUS_OK) { return status; }
    if (error_result)
    {               
        status = fbe_xor_eval_reconstruct_1(scratch_p,
                                            parity_pos,
                                            eboard_p,
                                            bitkey,
                                            rebuild_pos,
                                            &reconstruct_result);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        if (reconstruct_result) /* Physical rebuild positions. */
        {
            fbe_u32_t parity_bitkey; /* Bitkey of parity positions. */ 
            
            /* Create a bitkey of parity positions so the below
             * function will just rebuild the parity stamps.
             */
            parity_bitkey = 
                bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]] | 
                bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
        
            /* If the rebuild[1] position is parity, then we will be going
             * to rebuild parity state next.
             */
            if ( rebuild_pos[1] != parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW] &&
                 rebuild_pos[1] != parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG] )
            {
                /* Only call rebuild parity stamps if we will not
                 * be going to rebuild parity next.
                 * We will rebuild the stamps in rebuild parity and do not
                 * need to rebuild stamps now.
                 *
                 * Fix up the parity stamps so they match the data.
                 */
                status = fbe_xor_rbld_parity_stamps_r6(scratch_p,
                                                       eboard_p,
                                                       sector_p,
                                                       bitkey,
                                                       width,
                                                       rebuild_pos,
                                                       parity_pos,
                                                       seed,
                                                       FBE_FALSE /* Don't check parity stamps. */);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }
            /* If the rebuild was successful then unset c_crc_bitmap of original
             * rebuild position.  The c_crc_bitmap of the original positions are
             * unset because the calling function expects no bits set in the
             * rebuild positions if the rebuild was successful.
             */
            if ((initial_rebuild_position[0] != FBE_XOR_INV_DRV_POS)                    &&
                (initial_rebuild_position[0] != parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]) &&
                (initial_rebuild_position[0] != parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]))
            {
                eboard_p->c_crc_bitmap &= ~(bitkey[initial_rebuild_position[0]]);
            }
            if ((initial_rebuild_position[1] != FBE_XOR_INV_DRV_POS)              &&
                (initial_rebuild_position[1] != parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]) &&
                (initial_rebuild_position[1] != parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]))
            {
                eboard_p->c_crc_bitmap &= ~(bitkey[initial_rebuild_position[1]]);
            }
        }
        else /* The rebuild failed. */
        {
            /* Check if this error was found in verify.  If it was there might 
             * be a coherency error so only invalidate any blocks that were bad
             * when epu started.
             */
            if (scratch_p->strip_verified == FBE_TRUE)
            {
                status = fbe_xor_verify_rebuild_fail(eboard_p, parity_pos, bitkey, scratch_p,
                                                     initial_rebuild_position);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }

            /* Add invalidated sectors that we have run into already into the
             * eboard to be re-invalidated.  We pass 0 to the EPU tracing since
             * we don't want to trace these errors.
             */
            eboard_p->u_crc_bitmap |= eboard_p->crc_invalid_bitmap |
                                      eboard_p->crc_raid_bitmap    |
                                      eboard_p->corrupt_crc_bitmap;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, 0, 
                                  FBE_XOR_ERR_TYPE_CRC, 4, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }

            /* Invalidate the blocks.
             */
            status = fbe_xor_eval_parity_invalidate_r6(scratch_p, scratch_p->media_err_bitmap,
                                                       eboard_p, sector_p, /* Vector of sectors. */
                                                       bitkey,   /* Vector of bitkeys. */
                                                       width, seed, /* Usually the physical lba. */
                                                       parity_pos, rebuild_pos, initial_rebuild_position,
                                                       data_position,                                          
                                                       inv_parity_crc_error,
                                                       &invalid_bitmap);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
            scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_DONE;
        }
    }
    else /* if the stamp build fails */
    {
        if (b_needs_inv)
        {
            /* Add invalidated sectors that we have run into already into the
             * eboard to be re-invalidated.  We pass 0 to the EPU tracing since
             * we don't want to trace these errors.
             */
            eboard_p->u_crc_bitmap |= eboard_p->crc_invalid_bitmap |
                                      eboard_p->crc_raid_bitmap    |
                                      eboard_p->corrupt_crc_bitmap;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, 0, 
                                               FBE_XOR_ERR_TYPE_CRC, 6, FBE_TRUE);

            /* Invalidate the blocks.
             */
            status = fbe_xor_eval_parity_invalidate_r6(scratch_p,
                                                       scratch_p->media_err_bitmap,
                                                       eboard_p, 
                                                       sector_p, /* Vector of sectors. */
                                                       bitkey,   /* Vector of bitkeys. */
                                                       width, 
                                                       seed, /* Usually the physical lba. */
                                                       parity_pos, 
                                                       rebuild_pos,
                                                       initial_rebuild_position,
                                                       data_position,                                          
                                                       inv_parity_crc_error,
                                                       &invalid_bitmap );

            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
            scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_DONE;

            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_INFO,
                                  "xor: rg obj: 0x%x invalidate blks due to ts/ws mismatch. inv bm: 0x%x lba: 0x%llx c_ts: 0x%x c_ws: 0x%x",
                                  eboard_p->raid_group_object_id, invalid_bitmap, seed, eboard_p->c_ts_bitmap, eboard_p->c_ws_bitmap);
        }
    } /* end if if stamp rebuild fails */
    return FBE_STATUS_OK;
} /* fbe_xor_raid6_finish_reconstruct_1_state() */

/***************************************************************************
 *  fbe_xor_raid6_finish_reconstruct_2_state()
 ***************************************************************************
 * @brief
 *   This routine is called after each sector in a RAID 6 strip has been
 *   inspected and has had the reconstruct 2 precalculations performed on it.
 *   This function will evaluate the data and determine if the strip is done 
 *   being processed or not.
 *

 * @param scratch_p - ptr to the scratch database area.
 * @param eboard_p - error board for marking different types of errors.
 * @param bitkey - bitkey for stamps/error boards.
 * @param data_position - vector of data positions
 *   parity_pos[2], [I] - indexes containing parity info.  Always 2 valid drives
 *                        for RAID 6.
 *   rebuild_pos[2], [I] - indexes to be rebuilt, FBE_XOR_INV_DRV_POS in first position if
 *                         no rebuild required.
 * @param width - number of sectors in array.
 * @param sector -  vector of ptrs to sector contents.
 * @param initial_rebuild_position - indexes to the rebuild positions passed
 *                                   into eval_parity_unit.
 * @param seed - seed value for checksum.
 * @param inv_parity_crc_error - FBE_TRUE to update the parity on CRC error,
 *                               FBE_FALSE to not update the parity on CRC error.
 * @param parity_bitmap - bitmap of the parity positions.
 *
 * @return fbe_status_t
 *    
 *
 *  @author
 *    06/15/2006 - Created. RCL
 ***************************************************************************/
fbe_status_t fbe_xor_raid6_finish_reconstruct_2_state(fbe_xor_scratch_r6_t * const scratch_p,
                                          fbe_xor_error_t * eboard_p,
                                          const fbe_u16_t bitkey[],
                                          fbe_u16_t data_position[],
                                          fbe_u16_t parity_pos[],
                                          const fbe_u16_t width,
                                          fbe_sector_t * const sector_p[],
                                          fbe_u16_t rebuild_pos[],
                                          fbe_u16_t initial_rebuild_position[],
                                          fbe_lba_t seed,
                                          fbe_bool_t inv_parity_crc_error,
                                          fbe_u16_t parity_bitmap)
{
    fbe_bool_t error_result;
    fbe_status_t status;
    fbe_u16_t invalid_bitmap;
	fbe_bool_t b_needs_inv;

    /* Check stamps across strip first. 
     * If stamps are good, we'll continue to rebuild the data.
     * If not, we'll just invalidate the sectors.
     */
    status = fbe_xor_rbld_data_stamps_r6(scratch_p,
                                         eboard_p,
                                         sector_p,
                                         bitkey,
                                         parity_bitmap,
                                         width,
                                         rebuild_pos,
                                         parity_pos, 
                                         FBE_FALSE /* Calling from reconstruct 2. */,
                                         &error_result,
										 &b_needs_inv);
    if (status != FBE_STATUS_OK) { return status; }

    if (error_result)
    {     
        /* This is the variable for fbe_xor_eval_reconstruct_2 status. 
         */
        fbe_bool_t rbld_2_disks_status;

        /* Rebuild the data disks and return the status.
         */
        status = fbe_xor_eval_reconstruct_2(scratch_p, 
                                      data_position[rebuild_pos[0]], 
                                      data_position[rebuild_pos[1]],
                                      eboard_p, bitkey, rebuild_pos, 
                                      initial_rebuild_position,
                                      parity_pos,
                                      &rbld_2_disks_status);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        if(rbld_2_disks_status)
        {
            fbe_u32_t parity_bitkey; /* Bitkey of parity positions. */ 
            
            /* Create a bitkey of parity positions so the below
             * function will just rebuild the parity stamps.
             */
            parity_bitkey = 
                bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]] | 
                bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];

            /* If rebuild is successful, clean up the related
             * error bitmaps.
             */
            fbe_xor_raid6_correct_all(eboard_p);
        
            /* Fix up the parity stamps so they match the data.
             */
            status = fbe_xor_rbld_parity_stamps_r6(scratch_p,
                                                   eboard_p,
                                                   sector_p,
                                                   bitkey,
                                                   width,
                                                   rebuild_pos,
                                                   parity_pos,
                                                   seed,
                                                   FBE_FALSE /* Don't check parity stamps. */);
            if (status != FBE_STATUS_OK) 
            { 
                return status;
            }
        }
        else /* if the data rebuild fails */
        {
            /* Check if this error was found in verify.  If it was there might 
             * be a coherency error so only invalidate any blocks that were bad
             * when epu started.
             */
            if (scratch_p->strip_verified == FBE_TRUE)
            {
                status = fbe_xor_verify_rebuild_fail(eboard_p, parity_pos, bitkey, scratch_p,
                                                     initial_rebuild_position);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }

            /* Add invalidated sectors that we have run into already into the
             * eboard to be re-invalidated. We pass 0 to the EPU tracing since
             * we don't want to trace these errors.
             */
            eboard_p->u_crc_bitmap |= eboard_p->crc_invalid_bitmap |
                                      eboard_p->crc_raid_bitmap    |
                                      eboard_p->corrupt_crc_bitmap;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, 0, 
                                  FBE_XOR_ERR_TYPE_CRC, 5, FBE_TRUE);
            if (status != FBE_STATUS_OK) { return status; }

            status = fbe_xor_eval_parity_invalidate_r6(scratch_p,
                                                       scratch_p->media_err_bitmap,
                                                       eboard_p, 
                                                       sector_p, /* Vector of sectors. */
                                                       bitkey,   /* Vector of bitkeys. */
                                                       width, 
                                                       seed, /* Usually the physical lba. */
                                                       parity_pos, 
                                                       rebuild_pos,
                                                       initial_rebuild_position,
                                                       data_position,
                                                       inv_parity_crc_error,
                                                       &invalid_bitmap);               
             if (status != FBE_STATUS_OK) 
             { 
                 return status; 
             }
        }
    }
    else /* if the stamp build fails */
    {
        /* Add invalidated sectors that we have run into already into the
         * eboard to be re-invalidated.  We pass 0 to the EPU tracing since
         * we don't want to trace these errors.
         */
        eboard_p->u_crc_bitmap |= eboard_p->crc_invalid_bitmap |
                                  eboard_p->crc_raid_bitmap    |
                                  eboard_p->corrupt_crc_bitmap;
        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, 0, 
                              FBE_XOR_ERR_TYPE_CRC, 6, FBE_TRUE);

        /* Invalidate the blocks.
         */
        status = fbe_xor_eval_parity_invalidate_r6(scratch_p,
                                      scratch_p->media_err_bitmap,
                                      eboard_p, 
                                      sector_p, /* Vector of sectors. */
                                      bitkey,   /* Vector of bitkeys. */
                                      width, 
                                      seed, /* Usually the physical lba. */
                                      parity_pos, 
                                      rebuild_pos,
                                      initial_rebuild_position,
                                      data_position,                                          
                                      inv_parity_crc_error,
                                      &invalid_bitmap );
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_INFO,
                              "xor: rg obj: 0x%x invalidate blks due to ts/ws mismatch. inv bm: 0x%x lba: 0x%llx c_ts: 0x%x c_ws: 0x%x",
                              eboard_p->raid_group_object_id, invalid_bitmap, seed, eboard_p->c_ts_bitmap, eboard_p->c_ws_bitmap);

    } /* end if if stamp rebuild fails */

    /* For the reconstruct 2 case, 
     * if the rebuild succeeds, we return STATE_DONE.
     * If the rebuild fails, we invalidate the block,
     * so we still return STATE_DONE.
     */
    scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_DONE;
    return FBE_STATUS_OK;
} /* fbe_xor_raid6_finish_reconstruct_2_state() */

/***************************************************************************
 *  fbe_xor_raid6_finish_reconstruct_p_state()
 ***************************************************************************
 * @brief
 *   This routine is called after each sector in a RAID 6 strip has been
 *   inspected and has had the reconstruct parity precalculations performed on it.
 *   This function will evaluate the data and determine if the strip is done 
 *   being processed or not.
 *
 * @param scratch_p - ptr to the scratch database area.
 * @param eboard_p - error board for marking different types of errors.
 * @param bitkey - bitkey for stamps/error boards.
 * @param data_position - vector of data positions
 *   parity_pos[2], [I] - indexes containing parity info.  Always 2 valid drives
 *                        for RAID 6.
 *   rebuild_pos[2], [I] - indexes to be rebuilt, FBE_XOR_INV_DRV_POS in first position if
 *                         no rebuild required.
 * @param width - number of sectors in array.
 * @param sector -  vector of ptrs to sector contents.
 * @param initial_rebuild_position - indexes to the rebuild positions passed
 *                                   into eval_parity_unit.
 * @param seed - seed value for checksum.
 * @param inv_parity_crc_error - FBE_TRUE to update the parity on CRC error,
 *                               FBE_FALSE to not update the parity on CRC error.
 * @param parity_bitmap - bitmap of the parity positions.
 *
 * @return fbe_status_t
 *     
 * @author
 *    06/15/2006 - Created. RCL
 ***************************************************************************/
fbe_status_t  fbe_xor_raid6_finish_reconstruct_p_state(fbe_xor_scratch_r6_t * const scratch_p,
                                          fbe_xor_error_t * eboard_p,
                                          const fbe_u16_t bitkey[],
                                          fbe_u16_t data_position[],
                                          fbe_u16_t parity_pos[],
                                          const fbe_u16_t width,
                                          fbe_sector_t * const sector_p[],
                                          fbe_u16_t rebuild_pos[],
                                          fbe_u16_t initial_rebuild_position[],
                                          fbe_lba_t seed,
                                          fbe_bool_t inv_parity_crc_error)
{
    fbe_status_t status;
    fbe_bool_t reconstruct_result;
    fbe_u16_t invalid_bitmap;

    /* If we have two parities down make sure the second one is not a new error.
     */
    if ((scratch_p->fatal_cnt==2) && (rebuild_pos[1] == FBE_XOR_INV_DRV_POS))
    {
        /* This means we have two errors but the second one is new.  We should
         * never get here but eval_parity_sector can update this variable so we
         * should check it to make sure.  If we are here assert in debug mode
         * but it is safe to go through the loop again from the beginning in
         * a release.
         */
    }
    else
    {
        /* Make sure the stamps are consistent across the strip.
         */
        status = fbe_xor_rbld_parity_stamps_r6(scratch_p, eboard_p, sector_p, bitkey,
                                  width, rebuild_pos, parity_pos, seed, FBE_FALSE /* Rebuild parity stamps. */ );
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        status = fbe_xor_eval_reconstruct_parity(scratch_p, parity_pos,  eboard_p, 
                                                 bitkey, rebuild_pos, sector_p, &reconstruct_result);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        if (reconstruct_result)
        {
            /* If the rebuild was successful then unset c_crc_bitmap of original
             * rebuild positions if they are parity.  The c_crc_bitmap of the 
             * original positions are unset because the calling function expects
             * no bits set in the rebuild positions if the rebuild was successful.
             */
            if ((initial_rebuild_position[0] == parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]) ||
                (initial_rebuild_position[0] == parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]))
            {
                eboard_p->c_crc_bitmap &= ~(bitkey[initial_rebuild_position[0]]);
            }
            if ((initial_rebuild_position[1] == parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]) ||
                (initial_rebuild_position[1] == parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]))
            {
                eboard_p->c_crc_bitmap &= ~(bitkey[initial_rebuild_position[1]]);
            }
        } 
        else /* The rebuild failed. */
        {
            /* Add invalidated sectors that we have run into already into the
             * eboard to be re-invalidated.  We pass 0 to the EPU tracing since
             * we don't want to trace these errors.
             */
            eboard_p->u_crc_bitmap |= eboard_p->crc_invalid_bitmap |
                                      eboard_p->crc_raid_bitmap    |
                                      eboard_p->corrupt_crc_bitmap;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, 0, 
                                  FBE_XOR_ERR_TYPE_CRC, 7, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }

            /* Invalidate the blocks.
             */
            status = fbe_xor_eval_parity_invalidate_r6(scratch_p, scratch_p->media_err_bitmap, eboard_p, 
                                                       sector_p, /* Vector of sectors. */
                                                       bitkey,   /* Vector of bitkeys. */
                                                       width, seed, /* Usually the physical lba. */
                                                       parity_pos, 
                                                       rebuild_pos, initial_rebuild_position,
                                                       data_position, inv_parity_crc_error,
                                                       &invalid_bitmap);
            if (status != FBE_STATUS_OK) 
            { 
                return status;
            }

            scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_DONE;
        } /* End if fbe_xor_eval_reconstruct_parity */
    } /* End if new parity error */
    return FBE_STATUS_OK;
} /* fbe_xor_raid6_finish_reconstruct_p_state() */

/***************************************************************************
 * fbe_xor_eval_parity_unit_r6()
 ***************************************************************************
 * @brief
 *  This routine is used for Raid6 LUs.  It checks the CRC-ness of the sector
 *  from each drive in the LUN.  It will evaluate the coherency of the data
 *  or rebuild the lost disks (up to two).
 *
 * @param eboard_p - error board for marking different types of errors.
 * @param sector -  vector of ptrs to sector contents.
 * @param bitkey -  bitkey for stamps/error boards.
 * @param width - number of sectors in array.
 *  parity_pos[2], [I] - indexes containing parity info.  Always 2 valid drives
 *                 for RAID 6.
 *  rebuild_pos[2], [I] - indexes to be rebuilt, FBE_XOR_INV_DRV_POS in first position if
 *                 no rebuild required.
 * @param seed - seed value for checksum.
 * @param b_final_recovery_attempt, [I] - FBE_TRUE - This was the last attempt
 *                                          at recovery.
 *                              FBE_FALSE - There are still retries and thus data
 *                                          will not be written. 
 * @param data_position - vector of data positions
 * @param invalid_bitmask_p [O] - bitmask indicating which sectors have been invalidated.
 *
 * @return fbe_status_t
 *
 * @notes
 *  This function has three main responsibilities.
 *    1) Check the coherency of the strip
 *    2) Rebuild a lost disk
 *    3) Rebuild two lost disks
 *
 * @author
 *  03/25/06 - Created. RCL
 *
 ***************************************************************************/
 fbe_status_t fbe_xor_eval_parity_unit_r6(fbe_xor_error_t * eboard_p,
                                fbe_sector_t * sector_p[],
                                fbe_u16_t bitkey[],
                                fbe_u16_t width,
                                fbe_u16_t parity_pos[],
                                fbe_u16_t rebuild_pos[],
                                fbe_lba_t seed,
                                fbe_bool_t b_final_recovery_attempt,
                                fbe_u16_t data_position[],
                                fbe_xor_option_t option,
                                fbe_u16_t * const invalid_bitmask_p)
{
    fbe_u16_t parity_bitmap = 0;
    int times_through_loop = 0;
    fbe_u16_t logical_rebuild_position[2] = {FBE_XOR_INV_DRV_POS,FBE_XOR_INV_DRV_POS};
    fbe_u16_t initial_rebuild_position[2] = {FBE_XOR_INV_DRV_POS,FBE_XOR_INV_DRV_POS};
    fbe_u16_t pos;
    fbe_xor_r6_parity_positions_t parity_index;
    fbe_status_t status;
    fbe_bool_t zero_result;
    fbe_u16_t invalid_bitmap;
    /*! @todo We were advised to allocate this on the stack, 
     *        but we need to change this to have the caller allocate
     *        the scratch and pass this memory in as part of their request.
     */
    fbe_xor_r6_scratch_data_t raid6_scratch_data;

    fbe_xor_scratch_r6_t * Scratch = &raid6_scratch_data.raid6_scratch;
  
    /* Determine the locations of the parity disks.
     */
    parity_bitmap = bitkey[parity_pos[0]] | bitkey[parity_pos[1]];

    /* Init the scratch everytime through the loop so we make sure to
     * catch all the error bits that are set.
     */
    fbe_xor_init_scratch_r6(Scratch,
                            &raid6_scratch_data,
                            sector_p,
                            bitkey,
                            width,
                            seed,
                            parity_bitmap,
                            parity_pos,
                            rebuild_pos,
                            eboard_p,
                            initial_rebuild_position);

    /* Check if this is the special case where the disks are just bound and
     * contain no data.  If it is we should handle it and return, else keep
     * going as normal.
     */
    status = fbe_xor_raid6_handle_zeroed_state(Scratch,
                                               sector_p,
                                               bitkey,
                                               width, 
                                               eboard_p,
                                               parity_pos,
                                               rebuild_pos,
                                               seed,
                                               &zero_result);
    if (status != FBE_STATUS_OK) { return status; }

    if (zero_result)
    {
        *invalid_bitmask_p = Scratch->fatal_key;
        return FBE_STATUS_OK;
    }

    /* Make sure the rebuild positions are in the correct order before we 
     * try to fix anything.
     */
    status = fbe_xor_reset_rebuild_positions_r6(Scratch, sector_p, bitkey, width, parity_pos, rebuild_pos, data_position);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    /* For tracing initialize the loop cnt.
     */
    fbe_xor_epu_trc_init_loop();
    
    /* While the scratch state is not done keep going through the loop, upto
     * three times.  Three passes through the loop are allowable because that
     * is the maximum number of passes needed to correctly complete any state.
     */
    while ((Scratch->scratch_state != FBE_XOR_SCRATCH_R6_STATE_DONE) &&
           (times_through_loop < 3))
    {

        /* Setup logical rebuild positions.
         */
        fbe_xor_setup_logical_rebuild_positions(rebuild_pos, parity_pos,
                                            logical_rebuild_position, data_position);

        /* Set the scratch state based on the number of errors and error locations.
         */
        status = fbe_xor_set_scratch_state(Scratch, parity_bitmap);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        /* Save history on our current scratch state.
         */
        status = fbe_xor_epu_trc_save_state_history( times_through_loop,
                                                     Scratch->fatal_key, 
                                                     Scratch->fatal_cnt, 
                                                     (fbe_u16_t) Scratch->scratch_state);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        
        /* Process each of the sectors containing data, looking
         * for checksum errors or illegal stamp configurations.
         */
        for (pos = width; 0 < pos--;)
        {
            if ((parity_pos[0] != pos) &&
                (parity_pos[1] != pos) &&
                (0 == (bitkey[pos] & Scratch->fatal_key)))
            {
                fbe_bool_t error;
                status = fbe_xor_eval_data_sector_r6(Scratch,
                                                     eboard_p,
                                                     sector_p[pos],
                                                     bitkey[pos],
                                                     data_position[pos],
                                                     logical_rebuild_position,
                                                     parity_bitmap,
                                                     &error);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }
        } /* End for pos = width. */

        /* Process each of the parity sectors, looking
         * for checksum errors or illegal stamp configurations.
         */
        for (parity_index = FBE_XOR_R6_PARITY_POSITION_ROW; parity_index < 2; parity_index++)
        {
            if (0 == (bitkey[parity_pos[parity_index]] & Scratch->fatal_key))
            {
                fbe_bool_t error;

                status = fbe_xor_eval_parity_sector_r6(Scratch,
                                                       eboard_p,
                                                       sector_p[parity_pos[parity_index]],
                                                       bitkey[parity_pos[parity_index]],
                                                       width,
                                                       parity_index,
                                                       parity_bitmap,
                                                       logical_rebuild_position,
                                                       &error);
                if (status != FBE_STATUS_OK) 
                {
                    return status;
                }

                if (!error)
                {
                    /* If we caught another error update rebuild positions.
                     * We need to update this now because we can handle this new
                     * error on a parity sector if we started with one data disk
                     * in error.  If this is not that one case then we will just 
                     * fall through the loop again and start from the begining.
                     */
                    if ((rebuild_pos[0] != parity_pos[0]) && 
                        (rebuild_pos[0] != parity_pos[1]) && 
                        (rebuild_pos[0] != FBE_XOR_INV_DRV_POS) &&
                        (rebuild_pos[1] == FBE_XOR_INV_DRV_POS))
                    {
                        rebuild_pos[1] = parity_index;
                    }
                } /* End if xor_eval_parity_sector. */
            } /* End if (bitkey[parity_pos[parity_index]] & Scratch->fatal_key) */
        } /* End for parity_index */

        /* Evaluate the scratch state and the fatal count and either finish up 
         * the calculations or start again in a different state.
         */
        if ((FBE_XOR_IS_SCRATCH_VERIFY_STATE(Scratch, parity_bitmap, Scratch->fatal_key)) && 
            (Scratch->scratch_state == FBE_XOR_SCRATCH_R6_STATE_VERIFY))
        {
            status = fbe_xor_raid6_finish_verify_state(Scratch, eboard_p, bitkey, data_position, parity_pos,
                                          width, sector_p, rebuild_pos, seed);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }

        } /* End if verify state */
        else if ((FBE_XOR_IS_SCRATCH_RECONSTRUCT_1_STATE(Scratch, parity_bitmap, Scratch->fatal_key)) && 
             (Scratch->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_1))
        {
            status = fbe_xor_raid6_finish_reconstruct_1_state(Scratch, eboard_p, bitkey, data_position, parity_pos,
                                                 width, sector_p, rebuild_pos, initial_rebuild_position,
                                                 seed, b_final_recovery_attempt, parity_bitmap);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }

        } /* End if reconstruct 1 state */
        else if ((FBE_XOR_IS_SCRATCH_RECONSTRUCT_2_STATE(Scratch, parity_bitmap, Scratch->fatal_key)) && 
             (Scratch->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_2))
        {
            status = fbe_xor_raid6_finish_reconstruct_2_state(Scratch, eboard_p, bitkey, data_position, parity_pos, width,
                                                 sector_p, rebuild_pos, initial_rebuild_position, seed,
                                                 b_final_recovery_attempt, parity_bitmap);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        } /* End if reconstruct 2 state */
        else if ((FBE_XOR_IS_SCRATCH_RECONSTRUCT_P_STATE(Scratch, parity_bitmap, Scratch->fatal_key)) && 
            (Scratch->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P))
        {
            status = fbe_xor_raid6_finish_reconstruct_p_state(Scratch, eboard_p, bitkey, data_position, parity_pos, width,
                                                 sector_p, rebuild_pos, initial_rebuild_position,
                                                 seed, b_final_recovery_attempt);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }

        } /* End if reconstruct parity state */
        else if (Scratch->fatal_cnt > 2)
        {
            /* Add invalidated sectors that we have run into already into the
             * eboard to be re-invalidated.  We pass 0 to the EPU tracing since
             * we don't want to trace these errors.
             */
            eboard_p->u_crc_bitmap |= eboard_p->crc_invalid_bitmap |
                                      eboard_p->crc_raid_bitmap    |
                                      eboard_p->corrupt_crc_bitmap;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(Scratch, eboard_p, 0, 
                                  FBE_XOR_ERR_TYPE_CRC, 8, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }

            /* Invalidate the blocks.
             */
            status = fbe_xor_eval_parity_invalidate_r6( Scratch,
                                                        Scratch->media_err_bitmap,
                                                        eboard_p, 
                                                        sector_p, /* Vector of sectors. */
                                                        bitkey,   /* Vector of bitkeys. */
                                                        width, 
                                                        seed, /* Usually the physical lba. */
                                                        parity_pos, 
                                                        rebuild_pos,
                                                        initial_rebuild_position,
                                                        data_position,
                                                        b_final_recovery_attempt ,
                                                        &invalid_bitmap);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        } /* End if invalidate disks (state) */
        else
        {
            /* We finished the loop but we have changed states so we should go
             * through it one more time.
             */
        }
        
        times_through_loop++;
        status = fbe_xor_epu_trc_inc_loop();
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        
        /* Lets set up the rebuild positions and flags in case we are going
         * through the loop again.
         */
       status = fbe_xor_reset_rebuild_positions_r6(Scratch,
                                                  sector_p,
                                                  bitkey,
                                                  width,
                                                  parity_pos,
                                                  rebuild_pos,
                                                  data_position);
       if (status != FBE_STATUS_OK) 
       { 
           return status; 
       }
    } /* end while */
   
    /* We shouldn't leave the loop unless we are done.
     */
    if (Scratch->scratch_state != FBE_XOR_SCRATCH_R6_STATE_DONE)
    {
        /* This is really bad, if we are in a checked build stop now.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "unexpected error :Scratch->scratch_state 0x%x != FBE_XOR_SCRATCH_R6_STATE_DONE 0x%x\n",
                              Scratch->scratch_state,
                              FBE_XOR_SCRATCH_R6_STATE_DONE);

        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    /* If there were any invalidated sectors hit in this strip then they need
     * to be re-invalidated.
     */
    if (((eboard_p->crc_invalid_bitmap |
          eboard_p->crc_raid_bitmap | 
          eboard_p->corrupt_crc_bitmap) != 0) &&
        (eboard_p->u_crc_bitmap == 0))
    {
        /* If these were previously invalidated sectors, do not re-invalidate them
         * Else add invalidated sectors that we have run into already into the
         * eboard to be re-invalidated.  We pass 0 to the EPU tracing since
         * we don't want to trace these errors.
         */   
        eboard_p->u_crc_bitmap |= eboard_p->crc_invalid_bitmap |
                                  eboard_p->crc_raid_bitmap |
                                  eboard_p->corrupt_crc_bitmap;
        status = FBE_XOR_EPU_TRC_LOG_ERROR(Scratch, eboard_p, 0, 
                              FBE_XOR_ERR_TYPE_CRC, 9, FBE_TRUE);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        
        /* Invalidate the blocks.
         */
        status = fbe_xor_eval_parity_invalidate_r6(Scratch,
                                                   Scratch->media_err_bitmap,
                                                   eboard_p, 
                                                   sector_p, /* Vector of sectors. */
                                                   bitkey,   /* Vector of bitkeys. */
                                                   width, 
                                                   seed, /* Usually the physical lba. */
                                                   parity_pos, 
                                                   rebuild_pos,
                                                   initial_rebuild_position,
                                                   data_position,
                                                   b_final_recovery_attempt,
                                                   &invalid_bitmap);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
    }

    /* Remove any retryable errors from the modified bitmap. 
     * We do not want to write out these corrections. 
     * We also do not want to report these as correctable errors. 
     */
    eboard_p->m_bitmap &= ~eboard_p->retry_err_bitmap;
    eboard_p->c_crc_bitmap &= ~eboard_p->retry_err_bitmap;

    /* Were any checksum errors caused by media errors?
     *
     * This is information we need to setup in the eboard,
     * and this is the logical place to determine
     * which errors were the result of media errors.
     */
    status = fbe_xor_determine_media_errors( eboard_p, Scratch->media_err_bitmap );
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    /* We need to return rebuild_pos in the same state as we got it, so set it back.
     */
    rebuild_pos[0] = initial_rebuild_position[0];
    rebuild_pos[1] = initial_rebuild_position[1];
   
    /* Perform the debug checks before exiting.
     */
    status = fbe_xor_epu_debug_checks_r6(Scratch,
                                         eboard_p,
                                         sector_p,
                                         bitkey,
                                         width,
                                         parity_pos,
                                         rebuild_pos,
                                         seed,
                                         b_final_recovery_attempt,
                                         data_position,
                                         option,
                                         parity_bitmap);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    /* If necessary convert any correctable coherency errors.
     */
    fbe_xor_epu_convert_coherency_errors(eboard_p,
                                     parity_bitmap);

    /* Don't include crc_invalid_bitmap
     */
    *invalid_bitmask_p = (Scratch->fatal_key           |
                          eboard_p->crc_invalid_bitmap |
                          eboard_p->crc_raid_bitmap    |
                          eboard_p->corrupt_crc_bitmap  );

    return FBE_STATUS_OK;
}
/* end fbe_xor_eval_parity_unit_r6() */

/*!***************************************************************************
 * fbe_xor_epu_debug_checks_r6()
 *****************************************************************************
 * @brief
 *  This routine contains debug checks that we execute only in debug
 *  mode before exiting from fbe_xor_eval_parity_unit_r6.
 *

 * @param scratch_p - Pointer to the scratch structure.
 * @param eboard_p - error board for marking different types of errors.
 * @param sector -  vector of ptrs to sector contents.
 * @param bitkey -  bitkey for stamps/error boards.
 * @param width - number of sectors in array.
 *  parity_pos[2], [I] - indexes containing parity info.  Always 2 valid drives
 *                 for RAID 6.
 *  rebuild_pos[2], [I] - indexes to be rebuilt, FBE_XOR_INV_DRV_POS in first position if
 *                 no rebuild required.
 * @param seed - seed value for checksum.
 * @param b_final_recovery_attempt, [I] - FBE_TRUE - This was the last attempt
 *                                          at recovery.
 *                              FBE_FALSE - There are still retries and thus data
 *                                          will not be written. 
 * @param data_position - vector of data positions
 * @param parity_bitmap - Bitmap of parity positions.
 *
 * @return fbe_status_t
 *  
 *
 * @author
 *  05/17/07 - Created. Rob Foley
 *  03/22/10:  Omer Miranda -- Updated to use the new sector tracing facility.
 *
 ***************************************************************************/
static fbe_status_t fbe_xor_epu_debug_checks_r6(const fbe_xor_scratch_r6_t * const scratch_p,
                                   const fbe_xor_error_t * const eboard_p,
                                   fbe_sector_t * const sector_p[],
                                   const fbe_u16_t bitkey[],
                                   const fbe_u16_t width,
                                   const fbe_u16_t parity_pos[],
                                   const fbe_u16_t rebuild_pos[],
                                   fbe_lba_t seed,
                                   const fbe_bool_t b_final_recovery_attempt,
                                   const fbe_u16_t data_position[],
                                   fbe_xor_option_t option,
                                   const fbe_u16_t parity_bitmap)
{
    /* We only execute the below checks in debug builds since the
     * tracing that gets executed can be verbose.
     */
    fbe_u16_t   pos;
    fbe_status_t status;
    fbe_u16_t   intentional_crc;
    fbe_u16_t   u_err_bitmap;
    fbe_u16_t   actual_row_poc = 0;
    fbe_u16_t   calculated_row_poc = 0;
    fbe_u16_t   crc_array[FBE_XOR_MAX_FRUS];
    fbe_u16_t   local_crc_invalid_bitmap = eboard_p->crc_invalid_bitmap;
    
    /* We should never leave with an uncorrectable on a parity drive becuase
     * we can fix all of those by at least rebuilding parity.  RAID 6 does not
     * shed data when a data sector is invalidated but it updates parity with
     * the invalidated data.  Because of this the parity sectors are always
     * valid.
     */
    if (XOR_COND((scratch_p->fatal_key & parity_bitmap) != 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "(scratch_p->fatal_key 0x%x & parity_bitmap 0x%x) != 0\n",
                              scratch_p->fatal_key,
                              parity_bitmap);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* u_err_bitmap contains a bit set if it is an uncorrectable error
     * that was not caused by media errors.
     */
    u_err_bitmap = (eboard_p->u_crc_bitmap & ~parity_bitmap)                 |
                   eboard_p->u_coh_bitmap       | eboard_p->u_ts_bitmap      |
                   eboard_p->u_ws_bitmap        | eboard_p->u_ss_bitmap      |
                   eboard_p->u_n_poc_coh_bitmap | eboard_p->u_poc_coh_bitmap | 
                   eboard_p->u_coh_unk_bitmap;

    /* In most cases `data lost invalidated' sectors are not flagged as uncorrectable.
     * But RAID-6 also re-invalidates reconstructed `data lost invalidated' sectors.
     * Therefore if the `invalidated' bit is set but the `uncorrectable' bit
     * isn't, clear the local copy of `invalidated'.
     */
    if ((local_crc_invalid_bitmap != 0)                                                  &&
        ((local_crc_invalid_bitmap & eboard_p->u_crc_bitmap) != local_crc_invalid_bitmap)    )  
    {
        local_crc_invalid_bitmap = (local_crc_invalid_bitmap & eboard_p->u_crc_bitmap);
    }

    /*! @todo for now errors on parity are correctable so we take them out of the intentional crc error class.
     */
    intentional_crc  = (eboard_p->crc_raid_bitmap | eboard_p->corrupt_crc_bitmap) & ~parity_bitmap;
    intentional_crc |= local_crc_invalid_bitmap;

    /* Between the fatal_key, crc_invalid_bitmap, crc_raid_bitmap, and corrupt_crc_bitmap a bitmap
     * of all the errors should be returned.  Lets check it. 
     */
    if (XOR_COND((scratch_p->fatal_key | intentional_crc) != u_err_bitmap))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "xor: epu debug r6 - (fatal: 0x%x or intentional: 0x%x) doesn't equal u_err: 0x%x",
                              scratch_p->fatal_key, intentional_crc, u_err_bitmap);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Make sure if we are R6, that the time stamps and write stamps
     * are identical on both parities.
     */
    if (!(b_final_recovery_attempt == FBE_FALSE ||
        (parity_pos[1] == FBE_XOR_INV_DRV_POS ||
        ( (sector_p[parity_pos[0]]->time_stamp ==
           sector_p[parity_pos[1]]->time_stamp) &&
          (sector_p[parity_pos[0]]->write_stamp ==
           sector_p[parity_pos[1]]->write_stamp) ) ))) 
    {        
        /* Save the vector of sectors to trace for debugging purposes.
         */
         
        fbe_sector_trace_error_flags_t trace_coh_error_flag = FBE_SECTOR_TRACE_ERROR_FLAG_UCOH;
        if( option & FBE_XOR_OPTION_BVA){
			trace_coh_error_flag = FBE_SECTOR_TRACE_ERROR_FLAG_ECOH;
        }

        status = fbe_xor_sector_history_trace_vector_r6(seed,   /* Lba */           
                                                        (const fbe_sector_t * const *)sector_p,              /* Sector vector to save. */                   
                                                        bitkey,                /* Bitmask of positions in group. */
                                                        width,                 /* Width of group. */                                                         
                                                        parity_pos,            /* Parity position. */                                                
                                                        eboard_p->raid_group_object_id, /* RAID Group Object Identifier*/ 
                                                        eboard_p->raid_group_offset, /* RAID Group offset */                                                      
                                                        "Parity Stamps ",             /* A string to display in trace. */
                                                        FBE_SECTOR_TRACE_ERROR_LEVEL_CRITICAL, /* Trace level */
                                                        trace_coh_error_flag /* trace error type */);

        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        
       fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                             "Unexpected error: parity stamps do not match. t[0]:0x%x t[1]:0x%x w[0]:0x%x w[1]:0x%x\n",
                             sector_p[parity_pos[0]]->time_stamp,
                             sector_p[parity_pos[1]]->time_stamp,
                             sector_p[parity_pos[0]]->write_stamp,
                             sector_p[parity_pos[1]]->write_stamp);

       return FBE_STATUS_GENERIC_FAILURE;
    }  

    /* Validate parity checksums.
     */
    if (b_final_recovery_attempt)
    {
        for (pos = width; 0 < pos--;)
        {
            if ((parity_pos[0] == pos) || (parity_pos[1] == pos))
            {
                fbe_sector_t *pblk_p = sector_p[pos];
                const fbe_u16_t crc = xorlib_cook_csum(xorlib_calc_csum(pblk_p->data_word), 0);
                if (crc != pblk_p->crc)
                {
                      fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                            "unexpected checksum mismatch error. "
                                            "cooked check-sum = 0x%x, block check-sum = 0x%x "
                                            "Block address = 0x%p\n",
                                            crc,
                                            pblk_p->crc,
                                            pblk_p);
                }
            }
        }
    }

    /* Initialize per-position crc array (used to debug failures and compute PoC)
     */
    for (pos = 0; pos < FBE_XOR_MAX_FRUS; pos++)
    {
        crc_array[pos] = 0;
    }

    /* Validate data drive stamps.
     */
    for (pos = width; 0 < pos--;)
    {
        if ((parity_pos[0] != pos) &&
            (parity_pos[1] != pos))
        {
            fbe_sector_t *dblk_p = sector_p[pos];
            
            if (!(0 == (dblk_p->time_stamp & FBE_SECTOR_ALL_TSTAMPS))) 
            {
                /* Save the current sector to trace.
                 */
                status = fbe_xor_report_error_trace_sector( seed,    /* Lba */
                                                            bitkey[pos],      /* Bitmask of position in group. */
                                                            0, /* The number of crc bits in error isn't known */
                                                            eboard_p->raid_group_object_id, /* RAID Group object identifier */
                                                            eboard_p->raid_group_offset, /* RAID Group object offset */ 
                                                            dblk_p,           /* Sector to save. */
                                                            "Time Stamp ",    /* A string to display in trace. */
                                                            FBE_SECTOR_TRACE_ERROR_LEVEL_CRITICAL, /* Trace level */
                                                            FBE_SECTOR_TRACE_ERROR_FLAG_TS /* Trace error type */);

                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }

                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "Unexpected error: time stamps do not match. t[0]:0x%x t[1]:0x%x w[0]:0x%x w[1]:0x%x\n",
                                      sector_p[parity_pos[0]]->time_stamp,
                                      sector_p[parity_pos[1]]->time_stamp,
                                      sector_p[parity_pos[0]]->write_stamp,
                                      sector_p[parity_pos[1]]->write_stamp);

                return FBE_STATUS_GENERIC_FAILURE;
            }  

            if (!xorlib_is_valid_lba_stamp(&dblk_p->lba_stamp, seed, eboard_p->raid_group_offset)) 
            {        
                /* Save the current sector to trace.
                 */
                status = fbe_xor_report_error_trace_sector( seed,  /* Lba */
                                                            bitkey[pos],      /* Bitmask of position in group. */
                                                            0, /* The number of crc bits in error isn't known */
                                                            eboard_p->raid_group_object_id, /* RAID Group object identifier */
                                                            eboard_p->raid_group_offset, /* RAID Group offset */ 
                                                            dblk_p,           /* Sector to save. */
                                                            "LBA Stamp ",    /* A string to display in trace. */
                                                            FBE_SECTOR_TRACE_ERROR_LEVEL_CRITICAL, /* Trace level */
                                                            FBE_SECTOR_TRACE_ERROR_FLAG_LBA /* Trace error type */);
                if (status != FBE_STATUS_OK) 
                {  
                    return status; 
                }

                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "Unexpected error: lba stamp error stamp: 0x%x seed: 0x%llx bitkey: 0x%x\n",
                                      dblk_p->lba_stamp,
                                      (unsigned long long)seed,
                                      bitkey[pos]);
                
                return FBE_STATUS_GENERIC_FAILURE;
            }  

            /* Accumulate the calculated PoC
             */
            crc_array[pos] = dblk_p->crc;
            calculated_row_poc ^= crc_array[pos];
        }
    }

    /* Although not always valid (see below) get the row Parity of Checksums
     */
    actual_row_poc = sector_p[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]->lba_stamp;

    /*! @todo If `b_final_recovery_attempt' is set and this is the `new invalidate 
     *        method', validate that row and diagonal parity have not been
     *        invalidated!
     */

    /*! @note Currently we only update the PoC for the following conditions: 
     *          1) We are on the second pass (a.k.a. retry i.e. `final recovery
     *             attempt') and there is an uncorrectable error.
     *                                  OR
     *          2) There is no uncorrectable error (i.e. there is no possibility
     *             that we will invalidate parity).
     */
    if ((((b_final_recovery_attempt == FBE_TRUE) && 
          (u_err_bitmap != 0)                       ) || 
         (u_err_bitmap == 0)                             ) &&
        (actual_row_poc != calculated_row_poc)                )
    {
        /* Save the vector of sectors to trace for debugging purposes.
         */
        status = fbe_xor_sector_history_trace_vector_r6(seed,   /* Lba */           
                                                        (const fbe_sector_t * const *)sector_p,              /* Sector vector to save. */                   
                                                        bitkey,                /* Bitmask of positions in group. */
                                                        width,                 /* Width of group. */                                                         
                                                        parity_pos,            /* Parity position. */                                                
                                                        eboard_p->raid_group_object_id, /* RAID Group Object Identifier*/ 
                                                        eboard_p->raid_group_offset, /* RAID Group offset */                                                      
                                                        "Row PoC Bad ",             /* A string to display in trace. */
                                                        FBE_SECTOR_TRACE_ERROR_LEVEL_CRITICAL, /* Trace level */
                                                        FBE_SECTOR_TRACE_ERROR_FLAG_POC /* trace error type */);

        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        
       fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                             "xor: Row PoC: 0x%04x doesn't match calculated: 0x%04x\n", 
                             actual_row_poc, calculated_row_poc);

       return FBE_STATUS_GENERIC_FAILURE;
    }  

    return FBE_STATUS_OK;
}
/* end fbe_xor_epu_debug_checks_r6() */

/***************************************************************************
 * fbe_xor_epu_convert_coherency_errors()
 ***************************************************************************
 * @brief
 *  In EPU, if we get too many coherency errors and we cannot determine
 *  the location of the error, we reconstruct parity to make the strip
 *  consistent.  We mark the errors "correctable" in EPU for consistency
 *  since we do not allow uncorrectable errors on parity.  
 *  Parity always gets reconstructed when uncorrectables occur.
 *
 *  The Raid Driver needs XOR to return uncorrectable coherency errors
 *  in any case where the data is in question.  This function 
 *  checks for cases where both parity drives have the same kind of
 *  coherency error.  If it finds any, then it makes the error
 *  uncorrectable.
 *

 * @param eboard_p - error board for marking different types of errors.
 * @param parity_bitmap - Bitmap of parity positions.
 *
 * @return
 *  None.
 *
 * @author
 *  05/17/07 - Created. Rob Foley
 *
 ***************************************************************************/
static void fbe_xor_epu_convert_coherency_errors(fbe_xor_error_t * const eboard_p,
                                            const fbe_u16_t parity_bitmap)
{
    /* If both parity positions are marked with a coherency error,
     * this means that we were unable to determine the location
     * of the error.
     */
    if ( (eboard_p->c_coh_bitmap & parity_bitmap) == parity_bitmap )
    {
        /* Found a case of coherency errors that needs to be
         * converted to uncorrectable.
         */
        eboard_p->c_coh_bitmap &= ~parity_bitmap;
        eboard_p->u_coh_bitmap |= parity_bitmap;
    }

    /* If both parity positions are marked with a poc coherency error,
     * this means that we were unable to determine the location
     * of the error.
     */
    if ( (eboard_p->c_poc_coh_bitmap & parity_bitmap) == parity_bitmap )
    {
        /* Found a case of POC coherency errors that needs to be
         * converted to uncorrectable.
         */
        eboard_p->c_poc_coh_bitmap &= ~parity_bitmap;
        eboard_p->u_poc_coh_bitmap |= parity_bitmap;
    }
    
    /* If necessary convert any correctable coherency errors.
     */
    return;
}
/* end fbe_xor_epu_convert_coherency_errors() */

/***************************************************
 * end xor_raid6_eval_parity_unit.c 
 ***************************************************/
