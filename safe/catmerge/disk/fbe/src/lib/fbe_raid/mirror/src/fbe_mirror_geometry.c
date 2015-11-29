/*******************************************************************
 * Copyright (C) EMC Corporation 1999 - 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*******************************************************************
 *  fbe_mirror_geometry.c
 *******************************************************************
 *
 * @brief
 *    This file contains functions for the mirror geometry interface.
 *
 * @author
 *  09/21/2009  Ron Proulx  - Created.
 *
 *******************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library.h"
#include "fbe_spare.h"
#include "fbe_mirror_io_private.h"

/*!********************************************************************
 *          fbe_mirror_get_geometry()
 **********************************************************************
 *
 * @brief   The routine configures the siots geometry information.
 *          For mirror raid groups the primary information in the siots
 *          geometry is the position array.  The position array is used
 *          to map from a primary, secondary, etc index to the actual
 *          primary, secondary, etc position.  Thus the fact that the
 *          primary position is different for a hot-spare vs a proactive
 *          spare vs a mirror is all hidden from the mirror algorithms.
 *
 * @param   raid_geometry_p - Pointer to raid geometry structure
 * @param   lba - physical starting block address
 * @param   blocks - number of blocks for this request
 * @param   geo - geometry information for request.
 *
 * @return  status - Typically FBE_STATUS_OK 
 *
 * @notes   This routine does not take into account dead positions.
 *          Thus it is up to the algorithms to update the index to
 *          position mapping by invoking fbe_mirror_geometry_update_positions()
 *          as required.
 *
 * @author
 *  4-Apr-00 kls - Created.
 * 16-Aug-00 RG - Added mirror support.
 * 10-Oct-00 kls - Touched to add expansion support.
 *  3-Dec-09 Ron Proulx - Update to use index to position mapping
 *
 *********************************************************************/
fbe_status_t fbe_mirror_get_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                     fbe_lba_t lba,
                                     fbe_block_count_t blocks,
                                     fbe_raid_siots_geometry_t *geo)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_element_size_t sectors_per_element;
    fbe_u32_t       width, pos;
    fbe_mirror_prefered_position_t prefered_mirror_pos_p = FBE_MIRROR_PREFERED_POSITION_INVALID;

    /* Init geo struct with a background pattern.
     * Optimized out for free builds.
     */
#if DBG
    fbe_set_memory(geo, -1, sizeof(fbe_raid_siots_geometry_t));
#endif

    /*! Set the information that comes from the geometry.
     *  Element size on mirrors is not used for breaking up the request and no striping 
     *  is performed on mirrors.
     */
    fbe_raid_geometry_get_element_size(raid_geometry_p, &sectors_per_element);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    /*! @note Currently none of the following fields are used by the mirror
     *        code.
     */
    geo->start_offset_rel_parity_stripe = 0;
    geo->blocks_remaining_in_parity = 0;
    geo->blocks_remaining_in_data = blocks;
    geo->logical_parity_start = lba;
    geo->logical_parity_count = blocks;
        
    /* Validate the width and populate the position array.
     */
    if ((width < FBE_MIRROR_MIN_WIDTH) ||
        (width > FBE_MIRROR_MAX_WIDTH)    )    
    {
        /* Invalid mirror width, report an error.
         */
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "mirror obj get geometry width: %d out of range. min: %d max: %d\n",
                                      width, FBE_MIRROR_MIN_WIDTH, FBE_MIRROR_MAX_WIDTH);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Else populate the position array assuming not dead positions.
         */
        for (pos = 0; pos < FBE_MIRROR_MAX_WIDTH; pos++)
        {
             geo->position[pos] = (fbe_u8_t)FBE_RAID_INVALID_POS;
        }

        /* Based on the raid type populate the position array.
         * The index into the position array is:
         *  o 0 - The primary (for a read operation) mirror position
         *  o 1 - The secondary mirror position
         *  o 2 - Tertiary position
         */
        if (fbe_raid_geometry_is_sparing_type(raid_geometry_p))
        {
            /* Width must be (2).
             */
            if (width != 2)
            {
                /* Trace the error and set the error status.
                 */
                fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                              FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                              "mirror obj get geometry hot-spare width: %d should be 2.\n",
                                              width);
                                
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                /* Else configure the hot-spare mirror group.  We simply use the 
                 * information provided by the virtual drive object. 
                 */
                geo->position[FBE_MIRROR_TERTIARY_INDEX] = (fbe_u8_t)FBE_RAID_INVALID_POS;
                fbe_raid_geometry_get_mirror_prefered_position(raid_geometry_p, &prefered_mirror_pos_p);
                switch(prefered_mirror_pos_p)
                {
                    case FBE_MIRROR_PREFERED_POSITION_ONE:
                        geo->position[FBE_MIRROR_PRIMARY_INDEX] = 0;
                        geo->position[FBE_MIRROR_SECONDARY_INDEX] = 1;
                        break;

                    case FBE_MIRROR_PREFERED_POSITIOM_TWO:
                        geo->position[FBE_MIRROR_PRIMARY_INDEX] = 1;
                        geo->position[FBE_MIRROR_SECONDARY_INDEX] = 0;
                        break;

                    default:
                        /* Trace the error and set the error status.
                         */
                        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                                      "mirror obj get geometry - invalid preferred position: %d \n",
                                                      prefered_mirror_pos_p);
                        
                        status = FBE_STATUS_GENERIC_FAILURE;
                }
            }
        }
        else
        {
            /* Else standard mirror type.  Use a one-to-one mapping between
             * index and position. (Again, checking and handling of degraded
             * mirror groups is not handled here).
             */
            switch(width)
            {
                case 3:
                    geo->position[FBE_MIRROR_TERTIARY_INDEX] = FBE_MIRROR_DEFAULT_TERTIARY_POSITION;
                    /* Fall thru */

                case 2:
                    geo->position[FBE_MIRROR_SECONDARY_INDEX] = FBE_MIRROR_DEFAULT_SECONDARY_POSITION;
                    /* Fall thru */

                case 1:
                    geo->position[FBE_MIRROR_PRIMARY_INDEX] = FBE_MIRROR_DEFAULT_PRIMARY_POSITION;
                    break;

                default:
                    /* This is unexpected.
                     */
                    fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                                  FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                                  "mirror obj get geometry unexpected mirror width: %d\n",
                                                  width);
                    
                    status = FBE_STATUS_GENERIC_FAILURE;
                    break;
            } /* end switch on mirror width */
        } /* end else if standard mirror type */
    } /* end else if width is within range */

    /* The start index is always the primary index.
     */
    geo->start_index = FBE_MIRROR_PRIMARY_INDEX;

    /* Always return the execution status.
     */
    return(status);
}
/*****************************************
 * end fbe_mirror_get_geometry
 *****************************************/

/*!***************************************************************************
 *          fbe_mirror_update_position_map()
 *****************************************************************************
 *
 * @brief   We have determined that a new primary position for a particular
 *          request is desired.  We validate that it is ok to move the selected
 *          position to the primary index.
 *
 * @param   siots_p - Pointer to request containing position map
 * @param   new_primary_position - Position to set in primary index
 *
 * @return  status - Typically FBE_STATUS_OK 
 *
 * @author
 *  12/09/2009  Ron Proulx - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_update_position_map(fbe_raid_siots_t *siots_p,
                                            fbe_u32_t new_primary_position)
{
    /* The default status is that something is wrong.
     */
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t       index;
    fbe_u32_t       current_primary_position = fbe_mirror_get_primary_position(siots_p);

    /* Walk thru the siots position map to locate the index where
     * the new position is located.
     */
    for (index = 0; index < width; index++)
    {
        if (fbe_mirror_get_position_from_index(siots_p, index) == new_primary_position)
        {
            /* We have located the new position.
             */
            status = FBE_STATUS_OK;
            break;
        }
    }

    /* If the status is ok swap the failed and new primary positions.
     */
    if (status == FBE_STATUS_OK)
    {
        /* Since we are swapping positions due to an error we don't
         * allow the new primary position to be the same as the current.
         */
        if (index == FBE_MIRROR_PRIMARY_INDEX)
        {
            /* Fail with a generic failure.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            /* Else update the position maps.
             */
            fbe_mirror_set_position_for_index(siots_p,
                                              FBE_MIRROR_PRIMARY_INDEX,
                                              new_primary_position);
            fbe_mirror_set_position_for_index(siots_p,
                                              index,
                                              current_primary_position);
        }
    }

    /* Trace an error if the request failed
     */
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: attempt to change primary pos: %d to %d failed \n",
                             current_primary_position, new_primary_position);
        return status;
    }
    /* Always return the execution status.
     */
    return(status);
}
/*****************************************
 * end fbe_mirror_update_position_map()
 *****************************************/

/*!***************************************************************************
 *          fbe_mirror_optimization_update_position_map()
 *****************************************************************************
 *
 * @brief   We have determined that a new primary position for a particular
 *          request is desired.  We validate that it is ok to move the selected
 *          position to the primary index.
 *
 * @param   siots_p - Pointer to request containing position map
 * @param   new_primary_position - Position to set in primary index
 *
 * @return  status - Typically FBE_STATUS_OK 
 *
 * @author
 *  06/08/2011  Swati Fursule - Created from fbe_mirror_update_position_map
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_optimization_update_position_map(fbe_raid_siots_t *siots_p,
                                            fbe_u32_t new_primary_position)
{
    /* The default status is that something is wrong.
     */
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t       index;
    fbe_u32_t       current_primary_position = fbe_mirror_get_primary_position(siots_p);

    /* Walk thru the siots position map to locate the index where
     * the new position is located.
     */
    for (index = 0; index < width; index++)
    {
        if (fbe_mirror_get_position_from_index(siots_p, index) == new_primary_position)
        {
            /* We have located the new position.
             */
            status = FBE_STATUS_OK;
            break;
        }
    }

    /* If the status is ok swap the failed and new primary positions.
     */
    if (status == FBE_STATUS_OK)
    {
        /* Since we are swapping positions due to an error we don't
         * allow the new primary position to be the same as the current.
         */
        if (index == FBE_MIRROR_PRIMARY_INDEX)
        {
            /* Fail with a generic failure.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            /* Else update the position maps.
             */
            fbe_mirror_set_position_for_index(siots_p,
                                              FBE_MIRROR_PRIMARY_INDEX,
                                              new_primary_position);
            fbe_mirror_set_position_for_index(siots_p,
                                              index,
                                              current_primary_position);
        }
    }
    /* Always return the execution status.
     */
    return(status);
}
/*****************************************
 * end fbe_mirror_optimization_update_position_map()
 *****************************************/

/***************************************************************
 * fbe_mirror_get_small_read_geometry()
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
 *  2/9/2012 - Created. chenl6
 *
 ****************************************************************/

fbe_status_t fbe_mirror_get_small_read_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_lba_t lba,
                                                fbe_raid_small_read_geometry_t * geo)
{
    fbe_u32_t width;
    fbe_mirror_prefered_position_t prefered_mirror_pos = FBE_MIRROR_PREFERED_POSITION_INVALID;

    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    geo->start_offset_rel_parity_stripe = 0;
    geo->logical_parity_start = lba;

    {
        if (fbe_raid_geometry_is_sparing_type(raid_geometry_p))
        {
            fbe_raid_geometry_get_mirror_prefered_position(raid_geometry_p, &prefered_mirror_pos);
            geo->position = (prefered_mirror_pos == FBE_MIRROR_PREFERED_POSITION_ONE)? 0 : 1;
        }
        else
        {
            geo->position = FBE_MIRROR_DEFAULT_PRIMARY_POSITION;
        }
    }

    if (RAID_DBG_ENABLED)
    {
        fbe_raid_siots_geometry_t siots_geometry;
        fbe_mirror_get_geometry(raid_geometry_p, lba, 0, &siots_geometry);

        if (RAID_COND_GEO(raid_geometry_p, geo->position != siots_geometry.position[siots_geometry.start_index]))
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "mirror get small geo position %d != position %d\n",
                                         geo->position,
                                         siots_geometry.position[siots_geometry.start_index]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_COND_GEO(raid_geometry_p, geo->logical_parity_start != siots_geometry.logical_parity_start))
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "mirror get small geo log parity start 0x%llx != position 0x%llx\n",
                                         (unsigned long long)geo->logical_parity_start,
                                         (unsigned long long)siots_geometry.logical_parity_start);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_COND_GEO(raid_geometry_p, geo->start_offset_rel_parity_stripe != siots_geometry.start_offset_rel_parity_stripe))
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "mirror get small geo offset rel parity stripe 0x%llx != position 0x%llx\n",
                                         (unsigned long long)geo->start_offset_rel_parity_stripe,
                                         (unsigned long long)siots_geometry.start_offset_rel_parity_stripe);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_mirror_get_small_read_geometry
 *****************************************/

/*****************************************
 * end of  fbe_mirror_geometry.c
 *****************************************/
