 /*******************************************************************
 * Copyright (C) EMC Corporation 1999 - 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*******************************************************************
 *  fbe_parity_geometry.c
 *******************************************************************
 *
 * @brief
 *  This file contains functions of the parity geometry interface.
 *
 * @author
 *  7/8/09: - Created. Rob Foley
 *
 *******************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_geometry.h"
#include "fbe_parity_io_private.h"

/***************************************************************
 * fbe_parity_get_lun_geometry()
 ***************************************************************
 * @brief
 *   Determine the geometry of a given parity stripe
 * 
 * @param raid_geometry_p - Pointer to raid geometry structure
 * @param lba - host address
 * @param geo - geometry information.
 *
 * @return
 *  fbe_bool_t - FBE_FALSE on failure, FBE_TRUE on success
 *         Today we only return true, but
 *         in the future we may return false.
 *
 * @notes
 *
 * @author
 *  21-Mar-00 Rob Foley - Created.
 *  10-Oct-00 kls - Touched to add expansion support.
 *
 ****************************************************************/

fbe_status_t fbe_parity_get_lun_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                         fbe_lba_t lba,
                                         fbe_raid_siots_geometry_t * geo)
{
    fbe_status_t status;
    fbe_lba_t stripe_number;
    fbe_lba_t parity_stripe_number;
    fbe_lba_t parity_stripe_offset;
    fbe_block_count_t blocks_per_parity_stripe;
    fbe_block_count_t blocks_per_data_stripe;
    fbe_element_size_t sectors_per_element;
    fbe_elements_per_parity_t elements_per_parity_stripe;
    fbe_u32_t width;

    /* Raid 6 has 2 parity drives, everything else has 1.
     */
    fbe_u16_t parity_drives = 0;

    status = fbe_raid_geometry_get_parity_disks(raid_geometry_p, &parity_drives);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "fbe_raid_geometry_get_parity_disks returned status %d\n", 
                             status);
        return status;
    }

    /* Init geo struct with a background pattern.
     */
    //fbe_set_memory(geo, (fbe_u8_t)FBE_RAID_INVALID_POS, sizeof(geo));

    /*
     * Stage 0: Init basic geometry information.
     */
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_geometry_get_element_size(raid_geometry_p, &sectors_per_element);
    fbe_raid_geometry_get_elements_per_parity(raid_geometry_p, &elements_per_parity_stripe);

    /*
     * Stage 1: Perform initial calculations.
     */
    blocks_per_data_stripe = ((fbe_block_count_t)sectors_per_element) * (width - parity_drives);

    stripe_number = lba / blocks_per_data_stripe;

    parity_stripe_number = stripe_number / elements_per_parity_stripe;

    parity_stripe_offset = parity_stripe_number * elements_per_parity_stripe * 
        sectors_per_element;

    blocks_per_parity_stripe = blocks_per_data_stripe * elements_per_parity_stripe;

    /*
     * Stage 2a: Fill in rest of geometry structure.
     */

    /* Determine the start DATA position
     * 0..(num_frus - 1)
     */
    geo->start_index = (fbe_u32_t)
        ((lba / sectors_per_element) %
         (width - parity_drives));

    /* Blocks left in parity is calculated by
     * subtracting the blocks before the I/O start from
     * the total blocks in the parity stripe.
     */
    geo->blocks_remaining_in_parity = blocks_per_parity_stripe -
        (lba - (parity_stripe_number * blocks_per_parity_stripe));

    /* Blocks remaining in data element is just
     * blocks to end of first data element in this I/O.
     */
    geo->blocks_remaining_in_data = (sectors_per_element - (lba % sectors_per_element));

    if ( geo->blocks_remaining_in_parity > FBE_U32_MAX )
    {
        /* If the max_blocks is beyond the max block count
         * we support, then truncate it to 32 bits.
         * We can't possibly do an I/O of this size anyway.
         */
        geo->max_blocks = FBE_U32_MAX;
    }
    else
    {
        /* Note that we do a cast after checking to see if any truncation is needed. 
         */
        geo->max_blocks = (fbe_block_count_t)geo->blocks_remaining_in_parity;
    }


    /* Start offset relative to parity stripe
     * is # of full stripe elements before this element,
     * plus the offset into this stripe element.
     */
    geo->start_offset_rel_parity_stripe = (fbe_u32_t)
        (((stripe_number - (parity_stripe_number *
            elements_per_parity_stripe)) *
          sectors_per_element) +
         (lba % sectors_per_element));

    geo->logical_parity_start = parity_stripe_offset;
    geo->logical_parity_count = sectors_per_element * elements_per_parity_stripe;

    /* We assume that RAID 5/6 units will
     * only be comprised of 3 or more drives...
     */
    if (RAID_DBG_COND((parity_drives == 2) && (width < FBE_PARITY_R6_MIN_WIDTH)))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "width %d not expected for parity drives %d min is %d\n", 
                             width, parity_drives, FBE_PARITY_R6_MIN_WIDTH);
        return FBE_STATUS_GENERIC_FAILURE;
    } 
    if (RAID_DBG_COND((parity_drives == 1) && (width < FBE_PARITY_MIN_WIDTH)))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "width %d not expected for parity drives %d min is %d\n", 
                             width, parity_drives, FBE_PARITY_MIN_WIDTH);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_COND(width > FBE_RAID_MAX_DISK_ARRAY_WIDTH))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "width %d not expected max is %d\n", 
                             width, FBE_RAID_MAX_DISK_ARRAY_WIDTH);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_COND((parity_drives == 0) || (parity_drives > 2)))
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "parity drives %d not expected\n",parity_drives);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Stage 2b: Fill in extent portion of geometry structure.
     */
    {
        fbe_u32_t parity_index;
        fbe_u32_t dparity_index = 0;
        fbe_u32_t i, index;
        fbe_bool_t b_raid_3 = fbe_raid_geometry_is_raid3(raid_geometry_p);

#if defined(LU_RAID5_RIGHT_ASSYMETRIC) || defined(LU_RAID5_LEFT_ASSYMETRIC)
        if (b_raid_3)
        {
            /* Bandwidth units have a parity index of zero.
             */
            parity_index = 0;
        }
        else
        {
            /* For Right asymmetric, the parity rotates "left" to "right"
             */
#ifdef LU_RAID5_LEFT_ASSYMETRIC
            parity_index = (width - parity_drives) - (parity_stripe_number % width);
#else /* LU_RAID5_RIGHT_ASSYMETRIC */
            parity_index = (fbe_u32_t) (parity_stripe_number % width);
#endif
        }

        for (i = 0, index = 0; i < width; i++)
        {
            if (i == parity_index)
            {
                geo->position[width - parity_drives] = i;
            }
            else
            {
                geo->position[index] = i;
                index++;
            }
        }
#elif defined(LU_RAID5_LEFT_SYMMETRIC)  || defined(LU_RAID5_RIGHT_SYMMETRIC)
        fbe_u32_t array_pos;

        if (b_raid_3)
        {
            /* Bandwidth units have a parity index of zero.
             */
            parity_index = 0;
        }
        else if (parity_drives == 2)
        {
            /* The parity index is calculated according to the following formula:
             * parity stripe number * 2 % width.
             * Diagonal parity is just the next drive, so we add one.
             */
            parity_index = (fbe_u32_t)((parity_stripe_number * 2) % width);
            dparity_index = (fbe_u32_t)(((parity_stripe_number * 2) + 1) % width);
        }
        else
        {
#ifdef LU_RAID5_LEFT_SYMMETRIC
            /* Since we have a left symmetric layout,
             * parity rotates from "right" to "left".
             */
            parity_index = (width - parity_drives) - 
                (fbe_u32_t)(parity_stripe_number % width);
#else /* LU_RAID5_RIGHT_SYMMETRIC */
            /* Since we have a left symmetric layout,
             * parity rotates from "left" to "right".
             */
            parity_index = (fbe_u32_t) (parity_stripe_number % width);
#endif
        }

        /* Since we have a left symmetric layout,
         * the first data drive begins just after
         * the parity drive, modulo width.
         */
        array_pos = parity_index;

        for (i = 0, index = 0; i < width; i++)
        {
#ifdef LU_RAID5_LEFT_SYMMETRIC
            /* Data rotates left to right
             */
            array_pos = (array_pos + 1) % width;
#else /* LU_RAID5_RIGHT_SYMMETRIC */
            /* Data rotates from right to left
             */
            array_pos = (array_pos == 0) ? (width - 1) : (array_pos - 1);
#endif
            if ( parity_drives == 2 && array_pos == dparity_index )
            {
                /* This is the RAID 6 diagonal parity case.
                 * For RAID 6 the parity goes in the last 2 indexes
                 * in the position array, with the Row Parity coming before
                 * the Diagonal Parity.
                 */
                geo->position[width - 1] = array_pos;
            }
            else if (array_pos == parity_index)
            {
                /* This case is used by R5, R3, R6.
                 */
                geo->position[width - parity_drives] = array_pos;
            }
            else
            {
                geo->position[index] = array_pos;
                if (index < parity_index)
                {
                    if (array_pos != (parity_index - index - 1))
                    {
                        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                             "incorrect array position.\n");
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }
                else
                {
                    if (array_pos != (parity_index - index - 1 + width))
                    {
                        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                             "incorrect array position 2.\n");
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }
                index++;
            }
        }
#endif /* LU_RAID5_LEFT_SYMMETRIC and LU_RAID5_RIGHT_SYMMETRIC */
    }
    if (RAID_DBG_ENABLED)
    {
        fbe_raid_extent_t parity_range[2];

        fbe_raid_get_stripe_range(
            parity_stripe_number * blocks_per_parity_stripe,
            sectors_per_element *
                elements_per_parity_stripe *
                (width - 1),
            sectors_per_element,
            width - parity_drives,
            2,
            parity_range);

        /* Validate our geometry structure agrees with
         * what fbe_raid_get_stripe_range gave us.
         */
        if (geo->logical_parity_start != parity_stripe_offset)
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "logical parity start %llx not pstripe offset %llx\n",
                                 (unsigned long long)geo->logical_parity_start, 
                                 (unsigned long long)parity_stripe_offset);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (geo->logical_parity_count != (sectors_per_element * elements_per_parity_stripe))
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "parity count %llx not expected elsz %d %d\n",
                                 (unsigned long long)geo->logical_parity_count, 
                                 sectors_per_element, elements_per_parity_stripe);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Make sure we at least filled in lbas and positions
         * for the first and last extent entries.
         */
        if (geo->position[width - 1] == (fbe_u8_t)FBE_RAID_INVALID_POS)
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "position %d not set for last entry %d\n",
                                 geo->position[width - 1], width - 1);
            return FBE_STATUS_GENERIC_FAILURE;

        }
        if (geo->position[0] == (fbe_u8_t)FBE_RAID_INVALID_POS)
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS, "position 0 not set\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Make sure we have not duplicated any positions.
     */
    if (RAID_DBG_ENABLED)
    {
        fbe_u32_t pos_bitmask = 0;
        fbe_u32_t index;
        
        for (index = 0; index < width; index++)
        {
            fbe_u32_t current_bit = 1 << geo->position[index];
            
            if ( (current_bit & pos_bitmask) != 0 )
            {
                fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                     "position %d set twice at index %d\n",
                                     geo->position[index], index);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            
            pos_bitmask |= current_bit;
        }
    }

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_get_lun_geometry
 *****************************************/

/***************************************************************
 * fbe_parity_get_physical_geometry()
 ***************************************************************
 * @brief
 *  Determine geometry for a given request.
 *
 * @param raid_geometry_p - Pointer to raid geometry structure
 * @param pba - parity relative logical address
 * @param geo - geometry information for request.
 *
 * @return
 *  fbe_status_t - FBE_FALSE on failure, FBE_TRUE on success.
 *
 * @notes
 *
 * @author
 *  21-Mar-00 Rob Foley - Created.
 *  10-Oct-00 kls - Touched to add expansion support.
 *
 ****************************************************************/

fbe_status_t fbe_parity_get_physical_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                              fbe_lba_t pba,
                                              fbe_raid_siots_geometry_t * geo)
{
    fbe_status_t status;

    fbe_lba_t lba;
    fbe_element_size_t sectors_per_element;
    fbe_u32_t blocks_per_stripe;
    fbe_u32_t array_width;
    fbe_elements_per_parity_t elements_per_parity_stripe; 
    fbe_block_count_t sectors_per_parity_stripe;

    /* Raid 6 has 2 parity drives, everything else has 1.
     */
    fbe_u16_t parity_drives;

    status = fbe_raid_geometry_get_parity_disks(raid_geometry_p, &parity_drives);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "fbe_raid_geometry_get_parity_disks returned status %d\n", 
                             status);
        return status;
    }

    fbe_raid_geometry_get_width(raid_geometry_p, &array_width);
    fbe_raid_geometry_get_element_size(raid_geometry_p, &sectors_per_element);
    fbe_raid_geometry_get_elements_per_parity(raid_geometry_p, &elements_per_parity_stripe);

    blocks_per_stripe = sectors_per_element * (array_width - parity_drives);

    /* Convert the pba that got passed in to a logical block address.
     * We convert to the first data position.
     */
    lba = (pba / sectors_per_element)
        * blocks_per_stripe
        + (pba % sectors_per_element);

    status = fbe_parity_get_lun_geometry(raid_geometry_p, lba, geo);

    sectors_per_parity_stripe = ((fbe_block_count_t)elements_per_parity_stripe) * sectors_per_element;

    {
        fbe_block_count_t  new_bl_remaining = sectors_per_parity_stripe - geo->start_offset_rel_parity_stripe;

        fbe_block_count_t parity_stripe_size = new_bl_remaining + geo->start_offset_rel_parity_stripe;

        if (((parity_stripe_size % sectors_per_element) != 0) ||
            (sectors_per_parity_stripe != parity_stripe_size))
        {
            /* Our blocks remaining plus our offset from parity stripe
             * should equal a parity stripe.
             */
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "raid parity stripe size %llx not expected elsz: %x %llx\n", 
                                 (unsigned long long)parity_stripe_size,
				 sectors_per_element,
				 (unsigned long long)sectors_per_parity_stripe);
            return FBE_STATUS_GENERIC_FAILURE;
        } 
        if (geo->start_index != 0)
        {
            /* Since we are mapping a physical to logical,
             * We always map to the first data position.
             */
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "raid start index %d unexpected\n", geo->start_index);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    /* Convert the parity remaining to the
     * amount remaining on just one position.
     */
    geo->blocks_remaining_in_parity = sectors_per_parity_stripe - geo->start_offset_rel_parity_stripe;

    if (geo->blocks_remaining_in_parity == 0)
    { 
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "blocks remaining in parity %llx unexpected\n", 
                             (unsigned long long)geo->blocks_remaining_in_parity);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (geo->blocks_remaining_in_parity > sectors_per_parity_stripe)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "blocks remaining in parity %llx greater than parity stripe %llx\n", 
                             (unsigned long long)geo->blocks_remaining_in_parity,
			     (unsigned long long)sectors_per_parity_stripe);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (geo->start_offset_rel_parity_stripe >= sectors_per_parity_stripe)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "offset relative to parity stripe %llx >= than parity stripe %llx\n", 
                             (unsigned long long)geo->start_offset_rel_parity_stripe,
			     (unsigned long long)sectors_per_parity_stripe);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (((geo->start_offset_rel_parity_stripe + geo->blocks_remaining_in_parity) % 
         sectors_per_element) != 0)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "parity offset %llx + remaining in parity %llx >= not a multiple of elsz %llx\n", 
                             (unsigned long long)geo->start_offset_rel_parity_stripe, 
                             (unsigned long long)geo->blocks_remaining_in_parity,
                             (unsigned long long)sectors_per_parity_stripe);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/*****************************************
 * end fbe_parity_get_physical_geometry
 *****************************************/

/***************************************************************
 * fbe_parity_get_small_read_geometry()
 ***************************************************************
 * @brief
 *   Determine the geometry of a given parity stripe for reads.
 * 
 * @param raid_geometry_p - Pointer to raid geometry structure
 * @param lba - host address
 * @param geo - geometry information.
 *
 * @return
 *  fbe_bool_t - FBE_FALSE on failure, FBE_TRUE on success
 *         Today we only return true, but
 *         in the future we may return false.
 *
 * @notes
 *
 * @author
 *  2/3/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_get_small_read_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_lba_t lba,
                                                fbe_raid_small_read_geometry_t * geo)
{
    fbe_lba_t stripe_number;
    fbe_lba_t parity_stripe_number;
    fbe_lba_t parity_stripe_offset;
    fbe_block_count_t blocks_per_data_stripe;
    fbe_element_size_t sectors_per_element;
    fbe_elements_per_parity_t elements_per_parity_stripe;
    fbe_u32_t width;
    fbe_u32_t start_index;
    fbe_u32_t parity_drives;

    fbe_raid_geometry_get_num_parity(raid_geometry_p, &parity_drives);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_geometry_get_element_size(raid_geometry_p, &sectors_per_element);
    fbe_raid_geometry_get_elements_per_parity(raid_geometry_p, &elements_per_parity_stripe);

    blocks_per_data_stripe = ((fbe_block_count_t)sectors_per_element) * (width - parity_drives);
    stripe_number = lba / blocks_per_data_stripe;
    parity_stripe_number = stripe_number / elements_per_parity_stripe;
    parity_stripe_offset = parity_stripe_number * elements_per_parity_stripe * 
        sectors_per_element;

    /* Determine the start DATA position
     * 0..(num_frus - 1)
     */
    start_index = (fbe_u32_t) ((lba / sectors_per_element) % (width - parity_drives));

    /* Start offset relative to parity stripe
     * is # of full stripe elements before this element,
     * plus the offset into this stripe element.
     */
    geo->start_offset_rel_parity_stripe = (fbe_u32_t)
        (((stripe_number - (parity_stripe_number * elements_per_parity_stripe)) *
          sectors_per_element) + (lba % sectors_per_element));

    geo->logical_parity_start = parity_stripe_offset;

    {
        fbe_u32_t parity_index;

        if (fbe_raid_geometry_is_raid5(raid_geometry_p))
        {
            /* Since we have a left symmetric layout,
             * parity rotates from "left" to "right".
             */
            parity_index = (fbe_u32_t) (parity_stripe_number % width);
        }
        else if (fbe_raid_geometry_is_raid3(raid_geometry_p))
        {
            /* Bandwidth units have a parity index of zero.
             */
            parity_index = 0;
        }
        else /* raid 6 */
        {
            /* The parity index is calculated according to the following formula:
             * parity stripe number * 2 % width.
             * Diagonal parity is just the next drive, so we add one.
             */
            parity_index = (fbe_u32_t)((parity_stripe_number * 2) % width);
        }

        /* Since we have a left symmetric layout,
         * the first data drive begins just after
         * the parity drive, modulo width.
         */
        if (start_index < parity_index)
        {
            geo->position = parity_index - start_index - 1;
        }
        else
        {
            geo->position = parity_index - start_index - 1 + width;
        }
    }
    if (RAID_DBG_ENABLED)
    {
        fbe_raid_siots_geometry_t siots_geometry = {0};
        fbe_parity_get_lun_geometry(raid_geometry_p, lba, &siots_geometry);

        if (RAID_COND_GEO(raid_geometry_p, geo->position != siots_geometry.position[siots_geometry.start_index]))
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "parity get small geo position %d != position %d\n",
                                         geo->position,
                                         siots_geometry.position[siots_geometry.start_index]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_COND_GEO(raid_geometry_p, geo->logical_parity_start != siots_geometry.logical_parity_start))
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "parity get small geo log parity start 0x%llx != position 0x%llx\n",
                                         (unsigned long long)geo->logical_parity_start,
                                         (unsigned long long)siots_geometry.logical_parity_start);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_COND_GEO(raid_geometry_p, geo->start_offset_rel_parity_stripe != siots_geometry.start_offset_rel_parity_stripe))
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "parity get small geo offset rel parity stripe 0x%llx != position 0x%llx\n",
                                         (unsigned long long)geo->start_offset_rel_parity_stripe,
                                         (unsigned long long)siots_geometry.start_offset_rel_parity_stripe);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_get_small_read_geometry
 *****************************************/

/***************************************************************
 * fbe_parity_get_small_write_geometry()
 ***************************************************************
 * @brief
 *   Determine the geometry of a given parity stripe for write.
 * 
 * @param raid_geometry_p - Pointer to raid geometry structure
 * @param lba - host address
 * @param geo - geometry information.
 *
 * @return
 *  fbe_bool_t - FBE_FALSE on failure, FBE_TRUE on success
 *         Today we only return true, but
 *         in the future we may return false.
 *
 * @notes
 *
 * @author
 *  2/8/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_get_small_write_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                 fbe_lba_t lba,
                                                 fbe_raid_siots_geometry_t * geo)
{
    fbe_lba_t stripe_number;
    fbe_lba_t parity_stripe_number;
    fbe_lba_t parity_stripe_offset;
    fbe_block_count_t blocks_per_data_stripe;
    fbe_element_size_t sectors_per_element;
    fbe_elements_per_parity_t elements_per_parity_stripe;
    fbe_u32_t width;
    fbe_u32_t parity_drives;

    fbe_raid_geometry_get_num_parity(raid_geometry_p, &parity_drives);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_geometry_get_element_size(raid_geometry_p, &sectors_per_element);
    fbe_raid_geometry_get_elements_per_parity(raid_geometry_p, &elements_per_parity_stripe);

    blocks_per_data_stripe = ((fbe_block_count_t)sectors_per_element) * (width - parity_drives);
    stripe_number = lba / blocks_per_data_stripe;
    parity_stripe_number = stripe_number / elements_per_parity_stripe;
    parity_stripe_offset = parity_stripe_number * elements_per_parity_stripe * 
        sectors_per_element;

    /* Determine the start DATA position
     * 0..(num_frus - 1)
     */
    geo->start_index = (fbe_u32_t) ((lba / sectors_per_element) % (width - parity_drives));

    /* Start offset relative to parity stripe
     * is # of full stripe elements before this element,
     * plus the offset into this stripe element.
     */
    geo->start_offset_rel_parity_stripe = (fbe_u32_t)
        (((stripe_number - (parity_stripe_number * elements_per_parity_stripe)) *
          sectors_per_element) + (lba % sectors_per_element));

    geo->logical_parity_start = parity_stripe_offset;

    if (fbe_raid_geometry_is_raid5(raid_geometry_p))
    {
        /* Since we have a left symmetric layout,
         * parity rotates from "left" to "right".
         */
        geo->position[width - 1] = (fbe_u8_t) (parity_stripe_number % width);
    }
    else if (fbe_raid_geometry_is_raid3(raid_geometry_p))
    {
        /* Bandwidth units have a parity index of zero.
         */
        geo->position[width - 1] = 0;
    }
    else    /* raid 6 */
    {
        /* The parity index is calculated according to the following formula:
         * parity stripe number * 2 % width.
         * Diagonal parity is just the next drive, so we add one.
         */
        geo->position[width - parity_drives] = (fbe_u8_t)((parity_stripe_number * 2) % width);
        geo->position[width - 1] = (fbe_u8_t)(((parity_stripe_number * 2) + 1) % width);
    }

    /* Since we have a left symmetric layout,
     * the first data drive begins just after
     * the parity drive, modulo width.
     */
    if (geo->start_index < geo->position[width - parity_drives])
    {
        geo->position[geo->start_index] = geo->position[width - parity_drives] - geo->start_index - 1;
    }
    else
    {
        geo->position[geo->start_index] = geo->position[width - parity_drives] - geo->start_index - 1 + width;
    }
    if (RAID_DBG_ENABLED)
    {
        fbe_raid_siots_geometry_t siots_geometry = {0};
        fbe_parity_get_lun_geometry(raid_geometry_p, lba, &siots_geometry);

        if (geo->position[geo->start_index] != siots_geometry.position[siots_geometry.start_index])
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "parity get small geo position %d != position %d\n",
                                         geo->position[geo->start_index],
                                         siots_geometry.position[siots_geometry.start_index]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (geo->position[width - parity_drives] != siots_geometry.position[width - parity_drives])
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "parity get small geo parity position %d != position %d\n",
                                         geo->position[width - parity_drives],
                                         siots_geometry.position[width - parity_drives]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (geo->logical_parity_start != siots_geometry.logical_parity_start)
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "parity get small geo log parity start 0x%llx != position 0x%llx\n",
                                         (unsigned long long)geo->logical_parity_start,
                                         (unsigned long long)siots_geometry.logical_parity_start);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (geo->start_offset_rel_parity_stripe != siots_geometry.start_offset_rel_parity_stripe)
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "parity get small geo offset rel parity stripe 0x%llx != position 0x%llx\n",
                                         (unsigned long long)geo->start_offset_rel_parity_stripe,
                                         (unsigned long long)siots_geometry.start_offset_rel_parity_stripe);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_get_small_write_geometry
 *****************************************/
/*****************************************
 * end of  fbe_parity_geometry.c
 *****************************************/
