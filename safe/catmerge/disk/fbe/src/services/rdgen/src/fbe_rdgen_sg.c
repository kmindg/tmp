/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_sg.c
 ***************************************************************************
 *
 * @brief
 *  This file contains rdgen sg list functions.
 *
 * @version
 *   6/06/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "xorlib_api.h"
#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private_inlines.h"
#include "fbe/fbe_xor_api.h"
#include "fbe/fbe_data_pattern.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/***************************************************************************
 * fbe_rdgen_sg_populate_with_memory()
 ***************************************************************************
 * @brief
 *  This function is responsible for initializing the elements in
 *  one S/G list from the contents of another.
 *
 *  @param mem_p - Ptr to header to allocate from.
 *  @param mem_left_p - Number of bytes left to allocate in this buffer.
 *  @param page_size_bytes - Number of bytes in each buffer or page.
 *  @param dest_sgl_ptr -  ptr to start of S/G list to be initialized.
 *  @param dest_bytecnt -   number of bytes to be referenced.
 *  @param block_size - Number of bytes in each block.
 *  @param sgl_count - Number of sgs we used.
 *
 * @return
 *  fbe_status_t
 * 
 * @author
 *  10/11/2010 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_rdgen_sg_populate_with_memory(fbe_rdgen_memory_info_t *info_p,
                                               fbe_sg_element_t * dest_sgl_ptr,
                                               fbe_block_count_t dest_bytecnt,
                                               fbe_u32_t block_size,
                                               fbe_u32_t *sgl_count_p)
{
    fbe_u16_t sg_total = 0;    /* count of S/G elements copied */
    fbe_u32_t bytes_allocated;
    
    if(block_size ==0)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: block_size is zero.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((info_p->bytes_remaining == 0) && 
        ((info_p->memory_header_p == NULL) ||
         (info_p->memory_header_p->u.queue_element.next == NULL)))
    {
        /* No more source memory so there is nothing to copy.
         */
        if (dest_bytecnt == 0)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: dest_bytecnt is zero.\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        fbe_block_count_t bytes_to_allocate;
        fbe_u8_t *memory_p = NULL;
        /* Assign portions of the memory provided by the source to the entries in the
         * destination S/G list; assign only the number of bytes specified. 
         */
        while (dest_bytecnt > 0)
        {
            sg_total++;

            /* Allocate either the amount leftover or thet amount asked for, whichever 
             * is smaller.  This allows us to not waste any memory. 
             */
            if (info_p->bytes_remaining >= block_size) 
            {
                /* Allocate what we need from the remainder in the current page.
                 */
                //info_p->bytes_remaining -= info_p->bytes_remaining % block_size;
                bytes_to_allocate = FBE_MIN(info_p->bytes_remaining, dest_bytecnt);
                bytes_to_allocate -= bytes_to_allocate % block_size;
            }
            else
            {
                /* Otherwise try to allocate all that we can from this page.
                 * Make sure it is a multiple of our block size. 
                 */
                //dest_bytecnt -= dest_bytecnt % block_size;
                bytes_to_allocate = dest_bytecnt;
            }

            if (bytes_to_allocate % block_size)
            {
                /* Not a block size multiple.
                  */
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: bytes_to_allocate: 0x%llx not a multiple of block size %d\n", 
                                        __FUNCTION__,
				        (unsigned long long)bytes_to_allocate,
				        block_size);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if(bytes_to_allocate > FBE_U32_MAX)
            {
                /* exceeding 32bit limit here
                  */
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: allocate of sg failed, Invalid bytes to allocate :bytes_to_allocate: 0x%llx\n", 
                                        __FUNCTION__,
					(unsigned long long)bytes_to_allocate);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            memory_p = fbe_rdgen_memory_allocate_next_buffer(info_p,
                                                             (fbe_u32_t)bytes_to_allocate,
                                                             &bytes_allocated,
                                                             FBE_FALSE /* Contiguous not needed. */);
            dest_sgl_ptr->address = memory_p;

            /* If this is null then we ran out of memory.
             */
            if (dest_sgl_ptr->address == NULL)
            {
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: no bytes allocated for sg.\n", 
                                        __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            fbe_sg_element_init(dest_sgl_ptr, bytes_allocated, memory_p);
            dest_bytecnt -= bytes_allocated;
            fbe_sg_element_increment(&dest_sgl_ptr);

            if (info_p->bytes_remaining < block_size)
            {
                info_p->bytes_remaining = 0;
            }
        }
    }

    /* Terminate the S/G list properly.
     */
    fbe_sg_element_terminate(dest_sgl_ptr);

    if (sgl_count_p != NULL)
    {
        *sgl_count_p = sg_total;
    }
    return FBE_STATUS_OK;
}  
/* end fbe_rdgen_sg_populate_with_memory */
/*!**************************************************************
 * fbe_rdgen_ts_setup_irp_mj_sgs()
 ****************************************************************
 * @brief
 *  Setup the sg lists for an irp mj.
 *  IRP MJ is interesting since it only uses the sg lists
 *  as a convenience in rdgen for checking the data pattern.
 *  We simply setup a single SGL.
 *
 * @param ts_p
 * @param total_bytes
 * @param mem_info_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  7/30/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_ts_setup_irp_mj_sgs(fbe_rdgen_ts_t *ts_p, 
                                           fbe_block_count_t total_bytes,
                                           fbe_rdgen_memory_info_t *mem_info_p)
{
    void* memory_p = NULL;
    void* data_p = NULL;
    fbe_u32_t bytes_allocated;
    fbe_sg_element_t *sg_p = NULL;

    memory_p = fbe_rdgen_ts_get_memory_p(ts_p);

    if (memory_p == NULL)
    {
        sg_p = fbe_rdgen_memory_allocate_next_buffer(mem_info_p, 
                                                     (fbe_u32_t)sizeof(fbe_sg_element_t) * 2, 
                                                     &bytes_allocated, 
                                                     FBE_TRUE    /* Contiguous needed. */);
        if ((sg_p == NULL) || (bytes_allocated == 0))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: allocate of irp mj sg failed, ptr: %p bytes_allocated: %d\n", 
                                    __FUNCTION__, sg_p, bytes_allocated);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        data_p = fbe_rdgen_memory_allocate_next_buffer(mem_info_p, 
                                                       (fbe_u32_t)total_bytes, 
                                                       &bytes_allocated, 
                                                       FBE_TRUE    /* Contiguous needed. */);
        if ((data_p == NULL) || (bytes_allocated == 0))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: allocate of irp mj data failed, ptr: %p bytes_allocated: %d\n", 
                                    __FUNCTION__, data_p, bytes_allocated);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        sg_p = (fbe_sg_element_t*)(((fbe_u8_t*)memory_p) + total_bytes);
        data_p = memory_p;
    }
    fbe_rdgen_ts_set_sg_ptr(ts_p, sg_p);
    fbe_sg_element_init(sg_p, (fbe_u32_t)total_bytes, data_p);
    fbe_sg_element_terminate(&sg_p[1]);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_ts_setup_irp_mj_sgs()
 ******************************************/
/*!****************************************************************************
 *  fbe_rdgen_ts_setup_sgs()
 ******************************************************************************
 * @brief
 *  Function to put memory into an sg list.
 *  
 * @param ts_p - Pointer to the rdgen_ts.
 * @param total_bytes - Total bytes to setup.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *  10/11/2010 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_rdgen_ts_setup_sgs(fbe_rdgen_ts_t *ts_p, fbe_block_count_t total_bytes)
{
    fbe_status_t status;
    fbe_memory_request_t *memory_request_p = fbe_rdgen_ts_get_memory_request(ts_p);
    fbe_u32_t page_size_bytes;
    fbe_u32_t sg_bytes;
    fbe_rdgen_memory_info_t mem_info = {0};
    fbe_rdgen_memory_info_t sg_mem_info = {0};
    fbe_block_count_t sg_bytes_to_allocate;
    fbe_block_count_t pr_sg_bytes_to_allocate;
    fbe_u32_t bytes_allocated;
    fbe_block_count_t pre_read_bytes = ts_p->pre_read_blocks * ts_p->block_size;
    fbe_u32_t data_sg_count;
    fbe_u32_t actual_sg_count;
    fbe_u32_t pr_sg_count;
    fbe_sg_element_t *sg_p = NULL;

    /* Use the chunk size that is the multiple of our block size.
     */
    page_size_bytes = memory_request_p->chunk_size;
    fbe_rdgen_ts_get_chunk_size(ts_p, &page_size_bytes);

    if (fbe_rdgen_ts_get_memory_p(ts_p) == NULL)
    {
        fbe_rdgen_memory_info_init(&mem_info, memory_request_p, page_size_bytes);
        sg_mem_info = mem_info;
    }

    if (ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_MJ || ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_SYNC_IO)
    {
        return fbe_rdgen_ts_setup_irp_mj_sgs(ts_p, total_bytes, &mem_info);
    }

    /* Skip over the data blocks so we can get to the memory for sgs. 
     * We need to allocate the sg first, then we can allocate the blocks. 
     * We need blocks to start at front of the allocation so it is aligned. 
     */
    status = fbe_rdgen_memory_info_apply_offset(&sg_mem_info, total_bytes, ts_p->block_size, &data_sg_count);
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: status %d from fbe_rdgen_sg_populate_with_memory().\n", 
                                __FUNCTION__, status);
        return status;
    }

    /*! @note We add (2) when allocating the sg elements:
     *      o 1 for the `extra sg element'
     *      o 1 for the terminator
     */
    sg_bytes_to_allocate = sizeof(fbe_sg_element_t) * (data_sg_count + 2);

    if (sg_bytes_to_allocate > FBE_U32_MAX)
    {
        /* exceeding 32bit limit here
          */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: allocate of sg failed, Invalid bytes to allocate ptr: %p bytes_allocated: 0x%llx\n", 
                                __FUNCTION__, sg_p, (unsigned long long)sg_bytes_to_allocate);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (ts_p->pre_read_blocks > 0)
    {
        status = fbe_rdgen_memory_info_apply_offset(&sg_mem_info, pre_read_bytes, ts_p->block_size, &pr_sg_count);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: status %d from fbe_rdgen_sg_populate_with_memory().\n", 
                                    __FUNCTION__, status);
            return status;
        }

        /* Determine bytes for data sgs.
         */
        pr_sg_bytes_to_allocate = sizeof(fbe_sg_element_t) * (pr_sg_count + 1);

        if (pr_sg_bytes_to_allocate > FBE_U32_MAX)
        {
            /* exceeding 32bit limit here
              */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: allocate of sg failed, Invalid bytes to allocate ptr: %p bytes_allocated: 0x%llx\n", 
                                    __FUNCTION__, ts_p->sg_ptr,
				    (unsigned long long)pr_sg_bytes_to_allocate);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, ts_p->object_p->object_id,
                                "%s: ts: 0x%x bytes: 0x%x pr_sg_bytes: 0x%x \n", 
                                __FUNCTION__, (fbe_u32_t)ts_p, (unsigned int)pre_read_bytes, (unsigned int)pr_sg_bytes_to_allocate );
    }
    sg_p = fbe_rdgen_memory_allocate_next_buffer(&sg_mem_info, 
                                                 (fbe_u32_t)sg_bytes_to_allocate, 
                                                 &bytes_allocated, 
                                                 FBE_TRUE /* Contiguous needed. */);
    if ((sg_p == NULL) || (bytes_allocated == 0))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: allocate of sg failed, ptr: %p bytes_allocated: %d\n", 
                                __FUNCTION__, ts_p->sg_ptr, bytes_allocated);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_rdgen_ts_set_sg_ptr(ts_p, sg_p);

    /* Populate the sg with the given amount of memory, using the given size of a chunk.
     */
    status = fbe_rdgen_sg_populate_with_memory(&mem_info, ts_p->sg_ptr, total_bytes, ts_p->block_size, &actual_sg_count);
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: status %d from fbe_rdgen_sg_populate_with_memory().\n", 
                                __FUNCTION__, status);
        return status;
    }
    if (actual_sg_count != data_sg_count)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: actual_sg_count %d != data_sg_count %d\n", 
                                __FUNCTION__, actual_sg_count, data_sg_count);
        return status;
    }

    /* Make sure the sg list contains the expected number of bytes.
     */
    sg_bytes = fbe_sg_element_count_list_bytes(sg_p);
    if (sg_bytes != total_bytes)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: sg list too short.  Found %d bytes expected %d bytes.\n", 
                                __FUNCTION__, sg_bytes, (int)(ts_p->current_blocks * ts_p->block_size));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We will randomly add an sg element at the end of the sg list. 
     * This simulates the case where we get an sg list that is not NULL terminated from the client. 
     * We use an sg count of 1, since it is not a nice multiple and if we look at it we should complain. 
     * We also use a known sg magic address so that if we try to access this memory we should get an access violation 
     *                                                                                                               
     * Physical package does not handle this, it assumes the SG list is setup properly.
     */
    if ( (ts_p->object_p->package_id != FBE_PACKAGE_ID_PHYSICAL) &&
         (ts_p->operation != FBE_RDGEN_OPERATION_WRITE_SAME) && (ts_p->operation != FBE_RDGEN_OPERATION_UNMAP) &&
         (ts_p->operation != FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK) &&
         ((ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_DISABLE_RANDOM_SG_ELEMENT) == 0) &&
         (fbe_random() % 2) )
    {
        sg_p = ts_p->sg_ptr;
        sg_p += actual_sg_count;
        fbe_sg_element_init(sg_p, 0x1, (void*)FBE_MAGIC_NUM_RDGEN_EXTRA_SG);
        fbe_sg_element_terminate(&sg_p[1]);
    }

    /* If we have pre-reads then allocate memory to it also.
     */
    if (ts_p->pre_read_blocks > 0)
    {
        /* First allocate the sg list.
         * We determine the number of sgs needed by bytes/page size (we add on 3 for rounding and sg list terminator). 
         */ 
        ts_p->pre_read_sg_ptr = fbe_rdgen_memory_allocate_next_buffer(&sg_mem_info, 
                                                                      (fbe_u32_t)pr_sg_bytes_to_allocate, 
                                                                      &bytes_allocated, 
                                                                      FBE_TRUE/* contiguous needed */);
        if ((ts_p->pre_read_sg_ptr == NULL) || (bytes_allocated == 0))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: allocate of pre-read sg failed, ptr: %p bytes_allocated: %d\n", 
                                    __FUNCTION__, ts_p->pre_read_sg_ptr, bytes_allocated);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* Populate the sg with the given amount of memory, using the given size of a chunk.
         */
        status = fbe_rdgen_sg_populate_with_memory(&mem_info, 
                                                   ts_p->pre_read_sg_ptr, 
                                                   pre_read_bytes, 
                                                   ts_p->block_size, 
                                                   &actual_sg_count);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: status %d from fbe_rdgen_sg_populate_with_memory().\n", 
                                    __FUNCTION__, status);
            return status;
        }
        if (actual_sg_count != pr_sg_count)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: actual_sg_count %d != pr_sg_count %d\n", 
                                    __FUNCTION__, actual_sg_count, pr_sg_count);
            return status;
        }
        /* Make sure the sg list contains the expected number of bytes.
         */
        sg_bytes = fbe_sg_element_count_list_bytes(ts_p->pre_read_sg_ptr);
        if (sg_bytes != (ts_p->pre_read_blocks * ts_p->block_size))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: pre-read sg list too short.  Found %d bytes expected %llu bytes.\n", 
                                    __FUNCTION__, sg_bytes,
				    (unsigned long long)(ts_p->pre_read_blocks * ts_p->block_size));
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
}
/**************************************
 * end fbe_rdgen_ts_setup_sgs()
 **************************************/
/*!***************************************************************
 *  fbe_rdgen_sg_fill_with_memory
 ****************************************************************
 * @brief
 *  Generate a sg list with random counts.
 *  Fill the sg list passed in.
 *  Fill in the memory portion of this sg list from the
 *  contiguous range passed in.
 *  
 * @param ts_p - Current ts.
 *
 * @return
 *  none 
 *
 * @author
 *  5/3/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_sg_fill_with_memory(fbe_sg_element_t *sg_p,
                                           fbe_block_count_t blocks,
                                           fbe_u8_t *memory_ptr,
                                           fbe_block_size_t block_size)
{
    fbe_status_t status;
    status = fbe_data_pattern_sg_fill_with_memory(sg_p,
                                                  memory_ptr,
                                                  blocks,
                                                  block_size,
                                                  FBE_RDGEN_MAX_SG_DATA_ELEMENTS,
                                                  FBE_RDGEN_MAX_BLOCKS_PER_SG);
    return status;
}
/**************************************
 * end fbe_rdgen_sg_fill_with_memory
 **************************************/

/*!***************************************************************
 *  fbe_rdgen_translate_data_pattern
 ****************************************************************
 * @brief
 *  This function translate the rdgen pattern into pattern defined
 *  in data_pattern library. They should be in sync. If translation
 *  could not be done, it returns the FBE_STATUS_GENERIC_FAILURE.
 *  
 * @param rdgen_pattern - Rdgen pattern
 * @param data_pattern_p - data pattern
 *
 * @return FBE_STATUS_OK on success, 
           FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  9/14/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_translate_data_pattern(fbe_rdgen_pattern_t rdgen_pattern,
                                                      fbe_data_pattern_t* data_pattern_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    switch(rdgen_pattern)
    {
    case FBE_RDGEN_PATTERN_INVALID:
        *data_pattern_p = FBE_DATA_PATTERN_INVALID;
        break;
    case FBE_RDGEN_PATTERN_ZEROS:
        *data_pattern_p = FBE_DATA_PATTERN_ZEROS;
        break;
    case FBE_RDGEN_PATTERN_LBA_PASS:
        *data_pattern_p = FBE_DATA_PATTERN_LBA_PASS;
        break;
    case FBE_RDGEN_PATTERN_INVALIDATED:
        *data_pattern_p = FBE_DATA_PATTERN_INVALIDATED;
        break;
    case FBE_RDGEN_PATTERN_CLEAR:
        *data_pattern_p = FBE_DATA_PATTERN_CLEAR;
        break;
    default:
        /* Data pattern not in sync with rdgen pattern*/
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }
    return status;
}
/******************************************
 * end fbe_rdgen_translate_data_pattern()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_fill_sg_list()
 ****************************************************************
 * @brief
 *  Fill an sg list with a given pattern.
 *
 * @param  -               
 *
 * @return FBE_STATUS_OK on success, 
           FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  6/9/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_fill_sg_list(fbe_rdgen_ts_t *ts_p,
                                    fbe_sg_element_t * sg_ptr,
                                    fbe_rdgen_pattern_t pattern,
                                    fbe_bool_t b_append_checksum,
                                    fbe_bool_t b_read_op)
{
    fbe_lba_t seed = ts_p->current_lba;
    fbe_u32_t block_size = ts_p->block_size;
    fbe_u32_t sequence_count = fbe_rdgen_ts_get_sequence_count(ts_p);
    fbe_data_pattern_sp_id_t rdgen_sp_id = fbe_rdgen_cmi_get_sp_id();
    fbe_data_pattern_info_t data_pattern_info = {0};
    fbe_u64_t  header_array[FBE_DATA_PATTERN_MAX_HEADER_PATTERN];
    fbe_status_t status;
    fbe_data_pattern_t data_pattern;
    fbe_data_pattern_flags_t data_pattern_flags = 0;
    fbe_u32_t sg_count = fbe_sg_element_count_entries(sg_ptr);

    /*!  Check for `Corrupt data' or `Corrupt CRC'
     *
     *  @todo Add support for FBE_DATA_PATTERN_FLAGS_RAID_VERIFY and
     *        FBE_DATA_PATTERN_FLAGS_DATA_LOST.
     */
    if ((ts_p->operation == FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY) ||
        (ts_p->operation == FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK))
    {
        data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA;

    }
    else if (ts_p->operation == FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK)
    {
        data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC;
    }

    /* If we are told not to check the invalidation reason do that.
     */
    if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_DO_NOT_CHECK_INVALIDATION_REASON)
    {
        data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_DO_NOT_CHECK_INVALIDATION_REASON;
    }

    status = fbe_rdgen_translate_data_pattern(pattern, &data_pattern);
    if (RDGEN_COND(status != FBE_STATUS_OK))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen pattern 0x%x not in sync with data pattern, status: 0x%x\n", 
                                __FUNCTION__, pattern, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_CREATE_REGIONS) ||
        fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_USE_REGIONS)) &&
        (!b_read_op)) /* For read Ops just clear the buffer as we will be reading into it */
    {
        sequence_count = ts_p->request_p->specification.region_list[ts_p->region_index].pass_count;
        data_pattern = ts_p->request_p->specification.region_list[ts_p->region_index].pattern;
    }
    /*
     * fill data pattern information structure
     */
    header_array[0] = FBE_RDGEN_BUILD_PATTERN_WORD(FBE_RDGEN_HEADER_PATTERN, rdgen_sp_id); 
    header_array[1] = ts_p->object_p->object_id;
    header_array[2] = data_pattern;
    status = fbe_data_pattern_build_info(&data_pattern_info,
                                         data_pattern,
                                         data_pattern_flags,
                                         seed,
                                         sequence_count,
                                         3, /*num_header_words*/
                                         header_array);
    if (status != FBE_STATUS_OK )
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen build data pattern info struct failed. \n", 
                                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* If we are not purposefully invalidating blocks, clear the corrupt bitmap.
     */
    if ((data_pattern_flags & (FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC | FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA |
                               FBE_DATA_PATTERN_FLAGS_RAID_VERIFY | FBE_DATA_PATTERN_FLAGS_DATA_LOST      )) == 0)
    {
        /* If we are not purposfully invalidating blocks, clear the corrupt bitmap.
         */
        ts_p->corrupted_blocks_bitmap = 0;
    }
    if (ts_p->request_p->specification.pattern == FBE_RDGEN_PATTERN_INVALIDATED) {
        ts_p->corrupted_blocks_bitmap = FBE_U64_MAX;
    }

    fbe_data_pattern_set_sg_blocks(&data_pattern_info, ts_p->current_blocks);

    /* Now fill the sg list with the correct data pattern.
     */
    status = fbe_data_pattern_fill_sg_list(sg_ptr,
                                           &data_pattern_info,
                                           block_size,
                                           ts_p->corrupted_blocks_bitmap,
                                           sg_count);
    if (status != FBE_STATUS_OK )
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen fill data pattern sg list failed. \n", 
                                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}                               /* fbe_rdgen_fill_sg_list */

/*!**************************************************************
 * fbe_rdgen_check_sg_list()
 ****************************************************************
 * @brief
 *  Check an sg list for a given pattern.
 *
 * @param ts_p - Current I/O.   
 * @param pattern - Pattern to check for.   
 * @param b_panic - FBE_TRUE to panic on data miscompare,   
 *                  FBE_FALSE to not panic on data miscompare.
 *
 * @return FBE_STATUS_OK on success, 
           FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  6/9/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_check_sg_list(fbe_rdgen_ts_t *ts_p,
                                     fbe_rdgen_pattern_t pattern,
                                     fbe_bool_t b_panic)
{
    fbe_sg_element_t * sg_ptr = ts_p->sg_ptr;
    fbe_lba_t seed = ts_p->current_lba;
    fbe_u32_t block_size = ts_p->block_size;
    fbe_u32_t sequence_count = fbe_rdgen_ts_get_sequence_count(ts_p);
    fbe_data_pattern_sp_id_t rdgen_originating_sp_id = fbe_rdgen_cmi_get_sp_id();
    fbe_data_pattern_info_t data_pattern_info = {0};
    fbe_u64_t  header_array[FBE_DATA_PATTERN_MAX_HEADER_PATTERN];
    fbe_status_t status;
    fbe_data_pattern_t data_pattern;
    fbe_data_pattern_flags_t data_pattern_flags = 0;
    fbe_u32_t sg_count = fbe_sg_element_count_entries(sg_ptr);

    /* Determine SP we expect in the data.
     */
    if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_CHECK_ORIGINATING_SP))
    {
        /* We expect a particular SP to be the originator of the data.
         */
        if ((ts_p->request_p->specification.originating_sp_id != FBE_DATA_PATTERN_SP_ID_A) &&
            (ts_p->request_p->specification.originating_sp_id != FBE_DATA_PATTERN_SP_ID_B)    )
        {
            /* Originating SP was not properly populated.  Report the error.
             */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                    "%s: originating sp ts lba: 0x%llx blks: 0x%llx sp id: 0x%x not valid\n", 
                                    __FUNCTION__, ts_p->lba, ts_p->blocks, ts_p->request_p->specification.originating_sp_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Set the originating SP for the data pattern check.
         */
        rdgen_originating_sp_id = ts_p->request_p->specification.originating_sp_id;
    }
    /* Since rdgen runs with logical error injection enabled, there are cases
     * where we have injected uncorrectable errors that are not expected.  In
     * this case we should have set the `skip compare' flag.
     */
    if (fbe_rdgen_ts_is_flag_set(ts_p, FBE_RDGEN_TS_FLAGS_SKIP_COMPARE))
    {
        /* Generate a low level trace, clear the flag and return success.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: ts lba: 0x%llx blks: 0x%llx invalids: %d skip compare\n", 
                                __FUNCTION__, (unsigned long long)ts_p->lba,
				(unsigned long long)ts_p->blocks,
				ts_p->inv_blocks_count);
        fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_SKIP_COMPARE);
        return FBE_STATUS_OK;
    }

    /* Check for `Corrupt data' or `Corrupt CRC'
     */
    if ((ts_p->operation == FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY) ||
        (ts_p->operation == FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK))
    {
        if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_USE_SEQUENCE_COUNT_SEED)
        {
            /* If this option is set then we should always expect the pattern.
             * Typically this pattern was written with a background pattern. 
             * This means that the corrupt data was able to be reconstructed.
             */
            sequence_count = ts_p->request_p->specification.sequence_count_seed;
        }
        else
        {
            /* Otherwise we just expect the corrupt data pattern.
             */
            data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA;
        }
        if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_INVALID_AND_VALID_PATTERNS)
        {
            data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_VALID_AND_INVALID_DATA;
            data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA;
        }

    }
    else if (ts_p->operation == FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK)
    {
        data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC;

        /* If `do not check checksum' is not set we expect a status of media error.
         */
        if (!fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC) &&
             (ts_p->blocks_remaining == 0)                                                                               &&
             (ts_p->last_packet_status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)                      )
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                    "%s: expected media error ts lba: 0x%llx blks: 0x%llx block_status: 0x%x\n", 
                                    __FUNCTION__, (unsigned long long)ts_p->lba,
				    (unsigned long long)ts_p->blocks,
				    ts_p->last_packet_status.block_status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* If we are told not to check the invalidation reason do that.
     */
    if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_DO_NOT_CHECK_INVALIDATION_REASON)
    {
        data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_DO_NOT_CHECK_INVALIDATION_REASON;
    }

    if (fbe_rdgen_specification_data_pattern_flags_is_set(&ts_p->request_p->specification,
                                                          FBE_RDGEN_DATA_PATTERN_FLAGS_CHECK_CRC))
    {
        data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_CHECK_CRC_ONLY;
    }
    if (fbe_rdgen_specification_data_pattern_flags_is_set(&ts_p->request_p->specification,
                                                          FBE_RDGEN_DATA_PATTERN_FLAGS_CHECK_LBA_STAMP))
    {
        data_pattern_flags |= FBE_DATA_PATTERN_FLAGS_CHECK_LBA_STAMP_ONLY;
    }

    /* Translate form rdgen pattern to data pattern.
     */
    status = fbe_rdgen_translate_data_pattern(pattern, &data_pattern);
    if (status != FBE_STATUS_OK )
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen pattern 0x%x not in sync with data pattern, status: 0x%x\n", 
                                __FUNCTION__, pattern, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If we we told to use regions setup now.
     */
    if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_CREATE_REGIONS) ||
        fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_USE_REGIONS))
    {
        sequence_count = ts_p->request_p->specification.region_list[ts_p->region_index].pass_count;
        data_pattern = ts_p->request_p->specification.region_list[ts_p->region_index].pattern;
        rdgen_originating_sp_id = ts_p->request_p->specification.region_list[ts_p->region_index].sp_id;
    }

    /*
     * fill data pattern information structure
     */
    header_array[0] = FBE_RDGEN_BUILD_PATTERN_WORD(FBE_RDGEN_HEADER_PATTERN, rdgen_originating_sp_id); 
    header_array[1] = ts_p->object_p->object_id;
    header_array[2] = data_pattern;
    status = fbe_data_pattern_build_info(&data_pattern_info,
                                         data_pattern,
                                         data_pattern_flags,
                                         seed,
                                         sequence_count,
                                         3, /*num_header_words*/
                                         header_array);
    if (status != FBE_STATUS_OK )
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen build data pattern info struct failed. \n", 
                                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_data_pattern_set_sg_blocks(&data_pattern_info, ts_p->current_blocks);

    status = fbe_data_pattern_check_sg_list(sg_ptr,
                                            &data_pattern_info,
                                            block_size,
                                            ts_p->corrupted_blocks_bitmap,
                                            ts_p->object_p->object_id,
                                            sg_count,
                                            b_panic);
    if (status != FBE_STATUS_OK )
    {
        fbe_trace_level_t trace_level;
        if (b_panic)
        {
            trace_level = FBE_TRACE_LEVEL_ERROR;
        }
        else
        {
            trace_level = FBE_TRACE_LEVEL_WARNING;
        }
        fbe_rdgen_service_trace(trace_level, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen check data pattern sg list failed. \n", 
                                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/******************************************
 * end fbe_rdgen_check_sg_list()
 ******************************************/

/*!**************************************************************
 *          fbe_rdgen_count_invalidated_and_bad_crc_sectors()
 ****************************************************************
 * @brief
 *  Count the number of invalid blocks or bad crc for a 
 *  list of sectors.
 *  
 * @params
 *  sectors,     [I] - Contiguous sectors to check or generate.
 *  memory_ptr,  [I] - Start of memory range.
 *  b_is_virtual,[I] - Indicates if the memory is virtual.
 *  inv_count    [O] - count of invalidated sectors
 *  bad_count    [O] - count of non-invalidated sectors with bad crc
 *
 * @return fbe_status_t - FBE_STATUS_OK if success, 
 *                      - FBE_STATUS_GENERIC_FAILURE if failure
 *
 * @author
 *   04/28/10: Kevin Schleicher - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_count_invalidated_and_bad_crc_sectors( fbe_u32_t sectors,
                                                       fbe_u32_t * memory_ptr,
                                                       fbe_bool_t b_is_virtual,
                                                       fbe_u32_t * inv_count,
                                                       fbe_u32_t * bad_count)
{
    fbe_sector_t *sector_p = NULL;
    
    void * mapped_memory_ptr = NULL;
    fbe_u32_t mapped_memory_cnt = 0;
    void * reference_ptr = NULL;

    /* Start the count at 0.
     */
    *inv_count = 0;

    /* MAP IN SECTOR HERE */
    mapped_memory_cnt = (sectors * FBE_BE_BYTES_PER_BLOCK);
    mapped_memory_ptr = (b_is_virtual)?
                  (void*)memory_ptr:
                  FBE_RDGEN_MAP_MEMORY(((fbe_u32_t *)memory_ptr), mapped_memory_cnt);
    reference_ptr = mapped_memory_ptr;
    sector_p = (fbe_sector_t *)reference_ptr;

    while (sectors--)
    {
        xorlib_sector_invalid_reason_t reason;
        
        /* First check if the crc is bad or not
         */
        if (sector_p->crc != fbe_xor_lib_calculate_checksum(sector_p->data_word))
        {
            /* We can't check the lba seed at this level for an invalidate.
             */
            if (fbe_xor_lib_is_sector_invalidated(sector_p, &reason)) 
            {
                /* Found an invalidated sector, increment the count.
                 */
                (*inv_count)++;
            }
            else
            {
                /* Found sector with a bad crc, increment the count.
                 */
                (*bad_count)++;
            }
        }

        sector_p++;
    }

    /* MAP OUT SECTOR HERE */
    if ( !b_is_virtual )
    {
        FBE_RDGEN_UNMAP_MEMORY(mapped_memory_ptr, mapped_memory_cnt);
    }

    return FBE_STATUS_OK;
}                               
/**********************************************************
 * end of fbe_rdgen_count_invalidated_and_bad_crc_sectors() 
 *********************************************************/

/*!**************************************************************
 * fbe_rdgen_sg_count_invalidated_and_bad_crc_blocks()
 ****************************************************************
 * @brief
 *  Count the number of invalidated or bad crc blocks found in the 
 *  sg list for the thread.
 *
 * @param ts_p - Current ts.
 *
 * @return fbe_status_t - FBE_STATUS_OK if success, 
                        - FBE_STATUS_GENERIC_FAILURE if failure
 *
 * @author
 *  4/28/2010 - Created. Kevin Schleicher
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_sg_count_invalidated_and_bad_crc_blocks(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t sg_count = 0;
    fbe_u32_t current_blocks;
    fbe_u32_t *memory_ptr;
    fbe_bool_t b_is_virtual = FBE_FALSE;
    fbe_sg_element_t * sg_ptr = ts_p->sg_ptr;
    fbe_u32_t block_size = ts_p->block_size;
    fbe_u32_t max_sg_count = fbe_sg_element_count_entries(sg_ptr);
    fbe_block_count_t total_blocks = 0;
    
    if ((ts_p->sg_ptr[max_sg_count].address != NULL) ||
        (ts_p->sg_ptr[max_sg_count].count != 0))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: invalid SG list.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Spin through each SG Entry.
     */
    while (((sg_ptr)->count != 0) && (total_blocks < ts_p->current_blocks))
    {
        if (RDGEN_COND(sg_ptr->count % block_size != 0))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: sg_ptr->count 0x%x is not aligned to block_size 0x%x. \n", 
                                    __FUNCTION__, sg_ptr->count, block_size);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        current_blocks = (sg_ptr)->count / FBE_BE_BYTES_PER_BLOCK;

        memory_ptr = (fbe_u32_t *) (sg_ptr)->address;
        b_is_virtual = FBE_RDGEN_VIRTUAL_ADDRESS_AVAIL (sg_ptr);

        /* If we find the token, just skip over this sg.
         */
        if (memory_ptr != (fbe_u32_t *) -1)
        {
            fbe_status_t status;
            fbe_u32_t inv_blocks_count = 0;
            fbe_u32_t bad_crc_blocks_count = 0;

            /* Add to the invalid block count for however many blocks in this sg entry are
             * invalidated.
             */
            status = fbe_rdgen_count_invalidated_and_bad_crc_sectors(current_blocks, 
                                                           memory_ptr,
                                                           b_is_virtual,
                                                           &inv_blocks_count,
                                                           &bad_crc_blocks_count);
            if (RDGEN_COND(status != FBE_STATUS_OK))
            {
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: error 0x%x counting inv blocks. \n", 
                                        __FUNCTION__, status);
                return status;
            } 

            /* Increment the global counts.
             */
            ts_p->inv_blocks_count += inv_blocks_count;
            ts_p->bad_crc_blocks_count += bad_crc_blocks_count;
                   
        }

        /* Increment sg ptr.
         */
        sg_ptr++;
        total_blocks += current_blocks;
        sg_count++;
        if (RDGEN_COND(sg_count > max_sg_count))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: sg count 0x%x >= FBE_RDGEN_MAX_SG 0x%x. \n", 
                                    __FUNCTION__, sg_count, max_sg_count);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    if ((ts_p->sg_ptr[max_sg_count].address != NULL) ||
        (ts_p->sg_ptr[max_sg_count].count != 0))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: invalid SG list.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the execution status
     */
    return status;
}
/***********************************************************
 * end of fbe_rdgen_sg_count_invalidated_and_bad_crc_blocks()
 ***********************************************************/

/***************************************************************************
 * fbe_rdgen_memory_info_apply_offset()
 ***************************************************************************
 * @brief
 *  This function is responsible for initializing the elements in
 *  one S/G list from the contents of another.
 *
 *  @param info_p - Ptr to memory information.
 *  @param dest_bytecnt -   number of bytes to be referenced.
 *  @param block_size - Number of bytes in each block.
 *  @param sgl_count_p - Number of sgs needed for this memory.
 *
 * @return
 *  fbe_status_t
 * 
 * @author
 *  2/13/2012 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_rdgen_memory_info_apply_offset(fbe_rdgen_memory_info_t *info_p,
                                                fbe_block_count_t dest_bytecnt,
                                                fbe_u32_t block_size,
                                                fbe_u32_t *sgl_count_p)
{
    fbe_u16_t sg_total = 0;    /* count of S/G elements copied */
    fbe_u32_t bytes_allocated;

    if ((info_p->bytes_remaining == 0) && 
        ((info_p->memory_header_p == NULL) ||
         (info_p->memory_header_p->u.queue_element.next == NULL)))
    {
        /* No more source memory so there is nothing to copy.
         */
        if (dest_bytecnt == 0)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: dest_bytecnt is zero.\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        fbe_block_count_t bytes_to_allocate;
        fbe_u8_t *memory_p = NULL;
        /* Assign portions of the memory provided by the source to the entries in the
         * destination S/G list; assign only the number of bytes specified. 
         */
        while (dest_bytecnt > 0)
        {
            sg_total++;

            /* Allocate either the amount leftover or thet amount asked for, whichever 
             * is smaller.  This allows us to not waste any memory. 
             */
            if (info_p->bytes_remaining >= block_size)
            {
                /* Allocate what we need from the remainder in the current page.
                 */
                //info_p->bytes_remaining -= info_p->bytes_remaining % block_size;
                bytes_to_allocate = FBE_MIN(info_p->bytes_remaining, dest_bytecnt);
            }
            else
            {
                /* Otherwise try to allocate all that we can from this page.
                 * Make sure it is a multiple of our block size. 
                 */
                dest_bytecnt -= dest_bytecnt % block_size;
                bytes_to_allocate = dest_bytecnt;
            }

            if(bytes_to_allocate > FBE_U32_MAX)
            {
                /* exceeding 32bit limit here
                  */
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: allocate of sg failed, Invalid bytes to allocate :bytes_to_allocate: 0x%llx\n", 
                                        __FUNCTION__,
					(unsigned long long)bytes_to_allocate);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            memory_p = fbe_rdgen_memory_allocate_next_buffer(info_p,
                                                             (fbe_u32_t)bytes_to_allocate,
                                                             &bytes_allocated,
                                                             FBE_FALSE /* Contiguous not needed. */);
            dest_bytecnt -= bytes_allocated;
        }
    }
    if (sgl_count_p != NULL)
    {
        *sgl_count_p = sg_total;
    }
    return FBE_STATUS_OK;
}  
/* end fbe_rdgen_memory_info_apply_offset */

/*************************
 * end file fbe_rdgen_sg.c
 *************************/

