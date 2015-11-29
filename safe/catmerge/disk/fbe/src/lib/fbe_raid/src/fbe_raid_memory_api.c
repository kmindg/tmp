/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_memory_api.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the interface for the backend of the raid memory
 *  module.
 * 
 *  This code is designed to abstract raid away from some of the details
 *  of how the memory is allocated.
 *
 * @version
 *   8/5/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_sg_element.h"
#include "fbe/fbe_memory.h"
#include "fbe_raid_memory_api.h"
#include "fbe_raid_pool_entries.h"
#include "fbe_raid_library_proto.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fbe_raid_memory_init_zero_buffer(void);

/*************************
 *   GLOBAL DEFINITIONS
 *************************/
static fbe_raid_memory_stats_t fbe_raid_memory_statistics;
static fbe_bool_t fbe_raid_memory_initialized = FBE_FALSE;


/*************************
 *   METHODS
 *************************/

fbe_bool_t fbe_raid_memory_api_is_initialized(void)
{
    return(fbe_raid_memory_initialized);
}

fbe_status_t fbe_raid_memory_initialize(void)
{
    fbe_status_t status;
    fbe_raid_memory_initialized = FBE_TRUE;
    fbe_zero_memory(&fbe_raid_memory_statistics, sizeof(fbe_raid_memory_stats_t));
    status = fbe_raid_memory_init_zero_buffer();

    /*! @note Validate that chunk size we will use is sufficient for the
     *        structure required.
     */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_siots_pool_entry_t)  <= FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_siots_pool_entry_t)  <= FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_vcts_pool_entry_t)   <= FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
    
	/* A fruts includes a packet and thus won't fit in a packet */
    //FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_fruts_pool_entry_t)  <= FBE_MEMORY_CHUNK_SIZE_FOR_32_BLOCKS_IO);

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_sg_1_pool_entry_t)   <= FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_sg_8_pool_entry_t)   <= FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_sg_32_pool_entry_t)  <= FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_sg_128_pool_entry_t) <= FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);

    //FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_sg_max_pool_entry_t) <= FBE_MEMORY_CHUNK_SIZE_FOR_32_BLOCKS_IO);

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_buffer_2kb_pool_entry_t) <= FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);

    //FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_buffer_pool_entry_t) <= FBE_MEMORY_CHUNK_SIZE_FOR_32_BLOCKS_IO);

    //FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_buffer_16kb_pool_entry_t) <= FBE_MEMORY_CHUNK_SIZE_FOR_32_BLOCKS_IO);

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_buffer_32kb_pool_entry_t) <= FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);

    /*! @note All raid library unit test do not support deferred allocations.
     *        Therefore do not remove the code below.
     */
    fbe_raid_memory_mock_setup_mocks();

    /* Always initialize the memory tracking
     */
    fbe_raid_memory_track_initialize();

    return status;
}
fbe_status_t fbe_raid_memory_destroy(void)
{
    /*! @note All raid library unit test do not support deferred allocations.
     *        Therefore do not remove the code below.
     */
    fbe_raid_memory_mock_destroy_mocks();

    /* Always destroy the memory tracking
     */
    fbe_raid_memory_track_destroy();

    fbe_raid_memory_initialized = FBE_FALSE;

    return FBE_STATUS_OK;
}
static fbe_sector_t fbe_raid_memory_zero_buffer[FBE_RAID_PAGE_SIZE_MAX];
static fbe_sg_element_t fbe_raid_memory_zero_buffer_sg[FBE_RAID_SG_MAX_ENTRIES];

void fbe_raid_memory_get_zero_sg(fbe_sg_element_t **sg_p)
{
    *sg_p = &fbe_raid_memory_zero_buffer_sg[0];
    return;
}
static fbe_status_t fbe_raid_memory_init_zero_buffer(void)
{
    fbe_xor_zero_buffer_t zero_buffer;
    fbe_status_t status;
    fbe_u32_t sg_index;

    /* Initialize the zero sg.
     */
    for (sg_index = 0; sg_index < FBE_RAID_SG_MAX_ENTRIES-1; sg_index++)
    {
        fbe_raid_memory_zero_buffer_sg[sg_index].count = FBE_BE_BYTES_PER_BLOCK * FBE_RAID_PAGE_SIZE_MAX;
        fbe_raid_memory_zero_buffer_sg[sg_index].address = (fbe_u8_t*)&fbe_raid_memory_zero_buffer[0];
    }
    
    /* Terminate the Sg.
     */
    fbe_sg_element_terminate(&fbe_raid_memory_zero_buffer_sg[sg_index]);

    zero_buffer.disks = 1;
    zero_buffer.status = FBE_STATUS_INVALID;
    zero_buffer.fru[0].sg_p = &fbe_raid_memory_zero_buffer_sg[0];
    zero_buffer.fru[0].count = 1;
    zero_buffer.fru[0].offset = 0;

    /* We use a seed and offset of 0 for the lba stamp since a zero lba stamp 
     * is always valid. 
     */
    zero_buffer.fru[0].seed = 0;
    zero_buffer.offset = 0;

    status = fbe_xor_lib_zero_buffer(&zero_buffer);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "status of %d from fbe_xor_zero_buffer\n", status);
    }
    return status;
}

/*!**************************************************************************
 * fbe_raid_memory_api_allocate()
 ****************************************************************************
 * @brief   This is the main entry point for allocating memory.
 *
 * @param   memory_request_p - Pointer to the memory service request
 * @param   b_memory_tracking_enabled - FBE_TRUE - Memory tracking is enabled
 *
 * @return  status - There are only (3) possible status values returned:
 *                  FBE_STATUS_OK - The memory was immediately available
 *                  FBE_STATUS_PENDING - The callback will be invoked when the
 *                                       memory is available
 *                  Other - (Typically FBE_STATUS_GENERIC_FAILURE) - Something
 *                          went wrong.
 * 
 * @author
 *  08/05/2009  Rob Foley - Created   
 *
 **************************************************************************/
fbe_status_t fbe_raid_memory_api_allocate(fbe_memory_request_t *memory_request_p, 
                                          fbe_bool_t b_memory_tracking_enabled)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_memory_chunk_size_t chunk_size = 0;

    /*! @note The priority is now determined by the memory service.  Therefore
     *        raid cannot compare against the class priority used to generate
     *        the memory request.
     */

    /* Validate the memory request state and number of objects
     */
    chunk_size = memory_request_p->chunk_size;
    if ((fbe_memory_request_is_build_chunk_complete(memory_request_p) == FBE_FALSE) ||
        ((chunk_size != FBE_MEMORY_CHUNK_SIZE_FOR_PACKET) && 
          (chunk_size != FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO) && 
          (chunk_size != FBE_MEMORY_CHUNK_SIZE_PACKET_PACKET) && 
          (chunk_size != FBE_MEMORY_CHUNK_SIZE_PACKET_64_BLOCKS) && 
          (chunk_size != FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET) && 
          (chunk_size != FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS)))
    {
        /* Fail the allocation request
         */
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: allocate memory request: 0x%p request_state: %d chunk_size: %d num objs: %d invalid\n",
                               memory_request_p, memory_request_p->request_state, chunk_size, memory_request_p->number_of_objects);
        return FBE_STATUS_GENERIC_FAILURE;
    }
#if RAID_DBG_MEMORY_ENABLED
    /*! Now invoke the memory service method.
     *
     *  @note Currently the memory service returns a status of other than:
     *        `FBE_STATUS_OK' for requests that aren't immediately satisfied.
     *        Therefore we can only check for `FBE_STATUS_GENERIC_FAILURE'
     */
    if (fbe_raid_memory_mock_is_mock_enabled() == FBE_TRUE)
    {
        status = (fbe_raid_memory_mock_api_allocate)(memory_request_p);
    }
    else
#endif
    {
        status = fbe_memory_request_entry(memory_request_p);
        if (RAID_DBG_MEMORY_ENABLED && b_memory_tracking_enabled == FBE_TRUE)
        {
            fbe_raid_memory_track_allocation(memory_request_p);
        }
    }
    if (RAID_MEMORY_COND((status != FBE_STATUS_OK)      &&
                         (status != FBE_STATUS_PENDING)    ))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: memory request: 0x%p priority: %d failed with status: 0x%x\n",
                               memory_request_p, memory_request_p->priority, status);
        return status;
    }

    /* Now determine if the request completed immediately or not.  If it
     * completed immediately return `Ok', else `pending'.
     */
    if (fbe_memory_request_is_immediate(memory_request_p))
    {
        return FBE_STATUS_OK;
    }
    
    /* Else return the fact that the allocation wasn't immediate
     */
    return FBE_STATUS_PENDING;
}
/**************************************
 * end fbe_raid_memory_api_allocate()
 **************************************/
/*!**************************************************************
 * fbe_raid_memory_init_headers()
 ****************************************************************
 * @brief
 *  
 *
 * @param  -               
 *
 * @return -   
 *
 * @author
 *  11/7/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_memory_init_headers(fbe_memory_header_t *memory_header_p, 
                                          fbe_u32_t pages_allocated, 
                                          fbe_raid_memory_type_t page_type)
{
    fbe_memory_header_t        *next_memory_header_p = NULL;
    fbe_u32_t                   index;
    fbe_raid_memory_header_t   *header_p = NULL;

    for (index = 0; index < pages_allocated; index++)
    {
        /* The memory header should be valid
         */
        if (RAID_MEMORY_COND(memory_header_p) == NULL)
        {
            /* This is unexpected
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: memory hdr: 0x%p failed to get memory header index: %d out of: %d pages \n",
                                   memory_header_p, index, pages_allocated); 
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* We expect the proper number of pages.  If the data is NULL it is an 
         * error
         */
        header_p = (fbe_raid_memory_header_t *)fbe_memory_header_to_data(memory_header_p);
        if (RAID_MEMORY_COND(header_p == NULL) )
        {
            /* This is unexpected
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: memory hdr: 0x%p failed to get page index: %d out of: %d pages \n",
                                   memory_header_p, index, pages_allocated); 
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*! @note We no longer zero the entire buffer
         */
        fbe_raid_memory_header_init(header_p, page_type);

        /*! @note The links are now owned by the memory service.  Therefore
         *        there is no reason to set the links for pages.
         */

        /* Get the next header
         */
        fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
        memory_header_p = next_memory_header_p;

    } /* for all allocated pages */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_memory_init_headers()
 ******************************************/
/*!***************************************************************************
 *          fbe_raid_memory_api_allocate_init_pages()
 *****************************************************************************
 * @brief   This function initialize the pages allocated by the memory service
 *          so that they can be consumed by raid.  This includes initialize the
 *          header (we do NOT zero the buffer space!!).
 *
 * @param   memory_request_p - Pointer to memory service request that determines
 *                  how many and the page size
 *
 * @param   FBE_STATUS_OK - if the resources were allocated,
 *          FBE_STATUS_INSUFFICIENT_RESOURCES - if we were unable to allocate.
 *
 * @author
 *  10/18/2010  Ron Proulx  - Created  
 *
 **************************************************************************/

static fbe_status_t fbe_raid_memory_api_allocate_init_pages(fbe_memory_request_t *memory_request_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_memory_header_t        *memory_header_p = NULL;
    fbe_raid_memory_type_t      page_type = FBE_RAID_MEMORY_TYPE_INVALID;
    fbe_memory_chunk_size_t     chunk_size;
    fbe_memory_number_of_objects_dc_t number_of_objects;
    number_of_objects.number_of_objects = memory_request_p->number_of_objects;
    
    /* Walk all the pages and initialize them so that raid can consume them
     */
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    fbe_memory_dps_chunk_size_to_ctrl_bytes(memory_request_p->chunk_size, &chunk_size);
    page_type = fbe_raid_memory_dps_size_to_raid_type(chunk_size);
    status = fbe_raid_memory_init_headers(memory_header_p, number_of_objects.split.control_objects, page_type);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: init ctrl headers failed status: 0x%x\n", status);
        return status;
    }

    if (number_of_objects.split.data_objects > 0)
    {
        memory_header_p = fbe_memory_get_first_data_memory_header(memory_request_p);
        fbe_memory_dps_chunk_size_to_data_bytes(memory_request_p->chunk_size, &chunk_size);
        page_type = fbe_raid_memory_dps_size_to_raid_type(chunk_size);
        status = fbe_raid_memory_init_headers(memory_header_p, number_of_objects.split.data_objects, page_type);
    
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: init data headers failed status: 0x%x\n", status);
            return status;
        }
    }

    /* We allocated everything.
     */
    return FBE_STATUS_OK;
}
/***********************************************
 * end fbe_raid_memory_api_allocate_init_pages()
 ***********************************************/

/*!***************************************************************************
 * fbe_raid_memory_api_allocation_complete()
 *****************************************************************************
 * @brief   This is memory api to invoke when a memory allocation has completed
 *          successfully.
 *
 * @param   memory_request_p - Pointer to memory service request for allocation
 * @param   b_track_deferred_allocation - FBE_TRUE track this allocation
 *
 * @return  FBE_STATUS_OK - Memory was allocated successfully.
 * 
 * @author
 *  08/05/2009  Rob Foley - Created   
 *
 **************************************************************************/

fbe_status_t fbe_raid_memory_api_allocation_complete(fbe_memory_request_t *memory_request_p,
                                                     fbe_bool_t b_track_deferred_allocation)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_atomic_t    bytes_allocated = 0;
    fbe_u32_t       num_pages = 0;
    fbe_memory_number_of_objects_dc_t number_of_objects;
    fbe_memory_chunk_size_t chunk_size_bytes;

    fbe_memory_dps_chunk_size_to_ctrl_bytes(memory_request_p->chunk_size, &chunk_size_bytes);

    number_of_objects.number_of_objects = memory_request_p->number_of_objects;

    /* If memory tracking is enabled track this deferred allocation
     */
    if (b_track_deferred_allocation == FBE_TRUE)
    {
        fbe_raid_memory_track_deferred_allocation(memory_request_p);
    }

    /* Increment the memory statistics
     */
    bytes_allocated = (((fbe_s64_t)chunk_size_bytes) * memory_request_p->number_of_objects);
    fbe_atomic_increment(&fbe_raid_memory_statistics.allocations);
    fbe_atomic_add(&fbe_raid_memory_statistics.allocated_bytes, bytes_allocated);

    /* Validate that the correct number of objects were allocated
     */
    num_pages = fbe_memory_get_queue_length(memory_request_p);
    if (RAID_MEMORY_COND(num_pages != number_of_objects.split.control_objects))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: memory request: 0x%p pages expected: %d actual: %d\n",
                               memory_request_p, number_of_objects.split.control_objects, num_pages);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* Validate that the correct number of objects were allocated
     */
    if (number_of_objects.split.data_objects > 0)
    {
        num_pages = fbe_memory_get_data_queue_length(memory_request_p);
        if (RAID_MEMORY_COND(num_pages != number_of_objects.split.data_objects))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: memory request: 0x%p pages expected: %d actual: %d\n",
                                   memory_request_p, number_of_objects.split.data_objects, num_pages);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (RAID_DBG_MEMORY_ENABLED)
    {
        /* Now initialize all the pages allocated */
        status = fbe_raid_memory_api_allocate_init_pages(memory_request_p);
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: memory request: 0x%p failed to init pages with status: 0x%x \n",
                                   memory_request_p, status); 
        }
    }

    /* Return the execution status
     */
    return status;
}
/************************************************
 * end fbe_raid_memory_api_allocation_complete()
 ************************************************/

/*!***************************************************************************
 * fbe_raid_memory_api_deferred_allocation()
 *****************************************************************************
 * @brief   Simply increment the number of deferred allocations
 *
 * @param   None
 *
 * @return  None
 * 
 **************************************************************************/
void fbe_raid_memory_api_deferred_allocation(void)
{
    /* Increment deferred allocations and decrement pending allocations
     */
    fbe_atomic_increment(&fbe_raid_memory_statistics.deferred_allocations);
    fbe_atomic_decrement(&fbe_raid_memory_statistics.pending_allocations);

    return;
}
/************************************************
 * end fbe_raid_memory_api_deferred_allocation()
 ************************************************/

/*!***************************************************************************
 * fbe_raid_memory_api_aborted_allocation()
 *****************************************************************************
 * @brief   Simply increment the number of aborted allocations
 *
 * @param   None
 *
 * @return  None
 * 
 **************************************************************************/
void fbe_raid_memory_api_aborted_allocation(void)
{
    /* Increment aborted allocations
     */
    fbe_atomic_increment(&fbe_raid_memory_statistics.aborted_allocations);

    return;
}
/************************************************
 * end fbe_raid_memory_api_aborted_allocation()
 ************************************************/

/*!***************************************************************************
 * fbe_raid_memory_api_pending_allocation()
 *****************************************************************************
 * @brief   Simply increment the number of pending allocations
 *
 * @param   None
 *
 * @return  None
 * 
 **************************************************************************/
void fbe_raid_memory_api_pending_allocation(void)
{
    /* Simply increment the memory statistics
     */
    fbe_atomic_increment(&fbe_raid_memory_statistics.pending_allocations);

    return;
}
/************************************************
 * end fbe_raid_memory_api_pending_allocation()
 ************************************************/

/*!***************************************************************************
 * fbe_raid_memory_api_allocation_error()
 *****************************************************************************
 * @brief   Simply increment the number of allocation errors
 *
 * @param   None
 *
 * @return  None
 * 
 **************************************************************************/
void fbe_raid_memory_api_allocation_error(void)
{
    /* Simply increment the memory statistics
     */
    fbe_atomic_increment(&fbe_raid_memory_statistics.allocation_errors);

    return;
}
/************************************************
 * end fbe_raid_memory_api_allocation_error()
 ************************************************/

/*!***************************************************************************
 *          fbe_raid_memory_validate_page_queue()
 *****************************************************************************
 * @brief   This function validates that raid did corrupt any of the page
 *          barriers before we free the memory.
 *
 * @param memory_header_p
 * @param pages_allocated
 * @param page_type
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/7/2012 - Created. Rob Foley
 *
 **************************************************************************/
static fbe_status_t fbe_raid_memory_validate_page_queue(fbe_memory_header_t *memory_header_p,
                                                        fbe_u32_t pages_allocated,
                                                        fbe_raid_memory_type_t page_type)
{
    fbe_memory_header_t        *next_memory_header_p = NULL;
    fbe_u32_t                   index;
    fbe_raid_memory_header_t   *header_p = NULL;
    fbe_bool_t                  b_header_valid = FBE_TRUE;

    /* Walk all the pages and validate that we haven't corrupted any
     */
    for (index = 0; index < pages_allocated; index++)
    {
        /* The memory header should be valid
         */
        if (RAID_MEMORY_COND(memory_header_p) == NULL)
        {
            /* This is unexpected
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: memory hdr: 0x%p failed to get memory header index: %d out of: %d pages \n",
                                   memory_header_p, index, pages_allocated); 
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* We expect the proper number of pages.  If the data is NULL it is an 
         * error
         */
        header_p = (fbe_raid_memory_header_t *)fbe_memory_header_to_data(memory_header_p);
        if (RAID_MEMORY_COND(header_p == NULL) )
        {
            /* This is unexpected
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: memory hdr: 0x%p failed to get page index: %d out of: %d pages \n",
                                   memory_header_p, index, pages_allocated); 
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Validate that we haven't corrupted any barriers
         */
        b_header_valid = fbe_raid_memory_header_is_valid(header_p);
        if (RAID_MEMORY_COND(b_header_valid != FBE_TRUE))
        {
            /* We have corrupted the buffer that we were given
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: memory hdr: 0x%p header_p: 0x%p page index: %d corrupted headers\n",
                                   memory_header_p, header_p, index); 
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get the next header
         */
        fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
        memory_header_p = next_memory_header_p;

    } /* for all allocated pages */

    /* We validated everything.
     */
    return FBE_STATUS_OK;
}
/***********************************************
 * end fbe_raid_memory_validate_page_queue()
 ***********************************************/

/*!***************************************************************************
 *          fbe_raid_memory_api_free_validate_pages()
 *****************************************************************************
 * @brief   This function validates that raid did corrupt any of the page
 *          barriers before we free the memory.
 *
 * @param   memory_request_p - Pointer to memory service request to validate
 *                             pages for
 *
 * @param   FBE_STATUS_OK - pages were successfully validate
 *          other - We have corrupted the memory we were given
 *
 * @author
 *  10/18/2010  Ron Proulx  - Created 
 *
 **************************************************************************/
static fbe_status_t fbe_raid_memory_api_free_validate_pages(fbe_memory_request_t *memory_request_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_memory_header_t        *memory_header_p = NULL;
    fbe_raid_memory_type_t      page_type = FBE_RAID_MEMORY_TYPE_INVALID;
    fbe_memory_chunk_size_t     chunk_size;
    fbe_memory_number_of_objects_dc_t number_of_objects;
    number_of_objects.number_of_objects = memory_request_p->number_of_objects;

    /* Walk all the pages and validate that we haven't corrupted any
     */
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    fbe_memory_dps_chunk_size_to_ctrl_bytes(memory_request_p->chunk_size, &chunk_size);
    page_type = fbe_raid_memory_dps_size_to_raid_type(chunk_size);
    status = fbe_raid_memory_validate_page_queue(memory_header_p, number_of_objects.split.control_objects, page_type);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: validate ctrl headers failed status: 0x%x\n", status);
        return status;
    }

    memory_header_p = fbe_memory_get_first_data_memory_header(memory_request_p);
    fbe_memory_dps_chunk_size_to_data_bytes(memory_request_p->chunk_size, &chunk_size);
    page_type = fbe_raid_memory_dps_size_to_raid_type(chunk_size);
    status = fbe_raid_memory_validate_page_queue(memory_header_p, number_of_objects.split.data_objects, page_type);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: validate data headers failed status: 0x%x\n", status);
        return status;
    }

    /* We validated everything.
     */
    return FBE_STATUS_OK;
}
/***********************************************
 * end fbe_raid_memory_api_free_validate_pages()
 ***********************************************/

/*!***************************************************************************
 * fbe_raid_memory_api_free()
 *****************************************************************************
 * @brief   This is the main entry point for freeing memory.  The parameter is
 *          is a memory service request pointer.
 *
 * @param   memory_request_p - Pointer to memory service request ot free 
 *                             resources for
 * @param   b_memory_tracking_enabled - FBE_TRUE - Enabled memory tracking
 *                             FBE_FALSE - Memory tracking disabled
 *
 * @param   FBE_STATUS_GENERIC_FAILURE - Invalid parameter or error freeing.
 *          FBE_STATUS_OK - Memory was allocated successfully.
 * 
 * @author
 *  08/05/2009  Rob Foley - Created   
 *
 **************************************************************************/
fbe_status_t fbe_raid_memory_api_free(fbe_memory_request_t *memory_request_p,
                                      fbe_bool_t b_memory_tracking_enabled)
{
    if (!RAID_DBG_MEMORY_ENABLED)
    {
        //fbe_memory_ptr_t        memory_ptr = NULL;
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(memory_request_p);
		return FBE_STATUS_OK;
    }
    else
    {
        fbe_status_t            status = FBE_STATUS_OK;
        fbe_status_t            free_status = FBE_STATUS_OK;
        fbe_atomic_t            bytes_allocated = 0;
        fbe_memory_chunk_size_t chunk_size = 0;

        fbe_memory_dps_chunk_size_to_ctrl_bytes(memory_request_p->chunk_size, &chunk_size);
    
        /* Validate that the allocation completed 
         */
        bytes_allocated = ((fbe_s64_t)chunk_size * memory_request_p->number_of_objects);
        if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
        {
            /* The request may have been aborted.  If so just return success
             */
            if (fbe_memory_request_is_aborted(memory_request_p) == FBE_TRUE)
            {
                /* Aborted request are infrequent so log an informational message
                 */
               fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "raid: free aborted request: 0x%p state: %d cnksz: %d objs: %d pri: %d\n",
                                      memory_request_p, memory_request_p->request_state, chunk_size, 
                                      memory_request_p->number_of_objects, memory_request_p->priority);
               return FBE_STATUS_OK;
            }
    
            /* Mark the free request in error but continue so that we don't
             * purposefully leak memory.
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: free request: 0x%p state: %d cnksz: %d objs: %d pri: %d unexpected state\n",
                                   memory_request_p, memory_request_p->request_state, chunk_size, 
                                   memory_request_p->number_of_objects, memory_request_p->priority);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        /* Validate the chunk size and number of objects
         */
        if (((chunk_size != FBE_MEMORY_CHUNK_SIZE_FOR_PACKET)       &&             
             (chunk_size != FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO)    )  ||
            (memory_request_p->number_of_objects < 1)                        )
        {
            /* Mark the free request in error but continue so that we don't
             * purposefully leak memory.
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: free request: 0x%p state: %d cnksz: %d objs: %d pri: %d invalid\n",
                                   memory_request_p, memory_request_p->request_state, chunk_size, 
                                   memory_request_p->number_of_objects, memory_request_p->priority);
            free_status = FBE_STATUS_GENERIC_FAILURE;
        }
        /* Walk the page list and validate that we haven't corrupted any of 
         * the barriers.
         */
        status = fbe_raid_memory_api_free_validate_pages(memory_request_p);
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            /*! In this case we have corrupted the memory we were given.
             *  Therefore we do not want to return these pages to the pool
             *  or we want to somehow tell the memory service to validate
             *  all the pages in the pool
             * 
             *  @notes We do not have code to handle this gracefully so we panic.
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                                   "raid: raid has corrupted pages for memory request: 0x%p not freeing memory. status: 0x%x\n",
                                   memory_request_p, status);
            return status;
        }
        status = free_status;
    
        /* Now free the memory
         */
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        if (fbe_raid_memory_mock_is_mock_enabled() == FBE_TRUE)
        {
            /* free_status = (fbe_raid_memory_mock_api_free)(memory_ptr); @note We could enable it. */
        }
        else
        {
            if (b_memory_tracking_enabled)
            {
                /* fbe_raid_memory_track_free((fbe_memory_header_t *const)memory_ptr); @note we could enable it  */
            }
            //free_status = fbe_memory_free_entry(memory_ptr);
			fbe_memory_free_request_entry(memory_request_p);
    
        }
        if (RAID_MEMORY_COND(free_status != FBE_STATUS_OK))
        {
            /* We have just leaked memory.  Report the failure
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                                   "raid: free memory for memory_request_p: 0x%p failed with status: 0x%x \n",
                                   memory_request_p, free_status);
            return free_status;
        }
    
        /*! @note We only increment the `frees' and associated bytes if the status was
         *        success.  This way we can see that a memory leak occurred by the 
         *        difference in bytes allocated and freed.
         */
        fbe_atomic_increment(&fbe_raid_memory_statistics.frees);
        fbe_atomic_add(&fbe_raid_memory_statistics.freed_bytes, bytes_allocated);
    
        /* Done freeing. Return the request status
         */
        return status;
    }
}
/**************************************
 * end fbe_raid_memory_api_free()
 **************************************/

/*!***************************************************************************
 * fbe_raid_memory_api_free_error()
 *****************************************************************************
 * @brief   Simply increment the number of free errors
 *
 * @param   None
 *
 * @return  None
 * 
 **************************************************************************/
void fbe_raid_memory_api_free_error(void)
{
    /* Simply increment the memory statistics
     */
    fbe_atomic_increment(&fbe_raid_memory_statistics.free_errors);

    return;
}
/************************************************
 * end fbe_raid_memory_api_free_error()
 ************************************************/

/*!*******************************************************************
 * fbe_raid_memory_get_index_by_sg_type()
 *********************************************************************
 * @brief Determine the type of sg from the index passed in.
 * 
 * @param type - The memory type of this SG.
 * 
 * @return fbe_u32_t - Index of sg type.
 *
 *********************************************************************/
fbe_raid_sg_index_t fbe_raid_memory_get_index_by_sg_type(fbe_raid_memory_type_t type)
{
    fbe_u32_t sg_index;

    switch (type)
    {
        case FBE_RAID_MEMORY_TYPE_SG_1:
            sg_index = FBE_RAID_SG_INDEX_SG_1;
            break;
        case FBE_RAID_MEMORY_TYPE_SG_8:
            sg_index = FBE_RAID_SG_INDEX_SG_8;
            break;
        case FBE_RAID_MEMORY_TYPE_SG_32:
            sg_index = FBE_RAID_SG_INDEX_SG_32;
            break;
        case FBE_RAID_MEMORY_TYPE_SG_128:
            sg_index = FBE_RAID_SG_INDEX_SG_128;
            break;
        case FBE_RAID_MEMORY_TYPE_SG_MAX:
            sg_index = FBE_RAID_SG_INDEX_SG_MAX;
            break;
        default:
            sg_index = FBE_RAID_SG_INDEX_INVALID;
            break;

    };
    return sg_index;
}
/**************************************
 * end fbe_raid_memory_get_index_by_sg_type()
 **************************************/

/*!*******************************************************************
 * fbe_raid_memory_get_sg_type_by_index()
 *********************************************************************
 * @brief Determine the type of sg from the index passed in.
 * 
 * @param sg_index - Index of sg type.
 * 
 * @return fbe_raid_memory_type_t - The memory type of this SG.
 *
 *********************************************************************/
fbe_raid_memory_type_t fbe_raid_memory_get_sg_type_by_index(fbe_raid_sg_index_t sg_index)
{
    fbe_raid_memory_type_t memory_type;

    switch (sg_index)
    {
        case FBE_RAID_SG_INDEX_SG_1:
            memory_type = FBE_RAID_MEMORY_TYPE_SG_1;
            break;
        case FBE_RAID_SG_INDEX_SG_8:
            memory_type = FBE_RAID_MEMORY_TYPE_SG_8;
            break;
        case FBE_RAID_SG_INDEX_SG_32:
            memory_type = FBE_RAID_MEMORY_TYPE_SG_32;
            break;
        case FBE_RAID_SG_INDEX_SG_128:
            memory_type = FBE_RAID_MEMORY_TYPE_SG_128;
            break;
        case FBE_RAID_SG_INDEX_SG_MAX:
            memory_type = FBE_RAID_MEMORY_TYPE_SG_MAX;
            break;
        default:
            memory_type = FBE_RAID_MEMORY_TYPE_INVALID;
            break;

    };
    return memory_type;
}
/**************************************
 * end fbe_raid_memory_get_sg_type_by_index()
 **************************************/
/*!**************************************************************
 * fbe_raid_memory_sg_count_index()
 ****************************************************************
 * @brief
 *  Determine the index of the sg allocation from the size of
 *  the sg list passed in.
 *
 * @param sg_size - size of the sg list that is needed.
 *                  This size does not include the null terminator.
 *
 * @return fbe_u32_t index of the sg allocation/pool to use.
 *         FBE_RAID_SG_INDEX_INVALID on error.
 *
 * @author
 *  8/4/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_sg_index_t fbe_raid_memory_sg_count_index(fbe_u32_t sg_size)
{
    fbe_u32_t index;

    if (sg_size <= FBE_RAID_SG_1_DATA_ENTRIES) 
    {
        index = FBE_RAID_SG_INDEX_SG_1;
    }
    else if (sg_size <= FBE_RAID_SG_8_DATA_ENTRIES)
    {
        index = FBE_RAID_SG_INDEX_SG_8;
    }
    else if (sg_size <= FBE_RAID_SG_32_DATA_ENTRIES)
    {
        index = FBE_RAID_SG_INDEX_SG_32;
    }
    else if (sg_size <= FBE_RAID_SG_128_DATA_ENTRIES)
    {
        index = FBE_RAID_SG_INDEX_SG_128;
    }
    else if (sg_size <= FBE_RAID_SG_MAX_DATA_ENTRIES)
    {
        index = FBE_RAID_SG_INDEX_SG_MAX;
    }
    else 
    {
        index = FBE_RAID_SG_INDEX_INVALID;
    }
    return index;
}
/******************************************
 * end fbe_raid_memory_sg_count_index()
 ******************************************/

/*!**************************************************************
 * fbe_raid_memory_sg_index_to_max_count()
 ****************************************************************
 * @brief
 *  Determine the index of the sg allocation from the size of
 *  the sg list passed in.
 *
 * @param sg_index - Index of the sg list type.
 *
 * @return fbe_u32_t Max number of sgs. 0 on error.
 *
 * @author
 *  8/4/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_raid_memory_sg_index_to_max_count(fbe_raid_sg_index_t sg_index)
{
    fbe_u32_t num_sgs;

    if (sg_index == FBE_RAID_SG_INDEX_SG_1) 
    {
        num_sgs = FBE_RAID_SG_1_DATA_ENTRIES;
    }
    else if (sg_index == FBE_RAID_SG_INDEX_SG_8)
    {
        num_sgs = FBE_RAID_SG_8_DATA_ENTRIES;
    }
    else if (sg_index == FBE_RAID_SG_INDEX_SG_32)
    {
        num_sgs = FBE_RAID_SG_32_DATA_ENTRIES;
    }
    else if (sg_index == FBE_RAID_SG_INDEX_SG_128)
    {
        num_sgs = FBE_RAID_SG_128_DATA_ENTRIES;
    }
    else if (sg_index == FBE_RAID_SG_INDEX_SG_MAX)
    {
        num_sgs = FBE_RAID_SG_MAX_DATA_ENTRIES;
    }
    else
    {
        /* Error, just return 0.
         */
        num_sgs = 0;
    }
    return num_sgs;
}
/******************************************
 * end fbe_raid_memory_sg_index_to_max_count()
 ******************************************/

/*!***************************************************************************
 *          fbe_raid_memory_page_element_to_buffer_pool_entry()
 *****************************************************************************
 *
 * @brief   Cast the input page element as the proper memory service structure.
 *          Then get the pointer to the data which is used by raid memory.
 * 
 * @param   page_element_p - Pointer to page element to get pool entry for
 * @param   pool_entry_pp - Address of pointer to update with pool entry   
 * 
 * @return  status  - Typically FBE_STATUS_OK
 *
 * @author
 *  10/18/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_memory_page_element_to_buffer_pool_entry(fbe_raid_memory_element_t *page_element_p,
                                                                        fbe_raid_buffer_pool_entry_t **pool_entry_pp)
{
    fbe_bool_t      b_header_valid = FBE_FALSE;

    /* Validate the input parameters
     */
    if (RAID_COND((page_element_p == NULL) ||
                  (pool_entry_pp  == NULL)   ))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: Either page element: 0x%p or pool entry: 0x%p is NULL \n",
                               page_element_p, pool_entry_pp);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now translate from the memory element back to he memory header
     */
    *pool_entry_pp = (fbe_raid_buffer_pool_entry_t *)fbe_memory_header_element_to_data(page_element_p);

    if (RAID_DBG_MEMORY_ENABLED &&
        ((*pool_entry_pp == NULL) ||
         ((b_header_valid = fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t *)(*pool_entry_pp))) 
                                                                                                == FBE_FALSE)   ))
    {
        /* Log an error and fail the request
         */
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: Either page entry: 0x%p is NULL or header valid: %d is FBE_FALSE\n",
                               *pool_entry_pp, b_header_valid); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Else success
     */
    return FBE_STATUS_OK;
}
/*****************************************************
 * fbe_raid_memory_page_element_to_buffer_pool_entry()
 *****************************************************/

/*!*******************************************************************
 * fbe_raid_memory_type_get_size()
 *********************************************************************
 * @brief Determine the size of the memory structure associated with this type.
 * 
 * @param type - Type of memory structure to get the size for.
 * 
 * @return fbe_u32_t - The size in bytes of the structure
 *                     associated with this type.
 *
 *********************************************************************/
fbe_u32_t fbe_raid_memory_type_get_size(fbe_raid_memory_type_t type)
{
    fbe_u32_t size = 0;
    switch (type)
    {
        case FBE_RAID_MEMORY_TYPE_SIOTS:
            size = sizeof(fbe_raid_siots_pool_entry_t);
            break; 
        case FBE_RAID_MEMORY_TYPE_VCTS:
            size = sizeof(fbe_raid_vcts_pool_entry_t);
            break; 
        case FBE_RAID_MEMORY_TYPE_FRUTS:
            size = sizeof(fbe_raid_fruts_pool_entry_t);
            break; 
        case FBE_RAID_MEMORY_TYPE_SG_1:
            size = sizeof(fbe_raid_sg_1_pool_entry_t);
            break;
        case FBE_RAID_MEMORY_TYPE_SG_8:
            size = sizeof(fbe_raid_sg_8_pool_entry_t);
            break;
        case FBE_RAID_MEMORY_TYPE_SG_32:
            size = sizeof(fbe_raid_sg_32_pool_entry_t);
            break;
        case FBE_RAID_MEMORY_TYPE_SG_128:
            size = sizeof(fbe_raid_sg_128_pool_entry_t);
            break;
        case FBE_RAID_MEMORY_TYPE_SG_MAX:
            size = sizeof(fbe_raid_sg_max_pool_entry_t);
            break;
        case FBE_RAID_MEMORY_TYPE_BUFFERS_WITH_SG:
            size = sizeof(fbe_raid_buffer_pool_entry_t);
            break;
        case FBE_RAID_MEMORY_TYPE_2KB_BUFFER:
            size = sizeof(fbe_raid_buffer_2kb_pool_entry_t);
            break;
        case FBE_RAID_MEMORY_TYPE_32KB_BUFFER:
            size = sizeof(fbe_raid_buffer_32kb_pool_entry_t);
            break;
        default:
            break;
    };
    return size;
}
/**************************************
 * end fbe_raid_memory_type_get_size()
 **************************************/

/*!*******************************************************************
 * fbe_raid_memory_size_to_dps_size()
 *********************************************************************
 * @brief Determine the size of the memory structure associated with this type.
 * 
 * @param page_size - Size of raid memory to get dps size for.
 * 
 * @return fbe_memory_chunk_size_t - Size in bytes of dps page.
 *
 *********************************************************************/
fbe_memory_chunk_size_t fbe_raid_memory_size_to_dps_size(fbe_u32_t page_size)
{
    fbe_u32_t size = 0;
    if (page_size <= FBE_MEMORY_CHUNK_SIZE_FOR_PACKET)
    {
        size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    }
    else if (page_size <= FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO)
    {
        size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    }
    else
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                               "%s page size %d unexpected \n",
                               __FUNCTION__, page_size);
    }
    return size;
}
/**************************************
 * end fbe_raid_memory_size_to_dps_size()
 **************************************/
/*!*******************************************************************
 * fbe_raid_memory_dps_size_to_raid_type()
 *********************************************************************
 * @brief Determine raid type for the given dps page size.
 * 
 * @param page_size - dps page size to get raid memory type for.
 * 
 * @return fbe_raid_memory_type_t - Memory type raid would use.
 *
 *********************************************************************/
fbe_raid_memory_type_t fbe_raid_memory_dps_size_to_raid_type(fbe_memory_chunk_size_t page_size)
{
    fbe_raid_memory_type_t type = 0;

    if (page_size >= fbe_raid_memory_type_get_size(FBE_RAID_MEMORY_TYPE_32KB_BUFFER))
    {
        type = FBE_RAID_MEMORY_TYPE_32KB_BUFFER;
    }
    else if (page_size >= fbe_raid_memory_type_get_size(FBE_RAID_MEMORY_TYPE_2KB_BUFFER))
    {
        type = FBE_RAID_MEMORY_TYPE_2KB_BUFFER;
    }
    else
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                               "%s page size %d unexpected \n",
                               __FUNCTION__, page_size);
    }
    return type;
}
/**************************************
 * end fbe_raid_memory_dps_size_to_raid_type()
 **************************************/

/*!*******************************************************************
 * fbe_raid_memory_header_init_footer()
 *********************************************************************
 * @brief Set the footer at the end of this structure.
 * 
 * @param header_p - Ptr to header of this struct.
 * 
 * @notes We assume that the type has already been setup in the header.
 * 
 * @return None.
 *
 *********************************************************************/
void fbe_raid_memory_header_init_footer(fbe_raid_memory_header_t *header_p)
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_type_t type = header_p->type;

    switch (type)
    {
        case FBE_RAID_MEMORY_TYPE_SIOTS:
            {
                fbe_raid_siots_pool_entry_t *siots_p = (fbe_raid_siots_pool_entry_t*)header_p;
                siots_p->magic = FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER;
            }
            break; 
        case FBE_RAID_MEMORY_TYPE_VCTS:
            {
                fbe_raid_vcts_pool_entry_t *vcts_p = (fbe_raid_vcts_pool_entry_t*)header_p;
                vcts_p->magic = FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER;
            }
            break; 
        case FBE_RAID_MEMORY_TYPE_FRUTS:
            {
                fbe_raid_fruts_pool_entry_t *fruts_p = (fbe_raid_fruts_pool_entry_t*)header_p;
                fruts_p->magic = FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER;
            }
            break; 
        case FBE_RAID_MEMORY_TYPE_BUFFERS_WITH_SG:
            {
                fbe_raid_buffer_pool_entry_t *buffer_p = (fbe_raid_buffer_pool_entry_t*)header_p;
                buffer_p->magic = FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER;
            }
            break; 
        case FBE_RAID_MEMORY_TYPE_2KB_BUFFER:
            {
                fbe_raid_buffer_2kb_pool_entry_t *buffer_p = (fbe_raid_buffer_2kb_pool_entry_t*)header_p;
                buffer_p->magic = FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER;
            }
            break; 
        case FBE_RAID_MEMORY_TYPE_32KB_BUFFER:
            {
                fbe_raid_buffer_32kb_pool_entry_t *buffer_p = (fbe_raid_buffer_32kb_pool_entry_t*)header_p;
                buffer_p->magic = FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER;
            }
            break; 
        case FBE_RAID_MEMORY_TYPE_SG_1:
            {
                fbe_raid_sg_1_pool_entry_t *sg_p = (fbe_raid_sg_1_pool_entry_t*)header_p;
                sg_p->magic = FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER;
                sg_p->sg[FBE_RAID_SG_1_ENTRIES - 1].count = 0;
                sg_p->sg[FBE_RAID_SG_1_ENTRIES - 1].address = NULL;
            }
            break;
        case FBE_RAID_MEMORY_TYPE_SG_8:
            {
                fbe_raid_sg_8_pool_entry_t *sg_p = (fbe_raid_sg_8_pool_entry_t*)header_p;
                sg_p->magic = FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER;
                sg_p->sg[FBE_RAID_SG_8_ENTRIES - 1].count = 0;
                sg_p->sg[FBE_RAID_SG_8_ENTRIES - 1].address = NULL;
            }
            break;
        case FBE_RAID_MEMORY_TYPE_SG_32:
            {
                fbe_raid_sg_32_pool_entry_t *sg_p = (fbe_raid_sg_32_pool_entry_t*)header_p;
                sg_p->magic = FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER;
                sg_p->sg[FBE_RAID_SG_32_ENTRIES - 1].count = 0;
                sg_p->sg[FBE_RAID_SG_32_ENTRIES - 1].address = NULL;
            }
            break;
        case FBE_RAID_MEMORY_TYPE_SG_128:
            {
                fbe_raid_sg_128_pool_entry_t *sg_p = (fbe_raid_sg_128_pool_entry_t*)header_p;
                sg_p->magic = FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER;
                sg_p->sg[FBE_RAID_SG_128_ENTRIES - 1].count = 0;
                sg_p->sg[FBE_RAID_SG_128_ENTRIES - 1].address = NULL;
            }
            break;
        case FBE_RAID_MEMORY_TYPE_SG_MAX:
            {
                fbe_raid_sg_max_pool_entry_t *sg_p = (fbe_raid_sg_max_pool_entry_t*)header_p;
                sg_p->magic = FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER;
                sg_p->sg[FBE_RAID_SG_MAX_ENTRIES - 1].count = 0;
                sg_p->sg[FBE_RAID_SG_MAX_ENTRIES - 1].address = NULL;
            }
            break;
        default:
            /* The header is not valid.
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                                   "%s This is not a valid header type : 0x%x. \n",
                                   __FUNCTION__, type);
            break;
    };
#endif // RAID_DBG_MEMORY_ENABLED
    return;
}
/**************************************
 * end fbe_raid_memory_header_init_footer()
 **************************************/
/*!*******************************************************************
 * fbe_raid_memory_header_validate_footer()
 *********************************************************************
 * @brief Make sure the footer is still valid for each structure.
 * 
 * @param header_p - Ptr to header of this struct.
 * 
 * @notes We assume that the type has already been setup in the header.
 * 
 * @return fbe_bool_t FBE_TRUE if it was valid.
 *                    FBE_FALSE otherwise.
 *
 *********************************************************************/
fbe_bool_t fbe_raid_memory_header_validate_footer(fbe_raid_memory_header_t *header_p)
{ 
    fbe_bool_t b_status = FBE_TRUE;
#if RAID_DBG_MEMORY_ENABLED  
    fbe_raid_memory_type_t type = header_p->type;

    /* Check that the memory footer is valid.
     */
    switch (type)
    {
        case FBE_RAID_MEMORY_TYPE_SIOTS:
            {
                fbe_raid_siots_pool_entry_t *siots_p = (fbe_raid_siots_pool_entry_t*)header_p;
                b_status = (siots_p->magic == FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER);
            }
            break; 
        case FBE_RAID_MEMORY_TYPE_VCTS:
            {
                fbe_raid_vcts_pool_entry_t *vcts_p = (fbe_raid_vcts_pool_entry_t*)header_p;
                b_status = (vcts_p->magic == FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER);
            }
            break; 
        case FBE_RAID_MEMORY_TYPE_FRUTS:
            {
                fbe_raid_fruts_pool_entry_t *fruts_p = (fbe_raid_fruts_pool_entry_t*)header_p;
                b_status = (fruts_p->magic == FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER);
            }
            break; 
        case FBE_RAID_MEMORY_TYPE_BUFFERS_WITH_SG:
            {
                fbe_raid_buffer_pool_entry_t *buffer_p = (fbe_raid_buffer_pool_entry_t*)header_p;
                b_status = (buffer_p->magic == FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER);
            }
            break;
        case FBE_RAID_MEMORY_TYPE_2KB_BUFFER:
            {
                fbe_raid_buffer_2kb_pool_entry_t *buffer_p = (fbe_raid_buffer_2kb_pool_entry_t*)header_p;
                b_status = (buffer_p->magic == FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER);
            }
            break;
        case FBE_RAID_MEMORY_TYPE_32KB_BUFFER: 
            {
                fbe_raid_buffer_32kb_pool_entry_t *buffer_p = (fbe_raid_buffer_32kb_pool_entry_t*)header_p;
                b_status = (buffer_p->magic == FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER);
            }
            break; 
        case FBE_RAID_MEMORY_TYPE_SG_1:
            {
                fbe_raid_sg_1_pool_entry_t *sg_p = (fbe_raid_sg_1_pool_entry_t*)header_p;
                b_status = (sg_p->magic == FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER);
                if (sg_p->sg[FBE_RAID_SG_1_ENTRIES - 1].count != 0)
                {
                    b_status = FBE_FALSE;
                }
                if (sg_p->sg[FBE_RAID_SG_1_ENTRIES - 1].address != NULL)
                {
                    b_status = FBE_FALSE;
                }
            }
            break;
        case FBE_RAID_MEMORY_TYPE_SG_8:
            {
                fbe_raid_sg_8_pool_entry_t *sg_p = (fbe_raid_sg_8_pool_entry_t*)header_p;
                b_status = (sg_p->magic == FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER);
                if (sg_p->sg[FBE_RAID_SG_8_ENTRIES - 1].count != 0)
                {
                    b_status = FBE_FALSE;
                }
                if (sg_p->sg[FBE_RAID_SG_8_ENTRIES - 1].address != NULL)
                {
                    b_status = FBE_FALSE;
                }
            }
            break;
        case FBE_RAID_MEMORY_TYPE_SG_32:
            {
                fbe_raid_sg_32_pool_entry_t *sg_p = (fbe_raid_sg_32_pool_entry_t*)header_p;
                b_status = (sg_p->magic == FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER);
                if (sg_p->sg[FBE_RAID_SG_32_ENTRIES - 1].count != 0)
                {
                    b_status = FBE_FALSE;
                }
                if (sg_p->sg[FBE_RAID_SG_32_ENTRIES - 1].address != NULL)
                {
                    b_status = FBE_FALSE;
                }
            }
            break;
        case FBE_RAID_MEMORY_TYPE_SG_128:
            {
                fbe_raid_sg_128_pool_entry_t *sg_p = (fbe_raid_sg_128_pool_entry_t*)header_p;
                b_status = (sg_p->magic == FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER);
                if (sg_p->sg[FBE_RAID_SG_128_ENTRIES - 1].count != 0)
                {
                    b_status = FBE_FALSE;
                }
                if (sg_p->sg[FBE_RAID_SG_128_ENTRIES - 1].address != NULL)
                {
                    b_status = FBE_FALSE;
                }
            }
            break;
        case FBE_RAID_MEMORY_TYPE_SG_MAX:
            {
                fbe_raid_sg_max_pool_entry_t *sg_p = (fbe_raid_sg_max_pool_entry_t*)header_p;
                b_status = (sg_p->magic == FBE_RAID_MEMORY_FOOTER_MAGIC_NUMBER);
                if (sg_p->sg[FBE_RAID_SG_MAX_ENTRIES - 1].count != 0)
                {
                    b_status = FBE_FALSE;
                }
                if (sg_p->sg[FBE_RAID_SG_MAX_ENTRIES - 1].address != NULL)
                {
                    b_status = FBE_FALSE;
                }
            }
            break;
        default:
            /* The header is not valid.
             */
            b_status = FALSE;
            break;
    };
#endif
    return b_status;
}
/**************************************
 * end fbe_raid_memory_header_validate_footer()
 **************************************/
/*!*******************************************************************
 * fbe_raid_memory_header_init()
 *********************************************************************
 * @brief Initialize the memory header.
 * 
 * @param header_p - Ptr to header.
 * 
 * @return None.
 *
 *********************************************************************/
void fbe_raid_memory_header_init(fbe_raid_memory_header_t *header_p,
                                 fbe_raid_memory_type_t type)
{
    if (RAID_DBG_MEMORY_ENABLED)
    {
        header_p->type = type;
        header_p->magic_number = FBE_RAID_MEMORY_HEADER_MAGIC_NUMBER;
        fbe_queue_element_init(&header_p->queue_element);
        fbe_raid_memory_header_init_footer(header_p);
    }
    return;
}
/**************************************
 * end fbe_raid_memory_header_init()
 **************************************/

/*!*******************************************************************
 * fbe_raid_memory_header_is_valid()
 *********************************************************************
 * @brief Initialize the memory header.
 * 
 * @param header_p - Ptr to header.
 * 
 * @return None.
 *
 *********************************************************************/
fbe_bool_t fbe_raid_memory_header_is_valid(fbe_raid_memory_header_t *header_p)
{
    fbe_bool_t b_status = FBE_TRUE;
    if (RAID_DBG_MEMORY_ENABLED)
    {
        /* The type must be within range.
         */
        if ((header_p->type == FBE_RAID_MEMORY_TYPE_INVALID) ||
            (header_p->type >= FBE_RAID_MEMORY_TYPE_MAX))
        {
            b_status = FBE_FALSE;
        }
    
        /* The magic number must be within range.
         */
        if (header_p->magic_number != FBE_RAID_MEMORY_HEADER_MAGIC_NUMBER)
        {
            b_status = FBE_FALSE;
        }
    
        /* Make sure the footer is valid.
         */
        if (!fbe_raid_memory_header_validate_footer(header_p))
        {
            b_status = FBE_FALSE;
        }
    }
    return b_status;
}
/**************************************
 * end fbe_raid_memory_header_is_valid()
 **************************************/

/*!***************************************************************************
 *          fbe_raid_memory_api_abort_request()
 ***************************************************************************** 
 * 
 * @brief   This api is used to abort an outstanding memory request
 *
 * @param   memory_request_p - Pointer to the memory service request to abort
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note    It is perfectlly acceptable to not locate the memory request (i.e.
 *          the memory service has complete the allocation and has invoked the
 *          completion method.
 * 
 * @author
 *  11/08/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_memory_api_abort_request(fbe_memory_request_t *memory_request_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    
    /* It is legal to invoke this method after a memory request has been
     * aborted.  If the memory request is already aborted simply return.
     */
    if (fbe_memory_request_is_aborted(memory_request_p) == FBE_TRUE)
    {
        return FBE_STATUS_OK;
    }

    /* Trace some information since we don't expect these aborts to occur often
     */
    fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                 "raid: abort memory req: 0x%p callback: 0x%p context: 0x%p \n",
                                 memory_request_p, memory_request_p->completion_function,  
                                 memory_request_p->completion_context);
    fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                 "raid: abort memory req: 0x%p chunk size: 0x%x num of objects: %d priority: %d\n",
                                 memory_request_p, memory_request_p->chunk_size,
                                 memory_request_p->number_of_objects, memory_request_p->priority);

    /* Determine if we need to invoke the mock or the dps abort
     */
    if (fbe_raid_memory_mock_is_mock_enabled() == FBE_TRUE)
    {
        /*! @note To verify that aborting a request corrects timeouts, 
         *        temporarily disable the mock memory abort call below.
         */
        status = (fbe_raid_memory_mock_api_abort)(memory_request_p);
    }
    else
    {
        status = fbe_memory_abort_request(memory_request_p);
    }
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: abort memory request: 0x%p ptr: 0x%p failed with status: 0x%x\n",
                               memory_request_p, memory_request_p->ptr, status);
        return status;
    }

    /* We aborted everything.
     */
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_raid_memory_api_abort_request()
 *****************************************/

/*!**************************************************************************
 * fbe_raid_memory_api_get_raid_memory_statistics()
 ****************************************************************************
 * @brief   This is used to get raid memory statistics information.
 *
 * @param   memory_stats_p - Pointer to the memory statistics structure
 *
 * @return  status - FBE_STATUS_OK - Always
 * 
 * @author
 *  03/07/2011  Swati Fursule - Created   
 *
 **************************************************************************/
fbe_status_t fbe_raid_memory_api_get_raid_memory_statistics(fbe_raid_memory_stats_t *memory_stats_p)
{
    /* copy the raid memory statistics*/
    memory_stats_p->allocations         = fbe_raid_memory_statistics.allocations;
    memory_stats_p->frees               = fbe_raid_memory_statistics.frees;
    memory_stats_p->allocated_bytes     = fbe_raid_memory_statistics.allocated_bytes;
    memory_stats_p->freed_bytes         = fbe_raid_memory_statistics.freed_bytes;
    memory_stats_p->deferred_allocations= fbe_raid_memory_statistics.deferred_allocations;
    memory_stats_p->pending_allocations = fbe_raid_memory_statistics.pending_allocations;
    memory_stats_p->aborted_allocations = fbe_raid_memory_statistics.aborted_allocations;
    memory_stats_p->allocation_errors   = fbe_raid_memory_statistics.allocation_errors;
    memory_stats_p->free_errors         = fbe_raid_memory_statistics.free_errors;
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_raid_memory_api_get_raid_memory_statistics()
 *****************************************/

/*!**************************************************************************
 * fbe_raid_memory_api_reset_raid_memory_statistics()
 ****************************************************************************
 * @brief   This is used to reset raid memory statistics information.
 *
 * @param   None
 *
 * @return  status - FBE_STATUS_OK - Always
 * 
 * @note    No lock / syncronization is performed !
 * 
 * @author
 *  02/11/2014  Ron Proulx  - Created.
 *
 **************************************************************************/
fbe_status_t fbe_raid_memory_api_reset_raid_memory_statistics(void)
{
    /*! @note Although each field will be zeroed atomically, there is no lock 
     *        for the counters as a whole (for performance reasons).  Therefore
     *        there is not guarantee for a consistent set of counters.
     */
    fbe_atomic_exchange(&fbe_raid_memory_statistics.allocations, 0);
    fbe_atomic_exchange(&fbe_raid_memory_statistics.frees, 0);
    fbe_atomic_exchange(&fbe_raid_memory_statistics.allocated_bytes, 0);
    fbe_atomic_exchange(&fbe_raid_memory_statistics.freed_bytes, 0);
    fbe_atomic_exchange(&fbe_raid_memory_statistics.deferred_allocations, 0);
    fbe_atomic_exchange(&fbe_raid_memory_statistics.pending_allocations, 0);
    fbe_atomic_exchange(&fbe_raid_memory_statistics.aborted_allocations, 0);
    fbe_atomic_exchange(&fbe_raid_memory_statistics.allocation_errors, 0);
    fbe_atomic_exchange(&fbe_raid_memory_statistics.free_errors, 0);
    return FBE_STATUS_OK;
}
/********************************************************
 * end fbe_raid_memory_api_reset_raid_memory_statistics()
 ********************************************************/

/********************************
 * end file fbe_raid_memory_api.c
 ********************************/


