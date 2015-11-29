/****************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_io_cmd_memory.c
 ***************************************************************************
 *
 * @brief
 *  This file contains logical drive io cmd object functions for 
 *  allocating and freeing memory.
 * 
 * @ingroup logical_drive_class_files
 *
 * @version
 *   10/29/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "EmcPAL_Memory.h"
/****************************************
 * Defines for our sg pool.
 ****************************************/

/*! Ptr to memory allocated for sg lists.
 */
static fbe_logical_drive_io_cmd_t *fbe_ldo_global_io_cmd_pool_p;

/*! Memory pool head for sg lists 
 */
static fbe_queue_head_t fbe_ldo_io_cmd_pool_free_head;

/*! Memory pool head for waiting requests 
 */
static fbe_queue_head_t fbe_ldo_io_cmd_pool_wait_head;

/*! Memory pool lock 
 */
static fbe_spinlock_t fbe_ldo_io_cmd_pool_lock;

/****************************************
 * LOCALLY DEFINED FUNCTIONS
 ****************************************/

/*!***************************************************************
 * fbe_ldo_io_cmd_queue_element_to_pool_element()
 ****************************************************************
 * @brief
 *  Convert from a queue element back to the fbe_logical_drive_io_cmd_t.
 *
 * @param queue_element_p - This is the queue element that we wish to
 *                          get the io cmd for.
 *
 * @return fbe_logical_drive_io_cmd_t - The io cmd object that
 *                                      has the input queue element.
 *
 * @author
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
fbe_logical_drive_io_cmd_t * 
fbe_ldo_io_cmd_queue_element_to_pool_element(fbe_queue_element_t * queue_element_p)
{
    /* We're converting an address to a queue element into an address to the containing io command. 
     * In order to do this we need to subtract the offset to the queue element from the 
     * address of the queue element.
     */
    fbe_logical_drive_io_cmd_t * pool_element_p;
    pool_element_p = (fbe_logical_drive_io_cmd_t  *)((fbe_u8_t *)queue_element_p - 
                                                    (fbe_u8_t *)(&((fbe_logical_drive_io_cmd_t  *)0)->queue_element));
    return pool_element_p;
}
/**************************************
 * end fbe_ldo_io_cmd_queue_element_to_pool_element()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_free_resources(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function frees resources for the io command object.
 *
 * @param io_cmd_p - The logical drive io cmd object to free for.
 *
 * @return
 *  NONE
 *
 * @author
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_io_cmd_free_resources(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    /* Currently nothing to free.
     */
    return;
}
/* end fbe_ldo_io_cmd_free_resources() */

/*!**************************************************************
 * @fn fbe_ldo_io_cmd_pool_init(void)
 ****************************************************************
 * @brief
 *  Initialize the pool of the logical drive.
 *
 * @param  - None.              
 *
 * @return - None.   
 *
 * @author
 *  9/15/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ldo_io_cmd_pool_init(void)
{
    fbe_logical_drive_io_cmd_t *pool_p;
    fbe_u32_t sg_number;

    /* Initialize queue and spinlock.
     */
    fbe_queue_init(&fbe_ldo_io_cmd_pool_free_head);
    fbe_queue_init(&fbe_ldo_io_cmd_pool_wait_head);
    fbe_spinlock_init(&fbe_ldo_io_cmd_pool_lock);

    /* Allocate memory for our pool.
     * Using the same allocate/free function that the memory service is using 
     * currently. 
     */
    pool_p = fbe_allocate_contiguous_memory(sizeof(fbe_logical_drive_io_cmd_t) * FBE_LDO_IO_CMD_POOL_MAX);

    /* Save the pointer to the pool so we can free it later.
     */
    fbe_ldo_global_io_cmd_pool_p = pool_p;
    fbe_zero_memory(pool_p, sizeof(fbe_logical_drive_io_cmd_t) * FBE_LDO_IO_CMD_POOL_MAX);

    if (pool_p == NULL)
    {
        /* Out of memory, return bad status.
         */
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Take each piece of the memory and add it to the queue.
     */
    for (sg_number = 0; sg_number < FBE_LDO_IO_CMD_POOL_MAX; sg_number++)
    {
        /* Just push the item onto the queue.
         */
        fbe_queue_push( &fbe_ldo_io_cmd_pool_free_head, &pool_p->queue_element);
        pool_p++;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ldo_io_cmd_pool_init()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_io_cmd_pool_destroy(void)
 ****************************************************************
 * @brief
 *  Destroy the sg list pool of the logical drive.
 *
 * @param  - None.
 *
 * @return  - None.  
 *
 * @author
 *  9/15/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_io_cmd_pool_destroy(void)
{
    /* Destroy all the queues.
     */
    fbe_queue_destroy(&fbe_ldo_io_cmd_pool_free_head);
    fbe_queue_destroy(&fbe_ldo_io_cmd_pool_wait_head);
    fbe_spinlock_destroy(&fbe_ldo_io_cmd_pool_lock);

    /* If the memory was allocated, then free it.
     * Using the same allocate/free function that the memory service is using 
     * currently. 
     */
    if (fbe_ldo_global_io_cmd_pool_p != NULL)
    {
        fbe_release_contiguous_memory(fbe_ldo_global_io_cmd_pool_p);
        fbe_ldo_global_io_cmd_pool_p = NULL;
    }
    return;
}
/******************************************
 * end fbe_ldo_io_cmd_pool_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_ldo_io_cmd_pool_get_free_count(void)
 ****************************************************************
 * @brief
 *  Return the count of how many objects are in the pool.
 *
 * @param - None.
 *
 * @return - fbe_u32_t - count of objects in the pool.
 *
 * @author
 *  03/20/2009 - Created. RPF
 *
 ****************************************************************/

fbe_u32_t fbe_ldo_io_cmd_pool_get_free_count(void)
{
    fbe_queue_element_t *element_p;
    fbe_u32_t number_of_objects;

    /* If the pool is empty, return 0 for count. 
     */
    if (fbe_ldo_global_io_cmd_pool_p == NULL)
    {
        return 0;
    }

    /* Set our count of elements to 0 to start. 
     * Also set our element pointer to the first element. 
     */
    number_of_objects = 0;
    element_p = fbe_queue_front(&fbe_ldo_io_cmd_pool_free_head);

    /* Loop over all elements counting, until we get a null.
     */
    while (element_p != NULL)
    {
        number_of_objects++;
        element_p = fbe_queue_next(&fbe_ldo_io_cmd_pool_free_head, element_p);
    }
    return number_of_objects;
}
/******************************************
 * end fbe_ldo_io_cmd_pool_get_free_count()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_allocate_io_cmd(
 *               fbe_packet_t * const packet_p,
 *               fbe_memory_completion_function_t completion_function,
 *               fbe_memory_completion_context_t completion_context)
 ****************************************************************
 * @brief
 *  This function allocates an io command for the given logical drive.
 *
 * @param packet_p - Memory request to allocate with.
 * @param completion_function - Fn to call when mem is available.
 * @param completion_context - Context to use when mem is available.
 *
 * @return
 *  FBE_STATUS_OK - This function will never fail.
 *                  Our completion function will always be called.
 *
 * @author
 *  06/11/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_allocate_io_cmd( fbe_packet_t * const packet_p,
                                      fbe_memory_completion_function_t completion_function,
                                      fbe_memory_completion_context_t completion_context )
{
    fbe_spinlock_lock(&fbe_ldo_io_cmd_pool_lock);

    /* If the wait queue is empty and the pool of free sgs is not empty,
     * then allocate a new one. 
     * This allows already waiting requests to have priority over 
     * the requests that are just arriving. 
     */
    if (fbe_queue_is_empty(&fbe_ldo_io_cmd_pool_wait_head) &&
        !fbe_queue_is_empty(&fbe_ldo_io_cmd_pool_free_head))
    {
        /* Pop the item off of the free queue and call our completion
         * function since memory was allocated successfully. 
         */
        fbe_queue_element_t *queue_element_p;
        fbe_logical_drive_io_cmd_t *io_cmd_p;

        queue_element_p = fbe_queue_pop(&fbe_ldo_io_cmd_pool_free_head);
        io_cmd_p = fbe_ldo_io_cmd_queue_element_to_pool_element(queue_element_p);

        packet_p->memory_request.ptr = io_cmd_p;
        fbe_spinlock_unlock(&fbe_ldo_io_cmd_pool_lock);
        completion_function( &packet_p->memory_request, completion_context);
    }
    else
    {
        /* Need to wait for the resource.
         * First init the memory request.
         */
        packet_p->memory_request.request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_PACKET;
        packet_p->memory_request.number_of_objects = 1;
        packet_p->memory_request.ptr = NULL;
        packet_p->memory_request.completion_function = completion_function;
        packet_p->memory_request.completion_context = completion_context;

        /* Next, push the memory request onto the wait list.
         */
        fbe_queue_push( &fbe_ldo_io_cmd_pool_wait_head,
                        &packet_p->memory_request.queue_element);

        fbe_spinlock_unlock(&fbe_ldo_io_cmd_pool_lock);
    }
    return FBE_STATUS_OK;
}
/* end fbe_ldo_allocate_io_cmd() */

/*!***************************************************************
 * @fn fbe_ldo_free_io_cmd(fbe_logical_drive_io_cmd_t *const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function frees the given input io cmd.
 *
 * @param io_cmd_p - The io cmd to free.
 *
 * @return
 *  NONE
 *
 * @author
 *  06/11/08 - Created. RPF
 *
 ****************************************************************/
void 
fbe_ldo_free_io_cmd(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
    /* Grab the lock since we're looking at global data.
     */
    fbe_spinlock_lock(&fbe_ldo_io_cmd_pool_lock);

    if (fbe_queue_is_empty(&fbe_ldo_io_cmd_pool_wait_head))
    {
        /* If no-one is waiting, then we can put the sg back on the free
         * list. 
         */
        fbe_queue_push( &fbe_ldo_io_cmd_pool_free_head, &io_cmd_p->queue_element);
        fbe_spinlock_unlock(&fbe_ldo_io_cmd_pool_lock);
    }
    else
    {
        /* Otherwise someone is waiting, give them the resource and wake 
         * them up. 
         */
        fbe_memory_request_t *memory_request_p = 
            (fbe_memory_request_t *)fbe_queue_pop(&fbe_ldo_io_cmd_pool_wait_head);

        memory_request_p->ptr = io_cmd_p;
        fbe_spinlock_unlock(&fbe_ldo_io_cmd_pool_lock);
        memory_request_p->completion_function(memory_request_p,
                                              memory_request_p->completion_context);
    }
    return;
}
/* end fbe_ldo_free_io_cmd() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_allocate_memory_callback(
 *                       fbe_memory_request_t * memory_request_p, 
 *                       fbe_memory_completion_context_t context)
 ****************************************************************
 * @brief
 *  This is the callback function for allocating an io cmd.
 * 
 * @param memory_request_p - memory request just allocated for.
 * @param context - The context associated with this callback.
 *                  This has the logical drive.
 *
 * @return
 *  FBE_STATUS_OK if allocation succeeded.
 *
 * @author
 *  6/11/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_ldo_io_cmd_allocate_memory_callback(fbe_memory_request_t * memory_request_p, 
                                        fbe_memory_completion_context_t context)
{   
    fbe_logical_drive_t *logical_drive_p = (fbe_logical_drive_t*)context;

    fbe_packet_t *packet_p;
    fbe_logical_drive_io_cmd_t *io_cmd_p;

    /* Get our packet ptr based on the memory request.
     */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* The memory we allocated is in the memory request.
     */
    io_cmd_p = memory_request_p->ptr;

    if (io_cmd_p == NULL)
    {
        /* If we failed to allocate memory, return a bad status.
         */
        fbe_base_object_trace(  (fbe_base_object_t*)logical_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, logical_drive allocate io cmd failed\n", __FUNCTION__);

        /* Transport status is OK, since this packet was delivered successfully.
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

        /* The payload status is bad since we could not complete this operation.
         */
        fbe_ldo_set_payload_status(packet_p,
                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INSUFFICIENT_RESOURCES);
        fbe_ldo_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocation successful, let's init and kick off the io cmd.
     */
    fbe_ldo_io_cmd_init(io_cmd_p, (fbe_object_handle_t)logical_drive_p, packet_p);

    /* Kick off the operation.
     */
    fbe_ldo_io_state_machine(io_cmd_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ldo_io_cmd_allocate_memory_callback()
 ******************************************/

/*******************************
 * end fbe_logical_drive_io_cmd_memory.c
 ******************************/
