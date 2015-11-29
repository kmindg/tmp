/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_job_service_hook.c
 ***************************************************************************
 *
 * @brief
 *  This file contains job service hook functions which process a control
 *  request to set a hook in the job service.
 *  
 * @version
 *  03/12/2014  Ron Proulx  - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_base_object.h"
#include "fbe_transport_memory.h"
#include "fbe_base_service.h"
#include "fbe_topology.h"
#include "fbe_job_service_private.h"
#include "fbe_job_service.h"
#include "fbe_job_service_cmi.h"
#include "fbe_spare.h"
#include "fbe_job_service_operations.h"
#include "fbe_cmi.h"

/***************** 
 * LOCAL GLOBALS
 *****************/
static fbe_semaphore_t              fbe_job_service_hook_sem;
static fbe_bool_t                   fbe_job_service_debug_hook_b_initialized = FBE_FALSE;
static fbe_u32_t                    fbe_job_service_hooks_in_use = 0;
static fbe_job_service_debug_hook_t fbe_job_service_debug_hooks[FBE_MAX_JOB_SERVICE_DEBUG_HOOKS];
static fbe_spinlock_t		        fbe_job_service_hook_lock;


/******************************* 
 * LOCAL
 ************/
static void fbe_job_service_debug_hook_init_lock(void)
{
    fbe_spinlock_init(&fbe_job_service_hook_lock);
    return;
}
static void fbe_job_service_debug_hook_lock(void)
{
    fbe_spinlock_lock(&fbe_job_service_hook_lock);
    return;
}
static void fbe_job_service_debug_hook_unlock(void)
{
    fbe_spinlock_unlock(&fbe_job_service_hook_lock);
    return;
}
static void fbe_job_service_debug_hook_destroy_lock(void)
{
    fbe_spinlock_destroy(&fbe_job_service_hook_lock);
    return;
}
static fbe_status_t fbe_job_service_debug_hook_inc_hooks_in_use(void)
{
    if (fbe_job_service_hooks_in_use >= FBE_MAX_JOB_SERVICE_DEBUG_HOOKS) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s in use: %d\n", __FUNCTION__, fbe_job_service_hooks_in_use);
        fbe_job_service_hooks_in_use = (FBE_MAX_JOB_SERVICE_DEBUG_HOOKS - 1);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_job_service_hooks_in_use++;
    return FBE_STATUS_OK;
}
static fbe_status_t fbe_job_service_debug_hook_dec_hooks_in_use(void)
{
    if ((fbe_s64_t)fbe_job_service_hooks_in_use <= 0) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s in use: %d\n", __FUNCTION__, fbe_job_service_hooks_in_use);
        fbe_job_service_hooks_in_use = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_job_service_hooks_in_use--;
    return FBE_STATUS_OK;
}

/*****************************************************************************
 *          fbe_job_service_debug_hook_init()
 ***************************************************************************** 
 * 
 * @brief   Initialize the debug hook for the job service.
 *
 * @param   None
 *
 * @return  fbe_status_t
 *
 * @author
 *  03/12/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_job_service_debug_hook_init(void)
{
    fbe_u32_t   hook_index;

    fbe_job_service_debug_hook_init_lock();
    fbe_semaphore_init(&fbe_job_service_hook_sem, 0, 1);
    for (hook_index = 0; hook_index < FBE_MAX_JOB_SERVICE_DEBUG_HOOKS; hook_index++) {
        fbe_job_service_debug_hooks[hook_index].object_id = FBE_OBJECT_ID_INVALID;
    }
    fbe_job_service_hooks_in_use = 0;
    fbe_job_service_debug_hook_b_initialized = FBE_TRUE;
    return FBE_STATUS_OK;
}

/*****************************************************************************
 *          fbe_job_service_debug_hook_destroy()
 ***************************************************************************** 
 * 
 * @brief   Destroy the debug hook for the job service.
 *
 * @param   None
 *
 * @return  fbe_status_t
 *
 * @author
 *  03/12/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_job_service_debug_hook_destroy(void)
{
    fbe_semaphore_release(&fbe_job_service_hook_sem, 0, 1, FBE_FALSE);
    fbe_semaphore_destroy(&fbe_job_service_hook_sem);
    fbe_job_service_hooks_in_use = 0;
    fbe_job_service_debug_hook_b_initialized = FBE_FALSE;
    fbe_job_service_debug_hook_destroy_lock();
    return FBE_STATUS_OK;
}
/*!**************************************************************
 *          fbe_job_service_locate_debug_hook()
 **************************************************************** 
 * 
 * @brief   Locate the give hook and return status
 *
 * @param   object_id - Object id of hook to search for
 * @param   hook_type - Hook type to search for
 * @param   hook_state - Hook state to search for
 * @param   hook_phase - Hook phase to search for
 *
 * @return  fbe_status_t - FBE_STATUS_OK if found
 *
 * @note    Lock should be taken by caller.
 *
 * @author
 *  03/12/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_job_service_locate_debug_hook(fbe_object_id_t object_id,
                                                      fbe_job_type_t hook_type,
                                                      fbe_job_action_state_t hook_state,
                                                      fbe_job_debug_hook_state_phase_t hook_phase,
                                                      fbe_job_service_debug_hook_t **job_hook_pp)
{
    fbe_u32_t       hook_index;

    if (fbe_job_service_hooks_in_use == 0) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *job_hook_pp = NULL;
    for (hook_index = 0; hook_index < FBE_MAX_JOB_SERVICE_DEBUG_HOOKS; hook_index++) {
        // find the hook in the scheduler debug hook array and NULL it out
        if ((fbe_job_service_debug_hooks[hook_index].object_id   == object_id)  &&
            (fbe_job_service_debug_hooks[hook_index].hook_type   == hook_type)  &&
            (fbe_job_service_debug_hooks[hook_index].hook_state  == hook_state) &&
            (fbe_job_service_debug_hooks[hook_index].hook_phase  == hook_phase)    ) {
            *job_hook_pp = &fbe_job_service_debug_hooks[hook_index];
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}
/*****************************************
 * end fbe_job_service_locate_debug_hook()
 *****************************************/

/*****************************************************************************
 *          fbe_job_service_set_debug_hook()
 ***************************************************************************** 
 * 
 * @brief   Set a debug hook that will `pause' (wait on a semaphore) when the
 *          specified job hit the specified state.
 *
 * @param   hook - A pointer to the debug hook
 *
 * @return fbe_status_t
 *
 * @author
 *  03/12/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_job_service_set_debug_hook(fbe_job_service_debug_hook_t *job_hook_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       hook_index;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                      "%s entry\n", __FUNCTION__);

    if (!fbe_job_service_debug_hook_b_initialized) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s not initialized\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (job_hook_p == NULL) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s invalid hook\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_job_service_debug_hook_lock();

    for (hook_index = 0; hook_index < FBE_MAX_JOB_SERVICE_DEBUG_HOOKS; hook_index++) {
        // find and set the first empty element in the scheduler debug hook array
        if (fbe_job_service_debug_hooks[hook_index].object_id == FBE_OBJECT_ID_INVALID) {
            status = fbe_job_service_debug_hook_inc_hooks_in_use();
            if (status != FBE_STATUS_OK) {
                fbe_job_service_debug_hook_unlock();
                return status;
            }
            fbe_job_service_debug_hooks[hook_index].object_id = job_hook_p->object_id;
            fbe_job_service_debug_hooks[hook_index].hook_type = job_hook_p->hook_type;
            fbe_job_service_debug_hooks[hook_index].hook_state = job_hook_p->hook_state;
            fbe_job_service_debug_hooks[hook_index].hook_phase = job_hook_p->hook_phase;
            fbe_job_service_debug_hooks[hook_index].hook_action = job_hook_p->hook_action;
            fbe_job_service_debug_hooks[hook_index].hit_counter = 0;
            fbe_job_service_debug_hooks[hook_index].job_number = 0;
            break;
        }
    }

    fbe_job_service_debug_hook_unlock();

    if (hook_index >= FBE_MAX_JOB_SERVICE_DEBUG_HOOKS) {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: could not find an empty slot!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_job_service_set_debug_hook()
 **************************************/

/*!**************************************************************
 *          fbe_job_service_get_debug_hook()
 **************************************************************** 
 * 
 * @brief   Locate the give hook and return it's contents (counter)
 *
 * @param    hook - A pointer to the debug hook
 *
 * @return fbe_status_t
 *
 * @author
 *  03/12/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_job_service_get_debug_hook(fbe_job_service_debug_hook_t *job_hook_p)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_job_service_debug_hook_t   *located_hook_p = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                      "%s entry\n", __FUNCTION__);

    if (!fbe_job_service_debug_hook_b_initialized) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s not initialized\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((job_hook_p == NULL)                             ||
        ((job_hook_p->object_id == FBE_OBJECT_ID_INVALID) && (job_hook_p->hook_type != FBE_JOB_TYPE_UPDATE_DB_ON_PEER))   ) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s invalid hook\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_job_service_debug_hook_lock();

    status = fbe_job_service_locate_debug_hook(job_hook_p->object_id,
                                               job_hook_p->hook_type,
                                               job_hook_p->hook_state,
                                               job_hook_p->hook_phase,
                                               &located_hook_p);
    if (status == FBE_STATUS_OK) {
        /* Copy any OUT data */
        job_hook_p->hit_counter = located_hook_p->hit_counter;
        job_hook_p->job_number = located_hook_p->job_number;
    }

    fbe_job_service_debug_hook_unlock();

    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: could not find hook job: 0x%llx\n", __FUNCTION__, job_hook_p->job_number);
        return status;
    }

    return FBE_STATUS_OK;
}
/**************************************** 
 * end fbe_job_service_get_debug_hook()
 ****************************************/

/*!**************************************************************
 *          fbe_job_service_del_debug_hook()
 **************************************************************** 
 * 
 * @brief   Delete the given debug hook and remove the hook function
 *          pointer from the job service.
 *
 * @param   hook - A pointer to the debug hook
 *
 * @return none
 *
 * @author
 *  03/12/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_job_service_del_debug_hook(fbe_job_service_debug_hook_t *job_hook_p)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_job_service_debug_hook_t   *located_hook_p = NULL;
    fbe_bool_t                      b_job_service_thread_released = FBE_FALSE;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                      "%s entry\n", __FUNCTION__);

    if (!fbe_job_service_debug_hook_b_initialized) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s not initialized\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((job_hook_p == NULL)                             ||
        (job_hook_p->object_id == FBE_OBJECT_ID_INVALID)    ) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s invalid hook\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_job_service_debug_hook_lock();

    status = fbe_job_service_locate_debug_hook(job_hook_p->object_id,  
                                               job_hook_p->hook_type,  
                                               job_hook_p->hook_state, 
                                               job_hook_p->hook_phase, 
                                               &located_hook_p);
    if (status == FBE_STATUS_OK) {
        /* First check if we need to release the semaphore
         */
        if ((located_hook_p->hook_action == FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_PAUSE) &&
            (located_hook_p->hit_counter > 0)                                           ) {
            fbe_semaphore_release(&fbe_job_service_hook_sem, 0, 1, FBE_FALSE);
            b_job_service_thread_released = FBE_TRUE;
        }

        status = fbe_job_service_debug_hook_dec_hooks_in_use();
        if (status != FBE_STATUS_OK) {
            fbe_job_service_debug_hook_unlock();
            return status;
        }
        fbe_zero_memory(located_hook_p, sizeof(*located_hook_p));
        located_hook_p->object_id = FBE_OBJECT_ID_INVALID;
    }

    fbe_job_service_debug_hook_unlock();

    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: could not find hook job: 0x%llx\n", __FUNCTION__, job_hook_p->job_number);
        return status;
    }

    if (b_job_service_thread_released) {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: obj: 0x%x type: 0x%x state: %d phase: %d job: 0x%llx thread released!\n", 
                          __FUNCTION__, job_hook_p->object_id,
                          job_hook_p->hook_type, job_hook_p->hook_state,
                          job_hook_p->hook_phase, job_hook_p->job_number);
    }

    return FBE_STATUS_OK;
}
/*************************************** 
 * end fbe_job_service_del_debug_hook()
 **************************************/

/*!**************************************************************
 *          fbe_job_service_hook_add_debug_hook()
 **************************************************************** 
 * 
 * @brief   Add the requested hook to the job service.
 *
 * @param   packet_p - Pointer to control packet
 *
 * @return  status
 *
 * @author
 *  03/12/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_job_service_hook_add_debug_hook(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_service_add_debug_hook_t   *add_hook_p = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    /* Get the payload to set the configuration information.*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &add_hook_p);
    if (add_hook_p == NULL){
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* If length of the buffer does not match, return an error*/
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_job_service_add_debug_hook_t)){
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    job_service_trace_object(add_hook_p->debug_hook.object_id, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: hook type: 0x%x state: %d phase: %d action: %d \n", 
                             __FUNCTION__, add_hook_p->debug_hook.hook_type, 
                             add_hook_p->debug_hook.hook_state,
                             add_hook_p->debug_hook.hook_phase,
                             add_hook_p->debug_hook.hook_action);

    /* Set the hook */
    status = fbe_job_service_set_debug_hook(&add_hook_p->debug_hook);
    if (status != FBE_STATUS_OK) 
    { 
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s set debug hook failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/********************************************* 
 * end fbe_job_service_hook_add_debug_hook()
 ********************************************/

/*!**************************************************************
 *          fbe_job_service_hook_get_debug_hook()
 **************************************************************** 
 * 
 * @brief   Get the requested hook to the job service.
 *
 * @param   packet_p - Pointer to control packet
 *
 * @return  status
 *
 * @author
 *  03/12/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_job_service_hook_get_debug_hook(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_service_get_debug_hook_t   *get_hook_p = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    /* Get the payload to set the configuration information.*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_hook_p);
    if (get_hook_p == NULL){
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* If length of the buffer does not match, return an error*/
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_job_service_get_debug_hook_t)){
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    job_service_trace_object(get_hook_p->debug_hook.object_id, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: hook type: 0x%x state: %d phase: %d action: %d \n", 
                             __FUNCTION__, get_hook_p->debug_hook.hook_type, 
                             get_hook_p->debug_hook.hook_state,
                             get_hook_p->debug_hook.hook_phase,
                             get_hook_p->debug_hook.hook_action);

    /* Get the debug hook*/
    status = fbe_job_service_get_debug_hook(&get_hook_p->debug_hook);
    if (status != FBE_STATUS_OK) 
    { 
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: get scheduler hook for obj: 0x%x failed. status: 0x%x\n", 
                        __FUNCTION__, get_hook_p->debug_hook.object_id, status);
        get_hook_p->b_hook_found = FBE_FALSE;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        get_hook_p->b_hook_found = FBE_TRUE;
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/********************************************* 
 * end fbe_job_service_hook_get_debug_hook()
 ********************************************/

/*!**************************************************************
 *          fbe_job_service_hook_get_debug_hook()
 **************************************************************** 
 * 
 * @brief   Remove the requested hook to the job service.
 *
 * @param   packet_p - Pointer to control packet
 *
 * @return  status
 *
 * @author
 *  03/12/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_job_service_hook_remove_debug_hook(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_service_remove_debug_hook_t *remove_hook_p = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    /* Get the payload to set the configuration information.*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &remove_hook_p);
    if (remove_hook_p == NULL){
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* If length of the buffer does not match, return an error*/
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_job_service_remove_debug_hook_t)){
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    job_service_trace_object(remove_hook_p->debug_hook.object_id, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: hook type: 0x%x state: %d phase: %d action: %d \n", 
                             __FUNCTION__, remove_hook_p->debug_hook.hook_type, 
                             remove_hook_p->debug_hook.hook_state,
                             remove_hook_p->debug_hook.hook_phase,
                             remove_hook_p->debug_hook.hook_action);

    /* Get the debug hook*/
    status = fbe_job_service_del_debug_hook(&remove_hook_p->debug_hook);
    if (status != FBE_STATUS_OK) 
    { 
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: remove scheduler hook for obj: 0x%x failed. status: 0x%x\n", 
                        __FUNCTION__, remove_hook_p->debug_hook.object_id, status);
        remove_hook_p->b_hook_found = FBE_FALSE;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        remove_hook_p->b_hook_found = FBE_TRUE;
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/********************************************* 
 * end fbe_job_service_hook_remove_debug_hook()
 ********************************************/

/*!**************************************************************
 *          fbe_job_service_hook_check_hook_and_take_action()
 **************************************************************** 
 * 
 * @brief   Check if any hook has been reached and take the action
 *          if it has.
 *
 * @param   job_queue_element_p - Pointer to job element.
 * @param   b_state_start - TRUE - Start pahse else End
 *
 * @return  status
 *
 * @author
 *  03/14/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_job_service_hook_check_hook_and_take_action(fbe_job_queue_element_t *job_queue_element_p,
                                                             fbe_bool_t b_state_start)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_job_service_debug_hook_t   *hook_p = NULL;
    fbe_job_debug_hook_state_phase_t hook_phase = FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_END;

    /* If no hooks are set then return.
     */
    if (fbe_job_service_hooks_in_use == 0) {
        return FBE_STATUS_OK;
    }

    /* Determine the transition 
     */
    if (b_state_start) {
        hook_phase = FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START;
    }

    /* Check if there is a corresponding hook */
    fbe_job_service_debug_hook_lock();
    status = fbe_job_service_locate_debug_hook(job_queue_element_p->object_id, 
                                               job_queue_element_p->job_type,
                                               job_queue_element_p->current_state,
                                               hook_phase,
                                               &hook_p);
    if (status != FBE_STATUS_OK) {
        /* Nothing to do */
        fbe_job_service_debug_hook_unlock();
        return FBE_STATUS_OK;
    }

    /* If found it increment the count and set the job number.
     */
    hook_p->hit_counter++;
    hook_p->job_number = job_queue_element_p->job_number;

    fbe_job_service_debug_hook_unlock();

    job_service_trace_object(hook_p->object_id, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: hook type: 0x%x state: %d phase: %d action: %d \n", 
                             __FUNCTION__, hook_p->hook_type, 
                             hook_p->hook_state,
                             hook_p->hook_phase,
                             hook_p->hook_action);

    /* If the action is to PAUSE do that */
    if (hook_p->hook_action == FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_PAUSE) {
        fbe_semaphore_wait(&fbe_job_service_hook_sem, NULL);
    }

    /* Return success
     */
    return status;

}
/******************************************************* 
 * end fbe_job_service_hook_check_hook_and_take_action()
 *******************************************************/


/********************************
 *end of fbe_job_service_hook.c
 *******************************/
