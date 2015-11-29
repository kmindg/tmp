/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_bitbucket.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions of the logical drive object's bitbucket.  The
 *  bitbucket has two uses.  It is used for both reads, where we need to throw
 *  away the edge data for edges the client is not requesting.  For writes we
 *  use the bitbucket for "lossy" cases where the imported optimal block size
 *  does not match the exported optimal block size, and some space is being
 *  wasted.
 * 
 *  For reads, the bitbucket is used for throwing away edge data that we do not
 *  care about. Take the example of a single 520 block read where the underlying
 *  block size is 512. In this case we want to read a single 520 byte block, but
 *  in order to do so, we need to read two underlying 512 byte blocks.  The
 *  "edges" of the 512 byte blocks around the data we care about (the 520 byte
 *  block), are not being requested by the host and need to get thrown away.
 *  Thus, we use the bitbucket to throw away these edges.
 * @verbatim
 * 
 *               |ioioioioioioioioioio|         <--- Exported block 520
 *  |--------------------|--------------------| <--- Imported block 512
 * /|\        /|\                    /|\     /|\                   
 *  |__________|                      |_______|
 *      First edge                     Second edge goes into bitbucket.
 *      goes into bitbucket.
 * 
 * @endverbatim
 *  For writes, the bitbucket is used for filling in the wasted space at the end
 *  of an optimal block.  Take a trivial case where we are mapping one 520 byte
 *  block onto two 512 byte blocks.  The mapping in this case has:
 * 
 *  An optimal block size of 1 for the exported optimal block size 1 x 520 = 520
 *  The imported optimal block size is 2 x 512 = 1024.
 * 
 *  Thus, there is a loss here of 1024 - 520 = 504 bytes.  Thus, at the end of
 *  every optimal block, we need to fill in 504 bytes.
 * 
 *  The example is shown in the picture below.  Each block in the imported or
 *  exported block range is labed with the lba number
 *@verbatim
 *  | Start of first        | Start of second
 *  | 520 byte block
 * \|/                     \|/
 *  |     0       | fill    |       1     | fill          exported - 520
 *  |---0-------|-----1-----|-----2-----|----3------|     imported - 512
 *               /|\       /|\         /|\         /|\ 
 *                |_________|           |___________|
 *                Lost or wasted
 *                space is at the end of every optimal block.
 *                This is the space that needs to get updated with "fill" on
 *                every write.
 * @endverbatim
 * 
 * @ingroup logical_drive_class_files
 * 
 * HISTORY
 *   11/08/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"

/****************************************
 * LOCALLY DEFINED FUNCTIONS
 ****************************************/

/* The read bitbucket is used for throwing away edges that are not needed on
 * reads. 
 */
static fbe_ldo_bitbucket_t fbe_ldo_read_bitbucket;

/* The write bitbucket is used for "lossy" cases where we are using less than 
 * the full physical block. e.g. importing 3 512 blocks and exporting 2 520 
 * blocks. 
 */
static fbe_ldo_bitbucket_t fbe_ldo_write_bitbucket;

/*! @fn fbe_ldo_get_bitbucket_bytes(void)
 *  
 * @brief 
 *   This is a function to hide that this operation might dynamically determine 
 *   the size of the bitbucket in the future. 
 */
fbe_u32_t fbe_ldo_get_bitbucket_bytes(void)
{
    return FBE_LDO_BITBUCKET_BYTES;
}

/*! @fn fbe_ldo_get_read_bitbucket_ptr(void)
 *  
 * @brief 
 *   This is a function to get the read bitbucket.
 */
fbe_ldo_bitbucket_t * fbe_ldo_get_read_bitbucket_ptr(void)
{
    return &fbe_ldo_read_bitbucket;
}

/*! @fn fbe_ldo_get_write_bitbucket_ptr(void)
 *  
 * @brief 
 *   This is a function to get the write bitbucket.
 */
fbe_ldo_bitbucket_t * fbe_ldo_get_write_bitbucket_ptr(void)
{
    return &fbe_ldo_write_bitbucket;
}

/*!***************************************************************
 * @fn fbe_ldo_bitbucket_get_sg_entries(const fbe_u32_t bytes)
 ****************************************************************
 * @brief
 *  This function counts the number of sg entries
 *  required for a bitbucket operation.
 *
 * @param bytes - The number of bytes in the operation
 *               for the bitbucket.
 *
 * @return Number of sg entries needed for this size of bitbucket op.
 *
 * HISTORY:
 *  11/28/07 - Created. RPF
 *
 ****************************************************************/
fbe_u32_t fbe_ldo_bitbucket_get_sg_entries(const fbe_u32_t bytes)
{
    fbe_u32_t sg_entries = 0;
    
    /* Add on the full sg entries needed for the first edge.
     */
    sg_entries += (bytes / fbe_ldo_get_bitbucket_bytes());
    
    /* Add on partial sg entry needed for this edge.
     * If there is a remainder, then add a single sg entry.
     */
    sg_entries += (bytes % fbe_ldo_get_bitbucket_bytes()) ? 1 : 0;

    return sg_entries;
}
/* end fbe_ldo_bitbucket_get_sg_entries() */

/*!***************************************************************
 * @fn fbe_ldo_fill_sg_with_bitbucket(
 *                        fbe_sg_element_t *sg_p,
 *                        fbe_u32_t bytes,
 *                        fbe_ldo_bitbucket_t *const bitbucket_p)
 ****************************************************************
 * @brief
 *  This function fills a given number of bytes with the
 *  bitbucket at the given starting sg entry.
 *
 *  @param sg_p - Starting sg element to start filling.
 *  @param bytes - Total number of bytes to fill into the sg.
 *  @param bitbucket_p - Bitbucket to fill with.
 *
 * @return
 *     sg_element_t* The next free sg entry after we filled it with the given
 *  bytes.
 *
 * HISTORY:
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
fbe_sg_element_t * fbe_ldo_fill_sg_with_bitbucket(fbe_sg_element_t *sg_p,
                                                  fbe_u32_t bytes,
                                                  fbe_ldo_bitbucket_t *const bitbucket_p)
{
    fbe_u32_t bitbucket_bytes = fbe_ldo_get_bitbucket_bytes();

    if (sg_p == NULL || bitbucket_p == NULL || bytes == 0)
    {
        /* If the sg_p is not usable, or the bitbucket is not usable, just
         * return an error. 
         */
        return NULL;
    }

    /* First fill out the sg with all the evenly aligned
     * pieces of the bitbucket.
     */
    while (bytes > bitbucket_bytes)
    {
        fbe_sg_element_init(sg_p, bitbucket_bytes, &bitbucket_p->bytes);
        fbe_sg_element_increment(&sg_p);
        bytes -= bitbucket_bytes;
    }
    
    /* Next fill out the final unaligned piece of the bitbucket.
     */
    fbe_sg_element_init(sg_p, bytes, &bitbucket_p->bytes);
    fbe_sg_element_increment(&sg_p);

    return sg_p;
}
/* end fbe_ldo_fill_sg_with_bitbucket() */

/*!***************************************************************
 * @fn fbe_ldo_bitbucket_init(void)
 ****************************************************************
 * @brief
 *  This function initializes the bitbuckets.
 * 
 *  Note that the read bitbucket does not need to be initted, since
 *  the data in the read bitbucket is constantly used for throwing away edges,
 *  and the data of the read bitbucket is never used.  That is,
 *  the data of the read bitbucket is never returned to a client and
 *  is never written out to the disk.
 * 
 *  The write bitbucket needs to get zeroed out since it is used
 *  for filling in the end of optimal blocks for "lossy" mappings.
 * 
 *  We technically do not need to zero out this fill data, since it is
 *  never actually used.  However, to be consistent and to put this area
 *  into a known state, we will init it to zeros.
 *
 * @param - None
 *
 * @return - None.
 *
 * HISTORY:
 *  06/24/08 - Created. RPF.
 *
 ****************************************************************/
void fbe_ldo_bitbucket_init(void)
{
    fbe_ldo_bitbucket_t * bitbucket_p;

    bitbucket_p = fbe_ldo_get_write_bitbucket_ptr();

    /* Init the bitbucket.
     */
    fbe_zero_memory(bitbucket_p, sizeof(fbe_ldo_bitbucket_t));
    return;
}
/* end fbe_ldo_bitbucket_init() */

/****************************************
 * end fbe_logical_drive_bitbucket()
 ****************************************/
