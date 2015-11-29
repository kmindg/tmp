/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_memory.c
 ***************************************************************************
 *
 * @brief
 *  This file contains rdgen functionality for dealing with memory.
 *
 * @version
 *   10/12/2010:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private_inlines.h"
#include "fbe_memory_private.h" /*for dps */
#include "fbe_cmm.h"

static fbe_status_t fbe_rdgen_cmm_memory_destroy(void);
static fbe_status_t fbe_rdgen_cmm_memory_init(void);

/*!**************************************************************
 * fbe_rdgen_memory_initialize()
 ****************************************************************
 * @brief
 *  Initialize the memory portion of rdgen.
 *  Currently this is only statistics.
 *
 * @param None.               
 *
 * @return fbe_status_t
 *
 * @author
 *  10/12/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_memory_initialize(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();

    fbe_zero_memory(&rdgen_p->memory_statistics, sizeof(fbe_rdgen_memory_stats_t));
    fbe_spinlock_init(&rdgen_p->memory_statistics.spin);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_rdgen_cmm_initialize()
 ****************************************************************
 * @brief
 *  Initialize the rdgen cmm memory.
 *
 * @param None.               
 *
 * @return fbe_status_t
 *
 * @author
 *  12/07/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_cmm_initialize(void)
{
    fbe_rdgen_cmm_memory_init();
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_rdgen_cmm_destroy()
 ****************************************************************
 * @brief
 *  destroy cmm with deregister .
 *
 * @param None.               
 *
 * @return fbe_status_t
 *
 * @author
 *  12/07/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_cmm_destroy(void)
{
    fbe_rdgen_cmm_memory_destroy();
    return FBE_STATUS_OK;
}

/******************************************
 * end fbe_rdgen_memory_initialize()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_memory_destroy()
 ****************************************************************
 * @brief
 *  Destroy the memory related pieces of rdgen.
 *
 * @param None.               
 *
 * @return fbe_status_t FBE_STATUS_GENERIC_FAILURE On error.
 *                      FBE_STATUS_OK for success.
 * @author
 *  12/18/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_memory_destroy(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();

    fbe_spinlock_destroy(&rdgen_p->memory_statistics.spin);

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: rdgen max bytes %d\n",
                            __FUNCTION__,
                            rdgen_p->memory_statistics.max_bytes);

    /* We should not have any memory allocated.
     */
    if (rdgen_p->memory_statistics.current_bytes != 0)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: rdgen leaked %d bytes\n",
                                 __FUNCTION__,
                                rdgen_p->memory_statistics.current_bytes);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    /* We should have freed all the objects.
     */
    if (rdgen_p->memory_statistics.obj_allocated_count !=
        rdgen_p->memory_statistics.obj_freed_count)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: obj_allocated 0x%llx != obj_freed 0x%llx\n",
                                 __FUNCTION__, 
                                (unsigned long long)rdgen_p->memory_statistics.obj_allocated_count,
                                (unsigned long long)rdgen_p->memory_statistics.obj_freed_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    /* We should have freed all our requests.
     */
    if (rdgen_p->memory_statistics.req_allocated_count !=
        rdgen_p->memory_statistics.req_freed_count)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: req_allocated 0x%llx != req_freed 0x%llx\n",
                                 __FUNCTION__, 
                                (unsigned long long)rdgen_p->memory_statistics.req_allocated_count,
                                (unsigned long long)rdgen_p->memory_statistics.req_freed_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (rdgen_p->memory_statistics.ts_allocated_count !=
        rdgen_p->memory_statistics.ts_freed_count)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: ts_allocated 0x%llx != ts_freed 0x%llx\n",
                                 __FUNCTION__, 
                                (unsigned long long)rdgen_p->memory_statistics.ts_allocated_count,
                                (unsigned long long)rdgen_p->memory_statistics.ts_freed_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (rdgen_p->memory_statistics.mem_allocated_count !=
        rdgen_p->memory_statistics.mem_freed_count)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: mem_allocated 0x%llx != mem_freed 0x%llx\n",
                                 __FUNCTION__, 
                                (unsigned long long)rdgen_p->memory_statistics.mem_allocated_count,
                                (unsigned long long)rdgen_p->memory_statistics.mem_freed_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (rdgen_p->memory_statistics.sg_allocated_count !=
        rdgen_p->memory_statistics.sg_freed_count)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: sg_allocated 0x%llx != sg_freed 0x%llx\n",
                                 __FUNCTION__, 
                                (unsigned long long)rdgen_p->memory_statistics.sg_allocated_count,
                                (unsigned long long)rdgen_p->memory_statistics.sg_freed_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/******************************************
 * end fbe_rdgen_memory_destroy()
 ******************************************/

/*!****************************************************************************
 *  fbe_rdgen_memory_info_init()
 ******************************************************************************
 * @brief
 *  Initialize our memory information structure.
 *  
 * @param info_p - Structure to init.
 * @param memory_request_p - Allocated memory request.
 * @param bytes_per_page - Number of bytes we want to use in every page.
 *                         If 0, we just use the entire memory chunk
 *                         which is specified inside memory request.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *  10/18/2010 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_rdgen_memory_info_init(fbe_rdgen_memory_info_t *info_p,
                                        fbe_memory_request_t *memory_request_p,
                                        fbe_u32_t bytes_per_page)
{
    if (memory_request_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: memory request is null. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (memory_request_p->ptr == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: memory request memory ptr is null. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (memory_request_p->chunk_size == 0)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: memory request chunk size is zero \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    info_p->memory_header_p = memory_request_p->ptr;

    /* If no chunk size passed, then use the default chunk size in the request.
     */
    if (bytes_per_page == 0)
    {
        bytes_per_page = memory_request_p->chunk_size;
    }
    info_p->bytes_remaining = bytes_per_page;
    info_p->page_size_bytes = bytes_per_page;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_rdgen_memory_info_init()
 **************************************/
/*!****************************************************************************
 *  fbe_rdgen_ts_get_chunk_size()
 ******************************************************************************
 * @brief
 *  Round the chunk size for this request.
 *  
 * @param ts_p - Pointer to the rdgen_ts.
 * @param chunk_size_p - Input and output.  Input is the chunk size
 *                       of the memory service we plan to use.
 *                       Output is the chunk size we will use.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *  10/13/2010 - Created. Rob Foley 
 *
 ******************************************************************************/
fbe_status_t fbe_rdgen_ts_get_chunk_size(fbe_rdgen_ts_t *ts_p, fbe_u32_t *chunk_size_p)
{
    fbe_u32_t new_chunk_size = *chunk_size_p;
    /* First make sure it is an even number of blocks.
     */
    new_chunk_size = (new_chunk_size / ts_p->block_size) * ts_p->block_size;

    /* Half the time we will use exactly the memory service chunk size. 
     * The other half of the time we will use a nice even multiple. 
     * This will give us reasonable coverage to test cases where sg counts are not the same. 
     *  
     * We use the corrupted blocks bitmap since this is a random number 
     * generated at the beginning of the pass. 
     *  
     * We need to use a consistent chunk size value across the entire pass since the counts in each SG need to be the 
     * same for a corrupt pass and the read pass since the corrupt bitmask determines placement of the corrupt pattern 
     * in an sg and the mod of the corrupt bitmask with the index of a sector into an sg determines if it is corrupt or 
     * not. 
     */
    if (ts_p->corrupted_blocks_bitmap % 2)
    {
        /* If the chunk size is large, make the memory left a multiple of 64 or 32.
         */
        if (new_chunk_size >= (64 * FBE_BE_BYTES_PER_BLOCK))
        {
            new_chunk_size -= new_chunk_size % (64 * FBE_BE_BYTES_PER_BLOCK);
        }
        else if (new_chunk_size > (32 * FBE_BE_BYTES_PER_BLOCK))
        {
            new_chunk_size -= new_chunk_size % (32 * FBE_BE_BYTES_PER_BLOCK);
        }
    }

    *chunk_size_p = new_chunk_size;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_rdgen_ts_get_chunk_size
 **************************************/

/*!**************************************************************
 * fbe_rdgen_allocate_memory()
 ****************************************************************
 * @brief
 *  Allocate memory for rdgen.  All allocates for rdgen
 *  should go through this.
 *
 * @param allocation_size_in_bytes - Number of bytes to alloc.               
 *
 * @return void * - ptr to allocated memory.   
 *
 * @author
 *  12/18/2009 - Created. Rob Foley
 *
 ****************************************************************/

void * fbe_rdgen_allocate_memory(fbe_u32_t allocation_size_in_bytes)
{
    void * memory_p = NULL;
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();

    memory_p = fbe_memory_native_allocate(allocation_size_in_bytes);

    if (memory_p == NULL)
    {
        fbe_atomic_increment(&rdgen_p->memory_statistics.memory_exausted_count);
    }
    else
    {
        fbe_atomic_32_add(&rdgen_p->memory_statistics.current_bytes, allocation_size_in_bytes);
        if (rdgen_p->memory_statistics.current_bytes >
            rdgen_p->memory_statistics.max_bytes)
        {
            fbe_spinlock_lock(&rdgen_p->memory_statistics.spin);
            if (rdgen_p->memory_statistics.current_bytes >
                rdgen_p->memory_statistics.max_bytes)
            {
                rdgen_p->memory_statistics.max_bytes = 
                    rdgen_p->memory_statistics.current_bytes;
            }
            fbe_spinlock_unlock(&rdgen_p->memory_statistics.spin);
        }
        fbe_atomic_increment(&rdgen_p->memory_statistics.allocations);
        fbe_atomic_add(&rdgen_p->memory_statistics.allocated_bytes, allocation_size_in_bytes);
    }
    return memory_p;
}
/******************************************
 * end fbe_rdgen_allocate_memory()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_free_memory()
 ****************************************************************
 * @brief
 *  Free memory for rdgen.
 *
 * @param  ptr - memory to free               
 * @param size - of memory to free
 *
 * @return - None.   
 *
 * @author
 *  12/18/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_free_memory(void * ptr, fbe_u32_t size)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    /* Adding a negative gives you a decrement.
     */
    fbe_atomic_32_add(&rdgen_p->memory_statistics.current_bytes, -((fbe_s32_t)size));
    fbe_atomic_increment(&rdgen_p->memory_statistics.frees);
    fbe_atomic_add(&rdgen_p->memory_statistics.freed_bytes, size);
    fbe_memory_native_release(ptr);
    return;
}
/******************************************
 * end fbe_rdgen_free_memory()
 ******************************************/

/*!****************************************************************************
 *  fbe_rdgen_ts_allocation_completion()
 ******************************************************************************
 * @brief
 *  Completion function for an rdgen ts to allocate memory.
 *  
 *
 * @param memory_request    - Pointer to memory request.
 * @param context           - Pointer to the rdgen_ts.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *  10/11/2010 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_rdgen_ts_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                   fbe_memory_completion_context_t context)
{
    fbe_rdgen_ts_t *ts_p = (fbe_rdgen_ts_t*) context;
    fbe_status_t status;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                "%s: request: 0x%p lba: %llx bl: 0x%llx pr_block: 0x%llx\n", 
                                __FUNCTION__, memory_request_p,
                                (unsigned long long)ts_p->lba,
				(unsigned long long)ts_p->blocks,
				(unsigned long long)ts_p->pre_read_blocks);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        fbe_rdgen_ts_state(ts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Sanity check object pointer */
    if (RDGEN_COND(ts_p->object_p == NULL))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                "%s: fbe_rdgen_ts object is null. \n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        fbe_rdgen_ts_state(ts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Simply put this on a thread.
     */
    fbe_rdgen_inc_mem_allocated();
    status = fbe_rdgen_ts_enqueue_to_thread(ts_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_rdgen_ts_allocation_completion()
 **************************************/

/*!**************************************************************
 * fbe_rdgen_ts_allocate_memory()
 ****************************************************************
 * @brief
 *  Allocate memory for this rdgen ts.
 *
 * @param ts_p - Current ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_WAITING if memory could not
 *         be allocated, else FBE_RDGEN_TS_STATE_STATUS_EXECUTING.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_allocate_memory(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status;
    fbe_u32_t bytes;
    fbe_u32_t sg_bytes;
    fbe_u32_t memory_chunk_size; /* Chunk size according to memory service. */
    fbe_u32_t num_chunks = 0;
    fbe_memory_request_t *memory_request_p = fbe_rdgen_ts_get_memory_request(ts_p);
    fbe_u32_t chunk_size;  /* Chunk size we are using to align our I/Os.  */

    /* Memory required is sg together with block memory.
     */

    if(((ts_p->current_blocks + ts_p->pre_read_blocks) * ts_p->block_size) > FBE_U32_MAX)
    {
        /* exceeding expected 32bit limit!
          */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                "%s: unexpected: bytes crossing 32bit limit\n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    bytes = (fbe_u32_t)((ts_p->current_blocks + ts_p->pre_read_blocks) * ts_p->block_size);
    if (ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_SYNC_IO)
    {
        /* Extra one sector for alignment */
        bytes += FBE_BYTES_PER_BLOCK;
    }
    
    ts_p->memory_size = bytes;

    if ((ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_MJ) &&
        ((ts_p->current_blocks * ts_p->block_size) > FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO) ||
        (ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_SYNC_IO) && (bytes >= FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO))
    {
        void* memory_p = NULL;

        /* Allocate enough space for two sgs.
         */
        bytes += 2 * sizeof(fbe_sg_element_t);

        /* We need contiguous memory.  Allocate it now synchronously.
         */
        if (fbe_rdgen_get_memory_type() == FBE_RDGEN_MEMORY_TYPE_CACHE)
        {
            /* This could be asynchrounous, but we currently don't support it.
             */
            memory_p = fbe_rdgen_alloc_cache_mem(bytes);
            fbe_rdgen_ts_set_memory_type(ts_p, FBE_RDGEN_MEMORY_TYPE_CACHE);
        }
        else
        {
            /* This is always synchronous.
             */
            memory_p = fbe_rdgen_cmm_memory_allocate(bytes);
            fbe_rdgen_ts_set_memory_type(ts_p, FBE_RDGEN_MEMORY_TYPE_CMM);
        }
        fbe_rdgen_ts_set_memory_p(ts_p, memory_p);
        if (memory_p == NULL)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                    "%s: Unable to allocate memory type: %d.\n", __FUNCTION__, fbe_rdgen_get_memory_type());
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
        }
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /* We just use a single DPS pool.
     */
    memory_chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;

    /* Decide how much memory in each chunk we want to use.
     */
    chunk_size = memory_chunk_size;
    fbe_rdgen_ts_get_chunk_size(ts_p, &chunk_size);

    /* determine the number of chunks we need
     */
    num_chunks = (bytes / chunk_size);

    /* Add one full chunk for remainder.
     */
    if (bytes % chunk_size)
    {
        num_chunks += 1;
    }
    num_chunks++; /* Add one so we can randomly add extra mem at end. */

    /* For sg lists we will need at least as many sg entries as chunks.
     */
    sg_bytes = sizeof(fbe_sg_element_t) * (num_chunks + 6);

    /* Round up to a full block. 
     * Since we allocate the sgs first, it causes the remaining memory to not be an 
     * even number of blocks.  We add on to account for the wasted memory caused by this. 
     */
    if (sg_bytes % FBE_BE_BYTES_PER_BLOCK)
    {
        sg_bytes += FBE_BE_BYTES_PER_BLOCK - (sg_bytes % FBE_BE_BYTES_PER_BLOCK);
    }
    bytes += sg_bytes;

    /* determine the number of chunks we need
     */
    num_chunks = (bytes / chunk_size);

    /* Add one full chunk for remainder.
     */
    if (bytes % chunk_size)
    {
        num_chunks += 1;
    }

    /* build the memory request for allocation.
     */
    fbe_memory_request_init(memory_request_p);
    fbe_memory_build_chunk_request(memory_request_p,
                                   memory_chunk_size,
                                   num_chunks,
                                   FBE_MEMORY_DPS_RDGEN_BASE_PRIORITY,
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                  (fbe_memory_completion_function_t) fbe_rdgen_ts_allocation_completion,
                                   ts_p);

    /* Issue the memory request to memory service.
     */
    status = fbe_memory_request_entry(memory_request_p);

    /* Pending and OK are valid status values.  Everything else is unexpected.
     */
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        /* memory request failed.  Return waiting to state machine and complete ts with error.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                "%s: status unexpected 0x%x bytes: 0x%x lba: %llx bl: 0x%llx pr_block: 0x%llx\n", 
                                __FUNCTION__, status, bytes,
				(unsigned long long)ts_p->lba,
				(unsigned long long)ts_p->blocks,
				(unsigned long long)ts_p->pre_read_blocks);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    return FBE_RDGEN_TS_STATE_STATUS_WAITING;
}
/******************************************
 * end fbe_rdgen_ts_allocate_memory()
 ******************************************/
/*!****************************************************************************
 *  fbe_rdgen_allocate_request_completion()
 ******************************************************************************
 * @brief
 *  Completion function for allocate rdgen request memory and rdgen ts memory. *  
 *
 * @param memory_request    - Pointer to memory request.
 * @param context           - Pointer to the packet.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *  10/12/2010 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_rdgen_allocate_request_completion(fbe_memory_request_t * memory_request_p, 
                                      fbe_memory_completion_context_t context)
{
    fbe_packet_t *packet_p = (fbe_packet_t*) context;

    if (packet_p == NULL)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: packet is null. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: request: 0x%p state: %d\n", 
                                __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    }

    /* caller to allocate memory setup a completion function on the packet.
     */
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_rdgen_allocate_request_completion()
 **************************************/
/*!**************************************************************
 * fbe_rdgen_allocate_request_memory()
 ****************************************************************
 * @brief
 *  Allocate memory for this rdgen request.  We will allocate
 *  the request and all ts' associated with this request.
 *
 * @param packet_p - Ptr to the packet.
 * @param num_ts - Total fbe_rdgen_ts_t to allocate to the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/12/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_allocate_request_memory(fbe_packet_t *packet_p, fbe_u32_t num_ts)
{
    fbe_status_t status;
    fbe_u32_t bytes;
    fbe_u32_t memory_chunk_size; /* Chunk size according to memory service. */
    fbe_u32_t num_chunks = 0;
    fbe_memory_request_t *memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_u32_t memory_chunk_bytes_remaining;
    fbe_u32_t num_ts_remaining;
    
    /* Memory required is sg together with block memory.
     */
    bytes = (sizeof(fbe_rdgen_ts_t) * num_ts) + sizeof(fbe_rdgen_request_t);

    /* Use a small chunk size (32 blocks) if everything fits in one chunk. 
     * This is an arbitrary choice.
     */
    memory_chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;

	num_ts_remaining = num_ts;
    /* First chunk will take out the request.
     */
    memory_chunk_bytes_remaining = memory_chunk_size - sizeof(fbe_rdgen_request_t);
    /* Remainder of first chunk used for other fbe_rdgen_ts_t.
     * Each loop iteration allocates the rest of a chunk to fbe_rdgen_ts_t until we 
     * run out of ts. 
     */
    while (num_ts_remaining > 0)
    {
        fbe_u32_t max_ts_for_chunk = (memory_chunk_bytes_remaining / sizeof(fbe_rdgen_ts_t));
        num_ts_remaining -= FBE_MIN(num_ts_remaining, max_ts_for_chunk);
        memory_chunk_bytes_remaining = memory_chunk_size;
        num_chunks++;
    }

    /* build the memory request for allocation.
     */
    fbe_memory_request_init(memory_request_p);
    fbe_memory_build_chunk_request(memory_request_p,
                                   memory_chunk_size,
                                   num_chunks,
                                   FBE_MEMORY_DPS_RDGEN_BASE_PRIORITY,
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                   (fbe_memory_completion_function_t) fbe_rdgen_allocate_request_completion,
                                   packet_p);

    /* Issue the memory request to memory service.
     */
    status = fbe_memory_request_entry(memory_request_p);

    /* Pending and OK are valid status values.  Everything else is unexpected.
     */
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        /* memory request failed.  Return waiting to state machine and complete ts with error.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: status unexpected 0x%x packet: %p\n", 
                                __FUNCTION__, status, packet_p);
        return status;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_allocate_request_memory()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_free_memory()
 ****************************************************************
 * @brief
 *  Free memory for this rdgen ts.
 *
 * @param ts_p - Current ts.               
 *
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_ts_free_memory(fbe_rdgen_ts_t *ts_p)
{
    void *memory_p = fbe_rdgen_ts_get_memory_p(ts_p);

    if (memory_p != NULL)
    {
        if (ts_p->memory_type == FBE_RDGEN_MEMORY_TYPE_CACHE)
        {
            fbe_rdgen_release_cache_mem(memory_p);
        }
        else
        {
            fbe_rdgen_cmm_memory_release(memory_p);
        }
        fbe_rdgen_ts_set_memory_p(ts_p, NULL);
        fbe_rdgen_ts_set_memory_type(ts_p, FBE_RDGEN_MEMORY_TYPE_INVALID);
    }
    /* Release the memory and null out the ptrs 
     * we carved out of this memory. 
     */
    if (ts_p->memory_request.ptr != NULL)
    {
		fbe_memory_free_request_entry(&ts_p->memory_request);

        fbe_rdgen_inc_mem_freed();
        ts_p->memory_size = 0;
        ts_p->memory_request.ptr = NULL;
    }
    return;
}
/******************************************
 * end fbe_rdgen_ts_free_memory()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_memory_allocate_next_buffer()
 ****************************************************************
 * @brief
 *  Allocate the next piece of memory from the set of
 *  pages we allocated previously.
 * 
 * @param info_p - Ptr to info on our memory allocation.
 * @param bytes_to_allocate
 * @param bytes_allocated_p
 * @param b_allocate_contiguous - FBE_TRUE to allocate a contiguous
 *                                piece of memory for this allocation.
 * 
 * @return void* 
 *
 * @author
 *  10/11/2010 - Created. Rob Foley
 *
 ****************************************************************/
void * fbe_rdgen_memory_allocate_next_buffer(fbe_rdgen_memory_info_t *info_p,
                                             fbe_u32_t bytes_to_allocate,
                                             fbe_u32_t *bytes_allocated_p,
                                             fbe_bool_t b_allocate_contiguous)
{
    void *memory_p = NULL;
    fbe_u8_t *address = NULL;
    fbe_u32_t bytes_allocated;

    if (info_p->memory_header_p == NULL)
    {
        return NULL;
    }

    /* If we are not allocating contiguous then we will take whatever is left. 
     * If we are allocating contiguous we only take what is left if it is enough.
     */
    if ((!b_allocate_contiguous && (info_p->bytes_remaining > 0)) ||
        (b_allocate_contiguous && (info_p->bytes_remaining >= bytes_to_allocate)))
    {
        /* Enough space left for this buffer.
         * Calculate the offset from the start to the fruts. 
         */
        address = info_p->memory_header_p->data;
        address += (info_p->page_size_bytes - info_p->bytes_remaining);
        memory_p = address;
    }
    else
    {
        /* Not enough space, goto the next page.
         * Setup our return memory_header_ptr to point to this next header. 
         */
        info_p->memory_header_p = info_p->memory_header_p->u.hptr.next_header;
        if (info_p->memory_header_p == NULL)
        {
            /* Memory list is empty, return NULL.
             */
            return NULL;
        }
        /* Get the pointer to the data we will be returning.
         */
        memory_p = info_p->memory_header_p->data;
        info_p->bytes_remaining = info_p->page_size_bytes;
    }

    /* Allocate what we can from this page.
     */
    bytes_allocated = FBE_MIN(info_p->bytes_remaining, bytes_to_allocate);
    info_p->bytes_remaining -= bytes_allocated;
    *bytes_allocated_p = bytes_allocated;
    return memory_p;
}
/******************************************
 * end fbe_rdgen_memory_allocate_next_buffer()
 ******************************************/

static CMM_POOL_ID              cmm_control_pool_id = 0;
static CMM_CLIENT_ID            cmm_client_id = 0;
static fbe_status_t fbe_rdgen_cmm_memory_init(void)
{
   /* Register usage with the available pools in the CMM 
    * and get a small seed pool for use by NEIT. 
    */
   cmm_control_pool_id = cmmGetLongTermControlPoolID();
   cmmRegisterClient(cmm_control_pool_id, 
                        NULL, 
                        0,  /* Minimum size allocation */  
                        CMM_MAXIMUM_MEMORY,   
                        "FBE NEIT memory", 
                        &cmm_client_id);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_rdgen_cmm_memory_destroy(void)
{
    cmmDeregisterClient(cmm_client_id);
    return FBE_STATUS_OK;
}

void * fbe_rdgen_cmm_memory_allocate(fbe_u32_t allocation_size_in_bytes)
{
    void * ptr = NULL;
    CMM_ERROR cmm_error;

    cmm_error = cmmGetMemoryVA(cmm_client_id, allocation_size_in_bytes, &ptr);
    if (cmm_error != CMM_STATUS_SUCCESS)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s entry 1 cmmGetMemoryVA fail 0x%x\n", __FUNCTION__, cmm_error);
	}
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s success allocate %d bytes ptr: %p\n", __FUNCTION__, allocation_size_in_bytes, ptr);
    }
    return ptr;
}

void fbe_rdgen_cmm_memory_release(void * ptr)
{
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s free ptr: %p\n", __FUNCTION__, ptr);
    cmmFreeMemoryVA(cmm_client_id, ptr);
    return;
}

fbe_status_t fbe_rdgen_release_memory(void)
{
    fbe_rdgen_lock();
    if ((fbe_rdgen_get_num_ts() > 0) || !fbe_rdgen_peer_thread_is_empty())
    {
        fbe_rdgen_unlock();
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Threads are active, cannot free memory.\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* Clear the flag so no new requests will get in.
     */
    fbe_rdgen_service_clear_flag(FBE_RDGEN_SERVICE_FLAGS_DPS_MEM_INITTED);

    fbe_rdgen_unlock();
    fbe_memory_dps_destroy();
    fbe_rdgen_set_memory_type(FBE_RDGEN_MEMORY_TYPE_INVALID);
    fbe_rdgen_set_memory_size(0);

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s rdgen memory freed successfully\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
/*************************
 * end file fbe_rdgen_memory.c
 *************************/


