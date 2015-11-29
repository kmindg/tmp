#include "fbe_lifecycle_private.h"

static fbe_status_t 
lifecycle_pending_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_object_t * p_object = (fbe_base_object_t*)context;
    fbe_base_object_t * base_object = NULL;
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_object_id_t object_id;
    fbe_status_t status;



    /* There were cases where the completion context gets corrupted in the packet stack.
     * Reconstruct the object pointer in case of corruption and log critical errors to avoid system panic.
     */
    base_object = (fbe_base_object_t *)((fbe_u8_t *)(packet->stack[0].completion_context) - 
                                        (fbe_u8_t *)(&((fbe_base_object_t *)0)->scheduler_element));

    if(base_object != p_object)
    {
        /* Assign the newly reconstructued pointer to p_object */
        p_object = base_object;

        /* Get a pointer to the instance data of the base class. */
        status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
        if (status != FBE_STATUS_OK) {
            lifecycle_error(NULL,
                "%s: Can't perform pending completion function, status: 0x%X\n",
                __FUNCTION__, status);
            return FBE_STATUS_OK;
        }

        fbe_base_object_get_object_id(base_object, &object_id);

        lifecycle_error(p_inst_base,
                        "%s: Reconstructed the Object Pointer. Packet: %p/0x%p, Object Id: 0x%X\n",
                        __FUNCTION__, packet, context, object_id);

    }
    else
    {        
        status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
        if (status != FBE_STATUS_OK) {
            lifecycle_error(NULL,
                "%s: Can't perform pending completion function, status: 0x%X\n",
                __FUNCTION__, status);
            return FBE_STATUS_OK;
        }
    }
    

    /* Reschedule the monitor. */
    (void)lifecycle_reschedule(p_object, p_inst_base, p_inst_base->reschedule_msecs);

    if (p_inst_base->p_packet != NULL) {
        /* Set the packet status. */
        fbe_transport_set_status(p_inst_base->p_packet, FBE_STATUS_OK, 0);
        /* Forget about the current monitor control packet. */
        p_inst_base->p_packet = NULL;
    }

    /* Forget about the current condition. */
    p_inst_base->p_cond_inst = NULL;

    return FBE_STATUS_OK;
}

/*
 * This function runs a pending function.
 */

fbe_lifecycle_status_t
lifecycle_run_pending_func(fbe_lifecycle_inst_base_t * p_inst_base,
                           const fbe_lifecycle_func_pending_t pending_func)
{
    fbe_lifecycle_status_t lifecycle_status;
    fbe_status_t status;

    /* Push our completion function onto the packet's completion stack. */
    status = fbe_transport_set_completion_function(p_inst_base->p_packet,
                                                   lifecycle_pending_completion,
                                                   (fbe_packet_completion_context_t)p_inst_base->p_object);
    if (status != FBE_STATUS_OK) {
        lifecycle_error(p_inst_base,
            "%s: Can't set pending completion function, status: 0x%X\n",
            __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Call the pending function. */
    lifecycle_status = pending_func(p_inst_base->p_object, p_inst_base->p_packet);

    switch(lifecycle_status) {
        case FBE_LIFECYCLE_STATUS_DONE:
        case FBE_LIFECYCLE_STATUS_PENDING:
            break;
        case FBE_LIFECYCLE_STATUS_RESCHEDULE:
            /* Reschedule the monitor. */
            p_inst_base->reschedule_msecs = 0;
            break;
        default:
            lifecycle_error(p_inst_base,
                "%s: Invalid pending function return, status code: 0x%X\n",
                __FUNCTION__, lifecycle_status);
            lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Remove the lifecycle_completion_function from the packet's stack. */
    status = fbe_transport_unset_completion_function(p_inst_base->p_packet,
                                                     lifecycle_pending_completion,
                                                     (fbe_packet_completion_context_t)p_inst_base->p_object);
    if (status != FBE_STATUS_OK) {
        lifecycle_error(p_inst_base,
            "%s: Can't unset pending completion function, status: 0x%X\n",
            __FUNCTION__, status);
    }

    return lifecycle_status;
}
