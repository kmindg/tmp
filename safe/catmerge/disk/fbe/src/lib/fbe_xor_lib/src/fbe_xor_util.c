/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_xor_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions used by the XOR library.
 *
 * FUNCTIONS:
 *
 * @notes
 *
 * @author
 *  07/13/2009 - Created. Rob Foley
 *
 ***************************************************************************/
 
/************************
 * INCLUDE FILES
 ************************/
#include "fbe/fbe_xor_api.h"
#include "fbe_xor_private.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_library_interface.h"


/********************************
 * static STRING LITERALS
 ********************************/

/*****************************************************
 * static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/

/****************************
 * GLOBAL VARIABLES
 ****************************/

/*************************************
 * EXTERNAL PROTOTYPES
 *************************************/
static fbe_bool_t fbe_xor_break_on_error = FBE_FALSE;

fbe_status_t fbe_xor_set_status(fbe_xor_status_t *status_p,
                                fbe_xor_status_t status_to_add)
{
    if ((fbe_xor_break_on_error == FBE_TRUE)       &&
        (status_to_add != FBE_XOR_STATUS_NO_ERROR)    ) 
    {
        /* Here, instead of panicking, we will display it as a critical
         * error and show input parameters.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                              "unexpected error: input parameters: status_p :0x%p, value: 0x%x",
                              status_p, status_to_add);
    }

    /* The xor_status is a bitmask and therefore can contain multiple error
     * flags.  Therefore OR in the error to be added to the status.
     */
    *status_p |= status_to_add;
    if (status_to_add != FBE_XOR_STATUS_NO_ERROR)
    {
        *status_p &= ~FBE_XOR_STATUS_NO_ERROR;
    }
    return FBE_STATUS_OK;
}

/**************************************************************************
 *                              fbe_xor_get_timestamp()
 **************************************************************************
 *
 *  @brief
 *    fbe_xor_get_timestamp - This routine returns a timestamp value to the caller.
 *
 *    thread_number   - Determines which thread is calling this function 
 *
 *  @return VALUES/ERRORS:
 *    fbe_u16_t  -[O] Random timestamp value
 *
 *  @notes
 *
 *  @author
 *    19-Feb-93       -SP-    Created.
 *    16-Oct-96       -DGM-   Don't allow zero as a timestamp.
 *    28-Jan-98       -SPN-   Moved, reformatted.
 *    10-Jan-99       -JED-   Added sequential factor and changed
 *                            to use clock_value.fraction
 *    18-Dec-00       -MPG-   Moved from fl_common_xor.c
 *    14-APR-08       -VV-    Made changes for DIMS 195197
 **************************************************************************/
fbe_u16_t fbe_xor_get_timestamp(const fbe_u16_t thread_number)
{
    fbe_u16_t stamp;
    fbe_u16_t time_us;


    time_us = (fbe_u16_t)fbe_get_time_in_us();
    stamp = time_us & (~FBE_SECTOR_ALL_TSTAMPS);

    /* We also do not allow the value 0x7FFF to be used. That
       * is the 'Invalid Timestamp' value which is used during
       * Corona operations.  Lastly, we avoid using the value
       * zero since that is what all freshly-bound disks contain.
       * A 'friendlier' value will be substituted for either.
     */
    return ((stamp == FBE_SECTOR_INVALID_TSTAMP) || 
            (stamp == FBE_SECTOR_INITIAL_TSTAMP) || 
            (stamp == FBE_SECTOR_R6_INVALID_TSTAMP)
            ? 0x1234u : stamp);

}                               /* fbe_xor_get_timestamp() */

/*!***************************************************************************
 *          fbe_xor_generate_positions_from_bitmask()
 *****************************************************************************
 *
 * @brief   Generate a bitmask using the array of positions
 *          passed.
 *
 * @param   bitmask - Bitmask of positions that are valid
 * @param   positions_p - Array of positions to populate
 * @param   positions_size - Size of positions array passed  
 *
 * @return  status - Always FBE_STATUS_OK
 *
 * @author
 *  01/04/2010  Ron Proulx  Created
 *
 *****************************************************************************/
fbe_status_t fbe_xor_generate_positions_from_bitmask(fbe_u16_t bitmask,
                                                     fbe_u16_t *positions_p,
                                                     fbe_u32_t positions_size)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       index;

    /* Either add valid or invalid position
     */
    for (index = 0; index < positions_size; index++)
    {
        /* Simply add the bit for each valid position.
         */
        if ((1 << index) & bitmask)
        {
            positions_p[index] = (1 << index);
        }
        else
        {
            /* We need to clear the bitmask to not use this position later. */
            positions_p[index] = 0;
        }
    }

    /* Always return status.
     */
    return(status);
}
/******************************************
 * fbe_xor_generate_positions_from_bitmask()
 *****************************************/

/************************************************************
 *  fbe_xor_get_bitcount()
 ************************************************************
 *
 * @brief   Count number of bits set in a bitmask.
 *
 * @param bitmask - Bitmask to count bits in.
 *
 * @return
 *  number of bits set in bitmask.
 *
 * @author
 *  01/04/2010  Ron Proulx  Created from fbe_raid_get_bitcount
 *
 ************************************************************/

fbe_u32_t fbe_xor_get_bitcount(fbe_u16_t bitmask)
{
    fbe_u32_t bitcount = 0;

    /* Count total bits set in a bitmask.
     */
    while (bitmask > 0)
    {
        /* Clear off the highest bit.
         */
        bitmask &= bitmask - 1;
        bitcount++;
    }

    return bitcount;
}
/*****************************************
 * fbe_xor_get_bitcount()
 *****************************************/

/************************************************************
 *          fbe_xor_get_first_position_from_bitmask()
 ************************************************************
 *
 * @brief   Returns the position of the first bit set in the
 *          bitmask supplied.
 *
 * @param   bitmask - Bitmask to get first bit for.
 * @param   width   - Maximum bit to check for
 *
 * @return  first position - Position of firt bit set.
 *
 * @author
 *  01/06/2010  Ron Proulx  - Created.
 *
 ************************************************************/

fbe_u32_t fbe_xor_get_first_position_from_bitmask(fbe_u16_t bitmask,
                                                  fbe_u32_t width)
{
    fbe_u32_t   position, first_position = FBE_XOR_INV_DRV_POS;

    /* Validate width.
     */
    if (XOR_COND(width > (sizeof(bitmask) * 8)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "width 0x%x > (sizeof(bitmask) * 8\n",
                              width);  
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return position for first bit set.
     */
    for (position = 0; position < width; position++)
    {
        if ((1 << position) & bitmask)
        {
            first_position = position;
            break;
        }
    }

    /* Always return the first position.
     */
    return(first_position);
}
/*****************************************
 * fbe_xor_get_first_position_from_bitmask()
 *****************************************/

/**************************************************************************
 *                              fbe_xor_copy_data()
 **************************************************************************
 *
 *  @brief
 *    xor_clear_memory - Clear the memory from src_p for count bytes
 *
 *    srcptr  - [I]   ptr to first word of source sector data
 *    tgtptr  - [O]   ptr to first word of target sector data
 *
 *  @return VALUES/ERRORS:
 *    NONE
 *
 *  @notes
 *
 *  @author
 *   17-Sep-98  -SPN-   Created. 
 *   18-Dec-00: -MPG-   Copied from fl_csum_util.c
 *
 **************************************************************************/

void fbe_xor_copy_data(const fbe_u32_t * srcptr, fbe_u32_t * tgtptr)
{
    fbe_u16_t passcnt;

    XORLIB_PASSES(passcnt, FBE_WORDS_PER_BLOCK)
    {
        XORLIB_REPEAT(*tgtptr++ = *srcptr++);
    }

    return;
}                               /* fbe_xor_copy_data() */


/**************************************************************************
 * fbe_xor_invalidate_checksum ()
 **************************************************************************
 *
 * @brief
 *   fbe_xor_invalidate_checksum - Compute AND insert a "bad" checksum for the given
 *   sector
 *
 *   data_p (I) Data to checksum
 *
 * @return VALUES/ERRORS:
 *
 * @notes
 *
 * @author
 *   12-Dec-97: SML -- Created
 *   08-Apr-98: SML -- use fbe_sector_t definition for positioning
 **************************************************************************/

void fbe_xor_invalidate_checksum(fbe_sector_t *sector_p)
{
    /* Insert the bad checksum and then invalidate the time stamp.
     */
    xorlib_invalidate_checksum((void *)sector_p);
    sector_p->time_stamp = FBE_SECTOR_INVALID_TSTAMP;

    return;
}                               /* fbe_xor_invalidate_checksum() */

/***************************************************************************
 * fbe_xor_sgld_init_with_offset()
 ***************************************************************************
 *
 * @brief
 *  This function adjust an sgld to account for an offset.
 *  The offset is applied and the sg ptr is updated accordingly.
 *  We also update other fields in the sg descriptor.
 *  The addreses for this sg list are also mapped in if required.
 *
 * @param sgd_p - the sg descriptor to adjust
 * @param sgptr - the sg list to use when applying the offset
 * @param blk_offset - the offset into the current sg list
 * @param blk_size - the size of blocks that the sg list references
 *
 * @return fbe_status_t
 *
 * @notes
 *  This function also modifies:
 *   sgd->total_mapped_bytes
 *   sgd->bytptr
 *   sgd->bytcnt
 *   sgd->sgptr
 *   sgd->map_ptr
 *   sgd->map_cnt
 *
 * @author
 *  23-Jan-02 Rob Foley - Created.
 *  21-Feb-02 PT  - need to include byte_offset in CONTROL addr returned too.
 *
 **************************************************************************/

fbe_status_t fbe_xor_sgld_init_with_offset(fbe_xor_sgl_descriptor_t *sgd_p,
                                        fbe_sg_element_t *sgptr,
                                        fbe_u32_t offset,
                                        fbe_u32_t block_size)
{
    fbe_u32_t sg_bytes_left;
    fbe_status_t status;

    if ((offset == 0) ||
        ((offset * block_size) < (sgptr)->count))
    {
        fbe_xor_sgld_init( sgd_p, sgptr, block_size );
        status = fbe_xor_sgld_inc_n( sgd_p, offset, block_size, &sg_bytes_left);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
    }
    else 
    {
        fbe_s32_t block_offset,
            sg_block_count;

        block_offset = offset;

        while (sg_block_count = sgptr->count / block_size,
               block_offset >= sg_block_count)
        {
            /*
             * This sg_ptr contains fewer blocks than our block_offset.
             * Walk on to the next sg_ptr in the list.
             */

            block_offset -= sg_block_count;
            (sgptr)++;
        }

        /* make sure there are enough blocks for the offset.
         */
        if (XOR_COND(sgptr->count <= (block_offset * block_size)))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "xor: %s : sgptr->count 0x%x <= (block_offset 0x%x * block_size 0x%x)\n",
				  __FUNCTION__,
                                  sgptr->count,
                                  block_offset,
                                  block_size);  
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* create a new set of bytptr and bytcnt
         */
#ifdef XOR_MAPPING_VALIDATION
        sgd_p->total_mapped_bytes = 0;
#endif
        sgd_p->bytcnt = sgptr->count - (block_offset * block_size);
        sgd_p->sgptr = sgptr;

        /* Map in the memory associated with this sg descriptor
         */
        {
            fbe_u32_t byte_offset = block_offset * block_size;
#if XOR_MAP_REQUIRED
            fbe_u32_t sg_entry_remainder = (sgd_p->sgptr)->count - byte_offset;
            if (fbe_xor_is_map_in_required(sgd_p->sgptr))
            {
                /* Map in the memory and set our 
                 * map ptr and bytptr to the mapped in address.
                 * Also increment the mapped bytes so e know how much to map out.
                 */
                sgd_p->bytptr = 
                    sgd_p->map_ptr = 
                       fbe_xor_sgl_map_memory(sgd_p->sgptr, byte_offset, 
                                   sg_entry_remainder);
            }
            else
            {
                /* Null the map ptr, because we will look at this later,
                 * when we try to map out.
                 */
                sgd_p->map_cnt = 0xFFFFFFFF;
                sgd_p->map_ptr = NULL, 
                    sgd_p->bytptr = (sgd_p->sgptr)->address + byte_offset;
            }
#else
            sgd_p->bytptr = (sgd_p->sgptr)->address + byte_offset;
#endif
        }
     
        /* We always set the sg ptr to reference the "next"
         * sg entry, as the current sg entry
         * values are already saved in bytcnt and bytptr.
         */
        if (block_offset >= (fbe_s32_t)(sgptr->count * block_size))
        {
            sgd_p->sgptr = sgptr;
        }
        else
        {
            sgd_p->sgptr = sgptr + 1;
        }
    }
    return FBE_STATUS_OK;
}/* fbe_xor_sgld_init_with_offset() */

/*!**************************************************************
 * fbe_xor_init_sg_and_sector_with_offset()
 ****************************************************************
 * @brief
 *  Initialize an sg ptr and sector ptr when
 *  there is an offset into the sg list.
 *
 * @param sg_pp - Pointer to sg_p to return.
 * @param sector_pp - Pointer to current sector at given offset.
 * @param block_offset - Block offset into the current sg list.
 *
 * @return fbe_status_t 
 *
 * @author
 *  4/24/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_xor_init_sg_and_sector_with_offset(fbe_sg_element_t **sg_pp, 
                                                    fbe_sector_t **sector_pp, 
                                                    fbe_u32_t block_offset)
{
    fbe_s32_t current_block_offset, sg_block_count;
    fbe_sg_element_t *sg_p = *sg_pp;
    fbe_u32_t block_size = FBE_BE_BYTES_PER_BLOCK;
    current_block_offset = block_offset;

    while (sg_block_count = sg_p->count / block_size,
           current_block_offset >= sg_block_count)
    {
        /* This sg_ptr contains fewer blocks than our current_block_offset.
         * Walk on to the next sg_ptr in the list.
         */
        current_block_offset -= sg_block_count;
        sg_p++;
    }

    /* make sure there are enough blocks for the offset.
     */
    if (XOR_COND(sg_p->count <= (current_block_offset * block_size)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "xor: sg_p->count 0x%x <= (current_block_offset 0x%x * block_size 0x%x)\n",
                              sg_p->count, current_block_offset, block_size);  
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *sector_pp = (fbe_sector_t*)sg_p->address + (current_block_offset * block_size);
    *sg_pp = sg_p;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_xor_init_sg_and_sector_with_offset()
 ******************************************/
/***************************************************************************
 * fbe_xor_sgld_null()
 ***************************************************************************
 *
 * @brief
 *  This function initializes an sgld.
 *
 * @param sgd_p - the sg descriptor to init
 *
 * @return
 *  none
 *
 * @author
 *  23-Jan-02 Rob Foley - Created.
 *
 **************************************************************************/

void fbe_xor_sgld_null(fbe_xor_sgl_descriptor_t *sgd_p)
{
    sgd_p->sgptr  = NULL;
    sgd_p->bytptr = NULL;
    sgd_p->bytcnt = 0;
#ifdef XOR_MAPPING_VALIDATION
    sgd_p->total_mapped_bytes = 0;
#endif

#if XOR_MAP_REQUIRED
    sgd_p->map_ptr = NULL;
    sgd_p->map_cnt = 0;
#endif
    return;
} /* fbe_xor_sgld_null */

/***************************************************************************
 * fbe_xor_sgld_map()
 ***************************************************************************
 *
 * @brief
 *  This function maps in the memory associated with an SG descriptor.
 *
 * @param sgd_p - the sg descriptor to map in
 * @param sgptr - the sg list to map in
 *
 * @return
 *  The ptr to the block of memory that we just mapped in.
 *
 * @author
 *  23-Jan-02 Rob Foley - Created.
 *
 **************************************************************************/

fbe_u8_t* fbe_xor_sgld_map(fbe_xor_sgl_descriptor_t *sgd_p,
                                    fbe_sg_element_t *m_sgl)
{
#if XOR_MAP_REQUIRED
   if (fbe_xor_is_map_in_required(m_sgl))
    {
#if defined (XORD_DEBUG) && !defined (UMODE_ENV) && !defined(SIMMODE_ENV)
        /* This sg entry is too big if it is bigger than an EMM block.
         */

        if (XOR_COND((m_sgl)->count > 0xffffffff))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "(m_sgl)->count 0x%x > 0xffffffff\n",
                                  (m_sgl)->count);  
            sgd_p->map_cnt = 0xFFFFFFFF;
            sgd_p->map_ptr = NULL;
            return (m_sgl)->address;
        }

#endif
        /* Assign the map count the remainder of this sg entry.
         */
        sgd_p->map_cnt = (m_sgl)->count;

        /* Map in the memory associated with this sg.
         * Note that we map in the remainder of the sg entry.
         */
        sgd_p->map_ptr = fbe_xor_sgl_map_memory(m_sgl, 0, (m_sgl)->count);
        return sgd_p->map_ptr;
    }
    else
    {
        /* Initialize the map ptr and cnt.
         */
        sgd_p->map_cnt = 0xFFFFFFFF;
        sgd_p->map_ptr = NULL;
        return (m_sgl)->address;    
    }
#else
    return (m_sgl)->address; 
#endif
}
/***************************************************************************
 * fbe_xor_sgld_init()
 ***************************************************************************
 *
 * @brief
 *  This function initializes an SG descriptor.
 *
 * @param sgd_p - the sg descriptor to init
 * @param sgl_p - the sg list to init
 * @param blksize - the sg list to init
 *
 * @return
 *  The ptr to the block of memory that we just mapped in.
 *
 * @author
 *  23-Jan-02 Rob Foley - Created.
 *  04-Aug-03 HAS - Added prefetch code.
 *
 **************************************************************************/
fbe_u32_t fbe_xor_sgld_init(fbe_xor_sgl_descriptor_t *sgd_p,
                               fbe_sg_element_t *sgl_p,
                               fbe_u32_t blksize)
{
    if (sgl_p == NULL)
    {
        sgd_p->bytcnt = 0;
        sgd_p->bytptr = NULL;
        sgd_p->sgptr  = NULL;
        return sgd_p->bytcnt;
    }

#ifdef XOR_MAPPING_VALIDATION
    sgd_p->total_mapped_bytes = 0;
#endif
    sgd_p->bytcnt = (sgl_p)->count;
    sgd_p->bytptr = (sgl_p)->address;
    sgd_p->sgptr  = (sgl_p) + (0 < sgd_p->bytcnt);
    /*
     * Start the CPU prefetching the 1st 128 bytes of data so that
     * it is in the processor cache when we start the XOR.
     */
    fbe_xor_prefetch(((fbe_sector_t*)sgd_p->bytptr)->data_word);
    
    if (XOR_COND(0 != sgd_p->bytcnt % blksize))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "0 != sgd_p->bytcnt 0x%x %% blksize 0x%x\n",
                              sgd_p->bytcnt,
                              blksize);  
        return 0;
    }

    return sgd_p->bytcnt;
} /* fbe_xor_sgld_init */


/***************************************************************************
 * fbe_xor_sgld_unmap()
 ***************************************************************************
 *
 * @brief
 *  This function unmaps the memory associated with an SG list.
 *

 * @param sgd_p - the sg descriptor to unmap
 *
 * @return fbe_status_t
 *
 * @author
 *  23-Jan-02 Rob Foley - Created.
 *
 **************************************************************************/

fbe_status_t fbe_xor_sgld_unmap(fbe_xor_sgl_descriptor_t *sgd_p)
{ 
#if XOR_MAP_REQUIRED
    if (fbe_xor_is_map_out_required(sgd_p->map_ptr))
    {
#if defined (XORD_DEBUG) && !defined (UMODE_ENV) && !defined(SIMMODE_ENV)
        /* This sg entry is too big if it is bigger than an EMM block.
         */
        
        if (XOR_COND(sgd_p->map_cnt > FBE_XOR_MAX_MAP_IN_SIZE))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "sgd_p->map_cnt 0x%x > FBE_XOR_MAX_MAP_IN_SIZE 0x%x\n",
                                  sgd_p->map_cnt,
                                  FBE_XOR_MAX_MAP_IN_SIZE);  
            return FBE_STATUS_GENERIC_FAILURE;
        }

#endif

        /* Unmap the same amount that we mapped in,
         * using the ptr we got back from the map in operation.
         */
        fbe_xor_sgl_unmap_memory(sgd_p->sgptr, sgd_p->map_cnt);

        sgd_p->map_ptr = NULL;
    }
    else
    {
        /* We initialized it to zero, it should still be.
         */
        if (XOR_COND(sgd_p->map_cnt != 0 && sgd_p->map_cnt != 0xFFFFFFFF))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "sgd_p->map_cnt != 0 && sgd_p->map_cnt != 0xFFFFFFFF,  sgd_p->map_cnt = 0x%x\n",
                                  sgd_p->map_cnt);  
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

#endif
    return FBE_STATUS_OK;
} /* fbe_xor_sgld_unmap */

/***************************************************************************
 * fbe_xor_sgld_inc_n()
 ***************************************************************************
 *
 * @brief
 *  This function increments an sg descriptor.
 *
 * @param sgd_p - the sg descriptor to init
 * @param blkcnt - number of blocks to inc by
 * @param blksize - size of each block
 * @param sg_bytes_p - bytes remaining in the current sg entry.
 *
 * @return fbe_status_t
 *  
 *
 * @author
 *  23-Jan-02 Rob Foley - Created.
 *  04-Aug-03 HAS - Added prefetch.
 *
 **************************************************************************/

fbe_status_t fbe_xor_sgld_inc_n(fbe_xor_sgl_descriptor_t *sgd_p,
                                       fbe_u32_t blkcnt,
                                       fbe_u32_t blksize,
                                       fbe_u32_t *sg_bytes_p)
{
    *sg_bytes_p = 0;

    if (XOR_COND(sgd_p->bytcnt < (blkcnt * blksize)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "sgd_p->bytcnt 0x%x < (blkcnt 0x%x * blksize 0x%x\n",
                              sgd_p->bytcnt,
                              blkcnt,
                              blksize);  
        return FBE_STATUS_GENERIC_FAILURE;
    }


    if (XOR_DBG_COND(0 != sgd_p->bytcnt % blksize))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "0 != sgd_p->bytcnt 0x%x %% blksize 0x%x\n",
                              sgd_p->bytcnt,
                              blksize);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    if (sgd_p->bytcnt > (blkcnt * blksize))
    {
        sgd_p->bytptr += (blkcnt * blksize);
        /*
         * Start the CPU prefetching the 1st 128 bytes of data so that
         * it is in the processor cache when we start the XOR.
         */
        fbe_xor_prefetch(((fbe_sector_t*)sgd_p->bytptr)->data_word);
        sgd_p->bytcnt -= (blkcnt * blksize);
    }
    else 
    {
#if XOR_MAP_REQUIRED
        fbe_status_t  status;
        status = fbe_xor_sgld_unmap(sgd_p);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
#endif
        fbe_xor_sgld_init(sgd_p, sgd_p->sgptr, blksize);
    }

    *sg_bytes_p = sgd_p->bytcnt;

    return FBE_STATUS_OK;
} /* fbe_xor_sgld_inc_n() */


fbe_sg_element_t * fbe_sg_element_with_offset_get_next_sg(fbe_sg_element_t * sg_p)
{
    fbe_sg_element_t *next_sg_p = sg_p + 1;
    
    return next_sg_p;
}
fbe_sg_element_t *fbe_sg_element_with_offset_get_current_sg(fbe_sg_element_t *sg_p)
{
    return sg_p;
}

/*!**************************************************************************
 *          fbe_xor_convert_lba_to_pba()
 ***************************************************************************
 *
 * @brief   This method converts a lba (doesn't include raid group offset) to
 *          a pba that includes the raid group offset on the associated virtual
 *          drives.
 *
 * @param   lba - The lba to convert (raid lba's do not include the offset since
 *                the block transport will the offset automatically)
 * @param   raid_group_offset - The starting pba of this raid group on the
 *                              virtual drives
 * 
 * @return fbe_lba_t - The pba that includes the raid group offset
 *  
 **************************************************************************/
fbe_lba_t fbe_xor_convert_lba_to_pba(const fbe_lba_t lba,
                                     const fbe_lba_t raid_group_offset)
{
    /* Simply add the raid group offset to the lba
     */
    return(lba + raid_group_offset);
}
/* end of fbe_xor_convert_lba_to_pba */

/*!***************************************************************************
 *          fbe_xor_invalidate_sectors()
 *****************************************************************************
 *
 * @brief   This routine is invoked to set invalid sectors into the sectors 
 *          indicated.
 *
 * @param   sector_p  - [I]   vector of ptrs to sector contents.
 * @param   bitkey    - [I]   bitkey for stamps/error boards.
 * @param   width     - [I]   number of sectors.
 * @param   seed      - [I]   seed value for checksum.
 * @param   ibitmap   - [I]   invalid sector bitmap.
 * @param   eboard_p  - [I]   eboard to get raid group offset
 *
 * @return  status - FBE Status
 *
 * @todo    Change XORLIB_SECTOR_INVALID_REASON_VERIFY to:
 *          ==>XORLIB_SECTOR_INVALID_REASON_DATA_LOST !!!
 *          When fbe_test is updated to handle read data with blocks
 *          that have been invalidated!!
 *
 * @author
 *  06-Nov-98       -SPN-   Redesigned to use checksum toolkit.
 ***************************************************************************/

fbe_status_t fbe_xor_invalidate_sectors(fbe_sector_t * const sector_p[],
                                const fbe_u16_t bitkey[],
                                const fbe_u16_t width,
                                const fbe_lba_t seed,
                                const fbe_u16_t ibitmap,
                                const fbe_xor_error_t *const eboard_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_sector_t   *isrc_p;
    fbe_u16_t       pos;
    fbe_u16_t       lba_stamp;
    fbe_u16_t       time_stamp;
    fbe_u16_t       write_stamp;

    isrc_p = NULL;

    /* Setup the stamps for a data position (we no longer invalidate parity)
     * lba_stamp is set to our lba stamp since this is a data position.
     * time_stamp is invalid.
     * write_stamp is zero.
     */
    lba_stamp = xorlib_generate_lba_stamp(seed, eboard_p->raid_group_offset);
    time_stamp = FBE_SECTOR_INVALID_TSTAMP;
    write_stamp = 0;

    for (pos = width; 0 < pos--;)
    {
        if (0 != (ibitmap & bitkey[pos]))
        {
            /* This sector couldn't be corrected.
             * Place an invalidated sector on the disk.
             */
            isrc_p = sector_p[pos];

            /*! If the sector is not invalid yet, invalidate it. If it was
             *  already invalidated, then update the information in the
             *  invalid sector. 
             */
            status = fbe_xor_lib_fill_update_invalid_sector(isrc_p,
                                                   seed,
                                                   XORLIB_SECTOR_INVALID_REASON_VERIFY, /* Invalidate reason is `raid verify' */
                                                   XORLIB_SECTOR_INVALID_WHO_RAID          /* Invalidated by is `RAID' */);
            if (status != FBE_STATUS_OK)
            {
                return status;
            }

            /* Set the stamps
             */
            isrc_p->lba_stamp = lba_stamp;
            isrc_p->time_stamp = time_stamp;
            isrc_p->write_stamp = write_stamp;

        }
    } 
    
    /* Always return the execution status
     */                         
    return status;

} /* fbe_xor_invalidate_sectors() */

/****************************************************************
 *                      fbe_xor_vr_increment_errcnt()
 ****************************************************************
 *      @brief
 *        This function will update the verify process error
 *        strructure based on the information in the error board. 
 *

 *        fbe_raid_verify_error_counts_t *vp_eboard_p - [I] Pointer to the vp_eboard
 *                                         the request.
 *        fbe_xor_error_t *eboard_p        - [I] Pointer to the eboard
 *
 *      @return VALUE:
 *         NONE
 *
 *      @author
 *        10/25/99 - Created. JJ
 *        04/28/00 - Moved from Raid component. MPG
 *
 ****************************************************************/
void fbe_xor_vr_increment_errcnt(fbe_raid_verify_error_counts_t * vp_eboard_p,
                                 fbe_xor_error_t * eboard_p,
                                 const fbe_u16_t *parity_positions, fbe_u16_t parity_drives)
{
    fbe_u16_t crc_pos_bitmap;

    /* If we took uncorrectable/correctable crc errors,
     * then only report the errors that are "new".
     */
    if (eboard_p->u_crc_bitmap)
    {
        crc_pos_bitmap = eboard_p->u_crc_bitmap;
        if (eboard_p->media_err_bitmap != 0)
        {
            /* Just account for a single media error. 
             * One media error can cause many individual errors, but we only wish to 
             * account for how many actual media errors were received, not how many blocks 
             * were reconstructed due to the media errors. 
             */
            vp_eboard_p->u_media_count = 1;
            crc_pos_bitmap &= ~eboard_p->media_err_bitmap;
        }

        /* Also do not count any retryable errors.
         */
        crc_pos_bitmap &= ~eboard_p->retry_err_bitmap;

        /* Count the lba_stamp_bitmap properly
         */
        if (crc_pos_bitmap && (eboard_p->u_crc_bitmap & eboard_p->crc_lba_stamp_bitmap)) 
        {
                vp_eboard_p->u_ls_count++;     
                if (!fbe_xor_bitmap_contains_non_lba_stamp_crc_error(crc_pos_bitmap, eboard_p) ) 
                {
                    crc_pos_bitmap &= ~eboard_p->crc_lba_stamp_bitmap;
                }
                
        } 

        if (crc_pos_bitmap)
        {
            vp_eboard_p->u_crc_count++;

            if ((eboard_p->crc_multi_bitmap & crc_pos_bitmap) != 0)
            {
                vp_eboard_p->u_crc_multi_count++;
            }
            if ((eboard_p->crc_single_bitmap & crc_pos_bitmap) != 0)
            {
                vp_eboard_p->u_crc_single_count++;
            }
        }
    }
    if (eboard_p->c_crc_bitmap)
    {
        crc_pos_bitmap = eboard_p->c_crc_bitmap;
        if (eboard_p->media_err_bitmap != 0)
        {
            /* Just account for a single media error. 
             * One media error can cause many individual errors, but we only wish to 
             * account for how many actual media errors were received, not how many blocks 
             * were reconstructed due to the media errors. 
             */
            vp_eboard_p->c_media_count = 1;
            crc_pos_bitmap &= ~eboard_p->media_err_bitmap;
        }
        /* Also do not count any retryable errors.
         */
        crc_pos_bitmap &= ~eboard_p->retry_err_bitmap;

        /* Count the lba_stamp_bitmap properly
         */
        if (crc_pos_bitmap && (eboard_p->c_crc_bitmap & eboard_p->crc_lba_stamp_bitmap)) 
        {
                vp_eboard_p->c_ls_count++;     
                if (!fbe_xor_bitmap_contains_non_lba_stamp_crc_error(crc_pos_bitmap, eboard_p) ) 
                {
                    crc_pos_bitmap &= ~eboard_p->crc_lba_stamp_bitmap;
                }
        } 

        if (crc_pos_bitmap)
        {
            vp_eboard_p->c_crc_count++;

            if ((eboard_p->crc_multi_bitmap & crc_pos_bitmap) != 0)
            {
                vp_eboard_p->c_crc_multi_count++;
            }
            if ((eboard_p->crc_single_bitmap & crc_pos_bitmap) != 0)
            {
                vp_eboard_p->c_crc_single_count++;
            }
        }
    }
    /* This set of 3 coherency errors all count towards
     * the coherency error count in the error report.
     */
    if (eboard_p->u_coh_bitmap ||
        eboard_p->u_poc_coh_bitmap ||
        eboard_p->u_n_poc_coh_bitmap)
    {
        fbe_u16_t u_bitmap =
                eboard_p->u_crc_bitmap |
                eboard_p->u_coh_bitmap |
                eboard_p->u_ts_bitmap |
                eboard_p->u_ws_bitmap |
                eboard_p->u_ss_bitmap |
                eboard_p->u_n_poc_coh_bitmap |
                eboard_p->u_poc_coh_bitmap |
                eboard_p->u_coh_unk_bitmap |        /* skip u_rm_magic_bitmap, it only exists in mirror */
                eboard_p->u_bitmap;

        /* Raid6 has some cases that will set u_coh bits, but only
         * parity drives reconstructed.
         * We don't need to increase u_coh_count in these cases.
         */
        if (parity_drives == 2 && parity_positions != NULL &&
            parity_positions[0] != FBE_XOR_INV_DRV_POS &&
            parity_positions[1] != FBE_XOR_INV_DRV_POS)
        {
            fbe_u16_t parity_bitmap = (1 << parity_positions[0]) | (1 << parity_positions[1]);

            /* Mask out parity drives. Only increase u_coh_count if uncorrectable errors on DATA drives */
            u_bitmap &= ~parity_bitmap;
            if (u_bitmap != 0)
            {
                /* If u_coh error is on DATA drives, this is a real u_coh error */
                vp_eboard_p->u_coh_count++;
            }
            else
            {
                /* u_coh error is on parity drives only. We treat is as c_coh. */
                if (! (eboard_p->c_coh_bitmap ||
                       eboard_p->c_poc_coh_bitmap ||
                       eboard_p->c_n_poc_coh_bitmap ))
                {
                    /* If we don't get c_coh error in this pass, increase c_coh_count.
                     * Else we will increase it later, so don't do it here.
                     */
                    vp_eboard_p->c_coh_count++;
                }
            }
        }
        else
        {
            /* For other raid types(3/5/mirror/stripe), just increase u_coh_count */
            vp_eboard_p->u_coh_count++;
        }
    }
    /* This set of 3 coherency errors all count towards
     * the coherency error count in the error report.
     */
    if (eboard_p->c_coh_bitmap ||
        eboard_p->c_poc_coh_bitmap ||
        eboard_p->c_n_poc_coh_bitmap )
    {
        vp_eboard_p->c_coh_count++;
    }
    if (eboard_p->u_ts_bitmap)
    {
        vp_eboard_p->u_ts_count++;
    }
    if (eboard_p->c_ts_bitmap)
    {
        vp_eboard_p->c_ts_count++;
    }
    if (eboard_p->u_ws_bitmap)
    {
        vp_eboard_p->u_ws_count++;
    }
    if (eboard_p->c_ws_bitmap)
    {
        vp_eboard_p->c_ws_count++;
    }
    if (eboard_p->u_ss_bitmap)
    {
        vp_eboard_p->u_ss_count++;
    }
    if (eboard_p->c_ss_bitmap)
    {
        vp_eboard_p->c_ss_count++;
    }
    if (eboard_p->c_rm_magic_bitmap)
    {
        vp_eboard_p->c_rm_magic_count++;
    }
    if (eboard_p->u_rm_magic_bitmap)
    {
        vp_eboard_p->u_rm_magic_count++;
    }
    if (eboard_p->c_rm_seq_bitmap)
    {
        vp_eboard_p->c_rm_seq_count++;
    }
    if(eboard_p->u_bitmap)
    {
        vp_eboard_p->invalidate_count++;
    }

    return;

}                               /* fbe_xor_vr_increment_errcnt() */

/***************************************************************************
 *              fbe_xor_bitmap_contains_non_lba_stamp_crc_error()
 ***************************************************************************
 *      @brief
 *        Determine if there are non lba stamp crc errors in the crc_pos_bitmap
 *
 *        fbe_u16_t crc_pos_bitmap    - bitmap containing crc errors
 *        fbe_xor_error_t  *eboard_p  - [I]   error board for marking different
 *                                              types of errors.
 *
 *      @return VALUE:
 *        boolean - whether or not there are non lba stamp errors in the bitmap
 *
 *      @author
 *        09-19-2013       Deanna Heng
 ***************************************************************************/
fbe_bool_t fbe_xor_bitmap_contains_non_lba_stamp_crc_error(fbe_u16_t crc_pos_bitmap, fbe_xor_error_t * eboard_p) 
{

    if (crc_pos_bitmap & eboard_p->crc_single_bitmap ||
        crc_pos_bitmap & eboard_p->crc_multi_bitmap ||
        crc_pos_bitmap & eboard_p->crc_bad_bitmap ||
        crc_pos_bitmap & eboard_p->crc_copy_bitmap ||
        crc_pos_bitmap & eboard_p->crc_dh_bitmap ||
        crc_pos_bitmap & eboard_p->crc_klondike_bitmap ||
        crc_pos_bitmap & eboard_p->crc_pvd_metadata_bitmap ||
        crc_pos_bitmap & eboard_p->crc_raid_bitmap ||
        crc_pos_bitmap & eboard_p->crc_unknown_bitmap||
        crc_pos_bitmap & eboard_p->crc_invalid_bitmap ||
        crc_pos_bitmap & eboard_p->media_err_bitmap ||
        crc_pos_bitmap & eboard_p->corrupt_data_bitmap)
    {
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}
/* fbe_xor_bitmap_contains_non_lba_stamp_crc_error() */

/***************************************************************************
 *              fbe_xor_correct_all()
 ***************************************************************************
 *      @brief
 *        This routine is used to mark corrected
 *        all which was marked uncorrected.
 *

 *        fbe_xor_error_t  *eboard_p  - [I]   error board for marking different
 *                                              types of errors.
 *
 *      @return VALUE:
 *        NONE
 *
 *      @notes
 *
 *      @author
 *              06-Nov-98       -SPN-   Redesigned to use checksum toolkit.
 ***************************************************************************/

void fbe_xor_correct_all(fbe_xor_error_t * eboard_p)
{
    eboard_p->c_crc_bitmap |= eboard_p->u_crc_bitmap,
        eboard_p->u_crc_bitmap = 0;
    eboard_p->c_coh_bitmap |= eboard_p->u_coh_bitmap,
        eboard_p->u_coh_bitmap = 0;
    eboard_p->c_ts_bitmap |= eboard_p->u_ts_bitmap,
        eboard_p->u_ts_bitmap = 0;
    eboard_p->c_ws_bitmap |= eboard_p->u_ws_bitmap,
        eboard_p->u_ws_bitmap = 0;
    eboard_p->c_ss_bitmap |= eboard_p->u_ss_bitmap,
        eboard_p->u_ss_bitmap = 0;

    return;
}
/* fbe_xor_correct_all() */


/*!***************************************************************************
 *          fbe_xor_handle_bad_crc_on_write()
 *****************************************************************************
 *
 * @brief   This routine handles the case where the checksum of a write buffer
 *          supplied by the client write contains a bad checksum.  In some
 *          cases this is perfectly valid since an associated data source was
 *          unable to read the data etc.  The current valid cases are:
 *              o RAID lost the data on a pre-read
 *              o Corrupt CRC was requested
 *              o RAID client (usually cache) is writing known lost sectors
 *              o pre-read encountered lost data (on area not being written)
 *          If this is not a known data lost pattern and invalid wasn't allowed
 *          then this routine will return the `bad memory' xor status.
 *
 * @param   data_blk_p - Pointer to data sector to check (possibly re-invalidate)
 * @param   lba - The drive lba (strip) of the sector
 * @param   position_bitmask - Bitmask of position that encountered error
 * @param   option - The xor options flag used to determine if we log certain errors
 * @param   eboard_p - Pointer to XOR error board to mark errors
 * @param   eregions_p - Pointer to array of errors to log into
 * @param   width - Total number of positions for this raid group
 *
 * @return  xor_status - Either FBE_XOR_STATUS_NO_ERROR - `data lost', `corrupt crc' etc pattern
 *                       or     FBE_XOR_STATUS_BAD_MEMORY - Unrecognized data pattern
 * 
 * @note    This method handles a value of NULL for either eboard_p or eregions_p.
 *
 * @author
 *  07/12/2011  Ron Proulx  - Created
 *****************************************************************************/
fbe_xor_status_t fbe_xor_handle_bad_crc_on_write(fbe_sector_t *const data_blk_p,
                                                 const fbe_lba_t lba,
                                                 const fbe_u16_t position_bitmask,
                                                 const fbe_xor_option_t option,
                                                 fbe_xor_error_t *const eboard_p,
                                                 fbe_xor_error_regions_t *const eregions_p,
                                                 const fbe_u16_t width)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_xor_status_t xor_status = FBE_XOR_STATUS_NO_ERROR;
    fbe_bool_t      b_sector_invalidated = FBE_FALSE;
    xorlib_sector_invalid_reason_t invalidated_reason = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;

    /* We only initialize the eboard when a checksum error is detected for
     * performance reasons.
     */
    fbe_xor_lib_eboard_init(eboard_p); 
    
    /*! Check if this is a known invalidated pattern. 
     * 
     *  @note Since client are allowed to send blocks that have been previously
     *        invalidated by RAID (`RAID Verify - Invalidated'), we do not
     *        validate the lba since the original invalidation could have
     *        occurred on a different volume. 
     */
    b_sector_invalidated = xorlib_is_sector_invalidated(data_blk_p,
                                                        &invalidated_reason,
                                                        lba,
                                                        FBE_FALSE /* Don't validate lba */);
    
    /* For debug purposes we 0 the checksum if the XOR_COND changes the status.
     */        
    if ((b_sector_invalidated == FBE_TRUE)          &&
        XOR_COND(b_sector_invalidated == FBE_FALSE)    )
    {
        fbe_u16_t   actual_crc = 0;

        /* Blast the checksum to 0 so that we recognize that we changed the
         * `is invalidated' from True to False.
         */
        b_sector_invalidated = FBE_FALSE;
        ((fbe_sector_t *)data_blk_p)->crc = 0;
        actual_crc = xorlib_cook_csum(xorlib_calc_csum(data_blk_p->data_word), 0); 
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "xor: %s block crc 0x%x != computed crc 0x%x\n",
                              __FUNCTION__,  data_blk_p->crc, actual_crc);

        /* Fall thru and report the `Bad Memory' failure
         */
    }

    /* If data and checksum didn't match any of the invalidated patterns
     * report the `Bad Memory' error. 
     */
    if (b_sector_invalidated == FBE_FALSE)
    {
        /* There are cases where there will not be an eboard or eregions array.
         */
        if (eboard_p != NULL)
        {
            eboard_p->u_crc_bitmap |= position_bitmask;
            eboard_p->crc_bad_bitmap |= position_bitmask;

            /* Save the sector for debugging purposes.
             */
            status = fbe_xor_report_error_trace_sector(lba,  /* Lba */
                                                  position_bitmask, /* Bitmask of position in group. */
                                                  0, /* Don't shoot the drive since error detected on write */
                                                  eboard_p->raid_group_object_id,
                                                  eboard_p->raid_group_offset, 
                                                  data_blk_p, /* Sector to save. */
                                                  "Bad CRC on Write", /* Error type. */ 
                                                  FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR, /* Tracing level */
                                                  FBE_SECTOR_TRACE_ERROR_FLAG_BAD_CRC  /* Error Type */);

            /*! @note It is up to the client to process the `bad memory' error
             *        and take any action necessary.
             */

        } /* end if eboard_p isn't NULL */

        /*! @note Set the xor_status to flag that the write request failed. 
         *        We will not write out any data for this write request.
         */
        xor_status = FBE_XOR_STATUS_BAD_MEMORY;

    }
    else if (invalidated_reason == XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA)
    {
        /* We log corrupt data in the eboard
         */
        if (eboard_p != NULL)
        {
            eboard_p->corrupt_data_bitmap |= position_bitmask;
        }

        /* For testing purposes we allow users to inject `Corrupt Data' error.
         * `Corrupt data' purposefully corrupts the crc of a data position but
         * unlike `data lost' we want to generate an error region etc.
         */
        if (option & FBE_XOR_OPTION_CORRUPT_DATA)
        {
            /* There are cases where we don't have the eregions array.
             */
            if ((eregions_p != NULL)        &&
                (width <= FBE_XOR_MAX_FRUS)    )
            {
                /* Generate an error region.  We supply an invalid parity 
                 * position since this method should only be invoked for a data
                 * position.
                 */
                status  = fbe_xor_create_error_region(FBE_XOR_ERR_TYPE_CORRUPT_DATA_INJECTED, 
                                                      position_bitmask, 
                                                      lba, 
                                                      eregions_p, 
                                                      width, 
                                                      FBE_XOR_INV_DRV_POS);
            }
        } /* end if `report corrupt data' regions */

        xor_status = FBE_XOR_STATUS_NO_ERROR;
    }
    else if (invalidated_reason == XORLIB_SECTOR_INVALID_REASON_DATA_LOST) 
    {
        /* Even though no error will be logged we set the eboard bits
         * for debug purposes.
         */
        if (eboard_p != NULL)
        {
            eboard_p->crc_invalid_bitmap |= position_bitmask;
        }

        /* For testing purposes we allow users to inject `Corrupt crc' error.
         * `Corrupt crc' purposefully corrupts the crc of a data position.
         */
        if (option & FBE_XOR_OPTION_CORRUPT_CRC)
        {
            /* There are cases where we don't have the eregions array.
             */
            if ((eregions_p != NULL)        &&
                (width <= FBE_XOR_MAX_FRUS)    )
            {
                /* Generate an error region.  We supply an invalid parity 
                 * position since this method should only be invoked for a data
                 * position.
                 */
                status  = fbe_xor_create_error_region(FBE_XOR_ERR_TYPE_CORRUPT_CRC_INJECTED, 
                                                      position_bitmask, 
                                                      lba, 
                                                      eregions_p, 
                                                      width, 
                                                      FBE_XOR_INV_DRV_POS);
            }
        } /* end if `report corrupt crc' regions */

        xor_status = FBE_XOR_STATUS_NO_ERROR;
    }
    else if (invalidated_reason == XORLIB_SECTOR_INVALID_REASON_VERIFY) 
    {
        /* We allow clients to write data that was previously invalidated by
         * RAID.  Log the fact in the eboard for debug purposes.
         */
        if (eboard_p != NULL)
        {
            eboard_p->crc_raid_bitmap |= position_bitmask;
        }

        xor_status = FBE_XOR_STATUS_NO_ERROR;
    }
    else
    {
        /* For all other cases trace an informational message and return success.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_INFO,
                              "xor: rg obj: 0x%x pos bm: 0x%x lba: 0x%llx reason: %d",
                              (eboard_p != NULL) ? eboard_p->raid_group_object_id : FBE_OBJECT_ID_INVALID,
                              position_bitmask, lba, invalidated_reason);

        xor_status = FBE_XOR_STATUS_NO_ERROR;
    }

    /* Return the xor_status
     */
    return(xor_status);
}
/******************************************
 * end fbe_xor_handle_bad_crc_on_write()
 ******************************************/

/*!***************************************************************************
 *          fbe_xor_handle_bad_crc_on_read()
 *****************************************************************************
 *
 * @brief   This routine handles the case where the checksum of a read from
 *          disk contains a checksum error.  This method determines the reason
 *          for the checksum error (for instance if it was purposefully
 *          invalidated due to `data lost') and sets the status accordingly.
 * 
 * @param   data_blk_p - Pointer to data sector to check (possibly invalidate)
 * @param   lba - The drive lba (strip) of the sector
 * @param   position_bitmask - Bitmask of position that encountered error
 * @param   option - The xor options flag used to determine if we log certain errors
 * @param   eboard_p - Pointer to XOR error board to mark errors
 * @param   eregions_p - Pointer to array of errors to log into
 * @param   width - Total number of positions for this raid group
 *
 * 
 * @return  xor_status - Either FBE_XOR_STATUS_NO_ERROR - `data lost' pattern
 *                       or     FBE_XOR_STATUS_CHECKSUM_ERROR - `other invalidated' pattern
 * 
 * @note    This method handles a value of NULL for either eboard_p or eregions_p.
 *
 * @author
 *  07/19/2011  Ron Proulx  - Created
 ***************************************************************************/
fbe_xor_status_t fbe_xor_handle_bad_crc_on_read(const fbe_sector_t *const data_blk_p,
                                                const fbe_lba_t lba,
                                                const fbe_u16_t position_bitmask,
                                                const fbe_xor_option_t option,
                                                fbe_xor_error_t *const eboard_p,
                                                fbe_xor_error_regions_t *const eregions_p,
                                                const fbe_u16_t width)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_xor_status_t xor_status = FBE_XOR_STATUS_CHECKSUM_ERROR;
    fbe_bool_t      b_sector_invalidated = FBE_FALSE;
    xorlib_sector_invalid_reason_t invalidated_reason = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;

    if (eboard_p != NULL)
    {
        /* We only initialize the eboard when a checksum error is detected for
         * performance reasons.
         */
        fbe_xor_lib_eboard_init(eboard_p); 
    }

    /* For debug purposes we 0 the checksum if the XOR_COND changes the status.
     */ 
    /*! Check if this is a known invalidated pattern. 
     * 
     *  @note To be a `known' pattern the drive lba must match
     *        the lba (strip) being written.
     */
    b_sector_invalidated = xorlib_is_sector_invalidated(data_blk_p,
                                                        &invalidated_reason,
                                                        lba,
                                                        FBE_TRUE);

    /* For debug purposes we 0 the checksum if the XOR_COND changes the status.
     */  
    if ((b_sector_invalidated == FBE_TRUE)          &&
        XOR_COND(b_sector_invalidated == FBE_FALSE)    )
    {          
        fbe_u16_t   actual_crc = 0;

        /* Blast the checksum to 0 so that we recognize that we changed the
         * `is invalidated' from True to False.
         */
        b_sector_invalidated = FBE_FALSE;
        ((fbe_sector_t *)data_blk_p)->crc = 0;
        actual_crc = xorlib_cook_csum(xorlib_calc_csum(data_blk_p->data_word), 0); 
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "xor: %s block crc 0x%x != computed crc 0x%x\n",
                              __FUNCTION__,  data_blk_p->crc, actual_crc);

        /* Fall thru and report the `Bad Memory' failure
         */
    }

    /* If data and checksum didn't match any of the invalidated patterns
     * report an checksum error.
     */
    if (b_sector_invalidated == FBE_FALSE)
    {
        /* Detected unexpected checksum error on a read, flag the error.
         * We don't report specific information since the caller must 
         * determine the exact region. 
         */
        xor_status = FBE_XOR_STATUS_CHECKSUM_ERROR;
    }
    else if (invalidated_reason == XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA)
    {
        /* We log corrupt data in the eboard
         */
        if (eboard_p != NULL)
        {
            eboard_p->corrupt_data_bitmap |= position_bitmask;
        }

        /* For testing purposes we allow users to inject `Corrupt Data' error.
         * `Corrupt data' purposefully corrupts the crc of a data position.
         * When a `Corrupt data' sector is encountered on a read request we
         * treat it as a checksum error.  This allows a host to test RAIDs
         * `sector reconstruction' mechanism.  Therefore report a checksum error
         * unless `option corrupt data' is set.  This indicates that this is a
         * pre-read for a `Corrupt Data' option.
         */
        if (option & FBE_XOR_OPTION_CORRUPT_DATA)
        {
            /* This is a pre-read for a `Corrupt Data' operation.  Don't report
             * and error.
             */
            xor_status = FBE_XOR_STATUS_NO_ERROR;
        }
        else
        {
            /* Else this is a read operation, report the error so that the
             * `corrupted' sectors are logged and corrected.
             */
            xor_status = FBE_XOR_STATUS_CHECKSUM_ERROR;
        }
    }
    else if (invalidated_reason == XORLIB_SECTOR_INVALID_REASON_DATA_LOST) 
    {
        /* We log `data lost - invalidated' in the eboard.
         */
        if (eboard_p != NULL)
        {
            eboard_p->crc_invalid_bitmap |= position_bitmask;
        }

        /* For testing purposes we allow users to inject `Corrupt crc' error.
         * `Corrupt crc' purposefully corrupts the crc of a data position.
         */
        if (option & FBE_XOR_OPTION_CORRUPT_CRC)
        {
            /* There are cases where we don't have the eregions array.
             */
            if ((eregions_p != NULL)        &&
                (width <= FBE_XOR_MAX_FRUS)    )
            {
                /* Generate an error region.  We supply an invalid parity 
                 * position since this method should only be invoked for a data
                 * position.
                 */
                status  = fbe_xor_create_error_region(FBE_XOR_ERR_TYPE_CORRUPT_CRC_INJECTED, 
                                                      position_bitmask, 
                                                      lba, 
                                                      eregions_p, 
                                                      width, 
                                                      FBE_XOR_INV_DRV_POS);
            }
        } /* end if `report corrupt crc' regions */

        /* In MCR we always report invalidated sectors.  All the read data will
         * be transferred.
         */
        xor_status = FBE_XOR_STATUS_CHECKSUM_ERROR;
    }
    else
    {
        /* All other cases are a checksum error
         */
        xor_status = FBE_XOR_STATUS_CHECKSUM_ERROR;
    }

    /* Return the xor_status
     */
    return(xor_status);
}
/******************************************
 * end fbe_xor_handle_bad_crc_on_read()
 ******************************************/

/*!***************************************************************************
 *          fbe_xor_validate_data_if_enabled()
 *****************************************************************************
 *
 * @brief   If enabled, this method validates that if the sector has been
 *          invalidated the checksum better be invalid.
 *
 * @param   sector_p - Pointer to sector to validate
 * @param   seed - Lba for sector
 * @param   option - xor option flags  
 *
 * @return  status - Typically FBE_STATUS_OK
 *                   Otherwise the data is incorrect
 *
 * @author
 *  08/03/2010  Ron Proulx  Created
 *
 *****************************************************************************/
fbe_status_t fbe_xor_validate_data_if_enabled(const fbe_sector_t *const sector_p,
                                              const fbe_lba_t seed,
                                              const fbe_xor_option_t option)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Only validate the data if the option is enabled
     */
    if (option & FBE_XOR_OPTION_VALIDATE_DATA)
    {
        status = fbe_xor_lib_validate_data(sector_p, seed, option);
    }
    return status;
}
/******************************************
 * end fbe_xor_validate_data_if_enabled()
 ******************************************/


/****************************************************************
 * fbe_xor_correct_all_one_pos()
 ****************************************************************
 * @brief
 *  This function will correct one position worth of errors
 *  within the xor eboard.
 *

 * @param eboard_p - Error board for marking different
 *                  types of errors.
 * @param bitkey - In other words, this is the bit position to 
 *                  modify within eboard_p.
 *
 * @return
 *  None.
 *
 * @author
 *  07/05/06 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_xor_correct_all_one_pos( fbe_xor_error_t * const eboard_p,
                                    const fbe_u16_t bitkey )
{
    
    /* For every eboard entry, check to see if the
     * uncorrectable exists for this bitkey.
     * If so, the uncorrectable bitkey will be cleared and
     * the same bitkey will be set in the correctable bitmask.
     */
    fbe_xor_eboard_correct_one( &eboard_p->u_crc_bitmap,
                            &eboard_p->c_crc_bitmap,
                            bitkey);
    
    fbe_xor_eboard_correct_one( &eboard_p->u_coh_bitmap,
                            &eboard_p->c_coh_bitmap,
                            bitkey);
    
    fbe_xor_eboard_correct_one( &eboard_p->u_ts_bitmap,
                            &eboard_p->c_ts_bitmap,
                            bitkey);
    
    fbe_xor_eboard_correct_one( &eboard_p->u_ws_bitmap,
                            &eboard_p->c_ws_bitmap,
                            bitkey);
    
    fbe_xor_eboard_correct_one( &eboard_p->u_ss_bitmap,
                            &eboard_p->c_ss_bitmap,
                            bitkey);
    
    fbe_xor_eboard_correct_one( &eboard_p->u_poc_coh_bitmap,
                            &eboard_p->c_poc_coh_bitmap,
                            bitkey);
    
    fbe_xor_eboard_correct_one( &eboard_p->u_n_poc_coh_bitmap,
                            &eboard_p->c_n_poc_coh_bitmap,
                            bitkey);
    
    fbe_xor_eboard_correct_one( &eboard_p->u_coh_unk_bitmap,
                            &eboard_p->c_coh_unk_bitmap,
                            bitkey);
    return;
}
/* fbe_xor_correct_all_one_pos() */

/****************************************************************
 * fbe_xor_correct_all_non_crc_one_pos()
 ****************************************************************
 * @brief
 *  This function will correct one position worth of stamp errors
 *  within the xor eboard.
 *

 * @param eboard_p - Error board for marking different
 *                  types of errors.
 * @param bitkey - In other words, this is the bit position to 
 *                  modify within eboard_p.
 *
 * @return
 *  None.
 *
 * @author
 *  11/07/06 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_xor_correct_all_non_crc_one_pos( fbe_xor_error_t * const eboard_p,
                                      const fbe_u16_t bitkey )
{
    
    /* For every stamp entry, check to see if the
     * uncorrectable exists for this bitkey.
     * If so, the uncorrectable bitkey will be cleared and
     * the same bitkey will be set in the correctable bitmask.
     */
    
    fbe_xor_eboard_correct_one( &eboard_p->u_ts_bitmap,
                            &eboard_p->c_ts_bitmap,
                            bitkey);
    
    fbe_xor_eboard_correct_one( &eboard_p->u_ws_bitmap,
                            &eboard_p->c_ws_bitmap,
                            bitkey);
    
    fbe_xor_eboard_correct_one( &eboard_p->u_ss_bitmap,
                            &eboard_p->c_ss_bitmap,
                            bitkey);
    return;
}
/* fbe_xor_correct_all_non_crc_one_pos() */
/*****************************************
 * End of fbe_xor_util.c
 *****************************************/ 
