/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_xor_interface_mirror.c
 ***************************************************************************
 *
 * @brief
 *       This file contains functions used by proc0 to execute XOR commands.
 *   Proc0 will execute XOR xommands based on Flare's xor library
 *   functions.  The commands are executed in-line. The operation and any
 *   necessary error recovery are all completed before the function returns
 *   to the calling driver. This component provides synchronous behavior
 *   and is assumed to provide the best performace in a single processor
 *   architecture.
 *
 * @notes
 *
 * @author
 *   02/25/2000 Created.  Bruss.
 *
 ***************************************************************************/

/************************
 * INCLUDE FILES
 ************************/
#include "fbe_xor_private.h"
#include "fbe_xor_epu_tracing.h"

/*****************************************************
 * static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/



/*!***************************************************************************
 *          fbe_xor_lib_mirror_add_error_region()
 *****************************************************************************
 * 
 * @brief   A mirror read was able to correct a unrecoverable read error using
 *          redundancy etc.  Since we didn't use the XOR algorithms to correct
 *          the issue we need to generate an XOR region manually.
 *
 * @param   eboard_p - The error board with errors.
 * @param   regions_p - The pointer to the error regions structure
 *                       that we should fill out.
 * @param   lba - The lba where this error occurred.
 * @param   width - Width of the mirror group
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  04/16/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_xor_lib_mirror_add_error_region(const fbe_xor_error_t *const eboard_p, 
                                             fbe_xor_error_regions_t *const regions_p, 
                                             const fbe_lba_t lba,
                                             const fbe_block_count_t blocks,
                                             const fbe_u16_t width)
{
    fbe_lba_t   seed;
    fbe_status_t status;

    /* Need to mark each block in error.  But all errors will be coalesced into
     * (1) error region.
     */
    for (seed = lba; seed < (lba + blocks); seed++)
    {
        /* We don't want to invalidate, the parity position is the highest data
         * position + 1 (i.e. width) and the number of parity drives is 0.
         */
        status = fbe_xor_save_error_region(eboard_p, 
                                           regions_p, 
                                           FBE_FALSE,
                                           seed,
                                           0,
                                           width,
                                           width);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
    }

    return(FBE_STATUS_OK);
}
/* end of fbe_xor_lib_mirror_add_error_region() */

/*****************************************************************************
 *                  fbe_xor_mirror_verify()        
 *****************************************************************************
 *
 * @brief:
 *   This function is called by xor_execute_processor_command
 *   to carry out the verify buffer command.  
 *
 * @param   verify_p - Pointer to mirror rebuild request which describes how
 *                      to rebuild the mirror extent.
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *   08/16/00 RG - Created.
 *   01/19/02 Rob Foley - Modified to always increment the sg desc
 *                  just before use so that the memory remains
 *                  mapped in.
 * 01/05/2010   Ron Proulx - Updated to use custom mirror verify request. 
 *
 *****************************************************************************/
fbe_status_t fbe_xor_mirror_verify(fbe_xor_mirror_reconstruct_t *verify_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_xor_option_t    options = verify_p->options;
    fbe_lba_t           seed = verify_p->start_lba;
    fbe_block_count_t  blocks = verify_p->blocks;
    fbe_u32_t           width = verify_p->width;
    fbe_raid_verify_error_counts_t *verify_counts_p = verify_p->error_counts_p;
    fbe_u32_t           parity_position = width;        /* The xor error methods require a parity position */
    fbe_u16_t           valid_bitmask = verify_p->valid_bitmask;
    fbe_u16_t           needs_rebuild_bitmask = verify_p->needs_rebuild_bitmask;
    fbe_u16_t           positions_bitkey[FBE_XOR_MAX_FRUS];
    fbe_xor_sgl_descriptor_t  sgd_v[FBE_XOR_MAX_FRUS];
    fbe_u32_t           block;
    fbe_u16_t           array_position;
    fbe_sector_t        *sector_v[FBE_XOR_MAX_FRUS];    /* Sector ptr for current position. */
    fbe_xor_error_t     *eboard_p = verify_p->eboard_p;
    fbe_u16_t           invalid_bitmap;
    fbe_u32_t           sg_count;

    /* Validate the verify request.
     */
    if (XOR_COND((status = fbe_xor_validate_mirror_verify(verify_p)) != FBE_STATUS_OK ))
    {
        /* Generate a trace and assert (checked builds).
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Validation of mirror verify request for 0x%llx failed with status: 0x%x \n",
                              (unsigned long long)seed, status);
        return(status);
    }

    /*! Now generate the array for all positions.
     *
     */
    fbe_xor_generate_positions_from_bitmask((valid_bitmask | needs_rebuild_bitmask),
                                            &positions_bitkey[0],
                                            width);

    /* Increment statistics (if enabled).
     */
    FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_R1_VR]);

    /* Setup the sgls for all positions.
     */
    for (array_position = 0; array_position < width; array_position++)
    {
        fbe_u32_t bytcnt;

        bytcnt = FBE_XOR_SGLD_INIT_BE(sgd_v[array_position], verify_p->sg_p[array_position]);

        /* If the position is degraded, skip it */
        if (verify_p->sg_p[array_position] == NULL)
        {
            continue;
        }

        /* If the byte count isn't valid, generate a trace event and 
         * assert (checked builds).
         */
        if (XOR_COND(bytcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "bytcnt == 0 \n");

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Now validate that there is a valid buffer supplied.
         */
        if (XOR_COND(FBE_XOR_SGLD_ADDR(sgd_v[array_position]) == NULL))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "Buffer for position: 0x%x is NUll \n",
                                  array_position);  
            return status;
        }
        sector_v[array_position] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[array_position]);
    }

    /* Initialize the previously allocated eboard.
     */
    fbe_xor_setup_eboard(eboard_p, 0); 

    /* For each block of the request execute a verify.
     */
    for (block = 0; block < blocks; block++)
    {
        fbe_xor_error_t     EBoard_local;
        fbe_xor_error_t    *local_eboard_p = &EBoard_local;

        /* Initialize the local (i.e. per sector) error board.
         * Set the u_crc bit for any hard errored drives.
         */
        fbe_xor_init_eboard((fbe_xor_error_t *)&EBoard_local,
                            eboard_p->raid_group_object_id,
                            eboard_p->raid_group_offset);
        fbe_xor_setup_eboard(&EBoard_local,
                            eboard_p->hard_media_err_bitmap); 

        /* Do the stripwise verify.
         */
        status =  fbe_xor_verify_r1_unit(&EBoard_local,
                                         sector_v,
                                         positions_bitkey,
                                         width,
                                         valid_bitmask,
                                         needs_rebuild_bitmask,
                                         seed,
                                         options,
                                         &invalid_bitmap,
                                         verify_p->raw_mirror_io_b);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }

        EBoard_local.u_bitmap = invalid_bitmap;

        /* If the user passed in an error region struct, then fill it out.
         */
        status = fbe_xor_save_error_region(&EBoard_local, 
                                           verify_p->eregions_p,
                                           FBE_TRUE, 
                                           seed,
                                           0,
                                           width,
                                           parity_position);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }

        /* Copy the error information,
         * so we know which frus to write when we finish.
         */
        FBE_XOR_INIT_EBOARD_WMASK(local_eboard_p);

        /* Update the global eboard structure.
         */
        fbe_xor_update_global_eboard(eboard_p, &EBoard_local);

        /* Update the verify counts if supplied.
         */
        if (verify_counts_p != NULL)
        {    
            fbe_xor_vr_increment_errcnt(verify_counts_p, &EBoard_local, NULL, 0);
        }

        /* Setup the sector vector for the next strip.
         */
        for (array_position = 0; 
            (block < (blocks - 1)) && (array_position < width); 
            array_position++)
        {
            /* If the position is degraded, skip it */
            if (verify_p->sg_p[array_position] == NULL)
            {
                continue;
            }

            /* Increment for the next pass.
             */
            status = FBE_XOR_SGLD_INC_BE(sgd_v[array_position], sg_count);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            /* Now validate that there is a valid buffer supplied.
             */
            if (XOR_COND(FBE_XOR_SGLD_ADDR(sgd_v[array_position]) == NULL))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "Buffer after increment for position: 0x%x is NULL \n",
                                      array_position);  
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Update position with new buffer information.
             */
            sector_v[array_position] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[array_position]);
        }

        /* Increment seed for the next strip.
         */
        seed++;

    } /* end for each block of the request */

    /* Always return the execution status.
     */
    return(status);
}  
/* end fbe_xor_mirror_verify() */

/*!***************************************************************************
 *          fbe_xor_mirror_rebuild()        
 *****************************************************************************
 *
 * @brief   This method is called by the raid library to execute a mirror 
 *          rebuild.  There are (2) source arrays that contain positions:
 *          1. `Valid' positions that contain read data
 *          2. `Needs to be Rebuilt' positions that have buffers to populate
 *             with the validated data.
 *          The sum of the valid and needs rebuild positions must be the total
 *          width of the mirror raid group. 
 * 
 * @param   rebuild_p - Pointer to mirror rebuild request which describes how
 *                      to rebuild the mirror extent.
 *
 * @return  status - Typically FBE_STATUS_OK
 *                             Other - The request wasn't properly formed
 *
 * @todo    Currently this method translates the `fbe_xor_mirror_reconstruct_t'
 *          into values that can be consumed by either `fbe_xor_verify_r1_unit' or
 *          `fbe_xor_rebuild_r1_unit' without changes to either of those methods.
 *          In the future `fbe_xor_mirror_reconstruct_t' will be modified and
 *          `fbe_xor_rebuild_r1_unit' will be updated to directly consume it. 
 *
 * @note    Example of 3-way mirror where position 1 was removed and replaced then
 *          position 2 was removed and re-inserted. (NR - Needs Rebuild, s - siots
 *          range).  Note that all 'Needs Rebuild' ranges are in `chunk' size 
 *          multiples where the chunk size is 2048 (0x800) blocks.
 *
 *  pba             Position 0           Position 1           Position 2  SIOTS
 *  0x11200         +--------+           +--------+           +--------+  -----
 *                  |        |           |        |           |        |
 *  0x11a00  siots  +--------+<-First RL +--------+           +--------+
 *         0x11e00->|NR NR NR|           |        |           |        |  <= 1
 *  0x12200         +--------+           +--------+ First RL->+========+
 *                  |ssssssss|           |        |           |NR NR NR|  <= 2
 *  0x12a00         +--------+ Degraded->+--------+           +========+
 *                  |ssssssss|           |NR NR NR|           |        |  <= 3
 *  0x13200         +--------+<-Second RL+--------+           +--------+
 *                  |NR NR NR|           |NR NR NR|           |        |  <= 4
 *  0x13a00         +--------+           +--------+Second RL->+========+
 *                  |ssssssss|           |NR NR NR|           |NR NR NR|
 *  0x14200         +--------+           +--------+           +========+
 *         0x14600->|ssssssss|           |NR NR NR|           |        |
 *  0x14a00         +--------+           +--------+           +--------+
 *                  |        |           |NR NR NR|           |        |
 *
 *          For this example the original siots request for 0x11e00 thru
 *          0x145ff will be broken into the following siots:    
 *          SIOTS   Primary (i.e. Read)Position     Degraded Positions that Need Rebuild
 *          -----   ---------------------------     ------------------------------------
 *          1.      Position 1                      Position 0
 *          2.      Position 0                      Position 2
 *          3.      Position 0                      Position 1
 *          4.      Position 2                      Positions 0 and 1 
 *          Breaking up the I/O is required by the current xor algorithms
 *          which do not handle a degraded change within a request.
 *
 * @author
 *   08/16/00 BJP - Created.
 *   01/19/02 Rob Foley - Modified to always increment the sg desc
 *                  just before use so that the memory remains
 *                  mapped in.
 * 12/22/2009   Ron Proulx - Updated to use custom mirror rebuild request. 
 *
 ****************************************************************/
fbe_status_t fbe_xor_mirror_rebuild(fbe_xor_mirror_reconstruct_t *rebuild_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_xor_option_t    options = rebuild_p->options;
    fbe_lba_t           seed = rebuild_p->start_lba;
    fbe_block_count_t  blocks = rebuild_p->blocks;
    fbe_u32_t           width = rebuild_p->width;
    fbe_u16_t           valid_bitmask = rebuild_p->valid_bitmask;
    fbe_u16_t           needs_rebuild_bitmask = rebuild_p->needs_rebuild_bitmask;
    fbe_u32_t           num_of_valid_positions;
    fbe_u32_t           num_of_positions_to_be_rebuilt;
    fbe_u32_t           primary_position = FBE_XOR_INV_DRV_POS;
    fbe_u32_t           position_to_be_rebuilt = FBE_XOR_INV_DRV_POS;
    fbe_u32_t           parity_position = width;        /* The xor error methods require a parity position */
    fbe_u16_t           positions_bitkey[FBE_XOR_MAX_FRUS];   
    fbe_xor_sgl_descriptor_t  sgd_v[FBE_XOR_MAX_FRUS];
    fbe_u32_t           block;
    fbe_u16_t           array_position;
    fbe_sector_t       *sector_v[FBE_XOR_MAX_FRUS];    /* Sector ptr for current position. */
    fbe_xor_error_t    *eboard_p = rebuild_p->eboard_p;
    fbe_u16_t           invalid_bitmap;
    fbe_u32_t           sg_count;

    /* Validate the rebuild request.
     */
    if (XOR_COND(((status = fbe_xor_validate_mirror_rebuild(rebuild_p)) != FBE_STATUS_OK )))
    {
        /* Generate a trace and assert (checked builds).
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Validation of mirror rebuild request for 0x%llx failed with status: 0x%x \n",
                              (unsigned long long)seed,
                              status);
        return status;
    }

    /* Construct a bitmask of the valid (i.e. primary and possibly secondary)
     * positions.  If there is only (1) source position set the primary.
     */
    num_of_valid_positions = fbe_xor_get_bitcount(valid_bitmask);
    if (num_of_valid_positions == 1)
    {
        primary_position = fbe_xor_get_first_position_from_bitmask(valid_bitmask, 
                                                                   width);
    }

    /* Construct a bitmask of the rebuild positions.  If there is only (1)
     * position to rebuild we set the needs rebuild position.
     */
    num_of_positions_to_be_rebuilt = fbe_xor_get_bitcount(needs_rebuild_bitmask);
    if (num_of_positions_to_be_rebuilt == 1)
    {
        position_to_be_rebuilt = fbe_xor_get_first_position_from_bitmask(needs_rebuild_bitmask, 
                                                                         width);
    }

    /* We have already validate the request above.
     */

    /*! Now generate the array for all positions.
     *
     */
    fbe_xor_generate_positions_from_bitmask((valid_bitmask | needs_rebuild_bitmask),
                                            &positions_bitkey[0],
                                            width);

    /* Increment statistics (if enabled).
     */
    FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_R1_RB]);

    /* Setup the sgls for all positions.
     */
    for (array_position = 0; array_position < width; array_position++)
    {
        fbe_u32_t bytcnt;

        /* Similar to a mirror verify, all positions must have a valid
         * buffer.
         */
        bytcnt = FBE_XOR_SGLD_INIT_BE(sgd_v[array_position], rebuild_p->sg_p[array_position]);

        /* If the byte count isn't valid, generate a trace event and 
         * assert (checked builds).
         */
        if (XOR_COND(bytcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  " bytcnt == 0 \n");  
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Now validate that there is a valid buffer supplied.
         */
        if (XOR_COND(FBE_XOR_SGLD_ADDR(sgd_v[array_position]) == NULL))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "Buffer for position: 0x%x is NUll \n",
                                  array_position);  
            return FBE_STATUS_GENERIC_FAILURE;
        }
        sector_v[array_position] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[array_position]);

    } /* end for all array positions */

    /* Initialize the previously allocated eboard.
     */
    fbe_xor_setup_eboard(eboard_p, 0);

    /* For each block of the request execute either a verify or a rebuild.
     */
    for (block = 0; block < blocks; block++)
    {
        fbe_xor_error_t     EBoard_local;
        fbe_xor_error_t    *local_eboard_p = &EBoard_local;

        /* Initialize the local (i.e. per sector) error board.
         * Set the u_crc bit for any hard errored drives.
         */
        fbe_xor_init_eboard((fbe_xor_error_t *)&EBoard_local,
                            eboard_p->raid_group_object_id,
                            eboard_p->raid_group_offset);
        fbe_xor_setup_eboard(&EBoard_local,
                            eboard_p->hard_media_err_bitmap);

        /* The existing rebuild algorithms (i.e. fbe_xor_rebuild_r1_unit) doesn't 
         * support rebuilding more than exactly (1) position.  But since we 
         * support 3-way mirrors we must be able to rebuild more than (1) position.  
         * Therefore we use the verify algorithm if there is more than (1) position
         * to rebuild or more than (1) position to read from.
         */
        if ((num_of_valid_positions         > 1) ||
            (num_of_positions_to_be_rebuilt > 1)    )
        {
            status = fbe_xor_verify_r1_unit(&EBoard_local,
                                            sector_v,
                                            positions_bitkey,
                                            width,
                                            valid_bitmask,
                                            needs_rebuild_bitmask,
                                            seed, 
                                            options,
                                            &invalid_bitmap,
                                            rebuild_p->raw_mirror_io_b);

            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            EBoard_local.u_bitmap = invalid_bitmap;
        } 
        else
        {
            status = fbe_xor_rebuild_r1_unit(&EBoard_local,
                                             sector_v,
                                             positions_bitkey,
                                             width,
                                             FBE_FALSE,
                                             primary_position,
                                             position_to_be_rebuilt,
                                             seed,
                                             options,
                                             &invalid_bitmap,
                                             rebuild_p->raw_mirror_io_b);

            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            EBoard_local.u_bitmap = invalid_bitmap;

            /* Were any checksum errors caused by media errors?
             *
             * This is information we need to setup in the eboard,
             * and this is the logical place to determine
             * which errors were the result of media errors.
             *
             * The caller supplied us with media errored positions
             * within the hard_media_err_bitmap.
             */
            status = fbe_xor_determine_media_errors(&EBoard_local,  
                                                    eboard_p->hard_media_err_bitmap);

            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
        }

        /* If the user passed in an error region struct, then fill it out.
         */
        status = fbe_xor_save_error_region(&EBoard_local, 
                                           rebuild_p->eregions_p,
                                           FBE_TRUE, 
                                           seed,
                                           0,
                                           width,
                                           parity_position);

        if (status != FBE_STATUS_OK)
        {
            return status; 
        }

        /* Update the local eboard write bitmap (i.e. EBoard_local.w_bitmap)
         * with any positions that require a write (uncorrectable/invalidated,
         * modified, etc).  Since we need to execute this rebuild method even
         * if there are no positions to write (i.e. the rebuild request spans
         * multiple `needs rebuild regions' and some of the regions are clean) 
         * it isn't an error if both the `needs rebuild' and `needs write'
         * bitmaps are 0.
         */
        FBE_XOR_INIT_EBOARD_WMASK(local_eboard_p);
        if (XOR_COND(EBoard_local.w_bitmap == 0))
        {
            /*! @todo Need to reduce the following trace level to debug high.
             */
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                  "Informational. write bitmap: 0x%x for needs rebuild bitmap: 0x%x is 0\n",
                                  EBoard_local.w_bitmap, needs_rebuild_bitmask);
        }

        /* Update the global eboard structure */
        fbe_xor_update_global_eboard(eboard_p, &EBoard_local);


        /* We must at least set up the write bit for the degraded drive(s).
         */
        if (XOR_COND(!(eboard_p->w_bitmap & needs_rebuild_bitmask)))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "Expected write bitmap: 0x%x for needs rebuild bitmap: 0x%x \n",
                                  eboard_p->w_bitmap, needs_rebuild_bitmask);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Setup the sector vector for the next strip.
         */
        for (array_position = 0; 
            (block < (blocks - 1)) && (array_position < width); 
            array_position++)
        {
            /* Similar to a mirror verify, all positions must have a valid
             * buffer.
             */

            /* Increment for the next pass.
             */
            status = FBE_XOR_SGLD_INC_BE(sgd_v[array_position], sg_count);

            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            /* Now validate that there is a valid buffer supplied.
             */
            if (XOR_COND(FBE_XOR_SGLD_ADDR(sgd_v[array_position]) == NULL))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "Buffer after increment for position: 0x%x is NUll \n",
                                      array_position);  
                return  FBE_STATUS_GENERIC_FAILURE;
            }

            /* Update position with new buffer information.
             */
            sector_v[array_position] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[array_position]);
        }

        /* Increment seed for the next strip.
         */
        seed++;

    } /* end for each block of the request */

    /* Always return the execution status.
     */
    return(status);
}
/* end fbe_xor_mirror_rebuild() */

/*!**************************************************************
 * fbe_xor_validate_mirror_stamps()
 ****************************************************************
 * @brief
 *  Determine if the stamps are ok for a mirror.
 *
 * @param sector_p - the block we will be validating.
 * @param lba - the disk lba of this block. 
 * @param offset - The raid group offset to use to check lba stamp
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.  
 *
 * @author
 *  11/29/2011  Ron Proulx  - Created.
 *
 ****************************************************************/

fbe_status_t fbe_xor_validate_mirror_stamps(fbe_sector_t *sector_p, fbe_lba_t lba, fbe_lba_t offset)
{
    const fbe_u16_t crc = xorlib_cook_csum(xorlib_calc_csum(sector_p->data_word), 0);

    /*! @note Currently the fbe_xor_get_timestamp() is not used since there 
     *        is no way to guarantee which one is correct.  Therefore the
     *        time_stamp mut be one of the following:
     *          o 0
     *              or
     *          o FBE_SECTOR_INVALID_TSTAMP
     */
    if ((sector_p->time_stamp != 0)                         &&
        (sector_p->time_stamp != FBE_SECTOR_INVALID_TSTAMP)    )
    {
        /* time_stamp is incorrect trace the error and return failure.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "mirror: time stamp is invalid!\n");
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "time_stamp = 0x%x, lba: 0x%llx\n", 
                              sector_p->time_stamp,
                              (unsigned long long)lba);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note Currently there is only (1) value allowed for the write_stamp 
     *        on mirror raid groups:
     *          o 0
     */
     if (sector_p->write_stamp != 0)
    {
        /* write_stamp is incorrect trace the error and return failure.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "mirror: write stamp is invalid!\n");
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "write_stamp = 0x%x, lba: 0x%llx\n", 
                              sector_p->write_stamp,
                              (unsigned long long)lba);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note Only check the lba_stamp if the crc is correct since there 
     *        are error injection cases where we corrupt the lba stamp.
     */
    if (crc == sector_p->crc)
    {
        /* Only check the lba stamp since this is the only stamp that should always be 
         * valid. 
         */
        if (XOR_COND(!fbe_xor_lib_is_lba_stamp_valid(sector_p, lba, offset)))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "mirror: lba stamp is invalid!\n");
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "lba_stamp = 0x%x, lba: 0x%llx\n", 
                                  sector_p->lba_stamp,
                                  (unsigned long long)lba);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_xor_validate_mirror_stamps()
 ******************************************/

/*****************************************
 * End of fbe_xor_interface_mirror.c
 *****************************************/
