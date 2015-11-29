/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_map.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions of the logical drive object's
 *  mapping functions.
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
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"

/****************************************
 * LOCALLY DEFINED FUNCTIONS
 ****************************************/


/*****************************************
 * GLOBAL DEFINITIONS
 *****************************************/

/* This is our lookup table defining various block sizes that we support.
 * We use this table in order to determine the optimum block size/alignment.
 */
fbe_ldo_supported_block_size_entry_t fbe_ldo_supported_block_sizes[FBE_LDO_BLOCK_SIZE_LAST] = 
{
    /* blk,  exp bl, imp opt,  exp opt,  imported opt
     *  
     * size  size    blk size  blk size  blk size 
     */

    /* Standard 512 byte formatted device. 
     */
    {512,    520,    1,       64,        65},

    /* Standard 4096 formatted device. 4k 
     */
    {4096,   520,    1,       512,       65},

    /* Standard 4160 formatted device. 4k + 64 (520*8)
     */
    {4160,   520,    1,       8,         1},

    /* Standard 4104 formatted device. 4k+8
     */
    {4104,   520,    1,       513,       65},

    /* Standard 520 formatted device. 
     */
    {520,    520,    1,       1,          1},
};

/*!***************************************************************
 * @fn ldo_get_optimum_block_sizes(fbe_block_size_t imported_block_size,
 *                           fbe_block_size_t exported_block_size,
 *                           fbe_block_size_t *imported_optimum_block_size_p,
 *                           fbe_block_size_t *exported_optimum_block_size_p)
 ****************************************************************
 * @brief
 *  This determines which imported and exported optimum block size
 *  to use for a given device.
 *
 * @param imported_block_size - The block size exported by lower level.
 * @param exported_block_size - The block size to export.
 * @param imported_optimum_block_size_p - Opt blk size exported by lower
 * level.
 * @param exported_optimum_block_size_p - Opt blk size exported to client.
 * 
 * @return fbe_status_t
 *  FBE_STATUS_OK means that this block size is supported.
 *  FBE_STATUS_GENERIC_FAILURE means that this block size is not supported.
 *
 * HISTORY:
 *  11/01/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
ldo_get_optimum_block_sizes(fbe_block_size_t imported_block_size,
                            fbe_block_size_t exported_block_size,
                            fbe_block_size_t *imported_optimum_block_size_p,
                            fbe_block_size_t *exported_optimum_block_size_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t lookup_index;

    if ( imported_block_size == exported_block_size )
    {
        /* If the imported and exported sizes match,
         * then our optimum block sizes are simply 1.
         */
        *exported_optimum_block_size_p = 1;
        *imported_optimum_block_size_p = 1;
        return FBE_STATUS_OK;
    }
    
    /* Find the match for the given imported block sizes.
     */
    for (lookup_index = 0; lookup_index < FBE_LDO_BLOCK_SIZE_LAST; lookup_index++)
    {
        if ( (fbe_ldo_supported_block_sizes[lookup_index].imported_block_size == 
              imported_block_size) &&
             (fbe_ldo_supported_block_sizes[lookup_index].imported_optimum_block_size ==
              *imported_optimum_block_size_p) &&
             (fbe_ldo_supported_block_sizes[lookup_index].exported_block_size == 
              exported_block_size) )
        {
            /* Found the match.
             * Return the appropriate optimum block size/align.
             */
            *exported_optimum_block_size_p = 
                fbe_ldo_supported_block_sizes[lookup_index].exported_optimum_block_size;
            *imported_optimum_block_size_p = 
                fbe_ldo_supported_block_sizes[lookup_index].imported_optimum_block_size_to_configure;
            status = FBE_STATUS_OK;
            return status;
        }
    } /* end for all supported block sizes. */

    /* If we did not find a match, then we should manually calculate the info.
     */
    return status;
}
/*********************************************
 * ldo_get_optimum_block_sizes()
 *********************************************/

/*!***************************************************************
 * @fn fbe_ldo_map_exported_lba_to_byte_offset(
 *                           fbe_lba_t lba,
 *                           fbe_block_size_t exported_block_size,
 *                           fbe_block_size_t exported_opt_block_size,
 *                           fbe_block_size_t imported_block_size,
 *                           fbe_block_size_t imported_opt_block_size,
 *                           fbe_lba_t physical_offset)
 ****************************************************************
 * @brief
 *  
 * @param lba - The logical block address in exported blocks
 * @param exported_block_size - The block size to export.
 * @param exported_opt_block_size - Opt blk size exported to client.
 * @param imported_block_size - The block size exported by lower level.
 * @param imported_opt_block_size - Opt blk size exported by lower level.
 * @param physical_offset - The physical offset is the byte offset from the
 *                          start of the drive where exported blocks start.
 * @verbatim
 * 
 *    physical
 *  | offset    |  Exported Block 0  |  Exported Block 1  |
 *  |-----------|--------------------|--------------------| logical (exported)
 *  |---------------|---------------|---------------| physical (imported)
 * /|\   Block 0    |  Block 1      |    Block 2    | 
 *  | Start of drive
 * 
 * @endverbatim
 * 
 * @return
 *  NONE.
 *
 * HISTORY:
 *  05/13/08 - Created. RPF
 *
 ****************************************************************/
fbe_lba_t 
fbe_ldo_map_exported_lba_to_byte_offset(fbe_lba_t lba,
                                        fbe_block_size_t exported_block_size,
                                        fbe_block_size_t exported_opt_block_size,
                                        fbe_block_size_t imported_block_size,
                                        fbe_block_size_t imported_opt_block_size,
                                        fbe_lba_t physical_offset)
{
    fbe_lba_t optimal_block_number;
    fbe_lba_t imported_start_byte;
    fbe_lba_t byte_offset_from_opt_block;

    /* Get the exported optimal block number this LBA is in.
     */
    optimal_block_number = lba / exported_opt_block_size;

    /* Get the offset from the optimal block boundary.
     */
    byte_offset_from_opt_block = (lba % exported_opt_block_size) * exported_block_size;

    /* Calculate the imported block start byte by adding: 
     * 1) The physical offset. 
     * 2) The byte offset to this optimal block. 
     * 3) The byte offset from the start of this optimal block. 
     */
    imported_start_byte = 
        (physical_offset + (optimal_block_number * imported_opt_block_size * imported_block_size) +
         byte_offset_from_opt_block);

    return imported_start_byte;
}
/**************************************
 * end fbe_ldo_map_exported_lba_to_byte_offset
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_get_edge_sizes(fbe_lba_t lba,
 *                            fbe_block_count_t blocks,
 *                            fbe_block_size_t exported_block_size,
 *                            fbe_block_size_t exported_opt_block_size,
 *                            fbe_block_size_t imported_block_size,
 *                            fbe_block_size_t imported_opt_block_size,
 *                            fbe_u32_t *const edge0_size_p,
 *                            fbe_u32_t *const edge1_size_p)
 ****************************************************************
 * @brief
 *  Get the sizes of the edges.  This takes an input lba and block count and
 *  calculates the sizes of the first and last edges.
 * 
 *  @verbatim
 *
 *             |ioioioioioioioioioio|--------------------|     <--- Exported
 *  |--------------------|--------------------|-------------   <--- Imported
 * /|\        /|\                  /|\       /|\
 *  |__________|                    |_________|   
 *      First edge                   Last edge
 *      size.                        size.
 * 
 * @endverbatim 
 * 
 * @param lba - The logical block address for this transfer.
 * @param blocks - The total blocks in the transfer.
 * @param exported_block_size - The size in bytes of the exported block.
 * @param exported_opt_block_size - The size in blocks (exported blocks) of the
 *                                  exported optimum block.
 * @param imported_block_size - The size in bytes of the physical device.
 * @param imported_opt_block_size - The size in blocks of the
 *                                  imported optimum block size.
 * @param edge0_size_p - Pointer to the first edge size in bytes.
 * @param edge1_size_p - Pointer to the last edge size in bytes.
 *
 * @return none
 *
 * HISTORY:
 *  11/05/07 - Created. RPF
 *  05/13/08 - Rewrote to use map_exported_lba_to_byte_offset. RPF
 *
 ****************************************************************/

void fbe_ldo_get_edge_sizes(fbe_lba_t lba,
                            fbe_block_count_t blocks,
                            fbe_block_size_t exported_block_size,
                            fbe_block_size_t exported_opt_block_size,
                            fbe_block_size_t imported_block_size,
                            fbe_block_size_t imported_opt_block_size,
                            fbe_u32_t *const edge0_size_p,
                            fbe_u32_t *const edge1_size_p)
{
    fbe_lba_t offset = 0;
    fbe_lba_t start_byte_offset;
    fbe_lba_t end_byte_offset;

    /* Offset from the start of the last imported block to the end of the
     * exported block.
     */
    fbe_lba_t imported_byte_offset;

    /* We pass in an lba that is normalized to the optimal block so as to avoid 
     * any possibility of overflow, since we are getting back a byte offset. 
     */
    start_byte_offset = 
        fbe_ldo_map_exported_lba_to_byte_offset( /* Lba normalized to opt block. */
                                                 (lba % exported_opt_block_size),
                                                 exported_block_size, 
                                                 exported_opt_block_size, 
                                                 imported_block_size, 
                                                 imported_opt_block_size, 
                                                 offset);
    
    /*! @par 
     * The first edge size is the offset of the edge from the
     * imported block size boundary.
     * @verbatim
     *             |ioioioioioioioioioio|--------------------|     <--- Exported
     *  |--------------------|--------------------|-------------   <--- Imported
     * /|\        /|\                   
     *  |__________|                    
     *      First edge                   
     *      size.
     * 
     * @endverbatim
     */
    *edge0_size_p = (fbe_u32_t)(start_byte_offset % imported_block_size);
    
    
    /*! @par
     * The second edge size is the offset of the end of the last exported
     * block to the end of the underlying imported block.
     * @verbatim
     *
     *             |ioioioioioioioioioio|--------------------|     <--- Exported
     *  |--------------------|--------------------|-------------   <--- Imported
     *                                  /|\      /|\
     *                                   |________|
     *                                       Second edge
     *                                       size.
     * @endverbatim 
     */

    /* We will then calculate the offset to the start of the last block.
     * We pass in an lba that is normalized to the optimal block so as to avoid 
     * any possibility of overflow, since we are getting back a byte offset.  
     */
    end_byte_offset = fbe_ldo_map_exported_lba_to_byte_offset(
        ((lba + blocks - 1) % exported_opt_block_size), /* Lba normalized to opt block. */
        exported_block_size, 
        exported_opt_block_size, 
        imported_block_size, 
        imported_opt_block_size, 
        offset);

    /* Add on the block size to get one past the last byte.
     */
    end_byte_offset += exported_block_size;

    /* Next, determine the imported byte offset.
     */
    imported_byte_offset = end_byte_offset % imported_block_size;

    /* If this offset is zero, then the last edge is size 0.
     */
    if ( imported_byte_offset == 0)
    {
        *edge1_size_p = 0;
    }
    else
    {
        /* Otherwise calculate the offset from the end of the exported block
         * to the end of the imported block (aka the second edge size).
         */
        *edge1_size_p = (fbe_u32_t)(imported_block_size - imported_byte_offset);
    }

    return;
}
/* end fbe_ldo_get_edge_sizes() */

/*!***************************************************************
 * @fn fbe_ldo_map_imported_lba_to_exported_lba(
 *                  fbe_block_size_t exported_block_size,
 *                  fbe_lba_t exported_opt_block_size,
 *                  fbe_lba_t imported_opt_block_size,
 *                  fbe_lba_t lba)
 ****************************************************************
 * @brief
 *  This function translates an imported lba from
 *  imported block size to exported block size.
 * 
 * @param exported_block_size - The size in bytes of the exported block.
 * @param exported_opt_block_size - The size in blocks (exported blocks) of the
 *                                  exported optimum block.
 * @param imported_opt_block_size - The size in blocks of the
 *                                  imported optimum block size.
 * @param lba - The logical block address for this transfer.
 *
 * @return fbe_lba_t - The converted lba.
 *
 * HISTORY:
 *  05/14/08 - Created. RPF
 *
 ****************************************************************/
fbe_lba_t
fbe_ldo_map_imported_lba_to_exported_lba(fbe_block_size_t exported_block_size,
                                         fbe_lba_t exported_opt_block_size,
                                         fbe_lba_t imported_opt_block_size,
                                         fbe_lba_t lba)
{
    fbe_lba_t optimal_block_number;
    fbe_lba_t exported_lba;

    /* Get the optimal block number this LBA is in.
     */
    optimal_block_number = lba / imported_opt_block_size;

    /* Determine the exported lba by multiplying by the exported optimum block 
     * size. 
     */
    exported_lba = optimal_block_number * exported_opt_block_size;

    return exported_lba;
}   
/*********************************************
 * fbe_ldo_map_imported_lba_to_exported_lba()
 *********************************************/

/*!***************************************************************
 * @fn fbe_ldo_map_get_pre_read_desc_offsets(   
 *                          fbe_lba_t lba,   
 *                          fbe_block_count_t blocks,   
 *                          fbe_lba_t pre_read_desc_lba,
 *                          fbe_block_size_t exported_block_size,
 *                          fbe_block_size_t exported_opt_block_size,
 *                          fbe_block_size_t imported_block_size,
 *                          fbe_u32_t first_edge_size,
 *                          fbe_u32_t *const first_edge_offset_p,
 *                          fbe_u32_t *const last_edge_offset_p)
 ****************************************************************
 * @brief
 *  This function returns the offsets into the edge descriptor
 *  sg list for the first and last edge.
 *
 * @param lba - The logical block address for this transfer in terms of the
 *              exported block..
 * @param blocks - The total blocks in the transfer.
 * @param pre_read_desc_lba - The start lba of the "pre-read descriptor"
 * @param exported_block_size - The size in bytes of the exported block.
 * @param exported_opt_block_size - The size of exported optimal block the in
 *                                  terms of exported blocks.
 * @param imported_block_size - The size in bytes of the imported block.
 * @param first_edge_size - The size in bytes of the first edge.
 * @param first_edge_offset_p - The byte offset from the
 *                              beginning of the pre-read descriptor to the
 *                              start of the host data for the first edge.
 * @param last_edge_offset_p - The byte offset from the beginning of the
 *                              pre-read descriptor to the start of the
 *                              host data for the last edge.
 * 
 * @return fbe_status_t
 *  FBE_STATUS_OK on success.  Other values imply error.
 *
 * HISTORY:
 *  6/4/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_map_get_pre_read_desc_offsets(fbe_lba_t lba,
                                      fbe_block_count_t blocks,
                                      fbe_lba_t pre_read_desc_lba,
                                      fbe_block_size_t exported_block_size,
                                      fbe_block_size_t exported_opt_block_size,
                                      fbe_block_size_t imported_block_size,
                                      fbe_u32_t first_edge_size,
                                      fbe_u32_t *const first_edge_offset_p,
                                      fbe_u32_t *const last_edge_offset_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Beginning byte of the client supplied edge. 
     */
    fbe_lba_t pre_read_desc_start_byte = pre_read_desc_lba * exported_block_size;
    fbe_lba_t last_edge_lba; /* Block where last edge starts */
    fbe_lba_t last_edge_byte;  /* Byte where last edge starts. */
    fbe_lba_t pre_read_to_host_start_offset; /* offset from pre-read lba to host lba*/

    /*! @par 
     * The first edge offset is the offset from the beginning of the edge 
     * descriptor to the start of the first edge. This can be calculated below 
     * by pre_read_to_host_start_offset = (D) - (B) first_edge_size = (D) - (A) 
     * first_edge_offset = pre_read_to_host_start_offset - first_edge_size 
     * @verbatim 
     *                                            
     * (B) - Edge descriptor start
     *  |    (C) - Optimal block boundary
     *  |    |        (D) Start of host I/O
     * \|/  \|/      \|/ 
     *  |----|--------|ioioioioioio|------------| <--- Exported
     *            |----------|----------|        <--- Imported
     *           /|\
     *           (A) - First edge start
     * @endverbatim
     */
    if ((lba % exported_opt_block_size) == 0)
    {
        /* Start is aligned, no pre-read descriptor data needed.
         */
        *first_edge_offset_p = 0;
    }
    else if ( pre_read_desc_lba <= lba)
    {
        pre_read_to_host_start_offset = (lba - pre_read_desc_lba) * exported_block_size;
        *first_edge_offset_p = (fbe_u32_t)(pre_read_to_host_start_offset - first_edge_size);
    }
    else
    {
        /* Error, the start edge needs to be in this edge descriptor.
         * Or we received an overflow for a 32 bit integer.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* Calculate where the client io ends.  This is also the
     * start of the last edge.
     */
    last_edge_lba = lba + blocks;
    last_edge_byte = last_edge_lba * exported_block_size;
    
    /* The last edge offset is the offset from the
     * start of the edge descriptor to the start of the last edge (the
     * start of the last edge is also the end of the io).
     * This can be calculated by (C) - (D) in the diagram below.
     *
     *     Edge 
     * (D) descriptor start      (C) - End of the io.
     * \|/                       \|/
     *  |------------|ioioioioioio|------------| <--- Exported
     *            |----------|----------|        <--- Imported
     *                           /|\                   
     *                           (C) - Last edge start.
     */
    if (((lba + blocks) % exported_opt_block_size) == 0)
    {
        /* End is aligned, no pre-read descriptor data needed.
         */
        *last_edge_offset_p = 0;
    }
    else if ( last_edge_byte >= pre_read_desc_start_byte &&
              (last_edge_byte - pre_read_desc_start_byte) < FBE_U32_MAX)
                                                       
    {
        *last_edge_offset_p = (fbe_u32_t)(last_edge_byte - pre_read_desc_start_byte);
    }
    else
    {
        /* Error, the end edge needs to be in this edge descriptor.
         * Or we received an overflow for a 32 bit integer.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************  
 * end fbe_ldo_map_get_pre_read_desc_offsets()
 **************************************/

/***************************************************************
 * @fn fbe_ldo_calculate_pad_bytes(fbe_logical_drive_io_cmd_t *io_cmd_p)
 ****************************************************************
 * @brief
 *  This function calculates the pad bytes needed.   
 *   
 *  Padding is needed in cases where the imported optimal block size does  
 * not equal the exported optimal block size. 
 *  
 * For example if we map 2 520 byte blocks onto 3 512 byte blocks, we 
 * end up with the below. 
 *  
 *         Padding needed for these areas.  
 *                     |------|                   |------| 
 *                    \|/    \|/                 \|/    \|/
 * |---------|---------| pad  |---------|---------| pad  | Exported(520)
 * |--------|--------|--------|--------|--------|--------| Imported(512)
 *
 * @param io_cmd_p - The io cmd to calculate pad bytes for.
 *
 * @return fbe_u32_t - the pad bytes needed. 
 *
 * HISTORY:
 *  6/17/2008 - Created. RPF
 *
 ****************************************************************/

fbe_u32_t fbe_ldo_calculate_pad_bytes(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
    fbe_logical_drive_t *const logical_drive_p = fbe_ldo_io_cmd_get_logical_drive(io_cmd_p);
    fbe_block_size_t exported_block_size = fbe_ldo_io_cmd_get_block_size(io_cmd_p);
    fbe_block_size_t exported_opt_blk_size = fbe_ldo_io_cmd_get_optimum_block_size(io_cmd_p);
    fbe_block_size_t imported_opt_blk_size = fbe_ldo_io_cmd_get_imported_optimum_block_size(io_cmd_p);
    fbe_block_size_t imported_block_size = fbe_ldo_get_imported_block_size(logical_drive_p);

    /* The padding needed is simply the difference between the exported and 
     * imported sizes. 
     */
    fbe_u32_t pad_bytes = (fbe_u32_t)((imported_opt_blk_size * imported_block_size) - 
                                      (exported_opt_blk_size * exported_block_size));

    return pad_bytes;
}
/******************************************
 * end fbe_ldo_calculate_pad_bytes()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_is_padding_required(   
 *                            fbe_block_size_t exported_block_size,   
 *                            fbe_block_size_t exported_opt_block_size,
 *                            fbe_block_size_t imported_block_size,
 *                            fbe_block_size_t imported_opt_block_size)
 ****************************************************************
 * @brief
 *  This function determines if padding is needed for a given command.
 * 
 * @param exported_block_size - The size in bytes of the exported block.
 * @param exported_opt_block_size - The size of exported optimal block the in
 *                                  terms of exported blocks.
 * @param imported_block_size - The size in bytes of the imported block.
 * @param imported_opt_block_size - The size in blocks of the
 *                                  imported optimal block.
 *
 * @return fbe_bool_t - FBE_TRUE if padding needed FBE_FALSE otherwise.
 *
 * HISTORY:
 *  9/12/2008 - Created. RPF
 *
 ****************************************************************/

fbe_bool_t
fbe_ldo_is_padding_required(fbe_block_size_t exported_block_size,
                            fbe_block_size_t exported_opt_block_size,
                            fbe_block_size_t imported_block_size,
                            fbe_block_size_t imported_opt_block_size)
{
    /* Padding is needed in cases where the imported optimal block size does 
     * not equal the exported optimal block size. 
     *  
     * For example if we map 2 520 byte blocks onto 3 512 byte blocks, we 
     * end up with the below. 
     *  
     *         Padding needed for these areas.  
     *                     |------|                   |------| 
     *                    \|/    \|/                 \|/    \|/
     * |---------|---------| pad  |---------|---------| pad  | Exported(520) 
     * |--------|--------|--------|--------|--------|--------| Imported(512)
     */
    if ( (exported_block_size * exported_opt_block_size) ==
         (imported_block_size * imported_opt_block_size) )
    {
        /* If our imported optimal block size equals our exported optimal block size,
         * then we will not require padding.
         */
        return FBE_FALSE;
    }
    else
    {
        /* Padding will be needed since the exported optimal block size (in bytes) and 
         * imported optimal block size (in bytes) are different. 
         */
        return FBE_TRUE;
    }
}
/**************************************
 * end fbe_ldo_is_padding_required()
 **************************************/

/*!**************************************************************
 * @fn fbe_ldo_io_cmd_is_padding_required(
 *                          fbe_logical_drive_io_cmd_t *const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function determines if padding is needed for a given command.
 * 
 * @param io_cmd_p - The io cmd to calculate pad bytes for.
 *
 * @return fbe_bool_t - FBE_TRUE if padding needed FBE_FALSE otherwise.
 *
 * HISTORY:
 *  9/12/2008 - Created. RPF
 *
 ****************************************************************/

fbe_bool_t
fbe_ldo_io_cmd_is_padding_required(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
    fbe_logical_drive_t *const logical_drive_p = fbe_ldo_io_cmd_get_logical_drive(io_cmd_p);
    fbe_bool_t b_padding_required;

    b_padding_required = 
        fbe_ldo_is_padding_required( fbe_ldo_io_cmd_get_block_size(io_cmd_p),
                                     fbe_ldo_io_cmd_get_optimum_block_size(io_cmd_p),
                                     fbe_ldo_get_imported_block_size(logical_drive_p),
                                     fbe_ldo_io_cmd_get_imported_optimum_block_size(io_cmd_p) );
    return b_padding_required;
}
/**************************************
 * end fbe_ldo_io_cmd_is_padding_required()
 **************************************/

/*!**************************************************************
 * @fn fbe_ldo_calculate_imported_optimum_block_size(
 *                       fbe_block_size_t exported_block_size,
 *                       fbe_block_size_t exported_optimum_block_size,
 *                       fbe_block_size_t imported_block_size)
 ****************************************************************
 * @brief
 *  Calculate the imported optimal block size given a set of
 *  input block sizes.
 *
 * @param exported_block_size - The size of the exported block in bytes.
 * @param exported_optimum_block_size - The size of the exported optimum block
 *                                      in exported blocks.
 * @param imported_block_size - The size of the imported block in bytes.
 *
 * @return - calculated imported optimum block size in blocks.
 *
 * HISTORY:
 *  7/24/2008 - Created. RPF
 *
 ****************************************************************/

fbe_block_size_t 
fbe_ldo_calculate_imported_optimum_block_size(fbe_block_size_t exported_block_size,
                                              fbe_block_size_t exported_optimum_block_size,
                                              fbe_block_size_t imported_block_size)
{
    fbe_block_size_t imported_optimum_block_size;

    /* First calculate the whole number of imported blocks per optimum
     * block. 
     */
    imported_optimum_block_size = 
        (exported_optimum_block_size * exported_block_size) / imported_block_size;

    /* Add on one for any remainder.
     */
    if ((exported_optimum_block_size * exported_block_size) % imported_block_size)
    {
        imported_optimum_block_size++;
    }
    return imported_optimum_block_size;
}
/******************************************
 * end fbe_ldo_calculate_imported_optimum_block_size()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_calculate_exported_optimum_block_size(
 *                       fbe_block_size_t requested_block_size,
 *                       fbe_block_size_t requested_optimum_block_size,
 *                       fbe_block_size_t imported_block_size)
 ****************************************************************
 * @brief
 *  Calculate the exported optimal block size given a requested block size and
 *  optimum block block size for an imported block size.
 * 
 *  The main purpose of this function is to minimize loss.  If someone requests
 *  an exported optimal block size with excessive loss of more than one exported
 *  block, then we will round up the exported optimum block size to minimize the
 *  loss.
 *
 * @param requested_block_size - The size of the desired exported block in
 *                               bytes.
 * @param requested_optimum_block_size - The size of the desired exported
 *                                       optimum block in exported blocks.
 * @param imported_block_size - The size of the imported block in bytes.
 *
 * @return - Exported optimum block size in blocks.
 *
 * HISTORY:
 *  7/24/2008 - Created. RPF
 *
 ****************************************************************/

fbe_block_size_t 
fbe_ldo_calculate_exported_optimum_block_size(fbe_block_size_t requested_block_size,
                                              fbe_block_size_t requested_optimum_block_size,
                                              fbe_block_size_t imported_block_size)
{
    fbe_block_size_t exported_optimum_block_size; /* Calculated value to be returned. */
    fbe_u32_t bytes_in_requested_opt_block;
    fbe_u32_t leftover_bytes;
    fbe_u32_t additional_blocks;

    /* Make sure that the amount lost is less than a full exported 
     * block, otherwise we are wasting too much.
     * The goal is to minimize loss.
     * First, calculate the amount leftover in the block.
     *
     * |---|---|---|             <-  Exported block
     * |----------------------|  <- Imported block
     *            /|\        /|\
     *             |__________|
     *           Amount leftover in physical block.
     *  If we divide this amount by the exported block size,
     *  we will end up with the number of blocks to add on to the
     *  requested optimal block size in order to minimize wasted space.
     */
    bytes_in_requested_opt_block = (requested_optimum_block_size * requested_block_size);
    leftover_bytes = imported_block_size - (bytes_in_requested_opt_block % imported_block_size);
    
    /* Determine how many exported blocks fit into this leftover space. 
     * This is the amount to add on to the requested optimum size. 
     */
    additional_blocks = leftover_bytes / requested_block_size;

    /* Calculate the value to return.  This starts off with the requested, and 
     * adds on the additional caculated above. 
     */
    exported_optimum_block_size = requested_optimum_block_size;
    exported_optimum_block_size += additional_blocks;

    return exported_optimum_block_size;
}
/******************************************
 * end fbe_ldo_calculate_exported_optimum_block_size()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_io_is_aligned(fbe_lba_t lba,
 *                           fbe_block_count_t blocks,
 *                           fbe_block_size_t opt_blk_size)
 ****************************************************************
 * @brief
 *  This function determines if the command is aligned
 *  and does not have edges.
 *
 * @param lba - The starting logical block address.
 * @param blocks - The number of blocks in the request.
 * @param opt_blk_size - The optimal block size in blocks.
 *
 * @return fbe_bool_t
 *  TRUE if aligned to optimal block.
 *  FALSE if not aligned to optimal block.
 *
 * HISTORY:
 *  11/08/07 - Created. RPF
 *
 ****************************************************************/
fbe_bool_t fbe_ldo_io_is_aligned(fbe_lba_t lba,
                                 fbe_block_count_t blocks,
                                 fbe_block_size_t opt_blk_size)
{
    /* Assume it will not be aligned and initialize to FALSE.
     */
    fbe_bool_t b_aligned = FBE_FALSE;
    
    /* If the start and end of the command is aligned to the
     * optimal block size, then we are considered aligned.
     */
    if ( (lba % opt_blk_size) == 0 &&
         ((lba + blocks) % opt_blk_size) == 0)
    {
        b_aligned = FBE_TRUE;
    }
    return b_aligned;
}
/* end fbe_ldo_io_is_aligned() */

/*!***************************************************************
 * @fn fbe_ldo_validate_pre_read_desc(
 *                 fbe_payload_pre_read_descriptor_t * const pre_read_desc_p,
 *                 fbe_block_size_t exported_block_size,
 *                 fbe_block_size_t exported_opt_block_size,
 *                 fbe_block_size_t imported_block_size,
 *                 fbe_block_size_t imported_opt_block_size,
 *                 fbe_lba_t exported_lba,
 *                 fbe_block_count_t exported_blocks,
 *                 fbe_lba_t imported_lba,
 *                 fbe_block_count_t imported_blocks)
 ****************************************************************
 * @brief
 *  This function determines if an edge descriptor is valid
 *  for the given io cmd.
 *
 * @param pre_read_desc_p - The pointer to the edge desc to validate.
 * @param exported_block_size - The block size in bytes to export to 
 *                              the higher level.
 * @param exported_opt_block_size - The exported optimum block size in blocks.
 * @param imported_block_size - The block size exported by lower level.
 * @param imported_opt_block_size - The imported optimum block size in blocks.
 * @param exported_lba - The start lba in terms of exported blocks.
 * @param exported_blocks - The block count in exported blocks.
 * @param imported_lba - The start lba in terms of imported blocks.
 * @param imported_blocks - The block count in imported blocks.
 *
 * @return fbe_status_t
 *  FBE_STATUS_OK on success.  Other values imply error.
 *
 * HISTORY:
 *  11/21/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
fbe_ldo_validate_pre_read_desc(fbe_payload_pre_read_descriptor_t * const pre_read_desc_p,
                                            fbe_block_size_t exported_block_size,
                                            fbe_block_size_t exported_opt_block_size,
                                            fbe_block_size_t imported_block_size,
                                            fbe_block_size_t imported_opt_block_size,
                                            fbe_lba_t exported_lba,
                                            fbe_block_count_t exported_blocks,
                                            fbe_lba_t imported_lba,
                                            fbe_block_count_t imported_blocks)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t pre_read_desc_lba;
    fbe_block_count_t pre_read_desc_blocks;
    fbe_sg_element_t * pre_read_desc_sg_p;
    fbe_bool_t b_start_aligned;
    fbe_bool_t b_end_aligned;

    b_start_aligned = ((imported_lba % imported_opt_block_size) == 0);
    b_end_aligned = ((imported_lba + imported_blocks) % imported_opt_block_size == 0);

    if (pre_read_desc_p == NULL)
    {
        /* We should have an edge descriptor but it is not available.
         */
        fbe_topology_class_trace(FBE_CLASS_ID_LOGICAL_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "logical drive: Edge desc ptr is null.  %s\n", __FUNCTION__);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* We know the edge descriptor is good, get the values from the
     * descriptor.
     */
    fbe_payload_pre_read_descriptor_get_lba(pre_read_desc_p, &pre_read_desc_lba);
    fbe_payload_pre_read_descriptor_get_block_count(pre_read_desc_p, &pre_read_desc_blocks);
    fbe_payload_pre_read_descriptor_get_sg_list(pre_read_desc_p, &pre_read_desc_sg_p);

    /* Check the edge descriptor values.
     */
    if (pre_read_desc_blocks == 0)
    {
        /* The edge descriptor indicates that the number of blocks is zero.
         * A valid edge descriptor must have a non-zero block count.
         */ 
        status = FBE_STATUS_GENERIC_FAILURE;

        fbe_topology_class_trace(FBE_CLASS_ID_LOGICAL_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "logical drive: Edge desc blocks is zero. 0x%p %s \n", 
                                 pre_read_desc_p, __FUNCTION__);
    }
    /* Check the exported blocks.
     */
    else if (exported_blocks == 0)
    {
        /* If the exported blocks is zero something is wrong.
         */ 
        status = FBE_STATUS_GENERIC_FAILURE;

        fbe_topology_class_trace(FBE_CLASS_ID_LOGICAL_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "logical drive: exported blocks is zero. 0x%p %s \n", 
                                 pre_read_desc_p, __FUNCTION__);
    }
    else if (pre_read_desc_sg_p == NULL)
    {
        /* The edge descriptor indicates that there is no sg.
         * A valid edge descriptor must have a valid SG.
         */ 
        status = FBE_STATUS_GENERIC_FAILURE;

        fbe_topology_class_trace(FBE_CLASS_ID_LOGICAL_DRIVE,
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "LDO: Edge desc sg ptr is null. 0x%p %s \n", 
                                 pre_read_desc_p, __FUNCTION__);
    }
    else
    {
        /* Next validate that the range of the edge descriptor matches our
         * imported lba and blocks.
         */
        fbe_lba_t pre_read_desc_start_opt_blk = pre_read_desc_lba / exported_opt_block_size;
        fbe_lba_t pre_read_desc_start_opt_blk_offset = 
            (pre_read_desc_lba % exported_opt_block_size) * exported_block_size;
        fbe_lba_t pre_read_desc_end_lba = pre_read_desc_lba + pre_read_desc_blocks - 1;
        fbe_lba_t pre_read_desc_end_opt_blk = pre_read_desc_end_lba /exported_opt_block_size;
        fbe_lba_t pre_read_desc_end_opt_blk_offset = 
            (pre_read_desc_end_lba % exported_opt_block_size) * exported_block_size;

        fbe_lba_t imported_block_start_opt_blk;
        fbe_lba_t imported_block_end_opt_blk;
        fbe_lba_t imported_block_start_opt_blk_offset;
        fbe_lba_t imported_block_end_opt_blk_offset;
        fbe_lba_t imported_block_end_lba = imported_lba + imported_blocks - 1;

        pre_read_desc_end_opt_blk_offset += exported_block_size - 1;

        /* Next determine the byte where the I/O starts and total bytes in I/O.
         */
        imported_block_start_opt_blk = imported_lba / imported_opt_block_size;
        imported_block_start_opt_blk_offset = 
            (imported_lba % imported_opt_block_size) * imported_block_size;
        imported_block_end_opt_blk = imported_block_end_lba / imported_opt_block_size;
        imported_block_end_opt_blk_offset = 
            (imported_block_end_lba % imported_opt_block_size) * imported_block_size;
        imported_block_end_opt_blk_offset += imported_block_size - 1;

        /* If our imported start (A) is less than our client supplied 
         * edge start (B), then the edge descriptor does not contain 
         * the entire start edge.
         *
         * Similarly if our imported end (C) is greater than our client 
         * supplied edge end (D), then the edge descriptor does not 
         * contain the entire end edge.
         *                                            Client supplied
         * (B)-Client supplied edge start         (D) edge end
         * \|/                                    \|/ 
         *  |------------|ioioioioioio|------------| <--- Exported
         *            |----------|----------|        <--- Imported
         *           /|\                   /|\                   
         *           (A)-Imported start    (C)-Imported end
         * 
         * Make sure that the optimal block for the edge start is less than
         * or equal to the imported start.  Similarly make sure that if both are
         * in the same optimal block then the offset of the edge start is less
         * than or equal to the imported block start.
         */
        if ( !b_start_aligned &&
             (pre_read_desc_start_opt_blk > imported_block_start_opt_blk ||
              (pre_read_desc_start_opt_blk == imported_block_start_opt_blk &&
               pre_read_desc_start_opt_blk_offset > imported_block_start_opt_blk_offset)) )
        {

            status = FBE_STATUS_GENERIC_FAILURE;

            fbe_topology_class_trace(FBE_CLASS_ID_LOGICAL_DRIVE,
                                     FBE_TRACE_LEVEL_WARNING, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "LDO: Edge Descriptor too small at beginning."
                                     "  0x%p 0x%llx 0x%llx 0x%llx 0x%llx \n", 
                                     (char *)pre_read_desc_p, 
                                     (unsigned long long)pre_read_desc_p->lba,
				     (unsigned long long)pre_read_desc_p->block_count,
                                     (unsigned long long)exported_lba,
				     (unsigned long long)exported_blocks);
        }

        /* If the start is aligned, but the end is not, make sure the pre-read
         * data starts immediately following the last block in the transfer.
         * This will give us enough exported blocks to contain the needed
         * pre-read data.
         *
         * Pre read must start from            Pre read must end at or after
         * at or before this point    |            | this point.
         * to contain enough trailing |
         * edge data.                 |
         *                           \|/          \|/ 
         *  |------------|ioioioioioio|------------| <--- Exported
         *  |----------|----------|----------|        <--- Imported
         *           /|\                   /|\                   
         *           (A)-Imported start    (C)-Imported end
         */
        if ( b_start_aligned && !b_end_aligned &&
             (pre_read_desc_p->lba > exported_lba + exported_blocks) )
        {
            status = FBE_STATUS_GENERIC_FAILURE;

            fbe_topology_class_trace(FBE_CLASS_ID_LOGICAL_DRIVE,
                                     FBE_TRACE_LEVEL_WARNING, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "LDO: Edge Descriptor start aligned too small at beginning."
                                     "  0x%p 0x%llx 0x%llx 0x%llx 0x%llx \n", 
                                     (char *)pre_read_desc_p,
                                     (unsigned long long)pre_read_desc_p->lba,
				     (unsigned long long)pre_read_desc_p->block_count,
                                     (unsigned long long)exported_lba,
				     (unsigned long long)exported_blocks);
        }

        /*
         * Make sure that the optimal block for the end edge is greater than or 
         * equal to the imported end.  Similarly make sure that if both are in
         * the same optimal block then the offset of the edge end is greater 
         * than or equal to the imported block end. 
         * If we are in the same optimal block, make sure that we either have an 
         * offset 
         */
        if ( !b_end_aligned &&
             (pre_read_desc_end_opt_blk < imported_block_end_opt_blk ||
              (pre_read_desc_end_opt_blk == imported_block_end_opt_blk &&
               pre_read_desc_end_opt_blk_offset < imported_block_end_opt_blk_offset &&
               ((pre_read_desc_end_lba + 1) % exported_opt_block_size) != 0)) )
        {

            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_topology_class_trace(FBE_CLASS_ID_LOGICAL_DRIVE,
                                     FBE_TRACE_LEVEL_WARNING, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "LDO: Edge Descriptor too small at end."
                                     "  0x%p 0x%llx 0x%llx 0x%llx 0x%llx \n", 
                                     (char *)pre_read_desc_p, 
                                     (unsigned long long)pre_read_desc_p->lba,
				     (unsigned long long)pre_read_desc_p->block_count,
                                     (unsigned long long)exported_lba,
				     (unsigned long long)exported_blocks);
        }

        /* If the end is aligned, but the start is not, make sure the pre-read
         * data starts immediately following the last block in the transfer.
         * This will give us enough exported blocks to contain the needed
         * pre-read data.
         *
         * Pre read must end at or beyond this point in order to contain
         *                       | enough leading edge data.
         * Pre-read |            | 
         * start   \|/          \|/
         *          |------------|ioioioioioio|------------| <--- Exported
         *                   |---------|---------|---------| <--- Imported
         *                  /|\                 /|\                   
         *                 (A)-Imported start    (C)-Imported end
         */
        if ( !b_start_aligned && b_end_aligned &&
             (pre_read_desc_p->lba + pre_read_desc_p->block_count) < exported_lba)
        {
            status = FBE_STATUS_GENERIC_FAILURE;

            fbe_topology_class_trace(FBE_CLASS_ID_LOGICAL_DRIVE,
                                     FBE_TRACE_LEVEL_WARNING, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "LDO: Edge Descriptor end aligned too small at end."
                                     "  0x%p 0x%llx 0x%llx 0x%llx 0x%llx\n", 
                                     (char *)pre_read_desc_p,
                                     (unsigned long long)pre_read_desc_p->lba,
				     (unsigned long long)pre_read_desc_p->block_count,
                                     (unsigned long long)exported_lba,
				     (unsigned long long)exported_blocks);
        }
    }    /* end else check edge descriptor against I/O. */

    return status;
}
/* end fbe_ldo_validate_pre_read_desc() */

/*******************************
 * end fbe_logical_drive_map.c
 ******************************/
