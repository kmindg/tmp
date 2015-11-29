/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_parity_468_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for the parity object's small write
 *  aka. 468 algorithm.
 *
 * @author
 *  06/29/99:  Rob Foley -- Created.
 *
 ***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"


fbe_status_t fbe_parity_468_setup_sgs(fbe_raid_siots_t * siots_p,
                                      fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_parity_468_setup_fruts(fbe_raid_siots_t * siots_p, 
                                        fbe_raid_fru_info_t *read_p,
                                        fbe_raid_fru_info_t *write_p,
                                        fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_parity_468_validate_fruts_sgs(fbe_raid_siots_t *siots_p);
static fbe_status_t fbe_parity_468_validate_sgs(fbe_raid_siots_t * siots_p);

/*!**************************************************************************
 * fbe_parity_small_468_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param num_pages_to_allocate_p - Number of memory pages to allocate
 * 
 * @return None.
 *
 **************************************************************************/

fbe_status_t fbe_parity_small_468_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                       fbe_raid_memory_type_t *type_p)
{
    fbe_u32_t num_fruts = 0;
    fbe_block_count_t total_blocks = 0;
    fbe_status_t status;
    fbe_u32_t num_parity = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_block_count_t mem_left = 0;
    fbe_u32_t sg_count = 0;
    fbe_u32_t parity_sg_count = 0;

    /* Total pr blocks to allocate is for one data position and parity.
     */
    total_blocks = (fbe_u32_t)(siots_p->xfer_count * (num_parity + 1));
    if (total_blocks > FBE_U32_MAX)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "%s total blocks 0x%llx > FBE_U32_MAX\n",
				     __FUNCTION__,
				     (unsigned long long)total_blocks);
    }
    /* First start off with 1 fruts for every data position we are writing 
     * and for every parity position. 
     * We multiply by 2 since we will have a read and a write. 
     * Also need a fruts for the possibility of doing journal log requests if we go degraded.
     */
    num_fruts = ((siots_p->data_disks + num_parity) * 2) + 1;

    /*! Save the number of blocks to alloc.
     */
    fbe_raid_siots_set_total_blocks_to_allocate(siots_p, (fbe_u32_t)total_blocks); 

    fbe_raid_siots_set_page_size(siots_p,
                                 fbe_raid_memory_calculate_ctrl_page_size(siots_p, num_fruts));
    fbe_raid_siots_set_data_page_size(siots_p, siots_p->ctrl_page_size);

    if ((siots_p->xfer_count * (num_parity + 1)) <= siots_p->data_page_size)
    {
        /* If all our buffers fit in a single page, we're going to use just a single SG.
         */
        sg_count = 1;
    }
    else
    {
        mem_left = fbe_raid_sg_count_uniform_blocks(siots_p->xfer_count,
                                                    siots_p->data_page_size,
                                                    mem_left,
                                                    &sg_count);
        mem_left = fbe_raid_sg_count_uniform_blocks(siots_p->xfer_count,
                                                    siots_p->data_page_size,
                                                    mem_left,
                                                    &parity_sg_count);
        sg_count = FBE_MAX(sg_count, parity_sg_count);
        if (num_parity == 2)
        {
            mem_left = fbe_raid_sg_count_uniform_blocks(siots_p->xfer_count,
                                                        siots_p->data_page_size,
                                                        mem_left,
                                                        &parity_sg_count);
            sg_count = FBE_MAX(sg_count, parity_sg_count);
        }
    }
    *type_p = fbe_raid_memory_get_sg_type_by_index(fbe_raid_memory_sg_count_index(sg_count));
    
    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_memory_calculate_num_pages_fast(siots_p->ctrl_page_size,
                                                      siots_p->data_page_size,
                                                      &siots_p->num_ctrl_pages,
                                                      &siots_p->num_data_pages,
                                                      num_fruts,
                                                      siots_p->data_disks + num_parity, /* num sg lists */
                                                      sg_count, /* how big each sg should be*/
                                                      siots_p->total_blocks_to_allocate);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_468_calc_mem_size" ,
                             "parity:468 fail to calc num pages with stat 0x%x,siots=0x%p\n",
                             status, siots_p);
        return(status); 
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_small_468_calculate_memory_size()
 **************************************/
/*!**************************************************************
 * fbe_parity_small_468_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param   siots_p - current I/O.     
 * @param   read_p - Pointer to per position read information          
 * @param   write_p - Pointer to per position write information          
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_small_468_setup_resources(fbe_raid_siots_t * siots_p,
                                                 fbe_raid_memory_type_t type)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_memory_info_t  memory_info;
    fbe_raid_memory_info_t  sg_memory_info;
    fbe_raid_memory_info_t  data_memory_info;
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_u16_t sg_total = 0;
    fbe_u64_t dest_byte_count = 0;
    fbe_sg_element_t *sgl_ptr = NULL;

    dest_byte_count = FBE_BE_BYTES_PER_BLOCK * siots_p->xfer_count;
    if (dest_byte_count > FBE_U32_MAX)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "%s: byte count exceeding 32bit limit \n",
                             __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Initialize our data memory information
     */
    status = fbe_raid_siots_init_data_mem_info(siots_p, &data_memory_info);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to init dmem info with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }

    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to init cmem info with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }
    sg_memory_info = memory_info;
    /* Skip over the area we will use for fruts.
     */
    status = fbe_raid_memory_apply_allocation_offset(&sg_memory_info,
                                                     (1 + parity_drives) * 2, /* Num allocations */
                                                     sizeof(fbe_raid_fruts_t) /* size of each allocation */);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to apply offset to buffer for siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return status;
    }

    /* Data pre-read.
     */
    read_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p, &siots_p->read_fruts_head, &memory_info);
    status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->start_pos);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid:fail to init read pre-read fruts :siots=0x%p,stat=0x%x\n",
                             siots_p, status);
        return(status);
    }
    /* Allocate sg.
     */
    status = fbe_raid_fru_info_allocate_sgs(siots_p, read_fruts_ptr, &sg_memory_info, type);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid:fail to alloc mem:siots=0x%p,stat=0x%x\n",
                             siots_p, status);
        return(status);
    }
    /* Allocate memory to sg for pre-read blocks.
     */
    sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);
    sg_total =  fbe_raid_sg_populate_with_memory(&data_memory_info, sgl_ptr, (fbe_u32_t)dest_byte_count);
    if (sg_total == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "%s: failed to populate sg for: siots = 0x%p\n",
                             __FUNCTION__,siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Data write.
     */
    write_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p, &siots_p->write_fruts_head, &memory_info);

    status = fbe_raid_fruts_init_fruts(write_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->start_pos);

    write_fruts_ptr->write_preread_desc.lba = siots_p->parity_start;
    write_fruts_ptr->write_preread_desc.block_count = siots_p->xfer_count;
    /* Use the caller's sg.
     */
    fbe_raid_siots_get_sg_ptr(siots_p, &write_fruts_ptr->sg_p);

    /* Parity pre-read.
     */
    read_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p, &siots_p->read_fruts_head, &memory_info);
    status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->parity_pos);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid:fail to alloc mem:siots=0x%p,stat=0x%x\n",
                             siots_p, status);
        return  (status);
    }
    /* Allocate sg.
     */
    status = fbe_raid_fru_info_allocate_sgs(siots_p, read_fruts_ptr, &sg_memory_info, type);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid:fail to alloc mem:siots=0x%p,stat=0x%x\n",
                             siots_p, status);
        return  (status);
    }
    /* Allocate memory to sg for parity pre-read blocks.
     */
    sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);
    sg_total =  fbe_raid_sg_populate_with_memory(&data_memory_info, sgl_ptr, (fbe_u32_t)dest_byte_count);
    if (sg_total == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "%s: failed to populate sg for: siots = 0x%p\n",
                             __FUNCTION__,siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Parity write
     */
    write_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p, &siots_p->write_fruts_head, &memory_info);
    status = fbe_raid_fruts_init_fruts(write_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->parity_pos);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid:fail to alloc mem:siots=0x%p,stat=0x%x\n",
                             siots_p, status);
        return  (status);
    }
    write_fruts_ptr->sg_id = read_fruts_ptr->sg_id;
    write_fruts_ptr->write_preread_desc.lba = siots_p->parity_start;
    write_fruts_ptr->write_preread_desc.block_count = siots_p->xfer_count;

    if (parity_drives == 2)
    {
        /* Parity pre-read.
         */
        read_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p, &siots_p->read_fruts_head, &memory_info);
        status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                           siots_p->parity_start,
                                           siots_p->xfer_count,
                                           siots_p->parity_pos + 1);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid:fail to alloc mem:siots=0x%p,stat=0x%x\n",
                                 siots_p, status);
            return(status);
        }
        /* Allocate sg.
         */
        status = fbe_raid_fru_info_allocate_sgs(siots_p, read_fruts_ptr, &sg_memory_info, type);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid:fail to alloc mem:siots=0x%p,stat=0x%x\n",
                                 siots_p, status);
            return(status);
        }
        /* Allocate memory to sg for parity pre-read blocks.
         */
        sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);
        sg_total =  fbe_raid_sg_populate_with_memory(&data_memory_info, sgl_ptr, (fbe_u32_t)dest_byte_count);
        if (sg_total == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "%s: failed to populate sg for: siots = 0x%p\n",
                                 __FUNCTION__,siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* Parity write
         */
        write_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p, &siots_p->write_fruts_head, &memory_info);
        status = fbe_raid_fruts_init_fruts(write_fruts_ptr,
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                           siots_p->parity_start,
                                           siots_p->xfer_count,
                                           siots_p->parity_pos + 1);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid:fail to alloc mem:siots=0x%p,stat=0x%x\n",
                                 siots_p, status);
            return(status);
        }
        write_fruts_ptr->sg_id = read_fruts_ptr->sg_id;
        write_fruts_ptr->write_preread_desc.lba = siots_p->parity_start;
        write_fruts_ptr->write_preread_desc.block_count = siots_p->xfer_count;
    }
    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */

    if (RAID_DBG_ENABLED)
    {
        /* Make sure our sgs are sane.
         */
        status = fbe_parity_468_validate_sgs(siots_p);
    }
    return status;
}
/******************************************
 * end fbe_parity_small_468_setup_resources()
 ******************************************/
/*!**************************************************************
 * fbe_parity_small_468_setup_embed_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param   siots_p - current I/O.     
 * @param   read_p - Pointer to per position read information          
 * @param   write_p - Pointer to per position write information          
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_parity_small_468_setup_embed_resources1(fbe_raid_siots_t * siots_p)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_memory_info_t  memory_info;
    fbe_raid_memory_info_t  data_memory_info;
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_u16_t sg_total = 0;
    fbe_u64_t dest_byte_count = 0;

    dest_byte_count = FBE_BE_BYTES_PER_BLOCK * siots_p->xfer_count;
    if (dest_byte_count > FBE_U32_MAX)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "%s: byte count exceeding 32bit limit \n",
                             __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to init memory info with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }
    status = fbe_raid_siots_init_data_mem_info(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to init data memory info: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Data pre-read.
     */
    read_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p, &siots_p->read_fruts_head, &memory_info);
    status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->start_pos);
    /* Allocate sg.
     */
    read_fruts_ptr->sg_p = &read_fruts_ptr->sg[0];
    /* Allocate memory to sg for pre-read blocks.
     */
    sg_total =  fbe_raid_sg_populate_with_memory(&data_memory_info, read_fruts_ptr->sg_p, (fbe_u32_t)dest_byte_count);
    if (sg_total == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "%s: failed to populate sg for: siots = 0x%p\n",
                             __FUNCTION__,siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Data write.
     */
    write_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p, &siots_p->write_fruts_head, &memory_info);

    status = fbe_raid_fruts_init_fruts(write_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->start_pos);
    /* Use the caller's sg.
     */
    fbe_raid_siots_get_sg_ptr(siots_p, &write_fruts_ptr->sg_p);

    /* Parity pre-read.
     */
    read_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p, &siots_p->read_fruts_head, &memory_info);
    status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->parity_pos);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid:fail to alloc mem:siots=0x%p,stat=0x%x\n",
                             siots_p, status);
        return  (status);
    }
    /* Allocate sg.
     */
    read_fruts_ptr->sg_p = &read_fruts_ptr->sg[0];
    /* Allocate memory to sg for parity pre-read blocks.
     */
    sg_total =  fbe_raid_sg_populate_with_memory(&data_memory_info, read_fruts_ptr->sg_p, (fbe_u32_t)dest_byte_count);
    if (sg_total == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "%s: failed to populate sg for: siots = 0x%p\n",
                             __FUNCTION__,siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Parity write
     */
    write_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p, &siots_p->write_fruts_head, &memory_info);
    status = fbe_raid_fruts_init_fruts(write_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->parity_pos);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid:fail to alloc mem:siots=0x%p,stat=0x%x\n",
                             siots_p, status);
        return  (status);
    }
    write_fruts_ptr->sg_p = read_fruts_ptr->sg_p;

    /*! @note, this path does not yet support raid6   */
    if (parity_drives == 2)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: not implemented yet\n");
    }
    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */

    if (RAID_DBG_ENABLED)
    {
        /* Make sure our sgs are sane.
         */
        status = fbe_parity_468_validate_sgs(siots_p);
    }
    return status;
}
/******************************************
 * end fbe_parity_small_468_setup_embed_resources()
 ******************************************/
void fbe_raid_fruts_init_common(fbe_raid_fruts_t *fruts_p)
{
    fbe_raid_fruts_common_init(&fruts_p->common);
    fbe_raid_common_init_flags(&fruts_p->common, FBE_RAID_COMMON_FLAG_TYPE_FRU_TS);

    /* Mark it as uninitialized now. 
     * This is needed to keep track of when to destroy the packet.
     */
    fbe_raid_fruts_init_flags(fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);
}
/*!**************************************************************
 * fbe_parity_small_468_setup_embed_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param   siots_p - current I/O.     
 * @param   read_p - Pointer to per position read information          
 * @param   write_p - Pointer to per position write information          
 *
 * @return fbe_status_t
 *
 ****************************************************************/
static fbe_sector_t sector_data[16];
static fbe_sector_t sector_parity[16];
static fbe_raid_fruts_t read_fruts[2];
static fbe_raid_fruts_t write_fruts[2];
fbe_status_t fbe_parity_small_468_setup_embed_resources(fbe_raid_siots_t * siots_p)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_u64_t dest_byte_count = 0;

    dest_byte_count = FBE_BE_BYTES_PER_BLOCK * siots_p->xfer_count;

    /* Data pre-read.
     */
    read_fruts_ptr = &read_fruts[0];
    fbe_raid_fruts_init_common(read_fruts_ptr);
    fbe_raid_fruts_set_siots(read_fruts_ptr, siots_p);
    fbe_raid_common_enqueue_tail(&siots_p->read_fruts_head, (fbe_raid_common_t*)read_fruts_ptr);

    status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->start_pos);
    /* Allocate sg.
     */
    read_fruts_ptr->sg_p = &read_fruts_ptr->sg[0];
    read_fruts_ptr->sg[0].address = (fbe_u8_t*)&sector_data[0].data_word[0];
    read_fruts_ptr->sg[0].count = (fbe_u32_t)dest_byte_count;

    /* Data write.
     */
    write_fruts_ptr = &write_fruts[0];
    fbe_raid_fruts_init_common(write_fruts_ptr);
    fbe_raid_fruts_set_siots(write_fruts_ptr, siots_p);
    fbe_raid_common_enqueue_tail(&siots_p->write_fruts_head, (fbe_raid_common_t*)write_fruts_ptr);

    status = fbe_raid_fruts_init_fruts(write_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->start_pos);
    /* Use the caller's sg.
     */
    fbe_raid_siots_get_sg_ptr(siots_p, &write_fruts_ptr->sg_p);

    /* Parity pre-read.
     */
    read_fruts_ptr = &read_fruts[1];
    fbe_raid_fruts_init_common(read_fruts_ptr);
    fbe_raid_fruts_set_siots(read_fruts_ptr, siots_p);
    fbe_raid_common_enqueue_tail(&siots_p->read_fruts_head, (fbe_raid_common_t*)read_fruts_ptr);
    status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->parity_pos);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid:fail to alloc mem:siots=0x%p,stat=0x%x\n",
                             siots_p, status);
        return  (status);
    }
    /* Allocate sg.
     */
    read_fruts_ptr->sg_p = &read_fruts_ptr->sg[0];
    read_fruts_ptr->sg[0].address = (fbe_u8_t*)&sector_parity[0].data_word[0];
    read_fruts_ptr->sg[0].count = (fbe_u32_t)dest_byte_count;

    /* Parity write
     */
    write_fruts_ptr = &write_fruts[1];
    fbe_raid_fruts_init_common(write_fruts_ptr);
    fbe_raid_fruts_set_siots(write_fruts_ptr, siots_p);
    fbe_raid_common_enqueue_tail(&siots_p->write_fruts_head, (fbe_raid_common_t*)write_fruts_ptr);
    status = fbe_raid_fruts_init_fruts(write_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                       siots_p->parity_start,
                                       siots_p->xfer_count,
                                       siots_p->parity_pos);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid:fail to alloc mem:siots=0x%p,stat=0x%x\n",
                             siots_p, status);
        return  (status);
    }
    write_fruts_ptr->sg_p = read_fruts_ptr->sg_p;

    /*! @note raid6 is not supported in this code path.   */
    if (parity_drives == 2)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: not implemented yet\n");
    }
    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */

    if (RAID_DBG_ENABLED)
    {
        /* Make sure our sgs are sane.
         */
        status = fbe_parity_468_validate_sgs(siots_p);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_small_468_setup_embed_resources()
 ******************************************/
/*!**************************************************************************
 * fbe_parity_468_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param num_pages_to_allocate_p - Number of memory pages to allocate
 * 
 * @return None.
 *
 **************************************************************************/

fbe_status_t fbe_parity_468_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                  fbe_raid_fru_info_t *read_info_p,
                                                  fbe_raid_fru_info_t *write_info_p,
                                                  fbe_u32_t *num_pages_to_allocate_p)
{
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t num_fruts = 0;
    fbe_u32_t total_blocks = 0;
    fbe_status_t status;

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));
    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_468_get_fruts_info(siots_p, 
                                           read_info_p,
                                           write_info_p,
                                           (fbe_block_count_t*)&total_blocks);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_468_calc_mem_size" ,
                             "parity:468 fail get fru:stat 0x%x,"
                             "siots=0x%p>\n",
                             status, siots_p);
		fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_468_calc_mem_size" ,
                             "read_info=0x%p,write_info=0x%p\n",
                             read_info_p, write_info_p);
        return(status); 
    }

    /* First start off with 1 fruts for every data position we are writing 
     * and for every parity position. 
     * We multiply by 2 since we will have a read and a write. 
     */
    num_fruts = (siots_p->data_disks + fbe_raid_siots_parity_num_positions(siots_p)) * 2;

    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        /* Buffered transfer needs blocks allocated.
         */
        total_blocks += (fbe_u32_t)siots_p->xfer_count;
    }

    /* Increase the total blocks allocated by 1 per write position for write log headers if required
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD))
    {
        fbe_u32_t journal_log_hdr_blocks;
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);
        total_blocks += (siots_p->data_disks + fbe_raid_siots_parity_num_positions(siots_p)) * journal_log_hdr_blocks;
    }

    fbe_raid_siots_set_total_blocks_to_allocate(siots_p, total_blocks);
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, 
                                         siots_p->total_blocks_to_allocate);

    /* Next calculate the number of sgs of each type.
     */
    status = fbe_parity_468_calculate_num_sgs(siots_p, &num_sgs[0], read_info_p, write_info_p, FBE_TRUE);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_468_calc_mem_size",
                             "parity:468 fail calc num sgs:stat 0x%x,siots=0x%p>\n",
                             status, siots_p);
		fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_468_calc_mem_size",
                             "read_info=0x%p,write_info=0x%p<\n",
                             read_info_p, write_info_p);
        return (status); 
    }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts,
                                                &num_sgs[0], FBE_TRUE /* In-case recovery verify is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_468_calc_mem_size" ,
                             "parity:468 fail to calc num pages with stat 0x%x,siots=0x%p\n",
                             status, siots_p);
        return(status); 
    }
    *num_pages_to_allocate_p = siots_p->num_ctrl_pages;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_468_calculate_memory_size()
 **************************************/
/*!**************************************************************
 * fbe_parity_468_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param   siots_p - current I/O.     
 * @param   read_p - Pointer to per position read information          
 * @param   write_p - Pointer to per position write information          
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_468_setup_resources(fbe_raid_siots_t * siots_p, 
                                            fbe_raid_fru_info_t *read_p,
                                            fbe_raid_fru_info_t *write_p)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_memory_info_t  memory_info;
    fbe_raid_memory_info_t  data_memory_info;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to init memory info with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }

    status = fbe_raid_siots_init_data_mem_info(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to init data memory info: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Set up the FRU_TS.
     */
    status = fbe_parity_468_setup_fruts(siots_p, read_p, write_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:fail to setup fruts for 468 opt:stat=0x%x,siots=0x%p\n",
                             status, siots_p);
        return(status);
    }
    /* Plant the nested siots in case it is needed for recovery verify.
     * We don't initialize it until it is required.
     */
    status = fbe_raid_siots_consume_nested_siots(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to consume nested siots: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return (status);
    }
        
    /* Initialize the RAID_VC_TS now that it is allocated.
     */
    status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity : failed to init vcts for siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return status;
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity : failed to init vrts for siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return status;
    }
    /* Plant the allocated sgs into the locations calculated above.
     */
    status = fbe_raid_fru_info_array_allocate_sgs(siots_p, &memory_info,
                                                  read_p, NULL, write_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to allocate sgs with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }

    /* Assign buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory. 
     */
    status = fbe_parity_468_setup_sgs(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:468 fail to setup sgs with stat 0x%x,siots=0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_468_setup_resources()
 ******************************************/

/*!***************************************************************
 * fbe_parity_468_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fru info.
 *
 * @param siots_p - Current sub request.
 * @param read_p - read info to calculate for this request.
 * @param write_p - write info to calculate for this request.
 * @param read_blocks_p - number of blocks being read.
 *
 * @return
 *  None
 *
 ****************************************************************/

fbe_status_t fbe_parity_468_get_fruts_info(fbe_raid_siots_t * siots_p,
                                           fbe_raid_fru_info_t *read_p,
                                           fbe_raid_fru_info_t *write_p,
                                           fbe_block_count_t *read_blocks_p)
{
    fbe_lba_t write_lba;         /* block offset where access starts */
    fbe_block_count_t  write_blocks;       /* number of blocks accessed */
    fbe_u32_t  array_pos;         /* Actual array position of access */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t data_pos;          /* data position in array */
    fbe_u16_t num_disks;          /* loop counter */

    fbe_lba_t parity_range_offset;  /* Distance from parity boundary to parity_start */

    fbe_block_count_t r1_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];    /* block counts for pre-reads, */
    fbe_block_count_t r2_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];    /* distributed across the FRUs */

    fbe_block_count_t ret_read_blocks = 0;

    fbe_u8_t *position = siots_p->geo.position;

    /* Check number of parity drives that are supported. R5=1, R6=2
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;

    /* Leverage MR3 calculations to describe what
     * portions of stripe(s) are *not* being accessed.
     */
    status = fbe_raid_geometry_calc_preread_blocks(siots_p->start_lba,
                                                   &siots_p->geo,
                                                   siots_p->xfer_count,
                                                   (fbe_u32_t)fbe_raid_siots_get_blocks_per_element(siots_p),
                                                   fbe_raid_siots_get_width(siots_p) - parity_drives,
                                                   siots_p->parity_start,
                                                   siots_p->parity_count,
                                                   r1_blkcnt,
                                                   r2_blkcnt);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail calc pre-read blocks:status=0x%x>\n",
                             status);
	fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "siots=0x%p,r1_blkcnt = 0x%p,r2_blkcnt=0x%p<\n",
                             siots_p, r1_blkcnt, r2_blkcnt);
        return status;
    }

    /* Calculate the total offset from our parity range.
     */
    parity_range_offset = fbe_raid_siots_get_parity_start_offset(siots_p);

    /* Loop through all data positions, setting up
     * each fru info.
     */
    for (data_pos = 0,
         num_disks = siots_p->data_disks;
         num_disks > 0;
         data_pos++)
    {
        /* Validate array position. */
        if (data_pos >= fbe_raid_siots_get_width(siots_p) - parity_drives)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "data_pos %d unexpected width: %d parity_drives: %d\n", 
                                 data_pos, fbe_raid_siots_get_width(siots_p), parity_drives);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* The pre-reads should NOT exceed the parity range.  */
        if ((r1_blkcnt[data_pos] + r2_blkcnt[data_pos]) > siots_p->parity_count)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "pre-read range %llu %llu exceeds parity range %llu\n", 
                                 (unsigned long long)r1_blkcnt[data_pos],
				 (unsigned long long)r2_blkcnt[data_pos],
				 (unsigned long long)siots_p->parity_count);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        write_blocks = siots_p->parity_count - r1_blkcnt[data_pos] - r2_blkcnt[data_pos];

        /* If a write_blocks exists for this position,
         * fill out read/write fruts.
         */
        if (write_blocks)
        {
            /* Should not be processing parity position here */
            if (position[data_pos] == siots_p->parity_pos)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: position %d equal to parity\n", position[data_pos]);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Init current pre-read and write's offset and position.
             * Note that the extent structure is used to determine offset/position.
             */
            write_lba = logical_parity_start + parity_range_offset + r1_blkcnt[data_pos];

            array_pos = position[data_pos];

            /* Validate the block count. */
            if (write_blocks == 0)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: write blocks is zero\n");
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Init the write fruts.
             */
            write_p->lba = write_lba;
            write_p->blocks = write_blocks;
            write_p->position = array_pos;

            /* Init the read fruts.
             * If this is an unaligned write, then it has 
             * a pre-read, so init the read fruts to 
             * cover the range of the pre-read. 
             */
            fbe_raid_siots_setup_preread_fru_info(siots_p, read_p, write_p, array_pos);

            ret_read_blocks += read_p->blocks;
            /* Go to the next fruts set,
             * and decrement the disk count.
             */
            num_disks--;
            read_p++;
            write_p++;
        }
    }                           /* For num_disks = data_disks; num_disks > 0 */

    /* Init the fruts for pre-read/write
     * of parity in the final fbe_raid_fruts_t.
     */
    write_lba = logical_parity_start + parity_range_offset;
    write_blocks = siots_p->parity_count;
    array_pos = position[fbe_raid_siots_get_width(siots_p) - parity_drives];
    write_p->lba = write_lba;
    write_p->blocks = write_blocks;
    write_p->position = array_pos;

    /* Init the read fruts. 
     * If this is unaligned, then a pre-read is needed, so init the lba, blocks 
     * the same as the pre-read. 
     */
    fbe_raid_siots_setup_preread_fru_info(siots_p, read_p, write_p, array_pos);
    ret_read_blocks += read_p->blocks;

    if (parity_drives == 2)
    {
        /* For RAID 6 initialize the 2nd parity drive.
         */
        read_p++;
        write_p++;

        write_lba = logical_parity_start + parity_range_offset;
        array_pos = position[fbe_raid_siots_get_width(siots_p) - 1];
        write_p->lba = write_lba;
        write_p->blocks = write_blocks;
        write_p->position = array_pos;

        /* Init the read fruts. 
         * If this is unaligned, then a pre-read is needed, so init the lba, blocks 
         * the same as the pre-read. 
         */ 
        fbe_raid_siots_setup_preread_fru_info(siots_p, read_p, write_p, array_pos);
        ret_read_blocks += read_p->blocks;

        /* If this is not a parity drive return error.
         */                             
        if (!fbe_raid_siots_is_parity_pos(siots_p, write_p->position))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: write position not parity position\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    if (read_blocks_p != NULL)
    {
        *read_blocks_p = ret_read_blocks;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_468_get_fruts_info() 
 **************************************/

/*!***************************************************************
 * fbe_parity_468_unaligned_get_pre_read_info()
 ****************************************************************
 * @brief
 *  This determines the correct pre-read lba and block count
 *  for a 468 operation.
 *  The 468 operation is interesting because we are always pre-reading
 *  exactly what we are writing over.
 * 
 *  Thus, a pre-read is required for a 468 always.
 * 
 *  This function determines how much extra is required to pre-read
 *  to account for the unalignment.
 *
 * @param pre_read_descriptor_p - The ptr to the descriptor for this
 *                          pre-read.  This pre-read may be only at
 *                          the beginning or only at the end of the total write range.
 * @param write_lba - The beginning of the write range.
 * @param write_blocks - The total size of the write range.
 * @param output_lba_p - Ptr to the pre-read lba to return.
 * @param output_blocks_p - Ptr to the pre-read blocks to return.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/14/2008 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_parity_468_unaligned_get_pre_read_info(fbe_payload_pre_read_descriptor_t *const pre_read_descriptor_p,
                                                        fbe_lba_t write_lba, 
                                                        fbe_block_count_t write_blocks,
                                                        fbe_lba_t *const output_lba_p, 
                                                        fbe_block_count_t *const output_blocks_p)
{
    fbe_lba_t pre_read_end;

    /* The pre-read will either be greater than or equal to the write.
     * When the logical drive tells us the pre-read range, it optimizes to
     * tell us the minimum amount that needs to be pre-read.  So for example, if
     * the start is aligned, but the end is not then we will only have a
     * pre-read at the end.
     *
     * Case 1: pre-read at beginning only.
     *
     * pdo pre-read  Write  old       new 468 
     *        range  range  pre-read  pre-read
     *                      range     range.
     *        ****                    ****  
     *        ****   ****   ****      ****
     *        ****   ****   ****      ****
     *               ****   ****      **** 
     *               ****   ****      ****
     *
     * Case 2: pre-read at end only.
     *
     * pdo pre-read  Write  old       new 468 
     *        range  range  pre-read  pre-read
     *                      range     range.
     *               ****   ****      ****  
     *               ****   ****      ****
     *        ****   ****   ****      ****
     *        ****                    **** 
     *        ****                    ****
     *
     * Case 3: pre-read at beginning and end.
     *
     * pdo pre-read  Write  old       new 468 
     *        range  range  pre-read  pre-read
     *                      range     range.
     *        ****                     
     *        ****                    ****
     *        ****   ****   ****      ****
     *        ****   ****   ****      ****
     *        ****                    **** 
     *        ****                    ****
     *
     * Init the read fruts to cover any extra area that is required for 
     * this unaligned write. 
     *  
     * The pre-read start lba is the minimum of the descriptor's lba or 
     * the write lba since the pre-read lba might be less than the 
     * write lba. 
     */  
    *output_lba_p = FBE_MIN(pre_read_descriptor_p->lba, write_lba);

    /* The new pre-read end lba is the greater of either the end of the write 
     * or the end of the pre-read, since the pre-read end might be greater than the write.
     */
    pre_read_end = pre_read_descriptor_p->lba + pre_read_descriptor_p->block_count;
    *output_blocks_p = (fbe_u32_t) FBE_MAX( pre_read_end - *output_lba_p, 
                                      (write_lba + write_blocks) - *output_lba_p );
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_468_unaligned_get_pre_read_info()
 ******************************************/
/*!**************************************************************
 * fbe_raid_siots_setup_preread_fru_info()
 ****************************************************************
 * @brief   
 *   This is a helper function for setting up a 468 read fruts.   
 *  
 * @param read_fruts_ptr - The ptr to the read fruts to setup.
 * @param write_fruts_ptr - The ptr to the write fruts to use   
 *                          to help us setup the read fruts.
 * @param array_pos - The position in the LUN to set.
 *
 * @return  None
 *
 * @author
 *  11/21/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_siots_setup_preread_fru_info(fbe_raid_siots_t *siots_p,
                                                 fbe_raid_fru_info_t * const read_fruts_ptr, 
                                                 fbe_raid_fru_info_t * const write_fruts_ptr,
                                                 fbe_u32_t array_pos)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    read_fruts_ptr->lba = write_fruts_ptr->lba;
    read_fruts_ptr->blocks = write_fruts_ptr->blocks;
    read_fruts_ptr->position = array_pos;

    if (!fbe_raid_geometry_position_needs_alignment(raid_geometry_p, array_pos)) {
        /* This is a normal write, so just init to cover the same range 
         * as the write. 
         */
    } else if (fbe_raid_geometry_io_needs_alignment(raid_geometry_p, read_fruts_ptr->lba, read_fruts_ptr->blocks)){
        fbe_raid_geometry_align_io(raid_geometry_p,
                                   &read_fruts_ptr->lba,
                                   &read_fruts_ptr->blocks);
    }
    return;
}
/**************************************
 * end fbe_raid_siots_setup_preread_fru_info()
 **************************************/
/*!***************************************************************
 * fbe_parity_468_setup_fruts()
 ****************************************************************
 * @brief
 *  This function will set up fruts for the state machine.
 *
 * @param siots_p   - ptr to the fbe_raid_siots_t
 * @param read_p    - pointer to read info
 * @param write_p   - pointer to write info
 * @param memory_info_p - Pointer to memory requet information
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_parity_468_setup_fruts(fbe_raid_siots_t * siots_p, 
                                           fbe_raid_fru_info_t *read_p,
                                           fbe_raid_fru_info_t *write_p,
                                           fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_u16_t data_pos;
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);

    /* Initialize the read fruts for data drives.
     */
    for (data_pos = 0; data_pos < siots_p->data_disks + parity_drives; data_pos++)
    {
        read_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p,
                                                       &siots_p->read_fruts_head,
                                                       memory_info_p);
        if (RAID_COND (read_fruts_ptr == NULL))
        {
             fbe_raid_siots_trace(siots_p, 
                                  FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "parity: read_fruts_ptr == NULL\n");
             return (FBE_STATUS_GENERIC_FAILURE);
        }
        status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                  read_p->lba,
                                  read_p->blocks,
                                  read_p->position);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail to init rd_fruts 0x%p:stat=0x%x,siots=0x%p\n",
                                 read_fruts_ptr, status, siots_p);
            return status;
        }

        if (read_p->blocks == 0)
        {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "parity: verify read blocks is 0x%llx lba: %llx pos: %d\n",
                                  (unsigned long long)read_p->blocks,
				  (unsigned long long)read_p->lba,
				  read_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        write_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p,
                                                       &siots_p->write_fruts_head,
                                                       memory_info_p);
        if (RAID_COND(write_fruts_ptr == NULL))
        {
             fbe_raid_siots_trace(siots_p, 
                                  FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "parity: write_fruts_ptr == NULL\n");
             return(FBE_STATUS_GENERIC_FAILURE);
        }

        status = fbe_raid_fruts_init_fruts(write_fruts_ptr,
                                  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                  write_p->lba,
                                  write_p->blocks,
                                  write_p->position);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail to init wt_fruts 0x%p:stat=0x%x,siots=0x%p\n",
                                 write_fruts_ptr, status, siots_p);
            return status;
        }
        /* This original write size will be needed later when we do the XOR.
         */
        write_fruts_ptr->write_preread_desc.lba = write_p->lba;
        write_fruts_ptr->write_preread_desc.block_count = write_p->blocks;

        if (write_p->blocks == 0)
        {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "parity: verify write blocks is 0x%llx lba: %llx pos: %d\n",
                                  (unsigned long long)write_p->blocks,
				  (unsigned long long)write_p->lba,
				  write_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        read_p++;
        write_p++;
    }

    return FBE_STATUS_OK;
}
/* end of fbe_parity_468_setup_fruts() */

/***************************************************************************
 * fbe_parity_468_calculate_num_sgs()
 ***************************************************************************
 * @brief
 *  This function counts the number of each type of sg needed.
 *  This function is executed from the RAID 5/RAID 3 468 state machine.
 *
 * @param siots_p - Pointer to siots
 * @param num_sgs_p - Pointer of array of each sg type length FBE_RAID_MAX_SG_TYPE.
 * @param read_p - array of read info.
 * @param write_p - array of write info.
 * @param b_generate - True if in generate and should not print certain errors.
 *
 * @return fbe_status_t
 *
 **************************************************************************/

fbe_status_t fbe_parity_468_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                              fbe_u16_t *num_sgs_p,
                                              fbe_raid_fru_info_t *read_p,
                                              fbe_raid_fru_info_t *write_p,
                                              fbe_bool_t b_generate)
{
    fbe_u16_t data_pos;    /* data position in array */
    fbe_block_count_t mem_left = 0;    /* sectors unused in last buffer */
    fbe_u32_t sg_count_vec[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];    /* S/G elements per data drive */
    fbe_u32_t pr_sg_count;    /* pre-read counts for all drives. */
    fbe_block_count_t pr_mem_left;    /* Mem left after counted for pre-reads. */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t write_log_header_count = 0;
    fbe_raid_fru_info_t *orig_read_p = read_p;

    /* Check number of parity drives that are supported. R5=1, R6=2
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u32_t read_sg_count = 0;
    fbe_u32_t write_sg_count = 0;
    fbe_sg_element_with_offset_t sg_desc;    /* tracks current location of data in S/G list */
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Check if we need to allocate and plant write log header buffers
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD))
    {
        fbe_u32_t journal_log_hdr_blocks;
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);
        write_log_header_count = 2;

        /* Since we're going to plant all the write fruts write log header buffers first in fbe_parity_mr3_setup_sgs,
         *  we need to skip over them here so the rest of these plants end up in the same place relative to buffer
         *  boundaries.  Since mem_left just tracks boundaries, it's ok if it's equal to 0 or page_size.
         */
        mem_left = siots_p->data_page_size - (((siots_p->data_disks + parity_drives) * journal_log_hdr_blocks) % siots_p->data_page_size);
    }

    /* Clear out the vectors of sg sizes per position.
     */
    for (data_pos = 0; data_pos < (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1); data_pos++)
    {
        sg_count_vec[data_pos] = 0;
    }

    if (RAID_COND(write_p == NULL))
	{
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: write_p fruts is null. write: %p\n", 
                             write_p);
		return FBE_STATUS_GENERIC_FAILURE;
	}
    /* Count for the pre-reads of data.
     */
    while (read_p->position != siots_p->parity_pos) {
        fbe_u32_t data_pos;

        if (read_p == NULL) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: read_p fruts is null. write: 0x%llx\n", 
                                 (unsigned long long)write_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (read_p->position == siots_p->parity_pos) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: read pos %d is parity\n", read_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = FBE_RAID_DATA_POSITION(read_p->position,
                                        siots_p->parity_pos,
                                        fbe_raid_siots_get_width(siots_p),
                                        parity_drives,
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }
        if (data_pos >= (fbe_raid_siots_get_width(siots_p) - parity_drives)) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: data position %d unexpected %d %d\n", 
                                 data_pos, fbe_raid_siots_get_width(siots_p), parity_drives);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Determine the size of SG list needed for the 
         * pre-read and put the sg_id address on the sg_id_list.
         */
        read_sg_count = 0;
        mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks,
                                                    siots_p->data_page_size,
                                                    mem_left,
                                                    &read_sg_count);

        status = fbe_raid_fru_info_set_sg_index(read_p, read_sg_count, !b_generate);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail to set sg index:stat=0x%x,siots=0x%p,read=0x%p\n", 
                                 status, siots_p, read_p);
            return status;
        }
        num_sgs_p[read_p->sg_index]++;
        read_p++;
    }    /* end while fruts position != parity_pos */

    if (!fbe_raid_siots_is_buffered_op(siots_p))
    {
        fbe_u32_t data_pos;
        /* Setup the sg descriptor to point to the "data" we will use
         * for this operation. 
         */
        status = fbe_raid_siots_setup_sgd(siots_p, &sg_desc);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to setup sg: status  = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }
        /* Count write sgs for a normal 468.
         */
        status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                        siots_p->parity_pos,
                                        fbe_raid_siots_get_width(siots_p),
                                        parity_drives,
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }


        status = fbe_raid_scatter_cache_to_bed(siots_p->start_lba,
                                               siots_p->xfer_count,
                                               data_pos,
                                               fbe_raid_siots_get_width(siots_p) - parity_drives,
                                               fbe_raid_siots_get_blocks_per_element(siots_p),
                                               &sg_desc,
                                               sg_count_vec);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to scatter cache: status = 0x%x, siots_p = 0x%p "
                                 "data_pos = 0x%x\n",
                                 status, siots_p, data_pos);
            return status;
        }
    }
    else
    {
        fbe_u32_t data_pos;
        status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                        siots_p->parity_pos,
                                        fbe_raid_siots_get_width(siots_p),
                                        parity_drives,
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }
        mem_left = fbe_raid_sg_scatter_fed_to_bed(siots_p->start_lba,
                                                  siots_p->xfer_count,
                                                  data_pos,
                                                  fbe_raid_siots_get_width(siots_p) - parity_drives,
                                                  fbe_raid_siots_get_blocks_per_element(siots_p),
                                                  (fbe_block_count_t)siots_p->data_page_size,
                                                  mem_left,
                                                  sg_count_vec);
    }
    
    /*
     * Stage 2: This operation must supply a set of S/G lists
     *          which reference buffers from the BM into which
     *          the older data residing on the disk may be
     *          pre-read, one list per disk.
     *
     *          These S/G lists will be referenced when 
     *          computing parity/writestamps from the pre-read data.
     */

    if (RAID_COND(write_p == NULL)) {
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: write_p fruts is null. write: %p\n", 
                             write_p);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    /* Now save the write counts from sg_count_vec into the write_p
     */
    read_p = orig_read_p;
    while (write_p->position != siots_p->parity_pos) {
        fbe_u32_t data_pos;

        if (read_p == NULL) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: read_p fruts is null. write: %p\n", 
                                 write_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (read_p->position == siots_p->parity_pos) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: read pos %d is parity\n", read_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (read_p->position != write_p->position) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: read pos %d != write_pos %d\n", 
                                 read_p->position, write_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = FBE_RAID_DATA_POSITION(read_p->position,
                                        siots_p->parity_pos,
                                        fbe_raid_siots_get_width(siots_p),
                                        parity_drives,
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }
        if (data_pos >= (fbe_raid_siots_get_width(siots_p) - parity_drives)) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: data position %d unexpected %d %d\n", 
                                 data_pos, fbe_raid_siots_get_width(siots_p), parity_drives);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* If the write is unaligned, then add on a constant number of sgs.
         * We might need these as we setup the write SG if we have an extra pre or post area due to alignment. 
         */
        if (fbe_raid_geometry_position_needs_alignment(raid_geometry_p, write_p->position)){
            sg_count_vec[data_pos] += FBE_RAID_EXTRA_ALIGNMENT_SGS;
        }
        /* Put the totals we calculated above 
         * for the write data onto the sg_id list.
         */
        if (sg_count_vec[data_pos] == 0) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: sg_count_vec for data_pos: %d is zero. \n", data_pos);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* add header count to the sg count, in case we need to prepend a write log header */
        status = fbe_raid_fru_info_set_sg_index(write_p, sg_count_vec[data_pos] + write_log_header_count, !b_generate);

        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail to set sg index:stat=0x%x,siots=0x%p, "
                                 "write=0x%p\n", 
                                 status, siots_p, write_p);
            return(status);
        }
        num_sgs_p[write_p->sg_index]++;
        write_p++;
        read_p++;

        if (write_p == NULL) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: write_p fruts is null. read: %p\n", 
                                 read_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }    /* end while fruts position != parity_pos */
    /*
     * Stage 3: This operation must supply an S/G list which
     *          references buffers from the BM into which
     *          the old parity residing on the disk may be pre-read
     *
     *          We use a single S/G list first to read parity from the
     *          disk, then again to write the new parity onto the disk.
     *          
     */
    if (write_p->blocks != siots_p->parity_count)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: write blocks %llu not parity count %llu.\n", 
                             (unsigned long long)write_p->blocks,
			     (unsigned long long)siots_p->parity_count);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    pr_sg_count = 0;

    /* Count for parity pre-read.
     */
    if (read_p->lba != write_p->lba)
    {
        mem_left = fbe_raid_sg_count_uniform_blocks(write_p->lba - read_p->lba,
                                                    siots_p->data_page_size,
                                                    mem_left,
                                                    &pr_sg_count);
    }
    /* Next count the num blocks for the pre-read.
     * This includes any trailing extra blocks.
     */
    pr_mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks - 
                                                   (write_p->lba - read_p->lba),
                                                   siots_p->data_page_size,
                                                   mem_left,
                                                   &pr_sg_count);
    
    status = fbe_raid_fru_info_set_sg_index(read_p, pr_sg_count, !b_generate);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail to set sg index:stat=0x%x,siots=0x%p,"
                             "read=0x%p\n", 
                             status, siots_p, read_p);

       return status;
    }

    num_sgs_p[read_p->sg_index]++;

    /* Handles stage 3 for row parity.
     */
    mem_left = fbe_raid_sg_count_uniform_blocks(write_p->blocks,
                                                siots_p->data_page_size,
                                                mem_left,
                                                &write_sg_count);

    /* If the write is unaligned, then add on a constant number of sgs.
     */
    if (fbe_raid_geometry_position_needs_alignment(raid_geometry_p, write_p->position)) {
        write_sg_count += FBE_RAID_EXTRA_ALIGNMENT_SGS;
    }

    /* add header count to the sg count, in case we need to prepend a write log header */
    if (!fbe_raid_fru_info_set_sg_index(write_p, write_sg_count + write_log_header_count, !b_generate))
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    num_sgs_p[write_p->sg_index]++;

    /* Set mem left to pre_read_mem_left since the pre-read 
     * can consume more blocks than the write. 
     */
    mem_left = pr_mem_left;

    /* If R6, handles stage 3 for diagonal parity.
     */                             
    if (parity_drives == 2)
    {
        pr_sg_count = 0;
        read_p++;
        write_p++;

        /* Count for parity pre-read.
         */
        if (read_p->lba != write_p->lba)
        {
            mem_left = fbe_raid_sg_count_uniform_blocks((fbe_u32_t)(write_p->lba - read_p->lba),
                                                        siots_p->data_page_size,
                                                        mem_left,
                                                        &pr_sg_count);
        }
        /* Next count the num blocks for the pre-read.
         * This includes any trailing extra blocks.
         */
        pr_mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks - 
                                                       (fbe_u32_t)(write_p->lba - read_p->lba),
                                                       siots_p->data_page_size,
                                                       mem_left,
                                                       &pr_sg_count);
        if (!fbe_raid_fru_info_set_sg_index(read_p, pr_sg_count, !b_generate))
        {
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        num_sgs_p[read_p->sg_index]++;

        write_sg_count = 0;
        mem_left = fbe_raid_sg_count_uniform_blocks(write_p->blocks,
                                                    siots_p->data_page_size,
                                                    mem_left,
                                                    &write_sg_count);
        /* If the write is unaligned, then add on a constant number of sgs.
         */
        if (fbe_raid_geometry_position_needs_alignment(raid_geometry_p, write_p->position)) {
            write_sg_count += FBE_RAID_EXTRA_ALIGNMENT_SGS;
        }

        /* add header count to the sg count, in case we need to prepend a write log header */
        if (!fbe_raid_fru_info_set_sg_index(write_p, write_sg_count + write_log_header_count, !b_generate))
        {
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        num_sgs_p[write_p->sg_index]++;
    }
    return status;
}
/*****************************************
 * fbe_parity_468_calculate_num_sgs
 *****************************************/
/***************************************************************************
 * fbe_parity_468_validate_sgs     
 ***************************************************************************
 * @brief
 *  This function is responsible for verifying the
 *  consistency of 468 sg lists
 *
 * @param siots_p - pointer to SUB_IOTS data
 *
 * @return fbe_status_t
 *
 * @author
 *  4/18/00:  Rob Foley -- Created.
 *
 **************************************************************************/

static fbe_status_t fbe_parity_468_validate_sgs(fbe_raid_siots_t * siots_p)
{
    fbe_block_count_t total_sg_blocks = 0; /* Total blocks in all sgs. */
    fbe_block_count_t total_fruts_blocks = 0; /* Total blocks in the fruts block counts. */
    fbe_block_count_t curr_sg_blocks;
    fbe_u32_t sgs = 0;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t *sg_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

    /* Validate the consistency of both write and read fruts.
     * We will verify that both write and read fruts
     * 1) reference the same number of blocks,
     * 2) have sgs that reference the correct number of blocks.
     */
    while (read_fruts_ptr)
    {
        sgs = 0;
        if (read_fruts_ptr->sg_id != NULL)
        {
            if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)read_fruts_ptr->sg_id))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "raid header invalid for fruts 0x%p, sg_id: 0x%px\n",
                                            read_fruts_ptr, read_fruts_ptr->sg_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        if (read_fruts_ptr->blocks == 0)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "read blocks zero for fruts 0x%p\n", read_fruts_ptr);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &sg_p);
        status = fbe_raid_sg_count_sg_blocks(sg_p, &sgs,&curr_sg_blocks );
        status = fbe_raid_sg_count_sg_blocks(fbe_raid_fruts_return_sg_ptr(read_fruts_ptr), 
                                             &sgs,
                                             &curr_sg_blocks );
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                        "parity: fail to count sg blocks: stat=0x%x,  siots=0x%p\n",
                                        status, siots_p);
            return status;
        }
        total_sg_blocks += curr_sg_blocks;

        /* Make sure the read has an SG for the correct block count */
        if (read_fruts_ptr->blocks != curr_sg_blocks)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                        "read blocks not match sg blocks for fruts 0x%p blocks:0x%llx sg:0x%llx\n",
                                        read_fruts_ptr,
				        (unsigned long long)read_fruts_ptr->blocks,
				        (unsigned long long)curr_sg_blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (siots_p->algorithm == R5_SMALL_468)
        {
            /* We get the sg from the caller.  It is not guaranteed to be null terminated.  Make sure it has enough
             * blocks. 
             */
            status = fbe_raid_sg_count_blocks_limit(sg_p, &sgs, write_fruts_ptr->blocks, &curr_sg_blocks );
            if (RAID_COND(curr_sg_blocks < write_fruts_ptr->blocks))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: client sg blocks %d < fruts blocks %d siots=0x%p\n",
                                     (int)curr_sg_blocks, (int)write_fruts_ptr->blocks, siots_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* It is possible for the sg blocks to be > write fruts blocks since it is from the client.
             */
            curr_sg_blocks = write_fruts_ptr->blocks;
        }
        else
        {
            fbe_raid_fruts_get_sg_ptr(write_fruts_ptr, &sg_p);
            status = fbe_raid_sg_count_sg_blocks(sg_p, &sgs, &curr_sg_blocks);
        }

        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: fail to count sg blocks:stat=0x%x,siots=0x%p\n",
                                 status, siots_p);
            return status;
        }
        total_sg_blocks += curr_sg_blocks;
        if (write_fruts_ptr->sg_id != NULL)
        {
            if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)write_fruts_ptr->sg_id))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                            "raid header invalid for write fruts 0x%px, sg_id: 0x%px\n",
                                            read_fruts_ptr, read_fruts_ptr->sg_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        if (write_fruts_ptr->blocks != curr_sg_blocks)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                        "write blocks not match sg blocks for fruts 0x%p blocks:0x%llx sg:0x%llx\n",
                                        write_fruts_ptr,
				        (unsigned long long)write_fruts_ptr->blocks,
				        (unsigned long long)curr_sg_blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (read_fruts_ptr->blocks < curr_sg_blocks)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                        "read blocks less than write sg blocks for fruts 0x%p blocks:0x%llx sg:0x%llx\n",
                                        read_fruts_ptr,
				        (unsigned long long)read_fruts_ptr->blocks,
				        (unsigned long long)curr_sg_blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Total the number of blocks the fruts plan to transfer.
         */
        total_fruts_blocks += write_fruts_ptr->blocks;
        total_fruts_blocks += read_fruts_ptr->blocks;

        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
    }

    /* Make sure the total blocks referenced by the SGs matches the total blocks
     * referenced by the fruts. 
     */
    if (total_sg_blocks != total_fruts_blocks)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                    "total sg blocks not equal to total fruts blocks %llu %llu\n",
                                    (unsigned long long)total_sg_blocks,
			            (unsigned long long)total_fruts_blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Aligned to optimal block size case.
     * Make sure the total sg blocks and total fruts blocks is equal 
     * to the total data we are writing plus the size of the parity data. 
     */
    if (total_sg_blocks < (siots_p->xfer_count + 
                           (siots_p->parity_count * fbe_raid_siots_parity_num_positions(siots_p))))
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                    "Not enough blocks found in sgs %llu\n",
                                    (unsigned long long)total_sg_blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/* end fbe_parity_468_validate_sgs() */

/***************************************************************************
 * fbe_parity_468_setup_parity_sgs()
 ***************************************************************************
 * @brief
 *  This function is responsible for initializing the S/G
 *  lists associated with the parity position(s).
 *
 *  This function is executed from the RAID 5/RAID 3 468 state
 *  machine once resources (i.e. memory) are allocated.
 *
 * @param   siots_p - pointer to SUB_IOTS data
 * @param   memory_info_p - Pointer to memory request information
 * @param   read_fruts_ptr - Pointer to parity read fruts
 * @param   write_fruts_ptr - Pointer to parity write fruts
 * @param   read2_mem_pp - Address of pointer to second parity read buffer
 * @param   read2_mem_left_p - Pointer to bytes left in second parity read buffer
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  8/2/99:  Rob Foley -- Created.
 *
 **************************************************************************/

static fbe_status_t fbe_parity_468_setup_parity_sgs(fbe_raid_siots_t * siots_p,
                                                    fbe_raid_memory_info_t *memory_info_p,
                                                    fbe_raid_fruts_t *read_fruts_ptr,
                                                    fbe_raid_fruts_t *write_fruts_ptr,
                                                    fbe_raid_memory_element_t **read2_mem_pp,
                                                    fbe_u32_t *read2_mem_left_p)

{
    fbe_status_t            status;
    fbe_raid_memory_element_t *read_mem_p = NULL;  /* Saves read buffer information for parity position*/
    fbe_u32_t               read_mem_left = 0;
    fbe_sg_element_t       *sgl_ptr;
    fbe_u16_t sg_total = 0;
    fbe_u32_t dest_byte_count =0;

    /* Validate this is is a parity position
     */
    if (RAID_DBG_COND(!fbe_raid_siots_is_parity_pos(siots_p, read_fruts_ptr->position)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                            "read pos %d != parity_pos %d\n", 
                            read_fruts_ptr->position, siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_COND(read_fruts_ptr->position != write_fruts_ptr->position))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                            "read pos %d != write pos %d\n", 
                            read_fruts_ptr->position, write_fruts_ptr->position);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_COND(write_fruts_ptr->lba < read_fruts_ptr->lba))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                            "write lba %llx < read lba %llx\n", 
                            (unsigned long long)write_fruts_ptr->lba,
			    (unsigned long long)read_fruts_ptr->lba);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Assign the read blocks from the beginning to the end.
     * We use a different sgd so that the write will start from the
     * right location.
     */
    sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);

    /* The belief is that this below code is leftover from BSV, so we commented it out.
     */
#if 0
    /* First do the difference between read and write, so that we
     * advance the buffer_sgd to where we want the write to start.
     */
    if (read_fruts_ptr->lba != write_fruts_ptr->lba)
    {
        if(!fbe_raid_get_checked_byte_count((write_fruts_ptr->lba - read_fruts_ptr->lba),
                                                                         &dest_byte_count))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                               "%s: byte count exceeding 32bit limit \n",
                                               __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        /* Assign newly allocated memory for the whole verify region.
         */
        sg_total = fbe_raid_sg_populate_with_memory(memory_info_p, sgl_ptr, dest_byte_count);
        if (sg_total == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                "%s: failed to populate sg for: siots = 0x%p\n",
                                                 __FUNCTION__,siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        sgl_ptr += sg_total;
        if (RAID_DBG_COND(!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)read_fruts_ptr->sg_id)))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                "read sg id (0x%p)header not valid\n", read_fruts_ptr->sg_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
#endif
    /* Now do the read all the way to the end.  Save the current buffer
     * information since we use the same buffers for the parity write.
     */
    read_mem_p = fbe_raid_memory_info_get_current_element(memory_info_p);
    read_mem_left = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);

    if(!fbe_raid_get_checked_byte_count((read_fruts_ptr->blocks - (write_fruts_ptr->lba - read_fruts_ptr->lba)),
                                                                      &dest_byte_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                           "%s: byte count exceeding 32bit limit \n",
                                           __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    sg_total = fbe_raid_sg_populate_with_memory(memory_info_p, sgl_ptr, dest_byte_count);
    if (sg_total == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                "%s: failed to populate sg for: siots = 0x%p\n",
                 __FUNCTION__,siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_ENABLED)
    {
        status = fbe_raid_fruts_validate_sg(read_fruts_ptr);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail to valid sg:siots=0x%p,rd_fruts=0x%p,stat=0x%x\n", 
                                 siots_p, read_fruts_ptr, status);
            return status;
        }
    }

    /* Save the current buffer information for the second parity position
     */
    *read2_mem_pp = fbe_raid_memory_info_get_current_element(memory_info_p);
    *read2_mem_left_p = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);

    /* Now populate the parity write buffers with the read buffers.  We
     * need to reset the buffer pointers back to the read buffers.
     */
    sgl_ptr = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);
    fbe_raid_memory_info_set_current_element(memory_info_p, read_mem_p);
    fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, read_mem_left);

    if(!fbe_raid_get_checked_byte_count(write_fruts_ptr->blocks, &dest_byte_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                           "%s: byte count exceeding 32bit limit \n",
                                           __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    sg_total = fbe_raid_sg_populate_with_memory(memory_info_p, sgl_ptr, dest_byte_count);
    if (sg_total == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                            "%s: failed to populate sg for: siots = 0x%p\n",
                                             __FUNCTION__,siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_ENABLED)
    {
        status = fbe_raid_fruts_validate_sg(write_fruts_ptr);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail to valid sg:siots=0x%p,wt_fruts=0x%p,stat=0x%x\n", 
                                 siots_p, write_fruts_ptr, status);
            return status;
        }
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_parity_468_setup_parity_sgs()
 *****************************************/

/***************************************************************************
 * fbe_parity_468_setup_sgs()
 ***************************************************************************
 * @brief
 *  This function is responsible for initializing the S/G
 *  lists allocated earlier by bem_468_alloc_sgs(). A
 *  full description of these lists (and 468) can be found
 *  in that code.
 *
 *  This function is executed from the RAID 5/RAID 3 468 state
 *  machine once resources (i.e. memory) are allocated.
 *
 * @param   siots_p - pointer to SUB_IOTS data
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  8/2/99:  Rob Foley -- Created.
 *
 **************************************************************************/

fbe_status_t fbe_parity_468_setup_sgs(fbe_raid_siots_t * siots_p,
                                      fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t            status;
    fbe_raid_fruts_t       *read_fruts_ptr = NULL;
    fbe_raid_fruts_t       *write_fruts_ptr = NULL;
    fbe_sg_element_with_offset_t host_sgd;      /* Used to reference our allocated buffers */
    fbe_raid_memory_element_t *read2_mem_p = NULL;  /* Saves read buffer information for second parity position*/
    fbe_u32_t               read2_mem_left = 0;
    fbe_sg_element_t        *current_sg_ptr = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* The w_sgl_ptr is used to hold the pointers to the write sgs.
     */
    fbe_sg_element_t *w_sgl_ptr[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];

    /* Check number of parity drives that are supported. R5=1, R6=2
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);


    /* If write log header required, need to populate the first sg_list element with the first buffer
     * before doing the rest, or the first sg_list element will cover the header and more
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD))
    {
        fbe_u16_t num_sg_elements;
        fbe_u32_t journal_log_hdr_blocks;

        fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);

        fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

        while (write_fruts_ptr)
        {
            current_sg_ptr = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);
            num_sg_elements = fbe_raid_sg_populate_with_memory(memory_info_p, 
                                                               current_sg_ptr, 
                                                               journal_log_hdr_blocks * FBE_BE_BYTES_PER_BLOCK);
            if (num_sg_elements != 1)
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: write log header num_sg_elements %d != 1, siots_p = 0x%p\n",
                                            num_sg_elements, siots_p);
            }

            write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
        } /* end while write_fruts_ptr*/

        /* Now set up the write fruts chain to skip the newly attached header */
        fbe_raid_fruts_chain_set_prepend_header(&siots_p->write_fruts_head, FALSE);
    }

    /* Now do the rest of the buffers */
    /* Obtain the locations of the S/G lists used
     * for the parity calculations/writes.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

    while (write_fruts_ptr->position != siots_p->parity_pos)
    {
        fbe_u32_t data_pos;
        current_sg_ptr = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);

        status = FBE_RAID_DATA_POSITION(write_fruts_ptr->position,
                                        siots_p->parity_pos,
                                        fbe_raid_siots_get_width(siots_p),
                                        parity_drives,
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }
        if (RAID_DBG_COND(data_pos >= (fbe_raid_siots_get_width(siots_p) - parity_drives)))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: data_pos %d > width %d - parity_drives %d.\n", 
                                 data_pos, fbe_raid_siots_get_width(siots_p), parity_drives);
            return FBE_STATUS_GENERIC_FAILURE; 
        }

        w_sgl_ptr[data_pos] = current_sg_ptr;

        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);

    } /* end while fruts position != parity  position*/


    /* Assign data to the S/G lists used for the data pre-reads.
     * If there are extra pre-reads for 4k, then these will also be included in the write.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    while(read_fruts_ptr->position != siots_p->parity_pos)
    {
        fbe_sg_element_with_offset_t sg_descriptor;
        fbe_u32_t current_data_pos;
        fbe_u32_t dest_byte_count =0;
        fbe_u16_t num_sg_elements;
        fbe_sg_element_t *sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);

        if(!fbe_raid_get_checked_byte_count(read_fruts_ptr->blocks, &dest_byte_count)) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                               "%s: byte count exceeding 32bit limit \n",
                                               __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        /* Assign newly allocated memory for the whole verify region.
         */
        num_sg_elements =  fbe_raid_sg_populate_with_memory(memory_info_p,
                                                     sgl_ptr, 
                                                     dest_byte_count);
        if (num_sg_elements == 0) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                    "%s: failed to populate sg for: siots = 0x%p\n",
                     __FUNCTION__,siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (read_fruts_ptr->lba != write_fruts_ptr->lba) {
            /* ******** <---- Read start
             * *      *            /\
             * *      * <--- This middle area needs to be added to the write.
             * *      *            \/
             * ******** <---- Write start
             * *      *
             * *      *
             * The write lba at this point represents where the user data starts. 
             * Our read starts before the new write data                  .
             * We will need to write out these extra blocks to align to 4k. 
             * Copy these extra blocks from the read to write sg. 
             */
            status = FBE_RAID_DATA_POSITION(read_fruts_ptr->position, siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p), parity_drives, &current_data_pos);

            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
                return status;
            }
            fbe_sg_element_with_offset_init(&sg_descriptor, 0, sgl_ptr, NULL);
            status = fbe_raid_sg_clip_sg_list(&sg_descriptor,
                                              w_sgl_ptr[current_data_pos],
                                              (fbe_u32_t)(write_fruts_ptr->lba - read_fruts_ptr->lba) * FBE_BE_BYTES_PER_BLOCK,
                                              &num_sg_elements);
            w_sgl_ptr[current_data_pos] += num_sg_elements;
        }
        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    /* Buffered operations need to allocate their own space for 
     * the data we will be writing.  Fill in the write sgs with 
     * the memory we allocated. 
     */
    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        fbe_u32_t data_pos;
        status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                        siots_p->parity_pos,
                                        fbe_raid_siots_get_width(siots_p),
                                        fbe_raid_siots_parity_num_positions(siots_p),
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }

        if (RAID_DBG_COND(siots_p->xfer_count > FBE_U32_MAX))
        {
            /* we do not expect the blocks to scatter to exceed 32bit limit here
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: xfer count 0x%llx exceeding 32bit limit , siots_p = 0x%p\n",
                                        (unsigned long long)siots_p->xfer_count,
					siots_p);
            
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = fbe_raid_scatter_sg_with_memory(siots_p->start_lba,
                                                 (fbe_u32_t)siots_p->xfer_count,
                                                 data_pos,
                                                 fbe_raid_siots_get_width(siots_p) - parity_drives,
                                                 fbe_raid_siots_get_blocks_per_element(siots_p),
                                                 memory_info_p,
                                                 w_sgl_ptr);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to scatter sg with memory: status = 0x%x, siots_p 0x%p "
                                        "data_pos = 0x%x, memory_info_p = 0x%p, w_sgl_ptr = 0x%p\n",
                                        status, siots_p, data_pos, memory_info_p, w_sgl_ptr);
            return status;
        }
    }
    else
    {
        fbe_u32_t data_pos;
        /* Setup the sg descriptor to point to the "data" we will use
         * for this operation. 
         */
        status = fbe_raid_siots_setup_sgd(siots_p, &host_sgd);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to setup sg: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }
        /*
         * Stage 3: Assign data to the S/G lists used for the partial
         *           product calculations. This is either buffered data
         *           from the FED or cache page locations in the write
         *           cache.
         */
        status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                        siots_p->parity_pos,
                                        fbe_raid_siots_get_width(siots_p),
                                        parity_drives,
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }

        status = fbe_raid_scatter_sg_to_bed(siots_p->start_lba,
                                           (fbe_u32_t)(siots_p->xfer_count),
                                           data_pos,
                                           fbe_raid_siots_get_width(siots_p) - parity_drives,
                                           fbe_raid_siots_get_blocks_per_element(siots_p),
                                           &host_sgd,
                                           w_sgl_ptr);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to scatter memory to sg : status: 0x%x, siots_p = 0x%p, "
                                        "data_pos = 0x%x, w_sgl_ptr = 0x%p\n",
                                        status, siots_p, data_pos, w_sgl_ptr);
            return (status);
        }
    }
    /* If there are extra pre-reads for 4k beyond the host write data,
     * then these will also be included in the write.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    while(read_fruts_ptr->position != siots_p->parity_pos) {
        fbe_sg_element_with_offset_t sg_descriptor;
        fbe_u32_t         current_data_pos;
        fbe_u16_t         num_sg_elements;
        fbe_sg_element_t *sgl_ptr;
        fbe_lba_t read_end, write_end;
        fbe_u32_t offset;
        read_end = read_fruts_ptr->lba + read_fruts_ptr->blocks - 1;
        write_end = write_fruts_ptr->lba + write_fruts_ptr->blocks - 1;

        if (read_end > write_end) {
            /* ******** <---- I/O start
             * *      *
             * *      *
             * ******** <---- Write end
             * *      *            /\
             * *      * <--- This middle area needs to be added to the write.
             * *      *            \/
             * ******** <---- Read end 
             *  
             * The write lba at this point represents where the user data starts 
             * Our read ends after the new write data ends. 
             * We will need to write out these extra blocks to align to 4k 
             * Copy these extra blocks from the read to write sg.
             */
            status = FBE_RAID_DATA_POSITION(read_fruts_ptr->position, siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p), parity_drives, &current_data_pos);

            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
                return status;
            }
            sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);
            /* The point to copy from is the end of the write I/O. 
             * Calculate the offset into the read sg list from the beginning to where the write ends. 
             */
            offset = (fbe_u32_t)(write_end - read_fruts_ptr->lba + 1) * FBE_BE_BYTES_PER_BLOCK;
            fbe_sg_element_with_offset_init(&sg_descriptor, offset, sgl_ptr, NULL);
            fbe_raid_adjust_sg_desc(&sg_descriptor);
            status = fbe_raid_sg_clip_sg_list(&sg_descriptor,
                                              w_sgl_ptr[current_data_pos],
                                              (fbe_u32_t)(read_end - write_end) * FBE_BE_BYTES_PER_BLOCK, /* Blocks to copy from read -> write */
                                              &num_sg_elements);
            w_sgl_ptr[current_data_pos] += num_sg_elements;
        }
        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
        /* Up until now the write fruts reference just the host data. 
         * Switch to reference the same range as the read, which includes 
         * any alignment pre-reads, which show up in read and write. 
         */
        fbe_raid_fruts_t *write_p = NULL;
        fbe_raid_fruts_t *read_p = NULL;
        fbe_raid_siots_get_read_fruts(siots_p, &read_p);
        fbe_raid_siots_get_write_fruts(siots_p, &write_p);
        while (read_p != NULL) {

            write_p->lba = read_p->lba;
            write_p->blocks = read_p->blocks;
            write_p = fbe_raid_fruts_get_next(write_p);
            read_p = fbe_raid_fruts_get_next(read_p);
        }
    }

    /*
     * Stage 4: Assign data to the S/G list used for the parity pre-read.
     *          The same sgs and buffers are used for the parity
     *          pre-read AND the parity write.
     *          Note how the read fruit gets the same sg_id as the write.
     */
    status = fbe_parity_468_setup_parity_sgs(siots_p,
                                             memory_info_p,
                                             read_fruts_ptr,
                                             write_fruts_ptr,
                                             &read2_mem_p,
                                             &read2_mem_left);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "parity:fail plant parity sgs:stat=0x%x,"
                                    "siots=0x%p>\n",
                                    status, siots_p);
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "mem_info=0x%p,read_fruts=0x%p,write_fruts=0x%p\n",
                                    memory_info_p, read_fruts_ptr, write_fruts_ptr);
        return (status);
    }

    /* If R6, handles stage 4 for diagonal parity.
     */                          
    if (parity_drives == 2)
    {
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);

        /* Copy the read_sgd since it clipped off more than the buffer_sgd.
         */
        fbe_raid_memory_info_set_current_element(memory_info_p, read2_mem_p);
        fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, read2_mem_left);

        /* Now plant the second parity position
         */
        status = fbe_parity_468_setup_parity_sgs(siots_p,
                                                 memory_info_p,
                                                 read_fruts_ptr,
                                                 write_fruts_ptr,
                                                 &read2_mem_p,
                                                 &read2_mem_left);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            /*Split trace to two lines*/
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity:fail plant diagonal parity sgs:stat=0x%x,"
                                        "siots=0x%p>\n",
                                        status, siots_p);
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mem_info=0x%p,read_fruts=0x%p,write_fruts=0x%p<\n",
                                        memory_info_p, read_fruts_ptr, write_fruts_ptr);
            return (status);
        }

    } /* end if second parity position */
    if (RAID_DBG_ENABLED)
    {
        /* Make sure our sgs are sane.
         */
        status = fbe_parity_468_validate_sgs(siots_p);
    }
    return status;
}
/*****************************************
 * fbe_parity_468_setup_sgs()
 *****************************************/

/****************************************************************
 * fbe_parity_468_setup_xor_vectors()
 ****************************************************************
 * @brief
 *  Setup the xor command's vectors for this r5 468 operation.
 *
 * @param siots_p - The siots to use when creating the xor command.
 * @param xor_write_p - The xor command to setup vectors in.
 *
 * @return fbe_status_t
 *
 * @author
 *  03/07/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_468_setup_xor_vectors(fbe_raid_siots_t *const siots_p, 
                                              fbe_xor_468_write_t * const xor_write_p)
{
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_s32_t fru_cnt;
    fbe_u32_t parity_disks;
    fbe_u32_t fru_index = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t width;

    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_geometry_get_num_parity(raid_geometry_p, &parity_disks);

    if (RAID_DBG_COND(!fbe_queue_is_empty(&siots_p->read2_fruts_head)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "read 2 queue not empty\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

    for (fru_cnt = (siots_p->data_disks + parity_disks);
        (fru_cnt-- > 0);
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr),
        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr))
    {
        fbe_u32_t data_pos;
        fbe_lba_t write_start_lba = write_fruts_ptr->write_preread_desc.lba;
        fbe_block_count_t write_block_count = write_fruts_ptr->write_preread_desc.block_count;

        if (read_fruts_ptr == NULL)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "read fruts is null\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (write_fruts_ptr == NULL)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "write fruts is null\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (write_fruts_ptr->write_preread_desc.block_count == 0 ||
            write_fruts_ptr->write_preread_desc.lba < read_fruts_ptr->lba) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "write data lba %llx < read lba %llx\n", 
                                 (unsigned long long)write_fruts_ptr->lba,
                                 (unsigned long long)read_fruts_ptr->lba);
        }
        if (RAID_DBG_ENA(read_fruts_ptr->position != write_fruts_ptr->position))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "read pos %d != write pos %d\n", 
                                 read_fruts_ptr->position, write_fruts_ptr->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_DBG_ENA(write_start_lba < read_fruts_ptr->lba))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "write lba %llx < read lba %llx\n", 
                                 (unsigned long long)write_fruts_ptr->lba,
				 (unsigned long long)read_fruts_ptr->lba);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* There are no guarantees that 
         * read_fruts_ptr->lba != write_start_lba only for aligned cases.
         * It is possible that it was an alignment case but then that situation changed.
         */

        xor_write_p->fru[fru_index].seed = write_start_lba;
        xor_write_p->fru[fru_index].count = write_block_count;
        xor_write_p->fru[fru_index].write_offset = 0;

        /* When we have an unaligned write with pre-read, we might have a read 
         * with a different range than the write.  The read range is always greater than 
         * or equal to the write range. 
         */
        if (RAID_DBG_ENA(write_start_lba < read_fruts_ptr->lba))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "write lba %llx < read lba %llx\n", 
                                 (unsigned long long)write_start_lba,
                                 (unsigned long long)read_fruts_ptr->lba);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_DBG_ENA(read_fruts_ptr->lba > write_fruts_ptr->lba))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "read lba %llx > write lba %llx\n", 
                                 (unsigned long long)read_fruts_ptr->lba,
				 (unsigned long long)write_fruts_ptr->lba);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_DBG_ENA(read_fruts_ptr->blocks < write_fruts_ptr->blocks))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "read blocks %llu < write blocks %llu\n", 
                                 (unsigned long long)read_fruts_ptr->blocks,
				 (unsigned long long)write_fruts_ptr->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* The read offset is the difference between the start of the read and the start 
         * of the write. 
         */
        xor_write_p->fru[fru_index].read_offset = (fbe_u32_t)(write_start_lba - read_fruts_ptr->lba);
        xor_write_p->fru[fru_index].read_count = read_fruts_ptr->blocks;
        xor_write_p->fru[fru_index].position_mask = (1 << read_fruts_ptr->position);
        xor_write_p->fru[fru_index].read_sg = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);
        xor_write_p->fru[fru_index].write_sg = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);

        if (xor_write_p->fru[fru_index].read_offset > 0){
            /* Our write sg list needs to get incremented to the point where the write data starts.
             */
            fbe_sg_element_with_offset_t sg_descriptor;
            fbe_sg_element_with_offset_init(&sg_descriptor, 
                                            xor_write_p->fru[fru_index].read_offset * FBE_BE_BYTES_PER_BLOCK, 
                                            xor_write_p->fru[fru_index].write_sg, NULL);
            fbe_raid_adjust_sg_desc(&sg_descriptor);
            xor_write_p->fru[fru_index].write_sg = sg_descriptor.sg_element;
            xor_write_p->fru[fru_index].write_offset = sg_descriptor.offset / FBE_BE_BYTES_PER_BLOCK;
        }

        /* Determine the data position of this fru and save it for possible later
         * use in RAID 6 algorithms.
         */
        status = 
            FBE_RAID_EXTENT_POSITION(write_fruts_ptr->position,
                               siots_p->parity_pos,
                               width,
                               parity_disks,
                               &data_pos);
        xor_write_p->fru[fru_index].data_position = data_pos;
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }
        fru_index++;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * fbe_parity_468_setup_xor_vectors()
 ******************************************/

/****************************************************************
 * fbe_parity_468_setup_xor()
 ****************************************************************
 * @brief
 *  This function sets up the xor command for an 468 operation.
 *
 * @param siots_p - The fbe_raid_siots_t for this operation.
 * @param xor_write_p - The fbe_xor_command_t to setup.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/10/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_468_setup_xor(fbe_raid_siots_t *const siots_p, 
                                      fbe_xor_468_write_t * const xor_write_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u32_t width;

    /* First init the command.
     */
    xor_write_p->status = FBE_XOR_STATUS_INVALID;

    /* Setup all the vectors in the xor command.
     */
    status = fbe_parity_468_setup_xor_vectors(siots_p, xor_write_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail to setup xor vec:stat=0x%x,siots=0x%p,xor_wt=0x%p\n", 
                             status, siots_p, xor_write_p);

        return status;
    }

    /* Default option flags is FBE_XOR_OPTION_CHK_CRC.  We always check the
     * checksum of data read from disk for a write operation.
     */
    xor_write_p->option = FBE_XOR_OPTION_CHK_CRC;

    /* Degraded operations allow previously invalidated data
     */
    if ( siots_p->algorithm == R5_DEG_468 )
    {
        /* Degraded 468 allows invalid sectors in pre-read data, since we
         * already performed a reconstruct. 
         */
        xor_write_p->option |= FBE_XOR_OPTION_ALLOW_INVALIDS;
    }

	/* Let XOR know if this is a corrupt data for
	 * logging purposes.  Use the opcode to check for the 
     * Corrupt Data operation.  If we are degraded,
	 * the algorithm may have changed.
	 */
	fbe_raid_iots_get_opcode(iots_p, &opcode);
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA )
	{
		xor_write_p->option |= FBE_XOR_OPTION_CORRUPT_DATA;
	}
    if (fbe_raid_iots_is_corrupt_crc(iots_p) == FBE_TRUE)
    {
        /* Need to log corrupted sectors
         */
        xor_write_p->option |= FBE_XOR_OPTION_CORRUPT_CRC;
    }

    if (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE){
        xor_write_p->flags = FBE_XOR_468_FLAG_NONE;
    }
    else {
        xor_write_p->flags = FBE_XOR_468_FLAG_BVA_WRITE;
    }
  
    /* Setup the rest of the command including the parity and data disks and
     * offset.
     */
    xor_write_p->data_disks = siots_p->data_disks;
    fbe_raid_geometry_get_num_parity(raid_geometry_p, &xor_write_p->parity_disks);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    xor_write_p->array_width = width;
    xor_write_p->eboard_p = fbe_raid_siots_get_eboard(siots_p);
    xor_write_p->eregions_p = &siots_p->vcts_ptr->error_regions;
    xor_write_p->offset = xor_write_p->eboard_p->raid_group_offset;
    return FBE_STATUS_OK;
}
/******************************************
 * fbe_parity_468_setup_xor()
 ******************************************/
/***************************************************************************
 *      fbe_parity_468_xor()
 ***************************************************************************
 * @brief
 *   This routine sets up S/G list pointers to interface with
 *   the XOR routine for parity operation for an 468 write.
 *
 * @param siots_p - pointer to fbe_raid_siots_t
 *
 * @return fbe_status_t
 *
 * @author
 *   06/29/99:  Rob Foley -- Created, borrowed heavily from bem_468_xor().
 *   05/04/00:  MPG -- Added XOR driver support
 *
 **************************************************************************/

fbe_status_t fbe_parity_468_xor(fbe_raid_siots_t * const siots_p)
{
    /* This is the command we will submit to xor.
     */
    fbe_xor_468_write_t xor_write;

    /* Status of starting this operation.
     */
    fbe_status_t status;

    /* Setup the xor command completely.
     */
    status = fbe_parity_468_setup_xor(siots_p, &xor_write);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to setup xor command: status = 0x%x,  siots_p = 0x%p\n", 
                             status, siots_p);

        return status;
    }

    /* Send xor command now. 
     */ 
    status = fbe_xor_lib_parity_468_write(&xor_write);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to send xor command: status = 0x%x, siots_p = 0x%p\n", 
                             status, siots_p);

        return status;
    }

    /* Save status.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, xor_write.status);

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_468_xor
 *****************************************/
 
/*!***************************************************************
 *  fbe_parity_is_preread_degraded()
 ****************************************************************
 * @brief
 *  This function determines if we are degraded during the pre-read.  
 *
 * @param siots_p - current siots
 * @param b_return - value to return fbe_bool_t
 *                   FBE_TRUE - Preread degraded.
 *                   FBE_FALSE - Preread is not degraded.
 * 
 * @return fbe_status_t
 *   FBE_STATUS_GENERIC_FAILURE - error
 *   FBE_STATUS_OK - success
 *
 * Notes:
 *  We assume this only use for Raid 6.
 *
 * @author
 *  04/10/06 - Created. CLC
 *
 ****************************************************************/
 fbe_status_t fbe_parity_is_preread_degraded(fbe_raid_siots_t *const siots_p,
                                             fbe_bool_t *b_return)
{
    fbe_raid_fruts_t *read_fruts_ptr = NULL;   /* Ptr to current fruts in read chain. */
    fbe_raid_fruts_t *read2_fruts_ptr = NULL;  /* Ptr to current fruts in read2 chain. */
    fbe_u32_t first_dead_pos;
    fbe_u32_t second_dead_pos;

    /* Refresh this siots view of the iots.
     */
    fbe_parity_check_for_degraded(siots_p);
    
    first_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);
    second_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);

    /* Set b_return to FBE_FALSE by default, it will be set to FBE_TRUE if a degraded
     * position is found.
     */
    *b_return = FBE_FALSE;
    
    /* This function only for rcw and mr3 and 468.
     */
    if ((siots_p->algorithm != R5_RCW) && 
        (siots_p->algorithm != R5_DEG_RCW) &&
        (siots_p->algorithm != R5_MR3) && 
        (siots_p->algorithm != R5_DEG_MR3) &&
        (siots_p->algorithm != R5_468) && 
        (siots_p->algorithm != R5_SMALL_468) && 
        (siots_p->algorithm != R5_DEG_468) &&
        (siots_p->algorithm != R5_CORRUPT_DATA))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "algorithm %d not expected\n", 
                             siots_p->algorithm);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (first_dead_pos != FBE_RAID_INVALID_DEAD_POS)
    {
        /* Loop over siots_p->read_fruts_ptr and siots_p->reads_fruts_ptr
         * fruts in the chain and if any hit the dead position, then return FBE_TRUE
         * otherwise return FBE_FALSE.
         */

        fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
        while ((read_fruts_ptr != NULL) &&
               (*b_return == FBE_FALSE))
        {   
            if ((read_fruts_ptr->position == first_dead_pos) ||
                    (read_fruts_ptr->position == second_dead_pos))
            {
                /* Found the dead position in the preread.
                 */
                *b_return = FBE_TRUE;
            }

            read_fruts_ptr = fbe_raid_fruts_get_next( read_fruts_ptr );
        }

        fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);
        while ((read2_fruts_ptr != NULL) &&
               (*b_return == FBE_FALSE))
        {   
            if (read2_fruts_ptr->position == first_dead_pos ||
                    read2_fruts_ptr->position == second_dead_pos )
            {
                /* Found the dead position in the preread.
                 */
                *b_return = FBE_TRUE;
            }

            read2_fruts_ptr = fbe_raid_fruts_get_next(read2_fruts_ptr );
        }
    }
    
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_is_preread_degraded
 *****************************************/

/****************************************************************
 *  fbe_parity_468_degraded_count()
 ****************************************************************
 * @brief
 *  This function counts the number of degraded positions in
 *  a 468 I/O.  
 *
 * @param siots_p - current siots
 *
 * @return
 *  fbe_bool_t FBE_TRUE - Preread degraded.
 *      FBE_FALSE - Preread is not degraded.
 *
 * ASSUMPTIONS:
 *  We assume this only use for Raid 6.
 *
 * @author
 *  05/08/06 - Created. Rob Foley
 *
 ****************************************************************/
fbe_u16_t fbe_parity_468_degraded_count(fbe_raid_siots_t *const siots_p)
{
    fbe_raid_fruts_t *read_fruts_ptr = NULL;   /* Ptr to current fruts in read chain. */
    
    fbe_u32_t first_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);
    fbe_u32_t second_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);
    fbe_u16_t degraded_count = 0;
    
    if (first_dead_pos == FBE_RAID_INVALID_DEAD_POS)
    {
        /* There are no degraded positions and thus nothing for
         * us to do, so we will return.
         */
        return degraded_count;
    }
    
    /* Loop over siots_p->read_fruts_ptr and siots_p->reads_fruts_ptr
     * fruts in the chain and if any hit the dead position, then return FBE_TRUE
     * otherwise return FBE_FALSE.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    while (read_fruts_ptr)
    {   
        if ((read_fruts_ptr->position == first_dead_pos) ||
            (read_fruts_ptr->position == second_dead_pos))
        {
            /* Found the dead position in the preread.
             */
            degraded_count++;
        }
        
        read_fruts_ptr = fbe_raid_fruts_get_next( read_fruts_ptr );
    }
    
    return degraded_count;
}
/*****************************************
 * end fbe_parity_468_degraded_count
 *****************************************/

/***************************************************************************
 * fbe_parity_zero_buffers()
 ***************************************************************************
 * @brief
 *  This function zeros memory.
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 *   
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  4/14/2014 - Created. Rob Foley
 *
 **************************************************************************/

static fbe_status_t fbe_parity_zero_buffers(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_xor_zero_buffer_t zero_buffer;
    fbe_u32_t data_pos;

    zero_buffer.disks = siots_p->data_disks;
    zero_buffer.offset = fbe_raid_siots_get_eboard_offset(siots_p);
    zero_buffer.status = FBE_STATUS_INVALID;

    /* Fill out the zero buffer structure from the write fruts.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    for (data_pos = 0; data_pos < siots_p->data_disks; data_pos++)
    {
        fbe_sg_element_t *sg_p = NULL;
        fbe_lba_t write_start_lba = write_fruts_ptr->write_preread_desc.lba;
        fbe_block_count_t write_block_count = write_fruts_ptr->write_preread_desc.block_count;
        fbe_raid_fruts_get_sg_ptr(write_fruts_ptr, &sg_p);

        if (write_start_lba > write_fruts_ptr->lba) {
            /* Our write sg list needs to get incremented to the point where the write data starts.
             */
            fbe_sg_element_with_offset_t sg_descriptor;
            fbe_sg_element_with_offset_init(&sg_descriptor, 
                                            (fbe_u32_t)(write_start_lba - write_fruts_ptr->lba) * FBE_BE_BYTES_PER_BLOCK, 
                                            sg_p, NULL);
            fbe_raid_adjust_sg_desc(&sg_descriptor);
            zero_buffer.fru[data_pos].sg_p = sg_descriptor.sg_element;
            zero_buffer.fru[data_pos].offset = sg_descriptor.offset / FBE_BE_BYTES_PER_BLOCK;
        } else {
            zero_buffer.fru[data_pos].sg_p = sg_p;
            zero_buffer.fru[data_pos].offset = 0;
        }
        zero_buffer.fru[data_pos].count = write_block_count;
        zero_buffer.fru[data_pos].seed = write_start_lba;

        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
    }

    status = fbe_xor_lib_zero_buffer(&zero_buffer);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "raid: failed to zero buffer with status = 0x%x, siots_p = 0x%p\n",
                                    status, siots_p);
    }
    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, zero_buffer.status);

    return status;
}
/**************************************
 * end fbe_parity_zero_buffers()
 **************************************/
/***************************************************************************
 * fbe_parity_get_buffered_data()
 ***************************************************************************
 * @brief
 *  This gets the data for a buffered operation.
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 *   
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  4/14/2014 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_parity_get_buffered_data(fbe_raid_siots_t * const siots_p)
{
    fbe_payload_block_operation_opcode_t opcode;
    fbe_status_t status;

    fbe_raid_siots_get_opcode(siots_p, &opcode);

    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS))
    {
        status = fbe_parity_zero_buffers(siots_p);
    }
    else if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME)
    {
        /*! @note We don't support write sames yet.
         * When it is supported it simply copies from the user block to the 
         * destination blocks. 
         */
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "raid: %s errored because of unexpected opcode %d\n", __FUNCTION__, opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_parity_get_buffered_data()
 **************************************/
/*****************************************
 * end fbe_parity_468_util.c
 *****************************************/
