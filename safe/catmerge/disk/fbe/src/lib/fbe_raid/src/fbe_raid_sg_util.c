/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/***************************************************************************
 * $Id: rg_sg_util.c,v 1.1.8.1 2000/10/13 14:25:49 fladm Exp $
 ***************************************************************************
 *
 * @brief
 *       This file contains common functions for sg_list manipulation.
 *
 * @author
 *  09/22/1999 Created.  Rob Foley.
 *
 ***************************************************************************/

/*************************
 **    INCLUDE FILES 
 *************************/


#include "fbe/fbe_types.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_library.h"

/********************************
 *      static MACROS
 ********************************/

/***************************************************************************
 * fbe_raid_sg_count_sg_blocks()
 ***************************************************************************
 *
 * @brief
 *  Determine the number of blocks and sg elements in a sg list.
 *      

 * @param sg_ptr - the type of sg to check for this SUB_IOTS.
 * @param sg_count - pointer to count of sg elements in list
 * @param block_in_sg_list_p - pointer to number of blocks in an SG list
 * @return VALUE:
 *  fbe_status_t
 *
 * @author
 *  04/15/00: Rob Foley -- Created. 
 *
 **************************************************************************/

fbe_status_t fbe_raid_sg_count_sg_blocks(fbe_sg_element_t * sg_ptr, 
                                                                    fbe_u32_t * sg_count, 
                                                                    fbe_block_count_t *block_in_sg_list_p )
{
    fbe_u32_t blocks = 0;
    fbe_status_t status = FBE_STATUS_OK;
    while (sg_ptr->count != 0)
    {
        if(RAID_COND((sg_ptr->count % FBE_BE_BYTES_PER_BLOCK) != 0) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s status: sg_ptr->count ox%x %% FBE_BE_BYTES_PER_BLOCK) != 0\n",
                                   __FUNCTION__, sg_ptr->count);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        blocks += (sg_ptr->count / FBE_BE_BYTES_PER_BLOCK);
        *sg_count += 1;
        sg_ptr++;
    }
    *block_in_sg_list_p = blocks;
    return status;
}  /* fbe_raid_sg_count_sg_blocks */

/***************************************************************************
 * fbe_raid_sg_count_blocks_limit()
 ***************************************************************************
 *
 * @brief
 *  Determine the number of blocks and sg elements in a sg list and stop
 *  when we reach a pre-determined limit.
 *  
 * @param sg_ptr - the type of sg to check for this SUB_IOTS.
 * @param sg_count - pointer to count of sg elements in list
 * @param limit - Number of blocks to stop after.
 * @param block_in_sg_list_p - pointer to number of blocks in an SG list
 * @return VALUE:
 *  fbe_status_t
 *
 * @author
 *  8/7/2012 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_raid_sg_count_blocks_limit(fbe_sg_element_t * sg_ptr, 
                                            fbe_u32_t * sg_count, 
                                            fbe_block_count_t limit,
                                            fbe_block_count_t *block_in_sg_list_p )
{
    fbe_u32_t blocks = 0;
    fbe_status_t status = FBE_STATUS_OK;
    while ((sg_ptr->count != 0) && (blocks < limit))
    {
        if(RAID_COND((sg_ptr->count % FBE_BE_BYTES_PER_BLOCK) != 0) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s status: sg_ptr->count ox%x %% FBE_BE_BYTES_PER_BLOCK) != 0\n",
                                   __FUNCTION__, sg_ptr->count);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        blocks += (sg_ptr->count / FBE_BE_BYTES_PER_BLOCK);
        *sg_count += 1;
        sg_ptr++;
    }
    *block_in_sg_list_p = blocks;
    return status;
}  /* fbe_raid_sg_count_blocks_limit */

/***************************************************************************
 * fbe_raid_sg_get_sg_total()
 ***************************************************************************
 *
 * @brief
 *
 *  Determine the number of this sg type we need for this sg_id_list
 *      

 * @param sg_type - the type of sg to check for this SUB_IOTS.
 * @param sg_id_p - pointer to sg id list
 *
 * @return VALUE:
 *
 *  This function returns the total SGs of this type that this
 *  SUB_IOTS should allocate. 
 *
 * @author
 *  06/30/99:      Rob Foley -- Created. 
 *
 **************************************************************************/

fbe_u32_t fbe_raid_sg_get_sg_total(fbe_memory_id_t * sg_id_list[], fbe_u32_t sg_type)
{
    fbe_u32_t total_sgs = 0;
    fbe_memory_id_t *sg_id_p;

    sg_id_p = sg_id_list[sg_type];

    /* Traverse the list of sg_ids and count the # of entries.
     */
    while ((sg_id_p) != NULL)
    {
        total_sgs++;
        sg_id_p = (fbe_memory_id_t *) * sg_id_p;
    }

    return(total_sgs);
}  /* fbe_raid_sg_get_sg_total() */



/***************************************************************************
 *      fbe_raid_sg_determine_sg_uniform()
 ***************************************************************************
 *
 * @brief
 *
 *  This function takes a single SG_ID addresses where an SG ids should be 
 *  placed, a block count associated with this sg id, and an array position.
 *
 *  We determine the size and type of SG needed for this block count.
 *  Then we place the SG_ID address at the head of the list assocated with
 *  this SG type.  The head of these lists are passed in as the sg_id_list.
 *      

 *
 *  block_count[] - [I],  a block count.
 *
 *  sg_id_addr[] - [I], the address of an SG_ID.
 *
 *  array_position - [I], The relative position in the array.
 *
 *  buffer_size - [I], the size of buffers we will be using.
 *
 *  remaining_count - [I], the blocks remaining in the previous buffer.
 *
 *  sg_id_list[] - [IO], a vector of pointers which keeps track of the
 *                       head of the linked list of sg_id addresses.
 *                       Each vector entry corresponds
 *                       to a different type of SG.
 *
 *  sg_count_vec[] - [IO], a vector of total sg elements needed for
 *                         each position.
 *
 * @return VALUE:
 *  fbe_status_t
 *
 * @author
 *  06/30/99:      Rob Foley -- Created. 
 *
 **************************************************************************/

fbe_status_t fbe_raid_sg_determine_sg_uniform(fbe_block_count_t block_count,
                                           fbe_memory_id_t * sg_id_addr,
                                           fbe_u32_t array_position,
                                           fbe_u32_t buffer_size,
                                           fbe_block_count_t *remaining_count_p,
                                           fbe_memory_id_t * sg_id_list[],
                                           fbe_u32_t sg_count_vec[])
{
    fbe_u32_t sg_size;
    fbe_status_t status = FBE_STATUS_OK;
    if(RAID_COND( (buffer_size <= 0) ||
                  (block_count <= 0) ||
                  (array_position >= FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH) ) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "%s status: buffer_size 0x%x <= 0 OR block_count 0x%llx <= 0"
                               "array_position 0x%x >= FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH\n",
                               __FUNCTION__, buffer_size,
			       (unsigned long long)block_count, array_position);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine the size of the sg needed for this count
     * entry.
     */
    sg_size = 0;
    *remaining_count_p = fbe_raid_sg_count_uniform_blocks(block_count,
                                                       buffer_size,
                                                       *remaining_count_p,
                                                       &sg_size);
    if (sg_count_vec != NULL)
    {
        sg_count_vec[array_position] += sg_size;
    }

    /* Store the address vector entry on the tail of the
     * list for this sg size.
     */
    if (sg_id_addr != NULL)
    {
        status = fbe_raid_memory_push_sgid_address(sg_size, sg_id_addr, sg_id_list);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s failed to push sgid address: status = 0x%x, "
                                   "sg_size = 0x%x, sg_id_addr = 0x%p, sg_id_list = 0x%p\n",
                                   __FUNCTION__, status, sg_size, sg_id_addr, sg_id_list);
            return status;
        }
    }

    return status;

}  /* end fbe_raid_sg_determine_sg_uniform */

/***************************************************************************
 *      rg_setup_fruts_addr_vec()
 ***************************************************************************
 *
 *      @brief
 *        For each fruts in a chain, copy the:
 *                               1) address of the sg_id within the fruts
 *                into:
 *                               1) vector of address counts
 *
 *        The index of the address vector and block vector is the
 *        fruts->position.
 *

 *
 *       address_vec - [IO], the vector of addresses of sg_ids to
 *                                              copy from the address of a sg_id in each fruts.
 *
 *       fruts_ptr - [I], The start of a chain of fruts structures to copy
 *                                        the block counts from.
 *
 *      @return VALUE:
 *              void
 *
 *      @notes
 *
 *      @author
 *        06/30/99:      Rob Foley -- Created. 
 *
 **************************************************************************/

void rg_setup_fruts_addr_vec(fbe_memory_id_t * address_vec[], fbe_raid_fruts_t * fruts_ptr)
{
    fbe_raid_fruts_t *current_fruts_ptr = fruts_ptr;

    fbe_memory_id_t **current_addr_vec_ptr = address_vec;

#ifdef RG_DEBUG
    fbe_u32_t i;
    for (i = 0; i < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH; i++)
    {
        address_vec[i] = NULL;
    }
#endif

    while (current_fruts_ptr != NULL)
    {
        *(current_addr_vec_ptr + current_fruts_ptr->position) = &(current_fruts_ptr)->sg_id;
        current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr);
    }

}

/***************************************************************************
 *      fbe_raid_sg_scatter_fed_to_bed()
 ***************************************************************************
 *
 *   @brief
 *     This function is employed for two slightly-different purposes,
 *     depending upon the memory model used.
 *
 *     Dual-Memory:    this function calculates the additional S/G elements
 *                     required for S/G lists which provide DMA_SOURCE
 *                     information to the sequencer.
 *
 *     Single-memory:  this function calculates the additional S/G elements
 *                     required for S/G lists which are used to write data
 *                     to the drives.
 *

 *     lda              [I] -   logical address of first block.
 *
 *     blks_to_scatter  [I] -   number of blocks to be scattered.
 *
 *     data_pos         [I] -   position of first block in vector.
 *
 *     data_disks       [I] -   number of data disks in the array.
 *
 *     blks_per_element [I] -   size of stripe element (blocks).
 *
 *     blks_per_buffer  [I] -   size of buffer (blocks).
 *
 *     blks_remaining   [I] -   number of unused blocks remaining from
 *                              preceding buffer.
 *
 *     sg_total         [IO] -  vector holding number of S/G list elements
 *                              for each data disk.
 *
 *  @return VALUE:
 *     Unused blocks remaining from final buffer allocation.
 *
 *  @notes
 *     The caller has already initialized the element count vector with
 *     element counts representing the pre-read data, or to zero.
 *
 *     See also rg_scatter_cache_to_bed().
 *
 *  @author
 *     25-Nov-96       SPN:    Created.
 *     09-Jul-99       Rob Foley:    Renamed to fbe_raid_sg_scatter_fed_to_bed
 *
 **************************************************************************/

fbe_block_count_t  fbe_raid_sg_scatter_fed_to_bed(fbe_lba_t lda,
                                         fbe_block_count_t blks_to_scatter,
                                         fbe_u16_t data_pos,
                                         fbe_u16_t data_disks,
                                         fbe_lba_t blks_per_element,
                                         fbe_block_count_t blks_per_buffer,
                                         fbe_block_count_t blks_remaining,
                                         fbe_u32_t sg_total[])
{
    fbe_block_count_t blks_for_disk;    /* num blocks to write into stripe element */

    blks_for_disk = FBE_MIN(blks_to_scatter, blks_per_element -
                                       (lda % blks_per_element));

    while (blks_for_disk)
    {
        /*
         * Process blocks within current element.
         */

        blks_remaining = fbe_raid_sg_count_uniform_blocks(blks_for_disk,
                                                    blks_per_buffer,
                                                    blks_remaining,
                                                    &sg_total[data_pos]);

        /*
         * Process the next element written.
         */

        data_pos = (data_pos + 1) % data_disks;

        blks_to_scatter -= blks_for_disk,
        blks_for_disk = (fbe_u32_t)FBE_MIN(blks_to_scatter, blks_per_element);
    }

    return blks_remaining;
}

/***************************************************************************
 *      fbe_raid_scatter_cache_to_bed()
 ***************************************************************************
 *
 * @brief
 *  This function is employed for two slightly-different purposes,
 *  depending upon the memory model used.
 *
 *  This function calculates the additional S/G elements
 *  required for S/G lists which are used to write data
 *  to the drives.
 *

 * @param lda - logical address of first block.
 * @param blks_to_scatter - number of blocks to be scattered.
 * @param data_pos - position of first block in vector.
 * @param data_disks - number of data disks in the array.
 *  blks_per_element,[I] - number of sectors per stripe element.
 * @param src_sgd_ptr - S/G descriptor providing current location
 *                         in cache source S/G list.
 * @param sg_total - vector holding number of S/G list elements
 *                         for each data disk.
 *
 * @return
 *  fbe_status_t.
 *
 * @notes
 *  The caller has already initialized the element count vector (with
 *  element counts representing the pre-read data).
 *
 *  See also fbe_raid_sg_scatter_fed_to_bed().
 *
 * @author
 *  25-Nov-96 SPN - Created as bem_scatter_fed_to_bed().
 *  08-Jul-99 Rob Foley - Renamed and modified to return sg_totals.
 *
 **************************************************************************/

fbe_status_t fbe_raid_scatter_cache_to_bed(fbe_lba_t lda,
                                           fbe_block_count_t blks_to_scatter,
                                           fbe_u16_t data_pos,
                                           fbe_u16_t data_disks,
                                           fbe_lba_t blks_per_element,
                                           fbe_sg_element_with_offset_t * src_sgd_ptr,
                                           fbe_u32_t sg_total[])
{
    fbe_block_count_t blks_remaining;    /* unprocessed blocks in current src S/G element */
    fbe_u16_t bytes_per_blk;    /* size of block (bytes) */
    fbe_block_count_t blks_for_disk;    /* num blocks to write into stripe element */
    fbe_status_t status = FBE_STATUS_OK;

    fbe_raid_adjust_sg_desc(src_sgd_ptr);
    bytes_per_blk = FBE_RAID_SECTOR_SIZE(src_sg_ptr->address);
    blks_remaining = (src_sgd_ptr->sg_element->count - src_sgd_ptr->offset) / bytes_per_blk;

    blks_for_disk = (fbe_u32_t)FBE_MIN(blks_to_scatter, blks_per_element -
                                       (lda % blks_per_element));

    while (0 < blks_for_disk)
    {
        /*
         * Process blocks within current element.
         */

        status  = fbe_raid_sg_count_nonuniform_blocks(blks_for_disk,
                                                      src_sgd_ptr,
                                                      bytes_per_blk,
                                                      &blks_remaining,
                                                      &sg_total[data_pos]);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s failed to count non-uniform blocks: status = 0x%x, blks_remaining 0x%llx"
                                   "src_sgd_ptr 0x%p src_sgd_ptr 0x%p\n",
                                   __FUNCTION__, 
                                   status, (unsigned long long)blks_remaining,
				   src_sgd_ptr, src_sgd_ptr);
            return status;
        }

        /*
         * Process the next element written.
         */

        data_pos = (data_pos + 1) % data_disks;

        blks_to_scatter -= blks_for_disk,
        blks_for_disk = (fbe_u32_t)FBE_MIN(blks_to_scatter, blks_per_element);
    }

    return status;
}
/**************************************
 * end fbe_raid_scatter_cache_to_bed()
 **************************************/
/***************************************************************************
 * fbe_raid_scatter_sg_with_memory()
 ***************************************************************************
 * @brief
 *  This function distributes memory into S/G lists for each disk,
 *  observing stripe-element boundaries; its employment differs
 *  slightly based upon the memory architecture:
 *
 *  This function calculates the additional S/G elements
 *  required for S/G lists which are used to write data
 *  to the drives.
 *
 *  @param lda -   logical address of first block.
 *  @param blks_to_scatter -   number of blocks to be scattered.
 *  @param data_pos -   position of first block in vector.
 *  @param data_disks -   number of data disks in the array.
 *  @param blks_per_element -   number of sectors per stripe element.
 *  @param memory_info_p - Pointer to meory request information
 *  @param dest_sgl_ptr -  vector holding ptrs to current S/G list
 *                           elements for each data disk.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/4/2010 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_raid_scatter_sg_with_memory(fbe_lba_t lda,
                                             fbe_u32_t blks_to_scatter,
                                             fbe_u16_t data_pos,
                                             fbe_u16_t data_disks,
                                             fbe_lba_t blks_per_element,
                                             fbe_raid_memory_info_t *memory_info_p,
                                             fbe_sg_element_t * dest_sgl_ptrv[])
{
    fbe_u16_t bytes_per_blk;    /* num bytes per block in source S/G list */
    fbe_u16_t blks_for_disk;    /* num blocks to access within stripe element */
    bytes_per_blk = FBE_BE_BYTES_PER_BLOCK;

    /*
     * We treat the first element somewhat differently, since access
     * probably does not begin precisely on the stripe element boundary.
     */

    blks_for_disk = (fbe_u32_t)FBE_MIN(blks_to_scatter, blks_per_element -
                                       (lda % blks_per_element));

    while (0 < blks_for_disk)
    {
        fbe_u32_t bytes_to_scatter;
        fbe_u16_t num_sg_elements;

        /* Process blocks within current element.
         */
        bytes_to_scatter = blks_for_disk * bytes_per_blk;
        num_sg_elements = fbe_raid_sg_populate_with_memory(memory_info_p, 
                                                           dest_sgl_ptrv[data_pos], 
                                                           bytes_to_scatter);
        if (num_sg_elements == 0)
        {
             fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                     __FUNCTION__,memory_info_p);
                return FBE_STATUS_GENERIC_FAILURE;
        }
        dest_sgl_ptrv[data_pos] += num_sg_elements;
        /* Process the next stripe element accessed.
         */
        data_pos = (data_pos + 1) % data_disks;

        blks_to_scatter -= blks_for_disk,
        blks_for_disk = (fbe_u32_t)FBE_MIN(blks_to_scatter, blks_per_element);
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_scatter_sg_with_memory()
 **************************************/
/***************************************************************************
 *      fbe_raid_scatter_sg_to_bed()
 ***************************************************************************
 * @brief
 *  This function distributes memory into S/G lists for each disk,
 *  observing stripe-element boundaries; its employment differs
 *  slightly based upon the memory architecture:
 *
 *  This function calculates the additional S/G elements
 *  required for S/G lists which are used to write data
 *  to the drives.
 *

 *  lda              [I] -   logical address of first block.
 *  blks_to_scatter  [I] -   number of blocks to be scattered.
 *
 *  data_pos         [I] -   position of first block in vector.
 *
 *  data_disks       [I] -   number of data disks in the array.
 *
 *  blks_per_element [I] -   number of sectors per stripe element.
 *
 *  src_sgd_ptr      [IO] -  S/G descriptor providing current location
 *                           in source S/G list.
 *
 *  dest_sgl_ptr     [IO] -  vector holding ptrs to current S/G list
 *                           elements for each data disk.
 *
 * @return VALUE:
 *  fbe_status_t.
 *
 * @author
 *  25-Nov-96       SPN:    Created.
 *  09-Jul-99       Rob Foley:    Renamed to fbe_raid_scatter_sg_to_bed
 *
 **************************************************************************/

fbe_status_t fbe_raid_scatter_sg_to_bed(fbe_lba_t lda,
                                 fbe_u32_t blks_to_scatter,
                                 fbe_u16_t data_pos,
                                 fbe_u16_t data_disks,
                                 fbe_lba_t blks_per_element,
                                 fbe_sg_element_with_offset_t * src_sgd_ptr,
                                 fbe_sg_element_t * dest_sgl_ptrv[])
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t bytes_per_blk;    /* num bytes per block in source S/G list */
    fbe_u16_t blks_for_disk;    /* num blocks to access within stripe element */

    if (RAID_COND( (src_sgd_ptr->get_next_sg_fn == NULL) ||
                  (src_sgd_ptr->get_next_sg_fn != fbe_sg_element_with_offset_get_current_sg) &&
                  (src_sgd_ptr->get_next_sg_fn != fbe_sg_element_with_offset_get_next_sg)) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid_scatter_sg_2_bed fgot unexpect err: get_next_sg_fn %p is NULL\n",src_sgd_ptr);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    bytes_per_blk = FBE_RAID_SECTOR_SIZE(src_sgd_ptr->sg_element->address);

    /*
     * We treat the first element somewhat differently, since access
     * probably does not begin precisely on the stripe element boundary.
     */

    blks_for_disk = (fbe_u32_t)FBE_MIN(blks_to_scatter, blks_per_element -
                                       (lda % blks_per_element));

    while (0 < blks_for_disk)
    {
        fbe_u32_t bytes_to_scatter;
        fbe_u16_t num_sg_elements;

        /*
         * Process blocks within current element.
         */

        bytes_to_scatter = blks_for_disk * bytes_per_blk;
        status = fbe_raid_sg_clip_sg_list(src_sgd_ptr,
                                          dest_sgl_ptrv[data_pos],
                                          bytes_to_scatter,
                                          &num_sg_elements);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid_scatter_sg_2_bed fail clip sg: st:0x%x off: 0x%x sg: %p bytes_scatter: 0x%x\n",
                                   status, src_sgd_ptr->offset, src_sgd_ptr->sg_element, bytes_to_scatter);
            return status;
        }

        dest_sgl_ptrv[data_pos] += num_sg_elements;

        /*
         * Process the next stripe element accessed.
         */

        data_pos = (data_pos + 1) % data_disks;

        blks_to_scatter -= blks_for_disk,
        blks_for_disk = (fbe_u32_t)FBE_MIN(blks_to_scatter, blks_per_element);
    }

    return status;
}  /* End fbe_raid_scatter_sg_to_bed */


/***************************************************************************
 *      fbe_raid_sg_count_uniform_blocks()
 ***************************************************************************
 *
 *  @brief
 *      This function determines the number of S/G elements required
 *      to reference the desired number of blocks, when allocated in
 *      buffers of the uniform size indicated.
 *

 *      blks_to_add     [I] -   number of blocks to be buffered.
 *
 *      blks_per_buffer [I] -   number of blocks each buffer holds.
 *
 *      blks_remaining  [I] -   number of blocks remaining to be
 *                              allocated from preceding buffer.
 *
 *      sg_totalp       [IO] -  ptr to running total of S/G elements.
 *
 *
 *  @return VALUE:
 *      Unused blocks remaining within the final unit allocation.
 *
 *  @notes
 *      See also fbe_raid_sg_count_nonuniform_blocks().
 *
 *  @author
 *      25-Nov-96       SPN:    Created.
 *      09-Jul-99       Rob Foley:    Renamed to fbe_raid_sg_count_uniform_blocks
 *
 **************************************************************************/

fbe_block_count_t fbe_raid_sg_count_uniform_blocks(fbe_block_count_t blks_to_add,
                                           fbe_block_count_t blks_per_buffer,
                                           fbe_block_count_t blks_remaining,
                                           fbe_u32_t * sg_totalp)
{
    if (0 < blks_to_add)
    {
        fbe_u16_t added_sgcnt;

        /*
         * As always, we've been passed the number of blocks still
         * unused from the last buffer to be allocated; if there are
         * unused sectors, we must consume them first.
         */

        added_sgcnt = ((blks_remaining > 0) ? 1 : 0);

        if (blks_to_add <= blks_remaining)
        {
            /*
             * We can allocate all of the blocks required from the
             * unused portion of the previously-allocated buffer.
             */

            blks_remaining -= blks_to_add;
        }

        else
        {
            /*
             * Some portion of the blocks required may be allocated
             * from the unused portion of the previously-allocated
             * buffer.
             */

            blks_to_add -= blks_remaining;

            /*
             * Calculate how much buffer will remain unused in the
             * last buffer to be allocated.
             */

            blks_remaining = (0 == (blks_to_add % blks_per_buffer)) ? 0 :
                             (blks_per_buffer - (blks_to_add % blks_per_buffer));

            /*
             * Update the S/G count by the number of buffers we'll be
             * allocating.
             */

            if(((blks_to_add + blks_remaining) / blks_per_buffer) > FBE_U32_MAX)
            {
                /* we do not expect the sg count to cross 32bit limit here
                 */
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                           "raid: %s sg count crossing 32bit limit 0x%llx \n",
                            __FUNCTION__, (unsigned long long)((blks_to_add + blks_remaining) / blks_per_buffer));
                return FBE_STATUS_GENERIC_FAILURE;
            }

            added_sgcnt += (fbe_u16_t)((blks_to_add + blks_remaining) / blks_per_buffer);
        }

        *sg_totalp += added_sgcnt;
    }

    return blks_remaining;

}  /* fbe_raid_sg_count_uniform_blocks */

/***************************************************************************
 *      fbe_raid_sg_count_nonuniform_blocks()
 ***************************************************************************
 *
 * @brief
 *  This function determines the number of S/G elements required
 *  to reference the desired number of blocks, when the source is
 *  provided by another S/G list.
 *

 * @param blks_to_count - number of blocks desired.
 * @param src_sg_ptr - pointer to address of current S/G element
 *                         (from cache S/G list).
 *
 * @param bytes_per_blk - size of a block (bytes).
 *
 * @param blks_remaining_p - number of uncounted blocks from current
 *                        cache S/G element.
 *
 * @param sg_total_ptr - ptr to running total S/G elements.
 *
 *
 * @return
 *  fbe_status_t.
 *
 * @notes
 *  It will return failure if more blocks are requested that source
 *  S/G list can provide!
 *  See also bem_sg_count_uniform_blocks().
 *
 * @author
 *  25-Nov-96 SPN - Created as bem_count_nonuniform_blocks.
 *  09-Jul-99 Rob Foley - Renamed to fbe_raid_sg_count_nonuniform_blocks
 *
 **************************************************************************/

fbe_status_t fbe_raid_sg_count_nonuniform_blocks(fbe_block_count_t blks_to_count,
                                              fbe_sg_element_with_offset_t * src_sgd_ptr,
                                              fbe_u16_t bytes_per_blk,
                                              fbe_block_count_t *blks_remaining_ptr,
                                              fbe_u32_t * sg_total_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    if(RAID_COND(0 >= *blks_remaining_ptr) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s errored:  0 >= *blks_remaining_ptr 0x%llx",
                               __FUNCTION__,
			       (unsigned long long)(*blks_remaining_ptr));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (0 < blks_to_count)
    {
        while (0 < *blks_remaining_ptr)
        {
            /*
             * Anticipate assignment of some portion
             * these blocks to another S/G element.
             */

            (*sg_total_ptr)++;

            if (*blks_remaining_ptr > blks_to_count)
            {
                /*
                 * We can scatter all of the blocks required
                 * from the (as yet) unscattered blocks in the
                 * current S/G element.
                 */

                *blks_remaining_ptr -= blks_to_count;
                break;
            }

            /*
             * All unscattered blocks in the current
             * S/G element are scattered. Advance to
             * the next S/G element. (All of its blocks
             * are unscattered.)
             */

            blks_to_count -= *blks_remaining_ptr;
            src_sgd_ptr->sg_element = src_sgd_ptr->get_next_sg_fn(src_sgd_ptr->sg_element);
            *blks_remaining_ptr = src_sgd_ptr->sg_element->count / bytes_per_blk;

            if (0 >= blks_to_count)
            {
                break;
            }
        }
    }

    return status;

}  /* fbe_raid_sg_count_nonuniform_blocks */


/***************************************************************************
 * fbe_raid_sg_clip_sg_list()
 ***************************************************************************
 * @brief
 *  This function is responsible for initializing the elements in
 *  one S/G list from the contents of another.
 *

 *  src_sgd_ptr     [IO] -  ptr to the S/G list descriptor which
 *                          provides source memory locations.
 *
 *  dest_sgl_ptr    [IO] -  ptr to start of S/G list to be initialized.
 *
 *  dest_bytecnt    [I] -   number of bytes to be referenced.
 *  sg_element_init_p [IO]  The number of S/G elements initialized (excluding terminator).
 *
 * @return VALUE: fbe_status_t
 *
 * @notes
 *  This function PANICs if the source S/G list does not contain the
 *  number of bytes required.
 *
 * @author
 *  25-Nov-96  SPN:    Created.
 *  09-Jul-99  Rob Foley:    Renamed to fbe_raid_sg_clip_sg_list
 *  12-Mar-03  PT:     Added phys_addr component to BM SGL
 *
 **************************************************************************/

fbe_status_t fbe_raid_sg_clip_sg_list(fbe_sg_element_with_offset_t * src_sgd_ptr,
                                   fbe_sg_element_t * dest_sgl_ptr,
                                   fbe_u32_t dest_bytecnt,
                                   fbe_u16_t *sg_element_init_ptr)
{
    fbe_u16_t sg_total = 0;    /* count of S/G elements copied */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_adjust_sg_desc(src_sgd_ptr);

    if (0 >= src_sgd_ptr->sg_element->count)
    {
        /*
         * Reached terminator of source S/G list,
         * so there is nothing to copy.
         */

        if(RAID_COND(0 != dest_bytecnt) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s errored: status: 0 != dest_bytecnt 0x%x\n",
                                   __FUNCTION__, dest_bytecnt);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    else if (src_sgd_ptr->sg_element == dest_sgl_ptr)
    {
        /*
         * We have to approach things a bit differently
         * if the elements of the list are coincident.
         * Adjust the first S/G element to eliminate
         * what we don't want included.
         */

        dest_sgl_ptr->count -= src_sgd_ptr->offset;

        /* Update the correct component of the SG list */ 
        if (fbe_raid_virtual_addr_available(dest_sgl_ptr))
        {
            dest_sgl_ptr->address += src_sgd_ptr->offset;
        }
        else
        {
            fbe_u64_t phys_addr_local =0; 
            fbe_raid_sg_get_physical_address(dest_sgl_ptr, phys_addr_local);
            phys_addr_local += (fbe_u64_t)src_sgd_ptr->offset;
            fbe_raid_sg_set_physical_address(dest_sgl_ptr, phys_addr_local);
        }

        /*
         * Now we can go directly to the last node in
         * the list.
         */

        src_sgd_ptr->offset = dest_bytecnt;

        fbe_raid_adjust_sg_desc(src_sgd_ptr);

        /*
         * If we've not reached the terminator, truncate
         * the final node and advance to where the terminator
         * will be placed.
         */

        if (0 >= src_sgd_ptr->sg_element->count)
        {
            if(RAID_COND(0 != src_sgd_ptr->offset) )
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "raid: %s errored: src_sgd_ptr->offset 0x%x\n",
                                       __FUNCTION__, src_sgd_ptr->offset);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        else
        {
            if(RAID_COND(src_sgd_ptr->offset >= 
                       (fbe_s32_t) src_sgd_ptr->sg_element->count))
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "raid: %s errored: src_sgd_ptr->offset 0x%x != src_sgd_ptr->sg_element->count 0x%x\n",
                                       __FUNCTION__, src_sgd_ptr->offset, src_sgd_ptr->sg_element->count);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        if (0 < src_sgd_ptr->offset)
        {
            src_sgd_ptr->sg_element->count = src_sgd_ptr->offset;
            src_sgd_ptr->offset = 0;
            src_sgd_ptr->sg_element = src_sgd_ptr->get_next_sg_fn(src_sgd_ptr->sg_element);
        }

        /*
         * Calculate the number of nodes in the new list.
         */

        sg_total = (fbe_u16_t)((ULONG_PTR)(src_sgd_ptr->sg_element - dest_sgl_ptr));
        dest_sgl_ptr = src_sgd_ptr->sg_element;
    }

    else
    {
        fbe_sg_element_t src_sg_element;    /* element from source S/G list */
        fbe_raid_sg_declare_physical_address(fbe_u64_t phys_addr_local = 0);

        /*
         * Create a 'copy' of the current source element from the
         * source S/G list. This element will account for the offset
         * given by the descriptor.
         */

        /* Update the correct component of the SG list */ 
        if (fbe_raid_virtual_addr_available(src_sgd_ptr->sg_element))
        {
            src_sg_element.address = 
            src_sgd_ptr->sg_element->address + src_sgd_ptr->offset;
        }
        else
        {
            fbe_raid_sg_set_physical_address(&src_sg_element,
                                        src_sgd_ptr->sg_element->phys_addr + src_sgd_ptr->offset);
            src_sg_element.address = src_sgd_ptr->sg_element->address;
        }

        src_sg_element.count = 
        src_sgd_ptr->sg_element->count - src_sgd_ptr->offset;

        /*
         * Assign portions of the memory provided by the source S/G list
         * to the entries in the destination S/G list; assign only the
         * number of bytes specified.
         */


        while (dest_bytecnt > 0)
        {
            if (RAID_COND((src_sg_element.count <= 0) ||
            /* look for blatantly bad address values (null or token) */
                          (src_sg_element.address == 0) ||
                          (src_sg_element.address == (PVOID) -1)) )
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "raid: %s errored: src_sg_element.count 0x%x <= 0 or"
                                       "src_sg_element.address 0x%p == 0 or -1\n",
                                       __FUNCTION__, src_sg_element.count, src_sg_element.address);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            // For Release-13 Buffer memory IS in virtual space so this
            // check is inappropriate. 
            // assert (HEMI_IS_PSEUDO_ADR_DATA(SG_ADDRESS(&src_sg_element)));
            sg_total++;

            dest_sgl_ptr->address = src_sg_element.address;


            fbe_raid_sg_get_physical_address(&src_sg_element, phys_addr_local);
            fbe_raid_sg_set_physical_address(dest_sgl_ptr, phys_addr_local);            

            if (dest_bytecnt < src_sg_element.count)
            {
                /*
                 * This source S/G element contains more than the
                 * number of bytes necessary to fulfill this
                 * request. Update the source descriptor offset
                 * and leave the loop.
                 */

                (dest_sgl_ptr++)->count = dest_bytecnt,
                src_sgd_ptr->offset += dest_bytecnt;

                break;
            }

            /*
             * This source S/G element contains too few or exactly
             * enough bytes necessary to fulfill this request, so
             * all will be contributed to the request. Advance the
             * source S/G descriptor to reference the next source
             * element.
             */

#ifndef XOR_ASIC_ARCH           
            // assert (HEMI_IS_PSEUDO_ADR_DATA(dest_sgl_ptr->address));
#endif
            (dest_sgl_ptr++)->count = src_sg_element.count,
            dest_bytecnt -= src_sg_element.count;

            src_sgd_ptr->sg_element = src_sgd_ptr->get_next_sg_fn(src_sgd_ptr->sg_element);
            src_sgd_ptr->offset = 0;
            src_sg_element.address = src_sgd_ptr->sg_element->address;
            src_sg_element.count = src_sgd_ptr->sg_element->count;
            fbe_raid_sg_get_physical_address(src_sgd_ptr->sg_element,phys_addr_local);
            fbe_raid_sg_set_physical_address(&src_sg_element,phys_addr_local);

        }
    }

    /*
     * Terminate the S/G list properly.
     */

    dest_sgl_ptr->address = 0,
    dest_sgl_ptr->count = 0;
    *sg_element_init_ptr = sg_total;
    return status;
}  /* end fbe_raid_sg_clip_sg_list */
/***************************************************************************
 * fbe_raid_sg_populate_with_memory()
 ***************************************************************************
 * @brief
 *  This function is responsible for initializing the elements in
 *  one S/G list from the contents of another.
 *
 * @param   memory_info_p - Pointer to memory request information 
 * @param   dest_sgl_ptr    -  ptr to start of S/G list to be initialized.
 * @param   dest_bytecnt    -  number of bytes to be referenced.
 *
 * @return VALUE:
 *  The number of S/G elements initialized (excluding terminator).
 *
 * @note
 *  This function PANICs if the source S/G list does not contain the
 *  number of bytes required.
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_u16_t fbe_raid_sg_populate_with_memory(fbe_raid_memory_info_t *memory_info_p,
                                           fbe_sg_element_t * dest_sgl_ptr,
                                           fbe_u32_t dest_bytecnt)
{
    fbe_u32_t   bytes_remaining_in_page = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);
    fbe_u16_t   sg_total = 0;    /* count of S/G elements copied */
    fbe_u32_t   bytes_allocated;
    fbe_u32_t   checked_dest_bytecnt;


    /* We will never expect destination byte count to exceed the 32bit limit
      */
    if(dest_bytecnt > FBE_U32_MAX)
   {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid: dest_bytecnt is beyond 32bit limit 0x%x \n",
                             dest_bytecnt);
        return 0;
    }
    checked_dest_bytecnt = dest_bytecnt;

    /* Check if we have exhausted our buffer space
     */
    if (fbe_raid_memory_is_buffer_space_remaining(memory_info_p, 
                                                  checked_dest_bytecnt) == FBE_FALSE)
    {
        /* No more source memory so there is nothing to copy.
         */
        if (dest_bytecnt == 0)
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "raid: dest_bytecnt is 0 with no buffers remaining \n");
            return 0;
        }
        else
        {
            /* Else we needed to populate an sg but there was not buffer
             * available.  This is an error.
             */
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "raid: dest_bytecnt: %d with no buffers remaining \n",
                                 checked_dest_bytecnt);
        }
    }
    else
    {
        fbe_u32_t   bytes_to_allocate;
        fbe_u8_t   *prev_buffer_address = NULL;

        /* Log memory request information if enabled
         */
        fbe_raid_memory_info_log_request(memory_info_p, FBE_RAID_MEMORY_INFO_TRACE_PARAMS_INFO,
                                         "raid: dest_bytecnt = %d\n",
                                         checked_dest_bytecnt);

        /* Assign portions of the memory provided by the source to the entries in the
         * destination S/G list; assign only the number of bytes specified. 
         */
        while (checked_dest_bytecnt > 0)
        {
            sg_total++;

            /* Allocate either the amount leftover or thet amount asked for, whichever 
             * is smaller.  This allows us to not waste any memory. 
             */
            if (bytes_remaining_in_page >= FBE_BE_BYTES_PER_BLOCK)
            {
                /* Allocate what we need from the remainder in the current page.
                 * Make sure it is a multiple of our block size. 
                 */
                bytes_remaining_in_page -= bytes_remaining_in_page % FBE_BE_BYTES_PER_BLOCK;
                bytes_to_allocate = FBE_MIN(bytes_remaining_in_page, checked_dest_bytecnt);
                fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, bytes_remaining_in_page);
            }
            else
            {
                /* Otherwise try to allocate all that we can from this page.
                 */
                bytes_to_allocate = checked_dest_bytecnt;
            }
            dest_sgl_ptr->address = fbe_raid_memory_allocate_next_buffer(memory_info_p,
                                                                         bytes_to_allocate,
                                                                         &bytes_allocated);

            /* If this is null then we ran out of memory.  Also validate that
             * we didn't re-use the same buffer space.
             */
            if (dest_sgl_ptr->address == NULL)
            {
                fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                     "raid: dest sgl %p unable to allocate buffers\n",
                                     dest_sgl_ptr);
                return 0;
            }
            else if (dest_sgl_ptr->address == prev_buffer_address)
            {
                fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                     "raid: Attempt to re-use buffer: 0x%p for sg index: %d and %d\n",
                                     dest_sgl_ptr->address, (sg_total - 2), (sg_total - 1));
                return 0;
            }

            /* Update for next buffer
             */
            prev_buffer_address = dest_sgl_ptr->address;
            dest_sgl_ptr->count = bytes_allocated;
            checked_dest_bytecnt -= bytes_allocated;
            dest_sgl_ptr++;
            bytes_remaining_in_page = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);
        }
    }

    /* Terminate the S/G list properly.
     */
    dest_sgl_ptr->address = 0,
    dest_sgl_ptr->count = 0;

    return sg_total;
}  /* end fbe_raid_sg_populate_with_memory */
#if 0
/***************************************************************************
 * rg_clip_buffer_list()
 ***************************************************************************
 * @brief
 *  This function is responsible for initializing the elements in
 *  one S/G list from a linked list of buffers
 *

 *  src_sgd_ptr     [IO] -  ptr to the S/G list descriptor which
 *                          provides source memory locations. 
 *                          The sg_element points to the current
 *                          memory vector we are on. Offset points
 *                          to the offset within that vector we are on.
 *
 *  dest_sgl_ptr    [IO] -  ptr to start of S/G list to be initialized.
 *
 *  dest_bytecnt    [I] -   number of bytes to be referenced.
 *
 * @return VALUE:
 *  The number of S/G elements initialized (excluding terminator).
 *
 * @notes
 *  This function PANICs if the source S/G list does not contain the
 *  number of bytes required.
 *
 * @author
 *  22-Mar-04  KLS:    Created.
 **************************************************************************/

fbe_u16_t rg_clip_buffer_list(fbe_sg_element_with_offset_t * src_sgd_ptr,
                                     fbe_sg_element_t * dest_sgl_ptr,
                                     fbe_u32_t dest_bytecnt)
{
    fbe_u16_t sg_total = 0;    /* count of S/G elements copied */
    fbe_sg_element_t src_sg_element;    /* element from source S/G list */
    BM_MEMORY_VECTOR  *curr_buffer_id;

    if (src_sgd_ptr == NULL)
    {
        /*
         * Reached terminator of source S/G list,
         * so there is nothing to copy.
         */

        if(RAID_COND(0 != dest_bytecnt) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s errored: 0 != dest_bytecnt 0x%x \n",
                                   __FUNCTION__, dest_bytecnt);
        }

        return 0;
    }

    curr_buffer_id = (BM_MEMORY_VECTOR *)src_sgd_ptr->sg_element;

    /*
     * Create a 'copy' of the current source element from the
     * source buffer vector. This element will account for the offset
     * given by the descriptor.
     */
    fbe_raid_memory_id_get_memory_ptr
    src_sg_element.address = ((fbe_u8_t *) fbe_raid_memory_id_get_memory_ptr((fbe_memory_id_t)curr_buffer_id)) +
                             src_sgd_ptr->offset,

                             src_sg_element.count = bm_extract_memory_size(curr_buffer_id) - src_sgd_ptr->offset;

    /*
     * Assign portions of the memory provided by the source buffer list
     * to the entries in the destination S/G list; assign only the
     * number of bytes specified.
     */

    while (dest_bytecnt > 0)
    {
        if(RAID_COND( (src_sg_element.count <= 0)||
                      (src_sg_element.address == 0) ||
                      (src_sg_element.address == (fbe_u8_t *) - 1) ) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s errored: src_sg_element.count 0x%x <= 0 or",
                                   "src_sg_element.address 0x%x == 0 Or -1\n",
                                   __FUNCTION__, src_sg_element.count, src_sg_element.address);
            return sg_total;
        }

        //assert (HEMI_IS_PSEUDO_ADR_DATA(src_sg_element.address));
        sg_total++;

        dest_sgl_ptr->address = src_sg_element.address;

        if (dest_bytecnt < src_sg_element.count)
        {
            /*
             * This source S/G element contains more than the
             * number of bytes necessary to fulfill this
             * request. Update the source descriptor offset
             * and leave the loop.
             */

            (dest_sgl_ptr++)->count = dest_bytecnt,
            src_sgd_ptr->offset += dest_bytecnt;

            break;
        }

        /*
         * This source buffer contains too few or exactly
         * enough bytes necessary to fulfill this request, so
         * all will be contributed to the request. Advance the
         * buffer to reference the next source
         * element.
         */

#ifndef XOR_ASIC_ARCH           
        //assert (HEMI_IS_PSEUDO_ADR_DATA(dest_sgl_ptr->address));
#endif
        (dest_sgl_ptr++)->count = src_sg_element.count,
        dest_bytecnt -= src_sg_element.count;

        src_sgd_ptr->sg_element = (fbe_sg_element_t *)bm_get_next_ptr(curr_buffer_id);
        curr_buffer_id = (BM_MEMORY_VECTOR *) src_sgd_ptr->sg_element;

        if (dest_bytecnt > 0)
        {
            /* There is more left for this request. 
             * Therefore there must be another buffer in the src. 
             * Move the next buffer
             */
            if (RAID_COND(curr_buffer_id == NULL) )
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "raid: %s errored: curr_buffer_id is NULL\n",
                                       __FUNCTION__);
            }

            src_sgd_ptr->offset = 0;
            src_sg_element.address = fbe_raid_memory_id_get_memory_ptr((fbe_memory_id_t)curr_buffer_id);
            src_sg_element.count = bm_extract_memory_size(curr_buffer_id);
        }
        else if (curr_buffer_id != NULL)
        {
            /* There is nothing left for this request.
             * However, there is another buffer in the list. Move to the
             * next buffer.
             */
            src_sgd_ptr->offset = 0;
            src_sg_element.address = fbe_raid_memory_id_get_memory_ptr((fbe_memory_id_t)curr_buffer_id);
            src_sg_element.count = bm_extract_memory_size(curr_buffer_id);
        }
        else
        {
            /* There is nothing left for this request
             * and there are no more buffers in the src. 
             * Set the src sg element to NULL
             */
            src_sg_element.address = NULL;
            src_sg_element.count = 0;
        }
    }

    /*
     * Terminate the S/G list properly.
     */

    dest_sgl_ptr->address = 0,
    dest_sgl_ptr->count = 0;

    return sg_total;
}  /* end rg_clip_buffer_list */
#endif

/***************************************************************************
 *      fbe_raid_copy_sg_list()
 ***************************************************************************
 * @brief
 *  This function is responsible for initializing the elements in
 *  one S/G list from the contents of another.
 *

 *  src_sgl_ptr     [I] - ptr to the S/G list which serves as
 *                        source.
 *
 *  dest_sgl_ptr    [I] - ptr to the S/G list which serves as
 *                        destination.
 *
 * @return VALUE:
 *  Pointer to S/G list terminator in destination.
 *
 * @author
 *  23-Jan-97       SPN:    Created.
 *  09-Jul-99       Rob Foley:    Renamed to fbe_raid_copy_sg_list
 *  12-Mar-03       PT:     Added phys_addr component to BM SGL
 *
 **************************************************************************/

fbe_sg_element_t *fbe_raid_copy_sg_list(fbe_sg_element_t * src_sgl_ptr,
                                         fbe_sg_element_t * dest_sgl_ptr)
{
    fbe_raid_sg_declare_physical_address(fbe_u64_t phys_addr_local = 0);

    while (0 != (dest_sgl_ptr->address = src_sgl_ptr->address,
                 dest_sgl_ptr->count = src_sgl_ptr->count))
    {
        fbe_raid_sg_get_physical_address(src_sgl_ptr, phys_addr_local);
        fbe_raid_sg_set_physical_address(dest_sgl_ptr, phys_addr_local);

        dest_sgl_ptr++, src_sgl_ptr++;
    }

    return dest_sgl_ptr;

}  /* end fbe_raid_copy_sg_list */

/****************************************************************
 *                fbe_raid_get_sg_ptr_offset
 ****************************************************************
 *
 * Description:
 *
 *     This function is called to get the offset into the sg list
 *
 * Input:
 *
 *              sg_ptr_ptr:     a pointer to the sg list pointer
 *              blocks:         block count
 *
 * Returns:
 *              block_offset
 *
 * History:
 *
 *      05/20/99 JJIN    Created
 *
 ****************************************************************/

fbe_s32_t fbe_raid_get_sg_ptr_offset(fbe_sg_element_t ** sg_ptr_ptr,
                                     fbe_s32_t blocks)
{
    fbe_s32_t block_offset,
    sg_block_count;

    block_offset = blocks;

    while (sg_block_count = (*sg_ptr_ptr)->count / FBE_BE_BYTES_PER_BLOCK,
           block_offset >= sg_block_count)
    {
        /*
         * this sg_ptr contains less blocks than our block_offset
         * so go on to the next sg_ptr in the list
         */

        block_offset -= sg_block_count;
        (*sg_ptr_ptr)++;
    }

    return block_offset;

}
/***************************************
 * End of fbe_raid_get_sg_ptr_offset()
 ***************************************/

#if 0
/****************************************************************
 *  fbe_raid_fill_invalid_sectors
 ****************************************************************
 *
 * Description:
 *
 *     This function is called to fill in the invalid sectors.
 *
 * Input:
 *
 *              sg_ptr:         a pointer to the scatter-gather list
 *              seed:           seed for checksum
 *              blocks:         blocks to invalidate
 *
 * Note:
 *      we will invalidate blocks of sectors starting from the beginning
 *      of the sg list.
 *
 * Returns: none
 *
 * History:
 *
 *      05/05/99 JJIN    Created
 *
 ****************************************************************/
fbe_status_t fbe_raid_fill_invalid_sectors(fbe_sg_element_t * sg_ptr,
                               fbe_u32_t seed,
                               fbe_s32_t blocks)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t *cur_sg_ptr = sg_ptr;
    fbe_s32_t j;
    fbe_u8_t *byte_ptr;
    fbe_s32_t sg_block_count,
    temp_blocks;

    while (blocks > 0)
    {
        if (RAID_COND(cur_sg_ptr->count <= 0) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: %s errored: cur_sg_ptr->count 0x%x <= 0\n",
                                   __FUNCTION__, cur_sg_ptr->count);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        sg_block_count = cur_sg_ptr->count / FBE_BE_BYTES_PER_BLOCK;
        temp_blocks = FBE_MIN(blocks, sg_block_count);

        for (j = 0, byte_ptr = (fbe_u8_t *) cur_sg_ptr->address;
            j < temp_blocks;
            j++, byte_ptr += FBE_BE_BYTES_PER_BLOCK, seed++)
        {
            status = fbe_xor_fill_invalid_sector(byte_ptr,
                                        seed,
                                        FBE_SECTOR_INVALID_REASON_RAID_CORRUPT_CRC,
                                        0);
            if (status != FBE_STATUS_OK)
            {
                return status;
            }
        }

        blocks -= temp_blocks;

        cur_sg_ptr++;
    }

    return FBE_STATUS_OK;
}
/***************************************
 * End of fbe_raid_fill_invalid_sectors()
 ***************************************/
/****************************************************************** 
 * RG_CHECK_DATA_GB
 *   This structure keeps track of the sector checking
 *   information.
 *  
 *  Fields:
 *   b_check_low_word - Only check the low 16 bytes of word.
 * 
 *   b_check_high - Only check high 16 bytes of word.
 * 
 *   end_word_to_check - Index of last word in block to check.
 * 
 *   b_check_byte_swap - Byte swap the word before checking.
 * 
 *   check_start_word - Index of first word in block to check. 
 * 
 *   patterns_per_block - check_start_lba word_check_mask
 * 
 *   fbe_panic_on_mismatch - FBE_TRUE to fbe_panic if a mismatch is detected.
 *                       FBE_FALSE to simply display the mismatch and continue.
 * 
 *   check_pattern_detected - FBE_TRUE if a pattern has been detected.
 * 
 *   check_word_increment - Words to increment between patterns in the block.
 * 
 *   disable_checking_on_miscompare - FBE_TRUE means to stop checking when a
 *                             mismatch is detected instead of fbe_panicking.
 *  
 *   detect_on_mismatch - If we detect a mismatch, then try to re-detect the
 *   pattern before fbe_panicking.
 *  
 *   detect_retries - Number of times we have attempted to retry in order to get
 *   a match.
 *  
 *   detect_retry_max - Max number of times we will retry in order to get a
 *   match.  In many cases, such as with arrayx or DAQ, we need a retry of 1 for
 *   since the pattern looks different when it is the first block.  With DAQ the
 *   first block has a pass count of zero, so a mask isn't used, but with the
 *   second pass, we need to use a mask since pass count is nonzero.
 ******************************************************************/
typedef struct RG_CHECK_DATA_GB
{
    fbe_bool_t detect_on_mismatch;
    fbe_bool_t disable_checking_on_miscompare;
    fbe_u32_t detect_retries;
    fbe_u32_t detect_retry_max;
    fbe_bool_t b_check_low_word;
    fbe_bool_t b_check_high;
    fbe_bool_t b_check_byte_swap;
    fbe_u32_t check_start_word;
    fbe_u32_t end_word_to_check;
    fbe_u32_t patterns_per_block;
    fbe_u32_t check_start_lba;
    fbe_u32_t word_check_mask;    //was 0xFFFFFF00
    fbe_bool_t fbe_panic_on_mismatch;
    fbe_bool_t check_pattern_detected;
    fbe_u32_t check_word_increment;
}
RG_CHECK_DATA_GB;

/************************************************** 
 *  rg_check_data_gb
 *    This is the global instance of RG_CHECK_DATA_GB.
 **************************************************/
RG_CHECK_DATA_GB rg_check_data_gb = 
{
    FBE_TRUE,    /* detect_on_mismatch */
    FBE_FALSE,    /* disable_checking_on_miscompare */
    0,    /* detect retries */
    2,    /* detect retry max */
    FBE_FALSE,    /* b_check_low_word */
    FBE_FALSE,    /* b_check_high */
    FBE_FALSE,    /* b_check_byte_swap */
    3,    /* check_start_word */
    128 - 4,    /* end_word_to_check */
    0,    /* patterns per block */
    1024,    /* check_start_lba */
    0xFFFFFFFF,    /* word_check_mask */
    FBE_TRUE,    /* fbe_panic_on_mismatch */
    FBE_FALSE,    /* check_pattern_detected */
    1,    /* check_word_increment  */
};
/* end rg_check_data_gb */

/************************************************** 
 *  Accessor methods for rg_check_data_gb.
 **************************************************/
fbe_u32_t raid_get_check_word_increment(void)
{
    return rg_check_data_gb.check_word_increment;
}
fbe_u32_t raid_get_check_word_start(void)
{
    return rg_check_data_gb.check_start_word;
}
fbe_u32_t raid_get_check_word_count(void)
{
    return rg_check_data_gb.end_word_to_check;
}
fbe_u32_t raid_get_check_pattern_count(void)
{
    return rg_check_data_gb.patterns_per_block;
}
fbe_bool_t raid_get_pattern_detected(void)
{
    return rg_check_data_gb.check_pattern_detected;
}
void raid_reset_check_pattern_detected(void)
{
    rg_check_data_gb.check_pattern_detected = 0;
    return;
}
void raid_reset_check_pattern_detect_retries(void)
{
    rg_check_data_gb.detect_retries = 0;
    return;
}
void rg_set_check_data(fbe_bool_t value)
{
    /* Reset pattern checking so we will attempt to detect a pattern
     * again.  
     */
    raid_reset_check_pattern_detected();
    raid_reset_check_pattern_detect_retries();

    RG_DATA_CHECKING() = value;

    if (value)
    {
        /* Data checking is enabled.
         *
         * Set the final state to be the data checking/
         * validation state.  This causes us to
         * check data and fbe_panic if the data pattern
         * is not:
         *  seeded SAD/ADR DAQ pattern
         *  or zeros.
         * For everything else we will fbe_panic.
         */
        rg_siots_finished = rg_siots_finished_state0;
        //KTRACE("RAID: Data Checking Enabled \n",0,0,0,0);
    }
    else
    {
        /* Data checking is disabled.
         *
         * Go back to normal mode where we do not
         * check data at all.
         */
        rg_siots_finished = rg_siots_finished_state2;
        //KTRACE("RAID: Data Checking Disabled \n",0,0,0,0);
    }
    return;
}
/************************************************** 
 *  End accessor methods for rg_check_data_gb.
 **************************************************/

/****************************************************************
 * raid_find_pattern_in_sector()
 ****************************************************************
 * @brief
 *  Find the location of a pattern in the sector.
 *

 * @param check_word - Pattern to find.       
 * @param ref_sect_p - Sector to search.               
 *
 * @return
 *  Index of the pattern in this block..             
 *
 * @author
 *  5/20/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t raid_find_pattern_in_sector(const fbe_u32_t check_word, SECTOR * const ref_sect_p)
{
    fbe_u32_t word_index;

    /* Find first word with lba match.
     */
    for (word_index = 0;
        word_index < BYTES_PER_BLOCK/sizeof(fbe_u32_t);
        word_index += rg_check_data_gb.check_word_increment)
    {
        /* First check for non-swapped.
         */
        if ((ref_sect_p->data_word[word_index] & rg_check_data_gb.word_check_mask) == check_word)
        {
            break;
        }
        /* Next check for byte swapped.
         */
        if ((ref_sect_p->data_word[word_index] & rg_check_data_gb.word_check_mask) == 
            SET_ENDIAN32(check_word))
        {
            /* Mark the fact that we found a byte swapped pattern. 
             */
            rg_check_data_gb.b_check_byte_swap = FBE_TRUE;
            break;
        }

    }    /* end for all words in block to check */

    return(word_index);
}
/******************************************
 * end raid_find_pattern_in_sector()
 ******************************************/

/****************************************************************
 * raid_detect_pattern()
 ****************************************************************
 * @brief
 *  Automatically detect the location of an lba seeded pattern
 *  within the 512 bytes of user data.
 *

 * @param sector_p - Sector to detect.
 * @param seed - lba to detect.
 * @param b_is_virtual - true if memory is virtual.
 *
 * @return
 *  FBE_TRUE if pattern detected, FBE_FALSE otherwise.             
 *
 * @author
 *  4/29/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t raid_detect_pattern(SECTOR *const sector_p, 
                               const fbe_u32_t seed,
                               const fbe_bool_t b_is_virtual)
{
    fbe_u32_t word_index;
    fbe_u32_t check_word = seed;
    PVOID reference_p = NULL;
    SECTOR *ref_sect_p = NULL;

    /* First init the pattern location.
     */
    rg_check_data_gb.check_start_word = 0;
    rg_check_data_gb.end_word_to_check = BYTES_PER_BLOCK/sizeof(fbe_u32_t) - rg_check_data_gb.check_start_word;
    rg_check_data_gb.check_word_increment = 1;
    rg_check_data_gb.b_check_byte_swap = FBE_FALSE;
    rg_check_data_gb.word_check_mask = 0xFFFFFFFF;

    /* MAP IN SECTOR HERE if we need to.
     */
    reference_p = (b_is_virtual)?
                  (PVOID)sector_p:
                  RAID_MAP_MEMORY(((fbe_u32_t *)sector_p), FBE_BE_BYTES_PER_BLOCK);
    ref_sect_p = (SECTOR *) reference_p;

    /* Try to find a match with word check mask set to the default.
     */
    word_index = raid_find_pattern_in_sector(check_word, ref_sect_p);

    if (word_index >= BYTES_PER_BLOCK/sizeof(fbe_u32_t))
    {
        /* No match found.
         * Try to find a match with word check mask set to the bottom 3 bytes.
         */
        rg_check_data_gb.word_check_mask = 0x00FFFFFF;

        /* Set the check word to only have as many bits as the check mask. 
         * This allows the comparison below to compare the same number of bits. 
         */
        check_word &= rg_check_data_gb.word_check_mask;

        word_index = raid_find_pattern_in_sector(check_word, ref_sect_p);
    }

    if (word_index >= BYTES_PER_BLOCK/sizeof(fbe_u32_t))
    {
        /* Map OUT memory just mapped in.
         */ 
        if ( !b_is_virtual )
        {
            RAID_UNMAP_MEMORY(reference_p, FBE_BE_BYTES_PER_BLOCK);
        }
        /* Pattern not found, return.
         */
        return FBE_FALSE;
    }

    if (rg_check_data_gb.b_check_byte_swap == FBE_TRUE)
    {
        /* If we detected a swapped pattern, then change the check word, to help
         * simplify checking below. 
         */
        rg_check_data_gb.b_check_byte_swap = FBE_TRUE;
        check_word = SET_ENDIAN32(check_word);
    }

    /* Pattern found, init location to start checking and number of words to
     * check. 
     */
    rg_check_data_gb.check_start_word = word_index;
    rg_check_data_gb.end_word_to_check = BYTES_PER_BLOCK/sizeof(fbe_u32_t) - rg_check_data_gb.check_start_word;

    /* Find next word.
     */
    for (word_index = rg_check_data_gb.check_start_word + 1;
         word_index < BYTES_PER_BLOCK/sizeof(fbe_u32_t);
         word_index += rg_check_data_gb.check_word_increment)
    {
        if ((ref_sect_p->data_word[word_index] & rg_check_data_gb.word_check_mask) == check_word)
        {
            break;
        }
    }    /* end for all words in block to check */

    if ( word_index == BYTES_PER_BLOCK/sizeof(fbe_u32_t))
    {
        /* We did not find the pattern again. 
         * Just assume an increment of 1. 
         */
        rg_check_data_gb.check_word_increment = 1;
    }
    else
    {
        /* We found the pattern again, calculate the increment.
         */
        rg_check_data_gb.check_word_increment = word_index - rg_check_data_gb.check_start_word;
    }

    /* Find number of words to check total with the given increment.
     */
    rg_check_data_gb.patterns_per_block = 0;
    for (word_index = rg_check_data_gb.check_start_word;
         word_index < BYTES_PER_BLOCK/sizeof(fbe_u32_t);
         word_index += rg_check_data_gb.check_word_increment)
    {
        if ((ref_sect_p->data_word[word_index] & rg_check_data_gb.word_check_mask) != check_word)
        {
            /* Found our first mismatch.  Stop checking.
             */
            break;
        }
        rg_check_data_gb.patterns_per_block++;
    }    /* end for all words in block to check. */

    /* Save end word to check.
     */
    rg_check_data_gb.end_word_to_check = word_index;
    if (RAID_COND(rg_check_data_gb.patterns_per_block == 1 && rg_check_data_gb.check_word_increment != 1 ))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: %s errored: patterns_per_block 0x%x == 1 && check_word_increment 0x%x != 1\n",
                               __FUNCTION__, patterns_per_block, check_word_increment);
        return FBE_FALSE;
    }

    /* Map OUT memory just mapped in.
     */
    if ( !b_is_virtual )
    {
        RAID_UNMAP_MEMORY(reference_p, FBE_BE_BYTES_PER_BLOCK);
    }

    /* If we are here, then a pattern was detected.
     */
    rg_check_data_gb.check_pattern_detected = FBE_TRUE;

    KTRACE("RAID: check pattern was detected. lba:0x%x\r\n", seed, 0, 0, 0);
    KTRACE("RAID: detect_on_mismatch. 0x%x\r\n", rg_check_data_gb.detect_on_mismatch, 0, 0, 0);
    KTRACE("RAID: disable_checking_on_miscompare 0x%x\r\n", rg_check_data_gb.disable_checking_on_miscompare, 0, 0, 0);
    KTRACE("RAID: detect_retries 0x%x\r\n", rg_check_data_gb.detect_retries, 0, 0, 0);
    KTRACE("RAID: detect_retry_max 0x%x\r\n", rg_check_data_gb.detect_retry_max, 0, 0, 0);
    KTRACE("RAID: b_check_low_word 0x%x\r\n", rg_check_data_gb.b_check_low_word, 0, 0, 0);
    KTRACE("RAID: b_check_high 0x%x\r\n", rg_check_data_gb.b_check_high, 0, 0, 0);
    KTRACE("RAID: b_check_byte_swap 0x%x\r\n", rg_check_data_gb.b_check_byte_swap, 0, 0, 0);
    KTRACE("RAID: check_start_word 0x%x\r\n", rg_check_data_gb.check_start_word, 0, 0, 0);
    KTRACE("RAID: check_word_increment 0x%x\r\n", rg_check_data_gb.check_word_increment, 0, 0, 0);
    KTRACE("RAID: end_word_to_check 0x%x\r\n", rg_check_data_gb.end_word_to_check, 0, 0, 0);
    KTRACE("RAID: patterns_per_block 0x%x\r\n", rg_check_data_gb.patterns_per_block, 0, 0, 0);
    KTRACE("RAID: check_start_lba 0x%x\r\n", rg_check_data_gb.check_start_lba, 0, 0, 0);
    KTRACE("RAID: word_check_mask 0x%x\r\n", rg_check_data_gb.word_check_mask, 0, 0, 0);
    KTRACE("RAID: fbe_panic_on_mismatch 0x%x\r\n", rg_check_data_gb.fbe_panic_on_mismatch, 0, 0, 0);
    KTRACE("RAID: check_pattern_detected 0x%x\r\n", rg_check_data_gb.check_pattern_detected, 0, 0, 0);
    return FBE_TRUE;
}
/******************************************
 * end raid_detect_pattern()
 ******************************************/
/**************************************************************************
 * fbe_raid_check_sector_for_pattern()
 **************************************************************************
 *
 * @brief
 *  Check data in sector for the lba seed we expect to find.
 *

 * @param sector_p - a ptr to the sector to check
 * @param seed - seed to check sector against
 *   b_is_virtual  [I] - indicates if sector_p is a virtual addr
 *
 * @return VALUES:
 *   FBE_TRUE - if Okay
 *   FBE_FALSE - if Failed check.
 *
 * @notes
 *   None
 *
 * @author
 *   04/29/08  Rob Foley -- Created.
 *
 **************************************************************************/

fbe_bool_t fbe_raid_check_sector_for_pattern(SECTOR *const sector_p, 
                                         const fbe_u32_t seed,
                                         const fbe_bool_t b_is_virtual)
{
    fbe_u32_t word_index;
    fbe_u32_t check_word;
    PVOID reference_p = NULL;
    SECTOR *ref_sect_p = NULL; 

    /* First determine the pattern to check against.
     */
    if (rg_check_data_gb.b_check_byte_swap)
    {
        check_word = SET_ENDIAN32(seed);
    }
    else
    {
        check_word = seed;
    }

    if (rg_check_data_gb.b_check_high)
    {
        check_word >>= 16;
    }
    if (rg_check_data_gb.b_check_low_word)
    {
        check_word &= 0xFFFF0000;
    }

    /* MAP IN SECTOR HERE if we need to.
     */
    reference_p = (b_is_virtual)?
                  (PVOID)sector_p:
                  RAID_MAP_MEMORY(((fbe_u32_t *)sector_p), FBE_BE_BYTES_PER_BLOCK);
    ref_sect_p = (SECTOR *) reference_p;

    for (word_index = rg_check_data_gb.check_start_word;
        word_index < rg_check_data_gb.end_word_to_check;
        word_index += rg_check_data_gb.check_word_increment)
    {
        if ( ((ref_sect_p->data_word[word_index] & rg_check_data_gb.word_check_mask) != check_word) &&
             ((ref_sect_p->data_word[word_index] & rg_check_data_gb.word_check_mask) != 0) )
        {
            KTRACE("RAID: detected mismatch at 0x%x\r\n", 
                   seed, 0, 0, 0);
            KTRACE("Expected:0x%x\r\n", check_word, 0, 0, 0);
            KTRACE("Received:0x%x\r\n", 
                   ref_sect_p->data_word[word_index], 0, 0, 0);
            KTRACE("block data:\r\n", 0, 0, 0, 0);

            for (word_index = 0;
                word_index < 128;
                word_index += 4 )
            {
                KTRACE("0x%x 0x%x 0x%x 0x%x\r\n", 
                       ref_sect_p->data_word[word_index],
                       ref_sect_p->data_word[word_index + 1],
                       ref_sect_p->data_word[word_index + 2],
                       ref_sect_p->data_word[word_index + 3]);
            }
            /* Map OUT memory just mapped in */
            if ( !b_is_virtual )
            {
                RAID_UNMAP_MEMORY(reference_p, FBE_BE_BYTES_PER_BLOCK);
            }
            return FBE_FALSE;
        }
    }    /* for word_index=check_start_word;word_index < end_word_to_check */

    /* Map OUT memory just mapped in */
    if ( !b_is_virtual )
    {
        RAID_UNMAP_MEMORY(reference_p, FBE_BE_BYTES_PER_BLOCK);
    }
    return FBE_TRUE;
}  /* fbe_raid_check_sector_for_pattern() */

/**************************************************************************
 * fbe_raid_check_sector()
 **************************************************************************
 *
 * @brief
 *  Check data on sector against a seed.
 *

 * @param sector_p - a ptr to the sector to check
 * @param seed - seed to check sector against
 *   isVirtual  [I] - indicates if sector_p is a virtual addr
 *
 * @return VALUES:
 *   FBE_TRUE - if Okay
 *   FBE_FALSE - if Failed check.
 *
 * @notes
 *   None
 *
 * @author
 *   05/10/01  Rob Foley -- Created.
 *   10/30/01  PT  -- mapin/mapout was faulty. Fixed to properly use functions
 *
 **************************************************************************/

fbe_bool_t fbe_raid_check_sector(SECTOR *sector_p, 
                             fbe_u32_t seed,
                             fbe_bool_t b_is_virtual)
{
    /* Transform the seed into a pattern to check
     * the sector against.
     */
    if (seed >= rg_check_data_gb.check_start_lba)
    {
        if (rg_check_data_gb.check_pattern_detected == FBE_FALSE)
        {
            if (!raid_detect_pattern(sector_p, seed, b_is_virtual))
            {
                /* If we didn't detect the pattern, then
                 * pattern checking remains disabled.
                 */
                return FBE_TRUE;
            }
        }

        /* If we are here then pattern checking is on.
         */
        if (!fbe_raid_check_sector_for_pattern(sector_p, seed, b_is_virtual))
        {
            rg_check_data_gb.detect_retries++;

            /* We may have incorrectly detected the pattern originally.
             * This especially can happen with arrayx pattern. 
             * Display that we will try again to detect the pattern.
             */
            if (rg_check_data_gb.detect_on_mismatch == FBE_FALSE ||
                rg_check_data_gb.detect_retries >= rg_check_data_gb.detect_retry_max || 
                !raid_detect_pattern(sector_p, seed, b_is_virtual))
            {
                /* If we didn't detect the pattern, then we have a real
                 * miscompare. 
                 */
                if (rg_check_data_gb.fbe_panic_on_mismatch)
                {
                    /* On miscompare fbe_panic.
                     */
#ifdef RG_DEBUG
                    DbgBreakpoint();
#endif
                    fbe_panic(RG_MISCOMPARE, __LINE__);
                    return FBE_FALSE;
                }
                else if (rg_check_data_gb.disable_checking_on_miscompare)
                {
                    /* On miscompare disable checking.
                     */
                    RG_DATA_CHECKING() = 0;
                    return FBE_FALSE;
                }
                else
                {
                    /* On miscompare continue checking.
                     */
                    return FBE_FALSE;
                }
                return FBE_TRUE;
            }
            else
            {
                /* We resolved the mismatch.
                 */
                KTRACE("RAID: detected match for 0x%x\r\n", seed, 0, 0, 0);
            }
        }

    }    /* end if lba < check_start_lba */

    /* Not checking pattern, return success.
     */
    return FBE_TRUE;
}  /* fbe_raid_check_sector() */

/**************************************************************************
 * fbe_raid_check_data()
 **************************************************************************
 *
 * @brief
 *  Check data on an sg element.  The data must be lba seeded.
 *

 * @param fbe_memory_id_t - a memory id to use
 * @param seedA - seed ptr
 * @param blkcntA - blkcnt ptr
 * @param check_csum - true to check checksum only, false otherwise.
 * @param fruts_p - Ptr to fruts to check.
 *
 * @return VALUES:
 *   fbe_status_t
 *
 * @notes
 *   none
 *
 * @author
 *   08/29/00  Rob Foley -- Created
 *   07/13/01  Rob Foley -- Changed to use fbe_raid_check_sector.
 *                    Also changed to check chained sg lists.
 *
 **************************************************************************/

fbe_status_t fbe_raid_check_data(fbe_memory_id_t fbe_memory_id_t,
                          VOID * seedA,
                          VOID * blkcntA,
                          fbe_bool_t check_csum,
                          const fbe_raid_fruts_t * const fruts_p)
{
    fbe_s32_t status = 0;
    fbe_sg_element_t * sgptr = (fbe_sg_element_t *) fbe_raid_memory_id_get_memory_ptr(fbe_memory_id_t);
    fbe_memory_id_t current_memory_id = fbe_memory_id_t;
    fbe_u32_t seed = *(fbe_u32_t *) seedA;
    fbe_u16_t blkcnt = *(fbe_u16_t *) blkcntA;

    if (!check_csum && !RG_DATA_CHECKING())
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    {
        /* Loop through all the blocks in our transfer.
         */
        while (blkcnt > 0)
        {
            fbe_u8_t *bytptr;
            fbe_u16_t cnt;

            if (RAID_COND((sgptr == NULL) ||
                          (0 >= sgptr->count) ||
                          (0 != sgptr->count % FBE_BE_BYTES_PER_BLOCK) ) )
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "raid: %s errored sgptr is NULL or sgptr->count is < 0 "
                                       "or sgptr->count is unexpected\n",
                                       __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            cnt = FBE_MIN(blkcnt, sgptr->count / FBE_BE_BYTES_PER_BLOCK);

            blkcnt -= cnt,
            bytptr = sgptr->address;

            /* Detect Token,  
             * skip those Tokenized elements.
             */
            if (sgptr->address == (fbe_u8_t *) RG_TOKEN_ADDRESS)
            {
                seed += cnt;
            }
            else
            {
                /* Loop through all the blocks 
                 * in this SG element.
                 */
                do
                {
                    SECTOR *blkptr = (SECTOR *) bytptr;
                    PVOID reference_p = NULL;
                    SECTOR *ref_sect_p = NULL;
                    fbe_u32_t rsum;

                    reference_p = (fbe_raid_virtual_addr_available(sgptr))?
                                  (PVOID)blkptr:
                                  RAID_MAP_MEMORY(((fbe_u32_t *)blkptr), FBE_BE_BYTES_PER_BLOCK);

                    ref_sect_p = (SECTOR *) reference_p;

                    rsum = xorlib_calc_csum(ref_sect_p->data_word);

                    if (check_csum)
                    {
                        if ( ref_sect_p->crc != xorlib_cook_csum(rsum, (fbe_u32_t) - 1) )
                        {
                            /* Just display the err'd block and then fbe_panic.
                             */
                            KTRACE("RAID: Writing invalid sector seed: %x bm_id:%x err_block: %x\n",
                                   seedA, (fbe_u32_t *)fbe_memory_id_t, ((fbe_u32_t *)blkcntA) - blkcnt, blkcntA);

                            fbe_panic(RG_INVALID_STATE, 0);
                        }
                        if (fruts_p != NULL)
                        {
                            fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);

                            /* On parity units, fbe_panic if we try to write out
                             * all timestamps on a data position.
                             */
                            if (RG_SUB_IOTS_RGDB(siots_p)->unit_type == RG_RAID5)
                            {
                                fbe_u16_tE current_bitmask = (1 << fruts_p->position);

                                if (fruts_p->position == siots_p->parity_pos)
                                {
                                    /* Write stamp should not be set for parity position.
                                     */
                                    if (RAID_COND((ref_sect_p->write_stamp & current_bitmask) != 0) )
                                    {
                                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                             "raid: %s errored: (ref_sect_p->write_stamp 0x%x & current_bitmask 0x%x ) != 0\n",
                                                             __FUNCTION__, ref_sect_p->write_stamp, current_bitmask);
                                        return FBE_STATUS_GENERIC_FAILURE;
                                    }
                                }
                                else
                                {
                                    /* If any writestamp set, should be for this position.
                                     */
                                    if (RAID_COND(ref_sect_p->write_stamp != current_bitmask &&
                                                  ref_sect_p->write_stamp != 0 ) )
                                    {
                                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                             "raid: %s errored: ref_sect_p->write_stamp 0x%x != current_bitmask 0x%x "
                                                             "&& ref_sect_p->write_stamp 0x5x != 0\n",
                                                             __FUNCTION__, ref_sect_p->write_stamp, current_bitmask, ref_sect_p->write_stamp);
                                        return FBE_STATUS_GENERIC_FAILURE;
                                    }

                                    if (RAID_COND(ref_sect_p->write_stamp != 0 && 
                                                  ref_sect_p->time_stamp != FBE_SECTOR_INVALID_TSTAMP))
                                    {
                                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                             "raid: %s errored: ref_sect_p->write_stamp 0x%x != 0"
                                                             "&& ref_sect_p->time_stamp 0x%x != FBE_SECTOR_INVALID_TSTAMP\n",
                                                             __FUNCTION__, ref_sect_p->write_stamp, ref_sect_p->time_stamp);
                                        return FBE_STATUS_GENERIC_FAILURE;
                                    }

                                    if (RAID_COND((ref_sect_p->time_stamp & FBE_SECTOR_ALL_TSTAMPS) != 0) )
                                    {
                                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                             "raid: %s errored: (ref_sect_p->time_stamp 0x%x & FBE_SECTOR_ALL_TSTAMPS) != 0\n",
                                                             __FUNCTION__, ref_sect_p->time_stamp, current_bitmask);
                                        return FBE_STATUS_GENERIC_FAILURE;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        /* Make sure this sector matches
                         * our seeded pattern.
                         */
                        status = fbe_fbe_raid_check_sector(blkptr, seed, fbe_raid_virtual_addr_available(sgptr));
                    }
                    bytptr += FBE_BE_BYTES_PER_BLOCK,
                    seed++;
                    /* Map OUT memory just mapped in */
                    if ( !fbe_raid_virtual_addr_available(sgptr) )
                    {
                        RAID_UNMAP_MEMORY(reference_p, FBE_BE_BYTES_PER_BLOCK);
                    }
                }
                while (0 < --cnt);
            }
            /* Increment to the next SG element in this SG list.
             */
            sgptr++;

            if (sgptr->count == NULL)
            {
                /* We hit the null terminator.
                 * If this is a concatenated SG list, then
                 * just try to grab the next link.
                 */
                if (bm_get_next(current_memory_id)!= NULL)
                {
                    /* Yea, we have a concatenated SG list 
                     * try to get the next link.
                     */ 
                    current_memory_id = bm_get_next(current_memory_id);
                    sgptr = (fbe_sg_element_t *) fbe_raid_memory_id_get_memory_ptr(current_memory_id);

                    /* If we are not done (blkcnt == 0)
                     * then we must still have an SG ptr.
                     * Otherwise the sg list we were passed
                     * is too small for the block count.
                     */
                    if (RAID_COND((blkcnt != 0) && (sgptr == NULL)) )
                    {
                        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                               "raid: %s errored: block count 0x%x is nonzero and sgptr is NULL\n",
                                               __FUNCTION__, blkcnt);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }
                else
                {
                    /* If we hit the end of the SG list and 
                     * there are no further links, 
                     * then this must be the end of the transfer
                     * and we are done checking.
                     */
                    if (RAID_COND(blkcnt != 0) )
                    {
                        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                               "raid: %s errored as blkcnt 0x%x != 0\n",
                                               __FUNCTION__, blkcnt);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }
            }
        }
    }

    return status;
}
/***************************************
 * end of fbe_raid_check_data()
 ***************************************/
#endif

/*************
 * end $Id: rg_sg_util.c,v 1.1.8.1 2000/10/13 14:25:49 fladm Exp $
 *************/
