/*@LB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 ****  Copyright (c) 2006 EMC Corporation.
 ****  All rights reserved.
 ****
 ****  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.
 ****  ACCESS TO THIS SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT.
 ****  THIS CODE IS TO BE KEPT STRICTLY CONFIDENTIAL.
 ****
 ****************************************************************************
 ****************************************************************************
 *@LE************************************************************************/

/*@FB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 **** FILE: fbe_memory_mgmt_transport.c
 ****
 **** DESCRIPTION:
 **** NOTES:
 ****    <TBS>
 ****
 ****************************************************************************
 ****************************************************************************
 *@FE************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_memory.h"

/****************************************************************************
 * Local variables
 ****************************************************************************/

/****************************************************************************
 * Forward declaration
 ****************************************************************************/
static void fbe_transport_allocate_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);

fbe_packet_t *
fbe_transport_allocate_packet(void)
{
    fbe_memory_request_t memory_request;
    fbe_semaphore_t sem;

    fbe_semaphore_init(&sem, 0, 1);

    memory_request.request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_PACKET;
    memory_request.number_of_objects = 1;
    memory_request.ptr = NULL;
    memory_request.completion_function = fbe_transport_allocate_completion;
    memory_request.completion_context = &sem;
    fbe_memory_allocate(&memory_request);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return (fbe_packet_t *)memory_request.ptr;
}

void *
fbe_transport_allocate_buffer(void)
{
    fbe_memory_request_t memory_request;
    fbe_semaphore_t sem;

    fbe_semaphore_init(&sem, 0, 1);

    memory_request.request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_BUFFER;
    memory_request.number_of_objects = 1;
    memory_request.ptr = NULL;
    memory_request.completion_function = fbe_transport_allocate_completion;
    memory_request.completion_context = &sem;
    fbe_memory_allocate(&memory_request);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return memory_request.ptr;
}

static void
fbe_transport_allocate_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
}

void
fbe_transport_release_packet(fbe_packet_t * packet)
{
	fbe_transport_destroy_packet(packet);
    fbe_memory_release(packet);
}

void
fbe_transport_release_buffer(void * buffer)
{
    fbe_memory_release(buffer);
}

fbe_status_t
fbe_transport_allocate_packet_async(fbe_packet_t * packet,
                                   fbe_memory_completion_function_t	completion_function,
                                   fbe_memory_completion_context_t completion_context)
{
    packet->memory_request.request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_PACKET;
    packet->memory_request.number_of_objects = 1;
    packet->memory_request.ptr = NULL;
    packet->memory_request.completion_function = completion_function;
    packet->memory_request.completion_context = completion_context;
    fbe_memory_allocate(&packet->memory_request);

    return FBE_STATUS_OK;
}
