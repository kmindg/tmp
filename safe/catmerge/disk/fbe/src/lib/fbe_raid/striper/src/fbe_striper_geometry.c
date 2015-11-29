/*******************************************************************
 * Copyright (C) EMC Corporation 1999 - 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*******************************************************************
 *  fbe_striper_geometry.c
 *******************************************************************
 *
 * @brief
 *    This file contains functions of the geometry interface.
 *
 * @author
 *    7/8/09: RFoley - Created.
 *
 *******************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_library.h"

/***************************************************************
 * initLunGeometry()
 ***************************************************************
 * @brief
 *  Initialize the geometry structure to contain a
 *  predefined background pattern.
 *
 * @param geo - geometry structure to init
 *
 * @return
 *  none
 *
 * @notes
 *
 * @author
 *  23-Mar-00 Rob Foley - Created.
 *
 ****************************************************************/

void initLunGeometry(fbe_raid_siots_geometry_t * geo)
{
    /* Optimized out for free builds.
     */
#if DBG
    fbe_set_memory(geo, -1, sizeof(fbe_raid_siots_geometry_t));
#endif
    return;
}
/*****************************************
 * initLunGeometry
 *****************************************/

/***************************************************************
 * fbe_striper_get_geometry()
 ***************************************************************
 * @brief
 *  Determine geometry for a given request.
 *
 * @param raid_geometry_p - Pointer to raid geometry structure
 * @param lba - parity relative logical address
 * @param geo - geometry information for request.
 * @param max_window_size - maximum size of our window.
 * @param unit_width - Output variable that is the actual unit width.
 *                     This is necessary since the width we put into
 *                     the geo is half the unit width in the case
 *                     of mirrors.
 * @return
 *  fbe_bool_t - FBE_FALSE on failure, FBE_TRUE on success.
 *         Today we only return true, but
 *         in the future we may return false.
 *
 * @notes
 *
 * @author
 *  4-Apr-00 kls - Created.
 * 16-Aug-00 RG - Added mirror support.
 * 10-Oct-00 kls - Touched to add expansion support.
 *
 ****************************************************************/
fbe_status_t fbe_striper_get_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                      fbe_lba_t lba,
                                      fbe_raid_siots_geometry_t *geo)
{
    fbe_lba_t stripe_number;
    fbe_block_count_t blocks_per_data_stripe;
    fbe_element_size_t  sectors_per_element;
    fbe_raid_group_type_t raid_type;
    fbe_u16_t width;
    fbe_u32_t index;

    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &width);

    /* Init geo struct with a background pattern.
     */
    initLunGeometry(geo);

    /*
     * Stage 0: Init basic geometry information.
     */
    fbe_raid_geometry_get_element_size(raid_geometry_p, &sectors_per_element);

    /*
     * Stage 1: Perform initial calculations.
     */
    blocks_per_data_stripe = (fbe_block_count_t)sectors_per_element * width;
    stripe_number = lba / blocks_per_data_stripe;

    /*
     * Stage 2a: Fill in rest of geometry structure.
     */

    /* Determine the start DATA position
     * 0..(num_frus)
     */
    geo->start_index = (fbe_u32_t)(lba / sectors_per_element) % width;

    /* Start offset relative to parity stripe
     * is the offset into this stripe element.
     */
    geo->start_offset_rel_parity_stripe = lba % sectors_per_element;

    /* Blocks left in parity is calculated by multiplying the 
     * max backend limit blocks with the current array width.
     * We are calculating blocks left in parity for the current IO 
     * based on the lun bound on raid group.
     * Since this is for R0 and R1/0 we don't consider for parity and 
     * fixed max backend limit as 1024(FBE_RAID_MAX_BACKEND_IO_BLOCKS) blocks.
     */
    geo->blocks_remaining_in_parity = width * FBE_RAID_MAX_BACKEND_IO_BLOCKS;

    geo->max_blocks = FBE_U32_MAX;

    /* Blocks remaining in data element is just
     * blocks to end of first data element in this I/O.
     */
    geo->blocks_remaining_in_data =
        sectors_per_element -
        geo->start_offset_rel_parity_stripe;

    geo->logical_parity_start =
        stripe_number * sectors_per_element;

    geo->logical_parity_count = 
        (sectors_per_element *
        (FBE_RAID_MAX_BACKEND_IO_BLOCKS / sectors_per_element)); 

    if (RAID_COND_GEO(raid_geometry_p, width > FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH))
    {
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "striper: width %d unexpected\n", width);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Stage 2b: Fill in extent portion of geometry structure.
     */
    for (index = 0; index < width; index++)
    {
        geo->position[index] = index;
    }

    /* Make sure we at least filled in lbas and positions
     * for the first and last extent entries.
     */
    if (RAID_COND_GEO(raid_geometry_p, geo->position[width - 1] == (fbe_u8_t)FBE_RAID_INVALID_POS))
    {
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "striper: last position %d not set\n", width - 1);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_COND_GEO(raid_geometry_p, geo->position[0] == (fbe_u8_t)FBE_RAID_INVALID_POS))
    {
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "striper: first position not set\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_striper_get_geometry
 *****************************************/

/***************************************************************
 * fbe_striper_get_physical_geometry()
 ***************************************************************
 * @brief
 *  Determine geometry for a given request.
 * 
 * @param raid_geometry_p - The raid geometry structure pointer
 * @param pba - parity relative logical address
 * @param geo - geometry information for request.
 * @param max_window_size - maximum size of our window.
 * @param unit_width - Output variable that is the actual unit width.
 *                     This is necessary since the width we put into
 *                     the geo is half the unit width in the case
 *                     of mirrors.
 * @return
 *  fbe_status_t
 *
 * @notes
 *
 ****************************************************************/

fbe_status_t fbe_striper_get_physical_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                               fbe_lba_t pba,
                                               fbe_raid_siots_geometry_t *geo,
                                               fbe_u16_t *const unit_width)
{
    fbe_bool_t ret_val = FBE_STATUS_OK;

    fbe_lba_t lba;
    fbe_block_count_t blocks_per_stripe;
    fbe_u32_t             array_width;
    fbe_element_size_t  sectors_per_element;
    fbe_u16_t             width;

    fbe_raid_geometry_get_width(raid_geometry_p, &array_width);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &width);

    *unit_width = array_width;
    fbe_raid_geometry_get_element_size(raid_geometry_p, &sectors_per_element);
    
    blocks_per_stripe = (fbe_block_count_t)sectors_per_element * width;

    /* Convert the pba that got passed in to a logical block address.
     * We convert to the first data position.
     */
    lba = (pba / sectors_per_element) * blocks_per_stripe
        + (pba % sectors_per_element);

    ret_val = fbe_striper_get_geometry(raid_geometry_p, lba, geo);
    {
        fbe_lba_t new_bl_remaining = sectors_per_element - geo->start_offset_rel_parity_stripe;

        fbe_lba_t parity_stripe_size = new_bl_remaining + geo->start_offset_rel_parity_stripe;

        if ((0 != (parity_stripe_size % sectors_per_element)) ||
            (sectors_per_element != parity_stripe_size))
        {
            /* Our blocks remaining plus our offset from parity stripe
             * should equal a parity stripe.
             */
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (geo->start_index != 0)
        {
            /* Since we are mapping a physical to logical,
             * We always map to the first data position.
             */
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    /* Convert the parity remaining to the
     * amount remaining on just one position.
     */
    geo->blocks_remaining_in_parity = sectors_per_element - geo->start_offset_rel_parity_stripe;

    if (RAID_COND_GEO(raid_geometry_p, geo->blocks_remaining_in_parity == 0))
    {
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "striper: blocks remaining in parity is 0\n");
        return FBE_STATUS_GENERIC_FAILURE;
    } 
    if (RAID_COND_GEO(raid_geometry_p, geo->blocks_remaining_in_parity > sectors_per_element))
    {
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "striper: blocks remaining in parity %llx > element %llx\n",
                                      (unsigned long long)geo->blocks_remaining_in_parity,
				      (unsigned long long)sectors_per_element);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_COND_GEO(raid_geometry_p, geo->start_offset_rel_parity_stripe >= sectors_per_element))
    {
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "striper: start offset rel stripe 0x%llx > element %llx\n",
                                     (unsigned long long)geo->start_offset_rel_parity_stripe,
				     (unsigned long long)sectors_per_element);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (RAID_COND_GEO(raid_geometry_p, ((geo->start_offset_rel_parity_stripe + geo->blocks_remaining_in_parity) % 
                   sectors_per_element) != 0))
    {
    
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "striper: start offset rel stripe 0x%llx + rem 0x%llx > element %llx\n",
                                      (unsigned long long)geo->start_offset_rel_parity_stripe, 
                                      (unsigned long long)geo->blocks_remaining_in_parity,
                                      (unsigned long long)sectors_per_element);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return ret_val;
}
/*****************************************
 * end fbe_striper_get_physical_geometry
 *****************************************/

/***************************************************************
 * fbe_striper_get_small_read_geometry()
 ***************************************************************
 * @brief
 *   Determine the geometry of a given stripe for reads.
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
 *  02/21/2012 - Created. chenl6
 *
 ****************************************************************/

fbe_status_t fbe_striper_get_small_read_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                 fbe_lba_t lba,
                                                 fbe_raid_small_read_geometry_t * geo)
{
    fbe_lba_t stripe_number;
    fbe_block_count_t blocks_per_data_stripe;
    fbe_element_size_t sectors_per_element;
    fbe_u16_t data_disks;
    fbe_u32_t start_index;

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_geometry_get_element_size(raid_geometry_p, &sectors_per_element);

    blocks_per_data_stripe = (fbe_block_count_t)sectors_per_element * data_disks;
    stripe_number = lba / blocks_per_data_stripe;

    /* Determine the start DATA position
     * 0..(num_frus)
     */
    start_index = (fbe_u32_t)(lba / sectors_per_element) % data_disks;

    /* Start offset relative to parity stripe
     * is the offset into this stripe element.
     */
    geo->start_offset_rel_parity_stripe = lba % sectors_per_element;
    geo->logical_parity_start = stripe_number * sectors_per_element;
    geo->position = start_index;

    /* Additional checking is enabled.
     */
    if (RAID_DBG_ENABLED)
    {
        fbe_raid_siots_geometry_t siots_geometry;
        fbe_striper_get_geometry(raid_geometry_p, lba, &siots_geometry);

        if (geo->position != siots_geometry.position[siots_geometry.start_index])
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "striper get small geo position %d != position %d\n",
                                         geo->position,
                                         siots_geometry.position[siots_geometry.start_index]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (geo->logical_parity_start != siots_geometry.logical_parity_start)
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "striper get small geo log parity start 0x%llx != position 0x%llx\n",
                                         (unsigned long long)geo->logical_parity_start,
                                         (unsigned long long)siots_geometry.logical_parity_start);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (geo->start_offset_rel_parity_stripe != siots_geometry.start_offset_rel_parity_stripe)
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "striper get small geo offset rel parity stripe 0x%llx != position 0x%llx\n",
                                         (unsigned long long)geo->start_offset_rel_parity_stripe,
                                         (unsigned long long)siots_geometry.start_offset_rel_parity_stripe);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    }

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_striper_get_small_read_geometry
 *****************************************/

/*****************************************
 * end of  fbe_raid_geometry.c
 *****************************************/
