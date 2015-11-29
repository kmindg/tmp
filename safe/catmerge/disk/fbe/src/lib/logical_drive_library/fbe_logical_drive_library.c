/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_library.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the logical drive library.
 * 
 *  This is code which is exported to clients so they can perform calculations
 *  to determine the sizes of pre-reads.
 * 
 *  In order to access this library, please include fbe_logical_drive.h.
 * 
 *  In order to link with this library, please add
 *  fbe_logical_drive_library.lib to your SOURCES.mak.
 *
 * @ingroup logical_drive_class_files
 * @ingroup logical_drive_library
 * 
 * HISTORY
 *   8/14/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_logical_drive.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_lba_t fbe_ldo_map_exported_end_lba_to_imported_lba(fbe_lba_t lba,
                                                              fbe_block_size_t exported_block_size,
                                                              fbe_block_size_t exported_opt_block_size,
                                                              fbe_block_size_t imported_block_size,
                                                              fbe_block_size_t imported_opt_block_size,
                                                              fbe_lba_t physical_offset);
static fbe_lba_t fbe_ldo_map_exported_lba_to_imported_lba(fbe_lba_t lba,
                                                          fbe_block_size_t exported_block_size,
                                                          fbe_block_size_t exported_opt_block_size,
                                                          fbe_block_size_t imported_block_size,
                                                          fbe_block_size_t imported_opt_block_size,
                                                          fbe_lba_t physical_offset);
/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!***************************************************************
 * @fn fbe_logical_drive_get_pre_read_lba_blocks_for_write(
 *                                  fbe_lba_t exported_block_size,
 *                                  fbe_lba_t exported_opt_block_size,
 *                                  fbe_lba_t imported_block_size,
 *                                  fbe_lba_t *const lba_p,
 *                                  fbe_block_count_t *const blocks_p)
 ****************************************************************
 * @brief
 *  This function determines how many blocks need to be pre-read
 *  by a client for a given write operation.
 * 
 *  The necessary pre-read range is returned in the lba_p and blocks_p.
 *
 * @param exported_block_size - The exported block size in bytes.
 * @param exported_opt_block_size - The exported optimal block size
 *                                 in bytes.
 * @param imported_block_size - The imported block size in bytes.
 * @param lba_p - the lba of the operation to determine pre
 *              read lba/blocks for.
 * @param blocks_p - The blocks of the operation to determine pre
 *                 read lba/blocks for.
 *
 * @return None.
 *
 * HISTORY:
 *  11/27/07 - Created. RPF
 *
 ****************************************************************/
void 
fbe_logical_drive_get_pre_read_lba_blocks_for_write( fbe_lba_t exported_block_size,
                                                     fbe_lba_t exported_opt_block_size,
                                                     fbe_lba_t imported_block_size,
                                                     fbe_lba_t *const lba_p,
                                                     fbe_block_count_t *const blocks_p )
{
    fbe_lba_t imported_lba;
    fbe_block_count_t imported_blocks;
    fbe_lba_t imported_opt_block_size;

    fbe_bool_t b_start_aligned = (*lba_p % exported_opt_block_size) == 0;
    fbe_bool_t b_end_aligned = ((*lba_p + *blocks_p) % exported_opt_block_size) == 0;

    /* Derive the optimum block size. 
     */
    imported_opt_block_size = ( (exported_block_size * exported_opt_block_size) /
                                imported_block_size );

    if ((exported_block_size * exported_opt_block_size) % imported_block_size)
    {
        /* There is a remainder, so add one onto the imported block size.
         */
        imported_opt_block_size++;
    }

    /* Start and end are aligned. Just return the same as was passed in.
     */
    if (b_start_aligned && b_end_aligned)
    {
        return;
    }

    /* Only start is unaligned, just assume we need pre-read data for the start
     * of the request. 
     */
    if (!b_start_aligned && b_end_aligned)
    {
        *blocks_p = 1;
    }
    /* Only end is unaligned.  Just assume we need pre-read data for the end
     * block. 
     */
    if (b_start_aligned && !b_end_aligned)
    {
        *lba_p = *lba_p + *blocks_p - 1;
        *blocks_p = 1;
    }
    /* Map from the exported lba to the imported lba.
     *  Assume that **** is the I/O we need to do
     *  
     * |--------------|*************|*************|-------------| 520 exported
     * |------------|--------------|--------------|--------------| 512 imported 
     *             /|\    This is the imported block range (3)  /|\
     *              |____________________________________________|
     *              |Start lba for imported block size
     */
    fbe_ldo_map_lba_blocks(*lba_p,
                           *blocks_p,
                           (fbe_block_size_t)exported_block_size,
                           (fbe_block_size_t)exported_opt_block_size,
                           (fbe_block_size_t)imported_block_size,
                           (fbe_block_size_t)imported_opt_block_size,
                           &imported_lba,
                           &imported_blocks);

    /* Now map the reverse, map the imported lba (A)
     * back onto the exported lba to find out how much we need to 
     * pre-read. *** indicates exported I/O 
     *                         
     *  __________________________pre-read size_________________
     *  |Exported lba                                          |
     * \|/                                                    \|/
     *  |------------|*************|*************|-------------| 520 exported
     *  |-----------|-------------|-------------|-------------| 512 imported
     *             /|\    
     *              | (A) Imported block lba
     */
    fbe_ldo_map_lba_blocks(imported_lba,
                           imported_blocks,
                           (fbe_block_size_t)imported_block_size,
                           (fbe_block_size_t)imported_opt_block_size,
                           (fbe_block_size_t)exported_block_size,
                           (fbe_block_size_t)exported_opt_block_size,
                           lba_p,
                           blocks_p);
    return; 
}
/* end fbe_logical_drive_get_pre_read_lba_blocks_for_write */

/*!***************************************************************
 * @fn fbe_ldo_map_lba_blocks(fbe_lba_t lba,
 *                            fbe_block_count_t blocks,
 *                            fbe_block_size_t exported_block_size,
 *                            fbe_block_size_t exported_opt_block_size,
 *                            fbe_block_size_t imported_block_size,
 *                            fbe_block_size_t imported_opt_block_size,
 *                            fbe_lba_t *const output_lba_p,
 *                            fbe_block_count_t *const output_blocks_p)
 ****************************************************************
 * @brief
 *  Map the lba and block count for the exported block size into an lba and
 *  block count in the imported block size.
 * 
 *  We return the imported lba and imported block count that the exported lba
 *  and block count map to.  So really the input is a range and the output is a
 *  range also.
 *
 * PARAMETERS:
 * @param lba - Logical block address to map.
 * @param blocks - Block count to map.
 * @param exported_block_size - Size of exported block in bytes.
 * @param exported_opt_block_size - Number of exported blocks per optimal size.
 * @param imported_block_size - Size of imported block in bytes. 
 * @param imported_opt_block_size - Number of imported blocks per optimal size.
 * @param output_lba_p - Pointer to the lba being returned.
 * @param output_blocks_p - Pointer to the blocks being returned.
 *
 * @return NONE.
 *
 * HISTORY:
 *  11/07/07 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_map_lba_blocks(fbe_lba_t lba,
                            fbe_block_count_t blocks,
                            fbe_block_size_t exported_block_size,
                            fbe_block_size_t exported_opt_block_size,
                            fbe_block_size_t imported_block_size,
                            fbe_block_size_t imported_opt_block_size,
                            fbe_lba_t *const output_lba_p,
                            fbe_block_count_t *const output_blocks_p)
{
    const fbe_lba_t offset = 0;
    fbe_lba_t imported_start_lba;
    fbe_block_count_t imported_blocks;
    fbe_lba_t imported_end_lba;

    imported_start_lba = 
        fbe_ldo_map_exported_lba_to_imported_lba(lba, 
                                                exported_block_size, 
                                                exported_opt_block_size, 
                                                imported_block_size, 
                                                imported_opt_block_size, 
                                                offset);
    
    /* Determine the size of the imported io.
     * This is the size of the io which includes underlying edges.
     *
     *             |ioioioioioioioioioio|--------------------|     <--- Exported
     *  |--------------------|--------------------|-------------   <--- Imported
     * /|\                                       /|\                   
     *  |_________________________________________|                    
     *      The imported io size includes the edges.
     *
     * First determine the last imported block which this I/O hits by
     * calculating the imported block that contains the last byte of the exported block.
     */
    imported_end_lba = 
        fbe_ldo_map_exported_end_lba_to_imported_lba(lba + blocks - 1, 
                                                 exported_block_size, 
                                                 exported_opt_block_size, 
                                                 imported_block_size, 
                                                 imported_opt_block_size, 
                                                 offset);

    /* Next determine how many imported blocks by calculating the difference 
     * between imported and exported lbas.
     */
    imported_blocks = (fbe_block_count_t)(imported_end_lba - imported_start_lba + 1);

    /* Finally setup our return values.
     */
    *output_lba_p = imported_start_lba;
    *output_blocks_p = imported_blocks;
    return;
}
/* end fbe_ldo_map_lba_blocks() */

/*!***************************************************************
 * @fn fbe_ldo_map_exported_lba_to_imported_lba(
 *                         fbe_lba_t lba,
 *                         fbe_block_size_t exported_block_size,
 *                         fbe_block_size_t exported_opt_block_size,
 *                         fbe_block_size_t imported_block_size,
 *                         fbe_block_size_t imported_opt_block_size,
 *                         fbe_lba_t physical_offset)
 ****************************************************************
 * @brief
 *  Map an lba from a given exported block size to a given imported block size.
 * 
 * @param lba - Logical block address to map.
 * @param exported_block_size - Size of exported block in bytes.
 * @param exported_opt_block_size - Number of exported blocks per optimal size.
 * @param imported_block_size - Size of imported block in bytes.
 * @param imported_opt_block_size - Number of imported blocks per optimal size.
 * @param physical_offset - Offset in bytes being imposed at the physical level.
 *
 * @return - None.
 *
 * HISTORY:
 *  05/13/08 - Created. RPF
 *
 ****************************************************************/
fbe_lba_t 
fbe_ldo_map_exported_lba_to_imported_lba(fbe_lba_t lba,
                                        fbe_block_size_t exported_block_size,
                                        fbe_block_size_t exported_opt_block_size,
                                        fbe_block_size_t imported_block_size,
                                        fbe_block_size_t imported_opt_block_size,
                                        fbe_lba_t physical_offset)
{
    fbe_lba_t optimal_block_number;
    fbe_lba_t byte_offset_from_opt_block;
    fbe_lba_t imported_lba;

    /* Get the optimal block number this LBA is in.
     */
    optimal_block_number = lba / exported_opt_block_size;

    byte_offset_from_opt_block = (lba % exported_opt_block_size) * exported_block_size;

    /* The physical offset is the byte offset from the start of the drive
     * where exported blocks start.
     *  
     *   physical
     *  | offset    |  Exported Block 0  |  Exported Block 1  |
     *  |-----------|--------------------|--------------------| logical (exported)
     *  |---------------|---------------|---------------| physical (imported)
     * /|\   Block 0    |  Block 1      |    Block 2    | 
     *  | Start of drive
     */
    imported_lba = (optimal_block_number * imported_opt_block_size);
    imported_lba += (physical_offset + byte_offset_from_opt_block) / imported_block_size;

    return imported_lba;
}
/**************************************
 * end fbe_ldo_map_exported_lba_to_imported_lba() 
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_map_exported_end_lba_to_imported_lba(
 *                  fbe_lba_t lba,
 *                  fbe_block_size_t exported_block_size,
 *                  fbe_block_size_t exported_opt_block_size,
 *                  fbe_block_size_t imported_block_size,
 *                  fbe_block_size_t imported_opt_block_size,
 *                  fbe_lba_t physical_offset)
 ****************************************************************
 * @brief
 *  Map a given exported end lba with a given exported block size into an
 *  imported block size with a given imported block size.
 * 
 * @param lba - Logical block address to map.
 * @param exported_block_size - Size of exported block in bytes.
 * @param exported_opt_block_size - Number of exported blocks per optimal size.
 * @param imported_block_size - Size of imported block in bytes.
 * @param imported_opt_block_size - Number of imported blocks per optimal size.
 * @param physical_offset - Offset in bytes being imposed at the physical level.
 * 
 * @return - lba that we mapped
 *
 * HISTORY:
 *  05/13/08 - Created. RPF
 *
 ****************************************************************/
static fbe_lba_t 
fbe_ldo_map_exported_end_lba_to_imported_lba(fbe_lba_t lba,
                                             fbe_block_size_t exported_block_size,
                                             fbe_block_size_t exported_opt_block_size,
                                             fbe_block_size_t imported_block_size,
                                             fbe_block_size_t imported_opt_block_size,
                                             fbe_lba_t physical_offset)
{
    /* Get the optimal block number this LBA is in.
     */
    fbe_lba_t optimal_block_number = lba / exported_opt_block_size;

    fbe_lba_t byte_offset_from_opt_block = 0;
    fbe_lba_t imported_lba;

    /* Determine our offset from the optimal block to the beginning of this 
     * block. 
     */
    byte_offset_from_opt_block = (lba % exported_opt_block_size) * exported_block_size;

    if ( (byte_offset_from_opt_block + exported_block_size - 1) <=
         (imported_block_size * imported_opt_block_size) )
    {
        /* Add on the contents of this block so that we get an offset from the
         * optimal block to the last byte in this block. 
         */ 
        byte_offset_from_opt_block += exported_block_size - 1;
    }
    else if ((exported_block_size * exported_opt_block_size) >
             (imported_block_size * imported_opt_block_size))
    {
        /* In this case the exported size is greater than the imported size.
         * We always need to map to the end of the exported block.
         *  
         *                                         Map to here   | 
         *   physical                                           \|/
         *  | offset    |  Exported Block 0                      |
         *  |-----------|----------------------------------------|logical(exported)  
         *  |---------------|---------------|---------------| physical (imported)  
         * /|\   Block 0    |  Block 1      |    Block 2    | 
         *  | Start of drive
         */
         imported_lba = ((optimal_block_number + 1) * imported_opt_block_size) - 1;
         return imported_lba;
    }
    else
    {
        /* We don't add it on since we are in the "pad region",
         * and want to map to the offset as given 
         */
    }
    /*  Determine the lba at the start of this optimal block.
     */
    imported_lba = (optimal_block_number * imported_opt_block_size);

    /* Next, add on the offset to the end of this block and divide by imported
     * block size to determine how many underlying physical blocks there are. 
     *  
     * The physical offset is the byte offset from the start of the drive where 
     * exported blocks start. 
     *  
     *   physical
     *  | offset    |  Exported Block 0  |  Exported Block 1  |
     *  |-----------|--------------------|--------------------| logical (exported)
     *  |---------------|---------------|---------------| physical (imported)
     * /|\   Block 0    |  Block 1      |    Block 2    | 
     *  | Start of drive
     */  
    imported_lba += (physical_offset + byte_offset_from_opt_block) / imported_block_size;
    
    return imported_lba;
}
/**************************************
 * end fbe_ldo_map_exported_end_lba_to_imported_lba() 
 **************************************/

/*!***************************************************************
 * @fn fbe_logical_drive_get_pre_read_lba_blocks_for_write(
 *                                  fbe_lba_t media_error_pba,
 *                                  fbe_lba_t io_start_lba,
 *                                  fbe_lba_t exported_block_size,
 *                                  fbe_lba_t exported_opt_block_size,
 *                                  fbe_lba_t imported_block_size,
 *                                  fbe_block_size_t imported_opt_block_size)
 ****************************************************************
 * @brief
 *  Map a physical address we recevied a media error on
 *  to a logical address.
 * 
 * @param media_error_pba - The physical block where the media error
 *                          was reported.
 * @param io_start_lba - The start address of the io in terms of an
 *                       exported block.
 * @param exported_block_size - The exported block size in bytes.
 * @param exported_opt_block_size - The number of exported blocks in the
 *                                  optimum block size.
 * @param imported_block_size - The imported block size in bytes.
 * @param imported_opt_block_size - The number of imported blocks in the
 *                                  optimum block size.
 *
 * @return None.
 *
 * HISTORY:
 *  01/13/08 - Created. RPF
 *
 ****************************************************************/
fbe_lba_t
fbe_ldo_get_media_error_lba( fbe_lba_t media_error_pba,
                             fbe_lba_t io_start_lba,
                             fbe_block_size_t exported_block_size,
                             fbe_block_size_t exported_opt_block_size,
                             fbe_block_size_t imported_block_size,
                             fbe_block_size_t imported_opt_block_size)
{
    fbe_lba_t media_error_lba;
    /* If we encountered a media error, then map the media error lba 
     * from the imported block to the exported block.
     */
    media_error_lba = 
        fbe_ldo_map_exported_lba_to_imported_lba(media_error_pba, 
                                                 imported_block_size, 
                                                 imported_opt_block_size, 
                                                 exported_block_size, 
                                                 exported_opt_block_size, 
                                                 0);

    if (media_error_lba < io_start_lba)
    {
        /* We might have rounded back to the first block in error, which could be a 
         * prior exported block. Just set the media error lba to the first lba in the 
         * transfer in error. 
         * In the below example, the physical block in error is block 1. 
         * When we do the above math it rounds back to the first exported block in 
         * error, block 0, however we want to report the first block in the transfer 
         * in error, block 1.
         *    0            1
         * |------------|ioioioioioio| 
         * |----------|xxxxxxxxxx|----------| 
         *  
         */
        media_error_lba = io_start_lba;
    }
    return media_error_lba;
}
/**************************************
 * end fbe_ldo_get_media_error_lba()
 **************************************/

/*************************
 * end file fbe_logical_drive_library.c
 *************************/
