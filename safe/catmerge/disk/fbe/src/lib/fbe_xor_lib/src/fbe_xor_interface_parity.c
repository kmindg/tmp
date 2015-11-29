/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_xor_interface_parity.c
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
 *  11/4/2010 - Created. Rob Foley
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
static fbe_u32_t fbe_xor_check_parity(fbe_xor_status_t *status_p,
                                      fbe_xor_option_t option,
                                      fbe_lba_t strip,
                                      fbe_sector_t* parity_blk_ptr,
                                      fbe_u16_t mask,
                                      fbe_u16_t array_width,
                                      fbe_u32_t parity_drives,
                                      fbe_u16_t parity_bitmask);
fbe_status_t fbe_xor_get_next_descriptor(fbe_xor_rcw_write_t *const write_p,
                            fbe_u16_t fru_index,
                            fbe_lba_t strip, 
                            fbe_xor_sgl_descriptor_t *const wsgd_v_p,
                            fbe_xor_sgl_descriptor_t *const rsgd_v_p,
                            fbe_xor_sgl_descriptor_t *const r2sgd_v_p,
                            fbe_xor_sgl_descriptor_t **sgd_v_p,
                            fbe_bool_t * is_read); 

/****************************************************************
 *  fbe_xor_validate_parity_array()
 ****************************************************************
 * @brief
 *  This function validates the array of parity positions.
 *
 * @param pos_array - Array of parity positions to validate.
 *
 * @return fbe_status_t
 *
 * @author
 *  04/11/06 - Created. Rob Foley
 *
 ****************************************************************/
static __inline fbe_status_t fbe_xor_validate_parity_array( fbe_u16_t *pos_array )
{
    /* Validate positions.
     * The first parity position is always set for R5 or R6.
     */
    if (XOR_COND(pos_array[0] == FBE_XOR_INV_DRV_POS))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "uncorrect first parity position: pos_array[0] 0x%x\n",
                              pos_array[0]);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The second position is optionally set.
     * R6 always has a second parity position.
     */
    if (XOR_COND((pos_array[1] != FBE_XOR_INV_DRV_POS) &&
                 (pos_array[1] >= FBE_XOR_MAX_FRUS)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "uncorrect second parity position : pos_array[1] = 0x%x\n",
                              pos_array[1]);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/* end fbe_xor_validate_parity_array */

/****************************************************************
 *  fbe_xor_validate_rb_array()
 ****************************************************************
 * @brief
 *  This function validates the rebuild position array.
 *
 * @param pos_array - Array of rebuild positions to validate.
 *
 * @return fbe_status_t
 *
 * @author
 *  04/11/06 - Created. Rob Foley
 *
 ****************************************************************/
static __inline fbe_status_t fbe_xor_validate_rb_array( fbe_u16_t *pos_array )
{
    /* Validate rebuild positions.  For these positions we will
     * optionally have either rebuild or parity positions.
     */
    if (XOR_COND((pos_array[0] != FBE_XOR_INV_DRV_POS) &&
                 (pos_array[0] >= FBE_XOR_MAX_FRUS)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "uncorrect first rebuild position : pos_array[0] = 0x%x\n",
                              pos_array[0]);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (XOR_COND((pos_array[1] != FBE_XOR_INV_DRV_POS) &&
                 (pos_array[1] >= FBE_XOR_MAX_FRUS)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "uncorrect second rebuild position : pos_array[1] = 0x%x\n",
                              pos_array[1]);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/* end fbe_xor_validate_rb_array */

/****************************************************************
 * fbe_xor_lib_parity_mr3_write       
 ****************************************************************
 * @brief
 *  This function is called to carry out the MR3 xor operation.
 *
 * @param xor_write_p - ptr to vector structure.
 *
 * @return fbe_status_t
 *
 * @author
 *   03/23/08:  Vamsi V -- Modified to use temp buffer to accumulate
 *                         XOR of data sectors before final write to
 *                         parity sector. This strategy is used to
 *                         minimize cache pollution. This applies 
 *                         only to single parity RGs.
 *   10/xx/13:  Vamsi V -- Modified to use temp buffer for R6 also.
 *****************************************************************/
fbe_status_t fbe_xor_lib_parity_mr3_write(fbe_xor_mr3_write_t *const xor_write_p)
{
    /* Call xor_get_timestamp with thread_number to indicate
     * in which thread we are executing
     */
    fbe_u16_t time_stamp = fbe_xor_get_timestamp(0);
    fbe_u16_t data_disks = xor_write_p->data_disks;
    fbe_u32_t parity_drives = xor_write_p->parity_disks;
    fbe_status_t status;

    /* Pointer to temp buffer for accumulating row parities 
     */
    /* VV: Why are we allocating this on the stack instead of global per-core buffers? */
    FBE_ALIGN(16) fbe_sector_t row_parity_scratch;
    FBE_ALIGN(16) fbe_sector_t diag_parity_scratch;
    fbe_sector_t *row_parity_scratch_blk_ptr = &row_parity_scratch;
    fbe_sector_t *diag_parity_scratch_blk_ptr = &diag_parity_scratch;

    /* Scratch buffers need to be 16-byte aligned.
     */
    if (((fbe_ptrhld_t)row_parity_scratch_blk_ptr->data_word & 0xF) ||
        ((fbe_ptrhld_t)diag_parity_scratch_blk_ptr->data_word & 0xF))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                              "unexpected alignment of MR3 scratch buffers row:0x%x, diag:0x%x \n",
                              (fbe_u32_t)(fbe_ptrhld_t)row_parity_scratch_blk_ptr->data_word, 
                              (fbe_u32_t)(fbe_ptrhld_t)diag_parity_scratch_blk_ptr->data_word);

        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* We assume the command has been initialized before entry.
     */
    if (XOR_COND(xor_write_p->status != FBE_XOR_STATUS_INVALID))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "unexpected initialized state of xor_write_p->status 0x%x\n",
                              xor_write_p->status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    xor_write_p->status = FBE_XOR_STATUS_NO_ERROR;

    {
        fbe_xor_sgl_descriptor_t wsgd_v[FBE_XOR_MAX_FRUS];
        fbe_xor_strip_range_t rnge_v[FBE_XOR_MAX_FRUS - 1];

        /* The position of the disk passed in.
         * It is used by Evenodd algorithm.
         */
        fbe_u32_t data_pos[FBE_XOR_MAX_FRUS];

        fbe_lba_t strip;
        fbe_u16_t diskcnt;
        fbe_u16_t emptycnt;
        fbe_u32_t bytcnt;

        if (XOR_COND(data_disks < FBE_XOR_MR3_WRITE_MIN_DATA_DISKS))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "data_disks count %d less than minimum: %d\n",
                                  data_disks,
                                  FBE_XOR_MR3_WRITE_MIN_DATA_DISKS);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (XOR_COND(data_disks >= FBE_XOR_MAX_FRUS))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "data_disk count %d >= FBE_XOR_MAX_FRUS: %d\n",
                                  data_disks,
                                  FBE_XOR_MAX_FRUS);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (XOR_COND(parity_drives == 2) && XOR_COND(data_disks+1 >= FBE_XOR_MAX_FRUS))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "R6: data disk count 0x%x >= maximum supported 0x%x\n",
                                  data_disks+1,
                                  FBE_XOR_MAX_FRUS);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Copy S/G list info to local structures.
         * Save range information used to distinguish
         * pre-read data from new data.
         */
        diskcnt = data_disks;

        /* For R6 need copy S/G list info to the diagonal parity.
         */
        if (parity_drives == 2)
        {
            bytcnt = FBE_XOR_SGLD_INIT_BE(wsgd_v[diskcnt + 1 ],
                                          xor_write_p->fru[diskcnt + 1].sg);
            data_pos[diskcnt+1] = xor_write_p->fru[diskcnt+1].data_position;
        }

        for (;;)
        {
            bytcnt = FBE_XOR_SGLD_INIT_BE(wsgd_v[diskcnt],
                                          xor_write_p->fru[diskcnt].sg);
            data_pos[diskcnt] = xor_write_p->fru[diskcnt].data_position;

            if (XOR_COND(bytcnt == 0))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "bytcnt == 0\n");

                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (0 >= diskcnt--)
            {
                break;
            }

            rnge_v[diskcnt].lo = xor_write_p->fru[diskcnt].start_lda;
            rnge_v[diskcnt].hi = xor_write_p->fru[diskcnt].end_lda;
        }

        /* Process each single-sector parity 'strip'.
         */
        emptycnt = 0;

        for (strip = xor_write_p->fru[data_disks].start_lda;; strip++)
        {
            fbe_sector_t *row_parity_blk_ptr;    /* Pointer of row parity block */
            fbe_sector_t *data_blk_ptr;    /* Pointer of data block */
            fbe_sector_t *diag_parity_blk_ptr;    /* Pointer of diagonal parity block */
            fbe_u32_t pcsum;    /* Checksum for row parity */
            fbe_u32_t dcsum;    /* Checksum for data disk */
            fbe_u16_t dcsum_cooked_16;    /* Cooked data disk checksum. */
            fbe_u32_t sg_bytes;

            /* Last 2 blocks are the parity blocks for R6.
             * It is one block for the other raid type.
             */
            row_parity_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(wsgd_v[data_disks]);

            /* Start with the last data block. Its content
             * is copied into the empty parity block, which
             * initializes it for subsequent processing.
             * For R6, there are 2 parity blocks to copy to.  
             */
            diskcnt = data_disks - 1;

            data_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(wsgd_v[diskcnt]);

            /* Calculates a checksum for data within a sector using the
             * "classic" checksum algorithm; this is the first pass over
             * the data so the parity blocks are initialized based on
             * the data.
             */
            if (parity_drives == 2)
            {
                diag_parity_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(wsgd_v[data_disks+1]);
                dcsum = xorlib_calc_csum_and_init_r6_parity_to_temp(data_blk_ptr->data_word,
                                                                    row_parity_scratch_blk_ptr->data_word,
                                                                    diag_parity_scratch_blk_ptr->data_word,
                                                                    data_pos[diskcnt]);
            } 
            else
            {
                dcsum = xorlib_calc_csum_and_cpy_to_temp(data_blk_ptr->data_word,
                                                          row_parity_scratch_blk_ptr->data_word);
            }

            pcsum = dcsum;
            dcsum_cooked_16 = xorlib_cook_csum(dcsum, (fbe_u32_t)strip);           

            if (parity_drives == 2)
            {
                /* Initializes and calculates the row and diagonal parity
                 * of the 16 bit checksum value (i.e. POC). 
                 */
                fbe_xor_calc_parity_of_csum_init(&data_blk_ptr->crc,
                                                 &row_parity_scratch_blk_ptr->lba_stamp,
                                                 &diag_parity_scratch_blk_ptr->lba_stamp,
                                                 data_pos[diskcnt]);
            }

            for (;;)
            {
                if ((strip >= rnge_v[diskcnt].lo) &&
                    (strip < rnge_v[diskcnt].hi))
                {
                    /* This is data just transferred from the host.
                     */
                    if (xor_write_p->option & FBE_XOR_OPTION_CHK_CRC)
                    {
                        /* There could be multiple reasons that the checksum
                         * isn't valid.  Including the case where the sector
                         * has already been invalidated.
                         */
                        if (data_blk_ptr->crc != dcsum_cooked_16)
                        {
                            fbe_xor_status_t    xor_status;

                            /* Check and handle known data lost cases.  If an 
                             * error was injected don't check.
                             */
                            xor_status = fbe_xor_handle_bad_crc_on_write(data_blk_ptr, 
                                                                         strip, 
                                                                         xor_write_p->fru[diskcnt].bitkey,
                                                                         xor_write_p->option, 
                                                                         xor_write_p->eboard_p, 
                                                                         xor_write_p->eregions_p,
                                                                         xor_write_p->array_width);
                            fbe_xor_set_status(&xor_write_p->status, xor_status);

                            /*! @note Since we now always use the CRC from the
                             *        block being written (even if the CRC is 
                             *        invalid), there is no need to regenerate 
                             *        the PoC.
                             */
                        } /* end if the checksum is bad on write data */
                    } 
                    else
                    {
                        /*! @note Else we were not told to check the checksum.
                         *        This option is not supported (since we always
                         *        use the checksum supplied).
                         */
                        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                              "xor: obj: 0x%x strip: 0x%llx bitmap: 0x%x options: 0x%x CHK_CRC not set",
                                              fbe_xor_lib_eboard_get_raid_group_id(xor_write_p->eboard_p),
					      (unsigned long long)strip, 
                                              xor_write_p->fru[diskcnt].bitkey, xor_write_p->option);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                } 
                else if (data_blk_ptr->crc != dcsum_cooked_16)
                {
                    /* The checksum we've calculated doesn't match the checksum 
                     * attached to data read from the disk. The disk media may
                     * have gone sour; this is a rare event, but is expected to 
                     * occur over time.
                     */
                    if (!(xor_write_p->option & FBE_XOR_OPTION_ALLOW_INVALIDS))
                    {
                        fbe_xor_status_t    xor_status;

                        xor_status = fbe_xor_handle_bad_crc_on_read(data_blk_ptr, 
                                                                    strip, 
                                                                    xor_write_p->fru[diskcnt].bitkey,
                                                                    xor_write_p->option, 
                                                                    xor_write_p->eboard_p, 
                                                                    xor_write_p->eregions_p,
                                                                    xor_write_p->array_width);
                        fbe_xor_set_status(&xor_write_p->status, xor_status);

                        /*! @note Since we now always use the CRC from the block 
                         *        read from disk (even if the CRC is invalid), 
                         *        there is no need to regenerate the PoC.
                         */
                    }
                    else
                    {
                        /* Else if we are here it means that we got sectors, which
                         * were invalidated in past. As we do always compute
                         * POC from the checksum stored on sector rather than
                         * computed checksum from data content of sector, we 
                         * do not need to do anything more here.
                         */
                    }
                } 
                else if (!xorlib_is_valid_lba_stamp(&data_blk_ptr->lba_stamp, strip, xor_write_p->offset))
                {
                    /* Shed stamp does not match the lba for this sector. Treat
                     * the same as a checksum error.  We only initialize the 
                     * eboard when a checksum error is detected for performance reasons.
                     */
                    fbe_xor_lib_eboard_init(xor_write_p->eboard_p);
                    xor_write_p->eboard_p->c_crc_bitmap |= xor_write_p->fru[diskcnt].bitkey;
                    fbe_xor_record_invalid_lba_stamp(strip, xor_write_p->fru[diskcnt].bitkey, xor_write_p->eboard_p, data_blk_ptr);
                    fbe_xor_set_status(&xor_write_p->status, FBE_XOR_STATUS_CHECKSUM_ERROR);
                }

#ifdef XOR_FULL_METACHECK
                else if ((data_blk_ptr->time_stamp & FBE_SECTOR_ALL_TSTAMPS) != 0)
                {
                    /* The timestamp field in the pre-read data
                     * has the ALL_TIMESTAMPS bit set.
                     */
                    fbe_xor_set_status(&xor_write_p->status, FBE_XOR_STATUS_BAD_METADATA);
                }

                else if ((data_blk_ptr->write_stamp != 0) &&
                         (data_blk_ptr->time_stamp != FBE_SECTOR_INVALID_TSTAMP))
                {
                    /* A bit is set in the writestamp field of
                     * the pre-read data, but there is also a
                     * timestamp.
                     */
                    fbe_xor_set_status(&xor_write_p->status, FBE_XOR_STATUS_BAD_METADATA);
                }
#endif
                /*! Set the remaining (3) metadata fields for the write request.
                 *
                 *  @note We always set the metadata for write even for invalidated
                 *        sectors since only the crc is invalidated.  The remaining
                 *        metadata fields should be valid.
                 */
                data_blk_ptr->time_stamp = time_stamp;
                data_blk_ptr->write_stamp = 0;
                data_blk_ptr->lba_stamp = xorlib_generate_lba_stamp(strip, xor_write_p->offset);

                status = FBE_XOR_SGLD_INC_BE(wsgd_v[diskcnt], sg_bytes);
                if (status != FBE_STATUS_OK)
                {
                    return status; 
                }
                if (0 >= sg_bytes)
                {
                    /* That was the last block in the S/G list
                     * for this data drive.
                     */
                    emptycnt++;
                }

                if (0 >= diskcnt--)
                {
                    /* That was the final data disk to process.
                     */
                    break;
                }

                /* Move on to the next data disk. Continue
                 * to build row parity by XORing the content of
                 * the next data block into the parity.
                 */
                data_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(wsgd_v[diskcnt]);

                if (parity_drives == 2)
                {
                    if (diskcnt == 0)
                    {
                        /* XOR data block and temp buffer and copy result to 
                         * parity sector
                         */
                        dcsum = xorlib_calc_csum_and_update_r6_parity_to_temp_and_cpy(data_blk_ptr->data_word,
                                                                                      row_parity_scratch_blk_ptr->data_word,
                                                                                      diag_parity_scratch_blk_ptr->data_word,
                                                                                      data_pos[diskcnt],
                                                                                      row_parity_blk_ptr->data_word,
                                                                                      diag_parity_blk_ptr->data_word);
                    }
                    else
                    {
                        /* Use temp buffer in cache to accumulate parity of data disks
                         */
                        dcsum = xorlib_calc_csum_and_update_r6_parity_to_temp(data_blk_ptr->data_word,
                                                                              row_parity_scratch_blk_ptr->data_word,
                                                                              diag_parity_scratch_blk_ptr->data_word,
                                                                              data_pos[diskcnt]);
                    }
                } 
                else
                {
                    if (diskcnt == 0)
                    {
                        /* XOR data block and temp buffer and copy result to 
                         * parity sector
                         */
                        dcsum = xorlib_calc_csum_and_xor_with_temp_and_cpy(data_blk_ptr->data_word,
                                                                            row_parity_scratch_blk_ptr->data_word,
                                                                            row_parity_blk_ptr->data_word);
                    } 
                    else
                    {
                        /* Use temp buffer in cache to accumulate parity of data disks
                         */
                        dcsum = xorlib_calc_csum_and_xor_to_temp(data_blk_ptr->data_word,
                                                                  row_parity_scratch_blk_ptr->data_word);
                    }
                } 

                pcsum ^= dcsum;
                dcsum_cooked_16 = xorlib_cook_csum(dcsum, (fbe_u32_t)strip);

                if (parity_drives == 2)
                {
                    /* For R6, the parity drives' lba stamp will contain
                     * parity of the data drive checksums.
                     */
                    if (diskcnt == 0)
                    {
                        fbe_xor_calc_parity_of_csum_update_and_cpy(&data_blk_ptr->crc,
                                                                   &row_parity_scratch_blk_ptr->lba_stamp,
                                                                   &diag_parity_scratch_blk_ptr->lba_stamp,
                                                                   data_pos[diskcnt],
                                                                   &row_parity_blk_ptr->lba_stamp,
                                                                   &diag_parity_blk_ptr->lba_stamp);
                    }
                    else
                    {
                        fbe_xor_calc_parity_of_csum_update(&data_blk_ptr->crc,
                                                           &row_parity_scratch_blk_ptr->lba_stamp,
                                                           &diag_parity_scratch_blk_ptr->lba_stamp,
                                                           data_pos[diskcnt]);
                    }
                }
            }    /*  end for (;;) */

            /* Set up the metadata in the row parity sector.
             */
            row_parity_blk_ptr->time_stamp = time_stamp | FBE_SECTOR_ALL_TSTAMPS,
            row_parity_blk_ptr->write_stamp = 0,
            row_parity_blk_ptr->crc = xorlib_cook_csum(pcsum, (fbe_u32_t)strip);

            /* If R6, the parity drive's shed stamp will contain parity of 
             * the data drive checksums otherwise it is 0.
             */
            if (parity_drives == 1)
            {
                row_parity_blk_ptr->lba_stamp = 0;
            }

            /* If R6, set up the metadata in the diagonal parity sector.
             */
            if (parity_drives == 2)
            {
                diag_parity_blk_ptr->time_stamp = time_stamp | FBE_SECTOR_ALL_TSTAMPS,
                diag_parity_blk_ptr->write_stamp = 0,
                diag_parity_blk_ptr->crc = xorlib_cook_csum(xorlib_calc_csum
                                                             (diag_parity_blk_ptr->data_word), (fbe_u32_t)strip);
                status = FBE_XOR_SGLD_INC_BE(wsgd_v[data_disks+1], sg_bytes);
                if (status != FBE_STATUS_OK)
                {
                    return status; 
                }
                if (0 >= sg_bytes)
                {
                    /* That was the last block in the S/G list
                    * for the parity drive.
                     */
                    emptycnt++;      
                }
            }

            status = FBE_XOR_SGLD_INC_BE(wsgd_v[data_disks], sg_bytes);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            if (0 >= sg_bytes)
            {
                /* That was the last block in the S/G list
                 * for the parity drive.
                 */
                emptycnt++;
                break;
            }

        }    /* end for (strip = xor_write_p->fru[data_disks].start_lda;; strip++) */


        if (XOR_COND(emptycnt != data_disks + parity_drives))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "unexpected error: emptycnt 0x%x, data disks = 0x%x, party drives=0x%x\n",
                                  emptycnt,
                                  data_disks,
                                  parity_drives);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return FBE_STATUS_OK;
}
/* END fbe_xor_lib_parity_mr3_write() */

/*!**************************************************************
 * fbe_xor_468_check_sgd_xsum()
 ****************************************************************
 * @brief
 *  Check a scatter gather descriptor's checksums.
 *  We will optionally check the lba stamps.
 *  
 * @param sgd_p - Ptr to scatter gather descriptor to check.
 * @param blocks - Number of blocks from this sgd to check.
 * @param seed - Current lba seed.
 * @param offset - Offset to add to lba stamp
 * @param b_check_lba_stamp - FBE_TRUE to check lba stamps.
 * @param csum_status_p -  [O] True, successful, no errors.
 *                          FBE_FALSE if not successful, checksum errors.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/17/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_xor_468_check_sgd_xsum(fbe_xor_sgl_descriptor_t *const sgd_p,
                                        fbe_block_count_t blocks,
                                        fbe_lba_t seed,
                                        fbe_lba_t offset,
                                        fbe_bool_t b_check_lba_stamp,
                                        fbe_bool_t * const csum_status_p,
                                        fbe_xor_option_t option_flags )
{
    fbe_bool_t b_status = FBE_TRUE;
    fbe_block_count_t current_block;
    fbe_u32_t sg_bytes;
    fbe_status_t fbe_status;

    /* Check checksums and lba stamps on all blocks.
     */
    for (current_block = 0; current_block < blocks; current_block++)
    {
        fbe_u16_t crc;
        fbe_sector_t *blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(*sgd_p);

        /* calculate the checksum for the block.
         */
        crc = xorlib_cook_csum(xorlib_calc_csum(blk_ptr->data_word), (fbe_u32_t)seed);

        /* Check checksum and lba stamp.
         */
        if (blk_ptr->crc != crc)
        {
            /* We found a checksum error.
             */
            b_status = FBE_FALSE;
            if (!(option_flags & FBE_XOR_OPTION_ALLOW_INVALIDS))
            {
                break; 
            }
        } 
        else if (b_check_lba_stamp &&
                 !xorlib_is_valid_lba_stamp(&blk_ptr->lba_stamp, seed, offset))
        {
            /* Not a valid lba stamp, treated the same as a checksum error.
             */
            b_status = FBE_FALSE;
            break;   
        }

        fbe_status = FBE_XOR_SGLD_INC_BE(*sgd_p, sg_bytes);
        if (fbe_status != FBE_STATUS_OK)
        {
            return fbe_status; 
        }
  
        seed++;
    }    /* End for all blocks to check checksum. */

    *csum_status_p = b_status;

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_xor_468_check_sgd_xsum()
 **************************************/

/*!**************************************************************
 * fbe_xor_468_chk_leading_pre_reads()
 ****************************************************************
 * @brief
 *  Helper function to check pre-reads that are ahead of the
 *  write data.  These are pre-reads that were done for block
 *  virtualization in order to provide the pre-read data for
 *  an unaligned write.
 *  
 * @param write_p - The command structure to check.
 * @param rsgd_v - The pre-read vector.
 * @param data_pos_index - Position to check.
 * @param b_check_lba_stamp - FBE_TRUE to check LBA Stamp.
 * 
 * @return fbe_bool_t - True, successful, no errors.
 *                FBE_FALSE if not successful, checksum errors.
 *
 * @author
 *  11/17/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_xor_468_chk_leading_pre_reads(fbe_xor_468_write_t *const write_p,
                                             fbe_xor_sgl_descriptor_t *const rsgd_v,
                                             fbe_u16_t data_pos_index,
                                             fbe_bool_t b_check_lba_stamp)
{
    fbe_u16_t read_offset;
    fbe_bool_t csum_status = FBE_TRUE;
    fbe_status_t status;

    read_offset = write_p->fru[data_pos_index].read_offset;

    /* First check checksums of any read blocks before the write.
     */
    if (read_offset > 0)
    {
        /* Initialize the seed.
         */
        fbe_block_count_t write_cnt;
        fbe_block_count_t read_cnt;
        fbe_block_count_t read2_cnt;
        fbe_lba_t seed;
        read_cnt = write_p->fru[data_pos_index].read_count;
        write_cnt = write_p->fru[data_pos_index].count;

        read2_cnt = read_cnt - read_offset - write_cnt;
        seed = write_p->fru[data_pos_index].seed - read_offset;

        /* The size of the trailing read cannot be bigger than the overall size of 
         * the read since it is a subset of the read. 
         */

        if (XOR_COND(read2_cnt > read_cnt))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "trailing read size 0x%llx > total read request 0x%llx\n",
                                  (unsigned long long)read2_cnt,
                                  (unsigned long long)read_cnt);

            return FBE_FALSE;
        }

        status = fbe_xor_468_check_sgd_xsum(&rsgd_v[data_pos_index], 
                                            read_offset, 
                                            seed,
                                            write_p->offset,
                                            b_check_lba_stamp,
                                            &csum_status,
                                            write_p->option);
        if (status != FBE_STATUS_OK)
        {
            return FBE_FALSE; 
        }

        if ((csum_status == FBE_FALSE) && !(write_p->option & FBE_XOR_OPTION_ALLOW_INVALIDS))
        {
            /* Checksum error.
             */
            fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CHECKSUM_ERROR);
            return FBE_FALSE;
        }

    }    /* End if read offset not zero. */
    return FBE_TRUE;
}
/**************************************
 * end fbe_xor_468_chk_leading_pre_reads()
 **************************************/

/*!**************************************************************
 * fbe_xor_468_chk_trailing_pre_reads()
 ****************************************************************
 * @brief
 *  Helper function to check the pre-reads that are beyond
 *  the write data.  These are pre-reads that were done for
 *  block virtualization in order to provide the pre-read data
 *  for an unaligned write.
 *  
 * @param write_p - The command structure to check.
 * @param rsgd_v - The pre-read vector.
 * @param data_pos_index - Position to check.
 * @param expected_seed - Seed we expect to find for this position.
 *                        used in checked builds.
 * @param b_check_lba_stamp - FBE_TRUE to check LBA Stamp.
 * 
 * @return fbe_bool_t - True, successful, no errors.
 *                FBE_FALSE if not successful, checksum errors.
 *
 * @author
 *  11/17/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_xor_468_chk_trailing_pre_reads(fbe_xor_468_write_t *const write_p,
                                              fbe_xor_sgl_descriptor_t *const rsgd_v,
                                              fbe_u16_t data_pos_index,
                                              fbe_lba_t expected_seed,
                                              fbe_bool_t b_check_lba_stamp)
{
    fbe_block_count_t write_cnt;
    fbe_block_count_t read_cnt;
    fbe_u16_t read_offset;
    fbe_block_count_t read2_cnt;
    fbe_status_t status;
    fbe_bool_t csum_status;


    read_cnt = write_p->fru[data_pos_index].read_count;
    write_cnt = write_p->fru[data_pos_index].count;            
    read_offset = write_p->fru[data_pos_index].read_offset;
    read2_cnt = read_cnt - read_offset - write_cnt;

    /* Check checksums of any read blocks after the write.
     */
    if (read2_cnt > 0)
    {
        fbe_lba_t seed;
        /* Initialize the seed.  The read starts after the write,
         * so the seed should match the next strip.
         */
        seed = write_p->fru[data_pos_index].seed + write_cnt;
        if (XOR_COND(expected_seed != seed))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "expected seed 0x%llx != actual seed 0x%llx\n",
                                  (unsigned long long)expected_seed,
                                  (unsigned long long)seed);

            return FBE_FALSE;
        }


        /* The size of the trailing read cannot be bigger than the overall size of 
         * the read since it is a subset of the read. 
         */
        if (XOR_COND(read2_cnt > read_cnt))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "trailing read size 0x%llx > total read request 0x%llx\n",
                                  (unsigned long long)read2_cnt,
                                  (unsigned long long)read_cnt);

            return FBE_FALSE;
        }

        status = fbe_xor_468_check_sgd_xsum(&rsgd_v[data_pos_index], 
                                            read2_cnt, 
                                            seed,
                                            write_p->offset,
                                            b_check_lba_stamp,
                                            &csum_status,
                                            write_p->option);
        if (status != FBE_STATUS_OK)
        {
            return FBE_FALSE;
        }

        if ((csum_status == FBE_FALSE) && !(write_p->option & FBE_XOR_OPTION_ALLOW_INVALIDS))
        {
            /* Checksum error.
             */
            fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CHECKSUM_ERROR);
            return FBE_FALSE;
        }
    }    /* End if there are trailing pre-read blocks. */
    return FBE_TRUE;
}  
/**************************************
 * end fbe_xor_468_chk_trailing_pre_reads()
 **************************************/

/*!**************************************************************
 * fbe_xor_468_chk_trailing_parity_pre_reads()
 ****************************************************************
 * @brief
 *  Check the pre-read on parity that is after the write data.
 *  These are pre-reads that were done for block virtualization in
 *  order to provide the pre-read data for an unaligned write.
 *  
 * @param write_p - The xor command structure, which contains
 *                        information about our operation including
 *                        sg lists and counts.
 * @param pos_index - The position to check.
 * @param expected_seed - The seed where we expect to find this parity.
 *                        Used in checked builds to validate seed.
 * 
 * @return fbe_bool_t - True, successful, no errors.
 *                FBE_FALSE if not successful, checksum errors.
 *
 * @author
 *  11/17/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_xor_468_chk_trailing_parity_pre_reads(fbe_xor_468_write_t *const write_p,
                                                     fbe_u16_t pos_index,
                                                     fbe_lba_t expected_seed)
{
    fbe_block_count_t write_cnt;
    fbe_block_count_t read_cnt;
    fbe_u16_t read_offset;
    fbe_block_count_t read2_cnt;
    fbe_bool_t csum_status;
    fbe_status_t status;

    read_cnt = write_p->fru[pos_index].read_count;
    write_cnt = write_p->fru[pos_index].count;            
    read_offset = write_p->fru[pos_index].read_offset;
    read2_cnt = read_cnt - read_offset - write_cnt;

    /* Check checksums of any read blocks after the write.
     */
    if (read2_cnt > 0)
    {
        fbe_xor_sgl_descriptor_t sgd;
        fbe_lba_t seed;
        /* Initialize the seed.  The read starts after the write,
         * so the seed should match the next strip.
         */
        seed = write_p->fru[pos_index].seed + write_cnt;
        if (XOR_COND(expected_seed != seed))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "expected seed 0x%llx != actual seed 0x%llx\n",
                                  (unsigned long long)expected_seed,
                                  (unsigned long long)seed);

            return FBE_FALSE;
        }


        /* The size of the trailing read cannot be bigger than the overall size of 
         * the read since it is a subset of the read. 
         */
        if (XOR_COND(read2_cnt > read_cnt))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "trailing read size 0x%llx > total read request 0x%llx\n",
                                  (unsigned long long)read2_cnt,
                                  (unsigned long long)read_cnt);

            return FBE_FALSE;
        }

        if((read_offset + write_cnt) > FBE_U32_MAX)
        {
            /* we exceeded the 32bit limit here which is not expected
              */
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                  "%s Exceeing 32bit limit for sg lelement count:  0x%llx\n",
                  __FUNCTION__, (unsigned long long)(read_offset + write_cnt));
            return FBE_FALSE;
        }

        /* Initializing with an offset that skips over the leading read data.
         */
        FBE_XOR_SGLD_INIT_WITH_OFFSET( sgd,
                                       write_p->fru[pos_index].read_sg,
                                       (fbe_u32_t)(read_offset + write_cnt),
                                       FBE_BE_BYTES_PER_BLOCK );

        status = fbe_xor_468_check_sgd_xsum(&sgd,
                                            read2_cnt,
                                            seed,
                                            write_p->offset,
                                            FBE_FALSE    /* don't check lba stamp. */,
                                            &csum_status,
                                            write_p->option);
        if (status != FBE_STATUS_OK)
        {
            return FBE_FALSE; 
        }

        if ((csum_status == FBE_FALSE) && !(write_p->option & FBE_XOR_OPTION_ALLOW_INVALIDS))
        {
            /* Checksum error.
             */
            fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CHECKSUM_ERROR);
            return FBE_FALSE;
        }
    }    /* End if there are trailing pre-read blocks. */
    return FBE_TRUE;
}  
/**************************************
 * end fbe_xor_468_chk_trailing_parity_pre_reads()
 **************************************/
/****************************************************************
 *  fbe_xor_lib_parity_468_write       
 ****************************************************************
 *
 * @brief
 *   This function is called by xor_execute_processor_command
 *   to carry out the parallel 468 command.  
 *
 * @param write_p - ptr to struct we will be processing.
 *
 * @return
 *   FBE_STATUS_OK - no error.  write_p has the error status.
 *   FBE_STATUS_GENERIC_FAILURE - Some programming error.
 *
 * @author
 *   10/01/99:  MPG -- Created.
 *   03/23/08:  Vamsi V -- Modified to make a single function call
 *                         which XORs both old and new data into
 *                         parity. This way parity data is touched
 *                         only once. This applies only to single
 *                         parity RGs.      
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_parity_468_write(fbe_xor_468_write_t *const write_p)
{
    fbe_status_t status;

    fbe_u16_t data_disks = write_p->data_disks;
    fbe_u16_t parity_disks = write_p->parity_disks;
    fbe_u16_t array_width = write_p->array_width;  


    if (XOR_COND(data_disks == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "data_disks == 0\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }


    if (XOR_COND(data_disks >= FBE_XOR_MAX_FRUS))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "data disk count 0x%x >= maximum supported 0x%x\n",
                              data_disks,
                              FBE_XOR_MAX_FRUS);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (XOR_COND(parity_disks == 2)&&XOR_COND(data_disks+1 >= FBE_XOR_MAX_FRUS))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "R6: data disk count 0x%x >= maximum supported 0x%x\n",
                              data_disks+1,
                              FBE_XOR_MAX_FRUS);

        return FBE_STATUS_GENERIC_FAILURE;
    }



    /* We assume the command has been initialized before entry.
     */
    if (XOR_COND(write_p->status != FBE_XOR_STATUS_INVALID))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "write_p->status 0x%x != FBE_XOR_STATUS_INVALID 0x%x\n",
                              write_p->status,
                              FBE_XOR_STATUS_INVALID);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    write_p->status = FBE_XOR_STATUS_NO_ERROR;
    {
        fbe_u16_t mask_v[FBE_XOR_MAX_FRUS];
        fbe_xor_sgl_descriptor_t wsgd_v[FBE_XOR_MAX_FRUS];
        fbe_xor_sgl_descriptor_t rsgd_v[FBE_XOR_MAX_FRUS];
        fbe_lba_t seed_v[FBE_XOR_MAX_FRUS - 1];
        fbe_block_count_t blocks_remaining[FBE_XOR_MAX_FRUS - 1];

        fbe_lba_t strip;    /* lba of the current 'strip' we are working on. */
        fbe_u16_t data_pos_index;
        fbe_u16_t diskcnt;
        fbe_u16_t emptycnt;
        fbe_u32_t bytcnt;
        fbe_u32_t sg_count;


        /* The position of the disk passed in. 
         * It is used by Evenodd algorithm.
         */
        fbe_u32_t data_pos[FBE_XOR_MAX_FRUS];

        /* This loop is oddly-structured as we've
         * got to account for the different sizes
         * of the pre-read/new data vectors.
         */
        diskcnt = data_disks;

        /* For R6 only, copy write S/G list, bitmask, 
         * and data pos to the diagonal parity.
         */
        if (parity_disks == 2)
        {
            mask_v[diskcnt + 1] = write_p->fru[diskcnt + 1].position_mask;
            /* Setup write sg.
             */
            bytcnt = FBE_XOR_SGLD_INIT_WITH_OFFSET(wsgd_v[diskcnt + 1],
                                                   write_p->fru[diskcnt + 1].write_sg,
                                                   write_p->fru[diskcnt + 1].write_offset,
                                                   FBE_BE_BYTES_PER_BLOCK);
            data_pos[diskcnt + 1] = write_p->fru[diskcnt + 1].data_position;

            if (XOR_COND(bytcnt == 0))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "bytcnt == 0\n");

                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Setup read sg.
             */
            bytcnt = FBE_XOR_SGLD_INIT_BE(rsgd_v[diskcnt + 1],
                                          write_p->fru[diskcnt + 1].read_sg);

            if (XOR_COND(bytcnt == 0))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "%s bytcnt == 0\n", __FUNCTION__);

                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Check any leading pre-reads before the write.
             */
            if (fbe_xor_468_chk_leading_pre_reads(write_p,
                                                  rsgd_v, diskcnt + 1,
                                                  FBE_FALSE    /* check lba stamp */ ) == FBE_FALSE)
            {
                return FBE_STATUS_OK;
            }
        }
        /* Initialize the Row parity write S/G list, bitmask and data position.
         */
        mask_v[diskcnt] = write_p->fru[diskcnt].position_mask;
        bytcnt = FBE_XOR_SGLD_INIT_WITH_OFFSET(wsgd_v[diskcnt],
                                               write_p->fru[diskcnt].write_sg,
                                               write_p->fru[diskcnt].write_offset,
                                               FBE_BE_BYTES_PER_BLOCK);
        data_pos[diskcnt] = write_p->fru[diskcnt].data_position;

        if (XOR_COND(bytcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "bytcnt == 0\n");

            return FBE_STATUS_GENERIC_FAILURE;
        }

        bytcnt = FBE_XOR_SGLD_INIT_BE(rsgd_v[diskcnt],
                                      write_p->fru[diskcnt].read_sg);

        if (XOR_COND(bytcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "xor: %s bytcnt == 0\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Check any leading pre-reads before the write.
         */
        if (fbe_xor_468_chk_leading_pre_reads(write_p,
                                              rsgd_v, diskcnt,
                                              FBE_FALSE    /* no check lba stamp */) == FBE_FALSE)
        {
            /* Error in pre-read data, return.
             */
            return FBE_STATUS_OK;
        }
        status = FBE_XOR_SGLD_UNMAP(rsgd_v[diskcnt]);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }

        /* Loop over all data positions, initializing the 
         * mask, write scatter gather descriptor, read scatter gather descriptor and seed.
         */
        for (data_pos_index = 0; data_pos_index < diskcnt; data_pos_index++)
        {
            /* Init the mask.
             */
            mask_v[data_pos_index] = write_p->fru[data_pos_index].position_mask;

            /* Init write scatter gather descriptor.
             */
            bytcnt = FBE_XOR_SGLD_INIT_WITH_OFFSET(wsgd_v[data_pos_index],
                                                   write_p->fru[data_pos_index].write_sg,
                                                   write_p->fru[data_pos_index].write_offset,
                                                   FBE_BE_BYTES_PER_BLOCK);
            data_pos[data_pos_index] = write_p->fru[data_pos_index].data_position;

            if (XOR_COND(bytcnt == 0))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "bytcnt == 0\n");

                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Init read scatter gather descriptor.
             */
            bytcnt = FBE_XOR_SGLD_INIT_BE(rsgd_v[data_pos_index],
                                          write_p->fru[data_pos_index].read_sg);

            if (XOR_COND(bytcnt == 0))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "bytcnt == 0\n");

                return FBE_STATUS_GENERIC_FAILURE;
            }

            seed_v[data_pos_index] = write_p->fru[data_pos_index].seed;
            blocks_remaining[data_pos_index] = write_p->fru[data_pos_index].count;
            /* Check any pre-read blocks before the write. 
             */
            if (fbe_xor_468_chk_leading_pre_reads(write_p,
                                                  rsgd_v, data_pos_index,
                                                  FBE_TRUE    /* check lba stamp */) == FBE_FALSE)
            {
                /* Error in pre-read data, return.
                 */
                return FBE_STATUS_OK;
            }
            /* Make sure there is something left in the read scatter gather.
             * The above code conveniently incremented the scatter gather beyond the 
             * offset, so it is setup in the correct position for the code below. 
             */
            if (XOR_COND(rsgd_v[data_pos_index].bytcnt == 0))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "rsgd_v[%d].bytcnt == 0\n",
                                      data_pos_index);

                return FBE_STATUS_GENERIC_FAILURE;
            }
        }    /* for all data positions */

        /* Process each single-sector parity 'strip'.
         */
        emptycnt = 0;

        for (strip = write_p->fru[data_disks].seed;; strip++)
        {
            fbe_sector_t *row_parity_blk_ptr;    /* Pointer of row parity block */
            fbe_sector_t *diag_parity_blk_ptr;    /* Pointer of diagonal parity block */
            fbe_sector_t *old_data_blk_ptr;    /* Pointer of old data block */
            fbe_sector_t *new_data_blk_ptr;    /* Pointer of new data block */

            fbe_u32_t pcsum;    /* Parity block checksum */
            fbe_u32_t dcsum[2];    /* Array for Old and New Data block checksums */
            fbe_u32_t old_dcsum;    /* Old Data block checksum */
            fbe_u32_t new_dcsum;    /* New Data block checksum */
            fbe_u16_t mask;
            fbe_u16_t parity_mask;

            /* Last block is the parity block. The parity has been
             * pre-read - since we're going to update it, we need
             * to check its checksum prior to proceeding further.
             * Last 2 blocks are the parity blocks for R6.
             * It is one block for the other raid type.
             */
            diskcnt = data_disks;

            /* Create bitmask for parity drive(s).
             */
            parity_mask = mask_v[diskcnt];
            if (parity_disks == 2)
            {
                parity_mask |= mask_v[diskcnt+1];
            }

            /* Check the checksum of the row parity.
             */
            row_parity_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(wsgd_v[diskcnt]);

            mask = mask_v[diskcnt];

            pcsum = fbe_xor_check_parity(&write_p->status,
                                         write_p->option,
                                         strip,
                                         row_parity_blk_ptr,
                                         mask,
                                         array_width,
                                         parity_disks,
                                         parity_mask);

            /* If R6, check the checksum of the diagonal parity.
             */
            if (parity_disks == 2)
            {
                diag_parity_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(wsgd_v[diskcnt+1]);

                mask = mask_v[diskcnt+1];

                (void) fbe_xor_check_parity(&write_p->status,
                                            write_p->option,
                                            strip,
                                            diag_parity_blk_ptr,
                                            mask,
                                            array_width,
                                            parity_disks,
                                            parity_mask);
                if (!fbe_xor_r6_eval_parity_of_checksums(row_parity_blk_ptr,
                                                         diag_parity_blk_ptr,
                                                         array_width,
                                                         parity_disks))
                {
                    /* We took an error in evaluating the parity of checksums.
                     * This means that there is an inconsistency so we should not
                     * continue the write until we fix it.
                     */
                    fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_BAD_METADATA);
                }
            }

            /* Process old and new data.
             */
            while (0 < diskcnt--)
            {
                if (strip < seed_v[diskcnt])
                {
                    /* We haven't reached the strip where this
                     * FRU is involved, or we've passed it by
                     * forever. (See below.)
                     */
                    continue;
                }
                blocks_remaining[diskcnt]--;
                /* First process any pre-read data that starts before the write data, 
                 * checking for checksum errors. This increments the read scatter gather 
                 * descriptor past any leading pre-read data. 
                 */

                /* Load old and new data block pointers.
                 */
                old_data_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(rsgd_v[diskcnt]);
                new_data_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(wsgd_v[diskcnt]);

                /* XOR the contents of the old data block into
                 * the parity block, effectively removing it
                 * from the parity.
                 */
                if (parity_disks == 2)
                {
                    /* If R6, xor the contents of the old data block into the row 
                     * parity and diagonal parity blocks and return the 
                     * checksum of data block.
                     */
                    old_dcsum = xorlib_calc_csum_and_update_r6_parity(old_data_blk_ptr->data_word,
                                                                       row_parity_blk_ptr->data_word,
                                                                       diag_parity_blk_ptr->data_word,
                                                                       data_pos[diskcnt]);

                    /* For R6, the parity drives' shed stamp will contain
                     * parity of the data drive checksums.
                     */
                    fbe_xor_calc_parity_of_csum_update(&old_data_blk_ptr->crc,
                                                       &row_parity_blk_ptr->lba_stamp,
                                                       &diag_parity_blk_ptr->lba_stamp,
                                                       data_pos[diskcnt]);
                } 
                else
                {
                    /* If R5/R3, xor the contents of the old data block and new data 
                     * block into the row parity block and return the checksum of old
                     * and new data blocks.
                     */
                    xorlib_468_calc_csum_and_update_parity(old_data_blk_ptr->data_word,
                                                            new_data_blk_ptr->data_word,
                                                            row_parity_blk_ptr->data_word,
                                                            &dcsum[0]);
                    old_dcsum = dcsum[0];
                    new_dcsum = dcsum[1];
                }   

                pcsum ^= old_dcsum;

                /* Before we modify the meta-data values,
                 * let's check coherency with the parity.
                 */
                mask = mask_v[diskcnt];

                /* Check the checksum for the data read.
                 */
                if (old_data_blk_ptr->crc != xorlib_cook_csum(old_dcsum, (fbe_u32_t)strip))
                {
                    if (!(write_p->option & FBE_XOR_OPTION_ALLOW_INVALIDS))
                    {
                        /* The checksums don't match in the pre-read data. The 
                         * block may have gone bad on the disk.  Only set this 
                         * error if invalidated blocks are not allowed.
                         */
                        fbe_xor_status_t    xor_status;

                        xor_status = fbe_xor_handle_bad_crc_on_read(old_data_blk_ptr, 
                                                                    strip, 
                                                                    write_p->fru[diskcnt].position_mask,
                                                                    write_p->option, 
                                                                    write_p->eboard_p, 
                                                                    write_p->eregions_p,
                                                                    write_p->array_width);
                        fbe_xor_set_status(&write_p->status, xor_status);
                    }
                } 
                else if (!xorlib_is_valid_lba_stamp(&old_data_blk_ptr->lba_stamp, strip, write_p->offset))
                {
                    /* Shed stamp does not match the lba for this sector. Treat
                     * the same as a checksum error.  We only initialize the 
                     * eboard when a checksum error is detected for performance reasons.
                     */
                    fbe_xor_lib_eboard_init(write_p->eboard_p);
                    write_p->eboard_p->c_crc_bitmap |= write_p->fru[diskcnt].position_mask;
                    fbe_xor_record_invalid_lba_stamp(strip, write_p->fru[diskcnt].position_mask, write_p->eboard_p, old_data_blk_ptr);
                    fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CHECKSUM_ERROR);
                }
#ifdef XOR_FULL_METACHECK
                else if ((old_data_blk_ptr->write_stamp != 0) &&
                         (old_data_blk_ptr->write_stamp != mask))
                {
                    /* Write stamps of the pre-read data which
                     * shouldn't be set.
                     */
                    fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_BAD_METADATA);
                }

                else if ((old_data_blk_ptr->time_stamp & FBE_SECTOR_ALL_TSTAMPS) != 0)
                {
                    /* The timestamp field in the pre-read data has
                     * the ALL_TIMESTAMPS bit set.
                     */
                    fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_BAD_METADATA);
                }
#endif 
                else if ((old_data_blk_ptr->write_stamp !=
                          (row_parity_blk_ptr->write_stamp & mask)) ||
                         ((parity_disks == 2)&& (old_data_blk_ptr->write_stamp !=
                                                 (diag_parity_blk_ptr->write_stamp & mask))))
                {
                    /* The writestamps don't match! The data
                     * is inconsistent with the parity.
                     */
                    fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CONSISTENCY_ERROR);
                }

                else if (((old_data_blk_ptr->time_stamp == FBE_SECTOR_INVALID_TSTAMP) &&
                          ((row_parity_blk_ptr->time_stamp & FBE_SECTOR_ALL_TSTAMPS) != 0)) ||
                         ((parity_disks == 2) &&
                          (old_data_blk_ptr->time_stamp == FBE_SECTOR_INVALID_TSTAMP) &&
                          ((diag_parity_blk_ptr->time_stamp & FBE_SECTOR_ALL_TSTAMPS) != 0)))
                {
                    /* The ALL_TIMESTAMPS bit is set in the
                     * parity, but the pre-read data has the
                     * INVALID_TIMESTAMP value.
                     */
                    fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CONSISTENCY_ERROR);
                }

                else if (((old_data_blk_ptr->time_stamp != FBE_SECTOR_INVALID_TSTAMP) &&
                          (old_data_blk_ptr->time_stamp !=
                           (row_parity_blk_ptr->time_stamp & ~FBE_SECTOR_ALL_TSTAMPS))) ||
                         ((parity_disks == 2) &&
                          (old_data_blk_ptr->time_stamp != FBE_SECTOR_INVALID_TSTAMP) &&
                          (old_data_blk_ptr->time_stamp !=
                           (diag_parity_blk_ptr->time_stamp & ~FBE_SECTOR_ALL_TSTAMPS))))
                {
                    /* The pre-read data has a valid timestamp
                     * which doesn't match the timestamp on the
                     * parity drive (or the pre-read data has
                     * the ALL_TIMESTAMPS bit set).
                     */
                    fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CONSISTENCY_ERROR);
                }
                else if ( (write_p->flags & FBE_XOR_468_FLAG_BVA_WRITE) &&
                         ( (row_parity_blk_ptr->time_stamp & FBE_SECTOR_ALL_TSTAMPS) ||
                           ((parity_disks == 2) &&
                            (diag_parity_blk_ptr->time_stamp & FBE_SECTOR_ALL_TSTAMPS)) ) )
                {
                    /* If parity has the all bit set and this is a BVA, then we will always
                     * force the full verify for BVA write to occur. 
                     * This solves the case where an incomplete MR3 followed by a partial stripe 468 
                     * clears the all bit and prevents us from detecting the incomplete write if 
                     * we go degraded. 
                     */
                    fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CONSISTENCY_ERROR);
                }

                /* Proceed with the new data block. XOR the
                 * content of the new data into the parity.
                 */

                if (parity_disks == 2)
                {
                    /* If R6, xor the contents of the new data block into the row 
                     * parity and diagonal parity blocks and return the 
                     * checksum of data block.
                     */
                    new_dcsum = xorlib_calc_csum_and_update_r6_parity(new_data_blk_ptr->data_word,
                                                                      row_parity_blk_ptr->data_word,
                                                                      diag_parity_blk_ptr->data_word,
                                                                      data_pos[diskcnt]);
                }

                /* Incase of single parity RG, new_dcsum is already handeled in the prior 
                 * call to fbe_xor_468_calc_csum_and_update_parity()
                 */
                pcsum ^= new_dcsum;

                /* Generate the expected checksum and compare it the actual.
                 */
                new_dcsum = xorlib_cook_csum(new_dcsum, (fbe_u32_t)strip);

                /* It is expected that all clients have generated the checksum.
                 */
                 if (!(write_p->option & FBE_XOR_OPTION_CHK_CRC))
                 {
                     /*! @note We were not told to check the checksum.
                      *        This option is not supported (since we always
                      *        use the checksum supplied).
                      */
                     fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                           "xor: obj: 0x%x strip: 0x%llx bitmap: 0x%x options: 0x%x CHK_CRC not set",
                                           fbe_xor_lib_eboard_get_raid_group_id(write_p->eboard_p),
					   (unsigned long long)strip, 
                                           write_p->fru[diskcnt].position_mask, write_p->option);
                     return FBE_STATUS_GENERIC_FAILURE;
                 }

                 /* Now compare the checksum in the data block against the
                  * calculated.  Only of it fails do we check it against the
                  * known invalid patterns.
                  */
                 if (new_data_blk_ptr->crc != new_dcsum)
                 {
                     fbe_xor_status_t    xor_status;

                     /* Check and handle known data lost cases.  If an error was 
                      * injected don't check.
                      */
                     xor_status = fbe_xor_handle_bad_crc_on_write(new_data_blk_ptr, 
                                                                  strip, 
                                                                  write_p->fru[diskcnt].data_position,
                                                                  write_p->option, 
                                                                  write_p->eboard_p, 
                                                                  write_p->eregions_p,
                                                                  write_p->array_width);
                     fbe_xor_set_status(&write_p->status, xor_status);
                 } /* end if checksums don't match */


                /*! Set up the metadata in the new data. Flip the appropriate
                 *  bit in the data/parity writestamps.
                 *  
                 *  @note We update the stamps even if the sector was `invalidated'.
                 */
                new_data_blk_ptr->lba_stamp = xorlib_generate_lba_stamp(strip, write_p->offset);
                new_data_blk_ptr->time_stamp = FBE_SECTOR_INVALID_TSTAMP;
                new_data_blk_ptr->write_stamp = (mask & (row_parity_blk_ptr->write_stamp ^= mask));

                if (parity_disks == 2)
                {
                    /* For R6, the parity drives' shed stamp will contain
                     * parity of the data drive checksums.
                     */
                    fbe_xor_calc_parity_of_csum_update(&new_data_blk_ptr->crc,
                                                       &row_parity_blk_ptr->lba_stamp,
                                                       &diag_parity_blk_ptr->lba_stamp,
                                                       data_pos[diskcnt]);
                }

                /* We cannot touch the sg beyond the write data since this might be a host sg 
                 * and host SGs are not guaranteed to be null terminated. 
                 */
                if (blocks_remaining[diskcnt] == 0)
                {
                    sg_count = 0;
                }
                else
                {
                    status = FBE_XOR_SGLD_INC_BE(wsgd_v[diskcnt], sg_count);
                    if (status != FBE_STATUS_OK)
                    {
                        return status; 
                    }
                }

                if (0 >= sg_count)
                {
                    /* No more blocks for this data disk.
                     */
                    emptycnt++;
                }

                /* If we have no blocks left on the read or if 
                 * the blocks on the write ended, check if we have 
                 * trailing read blocks to check the checksums on. 
                 */
                status = FBE_XOR_SGLD_INC_BE(rsgd_v[diskcnt], sg_count);
                if (status != FBE_STATUS_OK)
                {
                    return status;
                }

                if ((0 >= sg_count) ||
                    (emptycnt % 2))
                {
                    /* No more blocks for this data disk.
                     */
                    emptycnt++;
                    if (fbe_xor_468_chk_trailing_pre_reads(write_p,
                                                           rsgd_v,
                                                           diskcnt,
                                                           strip + 1,    /* expected seed */
                                                           FBE_TRUE    /* check lba stamp */) == FBE_FALSE)
                    {
                        /* We encountered an error in the pre-read data, return.
                         */
                        return FBE_STATUS_OK;
                    }
                    /* Reset the seed so impossibly high that
                     * we'll not look at this disk again.
                     */
                    seed_v[diskcnt] = (fbe_lba_t) - 1;
                }

                if (XOR_COND(0 != emptycnt % 2))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "0 != emptycnt %% 2. emptycnt is 0x%x\n",
                                          emptycnt);

                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }    /*  while (0 < diskcnt--) */

            /* Remove the ALL_TIMESTAMPS flag from the parity,
             * and set the new checksum.
             */
            if ( parity_disks == 2 && 
                 (row_parity_blk_ptr->time_stamp == FBE_SECTOR_INVALID_TSTAMP ) )
            {
                /* Raid 6 uses its own invalid timestamp so we know that
                 * the stripe contains data.
                 */
                row_parity_blk_ptr->time_stamp = FBE_SECTOR_R6_INVALID_TSTAMP;
            } 
            else
            {
                row_parity_blk_ptr->time_stamp &= FBE_SECTOR_INVALID_TSTAMP;
            }

            /* If R6, the parity drive's shed stamp will contain parity of 
             * the data drive checksums otherwise it is 0.
             */
            if (parity_disks == 1)
            {
                row_parity_blk_ptr->lba_stamp = 0;
            }

            row_parity_blk_ptr->crc = xorlib_cook_csum(pcsum, (fbe_u32_t)strip);

            if (parity_disks == 2)
            {
                /* If R6, set the stamps for diagonal parity block.
                 */
                if (diag_parity_blk_ptr->time_stamp == FBE_SECTOR_INVALID_TSTAMP)
                {
                    /* Raid 6 uses its own invalid timestamp so we know that
                     * the stripe contains data.
                     */
                    diag_parity_blk_ptr->time_stamp = FBE_SECTOR_R6_INVALID_TSTAMP;
                } 
                else
                {
                    diag_parity_blk_ptr->time_stamp &= FBE_SECTOR_INVALID_TSTAMP;
                }
                diag_parity_blk_ptr->write_stamp = row_parity_blk_ptr->write_stamp;
                diag_parity_blk_ptr->crc = 
                xorlib_cook_csum(xorlib_calc_csum                    
                                  (diag_parity_blk_ptr->data_word),
                                  (fbe_u32_t)strip);

                status = FBE_XOR_SGLD_INC_BE(wsgd_v[data_disks+1], sg_count);
                if (status != FBE_STATUS_OK)
                {
                    return status; 
                }
                if (0 >= sg_count || (strip + 1) >= (write_p->fru[data_disks+1].seed + write_p->fru[data_disks+1].count))
                {
                    /* No more blocks for the parity disk.
                     */
                    emptycnt++;

                    if (fbe_xor_468_chk_trailing_parity_pre_reads(
                                                                 write_p,
                                                                 data_disks + 1,
                                                                 strip + 1    /* expected seed */) == FBE_FALSE)
                    {
                        /* We encountered an error in the pre-read data, return.
                         */
                        return FBE_STATUS_OK;
                    }
                }    /* end if we are out of diagonal parity data. */

            }    /* end if parity disks == 2 */

            status = FBE_XOR_SGLD_INC_BE(wsgd_v[data_disks], sg_count);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            if ((sg_count == 0) || (strip + 1) >= (write_p->fru[data_disks].seed + write_p->fru[data_disks].count))
            {
                /* No more blocks for the parity disk.
                 */
                emptycnt++;

                if (fbe_xor_468_chk_trailing_parity_pre_reads(
                                                             write_p,
                                                             data_disks,
                                                             strip + 1    /* expected seed */) == FBE_FALSE)
                {
                    /* We encountered an error in the pre-read data, return.
                     */
                    return FBE_STATUS_OK;
                }
                break;
            }    /* end if out of row parity data. */
        }    /* for (strip = write_p->fru[data_disks].seed;; strip++)*/
    }
    return FBE_STATUS_OK;
}
/* END fbe_xor_lib_parity_468_write() */

/*!**************************************************************
 * fbe_xor_rcw_chk_leading_pre_reads()
 ****************************************************************
 * @brief
 *  Helper function to check pre-reads that are ahead of the
 *  write data.  These are pre-reads that were done for block
 *  virtualization in order to provide the pre-read data for
 *  an unaligned write.
 *  
 * @param write_p - The command structure to check.
 * @param rsgd_v - The pre-read vector.
 * @param data_pos_index - Position to check.
 * @param b_check_lba_stamp - FBE_TRUE to check LBA Stamp.
 * 
 * @return fbe_bool_t - True, successful, no errors.
 *                FBE_FALSE if not successful, checksum errors.
 *
 * @author
 *  4/7/2014 - Created. Deanna Heng
 *
 ****************************************************************/

fbe_bool_t fbe_xor_rcw_chk_aligned_pre_reads(fbe_xor_rcw_write_t *const write_p,
                                             fbe_xor_sgl_descriptor_t *const rsgd_v,
                                           
                                             fbe_lba_t seed, 
                                             fbe_u32_t read_offset,
                                             fbe_bool_t b_check_lba_stamp)
{
    fbe_bool_t csum_status;
    fbe_status_t status;

    /* First check checksums of any read blocks before the write.
     */
    if (read_offset > 0)
    {

        status = fbe_xor_468_check_sgd_xsum(rsgd_v, 
                                            read_offset, 
                                            seed,
                                            write_p->offset,
                                            b_check_lba_stamp,
                                            &csum_status,
                                            write_p->option);
        if (status != FBE_STATUS_OK)
        {
            return FBE_FALSE; 
        }

        if ((csum_status == FBE_FALSE) && !(write_p->option & FBE_XOR_OPTION_ALLOW_INVALIDS))
        {
            /* Checksum error.
             */
            fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CHECKSUM_ERROR);
            return FBE_FALSE;
        }
    }    /* End if read offset not zero. */
    return FBE_TRUE;
}
/**************************************
 * end fbe_xor_rcw_chk_leading_pre_reads()
 **************************************/

/****************************************************************
 *  fbe_xor_lib_parity_rcw_write       
 ****************************************************************
 *
 * @brief
 *   Carry out the rcw command.  
 *
 * @param write_p - ptr to struct we will be processing.
 *
 * @return
 *   FBE_STATUS_OK - no error.  write_p has the error status.
 *   FBE_STATUS_GENERIC_FAILURE - Some programming error.
 *
 * @author
 *   10/18/2013:  Deanna Heng -- Created.
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_parity_rcw_write(fbe_xor_rcw_write_t *const write_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_u16_t data_disks = write_p->data_disks;
    fbe_u16_t parity_disks = write_p->parity_disks;
    fbe_u16_t array_width = write_p->array_width;  
    /* Pointer to temp buffer for accumulating row parities 
     */
    fbe_u16_t mask_v[FBE_XOR_MAX_FRUS];
    fbe_xor_sgl_descriptor_t wsgd_v[FBE_XOR_MAX_FRUS];
    fbe_xor_sgl_descriptor_t rsgd_v[FBE_XOR_MAX_FRUS];
    fbe_xor_sgl_descriptor_t r2sgd_v[FBE_XOR_MAX_FRUS];
    fbe_xor_sgl_descriptor_t * sgd_v_p;
    fbe_lba_t seed_v[FBE_XOR_MAX_FRUS - 1];
    fbe_block_count_t blocks_remaining[FBE_XOR_MAX_FRUS - 1];

    fbe_lba_t strip;    /* lba of the current 'strip' we are working on. */
    fbe_u16_t data_pos_index;
    fbe_u16_t diskcnt;
    fbe_u16_t emptycnt;
    fbe_u32_t bytcnt;
    fbe_lba_t last_strip;
    fbe_u16_t parity_mask = 0;

    /* The position of the disk passed in. 
     * It is used by Evenodd algorithm.
     */
    fbe_u32_t data_pos[FBE_XOR_MAX_FRUS];

    /* Pointer to temp buffer for accumulating row parities 
     */
    FBE_ALIGN(16) fbe_sector_t row_parity_scratch;
    FBE_ALIGN(16) fbe_sector_t diag_parity_scratch;
    fbe_sector_t *row_parity_scratch_blk_ptr = &row_parity_scratch;
    fbe_sector_t *diag_parity_scratch_blk_ptr = &diag_parity_scratch;

    /* Scratch buffers need to be 16-byte aligned.
     */
    if (((fbe_ptrhld_t)row_parity_scratch_blk_ptr->data_word & 0xF) ||
        ((fbe_ptrhld_t)diag_parity_scratch_blk_ptr->data_word & 0xF))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                              "unexpected alignment of RCW scratch buffers row:0x%x, diag:0x%x \n",
                              (fbe_u32_t)(fbe_ptrhld_t)row_parity_scratch_blk_ptr->data_word, 
                              (fbe_u32_t)(fbe_ptrhld_t)diag_parity_scratch_blk_ptr->data_word);

        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* This loop is oddly-structured as we've
     * got to account for the different sizes
     * of the pre-read/new data vectors.
     */
    diskcnt = data_disks;

    if (XOR_COND(data_disks == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "data_disks == 0\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (XOR_COND(data_disks >= FBE_XOR_MAX_FRUS))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "data disk count 0x%x >= maximum supported 0x%x\n",
                              data_disks,
                              FBE_XOR_MAX_FRUS);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (XOR_COND(parity_disks == 2)&&XOR_COND(data_disks+1 >= FBE_XOR_MAX_FRUS))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "R6: data disk count 0x%x >= maximum supported 0x%x\n",
                              data_disks+1,
                              FBE_XOR_MAX_FRUS);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We assume the command has been initialized before entry.
     */
    if (XOR_COND(write_p->status != FBE_XOR_STATUS_INVALID))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "write_p->status 0x%x != FBE_XOR_STATUS_INVALID 0x%x\n",
                              write_p->status,
                              FBE_XOR_STATUS_INVALID);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    write_p->status = FBE_XOR_STATUS_NO_ERROR;

    /* Initialize the Row parity write S/G list, bitmask and data position.
     */
    mask_v[data_disks] = write_p->fru[data_disks].position_mask;
    parity_mask = write_p->fru[data_disks].position_mask;
    bytcnt = FBE_XOR_SGLD_INIT_WITH_OFFSET(wsgd_v[diskcnt],
                                           write_p->fru[diskcnt].write_sg,
                                           write_p->fru[diskcnt].write_offset,
                                           FBE_BE_BYTES_PER_BLOCK);
    data_pos[data_disks] = write_p->fru[data_disks].data_position;

    if (XOR_COND(bytcnt == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "bytcnt == 0\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }


    bytcnt = FBE_XOR_SGLD_INIT_BE(rsgd_v[data_disks],
                                  write_p->fru[data_disks].read_sg); 
    if (!fbe_xor_rcw_chk_aligned_pre_reads(write_p, 
                                      &rsgd_v[data_disks],
                                      write_p->fru[data_disks].seed - write_p->fru[data_disks].write_offset, 
                                      write_p->fru[data_disks].write_offset,
                                      FBE_FALSE)) 
    {
        return FBE_STATUS_OK;
    }


    if (XOR_COND(bytcnt == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "xor: %s bytcnt == 0\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* For R6 only, copy write S/G list, bitmask, 
     * and data pos to the diagonal parity.
     */
    if (parity_disks == 2)
    {
        mask_v[data_disks + 1] = write_p->fru[data_disks + 1].position_mask;
        parity_mask |= write_p->fru[data_disks + 1].position_mask;
        /* Setup write sg.
         */
        bytcnt = FBE_XOR_SGLD_INIT_WITH_OFFSET(wsgd_v[diskcnt + 1],
                                               write_p->fru[diskcnt + 1].write_sg,
                                               write_p->fru[diskcnt + 1].write_offset,
                                               FBE_BE_BYTES_PER_BLOCK);
        data_pos[data_disks + 1] = write_p->fru[data_disks + 1].data_position;

        if (XOR_COND(bytcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "bytcnt == 0\n");

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (write_p->fru[diskcnt +1].write_offset)
        {

            bytcnt = FBE_XOR_SGLD_INIT_BE(rsgd_v[diskcnt +1], 
                                      write_p->fru[diskcnt +1].read_sg); 
            if (!fbe_xor_rcw_chk_aligned_pre_reads(write_p,
                                          &rsgd_v[diskcnt +1],
                                          write_p->fru[diskcnt +1].seed - write_p->fru[diskcnt +1].write_offset, 
                                          write_p->fru[diskcnt +1].write_offset,
                                          FBE_FALSE)) 
            {
                 return FBE_STATUS_OK;
            }
        }
    }

    /* Loop over all data positions, initializing the 
     * mask, write scatter gather descriptor, read scatter gather descriptor and seed.
     */
    for (data_pos_index = 0; data_pos_index < data_disks; data_pos_index++)
    {
        /* Init the mask.
         */
        mask_v[data_pos_index] = write_p->fru[data_pos_index].position_mask;
        data_pos[data_pos_index] = write_p->fru[data_pos_index].data_position;

        /* Init read scatter gather descriptor.
         */
        if (write_p->fru[data_pos_index].read_count) {
            bytcnt = FBE_XOR_SGLD_INIT_BE(rsgd_v[data_pos_index],
                                          write_p->fru[data_pos_index].read_sg);
        }

        
        /* Init write scatter gather descriptor.
         */
        if (write_p->fru[data_pos_index].write_count) {
            fbe_lba_t write_seed = write_p->fru[data_pos_index].seed + write_p->fru[data_pos_index].read_count;
            if (write_p->fru[data_pos_index].write_offset &&
                write_p->fru[data_pos_index].read_count == 0)
            {
                
                bytcnt = FBE_XOR_SGLD_INIT_BE(wsgd_v[data_pos_index], 
                                              write_p->fru[data_pos_index].write_sg);
                if (!fbe_xor_rcw_chk_aligned_pre_reads(write_p,
                                                  &wsgd_v[data_pos_index],
                                                  write_seed - write_p->fru[data_pos_index].write_offset, 
                                                  write_p->fru[data_pos_index].write_offset,
                                                  FBE_TRUE)) 
                {
                     return FBE_STATUS_OK;
                }
            } 
            else
            {
                bytcnt = FBE_XOR_SGLD_INIT_WITH_OFFSET(wsgd_v[data_pos_index],
                                                       write_p->fru[data_pos_index].write_sg,
                                                       write_p->fru[data_pos_index].write_offset,
                                                       FBE_BE_BYTES_PER_BLOCK);
            }
        }
        
        /* Init read2 scatter gather descriptor.
         */
        if (write_p->fru[data_pos_index].read2_count) {
            bytcnt = FBE_XOR_SGLD_INIT_BE(r2sgd_v[data_pos_index],
                                          write_p->fru[data_pos_index].read2_sg);
 
        } 
        else if (write_p->fru[data_pos_index].write_end_offset)
        {
            fbe_lba_t read2_seed = write_p->fru[data_pos_index].seed + 
                                   write_p->fru[data_pos_index].read_count + 
                                   write_p->fru[data_pos_index].write_count;
            bytcnt = FBE_XOR_SGLD_INIT_BE(r2sgd_v[data_pos_index],
                                          write_p->fru[data_pos_index].read2_sg);
            if (!fbe_xor_rcw_chk_aligned_pre_reads(write_p,
                                              &r2sgd_v[data_pos_index],
                                              read2_seed, 
                                              write_p->fru[data_pos_index].write_end_offset,
                                              FBE_TRUE))
            {
                return FBE_STATUS_OK;

            }
        }
        seed_v[data_pos_index] = write_p->fru[data_pos_index].seed;
        blocks_remaining[data_pos_index] = write_p->fru[data_pos_index].count;

    }    

    /* Process each single-sector parity 'strip'.
     */
    emptycnt = 0;
    last_strip = write_p->fru[data_disks].seed + write_p->fru[data_disks].count;
    for (strip = write_p->fru[data_disks].seed; strip < last_strip; strip++)
    {
        fbe_sector_t *row_parity_blk_ptr;    /* Pointer of row parity block */
        fbe_sector_t *diag_parity_blk_ptr;    /* Pointer of diagonal parity block */
        fbe_sector_t *data_blk_ptr;    /* Pointer of data block */

        fbe_u32_t pcsum;    /* Parity block checksum */
        fbe_u32_t dcsum;    /* Data block checksum */
        fbe_u16_t dcsum_cooked_16;    /* Cooked data disk checksum. */
        fbe_u16_t mask = mask_v[data_disks]; /* mask of each data position */
        fbe_u32_t sg_bytes; 
        fbe_bool_t is_read = FBE_TRUE; /* whether the data block for this strip is a read */

        /* Last 2 blocks are the parity blocks for R6.
         * It is one block for the other raid type.
         */
        row_parity_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(wsgd_v[data_disks]);

        pcsum = fbe_xor_check_parity(&write_p->status,
                                     write_p->option,
                                     strip,
                                     row_parity_blk_ptr,
                                     mask,
                                     array_width,
                                     parity_disks,
                                     parity_mask);

        /* Process the data block for each disk
         */
        for (diskcnt = 0; diskcnt < data_disks; diskcnt++) {
            status = fbe_xor_get_next_descriptor(write_p, diskcnt, strip,
                                                &wsgd_v[diskcnt], &rsgd_v[diskcnt],
                                                &r2sgd_v[diskcnt],
                                                &sgd_v_p, &is_read);

            data_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(*sgd_v_p);

            /* Copy data over to parity if this is the first block
             */
            if (diskcnt == 0) {
               
                /* Calculates a checksum for data within a sector using the
                 * "classic" checksum algorithm; this is the first pass over
                 * the data so the parity blocks are initialized based on
                 * the data.
                 */
                if (parity_disks == 2)
                {
                    diag_parity_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(wsgd_v[data_disks+1]);
                    dcsum = xorlib_calc_csum_and_init_r6_parity_to_temp(data_blk_ptr->data_word,
                                                                    row_parity_scratch_blk_ptr->data_word,
                                                                    diag_parity_scratch_blk_ptr->data_word,
                                                                    data_pos[diskcnt]);
                } 
                else
                {
                    dcsum = xorlib_calc_csum_and_cpy_to_temp(data_blk_ptr->data_word,
                                                             row_parity_scratch_blk_ptr->data_word);
                }

                pcsum = dcsum;
                dcsum_cooked_16 = xorlib_cook_csum(dcsum, (fbe_u32_t)strip);  
        
                if (parity_disks == 2)
                {
                    fbe_xor_calc_parity_of_csum_init(&data_blk_ptr->crc,
                                                 &row_parity_scratch_blk_ptr->lba_stamp,
                                                 &diag_parity_scratch_blk_ptr->lba_stamp,
                                                 data_pos[diskcnt]);
                }
            } 
            else 
            {
                /* Continue to build row parity by XORing the content of
                 * the next data block into the parity.
                 */
                if (parity_disks == 2)
                {

                    /* if this is the last disk to xor copy it to the row parity 
                     */
                    if (diskcnt == data_disks-1)
                    {
                        dcsum = xorlib_calc_csum_and_update_r6_parity_to_temp_and_cpy(data_blk_ptr->data_word,
                                                                                      row_parity_scratch_blk_ptr->data_word,
                                                                                      diag_parity_scratch_blk_ptr->data_word,
                                                                                      data_pos[diskcnt],
                                                                                      row_parity_blk_ptr->data_word,
                                                                                      diag_parity_blk_ptr->data_word);
                    } 
                    else 
                    {
                        /* Use temp buffer in cache to accumulate parity of data disks
                         */
                        dcsum = xorlib_calc_csum_and_update_r6_parity_to_temp(data_blk_ptr->data_word,
                                                                              row_parity_scratch_blk_ptr->data_word,
                                                                              diag_parity_scratch_blk_ptr->data_word,
                                                                              data_pos[diskcnt]);
                    }

                } 
                else
                {
                    /* if this is the last disk to xor copy it to the row parity 
                     */
                    if (diskcnt == data_disks-1)
                    {
                        /* XOR data block and temp buffer and copy result to 
                         * parity sector
                         */
                        dcsum = xorlib_calc_csum_and_xor_with_temp_and_cpy(data_blk_ptr->data_word,
                                                                           row_parity_scratch_blk_ptr->data_word,
                                                                           row_parity_blk_ptr->data_word);
                    } 
                    else
                    {
                        /* Use temp buffer in cache to accumulate parity of data disks
                         */
                        dcsum = xorlib_calc_csum_and_xor_to_temp(data_blk_ptr->data_word,
                                                                 row_parity_scratch_blk_ptr->data_word);
                    }
                } 
    
                pcsum ^= dcsum;
                dcsum_cooked_16 = xorlib_cook_csum(dcsum, (fbe_u32_t)strip);

                
                if (parity_disks == 2)
                {
                    /* For R6, the parity drives' lba stamp will contain
                     * parity of the data drive checksums.
                     */
                    if (diskcnt == data_disks-1)
                    {
                        fbe_xor_calc_parity_of_csum_update_and_cpy(&data_blk_ptr->crc,
                                                                   &row_parity_scratch_blk_ptr->lba_stamp,
                                                                   &diag_parity_scratch_blk_ptr->lba_stamp,
                                                                   data_pos[diskcnt],
                                                                   &row_parity_blk_ptr->lba_stamp,
                                                                   &diag_parity_blk_ptr->lba_stamp);
                    }
                    else
                    {
                        fbe_xor_calc_parity_of_csum_update(&data_blk_ptr->crc,
                                                           &row_parity_scratch_blk_ptr->lba_stamp,
                                                           &diag_parity_scratch_blk_ptr->lba_stamp,
                                                           data_pos[diskcnt]);
                    }
                }
            }

            if (data_blk_ptr->crc != dcsum_cooked_16)
            {
                /* The checksum we've calculated doesn't match the checksum 
                 * attached to data read from the disk. The disk media may
                 * have gone sour; this is a rare event, but is expected to 
                 * occur over time.
                 */
                if (!(write_p->option & FBE_XOR_OPTION_ALLOW_INVALIDS))
                {
                    fbe_xor_status_t    xor_status;
                    if (is_read) 
                    {
                        xor_status = fbe_xor_handle_bad_crc_on_read(data_blk_ptr, 
                                                                    strip, 
                                                                    write_p->fru[diskcnt].position_mask,
                                                                    write_p->option, 
                                                                    write_p->eboard_p, 
                                                                    write_p->eregions_p,
                                                                    write_p->array_width);
                    } 
                    else 
                    {
                        xor_status = fbe_xor_handle_bad_crc_on_write(data_blk_ptr, 
                                                                     strip, 
                                                                     write_p->fru[diskcnt].position_mask,
                                                                     write_p->option, 
                                                                     write_p->eboard_p, 
                                                                     write_p->eregions_p,
                                                                     write_p->array_width);
                    }

                    fbe_xor_set_status(&write_p->status, xor_status);

                    /*! @note Since we now always use the CRC from the block 
                     *        read from disk (even if the CRC is invalid), 
                     *        there is no need to regenerate the PoC.
                     */
                }
                else
                {
                    /* Else if we are here it means that we got sectors, which
                     * were invalidated in past. As we do always compute
                     * POC from the checksum stored on sector rather than
                     * computed checksum from data content of sector, we 
                     * do not need to do anything more here.
                     */
                }
            } 
            else if (is_read && !xorlib_is_valid_lba_stamp(&data_blk_ptr->lba_stamp, strip, write_p->offset))
            {
                /* Shed stamp does not match the lba for this sector. Treat
                 * the same as a checksum error.  We only initialize the 
                 * eboard when a checksum error is detected for performance reasons.
                 */
                fbe_xor_lib_eboard_init(write_p->eboard_p);
                write_p->eboard_p->c_crc_bitmap |= write_p->fru[diskcnt].position_mask;
                fbe_xor_record_invalid_lba_stamp(strip, write_p->fru[diskcnt].position_mask, write_p->eboard_p, data_blk_ptr);
                fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CHECKSUM_ERROR);
            }

#ifdef XOR_FULL_METACHECK
            else if ((data_blk_ptr->time_stamp & FBE_SECTOR_ALL_TSTAMPS) != 0)
            {
                /* The timestamp field in the pre-read data
                 * has the ALL_TIMESTAMPS bit set.
                 */
                fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_BAD_METADATA);
            }

            else if ((data_blk_ptr->write_stamp != 0) &&
                     (data_blk_ptr->time_stamp != FBE_SECTOR_INVALID_TSTAMP))
            {
                /* A bit is set in the writestamp field of
                 * the pre-read data, but there is also a
                 * timestamp.
                 */
                fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_BAD_METADATA);
            }
#endif
           
            /*! Set the remaining (3) metadata fields for the write request.
             *
             *  @note We always set the metadata for write even for invalidated
             *        sectors since only the crc is invalidated.  The remaining
             *        metadata fields should be valid.
             */
            if (!is_read)
            {
                mask = mask_v[diskcnt];
                data_blk_ptr->write_stamp = (mask & (row_parity_blk_ptr->write_stamp ^= mask));
                data_blk_ptr->time_stamp = FBE_SECTOR_INVALID_TSTAMP;
                data_blk_ptr->lba_stamp = xorlib_generate_lba_stamp(strip, write_p->offset);              
            } 
            else  
            {   
                /* validate that the read metadata is sane */
                if (data_blk_ptr->write_stamp !=
                     (row_parity_blk_ptr->write_stamp  & write_p->fru[diskcnt].position_mask))
                {
                    /* The writestamps don't match! The data
                     * is inconsistent with the parity.
                     */
                    fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CONSISTENCY_ERROR);
                }
                else if ((data_blk_ptr->time_stamp == FBE_SECTOR_INVALID_TSTAMP) &&
                         ((row_parity_blk_ptr->time_stamp & FBE_SECTOR_ALL_TSTAMPS) != 0))
                {
                    /* The ALL_TIMESTAMPS bit is set in the
                     * parity, but the pre-read data has the
                     * INVALID_TIMESTAMP value.
                     */
                    fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CONSISTENCY_ERROR);
                }
                else if ((data_blk_ptr->time_stamp != FBE_SECTOR_INVALID_TSTAMP) &&
                          (data_blk_ptr->time_stamp !=
                           (row_parity_blk_ptr->time_stamp & ~FBE_SECTOR_ALL_TSTAMPS))) 
                {
                    /* The pre-read data has a valid timestamp
                     * which doesn't match the timestamp on the
                     * parity drive (or the pre-read data has
                     * the ALL_TIMESTAMPS bit set).
                     */
                    fbe_xor_set_status(&write_p->status, FBE_XOR_STATUS_CONSISTENCY_ERROR);
                }
            }

            status = FBE_XOR_SGLD_INC_BE(*sgd_v_p, sg_bytes);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            if (0 >= sg_bytes)
            {
                /* That was the last block in the S/G list
                 * for this data drive.
                 */
                emptycnt++;
            }
            

        }

        /* Set up the metadata in the row parity sector.
         */
        row_parity_blk_ptr->crc = xorlib_cook_csum(pcsum, (fbe_u32_t)strip);

        if (parity_disks == 1)
        {
            row_parity_blk_ptr->time_stamp &= FBE_SECTOR_INVALID_TSTAMP;
            row_parity_blk_ptr->lba_stamp = 0;
        }

        /* If R6, set up the metadata in the diagonal parity sector.
         */
        if (parity_disks == 2)
        {
            /* Raid 6 uses its own invalid timestamp so we know that
             * the stripe contains data.
             */
            if (row_parity_blk_ptr->time_stamp == FBE_SECTOR_INVALID_TSTAMP) {
                row_parity_blk_ptr->time_stamp = FBE_SECTOR_R6_INVALID_TSTAMP;
            } else {
                row_parity_blk_ptr->time_stamp &= FBE_SECTOR_INVALID_TSTAMP;
            }

            diag_parity_blk_ptr->time_stamp = row_parity_blk_ptr->time_stamp;
            diag_parity_blk_ptr->write_stamp = row_parity_blk_ptr->write_stamp;
            diag_parity_blk_ptr->crc = xorlib_cook_csum(xorlib_calc_csum
                                                       (diag_parity_blk_ptr->data_word), (fbe_u32_t)strip);
            status = FBE_XOR_SGLD_INC_BE(wsgd_v[data_disks+1], sg_bytes);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
            
            if (0 >= sg_bytes)
            {
                /* That was the last block in the S/G list
                 * for this data drive.
                 */
                emptycnt++;
            }
        }

        status = FBE_XOR_SGLD_INC_BE(wsgd_v[data_disks], sg_bytes);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }

        if (0 >= sg_bytes)
        {
            /* That was the last block in the S/G list
             * for this data drive.
             */
            emptycnt++;
        }
    }

    /* Check the trailing prereads for the parity positions 
     */
    for (diskcnt = data_disks; diskcnt < data_disks + parity_disks; diskcnt++) {
        if (write_p->fru[diskcnt].write_end_offset)
        {
            if (!fbe_xor_rcw_chk_aligned_pre_reads(write_p,
                                              &wsgd_v[diskcnt],
                                              last_strip, 
                                              write_p->fru[diskcnt].write_end_offset,
                                              FBE_FALSE))
            {
                return FBE_STATUS_OK;
            }
        }
    }

    return status;
}

/*!***************************************************************
 * fbe_xor_get_next_descriptor     
 ****************************************************************
 *
 * @brief
 *   Returns the correct read, write, or read2 sg descriptor based
 *   on the fru and lba. 
 *
 * @param write_p - the xor rcw write information
 * @param fru_index - fru index to inspect
 * @param strip - lba of the block
 * @param wsgd_v_p - write sg descriptor vector
 * @param rsgd_v_p - read sg descriptor vector
 * @param r2sgd_v_p - read2 sg descriptor vector
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.
 * 
 * @author
 *  10/13/2013:  Deanna Heng -- Created.
 *
 ****************************************************************/
fbe_status_t fbe_xor_get_next_descriptor(fbe_xor_rcw_write_t *const write_p,
                            fbe_u16_t fru_index,
                            fbe_lba_t strip, 
                            fbe_xor_sgl_descriptor_t *const wsgd_v_p,
                            fbe_xor_sgl_descriptor_t *const rsgd_v_p,
                            fbe_xor_sgl_descriptor_t *const r2sgd_v_p,
                            fbe_xor_sgl_descriptor_t **sgd_v_p, fbe_bool_t * is_read) 
{
    if (strip < write_p->fru[fru_index].seed + write_p->fru[fru_index].read_count) 
    {
        *sgd_v_p = rsgd_v_p;
        *is_read = FBE_TRUE;
    } 
    else if (strip < (write_p->fru[fru_index].seed + 
                      write_p->fru[fru_index].read_count +
                      write_p->fru[fru_index].write_count)) 
    {
        *sgd_v_p = wsgd_v_p;
        *is_read = FBE_FALSE;
    } 
    else if (strip < (write_p->fru[fru_index].seed + 
                      write_p->fru[fru_index].read_count +
                      write_p->fru[fru_index].write_count +
                      write_p->fru[fru_index].read2_count)) 
    {
        *sgd_v_p = r2sgd_v_p;
        *is_read = FBE_TRUE;
    } else {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "No matching sg descriptor for strip strip: 0x%llx",
                              strip);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_xor_parity_rebuild       
 ****************************************************************
 *
 * @brief
 *   This function executes a parity rebuild.
 *
 * @param rebuild_p - Ptr to struct with all info we need to rebuild.
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.
 * 
 * @author
 *  10/01/99:  MPG -- Created.
 *
 ****************************************************************/
fbe_status_t fbe_xor_parity_rebuild(fbe_xor_parity_reconstruct_t *rebuild_p)
{
    fbe_lba_t seed = rebuild_p->seed;
    fbe_block_count_t count = rebuild_p->count;
    fbe_u16_t parity_drives = (rebuild_p->parity_pos[1] == FBE_XOR_INV_DRV_POS) ? 1  : 2;
    fbe_u16_t width = (rebuild_p->data_disks + parity_drives);
    fbe_xor_sgl_descriptor_t sgd_v[FBE_XOR_MAX_FRUS];
    fbe_block_count_t block;
    fbe_u16_t array_pos;
    fbe_sector_t *sector_v[FBE_XOR_MAX_FRUS];    /* Sector ptr for current position. */
    fbe_u32_t i;
    fbe_xor_error_t * eboard_p = rebuild_p->eboard_p;
    fbe_status_t status;
    fbe_u16_t invalid_bitmask;
    fbe_u32_t sg_count;

    /* Validate positions. 
     */
    status = fbe_xor_validate_parity_array(rebuild_p->parity_pos);
    if (status != FBE_STATUS_OK)
    {
        return status; 
    }

    status = fbe_xor_validate_rb_array(rebuild_p->rebuild_pos);
    if (status != FBE_STATUS_OK)
    {
        return status; 
    }

    if (rebuild_p->rebuild_pos[0] == rebuild_p->parity_pos[0] || rebuild_p->rebuild_pos[0] == rebuild_p->parity_pos[1] || 
        rebuild_p->rebuild_pos[1] == rebuild_p->parity_pos[0] || rebuild_p->rebuild_pos[0] == rebuild_p->parity_pos[1])
    {
        FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_REBUILD_PARITY]);
    } 
    else
    {
        FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_REBUILD_DATA]);
    }

    for (i = 0; i < width; i++)
    {
        fbe_u32_t bytcnt;

        bytcnt = FBE_XOR_SGLD_INIT_BE(sgd_v[i], rebuild_p->sg_p[i]);

        if (XOR_COND(bytcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "bytcnt == 0\n");

            return FBE_STATUS_GENERIC_FAILURE;
        }

        sector_v[i] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[i]);

        if (XOR_COND(FBE_XOR_SGLD_ADDR(sgd_v[i]) == NULL))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "FBE_XOR_SGLD_ADDR(sgd_v[%d]) == NULL\n",
                                  i);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_xor_setup_eboard(eboard_p, 0);

    /* Verify that the Calling driver has work for the XOR driver, otherwise
     * this may be an unnecessary call into the XOR driver and will effect
     * system performance.
     */

    if (XOR_COND(count == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "count == 0\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Before calling into EPU, init the tracing global.
     * This does a minimum amount of work.  If we really hit an error,
     * we'll fully init this global at that time.
     */
    fbe_xor_epu_trc_quick_init();

    for (block = 0;
        block < count;
        block++)
    {
        fbe_xor_error_t EBoard_local;
        fbe_u16_t rpos_bits;

        /* Construct a bitmask of the rebuild positions.
         */
        rpos_bits = (1 << rebuild_p->rebuild_pos[0]);
        rpos_bits |= ( rebuild_p->rebuild_pos[1] != FBE_XOR_INV_DRV_POS) ? (1 << rebuild_p->rebuild_pos[1]) : 0;

        /* Initialize the error board.
         * Set the u_crc bit for the drive to be rebuilt.
         * Also set the u_crc bit for any hard errored drives.
         */
        fbe_xor_init_eboard((fbe_xor_error_t *)&EBoard_local,
                            eboard_p->raid_group_object_id,
                            eboard_p->raid_group_offset);
        fbe_xor_setup_eboard(((fbe_xor_error_t *)&EBoard_local),
                            rpos_bits);
        /* retry errors is not used in this code path.
         */
        EBoard_local.retry_err_bitmap = 0; 
        EBoard_local.hard_media_err_bitmap = eboard_p->hard_media_err_bitmap;
        EBoard_local.no_data_err_bitmap = eboard_p->no_data_err_bitmap;

        if (rebuild_p->parity_pos[1] != FBE_XOR_INV_DRV_POS)
        {
            /* Do the stripwise rebuild for R6
             */
            status = fbe_xor_eval_parity_unit_r6(&EBoard_local,
                                                 sector_v,
                                                 &rebuild_p->bitkey[0],
                                                 width,
                                                 rebuild_p->parity_pos,
                                                 rebuild_p->rebuild_pos,
                                                 seed, 
                                                 rebuild_p->b_final_recovery_attempt,
                                                 &rebuild_p->data_position[0],
                                                 rebuild_p->option,
                                                 &invalid_bitmask);

            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
            EBoard_local.u_bitmap = invalid_bitmask;
        } 
        else
        {
            /* Do the stripwise rebuild for R5/R3.
             */
            status = fbe_xor_eval_parity_unit(&EBoard_local,
                                              sector_v,
                                              &rebuild_p->bitkey[0],
                                              width,
                                              rebuild_p->parity_pos[0],
                                              rebuild_p->rebuild_pos[0],
                                              seed, 
                                              rebuild_p->b_final_recovery_attempt,
                                              FBE_FALSE    /* do not preserve shed data. */,
                                              rebuild_p->option,
                                              &invalid_bitmask);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
            EBoard_local.u_bitmap = invalid_bitmask;
        }

        /* If the user passed in an error region struct, then fill it out.
         */
        status =  fbe_xor_save_error_region( &EBoard_local, 
                                             /* The 4th index is the error regions struct.
                                              */
                                             rebuild_p->eregions_p,
                                             rebuild_p->b_final_recovery_attempt, 
                                             seed,
                                             parity_drives,
                                             width,
                                             rebuild_p->parity_pos[0]);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }

        for (array_pos = 0; array_pos < width; array_pos++)
        {
            if (XOR_COND(FBE_XOR_SGLD_ADDR(sgd_v[array_pos]) == NULL))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "FBE_XOR_SGLD_ADDR(sgd_v[%d]) == NULL\n",
                                      array_pos);

                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Setup the sector vector for the next strip.
             * Note that we *must* do this after touching
             * the data because this maps the memory out also.
             */
            status = FBE_XOR_SGLD_INC_BE(sgd_v[array_pos], sg_count);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            sector_v[array_pos] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[array_pos]);
        }
        /* Copy the error information,
         * so we know which frus to write when we finish.
         */
        FBE_XOR_INIT_EBOARD_WMASK(((fbe_xor_error_t *) & EBoard_local));

        /* Update the global eboard structure */
        fbe_xor_update_global_eboard(eboard_p, ((fbe_xor_error_t *) & EBoard_local));

        if (XOR_COND((EBoard_local.w_bitmap & (1 << rebuild_p->rebuild_pos[0])) == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "unexpected error: write bitmap = 0x%x, first rebuild pos = 0x%x\n",
                                  EBoard_local.w_bitmap,
                                  rebuild_p->rebuild_pos[0]);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (XOR_COND((rebuild_p->rebuild_pos[1] != FBE_XOR_INV_DRV_POS) &&
                     ((EBoard_local.w_bitmap & (1 << rebuild_p->rebuild_pos[1])) == 0)))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "unexpected error : write bitmap = 0x%x, second rebuild pos = 0x%x\n",
                                  EBoard_local.w_bitmap,
                                  rebuild_p->rebuild_pos[1]);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Increment seed for the next strip.
         */
        seed++;
    }    /* End current strip mine operation */

    /* We must at least set up the write bit for the dead drive.
     */
    if (XOR_COND((eboard_p->w_bitmap & (1 << rebuild_p->rebuild_pos[0])) == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "unexpected error : write bitmap = 0x%x, first rebuild pos = 0x%x\n",
                              eboard_p->w_bitmap,
                              rebuild_p->rebuild_pos[0]);

        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* If we have a 2nd rebuild position (R6 only),
     * then make sure that the write bitmap is set for it.
     */
    if (XOR_COND((rebuild_p->rebuild_pos[1] != FBE_XOR_INV_DRV_POS) &&
                 ((eboard_p->w_bitmap & (1 << rebuild_p->rebuild_pos[1])) == 0)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "unexpected error : write bitmap = 0x%x, second rebuild pos = 0x%x\n",
                              eboard_p->w_bitmap,
                              rebuild_p->rebuild_pos[1]);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}  /* fbe_xor_parity_rebuild() */

/*!***************************************************************
 * fbe_xor_parity_reconstruct      
 ****************************************************************
 *
 * @brief
 *    This function is called by xor_execute_processor_command
 *    to carry out the reconstruction of data.  
 *
 * @param xor_status_p - ptr to XOR_STATUS block
 * @param vec_info_p - ptr to vectors that comprise this operation
 * @param data_disks - number of disks involved in operation
 * @param eboard_p - pointer to error board structure
 * @param inv_parity_crc_error - FBE_TRUE to invalidate parity on a 
 *                                CRC error.
 * @param ppos - parity position vector.
 * @param rpos - rebuild position vector.
 *
 * @return fbe_status_t
 *
 *  @notes
 *    The XOR_VECTOR_LIST should have the following format:
 *
 *    [0] - sgl vector
 *    [1] - seed vector
 *    [2] - count vector
 *    [3] - bitmask vector
 *    [4] - offset vector
 *    [5] - fbe_xor_error_t_REGIONS struct
 *    [6] - data_positions vector
 *
 * @author
 *      10/01/99:  MPG -- Created.
 *
 ****************************************************************/
fbe_status_t fbe_xor_parity_reconstruct(fbe_xor_parity_reconstruct_t *rc_p)
{
    fbe_lba_t seed = rc_p->seed;
    fbe_block_count_t count = rc_p->count;
    fbe_block_count_t block;
    fbe_u16_t parity_drives = (rc_p->parity_pos[1] == FBE_XOR_INV_DRV_POS) ? 1  : 2;
    fbe_u16_t array_width = (rc_p->data_disks + parity_drives);
    fbe_xor_sgl_descriptor_t sgd_v[FBE_XOR_MAX_FRUS];
    fbe_sector_t *sector[FBE_XOR_MAX_FRUS];
    fbe_u16_t array_pos;
    fbe_u16_t invalid_bitmask;
    fbe_status_t status;
    fbe_u32_t sg_count;

    /* Validate positions. 
     */
    status = fbe_xor_validate_parity_array(rc_p->parity_pos);
    if (status != FBE_STATUS_OK)
    {
        return status; 
    }
    status = fbe_xor_validate_rb_array(rc_p->rebuild_pos);
    if (status != FBE_STATUS_OK)
    {
        return status; 
    }

    /* Initialize the fbe_xor_sgl_descriptor_t structure */
    for (array_pos = 0; array_pos < array_width; array_pos++)
    {
        FBE_XOR_SGLD_INIT_WITH_OFFSET(sgd_v[array_pos], 
                                      rc_p->sg_p[array_pos],
                                      rc_p->offset[array_pos],
                                      FBE_BE_BYTES_PER_BLOCK );

        if (XOR_COND(sgd_v[array_pos].bytcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "(sgd_v[%d].bytcnt == 0)\n",
                                  array_pos);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (XOR_COND(sgd_v[array_pos].bytcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "sgd_v[%d].bytcnt == 0\n",
                                  array_pos);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        sector[array_pos] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[array_pos]);
    }

    /* Initialize the global error board.
     */
    fbe_xor_setup_eboard(rc_p->eboard_p, 0);

    /* Verify that the Calling driver has work for the XOR driver, otherwise
       this may be an unnecessary call into the XOR driver and will effect
       system performance.
     */
    if (XOR_COND(count == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "count == 0\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Before calling into EPU, init the tracing global.
     * This does a minimum amount of work.  If we really hit an error,
     * we'll fully init this global at that time.
     */
    fbe_xor_epu_trc_quick_init();

    for (block = 0; block < count; block++)
    {
        fbe_xor_error_t EBoard_local;
        fbe_u16_t rebuild_pos_bits;

        /* Construct a bitmask of the rebuild positions.
         */
        rebuild_pos_bits = (1 << rc_p->rebuild_pos[0]);
        rebuild_pos_bits |= ( rc_p->rebuild_pos[1] != FBE_XOR_INV_DRV_POS) ? (1 << rc_p->rebuild_pos[1]) : 0;

        /* Initialize the local error board.
         */
        fbe_xor_init_eboard((fbe_xor_error_t *)&EBoard_local,
                            rc_p->eboard_p->raid_group_object_id,
                            rc_p->eboard_p->raid_group_offset);
        fbe_xor_setup_eboard(((fbe_xor_error_t *) & EBoard_local),
                            rebuild_pos_bits); 

        /* retry errors is not used in this code path.
         * We update the global uncorrectable below.  Use that value
         * to setup the local uncorrectable.
         */
        EBoard_local.retry_err_bitmap = 0;
        EBoard_local.u_bitmap = rc_p->eboard_p->u_bitmap;
        EBoard_local.hard_media_err_bitmap = rc_p->eboard_p->hard_media_err_bitmap; 
        EBoard_local.no_data_err_bitmap = 0; /* Not used for now. */

        if (rc_p->parity_pos[1] != FBE_XOR_INV_DRV_POS)
        {
            /* Do the stripwise rebuild for R6
             */
            status = fbe_xor_eval_parity_unit_r6(&EBoard_local,
                                                 sector,
                                                 &rc_p->bitkey[0],
                                                 array_width,
                                                 rc_p->parity_pos,
                                                 rc_p->rebuild_pos,
                                                 seed, 
                                                 rc_p->b_final_recovery_attempt,
                                                 &rc_p->data_position[0],
                                                 rc_p->option,
                                                 &invalid_bitmask);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
            EBoard_local.u_bitmap = invalid_bitmask;
        } 
        else
        {
            status = fbe_xor_eval_parity_unit(&EBoard_local,
                                              sector,
                                              &rc_p->bitkey[0],
                                              array_width,
                                              rc_p->parity_pos[0],
                                              rc_p->rebuild_pos[0],
                                              seed,
                                              rc_p->b_final_recovery_attempt,
                                              FBE_FALSE    /* do not preserve shed data. */ ,
                                              rc_p->option,
                                              &invalid_bitmask);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
            EBoard_local.u_bitmap = invalid_bitmask;
        }

        /* If the user passed in an error region struct, then fill it out.
         */
        status = fbe_xor_save_error_region( &EBoard_local, 
                                            /* The 4th index is the error regions struct.
                                             */
                                            rc_p->eregions_p,
                                            rc_p->b_final_recovery_attempt, 
                                            seed,
                                            parity_drives,
                                            array_width,
                                            rc_p->parity_pos[0]);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }

        for (array_pos = 0; array_pos < array_width; array_pos++)
        {
            /* Increment to point to the next sector.
             */
            status = FBE_XOR_SGLD_INC_BE(sgd_v[array_pos], sg_count);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            sector[array_pos] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[array_pos]);
        }

        /* All the "correctable" or "uncorrectable" errors
         * are logged as modifications.
         */
        FBE_XOR_INIT_EBOARD_WMASK(((fbe_xor_error_t *) & EBoard_local));

        /* Update the global eboard structure */
        fbe_xor_update_global_eboard(rc_p->eboard_p, ((fbe_xor_error_t *) & EBoard_local));

        /* Increment seed for the next strip.
         */
        seed++;
    }    /* end of for */

    /* We must at least set up the write bit for the dead drive.
     */

    if (XOR_COND((rc_p->eboard_p->w_bitmap & (1 << rc_p->rebuild_pos[0])) == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "uncorrect rebuild  behaviour : write bitmap 0x%x,"
                              "first rebuild pos = 0x%x\n",
                              rc_p->eboard_p->w_bitmap,
                              rc_p->rebuild_pos[0]);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If we have a 2nd rebuild position (R6 only),
     * then make sure that the write bitmap is set for it.
     */
    if (XOR_COND((rc_p->rebuild_pos[1] != FBE_XOR_INV_DRV_POS) &&
                 ((rc_p->eboard_p->w_bitmap & (1 << rc_p->rebuild_pos[1])) == 0)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "uncorrect rebuild behaviour : write bitmap 0x%x, second rebuild pos = 0x%x\n",
                              rc_p->eboard_p->w_bitmap,
                              rc_p->rebuild_pos[1]);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* In a reconstruct, it is possible that the sg list we were given
     * has more than we actually need in it.
     * Therefore we need to finish mapping out here.
     */
    for (array_pos = 0; array_pos < array_width; array_pos++)
    {
        status = FBE_XOR_SGLD_UNMAP(sgd_v[array_pos]);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
    }

    return FBE_STATUS_OK;
}  /* fbe_xor_parity_reconstruct() */

/****************************************************************
 * fbe_xor_parity_verify       
 ****************************************************************
 *
 * @brief
 *   This function is called to carry out the verify.  
 *
 * @param verify_p - Structure with all the verify vectors.
 *
 * @return fbe_status_t
 *
 * @notes
 *  The XOR_VECTOR_LIST should have the following format:
 *
 *    [0] - sgl
 *    [1] - seed
 *    [2] - count
 *    [3] - bitmask
 *    [4] - fbe_xor_error_t_REGION structure
 *    [5] - Data position vector.
 *    [6] - FBE_TRUE to preserve shed data
 *          FBE_FALSE to reconstruct parity.
 *
 ****************************************************************/
fbe_status_t fbe_xor_parity_verify(fbe_xor_parity_reconstruct_t *verify_p)
{
    fbe_lba_t seed = verify_p->seed;
    fbe_block_count_t count = verify_p->count;
    fbe_u16_t parity_drives = (verify_p->parity_pos[1] == FBE_XOR_INV_DRV_POS) ? 1  : 2;
    fbe_u16_t width = verify_p->data_disks + parity_drives;
    fbe_xor_sgl_descriptor_t sgd_v[FBE_XOR_MAX_FRUS];
    fbe_xor_error_t * eboard_p = verify_p->eboard_p;
    fbe_block_count_t block;
    fbe_u16_t array_pos;
    fbe_sector_t *sector_v[FBE_XOR_MAX_FRUS];
    fbe_u32_t i;
    fbe_u32_t original_error_region_index = verify_p->eregions_p->next_region_index;
    fbe_status_t status;
    fbe_u16_t invalid_bitmap;
    fbe_u32_t sg_count;

    /* Validate positions. 
     */
    status  = fbe_xor_validate_parity_array(verify_p->parity_pos);
    if (status != FBE_STATUS_OK)
    {
        return status; 
    }
    status = fbe_xor_validate_rb_array(verify_p->rebuild_pos);
    if (status != FBE_STATUS_OK)
    {
        return status; 
    }

    FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_VERIFY]);

    for (i = 0; i < width; i++)
    {
        fbe_u32_t bytcnt;

        bytcnt = FBE_XOR_SGLD_INIT_BE(sgd_v[i], verify_p->sg_p[i]);

        if (XOR_COND(bytcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "bytcnt == 0\n");

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (XOR_COND(FBE_XOR_SGLD_ADDR(sgd_v[i]) == NULL))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "FBE_XOR_SGLD_ADDR(sgd_v[%d]) == NULL\n",
                                  i);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        sector_v[i] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[i]);
    }

    /* Initialize the eboard
     */
    fbe_xor_setup_eboard(eboard_p, 0);

    /* Verify that the Calling driver has work for the XOR driver, otherwise
       this may be an unnecessary call into the XOR driver and will effect
       system performance.
     */

    if (XOR_COND(count == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "count == 0\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* Before calling into EPU, init the tracing global.
     * This does a minimum amount of work.  If we really hit an error,
     * we'll fully init this global at that time.
     */
    fbe_xor_epu_trc_quick_init();

    for (block = 0;
        block < count;
        block++)
    {
        fbe_xor_error_t EBoard_local;
        fbe_u16_t rpos_bits;

        /* Construct a bitmask of the rebuild positions.
         */
        rpos_bits = ( verify_p->rebuild_pos[0] != FBE_XOR_INV_DRV_POS ) ? (1 << verify_p->rebuild_pos[0]) : 0;
        rpos_bits |= ( verify_p->rebuild_pos[1] != FBE_XOR_INV_DRV_POS) ? (1 << verify_p->rebuild_pos[1]) : 0;

        /* Initialize the error board.
         * Set the u_crc bit for the drive to be rebuilt.
         * Also set the u_crc bit for any hard errored drives.
         */
        fbe_xor_init_eboard((fbe_xor_error_t *)&EBoard_local,
                            eboard_p->raid_group_object_id,
                            eboard_p->raid_group_offset);
        fbe_xor_setup_eboard(((fbe_xor_error_t *) & EBoard_local),
                            (rpos_bits | eboard_p->hard_media_err_bitmap | eboard_p->retry_err_bitmap));
        EBoard_local.retry_err_bitmap = eboard_p->retry_err_bitmap;
        EBoard_local.hard_media_err_bitmap = eboard_p->hard_media_err_bitmap;
        EBoard_local.no_data_err_bitmap = 0; /* Not used for now. */

        if (verify_p->parity_pos[1] != FBE_XOR_INV_DRV_POS)
        {
            /* Do the stripwise rebuild for R6
             */

            status  = fbe_xor_eval_parity_unit_r6(&EBoard_local,
                                                  sector_v,
                                                  &verify_p->bitkey[0],
                                                  width,
                                                  &verify_p->parity_pos[0],
                                                  &verify_p->rebuild_pos[0],
                                                  seed, 
                                                  verify_p->b_final_recovery_attempt,
                                                  &verify_p->data_position[0],
                                                  verify_p->option,
                                                  &invalid_bitmap);

            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            EBoard_local.u_bitmap = invalid_bitmap;
        } 
        else
        {
            /* Do the stripwise rebuild.
             */
            status = fbe_xor_eval_parity_unit(&EBoard_local,
                                              sector_v,
                                              &verify_p->bitkey[0],
                                              width,
                                              verify_p->parity_pos[0],
                                              verify_p->rebuild_pos[0],
                                              seed,
                                              verify_p->b_final_recovery_attempt,
                                              verify_p->b_preserve_shed_data,
                                              verify_p->option,
                                              &invalid_bitmap);

            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            EBoard_local.u_bitmap = invalid_bitmap;
        }

        /* If the user passed in an error region struct, then fill it out.
         */
        status = fbe_xor_save_error_region( &EBoard_local, 
                                            /* The 4th index is the error regions struct.
                                             */
                                            verify_p->eregions_p,
                                            verify_p->b_final_recovery_attempt, 
                                            seed,
                                            parity_drives,
                                            width,
                                            verify_p->parity_pos[0]);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }

        /* Setup the sector vector for the next strip.
         */
        for (array_pos = 0; array_pos < width; array_pos++)
        {
            if (XOR_COND(FBE_XOR_SGLD_ADDR(sgd_v[array_pos]) == NULL))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "FBE_XOR_SGLD_ADDR(sgd_v[%d]) != NULL\n",
                                      array_pos);

                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Increment for the next pass.
             */
            status = FBE_XOR_SGLD_INC_BE(sgd_v[array_pos], sg_count);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            sector_v[array_pos] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[array_pos]);
        }

        /* Copy the error information,
         * so we know which frus to write when we finish.
         */
        FBE_XOR_INIT_EBOARD_WMASK(((fbe_xor_error_t *)&EBoard_local));

        /* Update the global eboard structure */
        fbe_xor_update_global_eboard(eboard_p, ((fbe_xor_error_t *)&EBoard_local));

        if (XOR_COND((verify_p->rebuild_pos[0] != FBE_XOR_INV_DRV_POS) &&
                     ((EBoard_local.w_bitmap & (1 << verify_p->rebuild_pos[0])) == 0)))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "uncorrect rebuild behavior: first rebuild pos = 0x%x, write bitmap = 0x%x\n",
                                  verify_p->rebuild_pos[0],
                                  EBoard_local.w_bitmap);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (XOR_COND((verify_p->rebuild_pos[1] != FBE_XOR_INV_DRV_POS) &&
                     ((EBoard_local.w_bitmap & (1 << verify_p->rebuild_pos[1])) == 0)))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "uncorrect rebuild behaviour: second rebuild pos = 0x%x, write bitmap = 0x%x\n",
                                  verify_p->rebuild_pos[1],
                                  EBoard_local.w_bitmap);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (verify_p->error_counts_p != NULL)
        {
            fbe_xor_vr_increment_errcnt(verify_p->error_counts_p, &EBoard_local,
                                        &verify_p->parity_pos[0], parity_drives);
        }

        /* Increment the seed for the next strip.
         */
        seed++;
    }    /* End current strip mine operation */

    /* We always save media errors in the error region since we do not retry media errors. 
     * At the time we save a media error we do not know if there is some other 
     * crc/stamp/coh error that will cause a retry. 
     *  
     * If we got a media error and we got some other error that will cause a retry, then 
     * clear out the media errors from the error region since they will reappear (possibly
     * with a different correctableness) when we come through this path after retrying the
     * other errors. 
     */
    if ((verify_p->b_final_recovery_attempt == FBE_FALSE) &&
        (eboard_p->media_err_bitmap != 0) &&
        (eboard_p->media_err_bitmap != 
         fbe_xor_lib_error_get_total_bitmask(verify_p->eboard_p)))
    {
        /* Just reset the index to forget about these records.
         */
        verify_p->eregions_p->next_region_index = original_error_region_index;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_xor_parity_verify()
 **************************************/

/****************************************************************
 *            fbe_xor_r5_const_parity_recovery_verify       
 ****************************************************************
 *
 * @brief
 *   This function is called by xor_execute_processor_command
 *   to carry out the host recovery verify.  
 *
 * @param xor_status_p - ptr to XOR_STATUS block
 * @param vec_info_p - ptr to vectors that comprise this operation
 * @param data_disks - number of disks involved in operation
 * @param parity_pos - Vector of parity positions.
 * @param rebuild_pos - Vector of rebuild positions.
 *
 * @return VALUES/ERRORS:
 *   none
 *
 *  @notes
 *    The XOR_VECTOR_LIST should have the following format:
 *
 *    [0] - sgl
 *    [1] - seed
 *    [2] - count
 *    [3] - src sgl
 *    [4] - dest sgl
 *    [5] - src offset
 *    [6] - dest offset
 *    [7] - sgl offset
 *    [8] - bitmask
 *    [9] - data position.
 *
 * @author
 *    5/12/2006:  Rob Foley. Rewrote for R6.
 *
 ****************************************************************/
fbe_status_t fbe_xor_parity_recovery_verify_const_parity(fbe_xor_recovery_verify_t *xor_recovery_verify_p)
{
    fbe_u32_t block;
    fbe_status_t status;
    fbe_u32_t sg_count;

    fbe_u16_t data_disks = xor_recovery_verify_p->data_disks;


    /* Validate positions.
     */
    status = fbe_xor_validate_parity_array(xor_recovery_verify_p->parity_pos);
    if (status != FBE_STATUS_OK)
    {
        return status; 
    }
    status = fbe_xor_validate_rb_array(xor_recovery_verify_p->rebuild_pos);
    if (status != FBE_STATUS_OK)
    {
        return status; 
    }

    FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_VERIFY_WRITE]);

    /*! @note For recovery verify construct parity the state machine currently 
     *        ignores the status.  We still properly flag the status so that
     *        a caller can check it in the future.
     */
    xor_recovery_verify_p->status = FBE_XOR_STATUS_NO_ERROR;

    /* Construct new parity.
     */
    {
        fbe_u16_t time_stamp = fbe_xor_get_timestamp(0);
        fbe_u32_t data_index;
        fbe_lba_t seed = xor_recovery_verify_p->seed;
        fbe_u16_t parity_drives = (xor_recovery_verify_p->parity_pos[1] == FBE_XOR_INV_DRV_POS) ? 1 : 2;
        fbe_xor_sgl_descriptor_t row_parity_sgd;
        fbe_xor_sgl_descriptor_t diag_parity_sgd;
        fbe_xor_sgl_descriptor_t data_sgd_v[FBE_XOR_MAX_FRUS - 1];

        if (parity_drives == 2)
        {
            /* Get the diag parity sg descriptor from the data_disks + 1 element.
             */
            FBE_XOR_SGLD_INIT_WITH_OFFSET( diag_parity_sgd,
                                           xor_recovery_verify_p->fru[data_disks + 1].sg_p, 
                                           xor_recovery_verify_p->fru[data_disks + 1].offset,
                                           FBE_BE_BYTES_PER_BLOCK );

        }

        /* Last block is the Parity data.
         */
        FBE_XOR_SGLD_INIT_WITH_OFFSET( row_parity_sgd,
                                       xor_recovery_verify_p->fru[data_disks].sg_p, 
                                       xor_recovery_verify_p->fru[data_disks].offset,
                                       FBE_BE_BYTES_PER_BLOCK );

        /* Init the SGD vectors for the data disks.
         */
        for (data_index = 0; data_index < data_disks; data_index++)
        {
            FBE_XOR_SGLD_INIT_WITH_OFFSET( data_sgd_v[data_index], 
                                           xor_recovery_verify_p->fru[data_index].sg_p, 
                                           xor_recovery_verify_p->fru[data_index].offset,
                                           FBE_BE_BYTES_PER_BLOCK);
        }

        for (block = 0; block < xor_recovery_verify_p->count; block++)
        {
            fbe_u32_t pcsum = 0;    /* Running checksum of row parity. */
            fbe_u32_t dcsum;    /* The current uncooked data block checksum. */
            fbe_u16_t dcsum_cooked_16;    /* The cooked data block checksum. */
            fbe_sector_t *row_parity_blk_ptr = NULL;    /* Pointer of row parity block */
            fbe_sector_t *data_blk_ptr;    /* Pointer of data block */
            fbe_sector_t *diag_parity_blk_ptr = NULL;    /* Pointer of diagonal parity block */

            /* Loop over all data positions.
             */
            for ( data_index = 0; data_index < data_disks; data_index++)
            {
                /* Init the row parity pointer and data block pointer.
                 */
                row_parity_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(row_parity_sgd);
                data_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(data_sgd_v[data_index]);

                if ( data_index == 0 )
                {
                    if (parity_drives == 2)
                    {
                        /* Get pointer to diagonal parity from diag parity sg descriptor.
                         */
                        diag_parity_blk_ptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(diag_parity_sgd);

                        /* Initialize the row and diag parity.
                         */
                        dcsum = xorlib_calc_csum_and_init_r6_parity(data_blk_ptr->data_word,
                                                                     row_parity_blk_ptr->data_word,
                                                                     diag_parity_blk_ptr->data_word,
                                                                     xor_recovery_verify_p->fru[data_index].data_position);
                    } 
                    else
                    {
                        /* Initialize the row parity.
                         */
                        dcsum = xorlib_calc_csum_and_cpy(data_blk_ptr->data_word, 
                                                          row_parity_blk_ptr->data_word);
                    }
                }    /* end if data_index == 0 */
                else
                {
                    /* On R5, we only want to update parity if uncorrectables
                     * were not found.  When uncorrectables are found on
                     * R5 we also invalidate parity.
                     * On R6, we always update parity since parity is always good.
                     */
                    if (XOR_COND((data_blk_ptr == NULL) || (row_parity_blk_ptr == NULL)))
                    {
                        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                              "(data_blk_ptr 0x%p == NULL) || (row_parity_blk_ptr 0x%p == NULL)\n",
                                              data_blk_ptr,
                                              row_parity_blk_ptr);

                        return FBE_STATUS_GENERIC_FAILURE;
                    }


                    if (parity_drives == 2)
                    {
                        /* Update row and diag parity to include this data block.
                         * Also calculate the checksum of this data block.
                         */
                        dcsum = xorlib_calc_csum_and_update_r6_parity(data_blk_ptr->data_word,
                                                                      row_parity_blk_ptr->data_word,
                                                                      diag_parity_blk_ptr->data_word,
                                                                      xor_recovery_verify_p->fru[data_index].data_position);

                    } 
                    else
                    {
                        /* Update row parity.
                         */
                        dcsum = xorlib_calc_csum_and_xor(data_blk_ptr->data_word, 
                                                          row_parity_blk_ptr->data_word);
                    }
                }

                pcsum ^= dcsum;
                dcsum_cooked_16 = xorlib_cook_csum(dcsum, (fbe_u32_t)seed);

                /* If this is not a RAID-6 and the data block checksum doesn't 
                 * match, invoke method to update parity as required.
                 */
                if ((parity_drives == 1) && (data_blk_ptr->crc != dcsum_cooked_16))
                {
                    fbe_xor_status_t xor_status;

                    /* Handle the data block checksum error on read.
                     */
                    xor_status = fbe_xor_handle_bad_crc_on_read(data_blk_ptr, 
                                                                seed, 
                                                                xor_recovery_verify_p->fru[data_index].bitkey_p,
                                                                xor_recovery_verify_p->option,
                                                                xor_recovery_verify_p->eboard_p, 
                                                                xor_recovery_verify_p->eregions_p,
                                                                xor_recovery_verify_p->array_width);

                    /* Update the xor status.
                     */
                    if (xor_status != FBE_XOR_STATUS_NO_ERROR)
                    {
                        /*! @todo Currently the caller ignores the status but for 
                         *        some reason we only udpate the status if check
                         *        checksum is set.
                         */
                        if (xor_recovery_verify_p->option & FBE_XOR_OPTION_CHK_CRC)
                        {
                            /* Report that the xor request failed
                             */
                            fbe_xor_set_status(&xor_recovery_verify_p->status, xor_status);
                        }
                    }
                } /* end if parity drives is (1) and checksum error */
                else if (parity_drives == 2)
                {
                    /* For R6, calculate the parity of checksums.
                     */
                    if ( data_index == 0 )
                    {
                        /* Initializes and calculates the row and diagonal parity
                         * of the 16 bit checksum value.
                         */
                        fbe_xor_calc_parity_of_csum_init(&data_blk_ptr->crc,
                                                         &row_parity_blk_ptr->lba_stamp,
                                                         &diag_parity_blk_ptr->lba_stamp,
                                                         xor_recovery_verify_p->fru[data_index].data_position);
                    } 
                    else
                    {
                        /* Also update the parity of checksums to include this 
                         * data drive's checksum.
                         */
                        fbe_xor_calc_parity_of_csum_update(&data_blk_ptr->crc,
                                                           &row_parity_blk_ptr->lba_stamp,
                                                           &diag_parity_blk_ptr->lba_stamp,
                                                           xor_recovery_verify_p->fru[data_index].data_position);
                    }
                }

                /*! @note Data that has been lost (invalidated) should never
                 *        have a valid checksum.
                 */
                fbe_xor_validate_data_if_enabled(data_blk_ptr, seed, xor_recovery_verify_p->option);

                /* Set up the metadata in the data sector.
                 */

                /* Shed stamp in the data sector should already be legal since
                 * we reconstructing parity and the data we are
                 * reconstructing from should have already been found to be 
                 * legal, sanity check it.
                 */

                if (XOR_COND(!xorlib_is_valid_lba_stamp(&data_blk_ptr->lba_stamp, seed, xor_recovery_verify_p->offset)))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "unexpected stamp error: block lba stamp = 0x%x, seed = 0x%llx\n",
                                          data_blk_ptr->lba_stamp,
                                          (unsigned long long)seed);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /*! @note We always generate valid metadata (even if the sector 
                 *        has been invalidated).
                 */
                data_blk_ptr->time_stamp = time_stamp;
                data_blk_ptr->write_stamp = 0;
                data_blk_ptr->lba_stamp = xorlib_generate_lba_stamp(seed, xor_recovery_verify_p->offset);

                /* Move the sg descriptor to the next sector.
                 */
                status = FBE_XOR_SGLD_INC_BE(data_sgd_v[data_index], sg_count);
                if (status != FBE_STATUS_OK)
                {
                    return status; 
                }

            } /* End loop over all data drives. */

            /* We no longer invalidate parity.  Therefore update the stamps for
             * the parity position.  Since we are writing to every position the 
             * write_stamp is 0. 
                 */
#pragma warning(disable: 4068) /* Suppress warnings about unknown pragmas */
#pragma prefast(disable: 11)   /* Suppress Prefast warning 11 */
                row_parity_blk_ptr->time_stamp = time_stamp | FBE_SECTOR_ALL_TSTAMPS;
                row_parity_blk_ptr->write_stamp = 0;

            /* Only set the lba stamp if this is R5/R3.
                 */
                if (parity_drives == 1)
                {
                    row_parity_blk_ptr->lba_stamp = 0;
                }

            /* The parity checksum is always correct
             */
            row_parity_blk_ptr->crc = xorlib_cook_csum(pcsum, XORLIB_SEED_UNREFERENCED_VALUE);

                /* If R6, set up the metadata in the diagonal parity sector.
                 * The shed stamp was setup above.
                 */
                if (parity_drives == 2)
                {
                    diag_parity_blk_ptr->time_stamp = time_stamp | FBE_SECTOR_ALL_TSTAMPS;
                    diag_parity_blk_ptr->write_stamp = 0;
                diag_parity_blk_ptr->crc = xorlib_cook_csum(xorlib_calc_csum(diag_parity_blk_ptr->data_word), XORLIB_SEED_UNREFERENCED_VALUE);
                }
#pragma prefast(default: 11)   /* Restore defaults */
#pragma warning(default: 4068)

            /* Increment our seed since we are done with this strip.
             */
            seed++;

            status = FBE_XOR_SGLD_INC_BE(row_parity_sgd, sg_count);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
            if (parity_drives == 2)
            {
                status = FBE_XOR_SGLD_INC_BE(diag_parity_sgd, sg_count);
                if (status != FBE_STATUS_OK)
                {
                    return status; 
                }
            }

        }    /* End For block = 0 block < count */
    }    /* end of construct parity */
    return FBE_STATUS_OK;
}
/* end fbe_xor_parity_recovery_verify_const_parity() */

/*!**************************************************************
 * fbe_xor_parity_verify_copy_user_data()
 ****************************************************************
 * @brief
 *  Copy user data for a recovery verify.
 *
 * @param  mem_move_p - ptr to structure with all parameters for move.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  1/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_xor_parity_verify_copy_user_data(fbe_xor_mem_move_t *mem_move_p)
{
    fbe_status_t status;

    /* If we have been requested to check the checksum use the special
     * `512_8_to_520' move command.  The standard 520 to 520 does not.
       */
    if (mem_move_p->option & FBE_XOR_OPTION_CHK_CRC)
    {
        /* Set lba stamps so they are correct for this sector.
         */
        status = fbe_xor_mem_move(mem_move_p,
                                  FBE_XOR_MOVE_COMMAND_MEM512_8_TO_MEM520,
                                  (FBE_XOR_OPTION_GEN_LBA_STAMPS | mem_move_p->option), 
                                  mem_move_p->disks, 
                                  0x7fff,
                                  0,
                                  0);
    } 
    else
    {
        /* assign_csum flag will be false in case of corrupt crc operation.
         * Always set lba stamps so they are correct for the newly copied 
         * sector.
         */
        status = fbe_xor_mem_move(mem_move_p,
                                  FBE_XOR_MOVE_COMMAND_MEM520_TO_MEM520    /* Move 520 bytes to 520 bytes. */,
                                  (FBE_XOR_OPTION_GEN_LBA_STAMPS | mem_move_p->option) /* Set LBA stamps. */,
                                  mem_move_p->disks    /* Number of sources/destinations. */,
                                  0x7fff    /* Time Stamp. */,
                                  0    /* Shed Stamp. */,
                                  0    /* Write Stamp. */);
    }

    return status;
}
/******************************************
 * end fbe_xor_parity_verify_copy_user_data()
 ******************************************/

/**********************************************************************
 * fbe_xor_check_parity
 **********************************************************************
 * 
 * @brief
 *  This function is called by xor_execute_parallel_468 while checking 
 *  parity drive's checksum prior to processing further.
 *
 * @param write_p - ptr to structure to process.
 * @param strip - current LBA.
 *   parity_blk_pt,[I]  - ptr to row or diagonal parity block
 * @param mask - current drive position relative to the unit geometry
 * @param array_width - total width of the unit
 *   parity_drives,[I]  - number of parity drives involved in operation
 *                        R5 = 1, R6 = 2
 *   parity_bitmask,[I] - bitmask for parity drives
 * @return
 *  The "raw" checksum for the data in the sector.
 *
 *
 * @author
 *  05/01/06 - Created. CLC
 *  
 *********************************************************************/
static fbe_u32_t fbe_xor_check_parity(fbe_xor_status_t *status_p,
                                      fbe_xor_option_t option,
                                      fbe_lba_t strip,
                                      fbe_sector_t* parity_blk_ptr,
                                      fbe_u16_t mask,
                                      fbe_u16_t array_width,
                                      fbe_u32_t parity_drives,
                                      fbe_u16_t parity_bitmask)
{
    fbe_u16_t unit_width_mask;    /* Mask of the unit width. */
    fbe_u32_t pcsum;    

    /* Compare the calculated checksum with that of
     * the checksum stored in the parity sector.
     */
    pcsum = xorlib_calc_csum(parity_blk_ptr->data_word);

    /* The unit_width_mask has all 1's set for the width
     * of the unit.
     */
    unit_width_mask = (1 << (array_width + parity_drives)) - 1;

    if (parity_blk_ptr->crc != xorlib_cook_csum(pcsum, (fbe_u32_t)strip))
    {
        /* Only mark crc error if we have not just come back from recovery verify.
         */
        if (!(option & FBE_XOR_OPTION_ALLOW_INVALIDS))
        {
            /* The checksums don't match! The block
             * may have gone bad on the disk...
             */
            fbe_xor_set_status(status_p, FBE_XOR_STATUS_CHECKSUM_ERROR);
        }
    } 
    else if ((parity_drives < 2) && (0 != parity_blk_ptr->lba_stamp))
    {
        /* If R5/R3, we shouldn't be doing this to shed data!
         * If R6, the parity of checksum is stored in the shed stamp field.
         */
        fbe_xor_set_status(status_p, FBE_XOR_STATUS_BAD_SHEDSTAMP);
    } 
    else if (parity_blk_ptr->write_stamp & (~unit_width_mask) ||
               (0 != (parity_blk_ptr->write_stamp & parity_bitmask)))
    {
        /* The parity had a write stamp bit set for the parity
         * position(s), or beyond the width of the unit.
         */
        fbe_xor_set_status(status_p, FBE_XOR_STATUS_BAD_METADATA);
    }
#ifdef XOR_FULL_METACHECK
    else if (0 != (parity_blk_ptr->time_stamp & FBE_SECTOR_ALL_TSTAMPS))
    {
        /* The ALL_TIMESTAMPS flag is set.
         */
        fbe_xor_set_status(status_p, FBE_XOR_STATUS_BAD_METADATA);
    }
#endif 

    return pcsum;               
}
/* END fbe_xor_check_parity */

/*****************************************
 * End of fbe_xor_interface_parity.c
 *****************************************/
