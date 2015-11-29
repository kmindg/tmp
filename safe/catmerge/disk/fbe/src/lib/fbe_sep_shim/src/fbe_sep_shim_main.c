/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_sep_shim_main.c
 ***************************************************************************
 *
 *  Description
 *      common implementation for kernel and user of the SEP shim
 *      
 *
 *  History:
 *      06/24/09    sharel - Created
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe/fbe_queue.h"

/*******************************************
				Local functions
********************************************/

/*********************************************************************
 *            fbe_sep_shim_trace ()
 *********************************************************************
 *
 *  Description: tracing shortcut fro fbe API.
 *				 This API is intened only for tracing with FBE_TRACE_MESSAGE_ID_INFO
 *				 which is the most common one.
 *    
 *********************************************************************/
void fbe_sep_shim_trace(fbe_trace_level_t trace_level,
						const fbe_u8_t * fmt,
						...)
{
    va_list 			argList;
	fbe_trace_level_t 	default_trace_level = fbe_trace_get_default_trace_level();

    if (trace_level > default_trace_level) {
        return;
    }

    va_start(argList, fmt);
    fbe_trace_report (FBE_COMPONENT_TYPE_LIBRARY,
					  FBE_LIBRARY_ID_FBE_SEP_SHIM,
					  trace_level,
                      FBE_TRACE_MESSAGE_ID_INFO,
					  fmt,
					  argList); 
    va_end(argList);

}


fbe_u32_t fbe_sep_shim_calculate_allocation_chunks(fbe_block_count_t blocks, fbe_u32_t *chunks, fbe_memory_chunk_size_t *chunk_size)
{
    fbe_u32_t                           number_of_chunks = 0;
    fbe_memory_chunk_size_t             size;
    fbe_memory_chunk_size_block_count_t chunk_blocks;
    fbe_u32_t                           sg_list_size = 0;
    fbe_u64_t                           last_transfer_size = 0;
    fbe_u32_t                           xor_move_size = 0;

    if (blocks > FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET)
    {
        size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
        chunk_blocks = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64;
    }
    else
    {
        size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
        chunk_blocks = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET;
    }

    // 1. number_of_chunks need to enough to hold all blocks
    if((blocks/chunk_blocks) > FBE_U32_MAX)
    {
        /* we do not expect the number of chunks to cross 32 bit limit
          */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    number_of_chunks = (fbe_u32_t)(blocks / chunk_blocks);

    if (blocks % chunk_blocks)
    {
        last_transfer_size = (blocks % chunk_blocks)*FBE_BE_BYTES_PER_BLOCK;
        number_of_chunks ++; //one more chunk for the leftover
    }

    // 2. size the sg_list, enough to hold one extra chunk
    sg_list_size = (sizeof(fbe_sg_element_t) * (number_of_chunks+2));
    // round up to align with 64-bit boundary.
    sg_list_size += (sg_list_size % sizeof(fbe_u64_t));

    // 3. add xor_mem_move, with alignment
    xor_move_size = sizeof(fbe_xor_mem_move_t);
    xor_move_size += (xor_move_size % sizeof(fbe_u64_t));

    // 4. fit again, 
    if ((last_transfer_size==0) ||
        ((sg_list_size + last_transfer_size + sizeof(fbe_packet_t) + xor_move_size) > (fbe_u32_t)size))
    {
        /*
         * If xor_move_size + packet size is greater than chunck size, the chunck size
         * must be just for the packet.  In this case, we need 2 chuncks to fit both
         * xor_mov and the packets.  There will be no problem fitting sg_list with 
         * the xor_mov in the same chunk.  And we keep whatever data left in its own chunck.
         */
        if ((sg_list_size + sizeof(fbe_packet_t) + xor_move_size) > (fbe_u32_t)size)
        {
            number_of_chunks += 2;
        }
        else
        {
            number_of_chunks ++;
        }
    }

    *chunks = number_of_chunks;
    *chunk_size = size; 

    return (FBE_STATUS_OK);
}

