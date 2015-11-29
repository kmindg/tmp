/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_read_optimization.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the read optimization specifically for mirrors 
 *
 * @version
 *  05/23/2011  Swati Fursule 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_mirror.h"
#include "fbe/fbe_atomic.h"
#include "fbe_mirror_io_private.h"


/*************************
 *   FORWARD DECLARATIONS 
 *************************/

fbe_status_t fbe_mirror_optimize_determine_position(fbe_raid_geometry_t *raid_geometry_p,
                                                    fbe_raid_position_t *position,
                                                    fbe_payload_block_operation_opcode_t opcode,
                                                    fbe_lba_t lba,
                                                    fbe_block_count_t blocks);

/*!***************************************************************
 *          fbe_mirror_nway_mirror_optimize_fruts_start()
 ****************************************************************
 *
 * @brief   If optimization is enabled on this fru and this is a read,
 *   then attempt to optimize reads.
 *   If this is not a read, or optimization is not enabled,
 *   then just track the lba/opcode.
 *
 * @param fruts_p - ptr to the fbe_raid_fruts_t to operate on
 *
 * @return VALUE:
 *      This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  05/23/2011  Swati Fursule  - Created from nway_mirror_optimize_fruts_start
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_nway_mirror_optimize_fruts_start(fbe_raid_siots_t * siots_p,
                                                                  fbe_raid_fruts_t *fruts_p)
{
    fbe_raid_geometry_t                    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_mirror_optimization_t              *mirror_db_p = NULL;
    fbe_raid_position_t                     new_position = fruts_p->position;
    fbe_lba_t                               lba = fruts_p->lba;
    fbe_block_count_t                       blocks = fruts_p->blocks;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t             degraded_bitmask;

    /* Trace the fact that we are clearing the continue received
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);

    /* Get the mirror optimization structure pointers
     */
    fbe_raid_geometry_get_mirror_opt_db(raid_geometry_p, &mirror_db_p);
    if(mirror_db_p == NULL)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "mirror:  optimize failed to get mirror db for obj: 0x%x\n",
                                     fbe_raid_geometry_get_object_id(raid_geometry_p));
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_OPTIMIZE) && (degraded_bitmask == 0))
    {
        /* We set this flag here since we are modifying a count inside the mirror opt db. 
         * We clear this out when the request finishes and we decrement the count.
         */
        fbe_raid_fruts_set_flag(fruts_p, FBE_RAID_FRUTS_FLAG_FINISH_OPTIMIZE);
        status = fbe_mirror_optimize_determine_position(raid_geometry_p, &new_position, fruts_p->opcode, lba, blocks);
    }
        
    /* Attempt to update the preferred read position.
     */
    if(status == FBE_STATUS_OK)
    {
        status = fbe_mirror_optimization_update_position_map(siots_p, new_position);
        if(status == FBE_STATUS_OK)
        {
            /* update the read fruts position in the siots.
             */
            status = fbe_mirror_read_update_read_fruts_position(siots_p,
                                                                fruts_p->position,
                                                                new_position,
                                                                FBE_FALSE); /*do not log*/
            if(status == FBE_STATUS_OK)
            {
                fruts_p->position = new_position;
            }
        }
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}

/********************************************************************
 * end fbe_mirror_nway_mirror_optimize_fruts_start()
 ********************************************************************/

/*!***************************************************************
 *          fbe_mirror_optimize_fruts_finish()
 ****************************************************************
 *
 * @brief   
 *   Just track the lba/opcode by decrementing I/O counts.
 *
 * @param fruts_p - ptr to the fbe_raid_fruts_t to operate on
 *
 * @return VALUE:
 *      This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  05/23/2011  Swati Fursule  - Created from mirror_optimize_fruts_finish
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_optimize_fruts_finish(fbe_raid_siots_t *siots_p,
                                                         fbe_raid_fruts_t *fruts_p)
{
    fbe_mirror_optimization_t              *mirror_db_p = NULL;
    fbe_raid_geometry_t                    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t                              pos = fruts_p->position;

    fbe_raid_fruts_clear_flag(fruts_p, FBE_RAID_FRUTS_FLAG_OPTIMIZE);
    if (!fbe_raid_fruts_is_flag_set(fruts_p, FBE_RAID_FRUTS_FLAG_FINISH_OPTIMIZE))
    {
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    fbe_raid_geometry_get_mirror_opt_db(raid_geometry_p, &mirror_db_p);
    if(mirror_db_p == NULL)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "mirror:  optimize finish failed to get mirror db for obj: 0x%x\n",
                                     fbe_raid_geometry_get_object_id(raid_geometry_p));
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
    {
        fbe_spinlock_lock(&mirror_db_p->spinlock);
        if (mirror_db_p->num_reads_outstanding[pos] == 0)
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "mirror:  try to decrement object: 0x%x num_reads_outst: 0x%x position: 0x%x\n",
                                         fbe_raid_geometry_get_object_id(raid_geometry_p),
                                         mirror_db_p->num_reads_outstanding[pos],
                                         pos);
        }
        else
        {
            mirror_db_p->num_reads_outstanding[pos]--;
        }
        fbe_raid_fruts_clear_flag(fruts_p, FBE_RAID_FRUTS_FLAG_FINISH_OPTIMIZE);
        fbe_spinlock_unlock(&mirror_db_p->spinlock);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;

}
/********************************************************************
 * end fbe_mirror_optimize_fruts_finish()
 ********************************************************************/

/*!**************************************************************
 * fbe_mirror_optimize_determine_position()
 ****************************************************************
 * @brief
 *  This function optimizes mirror read in direct IO path.
 * 
 * @param raid_geometry_p - raid geometry pointer
 * @param position - position in the RAID group
 * @param opcode - read or write
 *
 * @return fbe_status_t
 *
 * @author
 *  02/24/2012 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t fbe_mirror_optimize_determine_position(fbe_raid_geometry_t *raid_geometry_p,
                                                    fbe_raid_position_t *position,
                                                    fbe_payload_block_operation_opcode_t opcode,
                                                    fbe_lba_t lba,
                                                    fbe_block_count_t blocks)
{
    fbe_mirror_optimization_t  *mirror_db_p         = NULL;
    fbe_bool_t                  is_sequential       = false;
    fbe_u32_t                   new_position        = 0;
    fbe_u32_t                   pos;
    fbe_u32_t                   width;

    fbe_raid_geometry_get_mirror_opt_db(raid_geometry_p, &mirror_db_p);
    if(mirror_db_p == NULL)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "mirror:  optimize determine failed to get mirror db for obj: 0x%x\n",
                                     fbe_raid_geometry_get_object_id(raid_geometry_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
    {
        /****************************************************************************
         * Mirror Optimization algorithm 
         * If it looks like we are going in sequential to a half of the mirror, then
         * we will ship the I/O off to that half. Otherwise, we will attempt to load
         * balance. For flash drives, sequential optimization is not used.
         ***************************************************************************/

        fbe_raid_geometry_get_width(raid_geometry_p, &width);
        fbe_spinlock_lock(&mirror_db_p->spinlock);
        if (!fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED))
        {
            for (pos = 0; pos < width; pos++)
            {
                is_sequential = (mirror_db_p->current_lba[pos] == lba);
                if(is_sequential)
                {
                    mirror_db_p->current_lba[pos] = lba + blocks; /*update next lba*/
                    new_position = pos;
                    break;
                }
            }
        }

        if (!is_sequential) /*and RotationalRate != 0*/
        {
            /*********************************************************************
             * Load Balancing algorithm:
             * When not degraded, load balance reads between disks in the mirror.
             * Writes are ignored as they always write to all disks in the mirror.
             *********************************************************************/

            fbe_u32_t qdepth;
            fbe_u32_t min = 0xFFFFFFFF;

            for (pos = new_position = 0; pos < width; pos++)
            {
                qdepth = mirror_db_p->num_reads_outstanding[pos]; 
                if (qdepth < min)
                {
                    min = qdepth;
                    new_position = pos;
                }
            }
        }
        mirror_db_p->num_reads_outstanding[new_position]++;
        fbe_spinlock_unlock(&mirror_db_p->spinlock);
    }

    *position = new_position;

    return FBE_STATUS_OK;
}
/********************************************************************
 * end fbe_mirror_optimize_determine_position()
 ********************************************************************/

/*!**************************************************************
 * fbe_mirror_optimize_direct_io_finish()
 ****************************************************************
 * @brief
 *  This function decrements I/O count after direct IO finishes.
 * 
 * @param raid_geometry_p - raid geometry pointer
 * @param position - position in the RAID group
 * @param opcode - read or write
 *
 * @return fbe_status_t
 *
 * @author
 *  02/24/2012 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t fbe_mirror_optimize_direct_io_finish(fbe_raid_geometry_t *raid_geometry_p,
                                                  fbe_payload_block_operation_t *block_operation_p)
{
    fbe_mirror_optimization_t              *mirror_db_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u32_t position = 0;
    fbe_block_edge_t *block_edge_p = NULL;
    
    fbe_raid_geometry_get_mirror_opt_db(raid_geometry_p, &mirror_db_p);
    if(mirror_db_p == NULL)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "mirror:  directio optimize failed to get mirror db for obj: 0x%x\n",
                                     fbe_raid_geometry_get_object_id(raid_geometry_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_get_opcode(block_operation_p, &opcode);
    fbe_payload_block_get_block_edge(block_operation_p, &block_edge_p);
    if(block_edge_p == NULL)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "mirror:  directio optimize failed to get edge for obj: 0x%x\n",
                                     fbe_raid_geometry_get_object_id(raid_geometry_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_block_transport_get_client_index(block_edge_p, &position);
    if(position == FBE_EDGE_INDEX_INVALID)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "mirror:  directio optimize failed to get edge for obj: 0x%x pos: %d\n",
                                     fbe_raid_geometry_get_object_id(raid_geometry_p), position);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
    {
        fbe_spinlock_lock(&mirror_db_p->spinlock);
        if (mirror_db_p->num_reads_outstanding[position] == 0)
        {
            fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "mirror:  directio decrement failed object: 0x%x reads: 0x%x pos: 0x%x\n",
                                         fbe_raid_geometry_get_object_id(raid_geometry_p),
                                         mirror_db_p->num_reads_outstanding[position],
                                         position);
        }
        else
        {
            mirror_db_p->num_reads_outstanding[position]--;
        }
        fbe_spinlock_unlock(&mirror_db_p->spinlock);
    }

   return FBE_STATUS_OK;

}
/********************************************************************
 * end fbe_mirror_optimize_direct_io_finish()
 ********************************************************************/


/********************************
 * end file fbe_raid_read_optimization.c
 ********************************/


