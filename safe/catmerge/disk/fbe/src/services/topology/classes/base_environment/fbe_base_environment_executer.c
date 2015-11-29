/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_environment_executer.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the base environment utilities to execute commands.
 *  (why does it always look like 'executioner' to me?)
 * 
 * 
 * @ingroup base_environment_class_files
 * 
 * @version
 *   30-Dec-2010:  Created. Brion Philbin
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_base_environment_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe_transport_memory.h"
#include "fbe_database.h"
#include "fbe/fbe_api_database_interface.h"

static fbe_status_t fbe_base_environment_read_persistent_data_completion(fbe_status_t read_status, 
                                                                  fbe_persist_entry_id_t next_entry, 
                                                                  fbe_u32_t entries_read, 
                                                                  fbe_persist_completion_context_t context);
static fbe_status_t fbe_base_environment_write_persistent_data_complete(fbe_status_t commit_status, 
                                                                        fbe_persist_completion_context_t context);
static fbe_status_t fbe_base_environment_is_sep_ready(fbe_base_environment_t *base_environment);
/*!**************************************************************
 * fbe_base_environment_read_persistent_data_completion()
 ****************************************************************
 * @brief
 *  This function checks if it is safe to read from the persistent
 *  data, if it is it allocates an 800k buffer and then issues
 *  an async request to the persistent storage service to read
 *  ESP data.
 * 
 * @param base_environment - pointer to our object's context
 *
 * @return fbe_status_t - The status 
 *
 * @author
 *  30-Dec-2010:  Created. Brion Philbin
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_send_read_persistent_data(fbe_base_environment_t *base_environment, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_environment_common_context_t *context;

    fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s function enter.\n",
                              __FUNCTION__);

    base_environment->read_outstanding = TRUE;
    
    /* Allocate a big contiguoous buffer to store the read data. */
    base_environment->persistent_storage_read_buffer = fbe_allocate_nonpaged_pool_with_tag(FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE, 'pfbe');
    if(base_environment->persistent_storage_read_buffer == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to allocate read buffer of size:%d bytes.\n",
                              __FUNCTION__, (int)FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE);
        fbe_transport_set_status(packet, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    status = fbe_base_environment_is_sep_ready(base_environment);
    if(status != FBE_STATUS_OK)
    {
        /* Free that big block of memory we allocated to store all the read data.*/
        fbe_release_nonpaged_pool_with_tag(base_environment->persistent_storage_read_buffer, 'pfbe');
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* 
     * Allocate memory to store our context, this will be used when the callback function 
     * is called after the read completes.
     */
    context = fbe_base_env_memory_ex_allocate(base_environment, sizeof(fbe_base_environment_common_context_t));
    context->base_environment = base_environment;
    context->packet = packet;
    fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s allocated context:%p\n",
                              __FUNCTION__, context);
    /* Kick off the async read, provide it with a callback function and context. */
    status = fbe_api_persist_read_sector(FBE_PERSIST_SECTOR_TYPE_ESP_OBJECTS,
                                base_environment->persistent_storage_read_buffer,
                                FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE,
                                FBE_PERSIST_SECTOR_START_ENTRY_ID,
                                fbe_base_environment_read_persistent_data_completion,
                                (fbe_persist_completion_context_t)context);

    fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s read sector resulted in status:0x%x\n",
                              __FUNCTION__, status);

    return status;
}

/*!**************************************************************
 * fbe_base_environment_read_persistent_data_completion()
 ****************************************************************
 * @brief
 *  This function handles the completion of reading 400 2k buffers.
 *  The buffer we are interested in will have a matching class ID
 *  in the header.  All buffers are sized the same at 2k.
 * 
 *  (A simple analogy for this is, you placed an order for 1 box and
 *  you got an entire delivery truck full of boxes.  Now you have
 *  to get your box out from that truck.)
 * 
 * @param read_status - Was the read successful
 * @param next_entry - next entry id or NULL if none
 * @param entries_read - number of entries read
 * @param context - pointer to our context
 *
 * @return fbe_status_t - The status 
 *
 * @author
 *  30-Dec-2010:  Created. Brion Philbin
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_read_persistent_data_completion(fbe_status_t read_status, 
                                                                  fbe_persist_entry_id_t next_entry, 
                                                                  fbe_u32_t entries_read, 
                                                                  fbe_persist_completion_context_t context)
{
    fbe_u32_t                   index;
    fbe_base_environment_t      *base_environment;
    fbe_u8_t                    *local_temp_buffer;
    fbe_status_t                data_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_environment_common_context_t *my_context;
    fbe_packet_t                *packet;
    fbe_base_environment_persistent_read_data_t *p_persistent_data;
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_environment_persistent_data_entry_t *p_persistent_data_entry;

    my_context = (fbe_base_environment_common_context_t *)context;
    
    base_environment = my_context->base_environment;
    packet = my_context->packet;

    local_temp_buffer = base_environment->persistent_storage_read_buffer;

    

    fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s function enter.\n",
                              __FUNCTION__);
    

    if(read_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s read completion status:0x%x\n",
                              __FUNCTION__, read_status);
        
    }
    else // good read_status
    {
        p_persistent_data_entry = (fbe_base_environment_persistent_data_entry_t *) local_temp_buffer;
        for(index = 0; index < entries_read; index++)
        {
            p_persistent_data = (fbe_base_environment_persistent_read_data_t *)&p_persistent_data_entry[index];
            if(p_persistent_data->header.persistent_id == base_environment->my_persistent_id)
            {
                /* 
                 * This is our persistent data copy it into our structure, the memory for this was 
                 * allocated at init time.
                 */
                base_environment->my_persistent_entry_id = p_persistent_data->persist_header.entry_id;
                fbe_copy_memory(base_environment->my_persistent_data, &p_persistent_data->header.data,
                        base_environment->my_persistent_data_size);
                data_status = FBE_STATUS_OK;
                break;
            }
            else if(index < 5) // only trace for the first five entries, we don't use more than that
            {
                fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "BaseEnv: Non-matching Header1:%llx Header2:%llx data ID %d data %llx\n",
                                      p_persistent_data->persist_header.entry_id,
                                      p_persistent_data->persist_header2.entry_id,
                                      p_persistent_data->header.persistent_id, 
                                      (unsigned long long) p_persistent_data->header.data);

            }
        }
        if( (data_status != FBE_STATUS_OK) &&
            (next_entry != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ))
        {
            /*
             * We haven't found our data and need to continue reading to the end of the persisted
             * data for ESP.  Re-use our context and keep reading.
             */
            status = fbe_api_persist_read_sector(FBE_PERSIST_SECTOR_TYPE_ESP_OBJECTS,
                                                 base_environment->persistent_storage_read_buffer,
                                                 FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE,
                                                 next_entry,
                                                 fbe_base_environment_read_persistent_data_completion,
                                                 (fbe_persist_completion_context_t)context);
            return FBE_STATUS_PENDING;
        }
    }

    /* Free that big block of memory we allocated to store all the read data.*/
    if(base_environment->persistent_storage_read_buffer)
    {
        fbe_release_nonpaged_pool_with_tag(base_environment->persistent_storage_read_buffer, 'pfbe');
        base_environment->persistent_storage_read_buffer = NULL;
    }

    /* Free the context that we allocated when we kicked off the read. */
    fbe_base_env_memory_ex_release(base_environment, my_context);

    if(data_status == FBE_STATUS_OK)
    {

        fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "BaseEnv: Found data in entry::0x%llx from persistent storage for id:%d.\n",
                              base_environment->my_persistent_entry_id, base_environment->my_persistent_id);
    }
    else 
    {
        fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "BaseEnv:Found no data from persistent storage for id:%d.\n",
                              base_environment->my_persistent_id);
    }

    // send the packet back to the monitor completion function to handle the next steps
    fbe_transport_set_status(packet, read_status, 0);
    fbe_transport_complete_packet(packet);

	return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_base_environment_send_write_persistent_data()
 ****************************************************************
 * @brief
 *  This function assumes we have permission to write to the
 *  persistent data storage service.  It issues the write request
 *  synchronously and then kicks off an async request to commit
 *  the data to the persistent storage.
 * 
 * @param read_status - Was the read successful
 * @param next_entry - next entry id or NULL if none
 * @param entries_read - number of entries read
 * @param context - pointer to our context
 *
 * @return fbe_status_t - The status 
 *
 * @author
 *  30-Dec-2010:  Created. Brion Philbin
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_send_write_persistent_data(fbe_base_environment_t *base_environment, fbe_packet_t *packet)
{
    fbe_status_t 	                        status = FBE_STATUS_OK;
    fbe_persist_transaction_handle_t	    transaction_handle;
    fbe_base_environment_persistent_data_t  *persistent_storage_write_buffer;
    fbe_base_environment_common_context_t   *context;
    fbe_status_t                            abort_status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s function enter.\n",
                              __FUNCTION__);

    status = fbe_base_environment_is_sep_ready(base_environment);
    if(status != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate a contiguous write buffer to store the data during the write/commit process */
    persistent_storage_write_buffer = fbe_allocate_nonpaged_pool_with_tag((fbe_u32_t) sizeof(fbe_persist_user_header_t) + FBE_BASE_ENVIRONMENT_PERSISTENT_DATA_SIZE(base_environment), 'pfbe');
    if(persistent_storage_write_buffer == NULL)
    {

        fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to allocate memory for write buffer.\n",
                              __FUNCTION__);

        return FBE_STATUS_INSUFFICIENT_RESOURCES;

    }
    fbe_zero_memory(persistent_storage_write_buffer, (fbe_u8_t) sizeof(fbe_persist_user_header_t) + FBE_BASE_ENVIRONMENT_PERSISTENT_DATA_SIZE(base_environment));
    /*
     * Use the persistent_entry_id we got when we read the data, or a default we set if there was
     * no data found previously.
     */
    persistent_storage_write_buffer->persist_header.entry_id = base_environment->my_persistent_entry_id;
    persistent_storage_write_buffer->header.persistent_id = base_environment->my_persistent_id;

    fbe_copy_memory(&persistent_storage_write_buffer->header.data, 
                     base_environment->my_persistent_data, 
                     base_environment->my_persistent_data_size);

    /* Get a transaction handle for the write/commit transaction */
	status = fbe_api_persist_start_transaction(&transaction_handle);
    fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s starting DB Persist transaction, size %d.\n",
                              __FUNCTION__, base_environment->my_persistent_data_size);


    if((status == FBE_STATUS_OK) && (persistent_storage_write_buffer->persist_header.entry_id == 0))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Entry:0x%llx ClassID:%d\n",
                              __FUNCTION__, 
                              persistent_storage_write_buffer->persist_header.entry_id, 
                              persistent_storage_write_buffer->header.persistent_id);

        // We do not already have an entry id, write the data out to a new location
        status = fbe_api_persist_write_entry_with_auto_entry_id_on_top(transaction_handle,
                                             FBE_PERSIST_SECTOR_TYPE_ESP_OBJECTS,
                                             (fbe_u8_t *)persistent_storage_write_buffer,
                                             ((fbe_u32_t)sizeof(fbe_persist_user_header_t) + (fbe_u32_t)FBE_BASE_ENVIRONMENT_PERSISTENT_DATA_SIZE(base_environment)),
                                             &persistent_storage_write_buffer->persist_header.entry_id);
        if(status == FBE_STATUS_OK)
        {
            // save the entry ID
            base_environment->my_persistent_entry_id = persistent_storage_write_buffer->persist_header.entry_id;
            /* Allocate a context to give to the callback function when the commit completes. */
            context = fbe_base_env_memory_ex_allocate(base_environment, sizeof(fbe_base_environment_common_context_t));

            fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s allocated context:%p\n",
                              __FUNCTION__, context);
            context->base_environment = base_environment;
            context->packet = packet;
            /* Kick off the commit request providing the callback function and context. */
            status = fbe_api_persist_commit_transaction(transaction_handle,
												fbe_base_environment_write_persistent_data_complete,
												(fbe_persist_completion_context_t)context);
            fbe_base_object_trace((fbe_base_object_t *)base_environment,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "BaseEnv Persist: committing DB Persist transaction in the new storage case.\n");

             if(status == FBE_STATUS_OK)
             {
                 base_environment->persistent_storage_write_buffer = (fbe_u8_t *)persistent_storage_write_buffer;
                 return status;
             }
             else
             {
                 abort_status = fbe_api_persist_abort_transaction(transaction_handle);
                 fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "BaseEnv Persist: failed to commit write transaction, status:0x%x abort_status 0x%x\n",
                              status, abort_status);
             }
        }
        else 
        {
            
            abort_status = fbe_api_persist_abort_transaction(transaction_handle);
            fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "BaseEnv Persist Error in writing the persistent entry status:0x%x abort_status:0x%x\n",
                              status, abort_status);
        }
    }
    else if(status == FBE_STATUS_OK)
    {

        fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Entry:0x%llx PersistID:%d\n",
                              __FUNCTION__, 
                              persistent_storage_write_buffer->persist_header.entry_id, 
                              persistent_storage_write_buffer->header.persistent_id);
        // we alredy have an entry ID overwrite the existing data
        status = fbe_api_persist_modify_entry(transaction_handle,
                                              (fbe_u8_t *)persistent_storage_write_buffer,
                                              ((fbe_u32_t)sizeof(fbe_persist_user_header_t) + (fbe_u32_t)FBE_BASE_ENVIRONMENT_PERSISTENT_DATA_SIZE(base_environment)),
                                              persistent_storage_write_buffer->persist_header.entry_id);

        if(status == FBE_STATUS_OK)
        {
            // save the entry ID
            base_environment->my_persistent_entry_id = persistent_storage_write_buffer->persist_header.entry_id;
            /* Allocate a context to give to the callback function when the commit completes. */
            context = fbe_base_env_memory_ex_allocate(base_environment, sizeof(fbe_base_environment_common_context_t));

            fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s allocated context:%p\n",
                              __FUNCTION__, context);
            context->base_environment = base_environment;
            context->packet = packet;
            /* Kick off the commit request providing the callback function and context. */
            status = fbe_api_persist_commit_transaction(transaction_handle,
												fbe_base_environment_write_persistent_data_complete,
												(fbe_persist_completion_context_t)context);
            fbe_base_object_trace((fbe_base_object_t *)base_environment,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "BaseEnv Persist: committing DB Persist transaction in the overwrite case.\n");

             if(status == FBE_STATUS_OK)
             {
                 base_environment->persistent_storage_write_buffer = (fbe_u8_t *)persistent_storage_write_buffer;
                 return status;
             }
             else
             {
                 fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to commit write transaction, status:0x%x\n",
                              __FUNCTION__, status);
             }
        }
        else
        {
            status = fbe_api_persist_abort_transaction(transaction_handle);
            fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in overwriting the persistent entry status:0x%x\n",
                              __FUNCTION__, status);
        }

    }
    else
    {
        // start transaction failed
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_environment,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s start transaction failed with status:0x%x\n",
                              __FUNCTION__, status);
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
        }
    }

    fbe_release_nonpaged_pool_with_tag(persistent_storage_write_buffer, 'pfbe');

    return status;
}


/*!**************************************************************
 * fbe_base_environment_write_persistent_data_complete()
 ****************************************************************
 * @brief
 *  This function handles the completion of the persistent data
 *  write.  The write itself is synchronous, however the commit
 *  command is asynchronous and will complete through this
 *  funciton.
 * 
 * @param commit_status - Was the commit successful
 * @param context - pointer to our context
 *
 * @return fbe_status_t - The status 
 *
 * @author
 *  30-Dec-2010:  Created. Brion Philbin
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_write_persistent_data_complete(fbe_status_t commit_status, 
                                                                 fbe_persist_completion_context_t context)
{
    fbe_base_environment_common_context_t   *base_env_context;
    fbe_packet_t                            *packet;
    fbe_base_environment_t                  *base_environment;

    base_env_context = (fbe_base_environment_common_context_t *)context;
    base_environment = base_env_context->base_environment;
    packet = base_env_context->packet;

    fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s function enter.\n",
                              __FUNCTION__);

    /* Release the memory we allocated to track our context */
    fbe_base_env_memory_ex_release(base_environment, base_env_context);

    if(base_environment->persistent_storage_write_buffer != NULL)
    {
        /* Release the contiguous memory that was allocated to do the write */
        fbe_release_nonpaged_pool_with_tag(base_environment->persistent_storage_write_buffer, 'pfbe');
        base_environment->persistent_storage_write_buffer = NULL;
    }

    // send the packet back to the monitor completion function to handle the next steps
    fbe_transport_set_status(packet, commit_status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_base_environment_is_sep_ready()
 ****************************************************************
 * @brief
 *  This function checks that the sep package exists and the
 *  DB service is in the ready state.
 * 
 * @param base_environment - our context
 *
 * @return fbe_status_t - OK if sep if everything checked out. 
 *
 * @author
 *  06-Aug-2012:  Created. Brion Philbin
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_is_sep_ready(fbe_base_environment_t *base_environment)
{
    fbe_bool_t sep_initialized;
    fbe_database_state_t db_state;
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_api_common_package_entry_initialized (FBE_PACKAGE_ID_SEP_0, &sep_initialized);

    if(((status != FBE_STATUS_OK)) || (sep_initialized != FBE_TRUE))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s SEP is not initialized, status:0x%x, retry needed.\n",
                              __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s SEP is initialized read/write should work.\n",
                              __FUNCTION__);
    }

    status = fbe_api_database_get_state(&db_state);
    if((status != FBE_STATUS_OK) || (db_state != FBE_DATABASE_STATE_READY))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s SEP DB service is not ready, status:0x%x, retry needed.\n",
                              __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s SEP DB service is ready, read/write should work.\n",
                              __FUNCTION__);
    }
    return FBE_STATUS_OK;
}


