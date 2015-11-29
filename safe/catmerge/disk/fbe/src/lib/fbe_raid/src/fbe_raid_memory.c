/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_memory.c
 ***************************************************************************
 *
 * @brief
 *  This file contains memory allocation functions of the raid group object.
 *
 * @version
 *   8/4/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_pool_entries.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/**************************************************************************
 * fbe_raid_memory_plant_one_sg()
 **************************************************************************
 * @brief
 *   This function is called to plant a single newly allocated sg list.
 *   The sg id is placed in the next available address for this sg type,
 *   which we get from the sg_id_list.
 * 
 * @param sg_id_list[] - The vector of sg id addresses that need sg ids.
 * @param sg_id_to_plant - The sg to plant.
 * @param sg_index - The index of the sg type to plant.
 * 
 * @return - None.
 *
 **************************************************************************/

static __inline void fbe_raid_memory_plant_one_sg(fbe_memory_id_t *sg_id_list[],
                                           fbe_memory_id_t sg_id_to_plant,
                                           fbe_u32_t sg_index)
{
    fbe_memory_id_t *current_memory_id;
    /* Save the location to plant to. 
     */
    current_memory_id = sg_id_list[sg_index];

    /* Dequeue the current location from the list.
     */
    sg_id_list[sg_index] = (fbe_memory_id_t*)*sg_id_list[sg_index];

    /* Plant the new sg into the saved location.
     */
    *current_memory_id = (fbe_memory_id_t)sg_id_to_plant;
    return;
}
/**************************************
 * end fbe_raid_memory_plant_one_sg()
 **************************************/
    
/***************************************************************************
 * fbe_raid_memory_push_sgid_address()
 ***************************************************************************
 * DESCRIPTION:
 *  Validate that the current list doesn't already have the pointer.  Then 
 *  add the sg_id address to the list.
 *
 * PARAMETERS:
 *  sg_size             [I],  Size of sgl use to determien which list to goto
 *  sg_id_addr_p        [I],  Address of sg_id to save in the list.
 *  dst_sg_id_list_p    [I],  Address of sg_id type array to get type list
 *
 * RETURN:
 *  fbe_status_t
 *
 * HISTORY:
 *  09/30/08: RDP -- Created. 
 *
 **************************************************************************/
fbe_status_t fbe_raid_memory_push_sgid_address(fbe_u32_t sg_size, 
                                       fbe_memory_id_t *const sg_id_addr_p,
                                       fbe_memory_id_t *dst_sg_id_list_p[])
{
    fbe_u32_t         sg_type;
    fbe_memory_id_t   *list_bm_id_p;
    fbe_status_t      status = FBE_STATUS_OK;

    /* First determine the sg type so we know which   
     * list to add the address to.                    
     */                                               
    sg_type = fbe_raid_memory_sg_count_index(sg_size);          
    if(RAID_COND( (dst_sg_id_list_p == NULL) ||
                  (sg_id_addr_p == NULL)))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "dst_sg_id_list_p %p or sg_id_addr_p %p are NULL\n",
                               dst_sg_id_list_p, sg_id_addr_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
                                                          
    /* Now validate that the contents of the current is NULL.
     */
    if(RAID_COND(*sg_id_addr_p != NULL) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                        "*sg_id_addr_p %p are not NULL\n",
                                        *sg_id_addr_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now validate that the item is not on the list already.
     */
    list_bm_id_p = dst_sg_id_list_p[sg_type];
    while (list_bm_id_p != NULL)
    {
        if(RAID_COND(list_bm_id_p == sg_id_addr_p) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                            "list_bm_id_p %p or sg_id_addr_p %p are same\n",
                                            list_bm_id_p, sg_id_addr_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        list_bm_id_p = *((fbe_memory_id_t **)list_bm_id_p);
    }
    /* Store address in last entry of the linked list.
     * Point the tail of the list to the new tail.    
     */                                               
    *sg_id_addr_p = (fbe_u32_t*)dst_sg_id_list_p[sg_type];   
    dst_sg_id_list_p[sg_type] = sg_id_addr_p;

    return status;
}
/*************************************
 * fbe_raid_memory_push_sgid_address()
 *************************************/

/*****************************************************************************
 *          fbe_raid_memory_info_log_request()
 ***************************************************************************** 
 * 
 * @brief   If enabled log information about the memory request information
 *
 * @param   memory_info_p - Pointer of memory request information structure
 *                             to populate
 * @param   memory_info_p - Pointer to memory request information
 * @param   line - The line number where failure occurred.
 * @param   function_p - File of failure
 * @param   msg_string_p - Pointer to message string (no parameters)
 * 
 * 
 * @return none
 *
 * @author
 *  9/28/2010   Ron Proulx  - Created
 *
 *****************************************************************************/
void fbe_raid_memory_info_log_request(fbe_raid_memory_info_t *const memory_info_p,
                                      fbe_trace_level_t trace_level,
                                      fbe_u32_t line,
                                      const char *function_p,
                                      fbe_char_t *msg_string_p, ...)
{
    fbe_raid_siots_t   *siots_p = fbe_raid_memory_info_get_siots(memory_info_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_INFO_TRACING))
    {
        if (msg_string_p != NULL)
        {
            char buffer[FBE_RAID_MAX_TRACE_CHARS];
            fbe_u32_t num_chars_in_string = 0;
            va_list argList;

            va_start(argList, msg_string_p);
            num_chars_in_string = _vsnprintf(buffer, FBE_RAID_MAX_TRACE_CHARS, msg_string_p, argList);
            buffer[FBE_RAID_MAX_TRACE_CHARS - 1] = '\0';
            va_end(argList);

            if (num_chars_in_string > 0)
            {
                fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                                       buffer);
            }
        }

        fbe_raid_memory_info_trace(memory_info_p, trace_level, line, function_p,
                                   NULL);
    }

    return;
}
/*********************************************
 * end fbe_raid_memory_info_log_request()
 *********************************************/

/*!***************************************************************************
 *          fbe_raid_iots_allocate_memory_complete()
 *****************************************************************************
 *
 * @brief   Completion routine for iots memory allocations.
 *
 * @param   memory_request_p - Pointer to memory request structure
 * @param   context_p - Pointer caller specific context (in this case a iots)
 *
 * @return  FBE_STATUS_OK - success
 *
 * @note    We always continue to the end of this method since we may need to
 *          invoke the state machine to continue the iots.
 * @author
 *  10/14/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static void fbe_raid_iots_allocate_memory_complete(fbe_memory_request_t *memory_request_p,
                                                   fbe_memory_completion_context_t context_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_iots_t        *iots_p = (fbe_raid_iots_t *)context_p;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_bool_t              b_track_deferred_allocation = FBE_FALSE;

    /*! Immediately flag the memory allocation as complete.  If the flag is
     *  already set it is an error.  Always clear the waiting for memory flag
     *  since this is our one and only callback.
     */
    fbe_raid_common_clear_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY);
    if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE))
    {
        /* Trace and flag the error.  We must continue since we may need
         * to invoke the state machine.
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: allocate complete completion flag: 0x%x already set flags: 0x%x\n",
                             FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE, fbe_raid_common_get_flags(&iots_p->common));
        fbe_raid_common_set_memory_allocation_error(&iots_p->common);
    }
    else if (fbe_memory_request_is_aborted(memory_request_p) == FBE_TRUE)
    {
        /* Else if the memory request was aborted, validate that no memory
         * was allocated and continue. By definition this was not immediate.
         */
        fbe_raid_memory_api_aborted_allocation();
        fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED);
        fbe_raid_memory_api_deferred_allocation();

        /* Check if the allocation is complete*/
        if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_TRUE)
        {
            /* Although this condition is rare there will always be a race between
             * the abort request and the memory allocation threads.  Therefore just
             * trace a warning and mark the allocation as complete.
             */
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_WARNING,
                                "raid: iots_p: 0x%p complete memory request aborted. quiescing: %d\n",
                                iots_p, fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE));
            fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE);
        }
        else if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
        {
            /* If we are quiescing do not continue and perform proper cleanup.
             */
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                                "raid: iots_p: 0x%p mem req aborted, iots quiescing..\n",
                                iots_p);

           /* Do not continue, since we are quiescing.
            */
           fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS);
           return;
        }
        else
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                                "raid: iots_p: 0x%p memory request aborted.\n",
                                iots_p);
        }

        /* we don't want to clear FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS here,
         * as we are going back to fbe_raid_iots_generate_siots.
         */
    }
    else if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_TRUE)
    {
        /* Else if the memory allocation completed.  This method does not validate
         * the allocated memory (it is done later).
         */
        fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE);
    }
    else
    {
        /* Else something is wrong flag the error and continue.
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                             "raid: allocate complete failed for request: 0x%p state: %d ptr: 0x%p or num objs: %d ch_size: %d unexpected \n",
                             memory_request_p, memory_request_p->request_state,
                             memory_request_p->ptr, memory_request_p->number_of_objects, memory_request_p->chunk_size);
        fbe_raid_common_set_memory_allocation_error(&iots_p->common);
    }

    /* Check if we need to track this deferred allocation
     */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING) &&
        (fbe_memory_request_is_immediate(memory_request_p) == FBE_FALSE)                                     )
    {
        b_track_deferred_allocation = FBE_TRUE;
    }

    /* Only if the allocation was successful do we want to process the
     * allocated memory.
     */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_TRUE)
    {
        /* The memory request succeeded.  Simply invoke the next siots state.  
         * Update memory statistics (we want the statistics to wrap)
         */
        status = fbe_raid_memory_api_allocation_complete(memory_request_p,
                                                         b_track_deferred_allocation);
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                                "raid: iots_p: 0x%p memory api complete  status: 0x%x\n",
                                iots_p, status);
            fbe_raid_common_set_memory_allocation_error(&iots_p->common);
        }

        /* Some low level debug information
         */
        if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING))
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                            "raid: iots_p: 0x%p alloc memory_request: 0x%p sz: 0x%x objs: %d pri: %d\n",
                            iots_p, memory_request_p, memory_request_p->chunk_size, 
                            memory_request_p->number_of_objects, memory_request_p->priority);
        }
    }

    /* Only restart the iots if this was a deferred allocation.  Otherwise
     * just return.  Notice the we use the memory request to determine this.
     */
    if (fbe_memory_request_is_immediate(memory_request_p) == FBE_FALSE)
    {
        /* Flag this as a deferred allocation, increment the deferred
         * allocation count and invoke the state machine.
         */
        fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION);
        fbe_raid_memory_api_deferred_allocation();
        fbe_raid_common_state((fbe_raid_common_t *)iots_p);
    }

    return;
}
/***********************************************
 * end fbe_raid_iots_allocate_memory_complete()
 ***********************************************/

/*!***************************************************************************
 * fbe_raid_siots_allocate_iots_buffer_complete()
 *****************************************************************************
 *
 * @brief   Completion routine for siots allocation of iots memory buffer.
 *
 * @param   memory_request_p - Pointer to memory request structure
 * @param   context_p - Pointer caller specific context (in this case a iots)
 *
 * @return  FBE_STATUS_OK - success
 *
 * @note    We always continue to the end of this method since we may need to
 *          invoke the state machine to continue the siots.
 * @author
 *  6/29/2012 - Created. Dave Agans
 *
 *****************************************************************************/
static void fbe_raid_siots_allocate_iots_buffer_complete(fbe_memory_request_t *memory_request_p,
                                                         fbe_memory_completion_context_t context_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_siots_t       *siots_p = (fbe_raid_siots_t *)context_p;
    fbe_raid_iots_t        *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_bool_t              b_track_deferred_allocation = FBE_FALSE;
    fbe_memory_number_of_objects_dc_t number_of_objects;
    number_of_objects.number_of_objects = memory_request_p->number_of_objects;

    /*! Immediately flag the memory allocation as complete.  If the flag is
     *  already set it is an error.  Always clear the waiting for memory flag
     *  since this is our one and only callback.
     */
    fbe_raid_common_clear_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY);
    if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE))
    {
        /* Trace and flag the error.  We must continue since we may need
         * to invoke the state machine.
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: allocate complete completion flag: 0x%x already set flags: 0x%x\n",
                             FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE, fbe_raid_common_get_flags(&iots_p->common));
        fbe_raid_common_set_memory_allocation_error(&iots_p->common);
    }
    else if (fbe_memory_request_is_aborted(memory_request_p) == FBE_TRUE)
    {
        /* Else if the memory request was aborted, validate that no memory
         * was allocated and continue.
         */
        fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED);
        fbe_raid_memory_api_aborted_allocation();
        if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_TRUE)
        {
            /* Although this condition is rare there will always be a race between
             * the abort request and the memory allocation threads.  Therefore just
             * trace a warning and mark the allocation as complete.
             */
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_WARNING,
                                "raid: allocate complete memory request aborted yet memory allocated. flags: 0x%x\n",
                                fbe_raid_common_get_flags(&iots_p->common));
            fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE);
        }
        else
        {
            /* Else flag request aborted.
             */
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                                "raid: iots_p: 0x%p memory request aborted.\n",
                                iots_p);
        }
    }
    else if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_TRUE)
    {
        /* Else if the memory allocation completed.  This method does not validate
         * the allocated memory (it is done later).
         */
        fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE);
    }
    else
    {
        /* Else something is wrong flag the error and continue.
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                             "raid: allocate complete failed for request: 0x%p state: %d ptr: 0x%p or num objs: %d chunk_size: %d unexpected \n",
                             memory_request_p, memory_request_p->request_state,
                             memory_request_p->ptr, memory_request_p->number_of_objects, memory_request_p->chunk_size);
        fbe_raid_common_set_memory_allocation_error(&iots_p->common);
    }

    /* Check if we need to track this deferred allocation
     */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING) &&
        (fbe_memory_request_is_immediate(memory_request_p) == FBE_FALSE)                                     )
    {
        b_track_deferred_allocation = FBE_TRUE;
    }

    /* Only if the allocation was successful do we want to process the
     * allocated memory.
     */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_TRUE)
    {
        /* The memory request succeeded.  Simply invoke the next siots state.  
         * Update memory statistics (we want the statistics to wrap)
         */
        status = fbe_raid_memory_api_allocation_complete(memory_request_p,
                                                         b_track_deferred_allocation);
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                                "raid: iots_p: 0x%p memory api complete status: 0x%x\n", iots_p, status);
            fbe_raid_common_set_memory_allocation_error(&iots_p->common);
        }

        /* Some low level debug information
         */
        if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING))
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                            "raid: iots_p: 0x%p alloc memory_request: 0x%p sz: 0x%x objs: %d pri: %d\n",
                            iots_p, memory_request_p, memory_request_p->chunk_size, 
                            memory_request_p->number_of_objects, memory_request_p->priority);
        }
    }

    /* Only restart the siots if this was a deferred allocation.  Otherwise
     * just return.  Notice the we use the memory request to determine this.
     */
    if (fbe_memory_request_is_immediate(memory_request_p) == FBE_FALSE)
    {
        /* Flag this as a deferred allocation, increment the deferred
         * allocation count and invoke the state machine.
         */
        fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION);
        fbe_raid_memory_api_deferred_allocation();
        fbe_raid_common_state((fbe_raid_common_t *)siots_p);
    }

    return;
}
/***********************************************
 * end fbe_raid_siots_allocate_iots_buffer_complete()
 ***********************************************/


/*!***************************************************************************
 *          fbe_raid_siots_allocate_memory_complete()
 *****************************************************************************
 *
 * @brief   Completion routine for siots memory allocations.
 *
 * @param   memory_request_p - Pointer to memory request structure
 * @param   context_p - Pointer caller specific context (in this case a siots)
 *
 * @return  FBE_STATUS_OK - success
 *
 * @note    We always continue to the end of this method since we may need to
 *          invoke the state machine to continue the siots.
 *
 * @author
 *  10/14/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static void fbe_raid_siots_allocate_memory_complete(fbe_memory_request_t *memory_request_p,
                                                    fbe_memory_completion_context_t context_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_siots_t       *siots_p = (fbe_raid_siots_t *)context_p;

    if (!RAID_DBG_MEMORY_ENABLED &&
        fbe_memory_request_is_immediate(memory_request_p))
    {
        fbe_raid_common_clear_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY);
        fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE);
        return;
    }
    else
    {
        fbe_raid_iots_t        *iots_p = (fbe_raid_iots_t *)fbe_raid_siots_get_iots(siots_p);
        fbe_raid_geometry_t    *raid_geometry_p = NULL;
        fbe_bool_t              b_track_deferred_allocation = FBE_FALSE;
        fbe_bool_t              b_memory_tracing_enabled = FBE_FALSE;
    
        /*! Immediately flag the memory allocation as complete.  If the flag is
         *  already set it is an error.  Always clear the waiting for memory flag
         *  since this is our one and only callback.
         */
        fbe_raid_common_clear_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY);
        if (fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE))
        {
            /* Trace and flag the error.  We must continue since we may need
             * to invoke the state machine.
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: allocate complete completion flag: 0x%x already set flags: 0x%x\n",
                                 FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE, fbe_raid_common_get_flags(&siots_p->common));
            fbe_raid_common_set_memory_allocation_error(&siots_p->common);
        }
        else if (fbe_memory_request_is_aborted(memory_request_p) == FBE_TRUE)
        {
            /* Else if the memory request was aborted, validate that no memory
             * was allocated and continue.
             */
            fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED);
            fbe_raid_memory_api_aborted_allocation();
            if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_TRUE)
            {
                /* Although this condition is rare there will always be a race between
                 * the abort request and the memory allocation threads.  Therefore just
                 * trace a warning and mark the allocation as complete.
                 */
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                     "raid: allocate complete memory request aborted yet memory allocated. flags: 0x%x\n",
                                     fbe_raid_common_get_flags(&siots_p->common));
                fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE);
            }
            else
            {
                /* Else flag request aborted.
                 */
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                     "raid: siots_p: 0x%p memory request aborted.\n",
                                     siots_p);
            }
        }
        else if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_TRUE)
        {
            /* Else if the memory allocation completed.  This method does not validate
             * the allocated memory (it is done later).
             */
            fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE);
        }
        else
        {
            /* Else something is wrong flag the error and continue.
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: allocate complete failed for request: 0x%p state: %d ptr: 0x%p or num objs: %d/%d num_pages: %d unexpected \n",
                                 memory_request_p, memory_request_p->request_state,
                                 memory_request_p->ptr, memory_request_p->number_of_objects, siots_p->num_ctrl_pages, siots_p->num_data_pages);
            fbe_raid_common_set_memory_allocation_error(&siots_p->common);
        }
    
        /* Unit test doesn't have an iots
         */
        if (iots_p != NULL)
        {
            raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    
            /* Check if we need to track this deferred allocation
            */
            if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING) &&
                (fbe_memory_request_is_immediate(memory_request_p) == FBE_FALSE)                                     )
            {
                b_track_deferred_allocation = FBE_TRUE;
            }
    
            if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING))
            {
                b_memory_tracing_enabled = FBE_TRUE;
            }
        }
    
        /* Only if the allocation was successful do we want to process the
         * allocated memory.
         */
        if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_TRUE)
        {
            /* The memory request succeeded.  Simply invoke the next siots state.  
             * Update memory statistics (we want the statistics to wrap)
             */
            status = fbe_raid_memory_api_allocation_complete(memory_request_p,
                                                             b_track_deferred_allocation);
            if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: memory api complete for: requested status: 0x%x\n",
                                 status);
                fbe_raid_common_set_memory_allocation_error(&siots_p->common);
            }
    
            /* Some low level debug information
             */
            if (b_memory_tracing_enabled == FBE_TRUE)
            {
                 fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                "raid: siots_p: 0x%p alloc memory_request: 0x%p sz: 0x%x objs: %d pri: %d\n",
                                siots_p, memory_request_p, memory_request_p->chunk_size,
                                memory_request_p->number_of_objects, memory_request_p->priority);
            }
        }
    
        /* Only restart the siots if this was a deferred allocation.  Otherwise
         * just return.  Notice the we use the memory request to determine this.
         */
        if (fbe_memory_request_is_immediate(memory_request_p) == FBE_FALSE)
        {
            /* Flag this as a deferred allocation, increment the deferred
             * allocation count and invoke the state machine.
             */
            fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION);
            fbe_raid_memory_api_deferred_allocation();
            fbe_raid_common_state((fbe_raid_common_t *)siots_p);
        }
        return;
    }
}
/***********************************************
 * end fbe_raid_siots_allocate_memory_complete()
 ***********************************************/

/*!***************************************************************************
 *          fbe_raid_siots_is_allocation_successful()
 *****************************************************************************
 *
 * @brief   Simply determine if the memory allocation request for a siots
 *          completed successfully or not (aborted, error etc).
 *
 * @param   siots_p - Pointer to siots 
 *
 * @return  bool - FBE_TRUE - Ok to process and consume memory
 *                 FBE_FALSE - Memory allocation failed
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_siots_is_allocation_successful(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t              b_allocation_successful = FBE_FALSE;

    /*! Check if allocation is complete, not aborted and no allocation error
     */
    if ( fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE) &&
        !fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED)     &&
        !fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR)       )
    {
        b_allocation_successful = FBE_TRUE;
    }

    /* Return allocation status
     */
    return(b_allocation_successful);
}
/***********************************************
 * end fbe_raid_siots_is_allocation_successful()
 ***********************************************/

/*!***************************************************************************
 *          fbe_raid_iots_is_allocation_successful()
 *****************************************************************************
 *
 * @brief   Simply determine if the memory allocation request for a iots
 *          completed successfully or not (aborted, error etc).
 *
 * @param   iots_p - Pointer to iots 
 *
 * @return  bool - FBE_TRUE - Ok to process and consume memory
 *                 FBE_FALSE - Memory allocation failed
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_iots_is_allocation_successful(fbe_raid_iots_t *iots_p)
{
    fbe_bool_t              b_allocation_successful = FBE_FALSE;

    /*! Check if allocation is complete, not aborted and no allocation error
     */
    if ( fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE) &&
        !fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED)     &&
        !fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR)       )
    {
        b_allocation_successful = FBE_TRUE;
    }

    /* Return allocation status
     */
    return(b_allocation_successful);
}
/***********************************************
 * end fbe_raid_iots_is_allocation_successful()
 ***********************************************/

/*!***************************************************************************
 *          fbe_raid_iots_is_deferred_allocation_successful()
 *****************************************************************************
 *
 * @brief   Simply determine if the memory allocation request for a iots
 *          completed immediately or not.
 *
 * @param   siots_p - Pointer to siots 
 *
 * @return  bool - FBE_TRUE - This is a successful deferred memory allocation
 *                 FBE_FALSE - This is not a successful immediate memory allocation
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_iots_is_deferred_allocation_successful(fbe_raid_iots_t *iots_p)
{
    fbe_bool_t  b_deferred_allocation_successful = FBE_FALSE;

    /*! Check if allocation is complete, not aborted and no allocation error
     */
    if (fbe_raid_iots_is_allocation_successful(iots_p)                                               &&
        fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION)    )
    {
        b_deferred_allocation_successful = FBE_TRUE;
    }

    /* Return allocation status
     */
    return(b_deferred_allocation_successful);
}
/********************************************************
 * end fbe_raid_iots_is_deferred_allocation_successful()
 ********************************************************/
 
/*!***************************************************************************
 *          fbe_raid_siots_is_deferred_allocation_successful()
 *****************************************************************************
 *
 * @brief   Simply determine if the memory allocation request for a siots
 *          completed immediately or not.
 *
 * @param   siots_p - Pointer to siots 
 *
 * @return  bool - FBE_TRUE - This is a successful deferred memory allocation
 *                 FBE_FALSE - This is not a successful immediate memory allocation
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_siots_is_deferred_allocation_successful(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t  b_deferred_allocation_successful = FBE_FALSE;

    /*! Check if allocation is complete, not aborted and no allocation error
     */
    if (fbe_raid_siots_is_allocation_successful(siots_p)                                               &&
        fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION)    )
    {
        b_deferred_allocation_successful = FBE_TRUE;
    }

    /* Return allocation status
     */
    return(b_deferred_allocation_successful);
}
/********************************************************
 * end fbe_raid_siots_is_deferred_allocation_successful()
 ********************************************************/

/*!***************************************************************************
 *          fbe_raid_memory_request_init()
 *****************************************************************************
 *
 * @brief   This method populates the passed memory service request (parameters
 *          have been validates against raid requirements).  It validates the
 *          parameters against the memory service requirements.
 *
 * @param   memory_request_p - Pointer to memory service request structure to
 *                             populate
 * @param   priority - Priority of memory request
 * @param   memory_type - The raid memory type to allocate
 * @param   num_items - Number if `memory type' items to allocate
 * @param   completion_function - The memory allocation completion callback
 * @param   completion_context - Pointer to context information
 * 
 * @return  status - Typically FBE_STATUS_OK
 * 
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 *****************************************************************************/

static fbe_status_t fbe_raid_memory_request_init(fbe_memory_request_t *memory_request_p,
                                                 fbe_u32_t raid_data_size,
                                                 fbe_u32_t raid_ctrl_size,
                                                 fbe_memory_number_of_objects_dc_t number_of_objects,
                                                 fbe_memory_completion_function_t completion_function,
                                                 fbe_memory_completion_context_t completion_context)

{
    fbe_status_t            status;
    fbe_memory_chunk_size_t ctrl_size = 0;
    fbe_memory_chunk_size_t data_size = 0;
    fbe_memory_chunk_size_t chunk_size;

    data_size = fbe_raid_memory_size_to_dps_size(raid_data_size);
    ctrl_size = fbe_raid_memory_size_to_dps_size(raid_ctrl_size);
    fbe_memory_dps_ctrl_data_to_chunk_size(ctrl_size, data_size, &chunk_size);

    /* Now build the chunk request.  The resource priority should have already
     * been populated in the memory request.
     */
    status = fbe_memory_build_dc_request(memory_request_p, 
                                         chunk_size,
                                         number_of_objects,
                                         memory_request_p->priority,
                                         memory_request_p->io_stamp,
                                         completion_function,
                                         completion_context);

    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR, 
                               "raid: build chunk failed chunk_size: %d num_items: %d/%d fn: 0x%p context: 0x%p status: 0x%x\n",
                               chunk_size, number_of_objects.split.control_objects,
                               number_of_objects.split.data_objects, completion_function, completion_context, status); 
        return (status);
    }   

    return (status);
}
/*********************************************
 * end fbe_raid_memory_request_init()
 *********************************************/

/*!***************************************************************************
 *          fbe_raid_siots_allocate_memory()
 *****************************************************************************
 *
 * @brief   This method is invoked to allocate the resources required for a 
 *          single siots.  This includes buffers, fruts, sgls and (1) nested
 *          siots.
 *
 * @param   siots_p - Pointer to siots that contains sufficient information to
 *                    generate the memory request.
 * 
 * @return  status - There are only (3) possible status values returned:
 *                  FBE_STATUS_OK - The memory was immediately available
 *                  FBE_STATUS_PENDING - The callback will be invoked when the
 *                                       memory is available
 *                  Other - (Typically FBE_STATUS_GENERIC_FAILURE) - Something
 *                          went wrong.
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 *****************************************************************************/

fbe_status_t fbe_raid_siots_allocate_memory(fbe_raid_siots_t *siots_p)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_memory_request_t       *memory_request_p = fbe_raid_siots_get_memory_request(siots_p);
    fbe_raid_iots_t            *iots_p = (fbe_raid_iots_t *)fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t        *raid_geometry_p = NULL;
    fbe_bool_t                  b_memory_tracking_enabled = FBE_FALSE;
    fbe_u32_t                   total_pages = siots_p->num_ctrl_pages + siots_p->num_data_pages;
    fbe_memory_number_of_objects_dc_t number_of_objects;

    /*! @note Currently the raid memory unit tests siots doesn't have
     *        an iots or geometry.
     */
    if (RAID_DBG_ENA(iots_p != NULL))
    {
        raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
        if ((raid_geometry_p != NULL)                                                                         &&
            fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING)    )
        {
            b_memory_tracking_enabled = FBE_TRUE;
        }
    }

    /* Validate that the page size and number of pages are valid.
     */
    if (RAID_DBG_ENA(!fbe_raid_siots_validate_page_size(siots_p)))
    {
        /* Mark the siots in error and return failure status
         */
        fbe_raid_common_set_memory_allocation_error(&siots_p->common);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: page size: %d is not valid \n",
                             siots_p->ctrl_page_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_ENA((total_pages < 1) || (total_pages >= 0x20000)))
    {
        /* Mark the siots in error and return failure status
         */
        fbe_raid_common_set_memory_allocation_error(&siots_p->common);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: num pages to allocate: %d/%d is not valid \n",
                             siots_p->num_ctrl_pages, siots_p->num_data_pages);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that the siots doesn't already have a memory request
     * pending.
     */
    if (RAID_DBG_ENA(fbe_memory_request_is_in_use(memory_request_p)))
    {
        /* Mark the siots in error and return failure status
         */
        fbe_raid_common_set_memory_allocation_error(&siots_p->common);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: memory request: 0x%p is still in use ptr: 0x%p request state: %d chunk_size: %d\n",
                             memory_request_p, memory_request_p->ptr, 
                             memory_request_p->request_state, memory_request_p->chunk_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Generate the preferred page size (we have already validated the
     * page size above)
     */
    number_of_objects.split.data_objects = siots_p->num_data_pages;
    number_of_objects.split.control_objects = siots_p->num_ctrl_pages;

    /* Fill out the request to allocate the neccessary resources.  The resource
     * priority should have already been populated in the siots memory request.
     */
    status = fbe_raid_memory_request_init(memory_request_p,
                                          siots_p->ctrl_page_size * FBE_BE_BYTES_PER_BLOCK, 
                                          siots_p->data_page_size * FBE_BE_BYTES_PER_BLOCK, 
                                          number_of_objects,
                                          (fbe_memory_completion_function_t) fbe_raid_siots_allocate_memory_complete, /* SAFEBUG - cast to supress warning but must fix because memory completion routine shouldn't return status */
                                          (fbe_memory_completion_context_t)siots_p);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /* Mark the siots in error and return failure status
         */
        fbe_raid_common_set_memory_allocation_error(&siots_p->common);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: initialize memory request: 0x%p page_size: %d num_pages: %d/%d failed with status: 0x%x\n",
                             memory_request_p, siots_p->ctrl_page_size, siots_p->num_ctrl_pages, siots_p->num_data_pages, status); 
        return (status);
    }

    /* Invoke the method to allocate the resource.  The status returned 
     * indicates if the allocation was immediate or not.  For all success cases 
     * the callback is invoked.  Any additional processing must be performed in
     * the state that processes the memory completion.
     */
    fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY);

    fbe_transport_memory_request_set_io_master(memory_request_p, fbe_raid_siots_get_iots(siots_p)->packet_p);

    status = fbe_raid_memory_api_allocate(memory_request_p,
                                          b_memory_tracking_enabled);
    if (RAID_MEMORY_COND((status != FBE_STATUS_OK)      &&
                         (status != FBE_STATUS_PENDING)    ))
    {
        /* Mark the siots in error and return failure status
         */
        fbe_raid_common_set_memory_allocation_error(&siots_p->common);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: memory allocation request: 0x%p page_size: %d num_pages: %d/%d failed with status: 0x%x\n",
                             memory_request_p, siots_p->ctrl_page_size, siots_p->num_ctrl_pages, siots_p->num_data_pages, status); 
        return (status);
    }

    /* If the request didn't complete immediately mark this request as `deferred' 
     * and increment the number of of pending allocations.
     */
    if (fbe_memory_request_is_immediate(memory_request_p) == FBE_FALSE)
    {
        /*! @note Do not touch the siots after a memory request since the
         *        callback can be invoked at anytime and it will set the 
         *        deferred allocation flag.
         */
        fbe_raid_memory_api_pending_allocation();
        status = FBE_STATUS_PENDING; 
    }

    return (status);
}
/*****************************************
 * fbe_raid_siots_allocate_memory()
 *****************************************/

/*!**************************************************************************
 * fbe_raid_siots_free_memory()
 ****************************************************************************
 * @brief   This function frees the memory associated with a siots..  It does
 *          not free the siots itself.  It is expected that when the iots is
 *          destroyed any allocated siots will be freed.  Unlike `destroy',
 *          the actual memory will be freed.
 *
 * @param   siots_p - Pointer to siots to free memory for  
 *
 * @param   FBE_STATUS_GENERIC_FAILURE - Invalid parameter or error freeing.
 *          FBE_STATUS_OK - Memory was allocated successfully.
 * 
 * @author
 *  08/05/2009  Rob Foley - Created   
 *
 **************************************************************************/
fbe_status_t fbe_raid_siots_free_memory(fbe_raid_siots_t *siots_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_memory_request_t       *memory_request_p = fbe_raid_siots_get_memory_request(siots_p);
    fbe_raid_iots_t            *iots_p = (fbe_raid_iots_t *)fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t        *raid_geometry_p = NULL;
    fbe_bool_t                  b_memory_tracking_enabled = FBE_FALSE;
    fbe_bool_t                  b_memory_tracing_enabled = FBE_FALSE;

    /* If we didn't need to allocate any memory (indicated by the memory 
     * in-use method), return success
     */
    if (fbe_memory_request_is_in_use(memory_request_p) == FBE_FALSE)
    {
        /* If we are still waiting for memory then something is wrong.
         * We should have aborted any outstanding memory requests.
         */
        if (fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY))
        {
            /* This is unexpected so trace the error
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: siots_p: 0x%p memory_request: 0x%p waiting for memory flag: 0x%x unexpected flags: 0x%x\n",
                                 siots_p, memory_request_p, 
                                 FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY, fbe_raid_common_get_flags(&siots_p->common));
            fbe_raid_common_set_memory_free_error(&siots_p->common);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        return FBE_STATUS_OK;
    }

    /*! @note Currently the raid memory unit tests siots doesn't have
     *        an iots or geometry.
     */
    if (RAID_DBG_MEMORY_ENABLED && iots_p != NULL)
    {
        raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
        if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING))
        {
            b_memory_tracking_enabled = FBE_TRUE;
        }
        if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING))
        {
            
            b_memory_tracing_enabled = FBE_TRUE;
        }
    }

    /* Now invoke the api that use the siots memory request to free any memory
     * that was allocated for this siots.  The free api will handle aborted
     * allocations.
     */

    /* Some low level debug information
     */
    if (RAID_DBG_MEMORY_ENABLED && b_memory_tracing_enabled == FBE_TRUE)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                        "raid: siots_p: 0x%p free  memory_request: 0x%p sz: 0x%x objs: %d pri: %d\n",
                        siots_p, memory_request_p, memory_request_p->chunk_size, 
                        memory_request_p->number_of_objects, memory_request_p->priority);
    }

    /* Invoke free api with proper setting of memory tracking flag
     */
    status = fbe_raid_memory_api_free(memory_request_p, 
                                      b_memory_tracking_enabled);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /* Trace the error and return failure status
         */
        fbe_raid_common_set_memory_free_error(&siots_p->common);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: siots_p: 0x%p memory api free request for memory request: 0x%p failed with status: 0x%x\n",
                             siots_p, memory_request_p, status);
        return status;
    }

    /* clear the common allocated flag so we can make subsequent memory allocations using this siots
    */
    fbe_raid_common_clear_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE);

    /* If we made it this far return success
     */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_siots_free_memory()
 **************************************/

/*!**************************************************************
 * fbe_raid_memory_calculate_page_size()
 ****************************************************************
 * @brief
 *  Determine the page size based on the number of fruts
 *  and the number of buffers needed.
 *  Note that this might change to include other characteristics
 *  including sg lengths in the future.
 *
 * @param  siots_p - pointer to siots
 * @param  num_fruts - number of fruts needed by siots         
 * @param num_blocks - number of blocks of buffers needed by siots
 *
 * @return fbe_u32_t the number of blocks in the page size.   
 *
 * @author
 *  9/16/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_raid_memory_calculate_page_size(fbe_raid_siots_t * siots_p, fbe_u16_t num_fruts, fbe_u16_t num_blocks)
{   
    fbe_u32_t page_size = FBE_RAID_PAGE_SIZE_STD;

    /* First decide what kind of page size to use.
     *  
     * We must use a larger size if the number of blocks needed is bigger than the 
     * standard buffer size, since this is what the code that uses memory assumes.
     * Picking PAGE_SIZE_MAX if xfer_count is greater than cache page size is an
     * optimization to minimze memory reserved for deadlock avoidance. This optimization
     * is in anticipation of memory that needs to be allocated if IO encounters error
     * and memory needs to allocated for verify.       
     */
    if ((num_blocks > FBE_RAID_PAGE_SIZE_STD)                                                        ||
        ((num_fruts * sizeof(fbe_raid_fruts_t)) > (FBE_RAID_PAGE_SIZE_STD * FBE_BE_BYTES_PER_BLOCK)) ||
        (siots_p->xfer_count > FBE_RAID_PAGE_SIZE_CACHE))
    {
        page_size = FBE_RAID_PAGE_SIZE_MAX;
    }
    return page_size;
}
/******************************************
 * end fbe_raid_memory_calculate_page_size()
 ******************************************/

/*!**************************************************************
 * fbe_raid_memory_calculate_ctrl_page_size()
 ****************************************************************
 * @brief
 *  Determine the page size for control buffers.
 *
 * @param  siots_p - pointer to siots
 * @param  num_fruts - number of fruts needed by siots
 *
 * @return fbe_u32_t the number of blocks in the page size.   
 *
 * @author
 *  10/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_raid_memory_calculate_ctrl_page_size(fbe_raid_siots_t * siots_p, fbe_u16_t num_fruts)
{   
    fbe_u32_t page_size = FBE_RAID_PAGE_SIZE_MAX;
    /* For now since even small operations need multiple small pages we will just use large.       
     */
    return page_size;
}
/******************************************
 * end fbe_raid_memory_calculate_ctrl_page_size()
 ******************************************/
/*!**************************************************************
 * fbe_raid_memory_calculate_data_page_size()
 ****************************************************************
 * @brief
 *  Determine the page size for control buffers.
 *
 * @param  siots_p - pointer to siots
 * @param  num_fruts - number of fruts needed by siots
 *
 * @return fbe_u32_t the number of blocks in the page size.   
 *
 * @author
 *  10/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_raid_memory_calculate_data_page_size(fbe_raid_siots_t * siots_p, fbe_u16_t num_blocks)
{   
    fbe_u32_t page_size = FBE_RAID_PAGE_SIZE_MAX;
    return page_size;
}
/******************************************
 * end fbe_raid_memory_calculate_data_page_size()
 ******************************************/
/*!**************************************************************
 * fbe_raid_memory_validate_page_size()
 ****************************************************************
 * @brief
 *  Determine if the page size is OK.
 *
 * @param page_size - number of blocks in the page
 *
 * @return fbe_bool_t FBE_TRUE on success FBE_FALSE on failure
 *
 ****************************************************************/

fbe_bool_t fbe_raid_memory_validate_page_size(fbe_u32_t page_size)
{   
    fbe_bool_t b_return = FBE_FALSE;

    if ((page_size == FBE_RAID_PAGE_SIZE_STD) || (page_size == FBE_RAID_PAGE_SIZE_MAX))
    {
        /* Page size is valid.
         */
        b_return = FBE_TRUE;
    }
    return b_return;
}
/******************************************
 * end fbe_raid_memory_validate_page_size()
 ******************************************/
/*!**************************************************************
 * fbe_raid_siots_validate_page_size()
 ****************************************************************
 * @brief
 *  Determine if the page size is OK.
 *
 * @param siots_p - current I/O.
 *
 * @return fbe_bool_t FBE_TRUE on success FBE_FALSE on failure
 *
 ****************************************************************/

fbe_bool_t fbe_raid_siots_validate_page_size(fbe_raid_siots_t *siots_p)
{   
    fbe_bool_t b_return = FBE_TRUE;

    if (!fbe_raid_memory_validate_page_size(siots_p->ctrl_page_size))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                     "mirror: ctrl page_size: 0x%x is invalid \n", siots_p->ctrl_page_size);
        b_return = FBE_FALSE;
    }
    if (!fbe_raid_memory_validate_page_size(siots_p->data_page_size))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                     "mirror: data page_size: 0x%x is invalid \n", siots_p->data_page_size);
        b_return = FBE_FALSE;
    }
    /*! @note For now they both need to be the same.
     */
    if (siots_p->ctrl_page_size != siots_p->data_page_size)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: ctrl page size %d != data page_size %d is invalid \n", 
                             siots_p->ctrl_page_size, siots_p->data_page_size);
        b_return = FBE_FALSE;
    }

    return b_return;
}
/******************************************
 * end fbe_raid_siots_validate_page_size()
 ******************************************/

/*!**************************************************************
 * fbe_raid_memory_chunk_size_to_page_size()
 ****************************************************************
 * @brief
 *  Convert from memory service chunk size to raid page size in
 *  blocks
 *
 * @param chunk_size - Memory service chunk size in bytes
 * @param b_control_mem - Indicates if memory we are interested in
 *                        is control or data.  This can determine
 *                        the size of the pages allcated.
 *
 * @return page_size - RAID page size in blocks
 *
 ****************************************************************/

fbe_u32_t fbe_raid_memory_chunk_size_to_page_size(fbe_u32_t chunk_size, fbe_bool_t b_control_mem)
{ 
    fbe_u32_t   page_size = 0;
      
    /* Just switch on supported chunks sizes
     */
    switch(chunk_size)
    {
        case FBE_MEMORY_CHUNK_SIZE_PACKET_PACKET:
        case FBE_MEMORY_CHUNK_SIZE_FOR_PACKET:
            page_size = FBE_RAID_PAGE_SIZE_MIN;
            break;

        case FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET:
            if (b_control_mem)
            {
                page_size = FBE_RAID_PAGE_SIZE_MIN;
            }
            else
            {
                page_size = FBE_RAID_PAGE_SIZE_MAX;
            }
            break;

        case FBE_MEMORY_CHUNK_SIZE_PACKET_64_BLOCKS:
            if (b_control_mem)
            {
                page_size = FBE_RAID_PAGE_SIZE_MAX;
            }
            else
            {
                page_size = FBE_RAID_PAGE_SIZE_MIN;
            }
            break;

        case FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS:
        case FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO:
            page_size = FBE_RAID_PAGE_SIZE_MAX;
            break;

        default:
            /* Logic error
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: chunk_size: %d isn't supported \n",
                                   chunk_size);
            break;
    }

    return page_size;
}
/**********************************************
 * end fbe_raid_memory_chunk_size_to_page_size()
 **********************************************/

/*!**************************************************************
 * fbe_raid_memory_calculate_pages_for_struct()
 ****************************************************************
 * @brief
 *  Calculate how many more pages are needed for this number of
 *  this type of structure.
 *
 * @param structure_size - Number of bytes in this structure.
 * @param num_structures - Number of structures to calculate for.
 * @param page_size_bytes - Size of each memory page.
 * @param num_pages_p - Number of pages needed.
 * @param mem_left_p - Memory remaining in the last page.
 *
 * @return None.
 *
 * @author
 *  2/4/2010 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_memory_calculate_pages_for_struct(fbe_u32_t structure_size,
                                                fbe_u32_t num_structures,
                                                fbe_u32_t page_size_bytes,
                                                fbe_u32_t *num_pages_p,
                                                fbe_u32_t *mem_left_p)

{
    fbe_u32_t num_structs_per_page = page_size_bytes / structure_size;
    fbe_u32_t num_pages = 0;
    fbe_u32_t mem_left;
    /* Determine how many full pages are needed to fit these structures.
     */
    num_pages += (num_structures / num_structs_per_page);

    if (num_structures % num_structs_per_page)
    {
        /* Add on one for remainder if needed for a partial page.
         * Also calculate how much memory is left in this page after 
         * using num_fruts_in_last_page in this page.
         */
        fbe_u32_t num_structs_in_last_page = num_structs_per_page - (num_structures % num_structs_per_page);
        num_pages++;
        mem_left = page_size_bytes - (num_structs_in_last_page * structure_size);
    }
    else
    {
        /* We used the majority of the page.  Just calculate what is leftover 
         * after we fill a page with structs. 
         */
        mem_left = page_size_bytes - (num_structs_per_page * structure_size);
    }

    /* Return the values we just calculated.
     */
    *mem_left_p = mem_left;
    *num_pages_p = num_pages;
    return;
}
/******************************************
 * end fbe_raid_memory_calculate_pages_for_struct()
 ******************************************/
/*!**************************************************************************
 * fbe_raid_memory_calculate_num_pages_fast()
 ****************************************************************************
 * @brief  Calculate the number of pages we need for a fast path write.
 *         This path does not handle errors so we calculate for a minimum
 *         amount of memory.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param page_size_p - Size of each page we expect to use.
 * @param num_control_pages_p - Number of memory pages to allocate
 * @param num_fruts - number of fruts to allocate.
 * @param num_sgs - number of sgs to allocate.
 * @param num_blocks - number of buffers to allocate.
 * @param b_allocate_nested_siots - FBE_TRUE - Allocate nested siots for recovery verify
 *                                  FBE_FALSE - This is backgroind request
 * 
 * @return fbe_status_t
 *
 * @note    The order in which the memory is consumed should match that of
 *          the state machines
 *
 * @author
 *  2/8/2012 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_raid_memory_calculate_num_pages_fast(fbe_u32_t ctrl_page_size,
                                                      fbe_u32_t data_page_size,
                                                      fbe_u16_t *num_control_pages_p,
                                                      fbe_u16_t *num_data_pages_p,
                                                      fbe_u16_t num_fruts,
                                                      fbe_u16_t num_sg_lists,
                                                      fbe_u16_t sg_count,
                                                      fbe_block_count_t num_blocks)
{
    fbe_u32_t ctrl_page_size_bytes = ctrl_page_size * FBE_BE_BYTES_PER_BLOCK;
    fbe_u32_t data_page_size_bytes = data_page_size * FBE_BE_BYTES_PER_BLOCK;
    fbe_u32_t pages = 0;
    fbe_u32_t new_pages;
    fbe_u32_t size;
    fbe_u32_t mem_left = 0;

    /* Step 1: Allocate for buffers.
     * We do this first since when we determine how many buffers and sgs are needed, 
     * we always assume we are starting with no remainder. 
     * Starting with no remainder allows us to not need to break up buffers into 
     * pieces in cases where the I/O we are doing can be broken up into aligned pieces of
     * our buffers.  This means that we will need to use fewer sg entries than we 
     * otherwise might if we had started unaligned. 
     */

    if((num_blocks * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        /*we do not expect the the byte count to exceed 32bit limit here
         */
         return FBE_STATUS_GENERIC_FAILURE;
    }

    size = (fbe_u32_t)(num_blocks * FBE_BE_BYTES_PER_BLOCK);

    /* Not enough left over for these buffers, allocate more pages.
     */
    pages += size / data_page_size_bytes;

    if (size % data_page_size_bytes)
    {
        /* Add on another page for any remainder.
         */
        pages++;
        mem_left = data_page_size_bytes - (size % data_page_size_bytes);
    }
    else
    {
        /* no remainder so mem left is zero.
         */
        mem_left = 0;
    }

    *num_data_pages_p = pages;
    pages = 0;
    mem_left = 0;

    /* Step 2: Add on the number of pages for the fruts.
     */
    if (num_fruts > 0)
    {
        /* Calculate how many more pages we need for the remaining fruts. 
         * We also get back the new mem_left value. 
         */
        fbe_raid_memory_calculate_pages_for_struct(sizeof(fbe_raid_fruts_t),
                                                    num_fruts, 
                                                    ctrl_page_size_bytes,
                                                    &new_pages, &mem_left);
        pages += new_pages;
    }

    /*! @note could we go directly from count to type? 
     * Or just allocate sizeof() * sg_count + 1  size instead? 
     */
    //type_size = sizeof(fbe_sg_element_t) * (sg_count + 1);
    if (sg_count > 1)
    {
        fbe_raid_memory_type_t type;
        fbe_u32_t type_size;

        type = fbe_raid_memory_get_sg_type_by_index(fbe_raid_memory_sg_count_index(sg_count));
        type_size = fbe_raid_memory_type_get_size(type);
        /* Calculate how many more pages we need for the sgs.
         * We also get back the new mem_left value.
         */
        fbe_raid_memory_calculate_pages_for_struct(type_size, 
                                                   num_sg_lists,
                                                   ctrl_page_size_bytes, 
                                                   &new_pages, 
                                                   &mem_left);
        pages += new_pages;
    }
    *num_control_pages_p = pages;
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_memory_calculate_num_pages_fast()
 *****************************************/
/*!**************************************************************************
 * fbe_raid_memory_calculate_num_pages()
 ****************************************************************************
 * @brief  This function is called to allocate the specified number
 *         of fixed sized pages of memory.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param page_size_p - Size of each page we expect to use.
 * @param num_pages_to_allocate_p - Number of memory pages to allocate
 * @param num_fruts - number of fruts to allocate.
 * @param num_sgs - number of sgs to allocate.
 * @param num_blocks - number of buffers to allocate.
 * @param b_allocate_nested_siots - FBE_TRUE - Allocate nested siots for recovery verify
 *                                  FBE_FALSE - This is backgroind request
 * 
 * @return fbe_status_t
 *
 * @note    The order in which the memory is consumed should match that of
 *          the state machines
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_raid_memory_calculate_num_pages(fbe_u32_t page_size,
                                                 fbe_u32_t *num_pages_to_allocate_p,
                                                 fbe_u16_t num_fruts,
                                                 fbe_u16_t *num_sgs_p,
                                                 fbe_block_count_t num_blocks,
                                                 fbe_bool_t b_allocate_nested_siots)
{
    fbe_u32_t page_size_bytes;
    fbe_u32_t pages = 0;
    fbe_u32_t new_pages;
    fbe_u16_t index = 0;
    fbe_u32_t size;
    fbe_u32_t mem_left = 0;
    page_size_bytes = page_size * FBE_BE_BYTES_PER_BLOCK;

    /* Step 1: Allocate for buffers.
     * We do this first since when we determine how many buffers and sgs are needed, 
     * we always assume we are starting with no remainder. 
     * Starting with no remainder allows us to not need to break up buffers into 
     * pieces in cases where the I/O we are doing can be broken up into aligned pieces of
     * our buffers.  This means that we will need to use fewer sg entries than we 
     * otherwise might if we had started unaligned. 
     */

    if((num_blocks * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        /*we do not expect the the byte count to exceed 32bit limit here
         */
         return FBE_STATUS_GENERIC_FAILURE;
    }

    size = (fbe_u32_t)(num_blocks * FBE_BE_BYTES_PER_BLOCK);
    if (mem_left >= size)
    {
         /* The memory remaining from the last allocation satisfies this request.
          */
        mem_left -= size;
    }
    else
    {
        /* Not enough left over for these buffers, allocate more pages.
         */
        pages += size / page_size_bytes;

        if (size % page_size_bytes)
        {
            /* Add on another page for any remainder.
             */
            pages++;
            mem_left = page_size_bytes - (size % page_size_bytes);
        }
        else
        {
            /* no remainder so mem left is zero.
             */
            mem_left = 0;
        }
    }

    /* Step 2: Add on the number of pages for the fruts.
     */
    if (mem_left >= sizeof(fbe_raid_fruts_t))
    {
        /* Allocate some number of fruts from the remainder.
         * We allocate the minimum of either the num we need or 
         * the num available from what is left. 
         */
        fbe_u32_t num_fruts_to_allocate = mem_left / sizeof(fbe_raid_fruts_t);
        num_fruts_to_allocate = FBE_MIN(num_fruts, num_fruts_to_allocate);
        num_fruts -= num_fruts_to_allocate;
        mem_left -= num_fruts_to_allocate * sizeof(fbe_raid_fruts_t);
    }

    if (num_fruts > 0)
    {
        /* Calculate how many more pages we need for the remaining fruts. 
         * We also get back the new mem_left value. 
         */
        fbe_raid_memory_calculate_pages_for_struct(sizeof(fbe_raid_fruts_t),
                                                    num_fruts, 
                                                    page_size_bytes,
                                                    &new_pages, &mem_left);
        pages += new_pages;
    }

    /* Step 3: If required, allocate a nested siots
     */
    if (b_allocate_nested_siots == FBE_TRUE)
    {
        /* Add on the number of pages for nested siots
         */
        if (mem_left >= sizeof(fbe_raid_siots_t))
        {
            /* Enough memory remaining for the siots, just subtract from mem_left.
             */
            mem_left -= sizeof(fbe_raid_siots_t);
        }
        else
        {
            /* Not enough memory remaining for siots, just allocate another page for siots.
             */
            pages++;
            mem_left = page_size_bytes - sizeof(fbe_raid_siots_t);
        }
    }

    /* Step 4: Add on the number of pages for the vcts
     */
    if (mem_left >= sizeof(fbe_raid_vcts_t))
    {
        /* Enough memory remaining for the vcts, just subtract from mem_left.
         */
        mem_left -= sizeof(fbe_raid_vcts_t);
    }
    else
    {
        /* Not enough memory remaining for vcts, just allocate another pages for vcts.
         */
        pages++;
        mem_left = page_size_bytes - sizeof(fbe_raid_vcts_t);
    }

    /* Step 5: Add on the number of pages for the vrts
     */
    if (mem_left >= sizeof(fbe_raid_vrts_t))
    {
        /* Enough memory remaining for the vrts, just subtract from mem_left.
         */
        mem_left -= sizeof(fbe_raid_vrts_t);
    }
    else
    {
        /* Not enough memory remaining for vrts, just allocate another pages for vcts.
         */
        pages++;
        mem_left = page_size_bytes - sizeof(fbe_raid_vrts_t);
    }

    /* Step 6: Add on number of pages for the sgs.
     */
    if (num_sgs_p != NULL)
    {
        for ( index = 0; index < FBE_RAID_MAX_SG_TYPE; index++)
        {
            /* Process each array entry that has a count of sgs that 
             * need to be allocated. 
             */
            if (num_sgs_p[index] > 0)
            {
                fbe_raid_memory_type_t type;
                fbe_u32_t type_size;

                type = fbe_raid_memory_get_sg_type_by_index(index);
                type_size = fbe_raid_memory_type_get_size(type);

                /* Calculate how many more pages we need for the sgs.
                 * We also get back the new mem_left value.
                 */
                fbe_raid_memory_calculate_pages_for_struct(type_size, 
                                                           num_sgs_p[index],
                                                           page_size_bytes, 
                                                           &new_pages, 
                                                           &mem_left);
                pages += new_pages;
            } /* end if num_sgs for this sg index is not zero. */
        } /* end for each sg type. */
    }
    /* Save return results.
     */
    *num_pages_to_allocate_p = pages;
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_memory_calculate_num_pages()
 *****************************************/

/*!**************************************************************************
 * fbe_raid_siots_calculate_num_pages()
 ****************************************************************************
 * @brief  This function is called to allocate the specified number
 *         of fixed sized pages of memory.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param num_fruts - number of fruts to allocate.
 * @param num_sgs - number of sgs to allocate.
 * @param b_allocate_nested_siots - FBE_TRUE - Allocate nested siots for recovery verify
 *                                  FBE_FALSE - This is backgroind request
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/15/2012 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_raid_siots_calculate_num_pages(fbe_raid_siots_t *siots_p,
                                                fbe_u16_t num_fruts,
                                                fbe_u16_t *num_sgs_p,
                                                fbe_bool_t b_allocate_nested_siots)
{
    fbe_u32_t ctrl_page_size_bytes;
    fbe_u32_t data_page_size_bytes;
    fbe_u32_t pages = 0;
    fbe_u32_t new_pages;
    fbe_u16_t index = 0;
    fbe_u32_t size;
    fbe_u32_t mem_left = 0;
    fbe_block_count_t num_blocks = siots_p->total_blocks_to_allocate;
    ctrl_page_size_bytes = siots_p->ctrl_page_size * FBE_BE_BYTES_PER_BLOCK;
    data_page_size_bytes = siots_p->data_page_size * FBE_BE_BYTES_PER_BLOCK;

    /* Step 1: Allocate for buffers.
     * We do this first since when we determine how many buffers and sgs are needed, 
     * we always assume we are starting with no remainder. 
     * Starting with no remainder allows us to not need to break up buffers into 
     * pieces in cases where the I/O we are doing can be broken up into aligned pieces of
     * our buffers.  This means that we will need to use fewer sg entries than we 
     * otherwise might if we had started unaligned. 
     */

    if((num_blocks * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        /*we do not expect the the byte count to exceed 32bit limit here
         */
         return FBE_STATUS_GENERIC_FAILURE;
    }

    size = (fbe_u32_t)(num_blocks * FBE_BE_BYTES_PER_BLOCK);
    if (mem_left >= size)
    {
         /* The memory remaining from the last allocation satisfies this request.
          */
        mem_left -= size;
    }
    else
    {
        /* Not enough left over for these buffers, allocate more pages.
         */
        pages += size / data_page_size_bytes;

        if (size % data_page_size_bytes)
        {
            /* Add on another page for any remainder.
             */
            pages++;
            mem_left = data_page_size_bytes - (size % data_page_size_bytes);
        }
        else
        {
            /* no remainder so mem left is zero.
             */
            mem_left = 0;
        }
    }
    siots_p->num_data_pages = pages;
    pages = 0;
    mem_left = 0;

    /* Step 2: Add on the number of pages for the fruts.
     */
    if (mem_left >= sizeof(fbe_raid_fruts_t))
    {
        /* Allocate some number of fruts from the remainder.
         * We allocate the minimum of either the num we need or 
         * the num available from what is left. 
         */
        fbe_u32_t num_fruts_to_allocate = mem_left / sizeof(fbe_raid_fruts_t);
        num_fruts_to_allocate = FBE_MIN(num_fruts, num_fruts_to_allocate);
        num_fruts -= num_fruts_to_allocate;
        mem_left -= num_fruts_to_allocate * sizeof(fbe_raid_fruts_t);
    }

    if (num_fruts > 0)
    {
        /* Calculate how many more pages we need for the remaining fruts. 
         * We also get back the new mem_left value. 
         */
        fbe_raid_memory_calculate_pages_for_struct(sizeof(fbe_raid_fruts_t),
                                                    num_fruts, 
                                                    ctrl_page_size_bytes,
                                                    &new_pages, &mem_left);
        pages += new_pages;
    }

    /* Step 3: If required, allocate a nested siots
     */
    if (b_allocate_nested_siots == FBE_TRUE)
    {
        /* Add on the number of pages for nested siots
         */
        if (mem_left >= sizeof(fbe_raid_siots_t))
        {
            /* Enough memory remaining for the siots, just subtract from mem_left.
             */
            mem_left -= sizeof(fbe_raid_siots_t);
        }
        else
        {
            /* Not enough memory remaining for siots, just allocate another page for siots.
             */
            pages++;
            mem_left = ctrl_page_size_bytes - sizeof(fbe_raid_siots_t);
        }
    }

    /* Step 4: Add on the number of pages for the vcts
     */
    if (mem_left >= sizeof(fbe_raid_vcts_t))
    {
        /* Enough memory remaining for the vcts, just subtract from mem_left.
         */
        mem_left -= sizeof(fbe_raid_vcts_t);
    }
    else
    {
        /* Not enough memory remaining for vcts, just allocate another pages for vcts.
         */
        pages++;
        mem_left = ctrl_page_size_bytes - sizeof(fbe_raid_vcts_t);
    }

    /* Step 5: Add on the number of pages for the vrts
     */
    if (mem_left >= sizeof(fbe_raid_vrts_t))
    {
        /* Enough memory remaining for the vrts, just subtract from mem_left.
         */
        mem_left -= sizeof(fbe_raid_vrts_t);
    }
    else
    {
        /* Not enough memory remaining for vrts, just allocate another pages for vcts.
         */
        pages++;
        mem_left = ctrl_page_size_bytes - sizeof(fbe_raid_vrts_t);
    }

    /* Step 6: Add on number of pages for the sgs.
     */
    if (num_sgs_p != NULL)
    {
        for ( index = 0; index < FBE_RAID_MAX_SG_TYPE; index++)
        {
            /* Process each array entry that has a count of sgs that 
             * need to be allocated. 
             */
            if (num_sgs_p[index] > 0)
            {
                fbe_raid_memory_type_t type;
                fbe_u32_t type_size;

                type = fbe_raid_memory_get_sg_type_by_index(index);
                type_size = fbe_raid_memory_type_get_size(type);

                /* Calculate how many more pages we need for the sgs.
                 * We also get back the new mem_left value.
                 */
                fbe_raid_memory_calculate_pages_for_struct(type_size, 
                                                           num_sgs_p[index],
                                                           ctrl_page_size_bytes, 
                                                           &new_pages, 
                                                           &mem_left);
                pages += new_pages;
            } /* end if num_sgs for this sg index is not zero. */
        } /* end for each sg type. */
    }
    /* Save return results.
     */
    siots_p->num_ctrl_pages = pages;
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_siots_calculate_num_pages()
 *****************************************/
/*!**************************************************************************
 * fbe_raid_memory_calculate_num_data_pages()
 ****************************************************************************
 * @brief  This function is called to allocate the specified number
 *         of fixed sized pages of memory.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param page_size_p - Size of each page we expect to use.
 * @param num_pages_to_allocate_p - Number of memory pages to allocate
 * @param num_blocks - number of buffers to allocate.
 * 
 * @return fbe_status_t
 *
 * @note    The order in which the memory is consumed should match that of
 *          the state machines
 *
 * @author
 *  10/15/2012 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_raid_memory_calculate_num_data_pages(fbe_u32_t page_size,
                                                      fbe_u32_t *num_pages_to_allocate_p,
                                                      fbe_block_count_t num_blocks)
{
    fbe_u32_t page_size_bytes;
    fbe_u32_t pages = 0;
    fbe_u32_t size;
    fbe_u32_t mem_left = 0;
    page_size_bytes = page_size * FBE_BE_BYTES_PER_BLOCK;

    /* Step 1: Allocate for buffers.
     * We do this first since when we determine how many buffers and sgs are needed, 
     * we always assume we are starting with no remainder. 
     * Starting with no remainder allows us to not need to break up buffers into 
     * pieces in cases where the I/O we are doing can be broken up into aligned pieces of
     * our buffers.  This means that we will need to use fewer sg entries than we 
     * otherwise might if we had started unaligned. 
     */

    if((num_blocks * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        /*we do not expect the the byte count to exceed 32bit limit here
         */
         return FBE_STATUS_GENERIC_FAILURE;
    }

    size = (fbe_u32_t)(num_blocks * FBE_BE_BYTES_PER_BLOCK);
    if (mem_left >= size)
    {
         /* The memory remaining from the last allocation satisfies this request.
          */
        mem_left -= size;
    }
    else
    {
        /* Not enough left over for these buffers, allocate more pages.
         */
        pages += size / page_size_bytes;

        if (size % page_size_bytes)
        {
            /* Add on another page for any remainder.
             */
            pages++;
            mem_left = page_size_bytes - (size % page_size_bytes);
        }
        else
        {
            /* no remainder so mem left is zero.
             */
            mem_left = 0;
        }
    }
    /* Save return results.
     */
    *num_pages_to_allocate_p = pages;
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_memory_calculate_num_data_pages()
 *****************************************/

/*!**************************************************************************
 *  fbe_raid_siots_allocate_iots_buffer()
 ****************************************************************************
 * @brief  This function is called to allocate a single page.
 *
 * @param   iots_p - Pointer to iots that contains the allocated page
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note    Currently only (1) page is allocated
 *
 * @author
 *  6/29/2012 - Created. Dave Agans
 *
 **************************************************************************/

fbe_status_t fbe_raid_siots_allocate_iots_buffer(fbe_raid_siots_t *siots_p)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_iots_t            *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_memory_request_t       *memory_request_p = fbe_raid_iots_get_memory_request(iots_p);
    fbe_raid_geometry_t        *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_bool_t                  b_memory_tracking_enabled = FBE_FALSE;
    fbe_memory_number_of_objects_dc_t number_of_objects;
    fbe_u32_t memory_size;

    /* If memory tracking is enabled set the flag
     */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING))
    {
        b_memory_tracking_enabled = FBE_TRUE;
    }

    /* Validate that the iots doesn't already have a memory request
     * pending.
     */
    if (RAID_MEMORY_COND(fbe_memory_request_is_in_use(memory_request_p)))
    {
        /* Flag the iots in error and return failure status
         */
        fbe_raid_common_set_memory_allocation_error(&iots_p->common);
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: memory request: 0x%p is still in use ptr: 0x%p request state: %d chunk_size: %d\n",
                            memory_request_p, memory_request_p->ptr, 
                            memory_request_p->request_state, memory_request_p->chunk_size); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We are going to allocate only control data since this is not user data.
     */
    number_of_objects.split.control_objects = 1;
    number_of_objects.split.data_objects = 0;
    
    memory_size = fbe_raid_memory_type_get_size(FBE_RAID_MEMORY_TYPE_2KB_BUFFER);
    /*! Fill out the request to allocate the neccessary resources
     */
    status = fbe_raid_memory_request_init(memory_request_p,
                                          memory_size, memory_size, /* ctrl and data size */
                                          number_of_objects,
                                          (fbe_memory_completion_function_t)fbe_raid_siots_allocate_iots_buffer_complete, /* SAFEBUG - cast to surpress warning but must fix because memory completion function shouldn't return status */
                                          (fbe_memory_completion_context_t)siots_p);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /* Flag the iots in error and return failure status
         */
        fbe_raid_common_set_memory_allocation_error(&iots_p->common);
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: initialize memory request: 0x%p chunk_size: %d num_objects: %d failed with status: 0x%x\n",
                            memory_request_p, memory_request_p->chunk_size,
                            memory_request_p->number_of_objects, status); 
        return (status);
    }

    /* Invoke the method to allocate the resource.  The status returned does not 
     * indicate if the allocation was immediate or not.  For all success cases 
     * the callback is invoked.  Any additional processing must be performed in
     * the state that processes the memory completion.
     */
    fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY);

    fbe_transport_memory_request_set_io_master(memory_request_p, iots_p->packet_p);
    
    status = fbe_raid_memory_api_allocate(memory_request_p,
                                          b_memory_tracking_enabled);

    if (RAID_MEMORY_COND((status != FBE_STATUS_OK)      &&
                         (status != FBE_STATUS_PENDING)    ))
    {
        /* Flag the iots in error and return failure status
         */
        fbe_raid_common_set_memory_allocation_error(&iots_p->common);
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: memory allocation request: 0x%p chunk_size: %d num_objects: %d failed with status: 0x%x\n",
                            memory_request_p, memory_request_p->chunk_size,
                            memory_request_p->number_of_objects, status); 
        return (status);
    }

    /* Validate that the memory allocation request was correct
     */
    if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR))
    {
        /* Trace some information and return failure
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: memory allocation request: 0x%p chunk_size: %d num_chunks: %d invalid flags: 0x%x\n",
                            memory_request_p, memory_request_p->chunk_size, 
                            number_of_objects.split.control_objects, fbe_raid_iots_get_flags(iots_p)); 
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* If the request didn't complete immediately mark this request as `deferred' 
     * and increment the number of of pending allocations.
     */
    if (fbe_memory_request_is_immediate(memory_request_p) == FBE_FALSE)
    {
        /*! @note Do not touch the iots after a memory request since the
         *        callback can be invoked at anytime and it will set the 
         *        deferred allocation flag.
         */
        fbe_raid_memory_api_pending_allocation();
        status = FBE_STATUS_PENDING; 
    }

    return (status);
}
/*****************************************
 * fbe_raid_siots_allocate_iots_buffer()
 *****************************************/

/*!**************************************************************************
 *  fbe_raid_iots_allocate_siots()
 ****************************************************************************
 * @brief  This function is called to allocate the specified number of siots.
 *
 * @param   iots_p - Pointer to iots that contains the allocated siots
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note    Currently only (1) siots is allocated
 *
 * @author
 *  8/05/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_raid_iots_allocate_siots(fbe_raid_iots_t *iots_p)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_memory_request_t       *memory_request_p = fbe_raid_iots_get_memory_request(iots_p);
    fbe_raid_memory_type_t      memory_type = FBE_RAID_MEMORY_TYPE_INVALID;
    fbe_u32_t memory_size;
    fbe_u32_t                   num_siots_required = 1;
    fbe_u32_t                   bytes_to_allocate = 0;
    fbe_u32_t                   num_chunks = 1;
    fbe_raid_geometry_t        *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_bool_t                  b_memory_tracking_enabled = FBE_FALSE;
    fbe_memory_number_of_objects_dc_t number_of_objects;

    /* If memory tracking is enabled set the flag
     */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING))
    {
        b_memory_tracking_enabled = FBE_TRUE;
    }

    /* Validate that the iots doesn't already have a memory request
     * pending.
     */
    if (RAID_MEMORY_COND(fbe_memory_request_is_in_use(memory_request_p)))
    {
        /* Flag the iots in error and return failure status
         */
        fbe_raid_common_set_memory_allocation_error(&iots_p->common);
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: memory request: 0x%p is still in use ptr: 0x%p request state: %d chunk_size: %d\n",
                            memory_request_p, memory_request_p->ptr, 
                            memory_request_p->request_state, memory_request_p->chunk_size); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note Now determine the memory type based on an estimate of how many 
     *        siots are required.  We divide the transfer size by the number
     *        of data disks and then divide the result by the maximum per-disk
     *        transfer limit.    The embedded siots will handle any rounding blocks.
     */
    num_siots_required = fbe_raid_iots_determine_num_siots_to_allocate(iots_p);
    if (num_siots_required == 0)
    {
       /* we must have crossed the 32bit limit for number of siots to allocate
        */
        fbe_raid_common_set_memory_allocation_error(&iots_p->common);
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: Failed to allocate siots as number of siots crossed 32bit limit: iots 0x%p\n",
                            iots_p); 
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Now determine the raid memory page based on the size of the siots and
     * how many siots are required.  Finally we need to round up to the nearest
     * block multiple since the raid memory library works on logical blocks.
     */
    bytes_to_allocate = (num_siots_required * sizeof(fbe_raid_siots_pool_entry_t));
    bytes_to_allocate = bytes_to_allocate + (FBE_BE_BYTES_PER_BLOCK - (bytes_to_allocate % FBE_BE_BYTES_PER_BLOCK));
    if (bytes_to_allocate <= fbe_raid_memory_type_get_size(FBE_RAID_MEMORY_TYPE_2KB_BUFFER))
    {
        memory_type = FBE_RAID_MEMORY_TYPE_2KB_BUFFER;
    }
    else if (bytes_to_allocate <= fbe_raid_memory_type_get_size(FBE_RAID_MEMORY_TYPE_32KB_BUFFER))
    {
        memory_type = FBE_RAID_MEMORY_TYPE_32KB_BUFFER;
    }
    else
    {
        fbe_u32_t unused_mem_left;
        memory_type = FBE_RAID_MEMORY_TYPE_32KB_BUFFER;
        fbe_raid_memory_calculate_pages_for_struct(sizeof(fbe_raid_siots_pool_entry_t),
                                                   num_siots_required, 
                                                   fbe_raid_memory_type_get_size(FBE_RAID_MEMORY_TYPE_32KB_BUFFER),
                                                   &num_chunks, &unused_mem_left);
    }
    memory_size = fbe_raid_memory_type_get_size(memory_type);

    /* We are only allocating control structures.
     */
    number_of_objects.split.data_objects = 0;
    number_of_objects.split.control_objects = num_chunks;

    /*! Fill out the request to allocate the neccessary resources
     */
    status = fbe_raid_memory_request_init(memory_request_p,
                                          memory_size, memory_size, /* ctrl and data size */
                                          number_of_objects,
                                          (fbe_memory_completion_function_t)fbe_raid_iots_allocate_memory_complete, /* SAFEBUG - cast to surpress warning but must fix because memory completion function shouldn't return status */
                                          (fbe_memory_completion_context_t)iots_p);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /* Flag the iots in error and return failure status
         */
        fbe_raid_common_set_memory_allocation_error(&iots_p->common);
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: initialize memory request: 0x%p chunk_size: %d num_objects: %d failed with status: 0x%x\n",
                            memory_request_p, memory_request_p->chunk_size,
                            memory_request_p->number_of_objects, status); 
        return (status);
    }

    /* Invoke the method to allocate the resource.  The status returned does not 
     * indicate if the allocation was immediate or not.  For all success cases 
     * the callback is invoked.  Any additional processing must be performed in
     * the state that processes the memory completion.
     */
    fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY);

    
    fbe_transport_memory_request_set_io_master(memory_request_p, iots_p->packet_p);
    
    status = fbe_raid_memory_api_allocate(memory_request_p,
                                          b_memory_tracking_enabled);
    if (RAID_MEMORY_COND((status != FBE_STATUS_OK)      &&
                         (status != FBE_STATUS_PENDING)    ))
    {
        /* Flag the iots in error and return failure status
         */
        fbe_raid_common_set_memory_allocation_error(&iots_p->common);
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: memory allocation request: 0x%p chunk_size: %d num_objects: %d failed with status: 0x%x\n",
                            memory_request_p, memory_request_p->chunk_size,
                            memory_request_p->number_of_objects, status); 
        return (status);
    }

    /* Validate that the memory allocation request was correct
     */
    if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR))
    {
        /* Trace some information and return failure
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: memory allocation request: 0x%p chunk_size: %d num_siots: %d invalid flags: 0x%x\n",
                            memory_request_p, memory_request_p->chunk_size, 
                            num_siots_required, fbe_raid_iots_get_flags(iots_p)); 
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* If the request didn't complete immediately mark this request as `deferred' 
     * and increment the number of of pending allocations.
     */
    if (fbe_memory_request_is_immediate(memory_request_p) == FBE_FALSE)
    {
        /*! @note Do not touch the iots after a memory request since the
         *        callback can be invoked at anytime and it will set the 
         *        deferred allocation flag.
         */
        fbe_raid_memory_api_pending_allocation();
        status = FBE_STATUS_PENDING; 
    }

    return (status);
}
/*****************************************
 * fbe_raid_iots_allocate_siots()
 *****************************************/

/*!***************************************************************************
 *          fbe_raid_iots_consume_siots()
 *****************************************************************************
 *
 * @brief   The method simply `consumes' the memory that was allocated for a
 *          siots.  It also initializes the siots.  This method simply places 
 *          initialized siots on the `available' queue.
 *
 * @param   iots_p    - ptr to the fbe_raid_iots_t
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  10/21/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_iots_consume_siots(fbe_raid_iots_t *iots_p,
                                         fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_u32_t bytes_allocated = 0;
    fbe_raid_siots_t *queued_siots_p = NULL;
    fbe_raid_siots_pool_entry_t *siots_pool_entry_p = NULL;

    /* Get the next piece of memory from the current location in the memory queue.
     */
    siots_pool_entry_p = fbe_raid_memory_allocate_contiguous(memory_info_p,
                                                             sizeof(fbe_raid_siots_pool_entry_t),
                                                             &bytes_allocated);
    siots_p = &siots_pool_entry_p->siots;

    if (RAID_MEMORY_COND(siots_p == NULL) ||
        (bytes_allocated != sizeof(fbe_raid_siots_pool_entry_t)) )
    {
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: nested_siots_p is NULL or bytes_allocated 0x%x != sizeof(fbe_raid_siots_t) 0x%llx; "
                            "iots_p = 0x%p\n",
                            bytes_allocated,
                (unsigned long long)sizeof(fbe_raid_siots_t), iots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that this siots isn't on the active queue.  It is assumed that
     * we are already under the iots lock so it is ok to walk the queue.
     */
    fbe_raid_iots_get_siots_queue_head(iots_p, &queued_siots_p);
    while (queued_siots_p != NULL)
    {
        /* If this siots is already on the queue something is wrong
         */
        if (RAID_MEMORY_COND(queued_siots_p == siots_p))
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                                "raid: iots_p: 0x%p siots_p: 0x%p is already on active queue \n",
                                iots_p, siots_p); 
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Goto next siots.
         */
        queued_siots_p = fbe_raid_siots_get_next(queued_siots_p);
    }

    /* Validate that this siots isn't on the available queue.  It is assumed that
     * we are already under the iots lock so it is ok to walk the queue.
     */
    fbe_raid_iots_get_available_queue_head(iots_p, &queued_siots_p);
    while (queued_siots_p != NULL)
    {
        /* If this siots is already on the queue something is wrong
         */
        if (RAID_MEMORY_COND(queued_siots_p == siots_p))
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                                "raid: iots_p: 0x%p siots_p: 0x%p is already on available queue \n",
                                iots_p, siots_p); 
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Goto next siots.
         */
        queued_siots_p = fbe_raid_siots_get_next(queued_siots_p);
    }

    /* Now initialize the siots and place it on the `available' queue
     */
    status = fbe_raid_siots_allocated_consume_init(siots_p, iots_p);
    if(RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /* As we are pretty sure that current allocated structure is of siots 
         * and failed only in initialization.
         */
        fbe_raid_common_init_flags(&siots_p->common, FBE_RAID_COMMON_FLAG_TYPE_SIOTS);

        /* We failed to initialize siots which we have allocated from
         * pool. As it is possible that another siots of current iots 
         * is under operation. And, hence we can not complete iots.
         * At best, we will set current siots to unexpected state 
         * kick off it for its completion.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                        "raid: Failed to initialize allocated siots with status: 0x%x \n",
                                        status);
    }

    /* Add the siots to the iots `available' queue
     */
    fbe_raid_common_enqueue_tail(&iots_p->available_siots_queue, &siots_p->common);

    /* Return the execution status
     */
    return status;
}
/**********************************************
 * end of fbe_raid_iots_consume_siots()
 **********************************************/

/*!**************************************************************************
 * fbe_raid_iots_free_memory()
 ****************************************************************************
 * @brief   This function frees the memory associated with a iots.  Currently
 *          the only memory is:
 *             (1) or more siots that needed to be allocated;
 *                 (usually the embedded siots is sufficient), or
 *             a buffer for temporary storage of write log header information
 *
 * @param   iots_p - Pointer to iots to free memory for  
 *
 * @param   FBE_STATUS_GENERIC_FAILURE - Invalid parameter or error freeing.
 *          FBE_STATUS_OK - Memory was allocated successfully.
 * 
 * @author
 *  08/05/2009  Rob Foley - Created   
 *
 **************************************************************************/
fbe_status_t fbe_raid_iots_free_memory(fbe_raid_iots_t *iots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_memory_request_t   *memory_request_p = fbe_raid_iots_get_memory_request(iots_p);
    fbe_raid_geometry_t    *raid_geometry_p;

    /* If we didn't need to allocate any memory (indicated by the memory 
     * in-use method), return success
     */
    if (fbe_memory_request_is_in_use(memory_request_p) == FBE_FALSE)
    {
        /* If we are still waiting for memory then something is wrong.
         * We should have aborted any outstanding memory requests.
         */
        if (RAID_DBG_ENA(fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY)))
        {
            /* This is unexpected transition to unexpected state
             */
            fbe_raid_iots_set_unexpected_error(iots_p, FBE_RAID_IOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                               "raid: iots_p: 0x%p memory_request: 0x%p waiting for memory flag: 0x%x unexpected flags: 0x%x\n",
                                               iots_p, memory_request_p, 
                                               FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY, fbe_raid_common_get_flags(&iots_p->common));
            fbe_raid_common_set_memory_free_error(&iots_p->common);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        return FBE_STATUS_OK;
    }

    /* Some low level debug information
     */
    raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    if (RAID_DBG_ENA(fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING)))
    {
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                            "raid: iots_p: 0x%p free  memory_request: 0x%p sz: 0x%x objs: %d pri: %d\n",
                            iots_p, memory_request_p, memory_request_p->chunk_size, 
                            memory_request_p->number_of_objects, memory_request_p->priority);
    }

    /* Now invoke the api that use the iots memory request to free any memory
     * that was allocated for this iots.  The free api handles aborted 
     * allocations.
     */
    status = fbe_raid_memory_api_free(memory_request_p, FBE_FALSE);
    if (RAID_MEMORY_COND(status == FBE_STATUS_OK))
    {
        /* If we made it this far return success
         */
        return FBE_STATUS_OK;
    }
    else
    {
        /* Trace the error and return failure status
         */
        fbe_raid_common_set_memory_free_error(&iots_p->common);
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_CRITICAL,
                            "raid: memory api free request for memory_request_p: 0x%p failed with status: 0x%x\n",
                            memory_request_p, status);
        return status;
    }
}
/**************************************
 * end fbe_raid_iots_free_memory()
 **************************************/

/*!***************************************************************************
 *          fbe_raid_iots_restart_allocate_siots()
 *****************************************************************************
 *
 * @brief   Validate the the memory request state is `aborted' then re-initialize
 *          any common flags and re-start the allocation. 
 *
 * @param   iots_p - Pointer to iots that contains the allocated siots
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note    Although the memory request and state are still setup to handle the
 *          the memory request, the cleanest and best solution is simply to 
 *          the standard `start' iots which will handle both the `immediate' and
 *          `deferred' allocation cases.
 *
 * @author
 *  11/09/2010  Ron Proulx - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_iots_restart_allocate_siots(fbe_raid_iots_t *iots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_memory_request_t   *memory_request_p = fbe_raid_iots_get_memory_request(iots_p);
    fbe_raid_iots_status_t  iots_status;

    /* Validate that the memory request has been aborted
     */
    fbe_raid_iots_get_status(iots_p, &iots_status);
    if (fbe_memory_request_is_aborted(memory_request_p) == FBE_FALSE)
    {
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: iots_p: 0x%p memory request state: %d isn't aborted \n",
                            iots_p, memory_request_p->request_state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If we are aborting for shutdown and we have marked the iots complete
     * then start the state machine to complete the failed request.
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ABORT_FOR_SHUTDOWN) &&
        (iots_status == FBE_RAID_IOTS_STATUS_COMPLETE)                              )
    {
        /* Complete the iots.
         */
        status = fbe_raid_common_state(&iots_p->common);
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                                "raid: iots_p: 0x%p failed to restart request aborted for shutdown status: 0x%x \n",
                                iots_p, status);
            return status;
        }
    }
    else if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_TRUE)
    {
        /* It is possible that the resources were properly allocated prior to the
         * request being aborted.  If that is the case simply mark the allocation
         * as deferred and invoke the state machine.
         */
        if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
        {
            /*! @note Although this is a logic error (thus we generated an error
             *        trace) it is recoverable by simply not restarting the
             *        allocation (siots) and returning success.
             */
            fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                                   "raid: iots %p restart alloc quiesce set flags 0x%x\n", iots_p, iots_p->flags);
            return FBE_STATUS_OK;
        }

        status = fbe_memory_request_mark_aborted_complete(memory_request_p);
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                                "raid: iots_p: 0x%p failed to mark request with status: 0x%x \n",
                                iots_p, status);
            return status;
        }

        /* Fake a deferred allocation and restart the iots state machine.
         */
        fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION);
        status = fbe_raid_common_state(&iots_p->common);
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                                "raid: iots_p: 0x%p failed to restart completed request with status: 0x%x \n",
                                iots_p, status);
            return status;
        }
    }
    else
    {
        /* Else we should have already cleared FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS so
         * simply re-start the iots.
         */
        if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
        {
            /*! @note Although this is a logic error (thus we generated an error
             *        trace) it is recoverable by simply not restarting the
             *        allocation (siots) and returning success.
             */
            fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                                   "raid: iots %p restart alloc quiesce set flags 0x%x\n", iots_p, iots_p->flags);
            return FBE_STATUS_OK;
        }
        fbe_memory_request_init(memory_request_p);
        status = fbe_raid_iots_start(iots_p);
        if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                                "raid: iots_p: 0x%p failed to re-start iots with status: 0x%x \n",
                                iots_p, status);
        }
        else
        {
            /* We don't expect requests to be restarted verify often.  Therefore trace
             * some information
             */
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                                "raid: iots_p: 0x%p restarted iots after memory request aborted.\n",
                                iots_p);
        }
    
    }

    /* Return the start status
     */
    return status;
}
/*********************************************
 * end fbe_raid_iots_restart_allocate_siots()
 ********************************************/

/*!***************************************************************************
 *          fbe_raid_siots_restart_allocate_memory()
 *****************************************************************************
 *
 * @brief   Validate the the memory request state is `aborted' then re-initialize
 *          any common flags and re-start the allocation. 
 *
 * @param   siots_p - Pointer to siots that contains the memory request
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note    Although the memory request and state are still setup to handle the
 *          the memory request, the cleanest and best solution is simply to 
 *          the standard `start' iots which will handle both the `immediate' and
 *          `deferred' allocation cases.
 *
 * @author
 *  11/09/2010  Ron Proulx - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_siots_restart_allocate_memory(fbe_raid_siots_t *siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_memory_request_t   *memory_request_p = fbe_raid_siots_get_memory_request(siots_p);

    /* Validate that the memory request has been aborted
     */
    if (fbe_memory_request_is_aborted(memory_request_p) == FBE_FALSE)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: siots_p: 0x%p memory request state: %d isn't aborted \n",
                            siots_p, memory_request_p->request_state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Aborting requests that were waiting for memory doesn't occur
     * very often.  Therefore trace some information
     */
    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                         "raid: siots_p: 0x%p restarted siots after memory request aborted.\n",
                         siots_p);

    /* It is possible that the resources were properly allocated prior to the
     * request being aborted.  If that is the case simply mark the allocation
     * as deferred and invoke the state machine.
     */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_TRUE)
    {
        status = fbe_memory_request_mark_aborted_complete(memory_request_p);
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: siots_p: 0x%p failed to mark request with status: 0x%x \n",
                                 siots_p, status);
            return status;
        }

        /* Fake a deferred allocation and start the state machine
         */
        fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION);
        status = fbe_raid_common_state(&siots_p->common);
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: siots_p: 0x%p failed to restart completed request with status: 0x%x \n",
                                 siots_p, status);
            return status;
        }
    }
    else
    {
        /* The siots state has already been set to the deferred allocation callback.
         * Yet this new allocation may complete immediately.  Therefore retry the
         * memory allocation and if it completes `immediately', `fake' and `deferred'
         * allocation and invoke the siots state machine to process the request.
         */
        fbe_memory_request_init(memory_request_p);
        status = fbe_raid_siots_allocate_memory(siots_p);
    
        /* Check if the allocation completed immediately
         */
        if (RAID_MEMORY_COND(status == FBE_STATUS_OK))
        {
            /* Fake a deferred allocation and start the state machine
             */
            fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION);
            status = fbe_raid_common_state(&siots_p->common);
        }
        else if (RAID_MEMORY_COND(status == FBE_STATUS_PENDING))
        {
            /* Else if a deferred allocation, change status to `ok'
             */
            status = FBE_STATUS_OK;
        }
        else
        {
            /* The restart failed
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                "raid: siots_p: 0x%p failed to re-start siots with status: 0x%x \n",
                                siots_p, status);
        }
    }

    /* Return the re-allocate status
     */
    return status;
}
/***********************************************
 * end ffbe_raid_siots_restart_allocate_memory()
 ***********************************************/

/*!**************************************************************
 * fbe_raid_memory_allocate_contiguous()
 ****************************************************************
 * @brief
 *  Allocate the next piece of memory from the set of
 *  pages we allocated previously.
 *
 * @param memory_info_p - Pointer to memory request information
 * @param bytes_to_allocate - Number of bytes to allocate
 * @param bytes_allocated_p - Pointer to bytes allocated
 * 
 * @return void*
 *
 * @note    Assumption is that memory is NOT zeroed and this method
 *          also will NOT zero it!!
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/
void * fbe_raid_memory_allocate_contiguous(fbe_raid_memory_info_t *memory_info_p,
                                           fbe_u32_t bytes_to_allocate,
                                           fbe_u32_t *bytes_allocated_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               page_size_bytes = fbe_raid_memory_info_get_page_size_in_bytes(memory_info_p);
    fbe_u32_t               bytes_remaining_in_page = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);
    fbe_raid_memory_element_t *current_page_p = fbe_raid_memory_info_get_current_element(memory_info_p);
    void                   *memory_p = NULL;
    fbe_raid_buffer_pool_entry_t *pool_entry_p = NULL;
    fbe_u8_t               *address = NULL;

    /* Initialize the bytes allocated to none and validate that the request
     * isn't greater than and page.
     */
    *bytes_allocated_p = 0;
    if (bytes_to_allocate > page_size_bytes)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: Attempt to allocate: %d which is greater than page size: %d\n", 
                               bytes_to_allocate, page_size_bytes);
        fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, 0);
        return NULL;
    }

    /* If we need to go to the next page, do so
     */
    if (bytes_remaining_in_page < bytes_to_allocate)
    {
        /* Not enough space, goto the next item.
         */
        if (!fbe_raid_memory_info_is_memory_left(memory_info_p))
        {
            fbe_raid_memory_info_trace(memory_info_p, FBE_RAID_MEMORY_INFO_TRACE_PARAMS_ERROR,
                                   "raid: Out of memory, queue empty cannot allocate %d bytes.\n", 
                                   bytes_to_allocate);
            fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, 0);
            return NULL;
        }
        if (!fbe_raid_memory_info_is_more_pages(memory_info_p))
        {
            fbe_raid_memory_info_trace(memory_info_p, FBE_RAID_MEMORY_INFO_TRACE_PARAMS_ERROR,
                                   "raid: Out of memory, cannot allocate %d bytes.\n", 
                                   bytes_to_allocate);
            fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, 0);
            return NULL;
        }

        /* Goto the next page
         */
        current_page_p = fbe_raid_memory_info_get_next_element(memory_info_p);
        if(RAID_MEMORY_COND(current_page_p == NULL))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid:  current_page_p is NULL cannot allocate %d bytes.\n",
                                   bytes_to_allocate);
            return NULL;
        }

        /* Update the memory information.
         */
        fbe_raid_memory_info_set_current_element(memory_info_p, current_page_p);
        bytes_remaining_in_page = page_size_bytes;
        fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, bytes_remaining_in_page);
    }

    /* If there are sufficient bytes on this page, allocate it
     */
    if (bytes_remaining_in_page >= bytes_to_allocate)
    {
        /* Enough space left for this fruts.
         * Calculate the offset from the start to the fruts. 
         */
        status = fbe_raid_memory_page_element_to_buffer_pool_entry(current_page_p, &pool_entry_p);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: page element to pool entry for: 0x%p failed with status: 0x%x.\n", 
                               current_page_p, status);
            fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, 0);
            return NULL;
        }

        /* Skip over the header and take into account the space already
         * spoken for on the page
         */
        address = &pool_entry_p->buffer[0];
        address += (page_size_bytes - bytes_remaining_in_page);

        /* Pointer to buffer and decrement the bytes available in the page
         * (buffer).
         */
        memory_p = address;
        *bytes_allocated_p = bytes_to_allocate;
        bytes_remaining_in_page -= bytes_to_allocate;
        fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, bytes_remaining_in_page);
    }

    /* Return the pointer to the memory
     */
    return memory_p;
}
/******************************************
 * end fbe_raid_memory_allocate_contiguous()
 ******************************************/

/*!**************************************************************
 * fbe_raid_memory_allocate_next_buffer()
 ****************************************************************
 * @brief
 *  Allocate the next piece of memory from the set of
 *  pages we allocated previously.
 * 
 * @param memory_info_p - Pointer to memory request information structure
 * @param bytes_to_allocate
 * @param bytes_allocated_p
 * 
 * @return void* 
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/
void * fbe_raid_memory_allocate_next_buffer(fbe_raid_memory_info_t *memory_info_p,
                                           fbe_u32_t bytes_to_allocate,
                                           fbe_u32_t *bytes_allocated_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    void *memory_p = NULL;
    fbe_raid_memory_element_t *mem_p = fbe_raid_memory_info_get_current_element(memory_info_p);
    fbe_raid_buffer_pool_entry_t *pool_entry_p = NULL;
    fbe_u8_t *address = NULL;
    fbe_u32_t bytes_allocated;
    fbe_u32_t page_size_bytes = fbe_raid_memory_info_get_page_size_in_bytes(memory_info_p);
    fbe_u32_t bytes_remaining_in_page = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);

    if (fbe_raid_memory_info_is_memory_left(memory_info_p) == FBE_FALSE)
    {
        /* If we could not allocate the memory, log an error
         */
        if (RAID_MEMORY_COND(bytes_to_allocate > 0))
        {
            fbe_raid_memory_info_trace(memory_info_p, FBE_RAID_MEMORY_INFO_TRACE_PARAMS_ERROR,
                                       "raid: insufficient buffer space to allocate: %d bytes.\n",
                                       bytes_to_allocate);
        }
        return NULL;
    }

    /* If there are any bytes remaining, there is enough
     */
    if (bytes_remaining_in_page > 0)
    {
        /* Enough space left for this buffer.
         * Calculate the offset from the start to the fruts. 
         */
        status = fbe_raid_memory_page_element_to_buffer_pool_entry(mem_p, &pool_entry_p);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: page element to pool entry for : 0x%p failed with status: 0x%x \n", 
                               mem_p, status);
            fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, 0);
            return NULL;
        }
        address = &pool_entry_p->buffer[0];
        address += (page_size_bytes - bytes_remaining_in_page);
        memory_p = address;
    }
    else
    {
        /* Not enough space, goto the next page.
         */
        mem_p = fbe_raid_memory_info_get_next_element(memory_info_p);
        if (RAID_COND(mem_p == NULL))
        {
            /* Memory list is empty, return NULL.
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: memory info: 0x%p exhausted memory\n", 
                                   memory_info_p);
            fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, 0);
            return NULL;
        }
        status = fbe_raid_memory_page_element_to_buffer_pool_entry(mem_p, &pool_entry_p);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: page element to pool entry for: 0x%p failed with status: 0x%x \n", 
                               mem_p, status);
            fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, 0);
            return NULL;
        }
        memory_p = &pool_entry_p->buffer[0];
        fbe_raid_memory_info_set_current_element(memory_info_p, mem_p);
        bytes_remaining_in_page = page_size_bytes;
    }

    /* Allocate what we can from this page.
     */
    bytes_allocated = FBE_MIN(bytes_remaining_in_page, bytes_to_allocate);
    bytes_remaining_in_page -= bytes_allocated;
    fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, bytes_remaining_in_page);
    *bytes_allocated_p = bytes_allocated;

    /* Return pointer to memory for buffer
     */
    return memory_p;
}
/******************************************
 * end fbe_raid_memory_allocate_next_buffer()
 ******************************************/
/*!**************************************************************
 * fbe_raid_memory_get_buffer_end_address()
 ****************************************************************
 * @brief
 *  Calculate the max address used in this buffer.
 *  We take the start address of the buffer and add on the
 *  offset to where we are using in the buffer.
 *
 *  |----------------A------------------| Entire buffer
 *  |----------B--------------|---C-----|
 *     Offset from           /|\ Mem left in buffer
 *      start of buffer       \
 *      to the point in use    \ This is the address we calculate.
 *                               A - C = B,
 *                              Start address + B = return value.
 *
 * @param  buffer_mem_p - Ptr to queue element for buffer    
 * @param  buffer_mem_left - Amount unallocated in this buffer. 
 *
 * @return Address of the end of the buffer that is in use.
 *
 * @author
 *  9/17/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u8_t *fbe_raid_memory_get_buffer_end_address(fbe_queue_element_t *buffer_mem_p,
                                                 fbe_u32_t buffer_mem_left,
                                                 fbe_u32_t page_size)
{
    fbe_raid_buffer_pool_entry_t *buffer_pool_entry_p = (fbe_raid_buffer_pool_entry_t*)buffer_mem_p;

    /* Start with the beginning buffer address. 
     */
    fbe_u8_t* buffer_end_p = &buffer_pool_entry_p->buffer[0];
    /* Add on the offset from the start of the buffer to the point that is in use.
     */
    buffer_end_p += (page_size * FBE_BE_BYTES_PER_BLOCK) - buffer_mem_left;

    /* Take one off so we return the last address that is in use.
     */
    return buffer_end_p - 1;
}
/********************************************
 * end fbe_raid_memory_get_buffer_end_address()
 ********************************************/

/*****************************************************************************
 *          fbe_raid_memory_info_init()
 ***************************************************************************** 
 * 
 * @brief   This method initializes the temporary raid memory request
 *          information structure.  It uses the siots (which contains the
 *          memory request information) to due this.
 *
 * @param   memory_info_p - Pointer of memory request information structure
 *                          to populate
 * @param   memory_request_p - Pointer to memory request that we will use to
 *                             populate memory info with
 * 
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      other - memory_request wasn't properly populated
 *
 * @author
 *  11/7/2012 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_memory_info_init(fbe_raid_memory_info_t *memory_info_p,
                                              fbe_memory_request_t *memory_request_p,
                                              fbe_bool_t b_control_mem)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           page_size = 0;
    fbe_raid_memory_element_t  *master_element_p = NULL;
    fbe_memory_number_of_objects_dc_t number_of_objects;
    fbe_u32_t num_pages;
    number_of_objects.number_of_objects = memory_request_p->number_of_objects;

    if (b_control_mem)
    {
        num_pages = number_of_objects.split.control_objects;
    }
    else
    {
        num_pages = number_of_objects.split.data_objects;
    }

    /*  Calculate the page size and then validate it and the number of objects
     *  We cannot use fbe_raid_memory_validate_page_size since that is specific
     *  to siots.
     */
    page_size = fbe_raid_memory_chunk_size_to_page_size(memory_request_p->chunk_size, b_control_mem);
    switch(page_size)
    {
        case FBE_RAID_PAGE_SIZE_MIN:
        case FBE_RAID_PAGE_SIZE_STD:
        case FBE_RAID_PAGE_SIZE_MAX:
            break;

        default:
            /* Unsupported page size
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR, 
                             "raid: memory_request_p: 0x%p for: %d chunks chunk_size: %d isn't supported\n",
                             memory_request_p, memory_request_p->number_of_objects, memory_request_p->chunk_size);
            return(FBE_STATUS_GENERIC_FAILURE);
    }
    if (num_pages > 0x2000)
    {
        /* Report the error and fail the request
         */        
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR, 
                             "raid: memory_request_p: 0x%p for chunk_size: %d chunks: %d is too large\n",
                             memory_request_p, memory_request_p->chunk_size, memory_request_p->number_of_objects);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Set the memory request pointer
     */
    fbe_raid_memory_info_set_memory_request(memory_info_p, memory_request_p);

    if (num_pages == 0)
    {
        fbe_raid_memory_info_set_queue_head(memory_info_p, NULL);
        fbe_raid_memory_info_set_current_element(memory_info_p, NULL);
        fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, 0);
        fbe_raid_memory_info_set_page_size_in_bytes(memory_info_p, 0);
        fbe_raid_memory_info_set_buffer_end_element(memory_info_p, NULL);
        fbe_raid_memory_info_set_buffer_end_bytes_remaining(memory_info_p, 0);
        return FBE_STATUS_OK;
    }
    /* If the master header isn't valid it means the memory allocation failed
     */
    if (b_control_mem)
    {
        status = fbe_raid_memory_request_get_master_element(memory_request_p, &master_element_p);
    }
    else
    {
        status = fbe_raid_memory_request_get_master_data_element(memory_request_p, &master_element_p);
    }
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR, 
                               "raid: memory_request_p: 0x%p get memory request master failed with status: 0x%x\n",
                               memory_request_p, status);
        return(status);
    }
    fbe_raid_memory_info_set_queue_head(memory_info_p, master_element_p);

    /* Set the current queue element to the first
     */
    fbe_raid_memory_info_set_current_element(memory_info_p, master_element_p);

    /* Set the bytes remaining in the current page to the page size in bytes
     */
    fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p,
                                                     (page_size * FBE_BE_BYTES_PER_BLOCK));

    /* Set the page size in bytes
     */
    fbe_raid_memory_info_set_page_size_in_bytes(memory_info_p,
                                                (page_size * FBE_BE_BYTES_PER_BLOCK));

    /* Now initialize the start of the non-buffer element to NULL
     */
    fbe_raid_memory_info_set_buffer_end_element(memory_info_p, NULL);
    fbe_raid_memory_info_set_buffer_end_bytes_remaining(memory_info_p, 0);

    /* Return success
     */
    return(FBE_STATUS_OK);
}
/******************************************
 * end fbe_raid_memory_info_init()
 ******************************************/
/*****************************************************************************
 * fbe_raid_siots_init_data_mem_info()
 ***************************************************************************** 
 * 
 * @brief
 *  Initalize the data memory information in the siots.
 * 
 * @param siots_p - current I/O.
 * @param memory_info_p - Pointer of memory request information structure
 *                          to populate
 * 
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      other - memory_request wasn't properly populated
 *
 * @author
 *  10/15/2012 - Created. Rob Foley 
 *
 *****************************************************************************/
fbe_status_t fbe_raid_siots_init_data_mem_info(fbe_raid_siots_t *siots_p, 
                                               fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status;
    status = fbe_raid_memory_info_init(memory_info_p, &siots_p->memory_request, FBE_FALSE /* not ctrl but data */);
    
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to init memory info with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }
    return status;
}
/**************************************
 * end fbe_raid_siots_init_data_mem_info()
 **************************************/

/*****************************************************************************
 * fbe_raid_siots_init_control_mem_info()
 ***************************************************************************** 
 * @brief
 *  Initalize the control memory information structure.
 * 
 * @param siots_p - current I/O.
 * @param memory_info_p - Pointer of memory request information structure
 *                          to populate
 * 
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      other - memory_request wasn't properly populated
 *
 * @author
 *  10/15/2012 - Created. Rob Foley 
 *
 *****************************************************************************/
fbe_status_t fbe_raid_siots_init_control_mem_info(fbe_raid_siots_t *siots_p, 
                                                  fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status;

    status = fbe_raid_memory_info_init(memory_info_p, &siots_p->memory_request, FBE_TRUE /* ctrl mem */);

    if(RAID_DBG_COND((siots_p->total_blocks_to_allocate * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX))
    {
        /* we do expect the bytes to allocate to exceed 32bit limit
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: byte count 0x%llx exceeding 32bit limit\n",
                              siots_p->total_blocks_to_allocate);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_raid_siots_init_control_mem_info()
 **************************************/
/*****************************************************************************
 *          fbe_raid_memory_init_memory_info()
 ***************************************************************************** 
 * 
 * @brief   This method initializes the temporary raid memory request
 *          information structure.  It uses the siots (which contains the
 *          memory request information) to due this.
 *
 * @param   memory_info_p - Pointer of memory request information structure
 *                          to populate
 * @param   memory_request_p - Pointer to memory request that we will use to
 *                             populate memory info with
 * @param b_control_mem - Indicates if memory we are interested in
 *                        is control or data.  This can determine
 *                        the size of the pages allcated.
 * 
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      other - memory_request wasn't properly populated
 *
 * @author
 *  9/28/2010   Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_memory_init_memory_info(fbe_raid_memory_info_t *memory_info_p,
                                              fbe_memory_request_t *memory_request_p, 
                                              fbe_bool_t b_control_mem)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           page_size = 0;
    fbe_raid_memory_element_t  *master_element_p = NULL;

    /*  Calculate the page size and then validate it and the number of objects
     *  We cannot use fbe_raid_memory_validate_page_size since that is specific
     *  to siots.
     */
    page_size = fbe_raid_memory_chunk_size_to_page_size(memory_request_p->chunk_size, b_control_mem);
    switch(page_size)
    {
        case FBE_RAID_PAGE_SIZE_MIN:
        case FBE_RAID_PAGE_SIZE_STD:
        case FBE_RAID_PAGE_SIZE_MAX:
            break;

        default:
            /* Unsupported page size
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR, 
                             "raid: memory_request_p: 0x%p for: %d chunks chunk_size: %d isn't supported\n",
                             memory_request_p, memory_request_p->number_of_objects, memory_request_p->chunk_size);
            return(FBE_STATUS_GENERIC_FAILURE);
    }
    if (memory_request_p->number_of_objects > 0x2000)

    {
        /* Report the error and fail the request
         */        
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR, 
                             "raid: memory_request_p: 0x%p for chunk_size: %d chunks: %d is too large\n",
                             memory_request_p, memory_request_p->chunk_size, memory_request_p->number_of_objects);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Set the memory request pointer
     */
    fbe_raid_memory_info_set_memory_request(memory_info_p, memory_request_p);

    /* If the master header isn't valid it means the memory allocation failed
     */
    status = fbe_raid_memory_request_get_master_element(memory_request_p, &master_element_p);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR, 
                             "raid: memory_request_p: 0x%p get memory request master failed with status: 0x%x\n",
                             memory_request_p, status);
        return(status);
    }
    fbe_raid_memory_info_set_queue_head(memory_info_p, master_element_p);

    /* Set the current queue element to the first
     */
    fbe_raid_memory_info_set_current_element(memory_info_p, master_element_p);

    /* Set the bytes remaining in the current page to the page size in bytes
     */
    fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p,
                                                     (page_size * FBE_BE_BYTES_PER_BLOCK));

    /* Set the page size in bytes
     */
    fbe_raid_memory_info_set_page_size_in_bytes(memory_info_p,
                                                (page_size * FBE_BE_BYTES_PER_BLOCK));

    /* Now initialize the start of the non-buffer element to NULL
     */
    fbe_raid_memory_info_set_buffer_end_element(memory_info_p, NULL);
    fbe_raid_memory_info_set_buffer_end_bytes_remaining(memory_info_p, 0);

    /* Return success
     */
    return(FBE_STATUS_OK);
}
/******************************************
 * end fbe_raid_memory_init_memory_info()
 ******************************************/

/***************************************************************************
 * fbe_raid_memory_apply_buffer_offset()
 ***************************************************************************
 * @brief
 *  This function applies an offset of some number of blocks
 *  to the memory list.  The result is that mem_p and mem_left_p
 *  are updated to point to the given offset in the memory chain.
 *
 * @param memory_info_p - Pointer to memory request info structure
 * @param buffer_bytes_to_allocate - Number of buffer bytes to allocate
 * 
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_INSUFFICIENT_RESOURCES on failure.
 *
 * @author
 *  9/17/2009 - Created. Rob Foley
 *
 **************************************************************************/
fbe_status_t fbe_raid_memory_apply_buffer_offset(fbe_raid_memory_info_t *memory_info_p,
                                                 fbe_u32_t buffer_bytes_to_allocate)
{
    fbe_u32_t   bytes_remaining_in_page = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);
    fbe_u32_t bytes_allocated;
    fbe_status_t status = FBE_STATUS_INVALID;

    if ((bytes_remaining_in_page == 0)                                   && 
        (fbe_raid_memory_info_is_more_pages(memory_info_p) == FBE_FALSE)    )
    {
        /* No more source memory so there is nothing to copy.
         */
        if(RAID_COND(buffer_bytes_to_allocate != 0))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "%s status: buffer_bytes_to_allocate 0x%x is nonzero\n",
                                   __FUNCTION__, buffer_bytes_to_allocate);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        fbe_u32_t bytes_to_allocate;
        void *memory_p = NULL;

        /* While we have not satisfied our offset, continue looping.
         * Each iteration of the loop we will allocate another piece of this buffer list. 
         */
        while (buffer_bytes_to_allocate > 0)
        {
            /* Allocate either the amount leftover or the amount asked for, whichever 
             * is smaller.  This allows us to not waste any memory. 
             */
            if (bytes_remaining_in_page >= FBE_BE_BYTES_PER_BLOCK)
            {
                /* Allocate what we need from the remainder in the current page.
                 * Make sure it is a multiple of our block size. 
                 */
                bytes_remaining_in_page -= bytes_remaining_in_page % FBE_BE_BYTES_PER_BLOCK;
                fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, bytes_remaining_in_page);
                bytes_to_allocate = FBE_MIN(bytes_remaining_in_page, buffer_bytes_to_allocate);
            }
            else
            {
                /* Otherwise try to allocate all that we can from this page.
                 */
                bytes_to_allocate = buffer_bytes_to_allocate;
            }
            /* Apply the offset by advancing the mem_p and mem_left_p.
             */
            memory_p = fbe_raid_memory_allocate_next_buffer(memory_info_p,
                                                            bytes_to_allocate,
                                                            &bytes_allocated);
            if (memory_p == NULL)
            {
                /* We ran out of memory.
                 */
                return FBE_STATUS_INSUFFICIENT_RESOURCES;
            }

            /* Update the bytes remaining to allocate and our local bytes 
             * remaining in page.
             */
            buffer_bytes_to_allocate -= bytes_allocated;
            bytes_remaining_in_page = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);

        } /* while there are more bytes to allocate */

        status = FBE_STATUS_OK;

    } /* end else there is sufficient space */

    /* If success update the `buffer end' information
     */
    if (status == FBE_STATUS_OK)
    {
        fbe_raid_memory_info_set_buffer_end_element(memory_info_p,
                                                    fbe_raid_memory_info_get_current_element(memory_info_p));
        fbe_raid_memory_info_set_buffer_end_bytes_remaining(memory_info_p,
                                                            fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p));
    }

    return status;
}  
/* end fbe_raid_memory_apply_buffer_offset */

/***************************************************************************
 * fbe_raid_memory_apply_allocation_offset()
 ***************************************************************************
 * @brief
 *  This function applies an offset of some number of blocks
 *  to the memory list.  The result is that mem_p and mem_left_p
 *  are updated to point to the given offset in the memory chain.
 *
 * @param memory_info_p - Pointer to memory request info structure
 * @param buffer_bytes_to_allocate - Number of buffer bytes to allocate
 * @param allocation_size - Size of the allocation we are carving out.
 * 
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_INSUFFICIENT_RESOURCES on failure.
 *
 * @author
 *  2/16/2012 - Created. Rob Foley
 *
 **************************************************************************/
fbe_status_t fbe_raid_memory_apply_allocation_offset(fbe_raid_memory_info_t *memory_info_p,
                                                     fbe_u32_t number_of_allocations,
                                                     fbe_u32_t allocation_size)
{
    fbe_u32_t   bytes_remaining_in_page = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);
    fbe_u32_t buffer_bytes_to_allocate = number_of_allocations * allocation_size;
    fbe_u32_t bytes_allocated;
    fbe_status_t status = FBE_STATUS_INVALID;

    if ((bytes_remaining_in_page == 0)                                   && 
        (fbe_raid_memory_info_is_more_pages(memory_info_p) == FBE_FALSE)    )
    {
        /* No more source memory so there is nothing to copy.
         */
        if(RAID_COND(buffer_bytes_to_allocate != 0))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "%s status: buffer_bytes_to_allocate 0x%x is nonzero\n",
                                   __FUNCTION__, buffer_bytes_to_allocate);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        fbe_u32_t bytes_to_allocate;
        void *memory_p = NULL;

        /* While we have not satisfied our offset, continue looping.
         * Each iteration of the loop we will allocate another piece of this buffer list. 
         */
        while (buffer_bytes_to_allocate > 0)
        {
            /* Allocate either the amount leftover or the amount asked for, whichever 
             * is smaller.  This allows us to not waste any memory. 
             */
            if (bytes_remaining_in_page >= allocation_size)
            {
                /* Allocate what we need from the remainder in the current page.
                 * Make sure it is a multiple of our block size. 
                 */
                //bytes_remaining_in_page -= bytes_remaining_in_page % allocation_size;
                //fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, bytes_remaining_in_page);
                bytes_to_allocate = FBE_MIN(bytes_remaining_in_page, buffer_bytes_to_allocate);
                bytes_to_allocate -= bytes_to_allocate % allocation_size;
            }
            else
            {
                /* Otherwise try to allocate all that we can from this page.
                 */
                bytes_to_allocate = buffer_bytes_to_allocate;
                fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, 0);
            }
            
            /* Apply the offset by advancing the mem_p and mem_left_p.
             */
            memory_p = fbe_raid_memory_allocate_next_buffer(memory_info_p,
                                                            bytes_to_allocate,
                                                            &bytes_allocated);
            if (memory_p == NULL)
            {
                /* We ran out of memory.
                 */
                return FBE_STATUS_INSUFFICIENT_RESOURCES;
            }
            if (bytes_allocated % allocation_size)
            {
                bytes_allocated -= bytes_allocated % allocation_size;
            }

            /* Update the bytes remaining to allocate and our local bytes 
             * remaining in page.
             */
            buffer_bytes_to_allocate -= bytes_allocated;
            bytes_remaining_in_page = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);

        } /* while there are more bytes to allocate */

        status = FBE_STATUS_OK;

    } /* end else there is sufficient space */

    /* If success update the `buffer end' information
     */
    if (status == FBE_STATUS_OK)
    {
        fbe_raid_memory_info_set_buffer_end_element(memory_info_p,
                                                    fbe_raid_memory_info_get_current_element(memory_info_p));
        fbe_raid_memory_info_set_buffer_end_bytes_remaining(memory_info_p,
                                                            fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p));
    }

    return status;
}  
/* end fbe_raid_memory_apply_struct_offset */

/*****************************************************************************
 *          fbe_raid_memory_is_buffer_space_remaining()
 ***************************************************************************** 
 * 
 * @brief   This method checks if there sufficient pre-allocated buffer space
 *          remaining without over writting the other raid data structures
 *          (fruts, sgls, etc).
 *
 * @param   memory_info_p - Pointer of memory request information structure
 *                             to populate
 * @param   bytes_to_plant - Number of bytes to plant into the sg
 * 
 * @return  bool - FBE_TRUE - There are sufficient bytes remaining
 *                 FBE_FALSE - There are not sufficient bytes remaining
 *                             (i.e. something is wrong)
 *
 * @author
 *  9/29/2010   Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_memory_is_buffer_space_remaining(fbe_raid_memory_info_t *memory_info_p,
                                                     fbe_u32_t bytes_to_plant)
{
    fbe_u32_t           page_size_in_bytes = fbe_raid_memory_info_get_page_size_in_bytes(memory_info_p);
    fbe_u32_t           bytes_remaining_in_page = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);
    fbe_raid_memory_element_t *last_buffer_element_p = fbe_raid_memory_info_get_buffer_end_element(memory_info_p);
    fbe_u32_t           bytes_remaining_in_buffer_page = fbe_raid_memory_info_get_buffer_end_bytes_remaining(memory_info_p);
    fbe_u32_t           buffer_bytes_available_in_last_buffer_page;

    /* First check that we haven't exceeded the pre-allocated buffer space
     */
    buffer_bytes_available_in_last_buffer_page = (page_size_in_bytes - bytes_remaining_in_buffer_page);
    if ((fbe_raid_memory_info_get_current_element(memory_info_p) == last_buffer_element_p) &&
        (bytes_to_plant > buffer_bytes_available_in_last_buffer_page)                         )
    {
        /* The request is attempting to go beyond the buffer space
         */
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid: bytes_to_plant: %d exceeds buffer space bytes remaining: %d \n",
                             bytes_to_plant, buffer_bytes_available_in_last_buffer_page);
        return FBE_FALSE;
    }

    /* Now check if we have hit the end of all our allocated space
     */
    if ((bytes_remaining_in_page < bytes_to_plant)                       && 
        (fbe_raid_memory_info_is_more_pages(memory_info_p) == FBE_FALSE)    )
    {
        /* There is in-sufficient space
         */
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                             "raid: bytes_to_plant: %d exceeds bytes remaining: %d \n",
                             bytes_to_plant, bytes_remaining_in_page);
        return FBE_FALSE;
    }

    /* There is sufficient space
     */
    return FBE_TRUE;
}
/*************************************************
 * end fbe_raid_memory_is_buffer_space_remaining()
 *************************************************/

/*!*************************************************************************
 *          fbe_raid_siots_get_next_fruts()
 **************************************************************************
 * @brief
 *  This is called to get the next fruts of an allocation list.
 *  We make a call here to peel one fruts off the allocated list,
 *  and attach the new fruts to the passed in fruts list
 *
 * @param   siots_p - The current siots.
 * @param   head_p - The head of the fruts list to attach new fruts.
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return  fruts_p - The newly assigned fruts.
 *
 * @note
 *  It is assumed that the siots->memory_queue_head either has enough fruts
 *  to fulfil this request or will use the fruts embedded in the siots.
 *  If no fruts are available, either on the memory_queue_head chain we panic.
 *
 * @author
 *  05/20/2009  Ron Proulx - Created from rg_get_next_fruts
 *
 **************************************************************************/

fbe_raid_fruts_t *fbe_raid_siots_get_next_fruts(fbe_raid_siots_t *siots_p, 
                                                fbe_queue_head_t *head_p,
                                                fbe_raid_memory_info_t *memory_info_p)
{
    fbe_raid_fruts_t   *new_fruts_p = NULL;
    fbe_u32_t           bytes_allocated = 0;

    /* Validate siots and allocated fruts chain.
     */
    if(RAID_ERROR_COND(siots_p == NULL))
    {
        /* We do not want to intentionally fail it as it is difficult 
         * to test it through integration testing.
         */
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "siots_get_next_fruts fail with status: siots_p is NULL\n");
        return NULL;
    }

    /* Get the next piece of memory from the current location in the memory queue.
     */
    new_fruts_p = fbe_raid_memory_allocate_contiguous(memory_info_p,
                                                      sizeof(fbe_raid_fruts_t),
                                                      &bytes_allocated);

    /* Make sure we have retrieved a fruts.
     */
    if(RAID_MEMORY_COND((new_fruts_p == NULL) ||
                  (bytes_allocated != sizeof(fbe_raid_fruts_t)) ) )
    {
        /* Just fail back with a NULL.  The caller will fail the request and all memory will get freed.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots_get_next_fruts stat: new_fruts 0x%p is NULL or bytes_allocate 0x%x is wrong\n",
                                    new_fruts_p,
                                    bytes_allocated);
        return NULL;
    }

    if (new_fruts_p == NULL)
    {
        /* Something went wrong, return NULL.
         */
        return NULL;
    }

    /* Initialize the general fruts fields.
     */
    fbe_raid_fruts_common_init(&new_fruts_p->common);
    fbe_raid_common_init_flags(&new_fruts_p->common, FBE_RAID_COMMON_FLAG_TYPE_FRU_TS);
    fbe_raid_fruts_set_siots(new_fruts_p, siots_p);

    /* Mark it as uninitialized now. 
     * This is needed to keep track of when to destroy the packet.
     */
    fbe_raid_fruts_init_flags(new_fruts_p, FBE_RAID_FRUTS_FLAG_INVALID);

    /* Assign new fruts to the fruts list passed in.
     */
    fbe_raid_common_enqueue_tail(head_p, (fbe_raid_common_t*)new_fruts_p);

    if (RAID_DBG_ENABLED)
    {
        /* Increment the count of initialized fruts
         */
        siots_p->total_fruts_initialized++;
    }
    return new_fruts_p;

}
/*****************************************
 * end fbe_raid_siots_get_next_fruts()
 *****************************************/ 
/*!*************************************************************************
 *          fbe_raid_siots_get_fruts_chain()
 **************************************************************************
 * @brief
 *  This is called to get a list of fruts.
 *
 * @param   siots_p - The current siots.
 * @param   head_p - The head of the fruts list to attach new fruts.
 * @param   count  - The number of fruts to allocate.
 * @param   memory_info_p - Poiner to memory request information
 *
 * @return  fbe_bool_t FBE_TRUE if we were able to allocate all fruts.
 *                     FBE_FALSE if we were not able to allocate all fruts.
 *
 * @note
 *  It is assumed that the siots->memory_queue_head either has enough fruts
 *  to fulfil this request or will use the fruts embedded in the siots.
 *  If no fruts are available on the memory_queue_head, we return FBE_FALSE.
 *
 * @author
 *  08/04/2009  Rob Foley - Created.
 *
 **************************************************************************/

fbe_status_t fbe_raid_siots_get_fruts_chain(fbe_raid_siots_t *siots_p, 
                                         fbe_queue_head_t *head_p,
                                         fbe_u32_t count,
                                         fbe_raid_memory_info_t *memory_info_p)
{
    fbe_raid_fruts_t *fruts_p = NULL;

    /* Validate siots and allocated fruts chain.
     */
    
    if (RAID_COND( (siots_p == NULL) ||
                  (!fbe_raid_memory_info_is_memory_left(memory_info_p)) ) )
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_p 0x%p is NULL or current element: 0x%p empty \n",
                             siots_p, fbe_raid_memory_info_get_current_element(memory_info_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Try to allocate all fruts.
     */
    while ((count > 0)                                        && 
           fbe_raid_memory_info_is_memory_left(memory_info_p)    )
    {
        fruts_p = fbe_raid_siots_get_next_fruts(siots_p, head_p, memory_info_p);

        if (fruts_p == NULL)
        {
            /* Ran out of fruts unexpectedly.
             */
            fbe_raid_memory_info_trace(memory_info_p, FBE_RAID_MEMORY_INFO_TRACE_PARAMS_ERROR, 
                                 "raid: get next fruts failed with count: %d fruts remaining \n",
                                 count);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        count--;
    }

    if (count > 0)
    {
        /* Not able to allocate all fruts.
         */
        fbe_raid_memory_info_trace(memory_info_p, FBE_RAID_MEMORY_INFO_TRACE_PARAMS_ERROR, 
                                   "raid: count non-zero: %d fruts remaining \n",
                                   count);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        /* Allocated all fruts successfully.
         */
        return FBE_STATUS_OK;
    }
}
/*****************************************
 * end fbe_raid_siots_get_fruts_chain()
 *****************************************/ 


void fbe_raid_group_memory_allocate_synchronous_completion(fbe_memory_request_t * memory_request, 
                                                           fbe_memory_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
}
void * fbe_raid_group_memory_allocate_synchronous(void)
{
    fbe_memory_request_t memory_request;
    fbe_semaphore_t sem;

    fbe_semaphore_init(&sem, 0, 1);

    memory_request.request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_BUFFER;
    memory_request.number_of_objects = 1;
    memory_request.ptr = NULL;
    memory_request.completion_function = fbe_raid_group_memory_allocate_synchronous_completion;
    memory_request.completion_context = &sem;
    fbe_memory_allocate(&memory_request);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return memory_request.ptr;
}

/*!**************************************************************************
 *          fbe_raid_memory_get_first_block_buffer()
 ****************************************************************************
 *
 * @brief   This method returns the address of the first block for the sgl
 *          passed.
 *
 * @param   sgl_p - Pointer to first element of the sgl
 * 
 * @return  buffer_p - Address of first block of the request.
 *
 * @author
 *  03/16/2010  Ron Proulx  - Created
 *
 **************************************************************************/
fbe_u8_t *fbe_raid_memory_get_first_block_buffer(fbe_sg_element_t *sgl_p)
{
    /* Simply return the address field of the sg.
     */
    if (sgl_p == NULL)
    {
        return(NULL);
    }
    return(fbe_sg_element_address(sgl_p));
}
/* end of fbe_raid_memory_get_first_block_buffer() */

/*!**************************************************************************
 *          fbe_raid_memory_get_last_block_buffer()
 ****************************************************************************
 *
 * @brief   This method returns the address of the last block for the sgl
 *          passed.
 *
 * @param   sgl_p - Pointer to first element of the sgl
 * 
 * @return  buffer_p - Address of last block of the request.
 *
 * @author
 *  03/16/2010  Ron Proulx  - Created
 *
 **************************************************************************/
fbe_u8_t *fbe_raid_memory_get_last_block_buffer(fbe_sg_element_t *sgl_p)
{
    fbe_sg_element_t   *prev_sgl_p;
    fbe_u8_t           *buffer_p;

    /* Find the terminator
     */
    if (sgl_p == NULL)
    {
        return(NULL);
    }

    /* Locate the terminator.
     */
    while (fbe_sg_element_count(sgl_p) != 0)
    {
        prev_sgl_p = sgl_p;
        sgl_p++;
    }
    if ((prev_sgl_p == NULL)                                               ||
        ((fbe_sg_element_count(prev_sgl_p) % FBE_BE_BYTES_PER_BLOCK) != 0)    )
    {
        return(NULL);
    }

    /* Get the address of the last block.
     */
    buffer_p = fbe_sg_element_address(prev_sgl_p) + fbe_sg_element_count(prev_sgl_p);
    buffer_p -= FBE_BE_BYTES_PER_BLOCK;
    return(buffer_p);
}
/* end of fbe_raid_memory_get_last_block_buffer() */

/*!***************************************************************************
 *          fbe_raid_memory_pointer_to_u32()                              
 *****************************************************************************
 *
 * @brief
 *    This function converts (and thus may truncate) a pointer to a 32-bit
 *    quantity.
 *
 * @param
 *     void * - Pointer 
 *
 * @return
 *     UINT_32  - 32-bit (truncated if neccessary) pointer value
 *
 * @note
 *     This function will truncate 64-bit pointer to the lower 32-bits
 *
 * @author
 *     06/04/2009   Ron Proulx - Created
 *
 *************************************************************************/
fbe_u32_t fbe_raid_memory_pointer_to_u32(void *ptr)
{
    fbe_u64_t temp;
    
#if !defined(_WIN64)
    temp = (fbe_u32_t)ptr;
#else
    temp = (fbe_u64_t)ptr;
#endif
    return((fbe_u32_t)temp);
}
/* end of fbe_raid_memory_pointer_to_u32() */

/*************************
 * end file fbe_raid_memory.c
 *************************/


