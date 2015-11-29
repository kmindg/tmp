/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_config_memory.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functionality to support the allocation of the
 *  packets, buffers and sg lists using memory service.
 * 
 *  @ingroup base_config_class_files

 *  All the derived object from the base_config class will use this
 *  functionality to to allocate the packets.
 * 
 * @version
 *   3/09/2010:  Created. Dhaval Patel
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "base_object_private.h"
#include "fbe_base_config_private.h"
#include "fbe/fbe_memory.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"

/*****************************************
 * LOCAL FUNCTION FORWARD DECLARATION
 *****************************************/

static fbe_status_t fbe_base_config_calculate_memory_chunks(fbe_base_config_t * base_config_p,
                                                            fbe_u32_t memory_chunk_size,
                                                            fbe_base_config_memory_allocation_chunks_t mem_alloc_chunks,
                                                            fbe_u32_t * number_of_chunks_p);

static fbe_status_t fbe_base_config_memory_assign_memory_address(fbe_base_config_t * base_config_p,
                                                                 fbe_memory_request_t * memory_request_p,
                                                                 fbe_memory_header_t ** current_memory_header_p,
                                                                 fbe_u32_t  * used_memory_in_chunk_p,
                                                                 fbe_u32_t * number_of_used_chunks_p,
                                                                 fbe_u32_t memory_alloc_size,
                                                                 fbe_u8_t ** buffer_p);


/*!****************************************************************************
 *  fbe_base_config_memory_allocate_chunks()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the required memory for the caller
 *  specified parameters (number of packets, sg elements and buffers).
 * 
 * @param base_config_p                 - pointer to the base config object.
 * @param packet_p                      - pointer to the packet.
 * @param number_of_packets             - number of packets.
 * @param sg_element_count              - number of sg element entries.
 * @param buffer_count                  - number of buffers.
 * @param buffer_size                   - buffer size.
 * @param memory_completion_function    - pointer to the completion function.
 *
 * @return fbe_status_t.                - status of the operation.
 *
 * @author
 *   10/11/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_base_config_memory_allocate_chunks(fbe_base_config_t * base_config_p,
                                       fbe_packet_t * packet_p,
                                       fbe_base_config_memory_allocation_chunks_t mem_alloc_chunks,
                                       fbe_memory_completion_function_t memory_completion_function)
{
    fbe_u32_t                  number_of_memory_chunks = 0;
    fbe_memory_chunk_size_t    memory_chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    fbe_memory_request_t *     memory_request_p = NULL;
    fbe_status_t               status;
    
     /*!@todo we can improve this logic to go through different memory chunk 
      * size until we find the most suitable chunk size.
      */
     /* determine number of memory chunks required to allocate the memory for the
      * packets, sg list and buffers.
      */
     status = fbe_base_config_calculate_memory_chunks(base_config_p,
                                                      memory_chunk_size,
                                                      mem_alloc_chunks,
                                                      &number_of_memory_chunks);
     if(status != FBE_STATUS_OK)
     {
         return status;
     }

     /* allocate the subpacket to handle the zero on demand operation on slow path. */
     memory_request_p = fbe_transport_get_memory_request(packet_p);

     /* build the memory request for allocation. */
     fbe_memory_build_chunk_request(memory_request_p,
                                    memory_chunk_size,
                                    number_of_memory_chunks,
                                    fbe_transport_get_resource_priority(packet_p),
                                    fbe_transport_get_io_stamp(packet_p),
                                    memory_completion_function,
                                    base_config_p);


	fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

     /* Issue the memory request to memory service. */
     status = fbe_memory_request_entry(memory_request_p);
     if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
     {
         fbe_base_object_trace((fbe_base_object_t *) base_config_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: memory allocation failed, status:0x%x\n", __FUNCTION__, status);
         return status;
     }

     return FBE_STATUS_OK;
 }
/******************************************************************************
 * end fbe_base_config_memory_allocate_chunks()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_base_config_calculate_memory_chunks()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the number of memory chunks required
 *  for allocation of the packets buffer and sg lists.
 * 
 * @param base_config_p         - pointer to the base config object.
 * @param chunk_size            - memory chunk size.
 * @param sg_element_count      - number of sg element entries.
 * @param number_of_packets     - number of packets.
 * @param buffer_count          - number of buffers.
 * @param buffer_size           - buffer size.
 * @param number_of_chunks_p    - pointer to number of chunks.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   10/11/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_base_config_calculate_memory_chunks(fbe_base_config_t * base_config_p,
                                        fbe_u32_t memory_chunk_size,
                                        fbe_base_config_memory_allocation_chunks_t mem_alloc_chunks,
                                        fbe_u32_t * number_of_chunks_p)
{
    fbe_u32_t         total_sg_list_size, pre_read_desc_size;
    fbe_u32_t         memory_left_in_chunk = 0;
    fbe_u32_t         packet_size;
    fbe_u32_t         index;
    fbe_u32_t         sg_element_count = mem_alloc_chunks.sg_element_count;
    fbe_u32_t         buffer_count = mem_alloc_chunks.buffer_count;
    fbe_u32_t         number_of_packets = mem_alloc_chunks.number_of_packets;
    fbe_u32_t         buffer_size = mem_alloc_chunks.buffer_size;
    fbe_bool_t        alloc_pre_read_desc = mem_alloc_chunks.alloc_pre_read_desc;
    
    /*!@todo we can rework this and apply algorithms for the best possible fit. */

    /* initialize number of chunks we need to allocate as zero. */
    *number_of_chunks_p = 0;

    /* if buffer size is greater than memory chunk size then return error. */
    buffer_size += (buffer_size % sizeof(fbe_u64_t));
    if(buffer_size > memory_chunk_size)
    {
         fbe_base_object_trace((fbe_base_object_t *) base_config_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: buff size greater than chunk size, buff_size:0x%x, chk_size:0x%x\n",
                               __FUNCTION__, buffer_size, memory_chunk_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* round the sg list size to align with 64-bit boundary. */
    total_sg_list_size = sizeof(fbe_sg_element_t) * sg_element_count;
    total_sg_list_size += (total_sg_list_size % sizeof(fbe_u64_t));
    
    /*if total sg list size is greater than memory chunk size then return error. */
    if(total_sg_list_size > memory_chunk_size)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: sg list size greater than chunk size, sg_size:0x%x, chk_size:0x%x\n",
                              __FUNCTION__, total_sg_list_size, memory_chunk_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* decrement the number of bytes from the current memory chunk. */
    if(total_sg_list_size != 0)
    {
        (*number_of_chunks_p)++;
        memory_left_in_chunk = memory_chunk_size - total_sg_list_size;
    }

    /* reserve the space for the packets pointers. */
    if(number_of_packets != 0)
    {
        if(memory_left_in_chunk < (number_of_packets * sizeof(fbe_packet_t *)))
        {
            (*number_of_chunks_p)++;
            memory_left_in_chunk = memory_chunk_size - (number_of_packets * sizeof(fbe_packet_t *));
        }
        else
        {
            memory_left_in_chunk -= (number_of_packets * sizeof(fbe_packet_t *));
        }
    }

    /* determine memory required to allocate the packets. */
    packet_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    for(index = 0; index < number_of_packets; index++)
    {
        if(memory_left_in_chunk < packet_size)
        {
            (*number_of_chunks_p)++;
            memory_left_in_chunk = memory_chunk_size - packet_size;
        }
        else
        {
            memory_left_in_chunk -= packet_size;
        }
    }   

    /* reserve the space for the buffer pointers. All allocations must be 
     * rounded to the buffer size.
     */
    if(buffer_count != 0)
    {
        if(memory_left_in_chunk < (buffer_count * sizeof(fbe_u8_t *)))
        {
            (*number_of_chunks_p)++;
            memory_left_in_chunk = memory_chunk_size - (buffer_count * sizeof(fbe_u8_t *)); 
        }
        else
        {
            memory_left_in_chunk -= (buffer_count * sizeof(fbe_u8_t *));
        }
    }

    /* determine memory required to allocate the buffers. */
    for(index = 0; index < buffer_count; index++)
    {
        if(memory_left_in_chunk < buffer_size)
        {
            (*number_of_chunks_p)++;
            memory_left_in_chunk = memory_chunk_size - buffer_size;
        }
        else
        {
            memory_left_in_chunk -= buffer_size;
        }
    }

    /* reserve space for the pre read desc. */
    if(alloc_pre_read_desc)
    {
        /* Preread descriptor size = (size of pre read descriptor + two blocks memory + two SG elements) */
        pre_read_desc_size = sizeof(fbe_payload_pre_read_descriptor_t) * 2 * FBE_RAID_MAX_DISK_ARRAY_WIDTH + 
                                (FBE_METADATA_BLOCK_SIZE * 2) + (sizeof(fbe_sg_element_t) * 2);

        /* if pre read desc size is greater than memory chunk size
         * then return error.
         */
        pre_read_desc_size += (pre_read_desc_size % sizeof(fbe_u64_t));
        if(pre_read_desc_size > memory_chunk_size)
        {
             fbe_base_object_trace((fbe_base_object_t *) base_config_p,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: pre_read_desc_size greater than chunk size, pre_read_desc_size:0x%x, chk_size:0x%x\n",
                                   __FUNCTION__, pre_read_desc_size, memory_chunk_size);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if(memory_left_in_chunk < pre_read_desc_size)
        {
            (*number_of_chunks_p)++;
            memory_left_in_chunk = memory_chunk_size - pre_read_desc_size;
        }
        else
        {
            memory_left_in_chunk -= pre_read_desc_size;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_calculate_memory_chunks()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_base_config_memory_assign_memory_address()
 ******************************************************************************
 * @brief
 *  This function is used to go through the allocated memory request and assign
 *  memory address to the caller based on requested size.
 * 
 * @param memory_request_p          - pointer to the allocated memory request.
 * @param current_memory_header_p   - pointer to the current memory header.
 * @param used_memory_in_chunk_p    - pointer to used memory in chunks.
 * @param number_of_used_chunks_p   - pointer to number of used chunks.
 * @param memory_alloc_size         - allocation size.
 * @param buffer_p                  - returns pointer to the memory address.
 *
 * @return fbe_status_t.            - status of the operation.
 *
 * @author
 *   10/11/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_base_config_memory_assign_memory_address(fbe_base_config_t * base_config_p,
                                             fbe_memory_request_t * memory_request_p,
                                             fbe_memory_header_t ** current_memory_header_p,
                                             fbe_u32_t  * used_memory_in_chunk_p,
                                             fbe_u32_t * number_of_used_chunks_p,
                                             fbe_u32_t memory_alloc_size,
                                             fbe_u8_t ** buffer_p)
{
    fbe_memory_header_t *master_memory_header = NULL;
    fbe_memory_header_t *current_memory_header = *current_memory_header_p;

    /* if user provides buffer pointer as null then return error. */
    if(memory_alloc_size == 0)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: allocation size cannot be zero\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* Get the pointer to the allocated memory from the memory request. */
    master_memory_header = memory_request_p->ptr;

    /* Since we are assigning memory the number of chunks used must be greater 
     * than 0.
     */
    if (*number_of_used_chunks_p == 0)
    {
        *number_of_used_chunks_p = 1;
    }

    /* if the memory allocation size can fit in current memory chunk then
     * assign a address at the current location else move to next memory header
     * to assign the memory address. 
     */ 
    if (((*used_memory_in_chunk_p) + memory_alloc_size) <= 
        ((fbe_u32_t) master_memory_header->memory_chunk_size))
    {
        *buffer_p = (fbe_u8_t *) (fbe_memory_header_to_data(current_memory_header) + (*used_memory_in_chunk_p));
        (*used_memory_in_chunk_p) += memory_alloc_size;
    }
    else
    {
        /* If number of chunks used chunks less than number of chunks allocated then
         * goto next chunk.  Otherwise it is an error
         */
        if( *number_of_used_chunks_p < master_memory_header->number_of_chunks)
        {
            if(memory_alloc_size <= ((fbe_u32_t) master_memory_header->memory_chunk_size))
            {
                /* increment the number of chunks used. */
                *number_of_used_chunks_p = *number_of_used_chunks_p + 1;

                /* get the next memory header. */
                fbe_memory_get_next_memory_header(*current_memory_header_p, &current_memory_header);
                if (current_memory_header == NULL)
                {
                    fbe_base_object_trace((fbe_base_object_t *) base_config_p,
                                          FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s: get next chunk failed\n",
                                          __FUNCTION__);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                *current_memory_header_p = current_memory_header;
                *buffer_p = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header);
                (*used_memory_in_chunk_p) = memory_alloc_size;
            }
            else
            {
                /* we need contigeous memory and so do not allow to span request across
                 * multiple chunks.
                 */
                fbe_base_object_trace((fbe_base_object_t *) base_config_p,
                                      FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s: allocation size cannot be greater than chunk size\n",
                                      __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else
        {
            /* not enough number of chunks available to allocate the memory. */
            fbe_base_object_trace((fbe_base_object_t *) base_config_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: not enough chks available, num_chk:0x%x, num_used_chk:0x%x\n",
                                  __FUNCTION__, master_memory_header->number_of_chunks, *number_of_used_chunks_p);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_memory_assign_memory_address()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_base_config_memory_assign_address_from_allocated_chunks()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the packets sg list and buffers for the
 *  caller.
 * 
 * @param base_config_p         - pointer to the base config object.
 * @param memory_request_p      - pointer to the allocated memory request.
 * @param new_packet_p          - arary of new packet pointers.
 * @param number_of_packets     - number of packets.
 * @param buffer_count          - number of buffers.
 * @param buffer_size           - buffer size.
 * @param number_of_chunks_p    - pointer to number of chunks.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
  *   10/11/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_base_config_memory_assign_address_from_allocated_chunks(fbe_base_config_t * base_config_p,
                                                            fbe_memory_request_t * memory_request_p,
                                                            fbe_base_config_memory_alloc_chunks_address_t *mem_alloc_chunks_addr)
{
    fbe_u32_t               sg_list_size = 0;
    fbe_u32_t               subpacket_size = 0;
    fbe_u32_t               buffer_ptr_size = 0;
    fbe_u32_t               pre_read_desc_size = 0;
    fbe_u32_t               index = 0;
    fbe_packet_t *          new_packet_p = NULL;
    fbe_u8_t *              buffer_p = NULL;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_memory_header_t *   master_memory_header = NULL;
    fbe_memory_header_t *   current_memory_header = NULL;
    fbe_u32_t               used_memory_in_chunk = 0;
    fbe_u32_t               number_of_used_chunks = 0;
    fbe_u32_t               sg_element_count = mem_alloc_chunks_addr->sg_element_count;
    fbe_u32_t               number_of_packets = mem_alloc_chunks_addr->number_of_packets;
    fbe_u32_t               buffer_count = mem_alloc_chunks_addr->buffer_count;
    fbe_sg_element_t        **sg_list_p = &mem_alloc_chunks_addr->sg_list_p;
    fbe_packet_t            ***packet_array_p = &mem_alloc_chunks_addr->packet_array_p;
    fbe_u8_t                ***buffer_array_p = &mem_alloc_chunks_addr->buffer_array_p;
    fbe_u8_t                **pre_read_buffer_p = &mem_alloc_chunks_addr->pre_read_buffer_p;
    fbe_payload_pre_read_descriptor_t **pre_read_desc_p = mem_alloc_chunks_addr->pre_read_desc_p;
    fbe_sg_element_t        **pre_read_sg_list_p = mem_alloc_chunks_addr->pre_read_sg_list_p;
    fbe_bool_t              assign_pre_read_desc = mem_alloc_chunks_addr->assign_pre_read_desc;
    fbe_u32_t               buffer_size = mem_alloc_chunks_addr->buffer_size;
    
    /* If memory request is null then return error. */
    if(memory_request_p->ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*) base_config_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s:memory request pointer is null, num_pkts:0x%x, sg_count:0x%x, buff_cnt:0x%x\n",
                              __FUNCTION__, number_of_packets, sg_element_count, buffer_count);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* get the pointer to the allocated memory from the memory request. */
    master_memory_header = fbe_memory_get_first_memory_header(memory_request_p);
    current_memory_header = master_memory_header;

    /* round the sg list to align with 64-bit boundary. */
    sg_list_size = sizeof(fbe_sg_element_t) * sg_element_count;
    sg_list_size += (sg_list_size % sizeof(fbe_u64_t));

    if((sg_list_size != 0) && (sg_list_p != NULL))
    {
        /* assign memory address for the sg element. */
        status = fbe_base_config_memory_assign_memory_address(base_config_p,
                                                              memory_request_p,
                                                              &current_memory_header,
                                                              &used_memory_in_chunk,
                                                              &number_of_used_chunks,
                                                              sg_list_size,
                                                              (fbe_u8_t **) sg_list_p);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s:assign addr failed, sg_size:0x%x, used_chk:0x%x, chk_size:0x%x, status:0x%x\n",
                                  __FUNCTION__, sg_list_size, master_memory_header->memory_chunk_size,
                                  number_of_used_chunks, status);
            return status;
        }
    }

    if((number_of_packets != 0) && (packet_array_p != NULL))
    {
        /* assign memory address for the packet pointers. */
        status = fbe_base_config_memory_assign_memory_address(base_config_p,
                                                              memory_request_p,
                                                              &current_memory_header,
                                                              &used_memory_in_chunk,
                                                              &number_of_used_chunks,
                                                              sizeof(fbe_packet_t *) * number_of_packets,
                                                              (fbe_u8_t **) packet_array_p);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s:assign addr failed, pkt_ptr_size:0x%llx, used_chk:0x%x, chk_size:0x%x, status:0x%x\n",
                                  __FUNCTION__,
				  (unsigned long long)(sizeof(fbe_packet_t *) * number_of_packets),
                                  master_memory_header->memory_chunk_size, number_of_used_chunks, status);
            return status;
        }
    }

    /* reserve the memory for the subpackets. */
    subpacket_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    for(index = 0; index < number_of_packets; index++)
    {
        new_packet_p = NULL;

        /* assign memory address for the packets. */
        status = fbe_base_config_memory_assign_memory_address(base_config_p,
                                                              memory_request_p,
                                                              &current_memory_header,
                                                              &used_memory_in_chunk,
                                                              &number_of_used_chunks,
                                                              subpacket_size,
                                                              (fbe_u8_t **) &new_packet_p);
        if(status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s:assign addr failed, pkt_size:0x%x, used_chk:0x%x, chk_size:0x%x, status:0x%x\n",
                                  __FUNCTION__, FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                  master_memory_header->memory_chunk_size, number_of_used_chunks, status);
            return status;
        }

        /* assign packet address to array of packet pointers. */
        (*packet_array_p)[index] = new_packet_p;
    }

    if((buffer_count != 0) && (buffer_array_p != NULL))
    {
        buffer_ptr_size = sizeof(fbe_u8_t *) * buffer_count;

        /* assign memory address for array of user buffer pointers. */
        status = fbe_base_config_memory_assign_memory_address(base_config_p,
                                                              memory_request_p,
                                                              &current_memory_header,
                                                              &used_memory_in_chunk,
                                                              &number_of_used_chunks,
                                                              buffer_ptr_size,
                                                              (fbe_u8_t **) buffer_array_p);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s:assign addr failed, buff_ptr_size:0x%x, used_chk:0x%x, chk_size:0x%x, status:0x%x\n",
                                  __FUNCTION__, buffer_ptr_size,
                                  master_memory_header->memory_chunk_size, number_of_used_chunks, status);
            return status;
        }
    }

    for(index = 0; index < buffer_count; index++)
    {
        buffer_p = NULL;

        /* reserve the memory for the subpackets. */
        status = fbe_base_config_memory_assign_memory_address(base_config_p,
                                                              memory_request_p,
                                                              &current_memory_header,
                                                              &used_memory_in_chunk,
                                                              &number_of_used_chunks,
                                                              buffer_size,
                                                              (fbe_u8_t **) &buffer_p);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s:assign addr failed, buff_size:0x%x, used_chk:0x%x, chk_size:0x%x, status:0x%x\n",
                                  __FUNCTION__, FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                  master_memory_header->memory_chunk_size, number_of_used_chunks, status);
            return status;
        }

        /* assign buffer address to array of buffer pointers. */
        (*buffer_array_p)[index] = buffer_p;
    }

    if(assign_pre_read_desc)
    {
        buffer_p = NULL;
        pre_read_desc_size = sizeof(fbe_payload_pre_read_descriptor_t) * 2 * FBE_RAID_MAX_DISK_ARRAY_WIDTH + 
                                (FBE_METADATA_BLOCK_SIZE * 2) + (sizeof(fbe_sg_element_t) * 2);
        pre_read_desc_size += (pre_read_desc_size % sizeof(fbe_u64_t));

        /* assign the memory for the pre read desc. */
        status = fbe_base_config_memory_assign_memory_address(base_config_p,
                                                              memory_request_p,
                                                              &current_memory_header,
                                                              &used_memory_in_chunk,
                                                              &number_of_used_chunks,
                                                              pre_read_desc_size,
                                                              (fbe_u8_t **) &buffer_p);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s:assign addr failed, pre_read_desc_size:0x%x, used_chk:0x%x, chk_size:0x%x, status:0x%x\n",
                                  __FUNCTION__, pre_read_desc_size,
                                  master_memory_header->memory_chunk_size, number_of_used_chunks, status);
            return status;
        }

        /* Set the pre read descriptor, sg list and its buffer. */
        pre_read_desc_p[FBE_BASE_CONFIG_SIGNATURE_MIRROR_INDEX_PRIMARY] = (fbe_payload_pre_read_descriptor_t *) buffer_p;
        buffer_p += sizeof(fbe_payload_pre_read_descriptor_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH;
        pre_read_desc_p[FBE_BASE_CONFIG_SIGNATURE_MIRROR_INDEX_SECONDARY] = (fbe_payload_pre_read_descriptor_t *) buffer_p;
        buffer_p += sizeof(fbe_payload_pre_read_descriptor_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH;

        *pre_read_sg_list_p = (fbe_sg_element_t *) buffer_p;
        buffer_p += (sizeof(fbe_sg_element_t) * 2);
        (*pre_read_buffer_p) = buffer_p;

    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_memory_assign_address_from_allocated_chunks()
 ******************************************************************************/


