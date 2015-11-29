/***************************************************************************
 * Copyright (C) EMC Corporation 2006-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_xor_epu_tracing.c
 ***************************************************************************
 *
 * @brief
 *   This file contains routines related to tracing for XOR Eval Parity Unit.
 *
 * @notes
 *
 * @author
 *
 *   07/31/06: Rob Foley -- Created.
 *   10/29/08: RDP -- Updated to support new xor tracing
 *
 ***************************************************************************/

/*************************
 * INCLUDE FILES
 *************************/

#include "fbe_xor_private.h"
#include "fbe_xor_epu_tracing.h"
#include "fbe_xor_raid6_util.h"
#include "fbe/fbe_sector_trace_interface.h"

/************************
 * GLOBAL DEFINITIONS.
 ************************/
/* This is the global we use for tracing.
 */
//fbe_xor_epu_tracing_global_t fbe_xor_epu_tracing;

/*!**************************************************************************
 *  fbe_xor_epu_trace_flare_debug
 ***************************************************************************
 * @brief
 *  This routine will trace the EPU information to KTRACE (if enabled) and then
 *  save the instance information.
 *

 * @param err_type - The xor error being traced
 * @param instance - Unique identifier for the error
 *
 *  @return fbe_status_t
 *    
 *
 *  @author
 *    06/19/2006 - Created. RCL
 *    10/29/2008 - Updated to support new XOR tracing facility
 ***************************************************************************/

static fbe_status_t fbe_xor_epu_trace_flare_debug(const fbe_xor_error_type_t  err_type, 
                                                  const fbe_u32_t instance)
{
    /*! @todo This code no longer works due to the fact that it uses globals.  
     *        This code is commented out since it is not critical. 
     */
#if 0
    /* Save loop count before we initialize the structure.
     */
    fbe_u32_t loop_count = fbe_xor_epu_tracing.loop_count;

    /* Now save the instance information if enabled.
     */
    if ( fbe_xor_epu_tracing.b_initialized == FBE_FALSE)
    {
        fbe_xor_epu_trc_init_global();
    }
    fbe_xor_epu_tracing.loop_count = loop_count;

    if (XOR_COND(fbe_xor_epu_tracing.loop_count >= FBE_XOR_EPU_MAX_LOOPS))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "xor epu trace: fbe_xor_epu_tracing.loop_count 0x%x >= "
                              "FBE_XOR_EPU_MAX_LOOPS 0x%x\n",
                              fbe_xor_epu_tracing.loop_count,
                              FBE_XOR_EPU_MAX_LOOPS);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Only display the epu tracing if it is enabled.
     */
#if 0
    if ( FlareTracingIsEnabled(FLARE_XOR_VERBOSE_TRACING) )
    {
        fbe_xor_epu_trc_log_error_info( err_type, instance );
    }
#endif
    fbe_xor_epu_tracing.record[fbe_xor_epu_tracing.loop_count].error_flags[err_type] |=
        (1 << instance);
#endif
    return FBE_STATUS_OK;

} /* end fbe_xor_epu_trace_flare_debug */

/*!**************************************************************************
 *  fbe_xor_epu_trace_setup_trace_positions
 ***************************************************************************
 * @brief
 *  This routine use the width and the already traced position to generate
 *  the arrays of valid positions to trace.
 *

 * @param width - Width of this unit
 * @param already_traced - Bitmask of positions already traced
 * @param err_posmask - Bitmask of new errors
 *   pos_bitmask[],     [I]   - Array of bitmask for each position
 *   trace_bitmask[],  [IO]   - Array of bitmask for positions that should be traced
 *
 *  @return
 * @param new_positions_to_trace - Position bitmask of new positions that should be traced
 *
 *  @author
 *    10/29/2008 - RDP - Created to support new XOR tracing facility
 ***************************************************************************/
static fbe_u16_t fbe_xor_epu_trace_setup_trace_positions(const fbe_u16_t width,
                                                         const fbe_u16_t already_traced,
                                                         const fbe_u16_t err_posmask,
                                                         const fbe_u16_t pos_bitmask[],
                                                         fbe_u16_t trace_bitmask[])
{
    fbe_u16_t new_positions_to_trace = 0;
    fbe_u16_t pos_index;

    /*! @note The parameters for the already traced now come from the stack. 
     *        Each core has its own stack therefore we can modify the value.                                                              .
     */

    /* Simply walk the trace array for each position and either
     * set it to the valid position bitmask or set it to FBE_XOR_INV_DRV_POS.
     */
    for (pos_index = 0; pos_index < width; pos_index++)
    {
        /* We cannot assume that the position maps with the
         * bitmask value.
         */
        if ( (pos_bitmask[pos_index] & err_posmask)    &&
            !(pos_bitmask[pos_index] & already_traced)    )
        {
            /* If it's a new error position and it has not already
             * been traced, add it to the trace array.
             */
            trace_bitmask[pos_index] = pos_bitmask[pos_index];
            new_positions_to_trace |= trace_bitmask[pos_index];
        }
        else
        {
            /* Mark this position as invalid so that we don't
             * trace it.
             */
            trace_bitmask[pos_index] = FBE_XOR_INV_DRV_POS;
        }
    }

    return(new_positions_to_trace);

} /* end fbe_xor_epu_trace_setup_trace_positions */

/*!**************************************************************************
 *  fbe_xor_epu_trace_error
 ***************************************************************************
 * @brief
 *  This routine will first invoke the Flare debug tracing routine which may
 *  generate a ktrace entry to the standard buffer and always save the instance
 *  information for debug purposes. Then we determine if the RAID-6 strip should
 *  or single block should be traced or not. If it should be traced, it invokes
 *  the strip trace routine which will trace the data to the xor trace buffer.
 *

 * @param scratch_p - ptr to the scratch database area.
 * @param eboard_p - error board for marking different types of errors.
 * @param err_posmask - The bitmap of the disks in error.
 * @param err_type - The xor error being traced
 * @param instance - Unique identifier for the error
 * @param function - Function that invoked this routine
 * @param line - Line number in module that invoked this routine
 *
 *  @return fbe_status_t
 *    
 *  @author
 *    06/19/2006 - Created. RCL
 *    10/29/2008 - Updated to support new XOR tracing facility
 *    03/22/2010 - Omer Miranda -- Updated to use the new sector tracing facility.
 ***************************************************************************/

fbe_status_t fbe_xor_epu_trace_error( fbe_xor_scratch_r6_t *const scratch_p,
                                      const fbe_xor_error_t *const eboard_p,
                                      const fbe_u16_t err_posmask,
                                      fbe_xor_error_type_t  err_type, 
                                      const fbe_u32_t instance,
                                      const fbe_bool_t uncorrectable,
                                      const char *const function,
                                      const fbe_u32_t line)

{
    char *error_string = NULL;
    fbe_u16_t trace_pos_bitmask[FBE_XOR_MAX_FRUS];
    fbe_u16_t new_pos_to_trace;
    fbe_status_t status;
    fbe_sector_trace_error_flags_t trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_NONE;
    fbe_sector_trace_error_level_t trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_INFO;

    /* If the caller has asked to log an error that includes an error flag,
     * remove the error flag, and only proceed if there is something to log.
     */
    err_type &= FBE_XOR_ERR_TYPE_MASK;
    if (err_type == FBE_XOR_ERR_TYPE_NONE)
    {
        /* Nothing to log here.
         */
        return FBE_STATUS_OK;
    }
    
    /* Execute the flare debug tracing.
     */
    status = fbe_xor_epu_trace_flare_debug(err_type, instance);
    if (status != FBE_STATUS_OK) 
    {
        return status; 
    }
    
    /* We always trace the error even if we don't trace the
     * data. Generate the appropriate trace identifier and string.
     */
    fbe_xor_sector_history_generate_sector_trace_info(err_type,
                                                      uncorrectable,
                                                      &trace_level,
                                                      &trace_flags,
                                                      (const char **)&error_string);
    
    /* We do not want to trace a sector that was purposefully corrupted
     * by flare. We also don't want to trace of the error if it is not 
     * above the trace level 
     */
    if ( (err_posmask != 0)                  &&
         (fbe_sector_trace_is_enabled(trace_level,trace_flags))    &&
         (!(fbe_xor_epu_trace_is_invalidated(err_posmask, 
                                             trace_flags, 
                                             eboard_p))) ) 
    {
        /* Due to the fact that there are so many locations that may
         * generate this request, save the location in the xor trace buffer.
         */
        if (function != NULL)
        {
            if (fbe_sector_trace_can_trace(1)) {
                fbe_sector_trace_entry(trace_level, trace_flags,
                                       FBE_TRUE, function, line, FBE_TRUE, FBE_FALSE,
                                       "EPU: err_type: 0x%02x err_posmask: 0x%04x lba: 0x%llx \n",
                                       FBE_SECTOR_TRACE_LOCATION_BUFFERS,
                                       err_type, err_posmask, scratch_p->seed);
            } else {
                return FBE_STATUS_OK;
            }
        }

        /* Based on the trace error type we either trace only those sectors
         * in error or we trace the entire strip.
         */
        switch(trace_flags)
        {
            case FBE_SECTOR_TRACE_ERROR_FLAG_COH:
            case FBE_SECTOR_TRACE_ERROR_FLAG_TS:
            case FBE_SECTOR_TRACE_ERROR_FLAG_WS:
            case FBE_SECTOR_TRACE_ERROR_FLAG_SS:
            case FBE_SECTOR_TRACE_ERROR_FLAG_POC:
            case FBE_SECTOR_TRACE_ERROR_FLAG_NPOC:
            case FBE_SECTOR_TRACE_ERROR_FLAG_UCOH:
                /* For all the above errors we want to trace all the
                 * sectors that have yet to have been traced.
                 */
                new_pos_to_trace = fbe_xor_epu_trace_setup_trace_positions(scratch_p->width,
                                                                       scratch_p->traced_pos_bitmap,
                                                                       ~scratch_p->traced_pos_bitmap,
                                                                       scratch_p->pos_bitmask,
                                                                       trace_pos_bitmask);
                break;

            case FBE_SECTOR_TRACE_ERROR_FLAG_LBA:
            default:
                /* For all other cases we only trace the new errors.
                 */
                new_pos_to_trace = fbe_xor_epu_trace_setup_trace_positions(scratch_p->width,
                                                                       scratch_p->traced_pos_bitmap,
                                                                       err_posmask,
                                                                       scratch_p->pos_bitmask,
                                                                       trace_pos_bitmask);
                break;
        }

        /* Don't bother calling the data trace if there are no new positions.
         */
        if (new_pos_to_trace != 0)
        {
            /* We have setup trace_pos_bitmask with the positions that should be traced.
             * Invoke the routine to trace the sectors for those positions.
             */
            status = fbe_xor_sector_history_trace_vector_r6(scratch_p->seed,    /* Lba */
                                                            (const fbe_sector_t * const *)scratch_p->sector_p,    /* Sector vector to save. */
                                                            trace_pos_bitmask,    /* Bitmask of positions in group. */
                                                            scratch_p->width,    /* Width of group. */
                                                            scratch_p->ppos,
                                                            eboard_p->raid_group_object_id,
                                                            eboard_p->raid_group_offset,
                                                            error_string,    /* Error type. */
                                                            trace_level,    /* Trace level */
                                                            trace_flags);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }

            /* Now update the bitmap to mark the additional positions
             * that were traced.
             */
            scratch_p->traced_pos_bitmap |= new_pos_to_trace;
        }
    
    } /* end if this trace id is enabled */

    return FBE_STATUS_OK;

} /* end fbe_xor_epu_trace_error */


/*!**************************************************************************
 *  fbe_xor_epu_trace_is_invalidated
 ***************************************************************************
 * @brief
 *  This routine will check whether the sector is invalidated by flare or not.
 *  For example: sector can be invalidated by usage of ddt, rdt and rderr 
 *  commands by user.
 *
 * @param err_posmask - The bitmap of the disks in error.
 * @param trace_flag - The sector error being traced
 * @param eboard_p - error board of different types of errors.
 *
 *  @return
 *    FBE_TRUE - if the sector is invalidated by flare 
 *    FBE_FALSE - if the sector is not invalidated by flare.
 *
 *  @author
 *    07/03/2009 - Created to determine whether sector is invalidated by flare.
 *    03/24/2010 - Omer Miranda - Updated to use the new sector tracing facility.
 ***************************************************************************/
fbe_bool_t fbe_xor_epu_trace_is_invalidated(const fbe_u16_t err_posmask, 
                                  const fbe_sector_trace_error_flags_t trace_flag, 
                                  const fbe_xor_error_t *const eboard_p) 
{ 
    fbe_bool_t invalidated = FBE_FALSE; 

    /* Allow tracing of invalidated sectors for certain types of errors. 
     */ 
    switch(trace_flag) 
    { 
        /* We are tracing following types of errors as these are not
         * intentionally invalidated by flare.
         */
        case FBE_SECTOR_TRACE_ERROR_FLAG_COH: 
        case FBE_SECTOR_TRACE_ERROR_FLAG_TS: 
        case FBE_SECTOR_TRACE_ERROR_FLAG_WS: 
        case FBE_SECTOR_TRACE_ERROR_FLAG_SS: 
        case FBE_SECTOR_TRACE_ERROR_FLAG_POC: 
        case FBE_SECTOR_TRACE_ERROR_FLAG_NPOC: 
        case FBE_SECTOR_TRACE_ERROR_FLAG_UCOH: 
            break; 

        default: 
            /* We are not tracing sectors which are invalidated 
             * by flare with known patterns 
             * Examples of known patterns are 
             *  1. Sector invalidated using rdt.
             *  2. Sector invalidated using ddt.
             *  3. Sector invalidated using rderr.
             */
            if ((eboard_p->crc_dh_bitmap & err_posmask)       || 
                (eboard_p->crc_raid_bitmap & err_posmask)     || 
                (eboard_p->crc_invalid_bitmap & err_posmask)  || 
                (eboard_p->corrupt_data_bitmap & err_posmask) || 
                (eboard_p->corrupt_crc_bitmap & err_posmask)  || 
                (eboard_p->crc_copy_bitmap & err_posmask)     || 
                (eboard_p->crc_klondike_bitmap & err_posmask) ||
                (eboard_p->crc_pvd_metadata_bitmap & err_posmask) )
            { 
                invalidated = FBE_TRUE; 
            } 
            break; 
    } 

    /* Return if this sector was invalidated by Flare or not. 
     */ 
    return(invalidated); 
} /* end fbe_xor_epu_trace_is_invalidated */


/*************************
 * end file fbe_xor_epu_tracing.c
 *************************/
