/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_memory_mock.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the memory mocks.
 *
 * @version
 *   8/6/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_random.h"
#include "fbe_raid_library_proto.h"

/*************************
 *   LITERAL DEFINITIONS
 *************************/
#define FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_CRITICAL FBE_TRACE_LEVEL_CRITICAL_ERROR, __LINE__, __FUNCTION__ 
#define FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_ERROR FBE_TRACE_LEVEL_ERROR, __LINE__, __FUNCTION__ 
#define FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_INFO FBE_TRACE_LEVEL_INFO, __LINE__, __FUNCTION__
#define FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_DEBUG_THREAD FBE_TRACE_LEVEL_DEBUG_LOW, __LINE__, __FUNCTION__
#define FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_DEBUG_REQUEST FBE_TRACE_LEVEL_DEBUG_HIGH, __LINE__, __FUNCTION__
#define FBE_RAID_MEMORY_MOCK_ADDITONAL_DELAY_MS (1 * 1000) /*! @note Change this to (10 * 1000) to test memory aborts */

/*************************
 *   STRUCTURE DEFINITIONS
 *************************/
typedef enum fbe_raid_memory_mock_thread_state_e
{
    FBE_RAID_MEMORY_THREAD_STATE_UNINITIALIZED = 0,
    FBE_RAID_MEMORY_THREAD_STATE_INITIALIZED,
    FBE_RAID_MEMORY_THREAD_STATE_RUNNING,
    FBE_RAID_MEMORY_THREAD_STATE_DELAYING,
    FBE_RAID_MEMORY_THREAD_STATE_DELAYING_ADDITIONAL_DELAY,
    FBE_RAID_MEMORY_THREAD_STATE_DESTROY_REQUESTED,
    FBE_RAID_MEMORY_THREAD_STATE_DESTROYED,
} fbe_raid_memory_mock_thread_state_t;

/*******************************
 *   LOCAL FUNCTION DEFINITIONS
 *******************************/
static fbe_status_t fbe_raid_memory_mock_request_entry(fbe_memory_request_t *memory_request_p);
static fbe_status_t fbe_raid_memory_mock_abort_request(fbe_memory_request_t *memory_request_p);
static fbe_status_t fbe_raid_memory_mock_free_entry(fbe_memory_ptr_t memory_p);

/*************************
 *   LOCAL GLOBALS
 *************************/
/*! @note Since no global data is used there is no need for a lock when
 *        allocating memory.  But we want to mimic the timing of dps and
 *        in addition we need a lock for our tracking structures.
 */
static fbe_spinlock_t   fbe_raid_memory_mock_memory_lock;
static fbe_thread_t     fbe_raid_memory_mock_thread;
static fbe_raid_memory_mock_thread_state_t fbe_raid_memory_mock_thread_state = FBE_RAID_MEMORY_THREAD_STATE_UNINITIALIZED; 
static fbe_semaphore_t  fbe_raid_memory_mock_semaphore;
static fbe_semaphore_t  fbe_raid_memory_mock_defer_delay_semaphore;
static fbe_u32_t        fbe_raid_memory_mock_percent_to_defer = 10;
static fbe_u32_t        fbe_raid_memory_mock_max_delay_ms = 100;
static fbe_time_t       fbe_raid_memory_mock_additional_delay = 0;
static fbe_queue_head_t fbe_raid_memory_mock_request_queue_head;
/*! Change the default below to make raid use the mock memory service
 *  which uses the native memory vs dps.
 */
static fbe_bool_t       fbe_raid_memory_mock_use_mocks = FBE_FALSE;
static fbe_bool_t       fbe_raid_memory_mock_allow_deferred_allocations = FBE_TRUE;
static fbe_raid_memory_mock_error_callback_t fbe_raid_memory_mock_error_callback = NULL;
static fbe_raid_memory_mock_allocate_mock_t fbe_raid_memory_mock_allocate_mock = NULL;
static fbe_raid_memory_mock_abort_mock_t fbe_raid_memory_mock_abort_mock = NULL;
static fbe_raid_memory_mock_free_mock_t fbe_raid_memory_mock_free_mock = NULL;

/*!***************************************************************************
 *          fbe_raid_memory_mock_api_allocate()
 *****************************************************************************
 *
 * @brief   This is raid's mock memory service allocate api.
 *
 * @param   memory_request_p - Pointer to memory request for allocation
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 *****************************************************************************/
fbe_status_t fbe_raid_memory_mock_api_allocate(fbe_memory_request_t *memory_request_p)
{
    if (fbe_raid_memory_mock_allocate_mock == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: fbe_raid_memory_mock_allocate_mock is NULL \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return(fbe_raid_memory_mock_allocate_mock(memory_request_p));
}
/*******************************************
 * end fbe_raid_memory_mock_api_allocate()
 *******************************************/

/*!***************************************************************************
 *          fbe_raid_memory_mock_api_abort()
 *****************************************************************************
 *
 * @brief   This is raid's mock memory service abort api.
 *
 * @param   memory_request_p - Pointer to memory request to abort
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 *****************************************************************************/
fbe_status_t fbe_raid_memory_mock_api_abort(fbe_memory_request_t *memory_request_p)
{
    if (fbe_raid_memory_mock_abort_mock == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: fbe_raid_memory_mock_abort_mock is NULL \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return(fbe_raid_memory_mock_abort_mock(memory_request_p));
}
/*******************************************
 * end fbe_raid_memory_mock_api_abort()
 *******************************************/

/*!***************************************************************************
 *          fbe_raid_memory_mock_api_free()
 *****************************************************************************
 *
 * @brief   This is raid's mock memory service free api.
 *
 * @param   master_memory_header_p - Pointer to master memory header to free
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 *****************************************************************************/
fbe_status_t fbe_raid_memory_mock_api_free(fbe_memory_header_t *master_memory_header_p)
{
    if (fbe_raid_memory_mock_free_mock == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: fbe_raid_memory_mock_free_mock is NULL \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return(fbe_raid_memory_mock_free_mock(master_memory_header_p));
}
/*******************************************
 * end fbe_raid_memory_mock_api_free()
 *******************************************/

/*!***************************************************************************
 *          fbe_raid_memory_mock_is_mock_enabled()
 *****************************************************************************
 *
 * @brief   Simply returns if mocks are enabled or not.
 *
 * @param   None
 *
 * @return  bool - FBE_TRUE - Mocks are enabled
 *                 FBE_FALSE - Mocks are not enabled
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_memory_mock_is_mock_enabled(void)
{
    return(fbe_raid_memory_mock_use_mocks);
}
/*******************************************
 * end fbe_raid_memory_mock_is_mock_enabled()
 *******************************************/

/*!***************************************************************************
 *          fbe_raid_memory_mock_configure_mocks()
 *****************************************************************************
 *
 * @brief   This method allows the caller (typically a unit test) to configure
 *          the raid memory mock service.  This includes enabling/disabling the
 *          use of the raid mock memory service, enabling/disabling deferred
 *          memory allocations in the raid mock memory service and pass a
 *          error callback method that is invoke when an un-expected error
 *          occurs.
 *
 * @param   b_enable_mock_memory - FBE_TRUE - Use the raid memory mock service
 *                                            instead if the fbe memory service
 *                                 FBE_FALSE - Use the default memory service
 * @param   b_allow_deferred_allocations - FBE_TRUE - raid memory mock service
 *                                                    will defer a certain percent
 *                                                    of allocation for 0 - x ms.
 *                                         FBE_FALSE - Memory is always immediately
 *                                                     available
 * @param   client_mock_allocate - If specified (i.e. not NULL) use this as the memory
 *                          allocate function
 * @param   client_mock_abort - If specified (i.e. not NULL) use this as the memory abort
 *                       function
 * @param   client_mock_free - If specified (i.e. not NULL) use this as the memory free
 *                       function
 * @param   client_error_callback - Optional error method to invoke when the mock memory
 *                           service encounters an un-expected error (NULL not allowed)
 *
 * @return  status - Typically FBE_STATUS_OK
 *                   Other - If this method is invoked after the raid memory has
 *                           been initialized it is an error (i.e. cannot re-configure
 *                           the mock memory service).
 *
 * @author
 *  10/27/2010  Ron Proulx  - Created  
 *
 *****************************************************************************/
fbe_status_t fbe_raid_memory_mock_configure_mocks(fbe_bool_t b_enable_mock_memory,                      
                                                  fbe_bool_t b_allow_deferred_allocations,              
                                                  fbe_raid_memory_mock_allocate_mock_t client_mock_allocate,   
                                                  fbe_raid_memory_mock_abort_mock_t client_mock_abort,         
                                                  fbe_raid_memory_mock_free_mock_t client_mock_free,           
                                                  fbe_raid_memory_mock_error_callback_t client_error_callback)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* If the raid memory has already been initialized return an error
     */
    if (fbe_raid_memory_api_is_initialized() == FBE_TRUE)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: Attempted to configure raid mock memory service after initialization \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The error callback cannot be NULL
     */
    if (client_error_callback == NULL)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: value for error_callback must not be NULL \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Simply save the configuration and trace the fact that it was configured
     */
    fbe_raid_memory_mock_use_mocks = b_enable_mock_memory;                          
    fbe_raid_memory_mock_allow_deferred_allocations = b_allow_deferred_allocations;    
    fbe_raid_memory_mock_error_callback = client_error_callback;
    if (client_mock_allocate != NULL)
    {
        fbe_raid_memory_mock_allocate_mock = client_mock_allocate;
    }
    else
    {
        fbe_raid_memory_mock_allocate_mock = fbe_raid_memory_mock_request_entry;
    }
    if (client_mock_abort != NULL)
    {
        fbe_raid_memory_mock_abort_mock = client_mock_abort;
    }
    else
    {
        fbe_raid_memory_mock_abort_mock = fbe_raid_memory_mock_abort_request;
    }
    if (client_mock_free != NULL)
    {
        fbe_raid_memory_mock_free_mock = client_mock_free;
    }
    else
    {
        fbe_raid_memory_mock_free_mock = fbe_raid_memory_mock_free_entry;
    }
    fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                 "raid: Configured mock memory. enable mocks: %d allow deferred: %d error callback: 0x%p\n",
                                 fbe_raid_memory_mock_use_mocks, fbe_raid_memory_mock_allow_deferred_allocations,
                                 fbe_raid_memory_mock_error_callback);

    return status;
}
/*******************************************
 * end fbe_raid_memory_mock_configure_mocks()
 *******************************************/

/*!**************************************************************
 * fbe_raid_memory_mock_trace()
 ****************************************************************
 * @brief
 *  Trace memory request information
 * 
 * @param memory_request_p - Pointer to memory request information
 * @param line - The line number where failure occurred.
 * @param function_p - File of failure.
 * @param fail_string_p - This is the string describing the failure.
 *
 * @return - None.
 *
 *
 ****************************************************************/
static void fbe_raid_memory_mock_trace(fbe_memory_request_t *const memory_request_p,
                                       fbe_trace_level_t trace_level,
                                       fbe_u32_t line,
                                       const char * function_p,
                                       const char * fail_string_p, ...)
                                       __attribute__((format(__printf_func__,5,6)));

static void fbe_raid_memory_mock_trace(fbe_memory_request_t *const memory_request_p,
                                       fbe_trace_level_t trace_level,
                                       fbe_u32_t line,
                                       const char * function_p,
                                       const char * fail_string_p, ...)
{
    char buffer[FBE_RAID_MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;

    /* Get the extra string
     */
    if (fail_string_p != NULL)
    {
        va_start(argList, fail_string_p);
        num_chars_in_string = _vsnprintf(buffer, FBE_RAID_MAX_TRACE_CHARS, fail_string_p, argList);
        buffer[FBE_RAID_MAX_TRACE_CHARS - 1] = '\0';
        va_end(argList);
    }

    if (memory_request_p != NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: memory mock memory_request_p: 0x%p state: %d chunk_size: %d num of objects: %d\n", 
                           memory_request_p, memory_request_p->request_state,
                           memory_request_p->chunk_size, memory_request_p->number_of_objects);
    }
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                           "raid: memory mock line: %d function: %s\n", line, function_p);

    /* Display the string, if it has some characters.
     */
    if (num_chars_in_string > 0)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                               buffer);
    }

    /* If we are running MUT and the trace level is error or less
     * invoke the error callback
     */
    if ((fbe_raid_memory_mock_error_callback != NULL) &&
        (trace_level <= FBE_TRACE_LEVEL_ERROR)           )           
    {
        fbe_raid_memory_mock_error_callback();
    }

    return;
}
/**************************************
 * end fbe_raid_memory_mock_trace()
 **************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_queue_element_to_header()
 ***************************************************************************** 
 * 
 * @brief   Converts from a queue element to the master memory header
 *  
 * @param   queue_element - Pointer queue element to retrieve master header from
 *  
 * @return  None
 *
 *****************************************************************************/
static fbe_memory_header_t *fbe_raid_memory_mock_queue_element_to_header(fbe_queue_element_t * queue_element)
{
    fbe_memory_header_t * memory_header;
    memory_header = (fbe_memory_header_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_memory_header_t  *)0)->u.queue_element));
    return memory_header;
}
/*******************************************************
 * end of fbe_raid_memory_mock_queue_element_to_header()
 *******************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_queue_element_to_request()
 ***************************************************************************** 
 * 
 * @brief   Converts from a queue element to the memory request
 *  
 * @param   queue_element - Pointer queue element to the master request
 *  
 * @return  None
 *
 *****************************************************************************/
static fbe_memory_request_t *fbe_raid_memory_mock_queue_element_to_request(fbe_queue_element_t * queue_element)
{
    fbe_memory_request_t * memory_request;
    memory_request = (fbe_memory_request_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_memory_request_t *)0)->queue_element));
    return memory_request;
}
/*******************************************************
 * end of fbe_raid_memory_mock_queue_element_to_request()
 *******************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_process_malloc_request()
 ***************************************************************************** 
 * 
 * @brief   Changes the default memory allocation/free from memory service to
 *          raid's `mock' (a.k.a. native allocate/free) memory service.
 *  
 * @param   memory_request - Pointer to memory request to allocate
 *  
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  10/18/2010  Ron Proulx  - Created
 * 
 *****************************************************************************/
static fbe_status_t fbe_raid_memory_mock_process_malloc_request(fbe_memory_request_t * memory_request)
{
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_memory_header_t * prev_memory_header = NULL;
    fbe_memory_header_t * current_memory_header = NULL;
    fbe_u32_t i = 0;
    
    /* Default is null */
    memory_request->ptr = NULL;

    /* Get get chunk for the master header (i.e. first chunk) */
    master_memory_header = (fbe_memory_header_t *)fbe_memory_native_allocate(memory_request->chunk_size);
    if (master_memory_header == NULL) {
        fbe_raid_memory_mock_trace(memory_request, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_ERROR,
                                   "raid: memory mock: memory_request: 0x%p failed to allocate: %d bytes for master chunk\n",
                                   memory_request, memory_request->chunk_size);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    fbe_raid_memory_mock_trace(memory_request, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_DEBUG_REQUEST, 
                               "raid: memory mock: allocated chunk: 0x%p index: %d for %d bytes \n",
                               master_memory_header, i, master_memory_header->memory_chunk_size);
    memory_request->ptr = master_memory_header;
    /* Fill master header */
    master_memory_header->magic_number = FBE_MEMORY_HEADER_MAGIC_NUMBER;
    master_memory_header->memory_chunk_size = memory_request->chunk_size; 
    master_memory_header->number_of_chunks = memory_request->number_of_objects; 
    master_memory_header->u.hptr.master_header = master_memory_header; 
    master_memory_header->u.hptr.next_header = NULL;
    master_memory_header->data = (fbe_u8_t *)master_memory_header + FBE_MEMORY_HEADER_SIZE;

    /* Now get any addiitonal chunks */
    prev_memory_header = master_memory_header;
    for(i = 1; i < memory_request->number_of_objects; i++){
        current_memory_header = (fbe_memory_header_t *)fbe_memory_native_allocate(memory_request->chunk_size);
        if (current_memory_header == NULL) {
            fbe_raid_memory_mock_trace(memory_request, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_ERROR,
                                   "raid: memory mock: memory_request: 0x%p failed to allocate: %d bytes for chunk index: %d\n",
                                   memory_request, memory_request->chunk_size, i);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_raid_memory_mock_trace(memory_request, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_DEBUG_REQUEST, 
                                   "raid: memory mock: allocated chunk: 0x%p index: %d for %d bytes \n",
                                   current_memory_header, i, master_memory_header->memory_chunk_size);
        prev_memory_header->u.hptr.next_header = current_memory_header;
        current_memory_header->magic_number = FBE_MEMORY_HEADER_MAGIC_NUMBER;
        current_memory_header->memory_chunk_size = 0; 
        current_memory_header->number_of_chunks = 0; 
        current_memory_header->u.hptr.master_header = master_memory_header; 
        current_memory_header->u.hptr.next_header = NULL;
        current_memory_header->data = (fbe_u8_t *)current_memory_header + FBE_MEMORY_HEADER_SIZE;
        prev_memory_header = current_memory_header;
    }

    /* Made it this far, success
     */
    return FBE_STATUS_OK;
}
/******************************************************
 * end of fbe_raid_memory_mock_process_malloc_request()
 ******************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_dispatch_queue()
 ***************************************************************************** 
 * 
 * @brief   Changes the default memory allocation/free from memory service to
 *          raid's `mock' (a.k.a. native allocate/free) memory service.
 *  
 * @param   b_use_mut_trace - FBE_TRUE - Use MUT to trace errors
 *  
 * @return  None
 *
 * @author
 *  10/25/2010  Ron Proulx  - Created from memory_dps_dispatch_queue
 * 
 *****************************************************************************/
static void fbe_raid_memory_mock_dispatch_queue(void)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_memory_request_t   *memory_request;

    /* If we are no longer running don't continue
     */
    if (fbe_raid_memory_mock_thread_state != FBE_RAID_MEMORY_THREAD_STATE_RUNNING)
    {
        return;
    }

    /* Take the lock and process all pending allocaiton requests
     */
    fbe_spinlock_lock(&fbe_raid_memory_mock_memory_lock);

    while(!fbe_queue_is_empty(&fbe_raid_memory_mock_request_queue_head)) 
    {
        memory_request = (fbe_memory_request_t *)fbe_queue_front(&fbe_raid_memory_mock_request_queue_head);
        fbe_queue_pop(&fbe_raid_memory_mock_request_queue_head);
        status = fbe_raid_memory_mock_process_malloc_request(memory_request);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_memory_mock_trace(memory_request, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_CRITICAL,
                                       "raid: memory mock: memory_request: 0x%p failed to allocate: %d bytes \n",
                                       memory_request, memory_request->chunk_size);
            break;
        }
        memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED;

        /* Else if the status was ok log the request in the tracking structure
         */
        fbe_raid_memory_track_deferred_allocation(memory_request);

        /* Now unlock and invoke the completion method
         */
        fbe_spinlock_unlock(&fbe_raid_memory_mock_memory_lock);
        memory_request->completion_function(memory_request, memory_request->completion_context);

        /*! Now re-take the lock to process the next request 
         * 
         *  @note Currently there is no limit to how many request we process
         */ 
        fbe_spinlock_lock(&fbe_raid_memory_mock_memory_lock);
    }

    /* Only when the entire queue is empty do we return
     */
    fbe_spinlock_unlock(&fbe_raid_memory_mock_memory_lock);    
}
/*******************************************************
 * end of fbe_raid_memory_mock_dispatch_queue()
 *******************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_get_defer_allocation_delay()
 ***************************************************************************** 
 * 
 * @brief   Determines a wait time in milliseconds between processing of
 *          deferred requests.  Once the time expires, any and all 
 *  
 * @param   None
 *  
 * @return  bool - FBE_TRUE - Start the memory request immediately
 *                 FBE_FALSE - This is a deferred memory re
 * 
 * @note    This method currently doesn't support deferred allocations
 *
 * @author
 *  10/25/2010  Ron Proulx  - Created
 * 
 *****************************************************************************/
fbe_status_t fbe_raid_memory_mock_get_defer_allocation_delay(fbe_time_t *random_delay_timeout_p)
{
    fbe_u64_t       random = 0;

    /* Generate a random number between 0 and (max delay) ms
     */
    random = fbe_random_64() % fbe_raid_memory_mock_max_delay_ms;
    *random_delay_timeout_p = (fbe_time_t)random;

    return(FBE_STATUS_OK);
}
/*************************************************************
 * end of fbe_raid_memory_mock_get_defer_allocation_delay()
 *************************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_validate_aborted_requests()
 ***************************************************************************** 
 * 
 * @brief   Validates that any pending request(s) that are waiting for memory
 *          and are marked quiesce have been aborted.
 *  
 * @param   None
 *  
 * @return  bool - FBE_TRUE - Start the memory request immediately
 *                 FBE_FALSE - This is a deferred memory re
 * 
 * @note    This method currently doesn't support deferred allocations
 *
 * @author
 *  10/25/2010  Ron Proulx  - Created
 * 
 *****************************************************************************/
static void fbe_raid_memory_mock_validate_aborted_requests(void)
{
    fbe_u32_t               queue_length = 0;
    fbe_queue_element_t    *current_element_p = NULL;
    fbe_memory_request_t   *memory_request_p = NULL;
    fbe_raid_common_t      *common_p = NULL;
    fbe_raid_common_flags_t common_type = 0;
    fbe_raid_iots_t        *iots_p = NULL;

    /* Need to take the lock
     */
    fbe_spinlock_lock(&fbe_raid_memory_mock_memory_lock);

    /*  Currently we find any request that are waiting for memory and a quiesce
     *  has been requested.  By this time there shouldn't be any.
     */
    queue_length = fbe_queue_length(&fbe_raid_memory_mock_request_queue_head);
    current_element_p = fbe_queue_front(&fbe_raid_memory_mock_request_queue_head);
    while(current_element_p != NULL)
    {
        /* Get the completion context
         */
        memory_request_p = fbe_raid_memory_mock_queue_element_to_request(current_element_p);
        common_p = (fbe_raid_common_t *)memory_request_p->completion_context;
        common_type = fbe_raid_common_get_flags(common_p) & FBE_RAID_COMMON_FLAG_STRUCT_TYPE_MASK;
        if (fbe_raid_common_is_flag_set(common_p, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY)) 
        {
            if (common_type == FBE_RAID_COMMON_FLAG_TYPE_IOTS)
            {
                iots_p = (fbe_raid_iots_t *)common_p;
            }
            else
            {
                iots_p = (fbe_raid_iots_t *)fbe_raid_common_get_parent(common_p);
                if (common_type == FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS)
                {
                    iots_p = (fbe_raid_iots_t *)fbe_raid_common_get_parent(&iots_p->common);
                }
            }

            /* If the quiesce or abort flag is set then the request wasn't properly aborted
             */
            if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE) ||
                fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ABORT)      )
            {
                fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_ERROR,
                                   "raid: memory mock: iots_p: 0x%p flags: 0x%x common_p: 0x%p flags: 0x%x not aborted\n",
                                   iots_p, fbe_raid_iots_get_flags(iots_p),
                                   common_p, fbe_raid_common_get_flags(common_p));
            }
        }

        current_element_p = fbe_queue_next(&fbe_raid_memory_mock_request_queue_head, current_element_p); 
    }

    /* Release the lock
     */
    fbe_spinlock_unlock(&fbe_raid_memory_mock_memory_lock);

    return;
}
/*************************************************************
 * end of fbe_raid_memory_mock_validate_aborted_requests()
 *************************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_thread_func()
 ***************************************************************************** 
 * 
 * @brief   Changes the default memory allocation/free from memory service to
 *          raid's `mock' (a.k.a. native allocate/free) memory service.
 *  
 * @param   context - Pointer to context for thread startup
 *  
 * @return  None
 *
 * @author
 *  10/25/2010  Ron Proulx  - Created from memory_dps_thread_func
 * 
 *****************************************************************************/
static void fbe_raid_memory_mock_thread_func(void * context)
{
    EMCPAL_STATUS        nt_status;
    fbe_status_t    status;
    fbe_time_t      random_delay_timeout = 0;
    fbe_u32_t       queue_length = 0;

    FBE_UNREFERENCED_PARAMETER(context);

    fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                 "raid: memory mock: starting deferred allocation thread. percent to defer: %d\n",
                                 fbe_raid_memory_mock_percent_to_defer);

    /* Set thread state to running
     */
    fbe_raid_memory_mock_thread_state = FBE_RAID_MEMORY_THREAD_STATE_RUNNING;
    while(1)    
    {
        /* Wait to be signalled by free thread
         */
        nt_status = fbe_semaphore_wait(&fbe_raid_memory_mock_semaphore, NULL);

        /* Now wait a random delay.  We allow for a delay of 0 and
         * fbe_semaphore_wait_ms doesn't handles a delay of 0.  
         * But fbe_semaphore_wait_ms can return either FBE_STATUS_OK or 
         * FBE_STATUS_TIMEOUT.
         */
        fbe_raid_memory_mock_get_defer_allocation_delay(&random_delay_timeout);

        /*  For debug and to determine if we need to delay or not get the queue length
         */
        queue_length = fbe_queue_length(&fbe_raid_memory_mock_request_queue_head);
        fbe_raid_memory_mock_trace(NULL, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_DEBUG_THREAD,
                                   "raid: memory mock: enter with %d deferred requests delay: %llu ms\n",
                                   queue_length,
				   (unsigned long long)random_delay_timeout);

        /* Only for non-zero delays do we wait
         */
        if ((queue_length > 0) && (random_delay_timeout > 0))
        {
            /* It is ok to over-write the state here since we shouldn't be 
             * destroying while we are deferring memory allocations.
             */
            fbe_raid_memory_mock_thread_state = FBE_RAID_MEMORY_THREAD_STATE_DELAYING;
            status = fbe_semaphore_wait_ms(&fbe_raid_memory_mock_defer_delay_semaphore, (fbe_u32_t)random_delay_timeout);

            /* Validate that the status is success or timeout
             */
            if ((status != FBE_STATUS_TIMEOUT) &&
                (status != FBE_STATUS_OK)         )
            {
                fbe_raid_memory_mock_trace(NULL, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_CRITICAL,
                                       "raid: memory mock: defered wait ms: %llu failed with status: 0x%x\n",
                                       (unsigned long long)random_delay_timeout,
				       status);
            }

            /* If the `additional' delay is set then delay a second time
             */
            if (fbe_raid_memory_mock_additional_delay != 0)
            {
                fbe_raid_memory_mock_thread_state = FBE_RAID_MEMORY_THREAD_STATE_DELAYING_ADDITIONAL_DELAY;
                fbe_semaphore_wait_ms(&fbe_raid_memory_mock_defer_delay_semaphore, (fbe_u32_t)fbe_raid_memory_mock_additional_delay);
                queue_length = fbe_queue_length(&fbe_raid_memory_mock_request_queue_head);
                fbe_raid_memory_mock_trace(NULL, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_INFO,
                                   "raid: memory mock: completed additional delay of: %llu seconds with: %d items waiting\n",
                                   (unsigned long long)(fbe_raid_memory_mock_additional_delay / 1000), queue_length);
                fbe_raid_memory_mock_additional_delay = 0;
                fbe_raid_memory_mock_validate_aborted_requests();
            }
            
            /* Change the thread state back to running
             */
            fbe_raid_memory_mock_thread_state = FBE_RAID_MEMORY_THREAD_STATE_RUNNING;
        }

        /* Unfortunately the only way to exit is to look at the thread state
         */
        if (fbe_raid_memory_mock_thread_state != FBE_RAID_MEMORY_THREAD_STATE_RUNNING)
        {
            break;
        }

        /* Sanity check that we setup the thread
         */
        if (fbe_raid_memory_mock_allow_deferred_allocations == FBE_TRUE)
        {
            fbe_raid_memory_mock_dispatch_queue();
        }
        else
        {
            /* Else something is seriously wrong
             */
            fbe_raid_memory_mock_trace(NULL, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_CRITICAL,
                                       "raid: deferred allocations allowed: %d not expected\n",
                                       fbe_raid_memory_mock_allow_deferred_allocations);
        }

        /*! @note Remove call when raid mock memory service is functional
         */
        {
            fbe_u32_t   queue_length = fbe_queue_length(&fbe_raid_memory_mock_request_queue_head);
            fbe_raid_memory_mock_trace(NULL, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_DEBUG_THREAD,
                                       "raid: memory mock: exit with %d deferred requests\n",
                                       queue_length);
        }

    }

    /*! Since all the resources are global, all the resources
     *  are cleanup up in fbe_raid_memory_mock_destroy_mocks()
     */
    fbe_raid_memory_mock_thread_state = FBE_RAID_MEMORY_THREAD_STATE_DESTROYED;

    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
/*******************************************
 * end of fbe_raid_memory_mock_thread_func()
 *******************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_should_start_request()
 ***************************************************************************** 
 * 
 * @brief   Detemines fo we should start the memory request immediately or not.
 *          Based on the settings of:
 *          o   fbe_raid_memory_mock_allow_deferred_allocations
 *          o   fbe_raid_memory_mock_percent_deferred
 *  
 * @param   None
 *  
 * @return  bool - FBE_TRUE - Start the memory request immediately
 *                 FBE_FALSE - This is a deferred memory request that
 *                            will be started later
 * 
 * @note    This method currently doesn't support deferred allocations
 *
 * @author
 *  10/25/2010  Ron Proulx  - Created
 * 
 *****************************************************************************/
static fbe_bool_t fbe_raid_memory_mock_should_start_request(void)
{
    fbe_bool_t  b_start_immediately = FBE_TRUE;
    fbe_u32_t   mod_percent;
    fbe_u32_t   random = 0;

    /* Only need to calaculated if we should defer or if enabled
     */
    if (fbe_raid_memory_mock_allow_deferred_allocations == FBE_TRUE)
    {
        /* Get a random number and mod it by percent*/
        mod_percent = (fbe_raid_memory_mock_percent_to_defer > 0) ? fbe_raid_memory_mock_percent_to_defer : 1;
        random = fbe_random() % 100;
        b_start_immediately = (random < mod_percent) ? FBE_FALSE : FBE_TRUE;
    }

   /* Return whether the request should be started immediately or not
    */
   return(b_start_immediately);
}
/***************************************************
 * end of fbe_raid_memory_mock_should_start_request()
 ***************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_defer_request()
 ***************************************************************************** 
 * 
 * @brief   Defers the memory request using the links provided in the memory
 *          request.
 *  
 * @param   memory_request_p - Pointer to memory request to allocate
 *  
 * @return  status - Typically FBE_STATUS_PENDING
 * 
 * @author
 *  10/25/2010  Ron Proulx  - Created
 * 
 *****************************************************************************/
static fbe_status_t fbe_raid_memory_mock_defer_request(fbe_memory_request_t *memory_request_p)
{
    fbe_status_t        status = FBE_STATUS_PENDING;
    fbe_u32_t           queue_length = 0;
    fbe_queue_element_t *queue_element_p = &memory_request_p->queue_element;
    fbe_queue_element_t *current_element_p = NULL;

    /* Validate that the element isn't on a queue
     */
    queue_length = fbe_queue_length(&fbe_raid_memory_mock_request_queue_head);
    current_element_p = fbe_queue_front(&fbe_raid_memory_mock_request_queue_head);
    if ((queue_element_p->next != NULL)                  &&
        (fbe_queue_is_element_on_queue(queue_element_p))    )
    {
        fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_ERROR,
                                   "raid: memory request is already on a queue. deferred queue head: 0x%p length: %d\n",
                                   current_element_p, queue_length);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now validate that the item isn't already on the queue
     */
    while (current_element_p != NULL)
    {
        if (current_element_p == queue_element_p)
        {
            fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_ERROR,
                                   "raid: memory request is already on deferred queue. head: 0x%p length: %d\n",
                                   current_element_p, queue_length);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        current_element_p = fbe_queue_next(&fbe_raid_memory_mock_request_queue_head, current_element_p); 
    }

    /* Insert the item onto the queue
     */
    fbe_queue_push(&fbe_raid_memory_mock_request_queue_head, queue_element_p);

    /* Always return the execution status
     */
    return status;
}
/***************************************************
 * end of fbe_raid_memory_mock_defer_request()
 ***************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_request_entry()
 ***************************************************************************** 
 * 
 * @brief   Changes the default memory allocation/free from memory service to
 *          raid's `mock' (a.k.a. native allocate/free) memory service.
 *  
 * @param   memory_request_p - Pointer to memory request to allocate
 *  
 * @return  status - Typically FBE_STATUS_OK
 * 
 * @author
 *  8/06/2009 - Created. Rob Foley
 * 
 *****************************************************************************/
static fbe_status_t fbe_raid_memory_mock_request_entry(fbe_memory_request_t *memory_request_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      b_request_started = FBE_FALSE;
    fbe_u32_t       queue_length = 0; 

    /* Allocate the requested bytes and set the status appropriately
     */
    fbe_spinlock_lock(&fbe_raid_memory_mock_memory_lock);

    /* Determine if we should allocated immediately or not
     */
    b_request_started = fbe_raid_memory_mock_should_start_request();
    if (b_request_started == FBE_TRUE)
    {
        /* Start the request immediately
         */
        status = fbe_raid_memory_mock_process_malloc_request(memory_request_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_ERROR,
                                   "raid: memory mock: mock memory entry failed with status: 0x%x \n",
                                   status);
        }
        else
        {
            /* Else if the status was ok log the request in the tracking structure
             */
            fbe_raid_memory_track_allocation(memory_request_p);
        }
        memory_request_p->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED_IMMEDIATELY;
    }
    else
    {
        /* Else the allocation request was deferred but we start tracking it
         * immediately
         */
        status = fbe_raid_memory_mock_defer_request(memory_request_p);
        if (status == FBE_STATUS_PENDING)
        {
            fbe_raid_memory_track_allocation(memory_request_p);
        }
    }

    /* We need to signal the allocation thread if there are more request that
     * need to be allocated.
     */
    if (fbe_raid_memory_mock_allow_deferred_allocations == FBE_TRUE)
    {
        queue_length = fbe_queue_length(&fbe_raid_memory_mock_request_queue_head);
        if(!fbe_queue_is_empty(&fbe_raid_memory_mock_request_queue_head)) 
        {
            fbe_semaphore_release(&fbe_raid_memory_mock_semaphore, 0, 1, FALSE);
            /*! @note Remove call when raid mock memory service is functional
             */
            {
                fbe_raid_memory_mock_trace(NULL, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_DEBUG_THREAD,
                                           "raid: memory mock: alloc signal with %d deferred requests\n",
                                           queue_length);
            }
        }
    }

    /* Now release the lock
     */
    fbe_spinlock_unlock(&fbe_raid_memory_mock_memory_lock);

    /*  If we started the request immediately invoke the callback now
     */
    if (b_request_started == FBE_TRUE)
    {
        memory_request_p->completion_function(memory_request_p, memory_request_p->completion_context);
    }
    
    /* Made it this far return success
     */
    return status;
}
/********************************************
 * end of fbe_raid_memory_mock_request_entry()
 ********************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_remove_pending_request()
 ***************************************************************************** 
 * 
 * @brief   Removes the memory request from the queue of pending memory request.
 *          Even if the item isn't located it isn't an error.
 *  
 * @param   memory_request_to_remove_p - Pointer to memory request to remove
 *  
 * @return  status - Typically FBE_STATUS_OK
 * 
 * @author
 *  11/09/2010  Ron Proulx  - Created
 * 
 *****************************************************************************/
static fbe_status_t fbe_raid_memory_mock_remove_pending_request(fbe_memory_request_t *const memory_request_to_remove_p)
{
    fbe_status_t            status = FBE_STATUS_COMPONENT_NOT_FOUND;
    fbe_u32_t               queue_length = 0;
    fbe_queue_element_t    *current_element_p = NULL;
    fbe_memory_request_t   *memory_request_p = NULL;

    /*! @note There is no way to known if the request was deferred or not. 
     *        The only way to locate the request is to match the master header
     *        pointer of a request that is on the queue with the one passed.
     */
    queue_length = fbe_queue_length(&fbe_raid_memory_mock_request_queue_head);
    current_element_p = fbe_queue_front(&fbe_raid_memory_mock_request_queue_head);
    while(current_element_p != NULL)
    {
        /* Check if we found a match
         */
        memory_request_p = fbe_raid_memory_mock_queue_element_to_request(current_element_p);
        if (memory_request_p == memory_request_to_remove_p)
        {
            /* We found the request.  Remove it from the pending allocation
             */
            fbe_queue_remove(current_element_p);
            status = FBE_STATUS_OK;
            break;
        }
        current_element_p = fbe_queue_next(&fbe_raid_memory_mock_request_queue_head, current_element_p); 
    }

    /* Always return the execution status
     */
    return status;
}
/*******************************************************
 * end of fbe_raid_memory_mock_remove_pending_request()
 *******************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_abort_request()
 ***************************************************************************** 
 * 
 * @brief   Changes the default memory abort from memory service to
 *          raid's `mock' abort api.
 *  
 * @param   memory_request_p - Pointer to memory request to abort
 *  
 * @return  status - Typically FBE_STATUS_OK
 * 
 * @note    It is not an error of the memory request isn't located
 * 
 * @author
 *  11/08/2010  Ron Proulx  - Created
 * 
 *****************************************************************************/
static fbe_status_t fbe_raid_memory_mock_abort_request(fbe_memory_request_t *memory_request_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      b_invoke_callback = FBE_FALSE;

    /* Get the memory lock and attempt to locate this request
     */
    fbe_spinlock_lock(&fbe_raid_memory_mock_memory_lock);

    /* Trace some information about the abort request
     */
    fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_INFO,
                               "raid: memory mock: request to abort memory request \n");

    /* A status of ok indicates that the memory request was located and
     * removed from the queue
     */
    status = fbe_raid_memory_mock_remove_pending_request(memory_request_p);
    if (status == FBE_STATUS_OK)
    {
        /* If the status is ok, remove the request from the tracking array 
         */
        fbe_raid_memory_track_remove_aborted_request(memory_request_p);

        /* Validate that there is no memory header
         */
        if (memory_request_p->ptr != NULL)
        {
            fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_ERROR,
                                   "raid: memory mock: cannot abort - master header ptr: 0x%p isn't NULL\n",
                                   memory_request_p->ptr);
        }
        memory_request_p->request_state = FBE_MEMORY_REQUEST_STATE_ABORTED;

        /* Trace some information and change the state to aborted
         */
        fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_INFO,
                                   "raid: memory mock: aborted memory request successfully.\n");

        /* Flag the fact that we need to invoke the callback
         */
        b_invoke_callback = FBE_TRUE;
    }

    /* Now release the lock
     */
    fbe_spinlock_unlock(&fbe_raid_memory_mock_memory_lock);

    /*  If we need to invoke the callback do it now
     */
    if (b_invoke_callback == FBE_TRUE)
    {
        memory_request_p->completion_function(memory_request_p, memory_request_p->completion_context);
    }
    
    /* Made it this far return success
     */
    return FBE_STATUS_OK;
}
/*******************************************
 * end of fbe_raid_memory_mock_abort_request()
 *******************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_remove_deferred_request()
 ***************************************************************************** 
 * 
 * @brief   Removes the deferred the memory request using the links provided
 *          in the memory request.
 *  
 * @param   memory_request_p - Pointer to memory request to allocate
 *  
 * @return  status - Typically FBE_STATUS_PENDING
 * 
 * @author
 *  10/25/2010  Ron Proulx  - Created
 * 
 *****************************************************************************/
static fbe_status_t fbe_raid_memory_mock_remove_deferred_request(fbe_memory_header_t *const master_memory_header,
                                                                 fbe_memory_request_t **memory_request_pp)
{
    fbe_status_t            status = FBE_STATUS_COMPONENT_NOT_FOUND;
    fbe_u32_t               queue_length = 0;
    fbe_queue_element_t    *current_element_p = NULL;
    fbe_memory_request_t   *memory_request_p = NULL;

    /*! @note There is no way to known if the request was deferred or not. 
     *        The only way to locate the request is to match the master header
     *        pointer of a request that is on the queue with the one passed.
     */
    queue_length = fbe_queue_length(&fbe_raid_memory_mock_request_queue_head);
    current_element_p = fbe_queue_front(&fbe_raid_memory_mock_request_queue_head);
    while(current_element_p != NULL)
    {
        /* Check if we found a match
         */
        memory_request_p = fbe_raid_memory_mock_queue_element_to_request(current_element_p);
        if ((fbe_memory_header_t *)memory_request_p->ptr == master_memory_header)
        {
            /* We found the request.  Remove it from the deferred alloocation
             * queue and set the passed pointer.
             */
            fbe_queue_remove(current_element_p);
            *memory_request_pp = memory_request_p;
            status = FBE_STATUS_OK;
            break;
        }
        current_element_p = fbe_queue_next(&fbe_raid_memory_mock_request_queue_head, current_element_p); 
    }

    /* Always return the execution status
     */
    return status;
}
/*******************************************************
 * end of fbe_raid_memory_mock_remove_deferred_request()
 *******************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_memory_free_mock()
 ***************************************************************************** 
 * 
 * @brief   Changes the default memory allocation/free from memory service to
 *          raid's `mock' (a.k.a. native allocate/free) memory service.
 *  
 * @param   master_memory_header - Pointer to master header to free
 *  
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  10/18/2010  Ron Proulx  - Created
 * 
 *****************************************************************************/
static fbe_status_t fbe_raid_memory_mock_memory_free_mock(fbe_memory_header_t *const master_memory_header)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_memory_request_t   *memory_request_p = NULL;    /* We don't have a link to the memory request */
    fbe_memory_header_t    *current_memory_header = NULL;
    fbe_memory_header_t    *next_memory_header = NULL;
    fbe_memory_chunk_size_t master_chunk_size = master_memory_header->memory_chunk_size;
    fbe_u32_t               i;
    fbe_u32_t               number_of_chunks;

    if(master_memory_header->magic_number != FBE_MEMORY_HEADER_MAGIC_NUMBER)
    {
        fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_CRITICAL,
                                   "raid: memory mock: master header: 0x%p magic_number: 0x%llx isn't: 0x%x\n",
                                   master_memory_header, 
                                   master_memory_header->magic_number, FBE_MEMORY_HEADER_MAGIC_NUMBER);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now get the master memory header and remove from deferred queue if
     * neccessary
     */
    if (fbe_raid_memory_mock_allow_deferred_allocations == FBE_TRUE)
    {
        fbe_status_t    expected_fail_status = FBE_STATUS_OK;

        /*! @note We don't expect this to succeed since the caller is attempting
         *        to free a previously allocated request!!
         */
        expected_fail_status = fbe_raid_memory_mock_remove_deferred_request(master_memory_header, &memory_request_p);
        if (expected_fail_status == FBE_STATUS_OK)
        {
            fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_ERROR,
                                   "raid: memory mock: free for master header: 0x%p failed with status: 0x%x\n",
                                   master_memory_header, expected_fail_status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* process request
     */
    number_of_chunks = master_memory_header->number_of_chunks;
    current_memory_header = master_memory_header;

    for(i = 0; i < number_of_chunks; i++){
        /* We already validated the master only check index 1 or greater*/
        if((i > 0)                                                                 &&
           (current_memory_header->magic_number != FBE_MEMORY_HEADER_MAGIC_NUMBER)    ){
            fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_CRITICAL,
                                   "raid: memory mock: current header: 0x%p magic_number: 0x%llx isn't: 0x%x\n",
                                   current_memory_header, 
                                   (unsigned long long)current_memory_header->magic_number, FBE_MEMORY_HEADER_MAGIC_NUMBER);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        next_memory_header = current_memory_header->u.hptr.next_header;

        /* Native free starting from the header */
        fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_DEBUG_REQUEST, 
                                   "raid: memory mock: freeing chunk: 0x%p index: %d for %d bytes \n",
                                   current_memory_header, i, master_chunk_size); 
        fbe_memory_native_release(current_memory_header);

        /* Goto next */
        current_memory_header = next_memory_header;
    }


    /* Validate that we freed all the chunks */
    if (i != number_of_chunks)
    {
        /* Even if we fail we return success below so that we don't purposfull leak memory
         */
        fbe_raid_memory_mock_trace(memory_request_p, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_ERROR,
                                   "raid: memory mock: master header: 0x%p chunks found: %d doesn't equal expected: %d\n",
                                   master_memory_header, i, number_of_chunks);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* Made it this far, success
     */
    return FBE_STATUS_OK;
}
/*************************************************
 * end of fbe_raid_memory_mock_memory_free_mock()
 *************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_free_entry()
 ***************************************************************************** 
 * 
 * @brief   Changes the default memory allocation/free from memory service to
 *          raid's `mock' (a.k.a. native allocate/free) memory service.
 *  
 * @param   memory_p - Pointer to master header with list of items to free
 *  
 * @return  status - Typically BFE_STATUS_OK
 *
 * @author
 *  8/06/2009 - Created. Rob Foley
 * 
 *****************************************************************************/
static fbe_status_t fbe_raid_memory_mock_free_entry(fbe_memory_ptr_t memory_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       queue_length = 0;

    /* Take the lock and process the request
     */
    fbe_spinlock_lock(&fbe_raid_memory_mock_memory_lock);

    /* Now free the request
     */
    status = fbe_raid_memory_mock_memory_free_mock(memory_p);
    if (status == FBE_STATUS_OK)
    {
        /* If the status is ok, remove the request from the tracking array */
        fbe_raid_memory_track_free(memory_p);
    }

    /* We need to signal the allocation thread if there are more request that
     * need to be allocated.
     */
    if (fbe_raid_memory_mock_allow_deferred_allocations == FBE_TRUE)
    {
        queue_length = fbe_queue_length(&fbe_raid_memory_mock_request_queue_head);
        if(!fbe_queue_is_empty(&fbe_raid_memory_mock_request_queue_head)) 
        {
            fbe_semaphore_release(&fbe_raid_memory_mock_semaphore, 0, 1, FALSE);
            /*! @note Remove call when raid mock memory service is functional
             */
            {
                fbe_raid_memory_mock_trace(NULL, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_DEBUG_THREAD,
                                           "raid: memory mock: free signal with %d deferred requests\n",
                                           queue_length);
            }
        }
    }

    /* Now release the spinlock
     */
    fbe_spinlock_unlock(&fbe_raid_memory_mock_memory_lock);

    return status;
}
/*******************************************
 * end of fbe_raid_memory_mock_free_entry()
 *******************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_validate_memory_leaks()
 ***************************************************************************** 
 * 
 * @brief   Changes the default memory allocation/free from memory service to
 *          raid's `mock' (a.k.a. native allocate/free) memory service.
 *  
 * @param   None
 *  
 * @return  None
 *
 * @author
 *  8/06/2009 - Created. Rob Foley
 * 
 *****************************************************************************/
void fbe_raid_memory_mock_validate_memory_leaks(void)
{
    fbe_atomic_t    memory_outstanding_allocations = fbe_raid_memory_track_get_outstanding_allocations();

    /* Make sure our current number of outstanding allocations is 0
     */
    if (memory_outstanding_allocations != 0)
    {
        fbe_raid_memory_mock_trace(NULL, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_CRITICAL,
                                   "raid: memory mock: outstanding allocations: %lld isn't 0\n",
                                   (long long)memory_outstanding_allocations);
    }
    return;
}
/****************************************************
 * end of fbe_raid_memory_mock_validate_memory_leaks()
 ****************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_setup_mocks()
 ***************************************************************************** 
 * 
 * @brief   Changes the default memory allocation/free from memory service to
 *          raid's `mock' (a.k.a. native allocate/free) memory service.
 * 
 * @param   None
 *  
 * @return  None
 *
 * @author
 *  8/06/2009 - Created. Rob Foley
 * 
 *****************************************************************************/
void fbe_raid_memory_mock_setup_mocks(void)
{
    EMCPAL_STATUS       nt_status;

    /* If mocks aren't enabled simply return
     */
    if (fbe_raid_memory_mock_is_mock_enabled() == FBE_FALSE)
    {
        return;
    }

    /* Mock out the calls to allocate and free with malloc/free. 
     */
    fbe_spinlock_init(&fbe_raid_memory_mock_memory_lock);

    /* Only initialize the deferred allocation thread if deferred allocations
     * are allowed.
     */
    if (fbe_raid_memory_mock_allow_deferred_allocations == FBE_TRUE)
    {
        /* If deferred allocations are allowed, start the deferred allocation
         * thread
         */
        ;
        nt_status = fbe_thread_init(&fbe_raid_memory_mock_thread, "fbe_raid_mmock",
                                    fbe_raid_memory_mock_thread_func, 
                                    NULL);
        if (nt_status != EMCPAL_STATUS_SUCCESS) 
        {
            /* Switch to non-deferred, report an error and continue
             */
            fbe_raid_memory_mock_allow_deferred_allocations = FBE_FALSE;
            fbe_raid_memory_mock_trace(NULL, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_CRITICAL,
                                       "raid: failed to initialized thread with nt_status: 0x%x \n",
                                       nt_status);
        }
        else
        {
            fbe_raid_memory_mock_thread_state = FBE_RAID_MEMORY_THREAD_STATE_INITIALIZED;
            fbe_semaphore_init(&fbe_raid_memory_mock_semaphore, 0, 0x0FFFFFFF);
            fbe_semaphore_init(&fbe_raid_memory_mock_defer_delay_semaphore, 0, 0x0FFFFFFF);
            fbe_queue_init(&fbe_raid_memory_mock_request_queue_head);
        }
    }

    /* Calls to allocate, abort and free should have been setup
     */
    if ((fbe_raid_memory_mock_allocate_mock == NULL) ||
        (fbe_raid_memory_mock_abort_mock == NULL)    ||
        (fbe_raid_memory_mock_free_mock  == NULL)       )
    {
        fbe_raid_memory_mock_trace(NULL, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_CRITICAL,
                                   "raid: either allocate: 0x%p abort: 0x%p or free: 0x%p is NULL\n",
                                   fbe_raid_memory_mock_allocate_mock, fbe_raid_memory_mock_abort_mock,
                                   fbe_raid_memory_mock_free_mock); 
    }

    return;
}
/*******************************************
 * end of fbe_raid_memory_mock_setup_mocks()
 *******************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_destroy_mocks()
 ***************************************************************************** 
 * 
 * @brief   Execute any cleanup
 *  
 * @param   None
 *  
 * @return  None
 * 
 * @note    The syncronization between the destroy and the deferred thread is
 *          critical
 *
 * @author
 *  8/06/2009 - Created. Rob Foley
 * 
 *****************************************************************************/
void fbe_raid_memory_mock_destroy_mocks(void)
{
    /* If mocks aren't enabled simply return
     */
    if (fbe_raid_memory_mock_is_mock_enabled() == FBE_FALSE)
    {
        return;
    }

    /* First validate that there aren't any memory leaks
     */
    fbe_raid_memory_mock_validate_memory_leaks();

    /* If we started the deferred allocation thread clean that up
     */
    if (fbe_raid_memory_mock_allow_deferred_allocations == FBE_TRUE)
    {
        /* First change the requested thread state then take the lock
         */
        fbe_raid_memory_mock_thread_state = FBE_RAID_MEMORY_THREAD_STATE_DESTROY_REQUESTED; 
        fbe_spinlock_lock(&fbe_raid_memory_mock_memory_lock);
        
        /* Now signal the thread so that it can cleanup
         */
        fbe_semaphore_release(&fbe_raid_memory_mock_semaphore, 0, 1, FALSE);
        fbe_semaphore_release(&fbe_raid_memory_mock_defer_delay_semaphore, 0, 1, FALSE);
        fbe_spinlock_unlock(&fbe_raid_memory_mock_memory_lock);

        /* Now execute the cleanup
         */
        fbe_thread_wait(&fbe_raid_memory_mock_thread);
        fbe_thread_destroy(&fbe_raid_memory_mock_thread);

        fbe_spinlock_lock(&fbe_raid_memory_mock_memory_lock);
        fbe_semaphore_destroy(&fbe_raid_memory_mock_semaphore);
        fbe_semaphore_destroy(&fbe_raid_memory_mock_defer_delay_semaphore);
        fbe_queue_destroy(&fbe_raid_memory_mock_request_queue_head);
    }
    else
    {
        /* Else take the lock immediately
         */
        fbe_spinlock_lock(&fbe_raid_memory_mock_memory_lock);
    }

    /* Now cleanup
     */
    fbe_raid_memory_mock_allow_deferred_allocations = FBE_FALSE;
    fbe_spinlock_destroy(&fbe_raid_memory_mock_memory_lock);

    return;
}
/****************************************************
 * end of fbe_raid_memory_mock_destroy_mocks()
 ****************************************************/

/*****************************************************************************
 *          fbe_raid_memory_mock_change_defer_delay()
 ***************************************************************************** 
 * 
 * @brief   Depending on the parameters passed and the state of the defer
 *          thread, set the `additional' delay (cleared automatically)
 *  
 * @param   b_quiesced - FBE_TRUE - This iots is already quiesced
 *                       FBE_FALSE - This iots is not quiesce
 *  
 * @return  None
 * 
 * @note    Currently there is no fbe api to allow us to change the defer
 *          allocation delay on the fly.  Therefore this method is invoked to
 *          give us hints that we might want to temporarily change the delay
 *          for memory allocations.  If the fbe api is added that changes the
 *          delay this should be removed.
 *
 * @author
 *  11/09/2010  - Ron Proulx - Created
 * 
 *****************************************************************************/
void fbe_raid_memory_mock_change_defer_delay(fbe_bool_t b_quiesced)
{
    fbe_u32_t               queue_length = 0;

    /* If there are still outstanding allocations, set the `additional'
     * delay.
     */
    if ((b_quiesced == FBE_FALSE)                                                    &&
        (fbe_raid_memory_mock_thread_state == FBE_RAID_MEMORY_THREAD_STATE_DELAYING) &&
        (fbe_raid_memory_mock_additional_delay == 0)                                    )
    {
        /* Trace some information and change delay
         */
        queue_length = fbe_queue_length(&fbe_raid_memory_mock_request_queue_head);
        fbe_raid_memory_mock_trace(NULL, FBE_RAID_MEMORY_MOCK_TRACE_PARAMS_INFO,
                                   "raid: memory mock: change defer delay to: %d seconds with: %d items waiting\n",
                                   (FBE_RAID_MEMORY_MOCK_ADDITONAL_DELAY_MS / 1000), queue_length);
        fbe_raid_memory_mock_additional_delay = FBE_RAID_MEMORY_MOCK_ADDITONAL_DELAY_MS;
    }

    return;
}
/****************************************************
 * end of fbe_raid_memory_mock_change_defer_delay()
 ****************************************************/

/*********************************
 * end file fbe_raid_memory_mock.c
 *********************************/
