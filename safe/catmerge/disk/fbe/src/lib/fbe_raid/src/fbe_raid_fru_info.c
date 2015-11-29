/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_fru_info.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for the fru info structure.
 *
 * @version
 *   12/14/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_raid_fru_info_init()
 ****************************************************************
 * @brief
 *  Initialize the fru info structure.
 *
 * @param lba - logical block address
 * @param blocks - block count
 * @param position - Position in the array.
 *
 * @return fbe_status_t   
 *
 * @author
 *  10/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_fru_info_init(fbe_raid_fru_info_t *const fru_p,
                                    fbe_lba_t lba,
                                    fbe_block_count_t blocks,
                                    fbe_u32_t position )
{
    fru_p->lba = lba;
    fru_p->blocks = blocks;
    fru_p->position = position;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_fru_info_init()
 ******************************************/

/*!**************************************************************
 * fbe_raid_fru_info_init_arrays()
 ****************************************************************
 * @brief
 *  Initialize the fru info arrays
 *
 * @param width - The entire width of the raid group
 * @param read_info_p - Pointer to read info to initialize
 * @param write_info_p - Pointer to write info to initialize
 * @param read2_info_p - Pointer to read2 info to initialize
 *
 * @return fbe_status_t   
 *
 ****************************************************************/

fbe_status_t fbe_raid_fru_info_init_arrays(fbe_u32_t    width,
                                           fbe_raid_fru_info_t *read_info_p,
                                           fbe_raid_fru_info_t *write_info_p,
                                           fbe_raid_fru_info_t *read2_info_p)
{
    fbe_u32_t   index;

    /* If the array is valid initialize it
     */
    for (index = 0; index < width; index++)
    {
        if (read_info_p != NULL)
        {
            read_info_p->lba = FBE_LBA_INVALID;
            read_info_p->blocks = 0;
            read_info_p++;
        }
        if (write_info_p != NULL)
        {
            write_info_p->lba = FBE_LBA_INVALID;
            write_info_p->blocks = 0;
            write_info_p++;
        }
        if (read2_info_p != NULL)
        {
            read2_info_p->lba = FBE_LBA_INVALID;
            read2_info_p->blocks = 0;
            read2_info_p++;
        }
    }

    return(FBE_STATUS_OK);
}
/******************************************
 * end fbe_raid_fru_info_init_arrays()
 ******************************************/

/*!**************************************************************
 * fbe_raid_fru_info_set_sg_index()
 ****************************************************************
 * @brief
 *  Set the sg index field of the fru info.
 *  This validates and will log an error if needed.
 *
 * @param fru_info_p - fru info to set sg index for.
 * @param sg_count - count we are setting the index for.
 * @param b_log - TRUE to log if error, FALSE otherwise.
 *                Sometimes the error case is "normal" and
 *                does not need to be traced.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_raid_fru_info_set_sg_index(fbe_raid_fru_info_t *fru_info_p,
                                            fbe_u16_t sg_count,
                                            fbe_bool_t b_log)
{
    fbe_status_t        status = FBE_STATUS_OK; /* To allow testing of exceeded sgl limit */
    fbe_raid_sg_index_t sg_index;

    sg_index = fbe_raid_memory_sg_count_index(sg_count);

    /* Check if there is an error before we set the sg index.
     */
    if (RAID_COND(sg_index >= FBE_RAID_MAX_SG_TYPE))
    {
        if (b_log)
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "raid: sg index %d unexpected for pos %d sg_count: %d 0x%p \n", 
                                 fru_info_p->sg_index, fru_info_p->position, 
                                 sg_count, fru_info_p);
        }
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    /* OK to set the index.
     */
    fru_info_p->sg_index = sg_index;
    return status;
}
/******************************************
 * end fbe_raid_fru_info_set_sg_index()
 ******************************************/

/*!**************************************************************
 *          fbe_raid_fru_info_populate_sg_index()
 ****************************************************************
 *
 * @brief   A wrapper method that determines and populates a fru
 *          information structure with the proper sg_index value.
 *
 * @param   fru_info_p - fru info to set sg index for.
 * @param   page_size - required to determine max sg_index
 * @param   mem_left_p - Pointer to mem_left value to update
 * @param   b_log - TRUE to log if error, FALSE otherwise.
 *                Sometimes the error case is "normal" and
 *                does not need to be traced.
 * 
 * @return  status - FBE_STATUS_OK - read fruts information successfully generated
 *                   FBE_STATUS_INSUFFICIENT_RESOURCES - Request is too large
 *                   other - Unexpected failure
 *
 ****************************************************************/
fbe_status_t fbe_raid_fru_info_populate_sg_index(fbe_raid_fru_info_t *fru_info_p,
                                                 fbe_u32_t page_size,
                                                 fbe_block_count_t *mem_left_p,
                                                 fbe_bool_t b_log)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       sg_count = 0;
    
    /* Page size must be set.
     */
    if (!fbe_raid_memory_validate_page_size(page_size))
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Need to allocate an sgl since the original sgl
     * has been split into sub-requests.
     */
    *mem_left_p = fbe_raid_sg_count_uniform_blocks(fru_info_p->blocks,
                                                   page_size,
                                                   *mem_left_p,
                                                   &sg_count);

    /* Set sg index returns a status.  The reason is that the sg_count
     * may exceed the maximum sgl length.
     */
    status = fbe_raid_fru_info_set_sg_index(fru_info_p,
                                            sg_count,
                                            b_log);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_library_trace(FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                               "raid: %s failed to set sg index: status = 0x%x, fru_info_p = 0x%p, "
                               "sg_count = 0x%x\n", 
                               __FUNCTION__,
                               status,
                               fru_info_p,
                               sg_count);

        return status;
    }

    /* Always return the execution status.
     */
    return(status);
}
/******************************************
 * end fbe_raid_fru_info_populate_sg_index()
 ******************************************/

/*!**************************************************************
 * fbe_raid_fru_info_get_by_position()
 ****************************************************************
 * @brief
 *  Get the fru info ptr from an array by position in the array.
 *  This assumes there is a terminator at the end of the array.
 *  
 * @param info_array_p - array ptr
 * @param position - position to get
 * @param found_p - bool FBE_TRUE if found, FBE_FALSE otherwise
 * 
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_raid_fru_info_get_by_position(fbe_raid_fru_info_t *info_array_p,
                                               fbe_u32_t position,
                                               fbe_u32_t width,
                                               fbe_raid_fru_info_t **found_p)
{
    fbe_status_t         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_fru_info_t *current_info_p = info_array_p;
    fbe_u32_t i;
    *found_p = NULL;

    for (i = 0; i < width; i++)
    {
        if (current_info_p->lba != FBE_LBA_INVALID && current_info_p->position == position)
        {
            *found_p = current_info_p;
            status = FBE_STATUS_OK;
            break;
        }
        current_info_p++;
    }
    return status;
}
/******************************************
 * end fbe_raid_fru_info_get_by_position()
 ******************************************/

/*!**************************************************************************
 * fbe_raid_fru_info_allocate_sgs()
 ****************************************************************************
 * @brief  This function is called to allocate the specified number
 *         of fixed sized pages of memory.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param fruts_p - Pointer to fruts to plance sg into
 * @param memory_info_p - Pointer to memory request information
 * @param sg_type - Type of sg to allocate
 * 
 * @return none.
 *
 * @author
 *  9/16/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_raid_fru_info_allocate_sgs(fbe_raid_siots_t *siots_p,
                                            fbe_raid_fruts_t *fruts_p,
                                            fbe_raid_memory_info_t *memory_info_p,
                                            fbe_raid_memory_type_t sg_type)
{
    fbe_memory_id_t memory_p = NULL;
    fbe_u32_t bytes_to_allocate;
    fbe_u32_t bytes_allocated;

    bytes_to_allocate = fbe_raid_memory_type_get_size(sg_type);

    /* Get the next piece of memory from the current location in the memory queue.
     */
    memory_p = fbe_raid_memory_allocate_contiguous(memory_info_p,
                                                   bytes_to_allocate,
                                                   &bytes_allocated);
    if(RAID_COND(bytes_allocated != bytes_to_allocate))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots lba: 0x%llx blks: 0x%llx bytes_allocated: %d != bytes_to_allocate: %d \n",
                             (unsigned long long)siots_p->start_lba,
			     (unsigned long long)siots_p->xfer_count, 
                             bytes_allocated, bytes_to_allocate);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_memory_header_init(memory_p, sg_type);

    /* Plant this sg.
     */
    fruts_p->sg_id = memory_p;

    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_fru_info_allocate_sgs()
 *****************************************/

/*!**************************************************************************
 * fbe_raid_fru_info_array_allocate_sgs()
 ****************************************************************************
 * @brief  This function is called to allocate the specified number
 *         of fixed sized pages of memory.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param memory_info_p - Pointer to memory request information
 * @param read_info_p - Pointer to array of read information per position
 * @param read2_info_p - Pointer to array of read2 information per position
 * @param write_info_p - Pointer to array of write information per position
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  9/16/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_raid_fru_info_array_allocate_sgs(fbe_raid_siots_t *siots_p,
                                                  fbe_raid_memory_info_t *memory_info_p,
                                                  fbe_raid_fru_info_t *read_info_p,
                                                  fbe_raid_fru_info_t *read2_info_p,
                                                  fbe_raid_fru_info_t *write_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t sg_index;
    fbe_raid_fruts_t *read_p = NULL;
    fbe_raid_fruts_t *write_p = NULL;
    fbe_raid_fruts_t *read2_p = NULL;

    for (sg_index = 0; sg_index < FBE_RAID_MAX_SG_TYPE; sg_index++)
    {
        fbe_raid_memory_type_t type;
        fbe_raid_fru_info_t *current_read_info_p = read_info_p;
        fbe_raid_fru_info_t *current_read2_info_p = read2_info_p;
        fbe_raid_fru_info_t *current_write_info_p = write_info_p;

        type = fbe_raid_memory_get_sg_type_by_index(sg_index);

        fbe_raid_siots_get_read_fruts(siots_p, &read_p);
        fbe_raid_siots_get_read2_fruts(siots_p, &read2_p);
        fbe_raid_siots_get_write_fruts(siots_p, &write_p);

        /* Allocate all sgs for this type.
         */
        while ((read_p != NULL)&&(current_read_info_p != NULL))
        {
            if (current_read_info_p->sg_index == sg_index)
            {
                if (RAID_COND(read_p->position != current_read_info_p->position))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "raid: read_p->position 0x%x != current_read_info_p->position 0x%x, siots_p =0x%p\n",
                                         read_p->position, current_read_info_p->position, siots_p);
                    return (FBE_STATUS_GENERIC_FAILURE);
                }
                status = fbe_raid_fru_info_allocate_sgs(siots_p, read_p, memory_info_p, type);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "raid:fail to alloc mem:read=0x%p,siots=0x%p,stat=0x%x\n",
                                         read_p, siots_p, status);
                    return  (status);
                }
            }
            current_read_info_p++;
            read_p = fbe_raid_fruts_get_next(read_p);
        }
        while ((read2_p != NULL)&&(current_read2_info_p != NULL))
        {
            if (current_read2_info_p->sg_index == sg_index)
            {
                if (RAID_COND(read2_p->position != current_read2_info_p->position))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "raid: read2_p->position 0x%x != current_read2_info_p->position 0x%x \n",
                                         read2_p->position, current_read2_info_p->position);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                status = fbe_raid_fru_info_allocate_sgs(siots_p, read2_p, memory_info_p, type);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "raid:fail to alloc mem:read2=0x%p,siots=0x%p,stat=0x%x\n",
                                         read2_p, siots_p, status);
                    return  (status);
                }
            }
            current_read2_info_p++;
            read2_p = fbe_raid_fruts_get_next(read2_p);
        }
        while (write_p != NULL)
        {
            if (current_write_info_p->sg_index == sg_index)
            {
                if (RAID_COND(write_p->position != current_write_info_p->position))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                "write_p->position 0x%x != current_write_info_p->position 0x%x\n",
                                                write_p->position, current_write_info_p->position);
                    return (FBE_STATUS_GENERIC_FAILURE);
                }
                status = fbe_raid_fru_info_allocate_sgs(siots_p, write_p, memory_info_p, type);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "raid:fail to alloc mem:write=0x%p,siots=0x%p,stat=0x%x\n",
                                         write_p, siots_p, status);
                    return  (status);
                }
            }
            current_write_info_p++;
            write_p = fbe_raid_fruts_get_next(write_p);
        }
    } /* end for all sg types.*/
    return (FBE_STATUS_OK);
}
/*****************************************
 * fbe_raid_fru_info_array_allocate_sgs()
 *****************************************/

/*!***************************************************************************
 *          fbe_raid_fru_info_allocate_sgs_sparse()
 *****************************************************************************
 *
 * @brief   This function is called to allocate and plant all the sgs for a
 *          particular fruts chain.
 *
 * @param   siots_p - Pointer to siots that contains the allocated fruts
 * @param   fruts_p - Pointer to fruts chain to allocate and plant for
 * @param   memory_info_p - Pointer to memory request information
 * @param   fru_info_size - Number of elements in each of the fru info arrays
 * @param   fru_info_p - Pointer to fru info array
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  03/12/2009  Ron Proulx - Created
 *
 **************************************************************************/
static fbe_status_t fbe_raid_fru_info_allocate_sgs_sparse(fbe_raid_siots_t *siots_p,
                                                          fbe_raid_fruts_t *fruts_p,
                                                          fbe_raid_memory_info_t *memory_info_p,
                                                          fbe_u32_t  fru_info_size,
                                                          fbe_raid_fru_info_t *fru_info_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               index;
    fbe_raid_memory_type_t  type;

    /* Process the specified fruts chain
     */
    while (fruts_p != NULL)
    {
        /* Locate this position in the fru info array.
         */
        for (index = 0; (fru_info_p != NULL) && (index < fru_info_size); index++)
        {
            /* If the fru info matches this position, allocate and plant sg.
             */
            if (fruts_p->position == fru_info_p->position)
            {
                /* It is valid to have an invalid index since there are cases
                 * where we don't need to allocate an sg.
                 */
                if (fru_info_p->sg_index != FBE_RAID_SG_INDEX_INVALID)
                {
                    /* Get the type, allocate and plant sg.
                     */
                    type = fbe_raid_memory_get_sg_type_by_index(fru_info_p->sg_index);
                    if (type == FBE_RAID_MEMORY_TYPE_INVALID)
                    {
                        fbe_raid_siots_trace(siots_p,
                                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "raid: siots - lba: %llx blks: 0x%llx invalid sg_index for position: 0x%x\n",
                                             (unsigned long long)siots_p->start_lba,
					     (unsigned long long)siots_p->xfer_count,
					     fruts_p->position);
                        return(FBE_STATUS_GENERIC_FAILURE);
                    }

                    /* Allocate and plant sg and then break out of for loop
                     */
                    status = fbe_raid_fru_info_allocate_sgs(siots_p, fruts_p, memory_info_p, type);
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                             "raid:fail to alloc mem:fruts=0x%p,siots=0x%p,stat=0x%x\n",
                                            fruts_p, siots_p, status);
                        return  (status);
                    }
                }
                break;
            }

            /* Goto next fru info.
             */
            fru_info_p++;
        
        } /* end for all fru info entries */
    
        /* Goto next fruts.
         */
        fruts_p = fbe_raid_fruts_get_next(fruts_p);

    } /* end for all fruts on chain */

    /* Always return the execution status.
     */
    return(status);
}
/************************************************
 * end of fbe_raid_fru_info_allocate_sgs_sparse()
 ************************************************/

/*!***************************************************************************
 *          fbe_raid_fru_info_array_allocate_sgs_sparse()
 *****************************************************************************
 *
 * @brief   This function is called to allocate and plant all the sgs for the
 *          read fruts, read2 fruts and write fruts.  This method supports a
 *          `sparsely' populated fru info array.  That is it will locate the
 *          correct fru infomation based on the position in the associated
 *          fruts (either read, read2 or write).
 *
 * @param   siots_p - Pointer to siots that contains the allocated fruts
 * @param   memory_info_p - Pointer to memory request information
 * @param   read_info_p - Pointer to read info array
 * @param   read2_info_p - Pointer to read2 info array
 * @param   write_info_p - Pointer to write info array
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  03/12/2009  Ron Proulx - Created from fbe_raid_fru_info_array_allocate_sgs
 *
 **************************************************************************/
fbe_status_t fbe_raid_fru_info_array_allocate_sgs_sparse(fbe_raid_siots_t *siots_p,
                                                         fbe_raid_memory_info_t *memory_info_p,
                                                         fbe_raid_fru_info_t *read_info_p,
                                                         fbe_raid_fru_info_t *read2_info_p,
                                                         fbe_raid_fru_info_t *write_info_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_fruts_t   *read_fruts_p = NULL;
    fbe_raid_fruts_t   *read2_fruts_p = NULL;
    fbe_raid_fruts_t   *write_fruts_p = NULL;
    fbe_u32_t           fru_info_size = fbe_raid_siots_get_width(siots_p);

    /* Get the fruts pointers from the siots.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    
    /* First process the read fruts.
     */
    status = fbe_raid_fru_info_allocate_sgs_sparse(siots_p,
                                                   read_fruts_p,
                                                   memory_info_p,
                                                   fru_info_size,
                                                   read_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    { 
        fbe_raid_siots_trace(siots_p, 
                             FBE_TRACE_LEVEL_ERROR, __LINE__, "raid_fru_info_arr_alloc_sgs_sparse" ,
                             "fail to alloc sg spare for siots_p 0x%p with stat:0x%x\n",
                             siots_p, status);
        return (status); 
    }

    /* Now process the read2 fruts.
     */
    status = fbe_raid_fru_info_allocate_sgs_sparse(siots_p,
                                                   read2_fruts_p,
                                                   memory_info_p,
                                                   fru_info_size,
                                                   read2_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    { 
        fbe_raid_siots_trace(siots_p, 
                             FBE_TRACE_LEVEL_ERROR, __LINE__, "raid_fru_info_arr_alloc_sgs_sparse" ,
                             "fail to alloc sg spare for siots_p 0x%p with stat:0x%x\n",
                             siots_p, status);
        return (status); 
    }

    /* Finally process the write fruts.
     */
    status = fbe_raid_fru_info_allocate_sgs_sparse(siots_p,
                                                   write_fruts_p,
                                                   memory_info_p,
                                                   fru_info_size,
                                                   write_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_TRACE_LEVEL_ERROR, __LINE__, "raid_fru_info_arr_alloc_sgs_sparse" ,
                             "fail to alloc sg spare for siots_p 0x%p with stat:0x%x\n",
                             siots_p, status);
        return status;
    }
    /* Always return the execution status.
     */
    return (FBE_STATUS_OK);
}
/******************************************************
 * end of fbe_raid_fru_info_array_allocate_sgs_sparse()
 ******************************************************/


/*******************************
 * end file fbe_raid_fru_info.c
 *******************************/


