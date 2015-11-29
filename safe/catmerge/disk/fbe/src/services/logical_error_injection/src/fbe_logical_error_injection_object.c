/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_error_injection_object.c
 ***************************************************************************
 *
 * @brief
 *  This file contains rdgen object related functions.
 *
 * @version
 *   12/21/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_logical_error_injection_private.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_logical_error_injection_proto.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_logical_error_injection_object_init()
 ****************************************************************
 * @brief
 *  This function initializes an rdgen object.
 *
 * @param object_p - Object to initialize.
 * @param object_id - The object id to init.
 * @param package_id - The package this object is in.
 *
 * @return None.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_object_init(fbe_logical_error_injection_object_t *object_p,
                                                     fbe_object_id_t object_id,
                                                     fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_zero_memory(object_p, sizeof(fbe_logical_error_injection_object_t));
    fbe_spinlock_init(&object_p->lock);
    fbe_logical_error_injection_object_init_active_record_count(object_p);
    object_p->object_id = object_id;
    object_p->package_id = package_id;
    object_p->capacity = FBE_LBA_INVALID;
    object_p->edge_hook_bitmask = FBE_U32_MAX;
    object_p->injection_lba_adjustment = 0;
    fbe_queue_element_init(&object_p->queue_element);

    if (object_id == LEI_RAW_MIRROR_OBJECT_ID)
    {
        object_p->class_id = FBE_CLASS_ID_INVALID;
    }
    else
    {
        status = logical_error_injection_get_object_class_id(object_id, &object_p->class_id, package_id);
    }
    return status;
}
/******************************************
 * end fbe_logical_error_injection_object_init()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_object_prepare_destroy()
 ****************************************************************
 * @brief
 *  This function prepares to destroy an object.
 *  We will wait until all I/Os have completed.
 * 
 * @param object_p - Object to enable.
 * 
 * @notes, the service lock and object lock is assumed to be held.
 * 
 * @return None.
 *
 * @author
 *  4/18/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_object_prepare_destroy(fbe_logical_error_injection_object_t *object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t in_progress_count;
    fbe_u32_t msec = 0;

    /* Set disabled so that no more injections start on this object.
     */
    fbe_logical_error_injection_object_set_enabled(object_p, FBE_FALSE);

    /* Before we destroy the object, wait for in progress errors to stop.
     */
    fbe_logical_error_injection_object_get_in_progress(object_p, &in_progress_count);
    while (in_progress_count != 0)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "LEI: %d in progress errors for object_id 0x%x\n", 
                                                  in_progress_count, object_p->object_id);
        fbe_logical_error_injection_object_unlock(object_p);
        fbe_logical_error_injection_unlock();

        /* Wait for the injections to finish.
         */
        fbe_thread_delay(FBE_LOGICAL_ERROR_INJECTION_OBJECT_DESTROY_WAIT_MSEC);
        msec += FBE_LOGICAL_ERROR_INJECTION_OBJECT_DESTROY_WAIT_MSEC;

        fbe_logical_error_injection_lock();
        fbe_logical_error_injection_object_lock(object_p);
        fbe_logical_error_injection_object_get_in_progress(object_p, &in_progress_count);

        /* If we loop too long, then we will return an error.
         */
        if (msec > FBE_LOGICAL_ERROR_INJECTION_OBJECT_DESTROY_MAX_MSEC)
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                      "LEI: Unable to destroy %d in progress errors for object_id 0x%x\n", 
                                                      in_progress_count, object_p->object_id);
            status = FBE_STATUS_TIMEOUT;
            break;
        }
    }

    return status;
}
/******************************************
 * end fbe_logical_error_injection_object_prepare_destroy()
 ******************************************/
/*!**************************************************************
 * fbe_logical_error_injection_object_destroy()
 ****************************************************************
 * @brief
 *  This function destroys an error injection object.
 * 
 * @param object_p - Object to enable.
 *
 * @return None.
 *
 * @author
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_logical_error_injection_object_destroy(fbe_logical_error_injection_object_t *object_p)
{
    fbe_spinlock_destroy(&object_p->lock);
    return;
}
/******************************************
 * end fbe_logical_error_injection_object_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_object_modify()
 ****************************************************************
 * @brief
 *  This function will enable error injection on a single
 *  error injection object.
 * 
 *  We will establish the hook on all edges for the object.
 *
 * @param object_p - Object to enable.
 * @param edge_enable_bitmask - Bitmask of edge (positions) to enable
 *          the edge hook for.
 * @param injection_lba_adjustment - Adjust error injection lba by this
 *          amount.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/22/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_object_modify(fbe_logical_error_injection_object_t *object_p,
                                                       fbe_u32_t edge_enable_bitmask,
                                                       fbe_lba_t injection_lba_adjustment)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Simply set the bitmask.
     */
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s: obj: 0x%x edge bitmask: 0x%x lba adjust: 0x%llx \n", 
                                              __FUNCTION__,  object_p->object_id,
                                              edge_enable_bitmask, (unsigned long long)injection_lba_adjustment);

    /* Do not set the hook for raw mirror object.
     * For raw mirrors, errors are injected on the raw mirror PVDs, but I/O is driven through
     * the raw mirror, so it needs an object ID, but no hook.
     */
    if (object_p->object_id != LEI_RAW_MIRROR_OBJECT_ID)
    {
        object_p->edge_hook_bitmask = edge_enable_bitmask;
    }
    object_p->injection_lba_adjustment = injection_lba_adjustment;

    return status;
}
/*************************************************
 * end fbe_logical_error_injection_object_modify()
 *************************************************/

/*!**************************************************************
 * fbe_logical_error_injection_object_enable()
 ****************************************************************
 * @brief
 *  This function will enable error injection on a single
 *  error injection object.
 * 
 *  We will establish the hook on all edges for the object.
 *
 * @param object_p - Object to enable.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/22/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_object_enable(fbe_logical_error_injection_object_t *object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_transport_control_set_edge_tap_hook_t hook_info;

    if (object_p->b_enabled == FBE_FALSE)
    {
        /* Since we are re-enabling, init the counters.
         */
        fbe_logical_error_injection_object_set_num_rd_media_errors(object_p, 0);
        fbe_logical_error_injection_object_set_num_wr_remaps(object_p, 0);
        fbe_logical_error_injection_object_set_num_errors(object_p, 0);

        hook_info.edge_index = FBE_U32_MAX; /* Set all edges. */
        hook_info.edge_bitmask = object_p->edge_hook_bitmask;
        hook_info.edge_tap_hook = fbe_logical_error_injection_hook_function;
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: enable for object: 0x%x package: 0x%x.\n", 
                                                  __FUNCTION__, object_p->object_id, object_p->package_id);
        fbe_logical_error_injection_lock();
        fbe_logical_error_injection_object_lock(object_p);
        object_p->b_enabled = FBE_TRUE;
        fbe_logical_error_injection_inc_num_objects_enabled();

        fbe_logical_error_injection_object_unlock(object_p);
        fbe_logical_error_injection_unlock();

        /* Do not set the hook for raw mirror object.
         * For raw mirrors, errors are injected on the raw mirror PVDs, but I/O is driven through
         * the raw mirror, so it needs an object ID, but no hook.
         */
        if (object_p->object_id != LEI_RAW_MIRROR_OBJECT_ID)
        {
            status = fbe_logical_error_injection_set_block_edge_hook(object_p->object_id,
                                                                     object_p->package_id,
                                                                     &hook_info);
        }
    }
    return status;
}
/******************************************
 * end fbe_logical_error_injection_object_enable()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_object_disable()
 ****************************************************************
 * @brief
 *  This function will enable error injection on a single
 *  error injection object.
 *
 * @param object_p - Object to enable.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/22/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_object_disable(fbe_logical_error_injection_object_t *object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_transport_control_set_edge_tap_hook_t hook_info;

    fbe_logical_error_injection_object_lock(object_p);
    /* Only disable if enabled.
     */
    if (object_p->b_enabled == FBE_TRUE)
    {
        hook_info.edge_index = FBE_U32_MAX;    /* Set all edges. */
        hook_info.edge_bitmask = object_p->edge_hook_bitmask;
        hook_info.edge_tap_hook = NULL;        /* Null to remove hook. */

        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: disable for object: 0x%x package: 0x%x.\n", 
                                                  __FUNCTION__, object_p->object_id, object_p->package_id);

        object_p->b_enabled = FBE_FALSE;
        fbe_logical_error_injection_dec_num_objects_enabled();
        fbe_logical_error_injection_object_unlock(object_p);

        /* Do not set the hook for raw mirror object.
         * For raw mirrors, errors are injected on the raw mirror PVDs, but I/O is driven through
         * the raw mirror, so it needs an object ID, but no hook.
         */
        if (object_p->object_id != LEI_RAW_MIRROR_OBJECT_ID)
        {
            status = fbe_logical_error_injection_set_block_edge_hook(object_p->object_id,
                                                                     object_p->package_id,
                                                                     &hook_info);
            object_p->edge_hook_bitmask = FBE_U32_MAX;
        }
        object_p->injection_lba_adjustment = 0;

        /* When disabling error injection on an object, we need to wake up the 
         * delay thread to release any packets that were held on that object
         */
        fbe_logical_error_injection_wakeup_delay_thread();
    }
    else
    {
        fbe_logical_error_injection_object_unlock(object_p);
    }

    return status;
}
/******************************************
 * end fbe_logical_error_injection_object_disable()
 ******************************************/
/*!**************************************************************
 * fbe_logical_error_injection_object_modify_for_class()
 ****************************************************************
 * @brief
 *  This function will enable error injection for the object specified
 *  use the bitmask passed to determine which edges are enabled.
 *
 * @param object_p - Object to enable.
 * @param edge_enable_bitmask - Bitmask of edges to enable
 * @param injection_lba_adjustment - Adjust the lba to inject on by
 *          this amount.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/22/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_object_modify_for_class(fbe_logical_error_injection_filter_t *filter_p,
                                                                 fbe_u32_t edge_enable_bitmask,
                                                                 fbe_lba_t injection_lba_adjustment)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t object_index;
    fbe_u32_t total_objects = 0;
    fbe_u32_t actual_num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;

    status = fbe_logical_error_injection_get_total_objects_of_class(filter_p->class_id, filter_p->package_id, &total_objects);
    if (status != FBE_STATUS_OK) 
    {
        return status;
    }

    if (total_objects == 0)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: zero objects of class %d found in package %d\n", 
                                                  __FUNCTION__, filter_p->class_id, filter_p->package_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    obj_list_p = (fbe_object_id_t *)fbe_memory_native_allocate(sizeof(fbe_object_id_t) * total_objects);
    if (obj_list_p == NULL) 
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: cannot allocate memory for enumerate of %d class  package: %d objs: %d\n", 
                                                  __FUNCTION__, filter_p->class_id, filter_p->package_id, total_objects);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_set_memory(obj_list_p, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_logical_error_injection_enumerate_class_exclude_system_objects(filter_p->class_id,
                                                                                filter_p->package_id,
                                                                                obj_list_p, total_objects, &actual_num_objects);
    if (status != FBE_STATUS_OK)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: error from enumerate status %d.\n", 
                                                  __FUNCTION__, status);
        fbe_memory_native_release(obj_list_p);
        return status;
    }
    if (actual_num_objects == 0)
    {
        /* No objects found.  Indicate error.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "%s no objects of this class found: 0x%x package: 0x%x", 
                                                  __FUNCTION__, filter_p->class_id, filter_p->package_id);
        fbe_memory_native_release(obj_list_p);
        return FBE_STATUS_OK;
    }

    for (object_index = 0; object_index < actual_num_objects; object_index++)
    {
        fbe_object_id_t object_id = obj_list_p[object_index];
        fbe_logical_error_injection_object_t *object_p = NULL;

        /* Now get the object.
         */
        status = fbe_logical_error_injection_get_object(object_id, filter_p->package_id, &object_p);
        if (status != FBE_STATUS_OK)
        {
            /* Out of resources, cannot allocate object.
             */
            break;
        }
        else
        {
            status = fbe_logical_error_injection_object_modify(object_p, 
                                                               edge_enable_bitmask,
                                                               injection_lba_adjustment);
        }

        if (status != FBE_STATUS_OK)
        {
            /* Out of resources, cannot allocate object.
             */
            fbe_memory_native_release(obj_list_p);
            return status;
        }
    } /* end for all objects to start on */

    fbe_memory_native_release(obj_list_p);

    /* First enumerate the class to get a list of objects.
     */
    return status;
}
/******************************************
 * end fbe_logical_error_injection_set_edge_enable_mask_class()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_object_enable_class()
 ****************************************************************
 * @brief
 *  This function will enable error injection on a single
 *  error injection object.
 *
 * @param object_p - Object to enable.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/22/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_object_enable_class(fbe_logical_error_injection_filter_t *filter_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t object_index;
    fbe_u32_t total_objects = 0;
    fbe_u32_t actual_num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;

    status = fbe_logical_error_injection_get_total_objects_of_class(filter_p->class_id, filter_p->package_id, &total_objects);
    if (status != FBE_STATUS_OK) 
    {
        return status;
    }

    if (total_objects == 0)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: zero objects of class %d found in package %d\n", 
                                                  __FUNCTION__, filter_p->class_id, filter_p->package_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    obj_list_p = (fbe_object_id_t *)fbe_memory_native_allocate(sizeof(fbe_object_id_t) * total_objects);
    if (obj_list_p == NULL) 
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: cannot allocate memory for enumerate of %d class  package: %d objs: %d\n", 
                                                  __FUNCTION__, filter_p->class_id, filter_p->package_id, total_objects);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_set_memory(obj_list_p, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_logical_error_injection_enumerate_class_exclude_system_objects(filter_p->class_id,
                                                                                filter_p->package_id,
                                                                                obj_list_p, total_objects, &actual_num_objects);
    if (status != FBE_STATUS_OK)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: error from enumerate status %d.\n", 
                                                  __FUNCTION__, status);
        fbe_memory_native_release(obj_list_p);
        return status;
    }
    if (actual_num_objects == 0)
    {
        /* No objects found.  Indicate error.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "%s no objects of this class found: 0x%x package: 0x%x", 
                                                  __FUNCTION__, filter_p->class_id, filter_p->package_id);
        fbe_memory_native_release(obj_list_p);
        return FBE_STATUS_OK;
    }

    for (object_index = 0; object_index < actual_num_objects; object_index++)
    {
        fbe_object_id_t object_id = obj_list_p[object_index];
        fbe_logical_error_injection_object_t *object_p = NULL;

        /* Now get the object.
         */
        status = fbe_logical_error_injection_get_object(object_id, filter_p->package_id, &object_p);
        if (status != FBE_STATUS_OK)
        {
            /* Out of resources, cannot allocate object.
             */
            break;
        }
        else
        {
            status = fbe_logical_error_injection_object_enable(object_p);
        }

        if (status != FBE_STATUS_OK)
        {
            /* Out of resources, cannot allocate object.
             */
            fbe_memory_native_release(obj_list_p);
            return status;
        }
    } /* end for all objects to start on */

    fbe_memory_native_release(obj_list_p);

    /* First enumerate the class to get a list of objects.
     */
    return status;
}
/******************************************
 * end fbe_logical_error_injection_object_enable_class()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_object_disable_class()
 ****************************************************************
 * @brief
 *  This function will disable error injection on a single
 *  error injection object.
 *
 * @param object_p - Object to disable.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/22/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_object_disable_class(fbe_logical_error_injection_filter_t *filter_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_queue_head_t *queue_head_p = NULL;

    fbe_logical_error_injection_lock();
    fbe_logical_error_injection_get_active_object_head(&queue_head_p);

    queue_element_p = fbe_queue_front(queue_head_p);

    /* Simply mark each item aborted, and then wait for it to finish.
     */
    while (queue_element_p != NULL)
    {
        fbe_logical_error_injection_object_t *object_p;
        object_p = (fbe_logical_error_injection_object_t*) queue_element_p;

        /* Disable each object.
         */
        fbe_logical_error_injection_unlock();
        status = fbe_logical_error_injection_object_disable(object_p);
        fbe_logical_error_injection_lock();

        /* restart from the beginning of the queue and look for the next one need to disable */
        queue_element_p = fbe_queue_front(queue_head_p);
        while (queue_element_p != NULL){
            object_p = (fbe_logical_error_injection_object_t*) queue_element_p;
            if (object_p->b_enabled == FBE_TRUE)
            {
                break;  /* this is has not been disabled */
            }
            queue_element_p = fbe_queue_next(queue_head_p, queue_element_p);
        }
    } /* while not at end of queue. */

    fbe_logical_error_injection_unlock();

    /* First enumerate the class to get a list of objects.
     */
    return status;
}
/******************************************
 * end fbe_logical_error_injection_object_disable_class()
 ******************************************/

/************************************************
 * end file fbe_logical_error_injection_object.c
 ************************************************/


