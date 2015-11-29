/***************************************************************************
* Copyright (C) EMC Corporation 2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_client_bouncer.c
***************************************************************************
*
* @brief
*  This file implements client policies
*  
***************************************************************************/

#include "fbe_cms_client_private.h"
#include "fbe_cms_main.h"
#include "fbe_cms_operations.h"
#include "fbe_cms_private.h"
#include "fbe_cms.h"

static fbe_cms_client_table_t fbe_cms_client_table;

/*! 
 * @brief 
 * 
 * @version
 *   2011-11-28 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_client_init()
{
    fbe_zero_memory(&fbe_cms_client_table, sizeof(fbe_cms_client_table_t));

    fbe_spinlock_init(&fbe_cms_client_table.lock);

    return(FBE_STATUS_OK);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-11-28 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_client_destroy()
{
    fbe_spinlock_destroy(&fbe_cms_client_table.lock);

    return(FBE_STATUS_OK);
}


/*! 
 * @brief 
 * 
 * @version
 *   2011-11-22 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_bouncer_register_client(fbe_cms_client_id_t            client_id,
                                     fbe_cms_client_policy_type_t   policy,
                                     void *                         event_callback,
                                     fbe_u64_t                      memory_reservation,
                                     fbe_u64_t                      memory_limit,
                                     fbe_u32_t                      min_owner_ids,
                                     fbe_u32_t                      max_owner_ids,
                                     fbe_u32_t                      avg_owner_ids)
{
    fbe_cms_client_data_t * client;
    fbe_u32_t owner_memory_size = 0;
    void * owner_memory = NULL;

    fbe_spinlock_lock(&fbe_cms_client_table.lock);

    /* Bounds check to make sure we don't run off the end of the table */
    if(client_id >= FBE_CMS_CLIENT_LAST)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Client ID is out of bounds: 0x%X", __FUNCTION__, client_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    client = &(fbe_cms_client_table.client_data[client_id]);

    /* Check to make sure we're not already registered */
    if(client->registered)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Client ID is oalready registered: 0x%X", __FUNCTION__, client_id);;
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* We only support the explicit policy, which requires secondary
       registration of owner_ids before we'll allocate memory.
     */
    if(policy != FBE_CMS_CLIENT_POLICY_EXPLICIT)
    {
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* I'm being very lazy for now, and just allocating space for a giant array of pointers.
       This means that each client consumes 128k of memory.
     */
    owner_memory_size   = sizeof(void *) * (2 ^ CMS_OWNER_ID_NUMBER_OF_NON_CLIENT_BITS);
    owner_memory        = fbe_cms_cmm_memory_allocate(owner_memory_size);
    if(owner_memory == NULL)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Failed to get memory for Client ID: 0x%X", __FUNCTION__, client_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_zero_memory(owner_memory, owner_memory_size);

    fbe_zero_memory(client, sizeof(fbe_cms_client_data_t));

    fbe_queue_init(&client->queued_requests);

    client->registered          = FBE_TRUE;
    client->client_id           = client_id;
    client->policy              = policy;
    client->event_callback      = event_callback;
    client->memory_reservation  = memory_reservation;
    client->memory_limit        = memory_limit;
    client->min_owner_ids       = min_owner_ids;
    client->max_owner_ids       = max_owner_ids;
    client->avg_owner_ids       = avg_owner_ids;

    client->owner_data          = (fbe_cms_client_owner_data_t *)owner_memory;

    client->number_of_owner_ids         = 0;
    client->number_of_bytes_allocated   = 0;
    client->number_of_allocations       = 0;
    client->number_of_shares            = 0;

    fbe_cms_client_table.number_of_clients++;

    fbe_spinlock_unlock(&fbe_cms_client_table.lock);

    return(FBE_STATUS_OK);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-11-22 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_bouncer_unregister_client(fbe_cms_client_id_t client_id)
{
    fbe_cms_client_data_t * client;

    fbe_spinlock_lock(&fbe_cms_client_table.lock);

    /* Bounds check to make sure we don't run off the end of the table */
    if(client_id >= FBE_CMS_CLIENT_LAST)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Client ID is out of bounds: 0x%X", __FUNCTION__, client_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    client = &(fbe_cms_client_table.client_data[client_id]);

    /* Check to make sure we're already registered */
    if(! client->registered)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Client ID is not registered: 0x%X", __FUNCTION__, client_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Make sure there are no outstanding allocations */
    if(client->number_of_allocations > 0)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Client ID has outstanding allocations: 0x%X", __FUNCTION__, client_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Check for queued allocations */
    if(! fbe_queue_is_empty(&client->queued_requests))
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Client ID has queued allocations: 0x%X", __FUNCTION__, client_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Check for registered owner ids */
    if(client->number_of_owner_ids != 0)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Client ID has registered owners: 0x%X", __FUNCTION__, client_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_cms_cmm_memory_release(client->owner_data);
    client->owner_data = NULL;

    fbe_cms_client_table.number_of_clients--;

    fbe_spinlock_unlock(&fbe_cms_client_table.lock);

    return(FBE_STATUS_OK);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-11-22 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_bouncer_register_owner(fbe_cms_client_id_t             client_id,
                                    fbe_u64_t                       owner_id,
                                    fbe_u64_t                       memory_reservation,
                                    fbe_u64_t                       memory_limit,
                                    fbe_u32_t                       shares_value)
{
    fbe_cms_client_data_t       * client    = NULL;
    fbe_cms_client_owner_data_t * owner     = NULL;

    if(owner_id > CMS_OWNER_ID_MAX_VALUE)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_spinlock_lock(&fbe_cms_client_table.lock);

    /* Validate that the first few bits of the owner_id match the client_id*/
    if(client_id != CMS_OWNER_ID_GET_CLIENT_ID(owner_id)) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Client ID 0x%X Owner ID 0x%X mismatch", __FUNCTION__, client_id, owner_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    client = &(fbe_cms_client_table.client_data[client_id]);

    owner  = client->owner_data + CMS_OWNER_ID_GET_NON_CLIENT_PORTION(owner_id);

    /* Check to make sure we're not already registered */
    if(owner->registered || (! client->registered))
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Client ID 0x%X Owner ID 0x%X Owner registration mismatch", __FUNCTION__, client_id, owner_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_zero_memory(owner, sizeof(fbe_cms_client_owner_data_t));

    fbe_queue_init(&owner->queued_requests);

    owner->registered                   = FBE_TRUE;
    owner->client_id                    = client_id;
    owner->owner_id                     = owner_id;

    owner->memory_reservation           = memory_reservation;
    owner->memory_limit                 = memory_limit;
    owner->shares_value                 = shares_value;

    owner->number_of_bytes_allocated    = 0;
    owner->number_of_allocations        = 0;

    client->number_of_owner_ids++;

    fbe_spinlock_unlock(&fbe_cms_client_table.lock);

    return(FBE_STATUS_OK);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-11-22 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_bouncer_unregister_owner(fbe_cms_client_id_t           client_id,
                                      fbe_u64_t                     owner_id)
{
    fbe_cms_client_data_t       * client    = NULL;
    fbe_cms_client_owner_data_t * owner     = NULL;

    /* Bounds check */
    if(owner_id > CMS_OWNER_ID_MAX_VALUE)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    /* Validate that the first few bits of the owner_id match the client_id*/
    if(client_id != CMS_OWNER_ID_GET_CLIENT_ID(owner_id)) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Client ID 0x%X Owner ID 0x%X mismatch", __FUNCTION__, client_id, owner_id);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_spinlock_lock(&fbe_cms_client_table.lock);

    client = &(fbe_cms_client_table.client_data[client_id]);

    owner  = client->owner_data + CMS_OWNER_ID_GET_NON_CLIENT_PORTION(owner_id);

    /* Check to make sure we're already registered */
    if(! owner->registered)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Owner ID 0x%X Owner not registered", __FUNCTION__, owner_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Check for outstanding allocations */
    if(owner->number_of_allocations > 0)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Owner ID 0x%X Owner has outstanding allocations", __FUNCTION__, owner_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Check for queued allocations */
    if(! fbe_queue_is_empty(&owner->queued_requests))
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Owner ID 0x%X Owner has queued requests", __FUNCTION__, owner_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    client->number_of_owner_ids--;

    owner->registered = FBE_FALSE;

    fbe_spinlock_unlock(&fbe_cms_client_table.lock);

    return(FBE_STATUS_OK);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-11-28 - Created. Matthew Ferson
 */
fbe_bool_t fbe_cms_policy_passes_owner_check(fbe_cms_client_data_t * client,
                                             fbe_cms_client_owner_data_t * owner,
                                             fbe_cms_buff_req_t * buf_req)
{
    /* Enforce policy for each client */
    if(client->policy == FBE_CMS_CLIENT_POLICY_EXPLICIT)
    {
        if(owner->memory_limit == 0)
        {
            return FBE_TRUE;
        }
    
        if((owner->number_of_bytes_allocated + buf_req->m_alloc_size) <= owner->memory_limit)
        {
            return FBE_TRUE;
        }
    }

    return FBE_FALSE;
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-11-28 - Created. Matthew Ferson
 */
fbe_bool_t fbe_cms_policy_passes_client_check(fbe_cms_client_data_t * client,
                                              fbe_cms_buff_req_t * buf_req)
{
    fbe_cms_client_data_t * temp_client = NULL;
    fbe_u64_t local_memory_bytes        = 0;
    fbe_u64_t unused_reserved_bytes     = 0;
    fbe_u64_t local_unreserved_bytes    = 0;
    fbe_u32_t i                         = 0;

    /* If it fits within the reserved memory, the allocation is always allowed */
    if((client->number_of_bytes_allocated + buf_req->m_alloc_size) <= client->memory_reservation)
    {
        return FBE_TRUE;
    }

    /* Don't allow us to consume somebody else's reserved memory */
    for(i = 0; i < FBE_CMS_CLIENT_LAST; i++)
    {
        temp_client = &fbe_cms_client_table.client_data[i];
        if((temp_client != client) && (temp_client->number_of_bytes_allocated < temp_client->memory_reservation))
        {
            unused_reserved_bytes += (temp_client->memory_reservation - temp_client->number_of_bytes_allocated);
        }
    }

    local_unreserved_bytes = (local_memory_bytes - (fbe_cms_client_table.number_of_bytes_allocated + unused_reserved_bytes));

    if(buf_req->m_alloc_size > local_unreserved_bytes)
    {
        return FBE_FALSE;
    }


    /* If we haven't set a limit, the allocation is allowed */
    if(client->memory_limit == 0)
    {
        return FBE_TRUE;
    }

    /* If we're under the limit, the allocation is allowed */
    if((client->number_of_bytes_allocated + buf_req->m_alloc_size) <= client->memory_limit)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-11-28 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_bouncer_allocate(fbe_cms_buff_req_t * buf_req)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_cms_client_id_t         client_id;
    fbe_u64_t                   owner_id;
    fbe_cms_client_data_t       * client    = NULL;
    fbe_cms_client_owner_data_t * owner     = NULL;

    if((buf_req->m_alloc_id.owner_id == NULL) || (buf_req->m_alloc_id.buff_id == NULL))
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    owner_id = buf_req->m_alloc_id.owner_id;

    /* Bounds check */
    if(owner_id > CMS_OWNER_ID_MAX_VALUE)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Owner ID 0x%X is out of bounds", __FUNCTION__, owner_id);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_spinlock_lock(&fbe_cms_client_table.lock);

    client_id   = CMS_OWNER_ID_GET_CLIENT_ID(owner_id);
    client      = &(fbe_cms_client_table.client_data[client_id]);

    /* Check to make sure the client is already registered */
    if(! client->registered)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Client ID 0x%X is not registered", __FUNCTION__, client_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    owner       = client->owner_data + CMS_OWNER_ID_GET_NON_CLIENT_PORTION(owner_id);

    /* Check to make sure the owner is already registered */
    if(! owner->registered)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Owner ID 0x%X is not registered", __FUNCTION__, owner_id);
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* This owner has too much memory allocated - add to the owner wait queue */
    if(! fbe_cms_policy_passes_owner_check(client, owner, buf_req))
    {
        fbe_queue_push(&owner->queued_requests, (fbe_queue_element_t *)buf_req);
    }
    /* This client has too much memory allocated - add to the client wait queue */
    else if(! fbe_cms_policy_passes_client_check(client, buf_req))
    {
        fbe_queue_push(&client->queued_requests, (fbe_queue_element_t *)buf_req);
    }
    /* Passed policy checks */
    else
    {
        if(owner->number_of_allocations == 0)
        {
            /*! @todo Notify clean/dirty module that the owner_id is going dirty */
        }
        fbe_cms_client_table.number_of_bytes_allocated += buf_req->m_alloc_size;
        fbe_cms_client_table.number_of_allocations++;

        client->number_of_bytes_allocated += buf_req->m_alloc_size;
        client->number_of_allocations++;

        owner->number_of_bytes_allocated += buf_req->m_alloc_size;
        owner->number_of_allocations++;

        status = fbe_cms_operations_handle_incoming_req(buf_req);
    }

    fbe_spinlock_unlock(&fbe_cms_client_table.lock);

    return(status);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-11-28 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_bouncer_abort(fbe_cms_buff_req_t * buf_req)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_buff_req_t          * next_req;
    fbe_cms_client_id_t         client_id;
    fbe_u64_t                   owner_id;
    fbe_cms_client_data_t       * client    = NULL;
    fbe_cms_client_owner_data_t * owner     = NULL;
    fbe_bool_t                  found_req   = FBE_FALSE;

    owner_id = buf_req->m_alloc_id.owner_id;

    /* Bounds check */
    if(owner_id > CMS_OWNER_ID_MAX_VALUE)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_spinlock_lock(&fbe_cms_client_table.lock);

    client_id   = CMS_OWNER_ID_GET_CLIENT_ID(owner_id);
    client      = &(fbe_cms_client_table.client_data[client_id]);

    /* Check to make sure the client is already registered */
    if(! client->registered)
    {
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    owner       = client->owner_data + CMS_OWNER_ID_GET_NON_CLIENT_PORTION(owner_id);

    /* Check to make sure the owner is already registered */
    if(! owner->registered)
    {
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* First, see if the request is on any of the policy engine's queues */
    next_req = (fbe_cms_buff_req_t *)fbe_queue_next(&client->queued_requests, &client->queued_requests);
    while(next_req != NULL)
    {
        if((next_req->m_alloc_id.owner_id == buf_req->m_alloc_id.owner_id)
           && (next_req->m_alloc_id.buff_id == buf_req->m_alloc_id.buff_id))
        {
            found_req = FBE_TRUE;
            fbe_queue_remove((fbe_queue_element_t *)next_req);
        }
        next_req = (fbe_cms_buff_req_t *)fbe_queue_next(&client->queued_requests, (fbe_queue_element_t *)next_req);
    }

    next_req = (fbe_cms_buff_req_t *)fbe_queue_next(&owner->queued_requests, &owner->queued_requests);
    while(next_req != NULL)
    {
        if((next_req->m_alloc_id.owner_id == buf_req->m_alloc_id.owner_id)
           && (next_req->m_alloc_id.buff_id == buf_req->m_alloc_id.buff_id))
        {
            found_req = FBE_TRUE;
            fbe_queue_remove((fbe_queue_element_t *)next_req);
        }
        next_req = (fbe_cms_buff_req_t *)fbe_queue_next(&owner->queued_requests, (fbe_queue_element_t *)next_req);
    }

    if(found_req = FBE_TRUE)
    {
        status = FBE_STATUS_OK;
        /*! @todo need to call the completion function in buf_req here?*/
    }
    else 
    {
        /* We didn't find the request on our queues, so pass it on to
           the Operations module to see if it's on one ofthose queues
         */
        status = fbe_cms_operations_handle_incoming_req(buf_req);
    }

    fbe_spinlock_unlock(&fbe_cms_client_table.lock);

    return(FBE_STATUS_OK);
}


/*! 
 * @brief 
 * 
 * @version
 *   2011-11-28 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_policy_free_abort_notify(fbe_cms_buff_req_t * buf_req)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_cms_client_id_t         client_id;
    fbe_u64_t                   owner_id;
    fbe_cms_client_data_t       * client    = NULL;
    fbe_cms_client_owner_data_t * owner     = NULL;
    fbe_cms_buff_req_t          * next_req  = NULL;

    if((buf_req->m_alloc_id.owner_id == NULL) || (buf_req->m_alloc_id.buff_id == NULL))
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    owner_id = buf_req->m_alloc_id.owner_id;

    /* Bounds check */
    if(owner_id > CMS_OWNER_ID_MAX_VALUE)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_spinlock_lock(&fbe_cms_client_table.lock);

    client_id   = CMS_OWNER_ID_GET_CLIENT_ID(owner_id);
    client      = &(fbe_cms_client_table.client_data[client_id]);

    /* Check to make sure the client is already registered */
    if(! client->registered)
    {
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    owner       = client->owner_data + CMS_OWNER_ID_GET_NON_CLIENT_PORTION(owner_id);

    /* Check to make sure the owner is already registered */
    if(! owner->registered)
    {
        fbe_spinlock_unlock(&fbe_cms_client_table.lock);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_cms_client_table.number_of_bytes_allocated -= buf_req->m_alloc_size;
    fbe_cms_client_table.number_of_allocations--;
    client->number_of_bytes_allocated -= buf_req->m_alloc_size;
    client->number_of_allocations--;
    owner->number_of_bytes_allocated -= buf_req->m_alloc_size;
    owner->number_of_allocations--;

    if(owner->number_of_allocations > 0)
    {
        /*! @todo Notify clean/dirty module that the owner_id is going clean */
    }


    /* See if this free/abort allows us to move a request from the owner queue down to the client queue */
    next_req = (fbe_cms_buff_req_t *)fbe_queue_front(&owner->queued_requests);
    while(next_req != NULL)
    {
        if(fbe_cms_policy_passes_owner_check(client, owner, next_req))
        {
            fbe_queue_remove((fbe_queue_element_t *)next_req);
            fbe_queue_push(&client->queued_requests, (fbe_queue_element_t *)next_req);
            next_req = (fbe_cms_buff_req_t *)fbe_queue_front(&owner->queued_requests);
        }
        else
        {
            break;
        }
    }

    /* See if this free/abort allows us to move a request from the client queue to the lower level allocator */
    next_req = (fbe_cms_buff_req_t *)fbe_queue_front(&client->queued_requests);
    while(next_req != NULL)
    {
        if(fbe_cms_policy_passes_client_check(client, next_req))
        {
            fbe_queue_remove((fbe_queue_element_t *)next_req);

            status = fbe_cms_operations_handle_incoming_req(next_req);

            next_req = (fbe_cms_buff_req_t *)fbe_queue_front(&client->queued_requests);
        }
        else
        {
            break;
        }
    }
    fbe_spinlock_unlock(&fbe_cms_client_table.lock);

    return(status);
}
