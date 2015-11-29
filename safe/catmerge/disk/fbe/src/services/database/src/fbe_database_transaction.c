/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_transaction.c
***************************************************************************
*
* @brief
*  This file contains logical database service functions which
*  process control request.
*  
* @version
*   2/24/2011 - Created. 
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database.h"
#include "fbe_database_hook.h"
#include "fbe_database_private.h"
#include "fbe_database_config_tables.h"
#include "fbe_database_drive_connection.h"
#include "fbe_database_cmi_interface.h"
#include "fbe_database_system_db_interface.h"
#include "fbe_cmi.h"

/*local functions */
static fbe_status_t database_transaction_commit_completion_function (fbe_status_t commit_status, fbe_persist_completion_context_t context);
static fbe_status_t fbe_database_transaction_operation_on_peer(fbe_cmi_msg_type_t transaction_operation_type,
															   fbe_database_service_t *database_service);

static fbe_status_t fbe_database_transaction_operation_on_peer_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context);

static fbe_status_t fbe_database_transaction_start_request_from_peer_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context);

static fbe_status_t fbe_database_transaction_commit_request_from_peer_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context);

static fbe_status_t fbe_database_transaction_abort_request_from_peer_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context);

static fbe_status_t fbe_database_transaction_invalidate_request_from_peer_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                    fbe_status_t completion_status,
                                                                     void *context);
static fbe_status_t fbe_database_transaction_operation_dma(fbe_cmi_msg_type_t transaction_operation_type,
															   fbe_database_service_t *database_service);

/***********************************************************************************************************************************************************************************/

/*local variables */
static fbe_database_transaction_id_t 		transaction_id_counter;
static fbe_database_peer_synch_context_t	peer_transaction_confirm;
static database_transaction_commit_context_t    persist_transaction_commit_context;

extern fbe_database_service_t fbe_database_service;


fbe_status_t fbe_database_allocate_transaction(fbe_database_service_t *fbe_database_service)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (fbe_database_service->transaction_ptr != NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction is already allocated, addr 0x%p.\n",
            __FUNCTION__, fbe_database_service->transaction_ptr);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_database_service->transaction_ptr = (database_transaction_t *)fbe_memory_native_allocate(sizeof(database_transaction_t));
    if ( fbe_database_service->transaction_ptr == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Can not allocate transaction.\n",
            __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    status = fbe_database_transaction_init(fbe_database_service->transaction_ptr);

    /* This is used for backup */
    fbe_database_service->transaction_backup_ptr = (database_transaction_t *)fbe_memory_native_allocate(sizeof(database_transaction_t));
    if ( fbe_database_service->transaction_backup_ptr == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Can not allocate transaction.\n",
            __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_database_service->transaction_alloc_size = sizeof(database_transaction_t);
    fbe_database_service->transaction_peer_physical_address = 0;

    return status;
}


fbe_status_t fbe_database_free_transaction(fbe_database_service_t *fbe_database_service)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (fbe_database_service->transaction_ptr == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction is already free. \n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_memory_native_release (fbe_database_service->transaction_ptr);
    fbe_database_service->transaction_ptr = NULL;
    fbe_memory_native_release (fbe_database_service->transaction_backup_ptr);
    fbe_database_service->transaction_backup_ptr = NULL;
    return status;
} 

fbe_status_t fbe_database_transaction_init(database_transaction_t *transaction)
{
    if (transaction != NULL)
    {
        transaction_id_counter = FBE_DATABASE_TRANSACTION_ID_INVALID;
        transaction->transaction_id = transaction_id_counter;
        transaction->state = DATABASE_TRANSACTION_STATE_INACTIVE;
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction is not allocated.\n",
            __FUNCTION__);
    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_transaction_destroy(database_transaction_t *transaction)
{
    while (transaction->state != DATABASE_TRANSACTION_STATE_INACTIVE)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction 0x%llx is in %d state.\n",
            __FUNCTION__, (unsigned long long)transaction->transaction_id,
	    transaction->state);
        fbe_thread_delay(100);
    }
    transaction_id_counter = FBE_DATABASE_TRANSACTION_ID_INVALID;
    transaction->transaction_id = transaction_id_counter;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_transaction_start(fbe_database_service_t *fbe_database_service, fbe_database_transaction_info_t   *transaction_info)
{
    fbe_status_t                        status = FBE_STATUS_INVALID;
    database_transaction_t *            transaction = fbe_database_service->transaction_ptr;


    fbe_zero_memory(transaction,sizeof(database_transaction_t));
    transaction_id_counter++;
    transaction->transaction_id = transaction_id_counter;
    transaction->state = DATABASE_TRANSACTION_STATE_ACTIVE;
    transaction->transaction_type = transaction_info->transaction_type;
    transaction->job_number = transaction_info->job_number;

    if (is_peer_update_allowed(fbe_database_service)) {
        status = fbe_database_transaction_operation_on_peer(FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_START,
                                                            fbe_database_service);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Transaction 0x%x can't start on the peer but it's present, no config change allowed\n",
                __FUNCTION__, (unsigned int)transaction->transaction_id);
            fbe_zero_memory(transaction,sizeof(database_transaction_t));
            transaction_id_counter--;    
            return status;
        }
    }

    return FBE_STATUS_OK;
}

/***************************************************************************** 
 *
 *****************************************************************************
 * @brief   This function will commit transaction on peer
 *          If we enable ext pool, it will use dma to send the transaction
 *          otherwise, it will use original method
 *
 * @param   database_service - Pointer to database tables.
 *
 * @return  status
 *
 * @version  2014-02-15. Created. Jamin Kang 
 *
 *****************************************************************************/
static fbe_status_t fbe_database_transaction_operation_commit_on_peer(fbe_database_service_t *database_service)
{
    fbe_status_t status;

    status = fbe_database_transaction_operation_on_peer(FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT,
                                                        database_service);
    return status;
}

/***************************************************************************** 
 *
 *****************************************************************************
 * @brief   This function performs (3) actions:
 *              1. Setup persist for each entry in the transaction.
 *              2. Commit each object in the transaction.
 *              3. Persist the updated objects.
 *          If any step fails we will rollback the in-memory tables.
 *
 * @param   database_service - Pointer to database tables.
 *
 * @return  status
 * 
 * @todo    This function needs to be rewritten.  See AR 633943 for details.
 *
 *****************************************************************************/
fbe_status_t fbe_database_transaction_commit(fbe_database_service_t *database_service)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               size = 0;

    database_transaction_t           *transaction = NULL;
    database_user_entry_t            *transaction_user_table_ptr = NULL;
    database_object_entry_t          *transaction_object_table_ptr = NULL;
    database_edge_entry_t            *transaction_edge_table_ptr = NULL;
    database_global_info_entry_t     *transaction_global_info_ptr = NULL;
    database_user_entry_t * service_user_table_ptr = NULL;
    database_object_entry_t * service_object_table_ptr = NULL;
    database_edge_entry_t * service_edge_table_ptr = NULL;
    database_global_info_entry_t     *service_global_info_ptr = NULL;

    fbe_persist_transaction_handle_t persist_handle; 
    fbe_status_t					 wait_status = FBE_STATUS_OK;
    fbe_u64_t                       retry_count=0;

    fbe_u64_t index;
    fbe_object_id_t last_system_object_id;
    fbe_database_system_db_persist_handle_t system_db_persist_handle;
    fbe_bool_t  is_active_sp = database_common_cmi_is_active();

    fbe_semaphore_init(&persist_transaction_commit_context.commit_semaphore, 0, 1);

    transaction = database_service->transaction_ptr;
    transaction_user_table_ptr = transaction->user_entries;
    transaction_object_table_ptr = transaction->object_entries;
    transaction_edge_table_ptr = transaction->edge_entries;
    transaction_global_info_ptr = transaction->global_info_entries;

    /* Can not commit is the transaction is not in active state */
    if (transaction->state != DATABASE_TRANSACTION_STATE_ACTIVE)
    {
		fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction 0x%llx is in %d state, can't commit transaction.\n",
            __FUNCTION__, (unsigned long long)transaction->transaction_id,
	    transaction->state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*check hook*/
    database_check_and_run_hook(NULL,FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_TRANSACTION, NULL);
    database_check_and_run_hook(NULL,FBE_DATABASE_HOOK_TYPE_PANIC_IN_UPDATE_TRANSACTION, NULL);

    transaction->state = DATABASE_TRANSACTION_STATE_COMMIT;
    /* start commit */
    status = fbe_database_system_db_start_persist_transaction(&system_db_persist_handle);
    if (status != FBE_STATUS_OK) {
		fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: start system db persist transaction, status:0x%x\n",
                       __FUNCTION__, status);
        return status;
    }

    status = fbe_database_get_last_system_object_id(&last_system_object_id);
    if (status != FBE_STATUS_OK)
    {
		fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s :failed to get last system obj id status: 0x%x\n",
            __FUNCTION__, status);
        return status;
    }

    /* open a persist service transaction get a handle back */
    do{
        status =  fbe_database_persist_start_transaction(&persist_handle);
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                        FBE_TRACE_MESSAGE_ID_INFO, 
                        "DB transaction commit:start persist transaction, status:%d, retry: %d\n",
                        status, (int)retry_count);
        if (status == FBE_STATUS_BUSY)
        {
            /* Wait 3 second */
            fbe_thread_delay(FBE_START_TRANSACTION_RETRY_INTERVAL);
            retry_count++;
        }
    }while((status == FBE_STATUS_BUSY) && (retry_count < FBE_START_TRANSACTION_RETRIES));

    if (status != FBE_STATUS_OK)
    {
        fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s :start persist transaction failed, status: 0x%x\n",
                        __FUNCTION__, status);

        /*abort the system db persist transaction*/
        fbe_database_system_db_persist_abort(system_db_persist_handle);
        return status;
    }


    /* go thru the transaction tables and persist each entry */ 

    /* update the service global info table first*/
    for (index = 0; index < DATABASE_TRANSACTION_MAX_GLOBAL_INFO; index++)
    {
        switch(transaction_global_info_ptr[index].header.state){
        case DATABASE_CONFIG_ENTRY_MODIFY:
            if(is_active_sp)
            {
                /*passive SP should stick the entry state so when the active SP panics, it can
                 *have the state to process incomplete transaction (rollback or commit)*/
                transaction_global_info_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            }
            /* If the committed size is bigger , update it*/
            database_common_get_committed_global_info_entry_size(transaction_global_info_ptr[index].type, &size);
            if (size > transaction_global_info_ptr[index].header.version_header.size)
            {
                transaction_global_info_ptr[index].header.version_header.size = size;
            }
            fbe_spinlock_lock(&database_service->global_info.table_lock);

            /* Get the persit entry id from the in-memory service table */
            fbe_database_config_table_get_global_info_entry(&database_service->global_info, 
                                                            transaction_global_info_ptr[index].type, 
                                                            &service_global_info_ptr);

            fbe_spinlock_unlock(&database_service->global_info.table_lock);

            /* The check for entry-id is required, coz, if it is an INVALID-id, it means,
             * there is no valid DATA in the persistence DB. So do a write straight-away.
             * If not, then we have some data to modify or overwrite.  
             */
            if(service_global_info_ptr->header.entry_id == 0)
            {
                /* Send to persist */
				
				status = fbe_database_persist_write_entry(persist_handle,
														  FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA,
														  (fbe_u8_t *)&transaction_global_info_ptr[index],
														  sizeof(transaction_global_info_ptr[index]),
														  &transaction_global_info_ptr[index].header.entry_id);
				if (status != FBE_STATUS_OK )
				{
					database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: Persist failed to write entry, status: 0x%x\n",
						__FUNCTION__, status);	
				}

				database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: Write persist entry 0x%llx completed for Global Info type 0x%x!\n",
							   __FUNCTION__, (unsigned long long)transaction_global_info_ptr[index].header.entry_id,
							   transaction_global_info_ptr[index].type);	
				
            }
            else
            {
                /* Send to persist */
				
				status = fbe_database_persist_modify_entry(persist_handle,
														   (fbe_u8_t *)&transaction_global_info_ptr[index],
														   sizeof(transaction_global_info_ptr[index]),
														   service_global_info_ptr->header.entry_id);
				if (status != FBE_STATUS_OK )
				{
					database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: Persist failed to modify entry 0x%llx, status: 0x%x\n",
						__FUNCTION__, (unsigned long long)service_global_info_ptr->header.entry_id, status);	
				}
			

				database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
						   "%s: Modifying persist entry 0x%llx completed for Global Info!\n",
						   __FUNCTION__, (unsigned long long)transaction_global_info_ptr[index].header.entry_id);	
				
            }

            /* We are doing something different here as compared to other table entries. 
             * For Global Info, we first do a persist write/Modify so that we will get a
             * valid entry-id. Then issue an update. If we do things the same old way,
             * we may end up in writing invalid entry-id to the peristence DB.
             */
            fbe_spinlock_lock(&database_service->global_info.table_lock);

            /* Update the global_info entry */
            fbe_database_config_table_update_global_info_entry(&database_service->global_info, 
                                                               &transaction_global_info_ptr[index]);

            fbe_spinlock_unlock(&database_service->global_info.table_lock);
            break;

        case DATABASE_CONFIG_ENTRY_DESTROY:

            /* Get the persit entry id from the in-memory service table */
            fbe_database_config_table_get_global_info_entry(&database_service->global_info, 
                                                            transaction_global_info_ptr[index].type, 
                                                            &service_global_info_ptr);

            /* send to persist */
			status = fbe_database_persist_delete_entry(persist_handle, service_global_info_ptr->header.entry_id);
			if (status != FBE_STATUS_OK )
			{
				database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
							   "%s: Persist failed to delete entry 0x%llx, status: 0x%x\n", 
							   __FUNCTION__, (unsigned long long)service_global_info_ptr->header.entry_id, status);	
			}

			database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
						   "%s: Deleted persist entry 0x%llx\n",
						   __FUNCTION__, 
						   (unsigned long long)service_global_info_ptr->header.entry_id);	
			

            /* update the in-memory service table */			
            fbe_database_config_table_remove_global_info_entry(&database_service->global_info, 
                                                               &transaction_global_info_ptr[index]);
            break;

        case DATABASE_CONFIG_ENTRY_INVALID:  /* skip invalid entry*/
            break;

        /* Nobody should be issuing the CREATE for GLOBAL INFO. Global Info is created when the database comes up. */
        case DATABASE_CONFIG_ENTRY_VALID:
            /* it's OK to see valid on the passive side, and it need to update its generation number at least*/
            if (!is_active_sp)
            {
                fbe_spinlock_lock(&database_service->global_info.table_lock);

                /* Update the global_info entry */
                fbe_database_config_table_update_global_info_entry(&database_service->global_info, 
                                                                   &transaction_global_info_ptr[index]);

                fbe_spinlock_unlock(&database_service->global_info.table_lock);
                break;
            }
            /* otherwise, it fall through to default, we should not see this for active side */
        case DATABASE_CONFIG_ENTRY_CREATE:
        default:
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Trans. Global Info Table: index %llu, Unexpected state:%d\n",
                __FUNCTION__, (unsigned long long)index, transaction_global_info_ptr[index].header.state);
            break;
        }
        /* zero the memory, so no leftover data on the buffer */
        //fbe_zero_memory(&transaction_global_info_ptr[index], sizeof(database_global_info_entry_t));
    }
    
    /* update the service user table */
    for (index = 0; index < DATABASE_TRANSACTION_MAX_USER_ENTRY; index++)
    {
        switch(transaction_user_table_ptr[index].header.state)
        {
        case DATABASE_CONFIG_ENTRY_CREATE:
            if(is_active_sp)
            {
                /*passive SP should stick the entry state so when the active SP panics, it can
                 *have the state to process incomplete transaction (rollback or commit)*/
                transaction_user_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            }
            /* If the committed size is bigger , update it*/
            database_common_get_committed_user_entry_size(transaction_user_table_ptr[index].db_class_id, &size);
            if (size > transaction_user_table_ptr[index].header.version_header.size)
            {
                transaction_user_table_ptr[index].header.version_header.size = size;
            }

            /* write entry and get a valid entry id*/

            /*check if this is a system object entry and persistent the system user entry*/
            if (transaction_user_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_user_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE,
                                                                &transaction_user_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            } else {
                /*For create, we also need to judge whether entry id is initialized. This considers the following case:
                 *Active SP runs db transaction commit logic, where the newly created entries are assigned new entry id by persist service
                 *Active SP sends db transaction commit cmd to passive SP, where the new entry ids are copied to the entries on passive SP
                 *Before or in the middle of persist on active SP, it panics
                 *When passive SP becomes active SP, it should commit the incomplete transaction, where it should use the already allocated
                 *entry id to do the persist, rather than allocating new entry ids
                 *Once persist service removes entry bitmap and begin to use object id as the index of persist location, this can
                 *be removed*/
                if(transaction_user_table_ptr[index].header.entry_id == 0)
                {
                    status = fbe_database_persist_write_entry(persist_handle, 
    													  FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION,
    													  (fbe_u8_t *)&transaction_user_table_ptr[index],
    													  sizeof(transaction_user_table_ptr[index]),
    													  &transaction_user_table_ptr[index].header.entry_id);
                    
                    if (status != FBE_STATUS_OK )
                    {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
    							   "%s: Persist failed to write entry, status: 0x%x\n",
    							   __FUNCTION__, status);	
                    }
                }
                else
                {
                    status = fbe_database_persist_modify_entry(persist_handle,
                                                               (fbe_u8_t *)&transaction_user_table_ptr[index],
                                                               sizeof(transaction_user_table_ptr[index]),
                                                               transaction_user_table_ptr[index].header.entry_id);
                    if (status != FBE_STATUS_OK )
                    {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                            __FUNCTION__, (unsigned int)transaction_user_table_ptr[index].header.entry_id, status);    
                    }
                    
                    
                    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Modifying persist entry 0x%llx completed for user entry!\n",
                               __FUNCTION__, transaction_user_table_ptr[index].header.entry_id);
                }
                
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: obj 0x%x written to persist entry 0x%llx completed!\n",
                               __FUNCTION__,
                               transaction_user_table_ptr[index].header.object_id,
                               transaction_user_table_ptr[index].header.entry_id);
            }

            /* update the in-memory service table */
            fbe_database_config_table_update_user_entry(&database_service->user_table, &transaction_user_table_ptr[index]);
            break;
        case DATABASE_CONFIG_ENTRY_MODIFY:
            if(is_active_sp)
            {
                /*passive SP should stick the entry state so when the active SP panics, it can
                 *have the state to process incomplete transaction (rollback or commit)*/
                transaction_user_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            }
            /* If the committed size is bigger , update it*/
            database_common_get_committed_user_entry_size(transaction_user_table_ptr[index].db_class_id, &size);
            if (size > transaction_user_table_ptr[index].header.version_header.size)
            {
                transaction_user_table_ptr[index].header.version_header.size = size;
            }

            /* get the persit entry id from the in-memory service table */
            switch(transaction_user_table_ptr[index].db_class_id)
            {
            case DATABASE_CLASS_ID_LUN:
                fbe_database_config_table_get_user_entry_by_lun_id(&database_service->user_table, 
                                                                   transaction_user_table_ptr[index].user_data_union.lu_user_data.lun_number,
                                                                   &service_user_table_ptr);
                break;
            case DATABASE_CLASS_ID_MIRROR:
            case DATABASE_CLASS_ID_PARITY:
            case DATABASE_CLASS_ID_STRIPER:
                fbe_database_config_table_get_user_entry_by_rg_id(&database_service->user_table, 
                                                                   transaction_user_table_ptr[index].user_data_union.rg_user_data.raid_group_number,
                                                                   &service_user_table_ptr);
                break;
            case DATABASE_CLASS_ID_PROVISION_DRIVE:
            case DATABASE_CLASS_ID_EXTENT_POOL:
            case DATABASE_CLASS_ID_EXTENT_POOL_LUN:
            case DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
                fbe_database_config_table_get_user_entry_by_object_id(&database_service->user_table, 
                                                                   transaction_user_table_ptr[index].header.object_id,
                                                                   &service_user_table_ptr);
                break;
            default:
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Un-supported class type: 0x%x\n",
                               __FUNCTION__, 
                               transaction_user_table_ptr[index].db_class_id);	
                break;
            }

            /*check if this is a system object entry and persistent the system user entry*/
            if (transaction_user_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_user_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE,
                                                                &transaction_user_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            }
            else 
            {
                if(service_user_table_ptr != NULL)
                {
                    /* send to persist */
                    if (service_user_table_ptr->header.entry_id == 0)
                    {
                        /* write entry and get a valid entry id*/
                        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: first time modify obj 0x%x user data, create any entry in persist!\n",
                                   __FUNCTION__, 
                                   transaction_user_table_ptr[index].header.object_id);	
                    
                        status = fbe_database_persist_write_entry(persist_handle, 
                                                                  FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION, 
                                                                  (fbe_u8_t *)&transaction_user_table_ptr[index], 
                                                                  sizeof(transaction_user_table_ptr[index]), 
                                                                  &transaction_user_table_ptr[index].header.entry_id);
                        if (status != FBE_STATUS_OK )
                        {
                            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                           "%s: Persist failed to write entry, status: 0x%x\n", 
                                           __FUNCTION__, status);	
                        }
                        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                                       "%s: obj 0x%x written to persist entry 0x%llx completed!\n", 
                                       __FUNCTION__, 
                                       transaction_user_table_ptr[index].header.object_id, 
                                       (unsigned long long)transaction_user_table_ptr[index].header.entry_id);	
                    }
                    else
                    {
                        status = fbe_database_persist_modify_entry(persist_handle, 
                                                                   (fbe_u8_t *)&transaction_user_table_ptr[index], 
                                                                   sizeof(transaction_user_table_ptr[index]), 
                                                                   service_user_table_ptr->header.entry_id);
                        if (status != FBE_STATUS_OK )
                        {
                            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                           "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                                           __FUNCTION__, (unsigned int)service_user_table_ptr->header.entry_id, status);	
                        }
                        /* update the in-memory service table */
                        transaction_user_table_ptr[index].header.entry_id = service_user_table_ptr->header.entry_id;
                    }
                }
            }
            
            /* we need to send notification to admin shim*/		
            fbe_database_commit_object(transaction_user_table_ptr[index].header.object_id, transaction_user_table_ptr->db_class_id, FBE_FALSE);
            fbe_database_config_table_update_user_entry(&database_service->user_table, &transaction_user_table_ptr[index]);
            break;
        case DATABASE_CONFIG_ENTRY_DESTROY:
            /* get the persit entry id from the in-memory service table */
            fbe_database_config_table_get_user_entry_by_object_id(&database_service->user_table,
                                                               transaction_user_table_ptr[index].header.object_id,
                                                               &service_user_table_ptr);
            database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: delete persist entry 0x%x\n",
                           __FUNCTION__, 
                           (unsigned int)service_user_table_ptr->header.entry_id);	

                        /*check if this is a system object entry and persistent the system user entry*/
            if (transaction_user_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_user_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE,
                                                                &transaction_user_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to delete entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            } else {
                /* send to persist */
                status = fbe_database_persist_delete_entry(persist_handle, service_user_table_ptr->header.entry_id);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: Deleted persist entry 0x%x, status: 0x%x\n",
							   __FUNCTION__, (unsigned int)service_user_table_ptr->header.entry_id, status);	
                }
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: obj 0x%x destroy persist entry 0x%llx completed!\n", 
                               __FUNCTION__, 
                               transaction_user_table_ptr[index].header.object_id, 
                               (unsigned long long)transaction_user_table_ptr[index].header.entry_id);	
            }
			
            /* update the in-memory service table */			
            fbe_database_config_table_remove_user_entry(&database_service->user_table, &transaction_user_table_ptr[index]);
            break;
        case DATABASE_CONFIG_ENTRY_INVALID:  /* skip invalid entry*/
            break;
        case DATABASE_CONFIG_ENTRY_VALID:
        default:
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Trans. User Table: index %d, Unexpected state:%d\n",
                __FUNCTION__, (int)index, transaction_user_table_ptr[index].header.state);
            break;
        }
        /* zero the memory, so no leftover data on the buffer */
        //fbe_zero_memory(&transaction_user_table_ptr[index], sizeof(database_user_entry_t));
    }

    /* update the service edge table */
    for (index = 0; index < DATABASE_TRANSACTION_MAX_EDGES; index++)
    {
        switch(transaction_edge_table_ptr[index].header.state){
        case DATABASE_CONFIG_ENTRY_CREATE:
            if(is_active_sp)
            {
                /*passive SP should stick the entry state so when the active SP panics, it can
                 *have the state to process incomplete transaction (rollback or commit)*/
                transaction_edge_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            }
            /* If the new size is bigger, update it*/
            database_common_get_committed_edge_entry_size(&size);
            if (size > transaction_edge_table_ptr[index].header.version_header.size)
            {
                transaction_edge_table_ptr[index].header.version_header.size = size;
            }

            /*check if this is a system object entry and persistent the system edge entry*/
            if (transaction_edge_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_edge_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE,
                                                                &transaction_edge_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            } else {
                /* write entry and get a valid entry id*/

                if(transaction_edge_table_ptr[index].header.entry_id == 0)
                {                
                    status = fbe_database_persist_write_entry(persist_handle, 
                                                              FBE_PERSIST_SECTOR_TYPE_SEP_EDGES,
                                                              (fbe_u8_t *)&transaction_edge_table_ptr[index],
                                                              sizeof(transaction_edge_table_ptr[index]),
                                                              &transaction_edge_table_ptr[index].header.entry_id);
                    if (status != FBE_STATUS_OK )
                    {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Persist failed to write entry, status: 0x%x\n",
                            __FUNCTION__, status);	
                    }
                }
                else
                {
                    status = fbe_database_persist_modify_entry(persist_handle,
                                                               (fbe_u8_t *)&transaction_edge_table_ptr[index],
                                                               sizeof(transaction_edge_table_ptr[index]),
                                                               transaction_edge_table_ptr[index].header.entry_id);
                    if (status != FBE_STATUS_OK )
                    {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                            __FUNCTION__, (unsigned int)transaction_edge_table_ptr[index].header.entry_id, status);    
                    }
                }
                
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: edge 0x%x-0x%x written to persist entry 0x%llx completed!\n",
                               __FUNCTION__, 
                               transaction_edge_table_ptr[index].header.object_id,
                               transaction_edge_table_ptr[index].client_index,
                               (unsigned long long)transaction_edge_table_ptr[index].header.entry_id);	
            }



            /* update the in-memory service table */
            fbe_database_config_table_update_edge_entry(&database_service->edge_table, &transaction_edge_table_ptr[index]);
            break;
        case DATABASE_CONFIG_ENTRY_MODIFY:
            if(is_active_sp)
            {
                /*passive SP should stick the entry state so when the active SP panics, it can
                 *have the state to process incomplete transaction (rollback or commit)*/
                transaction_edge_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            }
            /* If the new size is bigger, update it*/
            database_common_get_committed_edge_entry_size(&size);
            if (size > transaction_edge_table_ptr[index].header.version_header.size)
            {
                transaction_edge_table_ptr[index].header.version_header.size = size;
            }

            /* get the persit entry id from the in-memory service table */
            fbe_database_config_table_get_edge_entry(&database_service->edge_table,
                                                     transaction_edge_table_ptr[index].header.object_id,
                                                     transaction_edge_table_ptr[index].client_index,
                                                     &service_edge_table_ptr);
             /*check if this is a system object entry and persistent the system edge entry*/
            if (transaction_edge_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_edge_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE,
                                                                &transaction_edge_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            } else {
                /* send to persist */
                status = fbe_database_persist_modify_entry(persist_handle,
                                                           (fbe_u8_t *)&transaction_edge_table_ptr[index],
                                                           sizeof(transaction_edge_table_ptr[index]),
                                                           service_edge_table_ptr->header.entry_id);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                        __FUNCTION__, (unsigned int)service_edge_table_ptr->header.entry_id, status);	
                }
                
            }

            /* update the in-memory service table */
            fbe_database_config_table_update_edge_entry(&database_service->edge_table, &transaction_edge_table_ptr[index]);
            break;
        case DATABASE_CONFIG_ENTRY_DESTROY:
            /* get the persit entry id from the in-memory service table */
            fbe_database_config_table_get_edge_entry(&database_service->edge_table,
                                                     transaction_edge_table_ptr[index].header.object_id,
                                                     transaction_edge_table_ptr[index].client_index,
                                                     &service_edge_table_ptr);

            /*check if this is a system object entry and persistent the system edge entry*/
            if (transaction_edge_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_edge_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE,
                                                                &transaction_edge_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            } else {
                /* send to persist */
                status = fbe_database_persist_delete_entry(persist_handle, service_edge_table_ptr->header.entry_id);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Persist failed to delete entry 0x%x, status: 0x%x\n",
                        __FUNCTION__, (unsigned int)service_edge_table_ptr->header.entry_id, status);	
                }

                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: edge 0x%x-0x%x destroy persist entry 0x%llx completed!\n",
                               __FUNCTION__, 
                               transaction_edge_table_ptr[index].header.object_id,
                               transaction_edge_table_ptr[index].client_index,
                               (unsigned long long)transaction_edge_table_ptr[index].header.entry_id);	
            }
			
            /* update the in-memory service table */			
            fbe_database_config_table_remove_edge_entry(&database_service->edge_table, &transaction_edge_table_ptr[index]);
            break;
        case DATABASE_CONFIG_ENTRY_INVALID:  /* skip invalid entry*/
            break;
        case DATABASE_CONFIG_ENTRY_VALID:
        default:
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Trans. Edge Table: index %d, Unexpected state:%d\n",
                __FUNCTION__, (int)index, transaction_edge_table_ptr[index].header.state);
            break;
        }
        /* zero the memory, so no leftover data on the buffer */
        //fbe_zero_memory(&transaction_edge_table_ptr[index], sizeof(database_edge_entry_t));
    }

    /* update the service object table */
    for (index = 0; index < DATABASE_TRANSACTION_MAX_OBJECTS; index++)
    {
        switch(transaction_object_table_ptr[index].header.state){
        case DATABASE_CONFIG_ENTRY_CREATE:
            if(is_active_sp)
            {
                /*passive SP should stick the entry state so when the active SP panics, it can
                 *have the state to process incomplete transaction (rollback or commit)*/
                transaction_object_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            }
            /* If the size is bigger, update it*/
            database_common_get_committed_object_entry_size(transaction_object_table_ptr[index].db_class_id, &size);
            if (size > transaction_object_table_ptr[index].header.version_header.size)
            {
                 transaction_object_table_ptr[index].header.version_header.size = size;
            }

            /* write entry and get a valid entry id*/
			
            /*check if this is a system object entry and persistent the system object entry*/
            if (transaction_object_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_object_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE,
                                                                &transaction_object_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            } else {
                if(transaction_object_table_ptr[index].header.entry_id == 0)
                {
                    status = fbe_database_persist_write_entry(persist_handle,
                                                              FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS,
                                                              (fbe_u8_t *)&transaction_object_table_ptr[index],
                                                              sizeof(transaction_object_table_ptr[index]),
                                                              &transaction_object_table_ptr[index].header.entry_id);
                    if (status != FBE_STATUS_OK )
                    {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Persist failed to write entry, status: 0x%x\n",
                            __FUNCTION__, status);	
                    }
                }
                else
                {
                    status = fbe_database_persist_modify_entry(persist_handle,
                                                               (fbe_u8_t *)&transaction_object_table_ptr[index],
                                                               sizeof(transaction_object_table_ptr[index]),
                                                               transaction_object_table_ptr[index].header.entry_id);
                    if (status != FBE_STATUS_OK )
                    {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                            __FUNCTION__, (unsigned int)transaction_object_table_ptr[index].header.entry_id, status);    
                    }
                }
                
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: object 0x%x written to persist entry 0x%llx completed!\n",
                               __FUNCTION__, 
                               transaction_object_table_ptr[index].header.object_id,
                               (unsigned long long)transaction_object_table_ptr[index].header.entry_id);
            }
            

            /* update the in-memory service table */            
            fbe_database_config_table_update_object_entry(&database_service->object_table, 
                                                          &transaction_object_table_ptr[index]);

            if(transaction_object_table_ptr[index].db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)
            {
                /*If it's a PVD then connect it to the LDO*/
                status = fbe_database_add_pvd_to_be_connected(&database_service->pvd_operation, transaction_object_table_ptr[index].header.object_id);
                if (status != FBE_STATUS_OK)
                {
                      database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: Failed to connect PVD to LDO! Status=0x%x\n",
                                        __FUNCTION__,status);			
                }
             }

             fbe_database_commit_object(transaction_object_table_ptr[index].header.object_id, transaction_object_table_ptr[index].db_class_id, FBE_TRUE);
            
            
            break;
        case DATABASE_CONFIG_ENTRY_MODIFY:
            if(is_active_sp)
            {
                /*passive SP should stick the entry state so when the active SP panics, it can
                 *have the state to process incomplete transaction (rollback or commit)*/
                transaction_object_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            }
            /* If the size is bigger, update it*/
            database_common_get_committed_object_entry_size(transaction_object_table_ptr[index].db_class_id, &size);
            if (size > transaction_object_table_ptr[index].header.version_header.size)
            {
                 transaction_object_table_ptr[index].header.version_header.size = size;
            }

            fbe_spinlock_lock(&database_service->object_table.table_lock);
            /* get the persit entry id from the in-memory service table */
            fbe_database_config_table_get_object_entry_by_id (&database_service->object_table,
                                                              transaction_object_table_ptr[index].header.object_id,
                                                              &service_object_table_ptr);
            
            fbe_spinlock_unlock(&database_service->object_table.table_lock);

            /*check if this is a system object entry and persistent the system object entry*/
            if (transaction_object_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_object_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE,
                                                                &transaction_object_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            } else {
                /* send to persist */
                status = fbe_database_persist_modify_entry(persist_handle,
                                                           (fbe_u8_t *)&transaction_object_table_ptr[index],
                                                           sizeof(transaction_object_table_ptr[index]),
                                                           service_object_table_ptr->header.entry_id);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                        __FUNCTION__, (unsigned int)service_object_table_ptr->header.entry_id, status);	
                }
            }

            fbe_spinlock_lock(&database_service->object_table.table_lock);

            /* update the in-memory service table */
            /*  Don't use the following function, because this function will initialize the object_entry.
              
              fbe_database_config_table_update_object_entry(&database_service->object_table, 
                                                          &transaction_object_table_ptr[index]);*/
            fbe_copy_memory(
                &(database_service->object_table.table_content.object_entry_ptr[transaction_object_table_ptr[index].header.object_id]), 
                &transaction_object_table_ptr[index], 
                transaction_object_table_ptr[index].header.version_header.size);
            database_service->object_table.table_content.object_entry_ptr[transaction_object_table_ptr[index].header.object_id].header.state = DATABASE_CONFIG_ENTRY_VALID;
            fbe_spinlock_unlock(&database_service->object_table.table_lock);


            fbe_database_commit_object(transaction_object_table_ptr[index].header.object_id, transaction_object_table_ptr[index].db_class_id, FBE_FALSE);	
            break;
        case DATABASE_CONFIG_ENTRY_DESTROY:
            fbe_spinlock_lock(&database_service->object_table.table_lock);
            /* get the persit entry id from the in-memory service table */
            fbe_database_config_table_get_object_entry_by_id (&database_service->object_table,
                                                              transaction_object_table_ptr[index].header.object_id,
                                                              &service_object_table_ptr);
            fbe_spinlock_unlock(&database_service->object_table.table_lock);

            /*check if this is a system object entry and persistent the system object entry*/
            if (transaction_object_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_object_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE,
                                                                &transaction_object_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            } else {
                if (service_object_table_ptr->header.state != DATABASE_CONFIG_ENTRY_VALID )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: Did not find valid entry for object id 0x%x\n",
                                   __FUNCTION__,
                                   transaction_object_table_ptr[index].header.object_id);	
                }else{
                    /* send to persist */
                    
                    status = fbe_database_persist_delete_entry(persist_handle, service_object_table_ptr->header.entry_id);
                    if (status != FBE_STATUS_OK )
                    {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Persist failed to delete entry 0x%x, status: 0x%x\n",
                            __FUNCTION__, (unsigned int)service_object_table_ptr->header.entry_id, status);	
                    }

                    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: object 0x%x destroy persist entry 0x%llx completed!\n",
                               __FUNCTION__, 
                               transaction_object_table_ptr[index].header.object_id,
                               (unsigned long long)transaction_object_table_ptr[index].header.entry_id);
                }
				
            }

            /* update the in-memory service table, must be done after deleting the enrty from persist service. */				
            fbe_database_config_table_remove_object_entry(&database_service->object_table, 
                                                         &transaction_object_table_ptr[index]);
            break;
        case DATABASE_CONFIG_ENTRY_INVALID:  /* skip invalid entry*/
            break;
        case DATABASE_CONFIG_ENTRY_VALID:
        default:
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Trans. Object Table: index %d, Unexpected state:%d\n",
                __FUNCTION__, (int)index, transaction_object_table_ptr[index].header.state);
            break;
        }
        /* zero the memory, so no leftover data on the buffer */
        //fbe_zero_memory(&transaction_object_table_ptr[index], sizeof(database_object_entry_t));
    }

    /*check hook*/
    database_check_and_run_hook(NULL,FBE_DATABASE_HOOK_TYPE_WAIT_BEFORE_TRANSACTION_PERSIST, NULL);
    database_check_and_run_hook(NULL,FBE_DATABASE_HOOK_TYPE_PANIC_BEFORE_TRANSACTION_PERSIST, NULL);
    
    if (is_active_sp) {
        /*
          two independent transactions need commit here, we expect them commit in an
          atomic way. The system db persist operates raw-3way-mirror, while the persist
          service operates the DB lun. They use different journals. We must work out
          a solution to handle the case where one transaction committed and the other failed 
          (system panic here).currently we plan to handle this kind of failure case in the 
          database service boot path.
        */
        status = fbe_database_persist_commit_transaction(persist_handle, 
                                                         database_transaction_commit_completion_function,
                                                         &persist_transaction_commit_context);
        /* TODO:  need to revisit how the asynch is handled. */
        wait_status = fbe_semaphore_wait(&persist_transaction_commit_context.commit_semaphore, NULL);
        if (wait_status == FBE_STATUS_TIMEOUT || (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
            || persist_transaction_commit_context.commit_status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: commit persist tran fail. wait_status:%d send_req_status:%d commit_status:%d\n",
                __FUNCTION__, wait_status, status, persist_transaction_commit_context.commit_status);

            fbe_database_transaction_abort(database_service);
            /*abort the system db persist transaction*/
            fbe_database_system_db_persist_abort(system_db_persist_handle);

            fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
            
            return FBE_STATUS_GENERIC_FAILURE;
            
        } else {
            wait_status = fbe_database_system_db_persist_commit(system_db_persist_handle);
            if (wait_status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: system db persist transaction commit failed \n", __FUNCTION__);
            }
        }
        
        /* We have persisted and committed on the active SP.  Now commit
         * (i.e. update the in-memory entries) on the peer.
         */
        if(is_peer_update_allowed(database_service))
        {
            /* First commit (update the in-memory entries) and push to objects
             * the peer entries.
             */
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Persist transaction committed in memory, synching to peer...\n",
                           __FUNCTION__);

            status = fbe_database_transaction_operation_commit_on_peer(database_service);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Transacation 0x%x can't be committed on the peer but it's present\n",
                    __FUNCTION__, (unsigned int)transaction->transaction_id);
                wait_status = status;
        
                /*Do not abort as
                 *(1) cfg is already consistent between both SPs and between in-memory cfg and on-disk data
                 *(2) transaction on peer would be cleared when start next db transaction
                 *(3) if abort, need to rollback the on-disk cfg too, which may cause more error.
                 */
            }

            /*only active side leads the transaction invalidation operation.
            *passive side would not invalidate transaction by itself here. Only when
            *active side decides to invalidate db transaction can it do the invalidation.
            *The reason is that when active SP panics when it does persist, passive SP
            *can continue the persist when it becomes ACTIVE
            */    
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: begin to invalidate db transaction, synching to peer\n",
                           __FUNCTION__);
        
            status = fbe_database_transaction_operation_on_peer(FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_INVALIDATE,
                                                                database_service);
        
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Transacation 0x%x can't be invalidated on the peer but it's present\n",
                    __FUNCTION__, (unsigned int)transaction->transaction_id);
                wait_status = status;
        
                /*Do not abort as
                 *(1) cfg is already consistent between both SPs and between in-memory cfg and on-disk data
                 *(2) transaction on peer would be cleared when start next db transaction
                 *(3) if abort, need to rollback the on-disk cfg too, which may cause more error.
                 */
            }
        
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: transaction invalidated on peer, now invalidate ourselves'\n",
                __FUNCTION__);
        
        }
    
        fbe_zero_memory(database_service->transaction_ptr, sizeof(database_transaction_t));/*clean after ourselvs no matter what*/
        transaction->state = DATABASE_TRANSACTION_STATE_INACTIVE;
    } /*end if (database_common_cmi_is_active())*/
    else
    {
        /*passive side would not invalidate transaction by itself here. Only when
        *active side finishes all the transaction work can it do the invalidation.
        *The reason is that when active SP panics when it does persist, passive SP
        *can continue the persist when it becomes ACTIVE
        */    
    }

    fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);

    if (wait_status == FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Persist transaction committed successfully!\n",
            __FUNCTION__);
    }
    
    return wait_status;
}
/******************************************** 
 * end fbe_database_transaction_commit()
 ********************************************/

static fbe_status_t database_transaction_commit_completion_function (fbe_status_t commit_status, fbe_persist_completion_context_t context)
{
    database_transaction_commit_context_t * commit_context = (database_transaction_commit_context_t *)context;
    /* todo: change to debug trace level*/
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Persist transaction commit status: 0x%x\n",
        __FUNCTION__, commit_status);
    
    commit_context->commit_status = commit_status;
    fbe_semaphore_release(&(commit_context->commit_semaphore), 0, 1 ,FALSE);

    return FBE_STATUS_OK;
}

static fbe_bool_t fbe_database_transaction_does_object_exist(fbe_object_id_t object_id)
{
    fbe_status_t                               status;
    fbe_topology_mgmt_check_object_existent_t  check_object_existent;

    /* Populate the Object ID and set default exists to TRUE */
    check_object_existent.object_id = object_id;
    check_object_existent.exists_b = FBE_TRUE;

    /* Ask topology to check for object existent */
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_CHECK_OBJECT_EXISTENT,
                                             &check_object_existent,
                                             sizeof(fbe_topology_mgmt_check_object_existent_t),
                                             FBE_SERVICE_ID_TOPOLOGY,
                                             NULL,  /* no sg list*/
                                             0,  /* no sg list*/
                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                             FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "db_trans_does_object_exist: check for object id: 0x%x failed - status: 0x%x\n",
                       object_id, status);
        return FBE_FALSE;
    }

    return check_object_existent.exists_b;
}
/*!***************************************************************************
 *          fbe_database_transaction_abort_populate_update_from_transaction()
 *****************************************************************************
 *
 * @brief   Populate the update information from the transaction if needed.
 *
 * @param   temp_object_entry_p - Temporary object pointer (to modify)
 * @param   transaction_entry_p - Transaction object pointer with update information
 * @param   table_entry_p - The in-memory object entry  
 *
 * @return  bool - FBE_TRUE - Use temp_object_entry data
 *                 FBE_FALSE - Use object entry data
 *
 * @note    Currently we only correct the pvd update.
 *
 * @version
 *  04/09/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_bool_t fbe_database_transaction_abort_populate_update_from_transaction_if_needed(database_object_entry_t *temp_entry_p,
                                                                   database_object_entry_t *transaction_entry_p,
                                                                   database_object_entry_t *table_entry_p)
{
    fbe_class_id_t fbe_class_id;

    /* we have a valid entry */
    fbe_class_id = database_common_map_class_id_database_to_fbe(table_entry_p->db_class_id);
    switch(fbe_class_id) {
        case FBE_CLASS_ID_PROVISION_DRIVE:
            /* Validate the update type and mask */
            if ((table_entry_p->set_config_union.pvd_config.update_type != transaction_entry_p->set_config_union.pvd_config.update_type)                 ||
                (table_entry_p->set_config_union.pvd_config.update_type_bitmask != transaction_entry_p->set_config_union.pvd_config.update_type_bitmask)    ) {
                *temp_entry_p = *table_entry_p;
                temp_entry_p->set_config_union.pvd_config.update_type = transaction_entry_p->set_config_union.pvd_config.update_type;
                temp_entry_p->set_config_union.pvd_config.update_type_bitmask = transaction_entry_p->set_config_union.pvd_config.update_type_bitmask;
                return FBE_TRUE;
            }
            break;
        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            break;
        case FBE_CLASS_ID_STRIPER:
        case FBE_CLASS_ID_MIRROR:
        case FBE_CLASS_ID_PARITY:
            break;
//  case FBE_CLASS_ID_LUN:
//          break;
        default:
            break;
    }
    return FBE_FALSE;
}
/*****************************************************************************
 * end fbe_database_transaction_abort_populate_update_from_transaction_if_needed()
 ****************************************************************************/

fbe_status_t fbe_database_transaction_abort(fbe_database_service_t *database_service)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_transaction_t          *transaction = NULL;
    database_user_entry_t           *transaction_user_table_ptr = NULL;
    database_object_entry_t         *transaction_object_table_ptr = NULL;
    database_edge_entry_t           *transaction_edge_table_ptr = NULL;
    database_global_info_entry_t    *transaction_global_info_ptr = NULL;
    database_object_entry_t         *existing_object_entry=NULL;
    database_object_entry_t          temp_existing_object_entry;
    database_edge_entry_t           *existing_edge_entry=NULL;
    database_user_entry_t           *existing_user_entry=NULL;
    database_global_info_entry_t    *existing_global_info_entry = NULL; 
    fbe_s64_t index;

    transaction = database_service->transaction_ptr;
    transaction_user_table_ptr = transaction->user_entries;
    transaction_object_table_ptr = transaction->object_entries;
    transaction_edge_table_ptr = transaction->edge_entries;
    transaction_global_info_ptr = transaction->global_info_entries;

    if (is_peer_update_allowed(database_service)) {
        status = fbe_database_transaction_operation_on_peer(FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT, database_service);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Transaction 0x%llx can't start on the peer but it's present, no config change allowed\n",
                __FUNCTION__, (unsigned long long)transaction->transaction_id);
    
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Can not rollback if the transaction is not in active state or committing */
    if ((transaction->state != DATABASE_TRANSACTION_STATE_ACTIVE)&&
        (transaction->state != DATABASE_TRANSACTION_STATE_COMMIT))
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction 0x%llx is in %d state, can't rollback transaction.\n",
            __FUNCTION__, (unsigned long long)transaction->transaction_id,
	    transaction->state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    transaction->state = DATABASE_TRANSACTION_STATE_ROLLBACK;

    /* start rolling back */
    /* rollback should go the reverse order of what has been done, so start from the bottom of the table */
    /* for create, we destroy everything created so far*/
    /* for destory, we recreated what was destroyed base on the service tables */
    /* for modify, we change back to what was in the service tables */
    /* rollback the object first */
    for (index = DATABASE_TRANSACTION_MAX_OBJECTS - 1; index >= 0; index--)
    {

        switch (transaction_object_table_ptr[index].header.state){
        case DATABASE_CONFIG_ENTRY_CREATE:
            /*! @todo DE168 -  The check below is a temporary work-around so 
             *        so that we don't attempt to destroy an object that was
             *        never created.  The correct fix to not even attempt
             *        to allocate etc, unless the object can be created.
             */
            status = fbe_database_validate_object(transaction_object_table_ptr[index].header.object_id);
            if (status != FBE_STATUS_OK)
            {
                /*! @todo DE168 - Have found that the same object id is in the 
                 *        transaction table multiple times.  Since this is a
                 *        known issue the trace level is warning.
                 */
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DE168 - Transaction 0x%llx index: %lld duplicate object id: 0x%x found.\n",
                               __FUNCTION__, (unsigned long long)transaction->transaction_id, (long long)index, transaction_object_table_ptr[index].header.object_id);
            }
            else
            {
                /* Else destroy the object. 
                 */
                status = database_common_destroy_object_from_table_entry(&transaction_object_table_ptr[index]);
                if (status != FBE_STATUS_OK)
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: Transacation 0x%x failed to destory object 0x%x.\n",
                                   __FUNCTION__, (unsigned int)transaction->transaction_id, transaction_object_table_ptr[index].header.object_id);
                }
                status = database_common_wait_destroy_object_complete();
                if (status != FBE_STATUS_OK)
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: Transacation 0x%x Waiting for destory object failed 0x%x.\n",
                                   __FUNCTION__, (unsigned int)transaction->transaction_id, transaction_object_table_ptr[index].header.object_id);
                }
            }

            /*we delete the corresponding entries in the database service table, for system drive
              spare clone hack*/
            fbe_database_config_table_remove_object_entry(&database_service->object_table, &transaction_object_table_ptr[index]);
            break;
        case DATABASE_CONFIG_ENTRY_MODIFY:			
            /* get the entry in the service table base on the object id*/
            fbe_spinlock_lock(&database_service->object_table.table_lock);
            status = fbe_database_config_table_get_object_entry_by_id(&database_service->object_table,
                                                                    transaction_object_table_ptr[index].header.object_id,
                                                                    &existing_object_entry);	
            // we may need to hold this lock longer
            fbe_spinlock_unlock(&database_service->object_table.table_lock);
            			
            /*! @note Update the object configuration but there is a case where
             *        the in-memory object does not have the update information.
             *        If needed use a temporary object and add the update
             *        information to the data from the in-memory entry.
             */
            if (fbe_database_transaction_abort_populate_update_from_transaction_if_needed(&temp_existing_object_entry,
                                                                                          &transaction_object_table_ptr[index],
                                                                                          existing_object_entry)) {
                /* Use the temporary updated object entry.
                 */
                status = database_common_update_config_from_table_entry(&temp_existing_object_entry);
            } else {
                status = database_common_update_config_from_table_entry(existing_object_entry);
            }
            /*commit object*/
            fbe_database_commit_object(transaction_object_table_ptr[index].header.object_id, transaction_object_table_ptr[index].db_class_id, FBE_FALSE);
            if (status != FBE_STATUS_OK)
            {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Transacation 0x%x failed to update_config of object 0x%x.\n",
                __FUNCTION__, (unsigned int)transaction->transaction_id, transaction_object_table_ptr[index].header.object_id);
            }
            break;
        case DATABASE_CONFIG_ENTRY_DESTROY:
            /* update the in-memory service table */
            transaction_object_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;

            status = database_common_create_object_from_table_entry(&transaction_object_table_ptr[index]);
            if (status != FBE_STATUS_OK)
            {
                  database_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Failed to recreate object 0x%x\n",
                                    __FUNCTION__,transaction_object_table_ptr[index].header.object_id);           
            }
            else
            {            
                status = database_common_set_config_from_table_entry(&transaction_object_table_ptr[index]);
                if (status != FBE_STATUS_OK)
                {
                  database_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Failed to config object 0x%x\n",
                                    __FUNCTION__,transaction_object_table_ptr[index].header.object_id);          
                }
            }
            
            /* update the in-memory service table */
            fbe_database_config_table_update_object_entry(&database_service->object_table, 
                                                          &transaction_object_table_ptr[index]);
            if(transaction_object_table_ptr[index].db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)
            {
                /*If it's a PVD then connect it to the LDO*/
                status = fbe_database_add_pvd_to_be_connected(&database_service->pvd_operation, transaction_object_table_ptr[index].header.object_id);
                if (status != FBE_STATUS_OK)
                {
                      database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: Failed to connect PVD to LDO! Status=0x%x\n",
                                        __FUNCTION__,status);			
                }
             }

             fbe_database_commit_object(transaction_object_table_ptr[index].header.object_id, transaction_object_table_ptr[index].db_class_id, FBE_FALSE);
            
            break;
        case DATABASE_CONFIG_ENTRY_VALID:
        case DATABASE_CONFIG_ENTRY_INVALID:
        default:
            break;
        }
    }

    /* TODO: may need to keep the sequence of object and edge */
    for (index = DATABASE_TRANSACTION_MAX_EDGES-1; index >= 0; index--)
    { 
        switch (transaction_edge_table_ptr[index].header.state){
        case DATABASE_CONFIG_ENTRY_CREATE:
            /*! @note Since we destroy the objects before destroying the edges
             *        (should be the reverse order: destroy edges then objects),
             *        we must confirm if the object is still there before
             *        attempting to destroy the edge.
             */
            if (fbe_database_transaction_does_object_exist(transaction_edge_table_ptr[index].header.object_id)) {
                /* Since we are processing the entries in reverse order we will
                 * destroy any created edges before attempting to re-create the 
                 * original edge.
                 */
                status = database_common_destroy_edge_from_table_entry(&transaction_edge_table_ptr[index]);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Transaction 0x%llx failed to destroy edge object 0x%x, ind 0x%x.\n",
                                    __FUNCTION__,
                                   (unsigned long long)transaction->transaction_id, 
                                    transaction_edge_table_ptr[index].header.object_id, 
                                    transaction_edge_table_ptr[index].client_index);
                }
    
                /*! @note Validate that the edge is not in the in-memory table.
                 */
                status = fbe_database_config_table_get_edge_entry(&database_service->edge_table,
                                                                  transaction_edge_table_ptr[index].header.object_id,
                                                                  transaction_edge_table_ptr[index].client_index,
                                                                  &existing_edge_entry);
                if (status == FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Transaction 0x%llx destroy edge object 0x%x, ind 0x%x unexpected edge found\n",
                                    __FUNCTION__,
                                   (unsigned long long)transaction->transaction_id, 
                                    transaction_edge_table_ptr[index].header.object_id, 
                                    transaction_edge_table_ptr[index].client_index);
                }
            } /* end if client object exist */
            break;

        case DATABASE_CONFIG_ENTRY_DESTROY:	
            /*we have already saved the edges in the transaction table. create edges frm the table*/
            status = database_common_create_edge_from_table_entry(&transaction_edge_table_ptr[index]);
            if (status != FBE_STATUS_OK)
            {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Transaction 0x%llx failed to create edge object 0x%x, ind 0x%x.\n",
                                __FUNCTION__,
				(unsigned long long)transaction->transaction_id, 
                                transaction_edge_table_ptr[index].header.object_id, 
                                transaction_edge_table_ptr[index].client_index);
            }

            /*! @todo This shouldn't be necessary. */
            fbe_database_config_table_update_edge_entry(&database_service->edge_table, &transaction_edge_table_ptr[index]);
            break;

        case DATABASE_CONFIG_ENTRY_MODIFY: /* edge can not be modified */
        case DATABASE_CONFIG_ENTRY_VALID:
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Transaction 0x%llx: edge entry has unexpected state 0x%x.\n",
                __FUNCTION__, (unsigned long long)transaction->transaction_id,
		transaction_edge_table_ptr[index].header.state);
            break;
        case DATABASE_CONFIG_ENTRY_INVALID: /* skip the empty entry*/
        default:
            break;
        }
    }

    /*the user entries rollback only has effects on system spare entries*/
    for (index = DATABASE_TRANSACTION_MAX_USER_ENTRY - 1; index >= 0; index--)
    {
        switch(transaction_user_table_ptr[index].header.state)
        {
        case DATABASE_CONFIG_ENTRY_CREATE:
             /*we delete the corresponding entries in the database service table, for system drive
              spare clone hack*/			
            fbe_database_config_table_remove_user_entry(&database_service->user_table, &transaction_user_table_ptr[index]);
            break;
        case DATABASE_CONFIG_ENTRY_MODIFY:
            /* get the entry in the service table base on the object id*/
            fbe_spinlock_lock(&database_service->user_table.table_lock);
            status = fbe_database_config_table_get_user_entry_by_object_id(&database_service->user_table,
                                                                    transaction_user_table_ptr[index].header.object_id,
                                                                    &existing_user_entry);
            fbe_spinlock_unlock(&database_service->user_table.table_lock);

           
            fbe_database_commit_object(existing_user_entry->header.object_id, existing_user_entry->db_class_id, FBE_FALSE);
            if (status != FBE_STATUS_OK)
            {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Transacation 0x%x failed to update_config of object 0x%x.\n",
                __FUNCTION__, (unsigned int)transaction->transaction_id, transaction_object_table_ptr[index].header.object_id);
            }
            
            break;
        case DATABASE_CONFIG_ENTRY_DESTROY:
            transaction_user_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            /* update the in-memory service table */
            fbe_database_config_table_update_user_entry(&database_service->user_table, &transaction_user_table_ptr[index]);
         
            break;
        default:
            break;
        }
    }

    for (index = DATABASE_TRANSACTION_MAX_GLOBAL_INFO-1; index >= 0; index--)
    {
        switch (transaction_global_info_ptr[index].header.state){
        case DATABASE_CONFIG_ENTRY_CREATE:
        case DATABASE_CONFIG_ENTRY_VALID:
        case DATABASE_CONFIG_ENTRY_DESTROY:

            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Transaction 0x%llx: Global Info entry has unexpected state 0x%x.\n",
                __FUNCTION__, (unsigned long long)transaction->transaction_id,
		transaction_global_info_ptr[index].header.state);
            break;  

        case DATABASE_CONFIG_ENTRY_MODIFY: 
             /* Get the Global Info entry from the service table */	  
            fbe_spinlock_lock(&database_service->global_info.table_lock);         
            status = fbe_database_config_table_get_global_info_entry(&database_service->global_info, 
                                                             transaction_global_info_ptr[index].type, 
                                                             &existing_global_info_entry);
            fbe_spinlock_unlock(&database_service->global_info.table_lock); 

            status = database_common_update_global_info_from_table_entry(existing_global_info_entry);
            if (status != FBE_STATUS_OK)
            {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Transaction 0x%llx failed to update_global_info for type 0x%x.\n",
                __FUNCTION__, (unsigned long long)transaction->transaction_id,
	        transaction_global_info_ptr[index].type);
            }

            break;
                                  
        case DATABASE_CONFIG_ENTRY_INVALID: /* skip the empty entry*/
        default:
            break;
        }
    }

    
    fbe_zero_memory(transaction, sizeof(database_transaction_t));/*clean after ourselvs no matter what*/
    transaction->state = DATABASE_TRANSACTION_STATE_INACTIVE;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_transaction_get_id(database_transaction_t *transaction,
                                             fbe_database_transaction_id_t *transaction_id)
{
    *transaction_id = transaction->transaction_id;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_transaction_get_free_object_entry(database_transaction_t *transaction, 
                                                            database_object_entry_t **out_entry_ptr)
{
    /* Object entry can only be modified in active state */
    if (transaction->state != DATABASE_TRANSACTION_STATE_ACTIVE)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction 0x%llx is in %d state, can't provide object_entry!\n",
            __FUNCTION__, (unsigned long long)transaction->transaction_id,
	    transaction->state);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return fbe_database_config_table_get_free_object_entry(transaction->object_entries, 
        DATABASE_TRANSACTION_MAX_OBJECTS,
        out_entry_ptr);
}

fbe_status_t fbe_database_transaction_add_object_entry(database_transaction_t *transaction, 
                                                       database_object_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t * free_entry = NULL;
    status = fbe_database_transaction_get_free_object_entry(transaction, &free_entry);
    if ((status != FBE_STATUS_OK)||(free_entry == NULL))
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction 0x%llx can't get object_entry!\n",
            __FUNCTION__, (unsigned long long)transaction->transaction_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* we got a free entry from transaction table, fill it with data */
    fbe_copy_memory(free_entry, in_entry_ptr, sizeof(database_object_entry_t));
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_transaction_get_free_user_entry(database_transaction_t *transaction, 
                                                          database_user_entry_t **out_user_entry_ptr)
{
    /* user entry can only be modified in active state */
    if (transaction->state != DATABASE_TRANSACTION_STATE_ACTIVE)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction 0x%llx is in %d state, can't provide user_entry!\n",
            __FUNCTION__, (unsigned long long)transaction->transaction_id,
	    transaction->state);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return fbe_database_config_table_get_free_user_entry(transaction->user_entries, 
        DATABASE_TRANSACTION_MAX_USER_ENTRY,
        out_user_entry_ptr);
}

fbe_status_t fbe_database_transaction_add_user_entry(database_transaction_t *transaction, 
                                                     database_user_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t * free_entry = NULL;
    status = fbe_database_transaction_get_free_user_entry(transaction, &free_entry);
    if ((status != FBE_STATUS_OK)||(free_entry == NULL))
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction 0x%llx can't get user_entry!\n",
            __FUNCTION__, (unsigned long long)transaction->transaction_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* we got a free entry from transaction table, fill it with data */
    fbe_copy_memory(free_entry, in_entry_ptr, sizeof(database_user_entry_t));
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_transaction_get_free_edge_entry(database_transaction_t *transaction, 
                                                          database_edge_entry_t **out_entry_ptr)
{
    /* edge entry can only be modified in active state */
    if (transaction->state != DATABASE_TRANSACTION_STATE_ACTIVE)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction 0x%llx is in %d state, can't provide edge_entry!\n",
            __FUNCTION__, (unsigned long long)transaction->transaction_id,
	    transaction->state);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return fbe_database_config_table_get_free_edge_entry(transaction->edge_entries, 
        DATABASE_TRANSACTION_MAX_EDGES, 
        out_entry_ptr);
}

fbe_status_t fbe_database_transaction_add_edge_entry(database_transaction_t *transaction, 
                                                     database_edge_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_edge_entry_t * free_entry = NULL;
    status = fbe_database_transaction_get_free_edge_entry(transaction, &free_entry);
    if ((status != FBE_STATUS_OK)||(free_entry == NULL))
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction 0x%llx can't get edge_entry!\n",
            __FUNCTION__, (unsigned long long)transaction->transaction_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* we got a free entry from transaction table, fill it with data */
    fbe_copy_memory(free_entry, in_entry_ptr, sizeof(database_edge_entry_t));
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_transaction_add_global_info_entry
 *****************************************************************
 * @brief
 *   This function gets the global info entry from the database table.
 *
 * @param transaction - database transaction
 * @param out_entry_ptr - entry to be added
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/10/2011: Arun S - created
 *
 ****************************************************************/
fbe_status_t fbe_database_transaction_get_free_global_info_entry(database_transaction_t *transaction, 
                                                                 database_global_info_entry_t **out_entry_ptr)
{
    /* System Global Info entry can only be modified in active state */
    if (transaction->state != DATABASE_TRANSACTION_STATE_ACTIVE)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Transaction 0x%llx is in %d state, can't provide edge_entry!\n", 
                       __FUNCTION__,
		       (unsigned long long)transaction->transaction_id,
		       transaction->state);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return fbe_database_config_table_get_free_global_info_entry(transaction->global_info_entries, 
                                                                DATABASE_TRANSACTION_MAX_GLOBAL_INFO, 
                                                                out_entry_ptr);
}

/*!***************************************************************
 * @fn fbe_database_transaction_add_global_info_entry
 *****************************************************************
 * @brief
 *   This function adds the global info entry to the database table.
 *
 * @param transaction - database transaction
 * @param in_entry_ptr - entry to be added
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/10/2011: Arun S - created
 *
 ****************************************************************/
fbe_status_t fbe_database_transaction_add_global_info_entry(database_transaction_t *transaction, 
                                                            database_global_info_entry_t *in_entry_ptr)
{
    fbe_status_t                   status = FBE_STATUS_GENERIC_FAILURE;
    database_global_info_entry_t  *free_entry = NULL;

    status = fbe_database_transaction_get_free_global_info_entry(transaction, &free_entry);
    if ((status != FBE_STATUS_OK)||(free_entry == NULL))
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Transaction 0x%llx can't get global_info entry!\n", 
                       __FUNCTION__,
		       (unsigned long long)transaction->transaction_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* we got a free entry from transaction table, fill it with data */
    fbe_copy_memory(free_entry, in_entry_ptr, sizeof(database_global_info_entry_t));
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_transaction_is_valid_id
 *****************************************************************
 * @brief
 *   This function validates the transaction-id.
 *
 * @param packet - packet
 * @param packet_status - status to set for the packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/06/2011: Arun S - created
 *
 ****************************************************************/
fbe_status_t fbe_database_transaction_is_valid_id(fbe_database_transaction_id_t transaction_id, 
                                                  fbe_database_transaction_id_t db_transaction_id)
{
    /* Validate the transaction ids */
    if (transaction_id != db_transaction_id)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: Invalid transaction id 0x%llx != 0x%llx\n", __FUNCTION__,
                     (unsigned long long)transaction_id,
		     (unsigned long long)db_transaction_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/**************************************************************
 *  @brief
 *      Initialize fbe_database_cmi_transaction before sending
 *      to Peer SP by CMI message
 *  
 *  @version
 *  created by Gaohp 5/15/2012
 **************************************************************/
static void fbe_database_cmi_transaction_init(fbe_database_cmi_msg_t *msg_memory, 
                                              fbe_database_service_t *database_service)
{
	fbe_u32_t	entry_id_idx = 0;

    if (msg_memory == NULL || database_service == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: input parameter is NULL\n", __FUNCTION__);
        return;
    }

    msg_memory->payload.db_transaction.transaction_id = database_service->transaction_ptr->transaction_id;
    msg_memory->payload.db_transaction.state = database_service->transaction_ptr->state;
    msg_memory->payload.db_transaction.transaction_type = database_service->transaction_ptr->transaction_type;
    msg_memory->payload.db_transaction.job_number = database_service->transaction_ptr->job_number;
              
    msg_memory->payload.db_transaction.max_user_entries = DATABASE_TRANSACTION_MAX_USER_ENTRY;
    msg_memory->payload.db_transaction.max_object_entries = DATABASE_TRANSACTION_MAX_OBJECTS;
    msg_memory->payload.db_transaction.max_edge_entries = DATABASE_TRANSACTION_MAX_EDGES;
    msg_memory->payload.db_transaction.max_global_info_entries = DATABASE_TRANSACTION_MAX_GLOBAL_INFO;
              
    msg_memory->payload.db_transaction.user_entry_size = sizeof(database_user_entry_t);
    msg_memory->payload.db_transaction.object_entry_size = sizeof(database_object_entry_t);
    msg_memory->payload.db_transaction.edge_entry_size = sizeof(database_edge_entry_t);
    msg_memory->payload.db_transaction.global_info_entry_size = sizeof(database_global_info_entry_t);

    msg_memory->payload.db_transaction.user_entry_array_offset = (fbe_u32_t)&((database_cmi_transaction_t *)0)->user_entries;
    msg_memory->payload.db_transaction.object_entry_array_offset = (fbe_u32_t)&((database_cmi_transaction_t *)0)->object_entries;
    msg_memory->payload.db_transaction.edge_entry_array_offset = (fbe_u32_t)&((database_cmi_transaction_t *)0)->edge_entries;
    msg_memory->payload.db_transaction.global_info_entry_array_offset = (fbe_u32_t)&((database_cmi_transaction_t *)0)->global_info_entries;

    for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_USER_ENTRY; entry_id_idx++) {
        msg_memory->payload.db_transaction.user_entries[entry_id_idx].header.entry_id = 
            database_service->transaction_ptr->user_entries[entry_id_idx].header.entry_id;
    }

    for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_OBJECTS; entry_id_idx++) {
        msg_memory->payload.db_transaction.object_entries[entry_id_idx].header.entry_id = 
           database_service->transaction_ptr->object_entries[entry_id_idx].header.entry_id;
    }

    for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_EDGES; entry_id_idx++) {
        msg_memory->payload.db_transaction.edge_entries[entry_id_idx].header.entry_id = 
            database_service->transaction_ptr->edge_entries[entry_id_idx].header.entry_id;
    }

    /* we need to copy everything over */
    for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_GLOBAL_INFO; entry_id_idx++) {
        fbe_copy_memory(&msg_memory->payload.db_transaction.global_info_entries[entry_id_idx],
                        &database_service->transaction_ptr->global_info_entries[entry_id_idx],
                        sizeof(database_global_info_entry_t));

        //msg_memory->payload.db_transaction.global_info_entries[entry_id_idx] = 
        //    database_service->transaction_ptr->global_info_entries[entry_id_idx];
    }

    return;
}

/*ask the peer to start/commit/abort a transaction on it's end*/
static fbe_status_t fbe_database_transaction_operation_on_peer(fbe_cmi_msg_type_t transaction_operation_type,
															   fbe_database_service_t *database_service)
{
    fbe_database_peer_synch_context_t	peer_synch;
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_status_t						wait_status;

    /*send to the peer first*/
    fbe_semaphore_init(&peer_synch.semaphore, 0, 1);
    fbe_semaphore_init(&peer_transaction_confirm.semaphore, 0, 1);

    peer_transaction_confirm.status = FBE_STATUS_GENERIC_FAILURE;/*default value*/
    peer_synch.status = FBE_STATUS_GENERIC_FAILURE;/*default value*/
    
    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(msg_memory, sizeof(fbe_database_cmi_msg_t));
    msg_memory->msg_type = transaction_operation_type;
    msg_memory->completion_callback = fbe_database_transaction_operation_on_peer_completion;

    fbe_database_cmi_transaction_init(msg_memory, database_service);

    fbe_database_cmi_send_message(msg_memory, &peer_synch);

    wait_status = fbe_semaphore_wait_ms(&peer_synch.semaphore, 30000);
    fbe_semaphore_destroy(&peer_synch.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){

        fbe_semaphore_destroy(&peer_transaction_confirm.semaphore);

        /*peer may have been there when we started but just went away in mid transaction, in this case we just continue as ususal*/
        if (!database_common_peer_is_alive()) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: Persist transaction, peer died, we continue.\n",__FUNCTION__); 
            return FBE_STATUS_OK;
        }else{
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: Persist transaction request on peer timed out!!!\n",__FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /*is the peer even there ?*/
    if (peer_synch.status == FBE_STATUS_NO_DEVICE) {
         fbe_semaphore_destroy(&peer_transaction_confirm.semaphore);
        /*nope, so let's go on*/
        return FBE_STATUS_OK;
    }

    if (peer_synch.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to send command to peer!!!\n",__FUNCTION__);	

        fbe_semaphore_destroy(&peer_transaction_confirm.semaphore);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*we are not done yet. Since the CMI is completly asynch, the fact we got a good status here does not really mean
    the peer was succesfull in starting a transaction, we have to wait for his message to confirm we can go on*/
    wait_status = fbe_semaphore_wait_ms(&peer_transaction_confirm.semaphore, 30000);
    fbe_semaphore_destroy(&peer_transaction_confirm.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){
        /*peer may have been there when we started but just went away in mid transaction, in this case we just continue as ususal*/
        if (!database_common_peer_is_alive()) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: Persist transaction confirm, peer died, we continue.\n",__FUNCTION__); 
            return FBE_STATUS_OK;
        }else{
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: Persist transaction confirm on peer timed out!!!\n",__FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (peer_transaction_confirm.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Persist transaction confirm on peer failed!!\n",__FUNCTION__);	
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return peer_transaction_confirm.status;
}

static fbe_status_t fbe_database_transaction_operation_on_peer_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                          fbe_status_t completion_status,
                                                                          void *context)
{
    fbe_database_peer_synch_context_t *	peer_synch_ptr = (fbe_database_peer_synch_context_t *)context;

    /*let's return the message memory first*/
    fbe_database_cmi_return_msg_memory(returned_msg);
    peer_synch_ptr->status = completion_status;
    fbe_semaphore_release(&peer_synch_ptr->semaphore, 0, 1 ,FALSE);

    return FBE_STATUS_OK;

}

/*Active peer asked us to start a transaction*/
fbe_status_t fbe_database_transaction_start_request_from_peer(fbe_database_service_t *db_service,
                                                              fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_status_t					status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_cmi_msg_t * 		msg_memory = NULL;
    fbe_database_transaction_info_t transaction_info;
    fbe_database_state_t 			database_state;
    fbe_u32_t                       msg_size;

    msg_size = fbe_database_cmi_get_msg_size(db_cmi_msg->msg_type);

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }
	
    /* If the received cmi msg has a larger size, version information will be sent back in the confirm message */
    if(db_cmi_msg->version_header.size > msg_size) {
        msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_START_CONFIRM;
        msg_memory->completion_callback = fbe_database_transaction_start_request_from_peer_completion;
        msg_memory->payload.transaction_confirm.status = FBE_STATUS_GENERIC_FAILURE;

        msg_memory->payload.transaction_confirm.err_type = FBE_CMI_MSG_ERR_TYPE_LARGER_MSG_SIZE;
        msg_memory->payload.transaction_confirm.received_msg_type = db_cmi_msg->msg_type;
        msg_memory->payload.transaction_confirm.sep_version = SEP_PACKAGE_VERSION;
		database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
					   "%s: received msg 0x%x, size(%d) is larger than local cmi msg size(%d)!!\n",
					   __FUNCTION__, db_cmi_msg->msg_type,
                       db_cmi_msg->version_header.size, msg_size);	
        fbe_database_cmi_send_message(msg_memory, NULL);
        return FBE_STATUS_OK;
    }

    transaction_info.job_number = db_cmi_msg->payload.db_transaction.job_number;
    transaction_info.transaction_id = db_cmi_msg->payload.db_transaction.transaction_id;
    transaction_info.transaction_type = db_cmi_msg->payload.db_transaction.transaction_type;

    get_database_service_state(db_service,  &database_state);
    if (database_state != FBE_DATABASE_STATE_READY) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: received msg 0x%x, When DB is in %d state. !!\n",
                       __FUNCTION__, db_cmi_msg->msg_type, database_state);	
        status = FBE_STATUS_OK;  /* DB service is not ready yet, just return ok*/
    }else{
        status = fbe_database_transaction_start(db_service, &transaction_info);
    }

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_START_CONFIRM;
    msg_memory->completion_callback = fbe_database_transaction_start_request_from_peer_completion;
    msg_memory->payload.transaction_confirm.status = status;

    fbe_database_cmi_send_message(msg_memory, NULL);
    
    return FBE_STATUS_OK;

}


/*passive peer confirms he was able to start a transaction successfully*/
fbe_status_t fbe_database_transaction_start_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_cmi_msg_transaction_confirm_t * confirm = &db_cmi_msg->payload.transaction_confirm;

    peer_transaction_confirm.status = confirm->status;
    fbe_semaphore_release(&peer_transaction_confirm.semaphore, 0, 1 ,FALSE);
    
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_database_transaction_start_request_from_peer_completion(fbe_database_cmi_msg_t *returned_msg, 
                                                                                fbe_status_t completion_status, 
                                                                                void *context)
{
    /*let's return the message memory and for now that's all we need to do*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_database_transaction_invalidate_request_from_peer_completion(fbe_database_cmi_msg_t *returned_msg, 
                                                                                fbe_status_t completion_status, 
                                                                                void *context)
{
    /*let's return the message memory and for now that's all we need to do*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    return FBE_STATUS_OK;

}

/*Active peer asked us to commit a transaction*/
fbe_status_t fbe_database_transaction_commit_request_from_peer(fbe_database_service_t *db_service,
                                                               fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_status_t					status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_cmi_msg_t * 		msg_memory = NULL;
    fbe_database_state_t 			database_state;
	fbe_u32_t						entry_id_idx = 0;
    fbe_u32_t                       msg_size;

    msg_size = fbe_database_cmi_get_msg_size(db_cmi_msg->msg_type);
    /* If the received cmi msg has a larger size, version information will be sent back in the confirm message */
    if(db_cmi_msg->version_header.size > msg_size) {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT_CONFIRM;
        msg_memory->completion_callback = fbe_database_transaction_commit_request_from_peer_completion;
        msg_memory->payload.transaction_confirm.status = FBE_STATUS_GENERIC_FAILURE;

        msg_memory->payload.transaction_confirm.err_type = FBE_CMI_MSG_ERR_TYPE_LARGER_MSG_SIZE;
        msg_memory->payload.transaction_confirm.received_msg_type = db_cmi_msg->msg_type;
        msg_memory->payload.transaction_confirm.sep_version = SEP_PACKAGE_VERSION;

		database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
					   "%s: received msg 0x%x, size(%d) is larger than local cmi msg size(%d)!!\n",
					   __FUNCTION__, db_cmi_msg->msg_type,
                       db_cmi_msg->version_header.size, msg_size);	
        fbe_database_cmi_send_message(msg_memory, NULL);
        return FBE_STATUS_OK;
    }

    get_database_service_state(db_service,  &database_state);
    if (database_state != FBE_DATABASE_STATE_READY) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: received msg 0x%x, When DB is in %d state. !!\n",
                       __FUNCTION__, db_cmi_msg->msg_type, database_state);	
        status = FBE_STATUS_OK;  /* DB service is not ready yet.*/
    }else{
        status = FBE_STATUS_OK;
        
        if(db_service->transaction_ptr->job_number!= db_cmi_msg->payload.db_transaction.job_number)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: received job no 0x%x != current job no 0x%x !\n",
                           __FUNCTION__, 
                           (unsigned int)db_cmi_msg->payload.db_transaction.job_number,
                           (unsigned int)db_service->transaction_ptr->job_number);
            
            msg_memory = fbe_database_cmi_get_msg_memory();
            if (msg_memory == NULL) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);
            
                return FBE_STATUS_GENERIC_FAILURE;
            }
            
            /*we'll return the status as a handshake to the master*/
            msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT_CONFIRM;
            msg_memory->completion_callback = fbe_database_transaction_abort_request_from_peer_completion;
            msg_memory->payload.transaction_confirm.status = FBE_STATUS_GENERIC_FAILURE;

            msg_memory->payload.transaction_confirm.err_type = FBE_CMI_MSG_ERR_TYPE_DISMATCH_TRANSACTION;
            msg_memory->payload.transaction_confirm.received_msg_type = db_cmi_msg->msg_type;
            msg_memory->payload.transaction_confirm.sep_version = SEP_PACKAGE_VERSION;

            fbe_database_cmi_send_message(msg_memory, NULL);
            return FBE_STATUS_OK;
        }

        /*before the commit, let's copy the transaction table the peer send us since it contains
        all the entry id's for the persist service which we'll use in case the peer goes down and the user want's
        to remove or modify an entry*/
        for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_USER_ENTRY; entry_id_idx++) {
            db_service->transaction_ptr->user_entries[entry_id_idx].header.entry_id = 
                db_cmi_msg->payload.db_transaction.user_entries[entry_id_idx].header.entry_id;
        }

        for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_OBJECTS; entry_id_idx++) {
            db_service->transaction_ptr->object_entries[entry_id_idx].header.entry_id = 
                db_cmi_msg->payload.db_transaction.object_entries[entry_id_idx].header.entry_id;
        }

        for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_EDGES; entry_id_idx++) {
            db_service->transaction_ptr->edge_entries[entry_id_idx].header.entry_id = 
                db_cmi_msg->payload.db_transaction.edge_entries[entry_id_idx].header.entry_id;
        }

        /* we need to copy everything over */
        for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_GLOBAL_INFO; entry_id_idx++) {
            db_service->transaction_ptr->global_info_entries[entry_id_idx] = 
                db_cmi_msg->payload.db_transaction.global_info_entries[entry_id_idx];
        }

        status = fbe_database_transaction_commit(db_service);
    }

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT_CONFIRM;
    msg_memory->completion_callback = fbe_database_transaction_commit_request_from_peer_completion;
    msg_memory->payload.transaction_confirm.status = status;


    fbe_database_cmi_send_message(msg_memory, NULL);
    
    return FBE_STATUS_OK;

}

fbe_status_t fbe_database_transaction_commit_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_cmi_msg_transaction_confirm_t * confirm = &db_cmi_msg->payload.transaction_confirm;

    peer_transaction_confirm.status = confirm->status;
    fbe_semaphore_release(&peer_transaction_confirm.semaphore, 0, 1 ,FALSE);
    
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_database_transaction_commit_request_from_peer_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context)
{
    /*let's return the message memory and for now that's all we need to do*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    return FBE_STATUS_OK;

}

fbe_status_t fbe_database_transaction_abort_request_from_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_status_t					status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_cmi_msg_t * 		msg_memory = NULL;
    fbe_database_state_t 			database_state;
    fbe_u32_t                       msg_size;

    msg_size = fbe_database_cmi_get_msg_size(db_cmi_msg->msg_type);
    /* If the received cmi msg has a larger size, don't handle this CMI message.
       Version information and error will be sent back in the confirm message */
    if(db_cmi_msg->version_header.size > msg_size) {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*we'll return the status as a handshake to the master*/
        msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT_CONFIRM;
        msg_memory->completion_callback = fbe_database_transaction_abort_request_from_peer_completion;
        msg_memory->payload.transaction_confirm.status = FBE_STATUS_GENERIC_FAILURE;

        msg_memory->payload.transaction_confirm.err_type = FBE_CMI_MSG_ERR_TYPE_LARGER_MSG_SIZE;
        msg_memory->payload.transaction_confirm.received_msg_type = db_cmi_msg->msg_type;
        msg_memory->payload.transaction_confirm.sep_version = SEP_PACKAGE_VERSION;

		database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
					   "%s: received msg 0x%x, size(%d) is larger than local cmi msg size(%d)!!\n",
					   __FUNCTION__, db_cmi_msg->msg_type,
                       db_cmi_msg->version_header.size, msg_size);	
        fbe_database_cmi_send_message(msg_memory, NULL);
        return FBE_STATUS_OK;
    }

    get_database_service_state(db_service,  &database_state);
    if (database_state != FBE_DATABASE_STATE_READY) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: received msg 0x%x, When DB is in %d state. !!\n",
                       __FUNCTION__, db_cmi_msg->msg_type, database_state);	
        status = FBE_STATUS_OK;  /* DB service is not ready yet.*/
    }
    else
    {
        status = FBE_STATUS_OK;
        
        if(db_service->transaction_ptr->job_number!= db_cmi_msg->payload.db_transaction.job_number)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: received job no 0x%x != current job no 0x%x !\n",
                           __FUNCTION__, 
                           (unsigned int)db_cmi_msg->payload.db_transaction.job_number,
                           (unsigned int)db_service->transaction_ptr->job_number);
            
            msg_memory = fbe_database_cmi_get_msg_memory();
            if (msg_memory == NULL) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);
            
                return FBE_STATUS_GENERIC_FAILURE;
            }
            
            /*we'll return the status as a handshake to the master*/
            msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT_CONFIRM;
            msg_memory->completion_callback = fbe_database_transaction_abort_request_from_peer_completion;
            msg_memory->payload.transaction_confirm.status = FBE_STATUS_GENERIC_FAILURE;

            msg_memory->payload.transaction_confirm.err_type = FBE_CMI_MSG_ERR_TYPE_DISMATCH_TRANSACTION;
            msg_memory->payload.transaction_confirm.received_msg_type = db_cmi_msg->msg_type;
            msg_memory->payload.transaction_confirm.sep_version = SEP_PACKAGE_VERSION;

            fbe_database_cmi_send_message(msg_memory, NULL);
            return FBE_STATUS_OK;
        }
    
        status = fbe_database_transaction_abort(db_service);
    }

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT_CONFIRM;
    msg_memory->completion_callback = fbe_database_transaction_abort_request_from_peer_completion;
    msg_memory->payload.transaction_confirm.status = status;

    fbe_database_cmi_send_message(msg_memory, NULL);
    
    return FBE_STATUS_OK;

}


fbe_status_t fbe_database_transaction_abort_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_cmi_msg_transaction_confirm_t * confirm = &db_cmi_msg->payload.transaction_confirm;

    peer_transaction_confirm.status = confirm->status;
    fbe_semaphore_release(&peer_transaction_confirm.semaphore, 0, 1 ,FALSE);
    
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_database_transaction_abort_request_from_peer_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context)
{
    /*let's return the message memory and for now that's all we need to do*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    return FBE_STATUS_OK;

}

fbe_config_generation_t fbe_database_transaction_get_next_generation_id(fbe_database_service_t *database_service)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    database_transaction_t          *transaction = database_service->transaction_ptr;
    database_global_info_entry_t    *global_info_entry_ptr = NULL;
    database_global_info_entry_t    *service_global_info_ptr = NULL;

    fbe_u64_t index;
    /* we use one of the global info entries to keep the generation id when generation id is needed in a transaction*/

    /* go thru the global entries to see if we already have the generation id entry */
    global_info_entry_ptr = transaction->global_info_entries;
    for (index = 0; index < DATABASE_TRANSACTION_MAX_GLOBAL_INFO; index++)
    {
        if (global_info_entry_ptr->type == DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION)
        {
            global_info_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_MODIFY;
            global_info_entry_ptr->info_union.generation_info.current_generation_number++;
            return (global_info_entry_ptr->info_union.generation_info.current_generation_number);
        }
    }
    /* if we gets to here, we did not find a generation entry,
       let's create one and copy the number in service table to here.
       When the transaction is committed, the generation number will be persisted and saved back to service table. */
    status = fbe_database_transaction_get_free_global_info_entry(transaction, &global_info_entry_ptr);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: transaction is out of global info entry. You need to increase it!!\n",
                       __FUNCTION__);

    }
    fbe_spinlock_lock(&database_service->global_info.table_lock);
    /* Get the in-memory service table */
    fbe_database_config_table_get_global_info_entry(&database_service->global_info, 
                                                    DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION, 
                                                    &service_global_info_ptr);
    fbe_copy_memory(global_info_entry_ptr, service_global_info_ptr, sizeof(database_global_info_entry_t));
    fbe_spinlock_unlock(&database_service->global_info.table_lock);

    global_info_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_MODIFY;
    global_info_entry_ptr->info_union.generation_info.current_generation_number++;
	return (global_info_entry_ptr->info_union.generation_info.current_generation_number);
}

/* Get the number of PVD creations that exist in the current transaction */
fbe_u32_t fbe_database_transaction_get_num_create_pvds(database_transaction_t *transaction)
{
    database_object_entry_t * object_entry_ptr;
    fbe_u32_t index = 0;
    fbe_u32_t num_pvds = 0;

    for(index = 0; index < DATABASE_TRANSACTION_MAX_OBJECTS; index++) {
        object_entry_ptr = &transaction->object_entries[index];
        if(object_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_CREATE &&
           object_entry_ptr->db_class_id == database_common_map_class_id_fbe_to_database(FBE_CLASS_ID_PROVISION_DRIVE)) 
        {
            num_pvds++;
        }
    }
    return num_pvds;
}

static fbe_status_t database_persist_transaction_abort_completion_function (fbe_status_t commit_status, fbe_persist_completion_context_t context)
{
    database_transaction_commit_context_t * commit_context = (database_transaction_commit_context_t *)context;
    /* todo: change to debug trace level*/
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Persist transaction commit status: 0x%x\n",
        __FUNCTION__, commit_status);
    
    commit_context->commit_status = commit_status;
    fbe_semaphore_release(&(commit_context->commit_semaphore), 0, 1 ,FALSE);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_no_tran_persist_entries(fbe_u8_t *entries, fbe_u32_t entry_num,
                                          fbe_u32_t entry_size, 
                                          fbe_persist_sector_type_t type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_status_t wait_status = FBE_STATUS_OK;
    fbe_u32_t index = 0;
    fbe_persist_transaction_handle_t persist_handle;
    fbe_persist_entry_id_t entry_id;
    database_table_header_t *entry_header = NULL;
    fbe_bool_t    abort_persist = FBE_FALSE;
    fbe_u32_t       retry_count=0;

    if (!entries || entry_num == 0 || entry_size == 0 ) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* open a persist service transaction get a handle back */
     do{
         status =  fbe_database_persist_start_transaction(&persist_handle);
         database_trace(FBE_TRACE_LEVEL_WARNING, 
                         FBE_TRACE_MESSAGE_ID_INFO, 
                         "DB_no_tran_persist_entries:start persist transaction, status:%d, retry: %d\n",
                         status, retry_count);
         if (status == FBE_STATUS_BUSY)
         {
             /* Wait 3 second */
             fbe_thread_delay(FBE_START_TRANSACTION_RETRY_INTERVAL);
             retry_count++;
         }
     }while((status == FBE_STATUS_BUSY) && (retry_count < FBE_START_TRANSACTION_RETRIES));
    
     if (status != FBE_STATUS_OK)
     {
         database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s :start persist transaction failed, status: 0x%x\n",
                         __FUNCTION__, status);
         return status;
     }

    abort_persist = FBE_FALSE;
    /*persist service can only support writing valid object, user or edge 
      entries to the database LUN. so check the validity of entries here.*/
    for (index = 0; index < entry_num; index ++) {
        /*we can assume that all the entry have a common database_table_header_t 
          at the start*/
        entry_header = (database_table_header_t *)(entries + index * entry_size);
        if (entry_header->state != DATABASE_CONFIG_ENTRY_VALID) {
            continue;
        }
        entry_id = 0;   /* Auto entry id */
        status = fbe_database_persist_write_entry(persist_handle,
                                                  type,
                                                  entries + index * entry_size,
                                                  entry_size,
                                                  &entry_id);
        if (status != FBE_STATUS_OK )
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Persist failed to write entry, status: 0x%x\n",
                            __FUNCTION__, status);
            /*abort the persist transaction*/
            abort_persist = FBE_TRUE;
            break;	
        }
    }

    fbe_semaphore_init(&persist_transaction_commit_context.commit_semaphore, 0, 1);

    /*commit or abort the persist transaction*/
    if (!abort_persist) {
        status = fbe_database_persist_commit_transaction(persist_handle, 
                                                         database_transaction_commit_completion_function,
                                                         &persist_transaction_commit_context);
    } else {
        status = fbe_database_persist_abort_transaction(persist_handle, 
                                                         database_persist_transaction_abort_completion_function,
                                                         &persist_transaction_commit_context);
    }

    /* Wait for persist commit or abort to complete. */
    wait_status = fbe_semaphore_wait_ms(&persist_transaction_commit_context.commit_semaphore, 180000);
    if (wait_status == FBE_STATUS_TIMEOUT || (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
            || persist_transaction_commit_context.commit_status != FBE_STATUS_OK)
    {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: commit or abort persist tran fail.\n",
                __FUNCTION__);
            if (wait_status != FBE_STATUS_TIMEOUT) {
                fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
            }

            return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);

    /*if abort the persist transaction, should return caller the persist operation failure*/
    if (abort_persist) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}


/*Active peer asked us to invalidate a transaction*/
fbe_status_t fbe_database_transaction_invalidate_request_from_peer(fbe_database_service_t * db_service,fbe_database_cmi_msg_t * db_cmi_msg)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_database_cmi_msg_t *    msg_memory = NULL;
    fbe_database_state_t        database_state;
    fbe_u32_t                   msg_size;

    msg_size = fbe_database_cmi_get_msg_size(db_cmi_msg->msg_type);

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the received cmi msg has a larger size, version information will be sent back in the confirm message */
    if(db_cmi_msg->version_header.size > msg_size) {
        msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_INVALIDATE_CONFIRM;
        msg_memory->completion_callback = fbe_database_transaction_invalidate_request_from_peer_completion;
        msg_memory->payload.transaction_confirm.status = FBE_STATUS_GENERIC_FAILURE;

        msg_memory->payload.transaction_confirm.err_type = FBE_CMI_MSG_ERR_TYPE_LARGER_MSG_SIZE;
        msg_memory->payload.transaction_confirm.received_msg_type = db_cmi_msg->msg_type;
        msg_memory->payload.transaction_confirm.sep_version = SEP_PACKAGE_VERSION;
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: received msg 0x%x, size(%d) is larger than local cmi msg size(%d)!!\n",
                        __FUNCTION__, db_cmi_msg->msg_type,
                        db_cmi_msg->version_header.size, msg_size);
        fbe_database_cmi_send_message(msg_memory, NULL);
        return FBE_STATUS_OK;
    }

    get_database_service_state(db_service,  &database_state);
    if (database_state != FBE_DATABASE_STATE_READY) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: received msg 0x%x, When DB is in %d state. !!\n",
                       __FUNCTION__, db_cmi_msg->msg_type, database_state);	
        status = FBE_STATUS_OK;  /* DB service is not ready yet, just return ok*/
    }else{
        status = FBE_STATUS_OK;
        
        if(db_service->transaction_ptr->job_number!= db_cmi_msg->payload.db_transaction.job_number)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: received job no 0x%x != current job no 0x%x !\n",
                           __FUNCTION__, 
                           (unsigned int)db_cmi_msg->payload.db_transaction.job_number,
                           (unsigned int)db_service->transaction_ptr->job_number);
            status = FBE_STATUS_GENERIC_FAILURE;
        }

        if(status != FBE_STATUS_OK)
        {
            msg_memory = fbe_database_cmi_get_msg_memory();
            if (msg_memory == NULL) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);
            
                return FBE_STATUS_GENERIC_FAILURE;
            }
            
            /*we'll return the status as a handshake to the master*/
            msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT_CONFIRM;
            msg_memory->completion_callback = fbe_database_transaction_abort_request_from_peer_completion;
            msg_memory->payload.transaction_confirm.status = FBE_STATUS_GENERIC_FAILURE;

            msg_memory->payload.transaction_confirm.err_type = FBE_CMI_MSG_ERR_TYPE_DISMATCH_TRANSACTION;
            msg_memory->payload.transaction_confirm.received_msg_type = db_cmi_msg->msg_type;
            msg_memory->payload.transaction_confirm.sep_version = SEP_PACKAGE_VERSION;

            fbe_database_cmi_send_message(msg_memory, NULL);
            return FBE_STATUS_OK;
        }
    
        fbe_zero_memory(db_service->transaction_ptr, sizeof(database_transaction_t));/*clean after ourselvs no matter what*/
        db_service->transaction_ptr->state = DATABASE_TRANSACTION_STATE_INACTIVE;
    }

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_INVALIDATE_CONFIRM;
    msg_memory->completion_callback = fbe_database_transaction_invalidate_request_from_peer_completion;
    msg_memory->payload.transaction_confirm.status = status;

    fbe_database_cmi_send_message(msg_memory, NULL);
    
    return FBE_STATUS_OK;

}


/*passive peer confirms it was able to invalidate the transaction successfully*/
fbe_status_t fbe_database_transaction_invalidate_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_cmi_msg_transaction_confirm_t * confirm = &db_cmi_msg->payload.transaction_confirm;

    peer_transaction_confirm.status = confirm->status;
    fbe_semaphore_release(&peer_transaction_confirm.semaphore, 0, 1 ,FALSE);
    
    return FBE_STATUS_OK;
}

/*!***********************************************************************************************************
 * @fn fbe_database_process_incomplete_transaction_after_peer_died
 *************************************************************************************************************
 * @brief
 *   Based on the database transaction state (INACTIVE, ACTIVE, COMMIT), handle incomplete job.
 *   The purpuse is to restore the consistency of configuration data when it becomes active SP after peer dies,
 *   either by roll backing the job operations or continuing to finish the job operations.
 *
 * @param  database_service - database service pointer
 *
 * @return FBE_STATUS_OK if there is no error
 *
 * @version
 *    11/18/2012:   created Zhipeng Hu
 ****************************************************************/
fbe_status_t fbe_database_process_incomplete_transaction_after_peer_died(fbe_database_service_t *database_service_ptr)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    database_transaction_state_t tran_state = DATABASE_TRANSACTION_STATE_INACTIVE;
    fbe_u64_t   job_number;

    if(NULL == database_service_ptr)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid parameter\n",
                       __FUNCTION__);
        return status;
    }

    tran_state = database_service_ptr->transaction_ptr->state;

    switch(tran_state)
    {
    case DATABASE_TRANSACTION_STATE_INACTIVE:
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "db_proc_incomplete_tran: state INACTIVE, do nothing.\n");
        status = FBE_STATUS_OK;
        break;

    case DATABASE_TRANSACTION_STATE_ACTIVE:
        job_number  =  database_service_ptr->transaction_ptr->job_number;
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "db_proc_incomplete_tran: job no 0x%llx state ACTIVE, replay: %d rollback the config changes.\n",
                       job_number, fbe_database_service.journal_replayed);
        if(fbe_database_service.journal_replayed == FBE_TRI_STATE_TRUE)
        {
            status = fbe_database_transaction_rollback(database_service_ptr);
            if(FBE_STATUS_OK != status)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "db_proc_incomplete_tran: transaction rollback failed.\n");
            }
        } 
        else
        {
            status = fbe_database_transaction_abort(database_service_ptr);
            if(FBE_STATUS_OK != status)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "db_proc_incomplete_tran: transaction abort failed.\n");
            }
        }
        break;

    case DATABASE_TRANSACTION_STATE_COMMIT:
        job_number  =  database_service_ptr->transaction_ptr->job_number;
        if(fbe_database_service.journal_replayed == FBE_TRI_STATE_TRUE)
        {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "db_proc_incomplete_tran: job no 0x%llx state COMMIT, complete config changes.\n",
                       job_number);
            status = fbe_database_transaction_commit_after_peer_died(database_service_ptr);
            if (status == FBE_STATUS_OK) {
                fbe_job_queue_element_t job_notification;
                job_notification.job_number = job_number;
                /*let's send the transaction success notification to job service so that the job will
                 * be completed successfully*/
                status = fbe_database_send_packet_to_service(FBE_JOB_CONTROL_CODE_MARK_JOB_DONE,
                                                         &job_notification,
                                                         sizeof(job_notification),
                                                         FBE_SERVICE_ID_JOB_SERVICE,
                                                         NULL,  /* no sg list*/
                                                         0,  /* no sg list*/
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         FBE_PACKAGE_ID_SEP_0);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                               "%s: fail to send notification to job service for job 0x%x.\n", 
                               __FUNCTION__, (unsigned int)job_number);
                }
            }
        }
        else if(fbe_database_service.journal_replayed == FBE_TRI_STATE_FALSE)
        {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "db_proc_incomplete_tran: job no 0x%llx state COMMIT, rollback the config changes.\n",
                       job_number);
            status = fbe_database_transaction_abort(database_service_ptr);
        }
        else
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "db_proc_incomplete_tran: Persist journal state unknown\n");
        }
        
        if(status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "db_proc_incomplete_tran: job no 0x%llx state COMMIT, transaction config change failed.\n",
                           job_number);
        }

        break;

    case DATABASE_TRANSACTION_STATE_ROLLBACK:
        job_number  =  database_service_ptr->transaction_ptr->job_number;
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "db_proc_incomplete_tran: job no 0x%llx state ROLLBACK, rollback the transaction.\n",
                       job_number);
        
        status = fbe_database_transaction_abort(database_service_ptr);
        if(FBE_STATUS_OK != status)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "db_proc_incomplete_tran: transaction rollback failed.\n");
        }
        break;

    default:
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: unknown tran_state: %d.\n",
                       __FUNCTION__, tran_state);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(database_service_ptr->transaction_ptr, sizeof(database_transaction_t));
    database_service_ptr->transaction_ptr->state = DATABASE_TRANSACTION_STATE_INACTIVE;

    return status;
}
/*!***********************************************************************************************************
 * @end fbe_database_process_incomplete_transaction_after_peer_died
 *************************************************************************************************************/

/*!***********************************************************************************************************
 * @fn fbe_database_transaction_commit_after_peer_died
 *************************************************************************************************************
 * @brief
 *  When the active SP dies during a job execution, the surviving SP would call this function to continue to
 *  commit the remaining db transaction which is in commit state
 *
 * @param  database_service - database service pointer
 *
 * @return FBE_STATUS_OK if there is no error
 *
 * @version
 *    11/21/2012:   created Zhipeng Hu
 ****************************************************************/
fbe_status_t fbe_database_transaction_commit_after_peer_died(fbe_database_service_t *database_service)
{
    fbe_status_t            status = FBE_STATUS_OK;

    database_transaction_t           *transaction = NULL;
    database_user_entry_t            *transaction_user_table_ptr = NULL;
    database_object_entry_t          *transaction_object_table_ptr = NULL;
    database_edge_entry_t            *transaction_edge_table_ptr = NULL;
    database_global_info_entry_t     *transaction_global_info_ptr = NULL;
    database_global_info_entry_t     *service_global_info_ptr = NULL;

    fbe_persist_transaction_handle_t persist_handle; 
    fbe_status_t                     wait_status = FBE_STATUS_OK;

    fbe_u64_t index;
    fbe_object_id_t last_system_object_id;
    fbe_database_system_db_persist_handle_t system_db_persist_handle;

    fbe_semaphore_init(&persist_transaction_commit_context.commit_semaphore, 0, 1);

    transaction = database_service->transaction_ptr;
    transaction_user_table_ptr = transaction->user_entries;
    transaction_object_table_ptr = transaction->object_entries;
    transaction_edge_table_ptr = transaction->edge_entries;
    transaction_global_info_ptr = transaction->global_info_entries;

    transaction->state = DATABASE_TRANSACTION_STATE_COMMIT;

    status = fbe_database_system_db_start_persist_transaction(&system_db_persist_handle);
    if (status != FBE_STATUS_OK) {
        fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: start system db persist transaction, status:0x%x\n",
            __FUNCTION__, status);
        return status;
    }

    status = fbe_database_get_last_system_object_id(&last_system_object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s :failed to get last system obj id status: 0x%x\n",
            __FUNCTION__, status);
        return status;
    }

    /* open a persist service transaction get a handle back */
    status = fbe_database_persist_start_transaction(&persist_handle);
    if (status != FBE_STATUS_OK)
    {
        fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s :start persist transaction failed, status: 0x%x\n",
            __FUNCTION__, status);

        /*abort the system db persist transaction*/
        fbe_database_system_db_persist_abort(system_db_persist_handle);

        return status;
    }


    /* go thru the transaction tables and persist each entry */ 

    /* update the service global info table first*/
    for (index = 0; index < DATABASE_TRANSACTION_MAX_GLOBAL_INFO; index++)
    {
        switch(transaction_global_info_ptr[index].header.state){
        case DATABASE_CONFIG_ENTRY_MODIFY:
            transaction_global_info_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            fbe_spinlock_lock(&database_service->global_info.table_lock);

            /* Get the persit entry id from the in-memory service table */
            fbe_database_config_table_get_global_info_entry(&database_service->global_info, 
                                                            transaction_global_info_ptr[index].type, 
                                                            &service_global_info_ptr);

            fbe_spinlock_unlock(&database_service->global_info.table_lock);

             /* The check for entry-id is required, coz, if it is an INVALID-id, it means,
             * there is no valid DATA in the persistence DB. So do a write straight-away.
             * If not, then we need to find the original place to modify or overwrite.  
             */
            if(service_global_info_ptr->header.entry_id == 0)
            {
                /* Send to persist */
				status = fbe_database_persist_write_entry(persist_handle,
														  FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA,
														  (fbe_u8_t *)&transaction_global_info_ptr[index],
														  sizeof(transaction_global_info_ptr[index]),
														  &transaction_global_info_ptr[index].header.entry_id);
				if (status != FBE_STATUS_OK )
				{
					database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: Persist failed to write entry, status: 0x%x\n",
						__FUNCTION__, status);	
				}

				database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: Write persist entry 0x%llx completed for Global Info type 0x%x!\n",
							   __FUNCTION__, (unsigned long long)transaction_global_info_ptr[index].header.entry_id,
							   transaction_global_info_ptr[index].type);	
				
            }
            else
            {
                /* Send to persist */
				status = fbe_database_persist_modify_entry(persist_handle,
														   (fbe_u8_t *)&transaction_global_info_ptr[index],
														   sizeof(transaction_global_info_ptr[index]),
														   service_global_info_ptr->header.entry_id);
				if (status != FBE_STATUS_OK )
				{
					database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: Persist failed to modify entry 0x%llx, status: 0x%x\n",
						__FUNCTION__, (unsigned long long)service_global_info_ptr->header.entry_id, status);	
				}
			

				database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
						   "%s: Modifying persist entry 0x%llx completed for Global Info!\n",
						   __FUNCTION__, (unsigned long long)transaction_global_info_ptr[index].header.entry_id);	
				
            }
            /* We are doing something different here as compared to other table entries. 
             * For Global Info, we first do a persist write/Modify so that we will get a
             * valid entry-id. Then issue an update. If we do things the same old way,
             * we may end up in writing invalid entry-id to the peristence DB.
             */
            fbe_spinlock_lock(&database_service->global_info.table_lock);

            /* Update the global_info entry */
            fbe_database_config_table_update_global_info_entry(&database_service->global_info, 
                                                               &transaction_global_info_ptr[index]);

            fbe_spinlock_unlock(&database_service->global_info.table_lock);
            break;
            
          case DATABASE_CONFIG_ENTRY_DESTROY:
            /* send to persist */
            status = fbe_database_persist_delete_entry(persist_handle, transaction_global_info_ptr[index].header.entry_id);
            if (status != FBE_STATUS_OK )
            {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: Persist failed to delete entry 0x%x, status: 0x%x\n", 
                               __FUNCTION__, (unsigned int)transaction_global_info_ptr[index].header.entry_id, status); 
            }

            database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Deleted persist entry 0x%x\n",
                           __FUNCTION__, 
                           (unsigned int)transaction_global_info_ptr[index].header.entry_id);   

            fbe_spinlock_lock(&database_service->global_info.table_lock);

            /* update the in-memory service table */
            fbe_database_config_table_remove_global_info_entry(&database_service->global_info,
                                                               &transaction_global_info_ptr[index]);

            fbe_spinlock_unlock(&database_service->global_info.table_lock);

            break;

          case DATABASE_CONFIG_ENTRY_INVALID:
          case DATABASE_CONFIG_ENTRY_VALID:
            break;
          case DATABASE_CONFIG_ENTRY_CREATE:
          default:
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Trans. Global Info Table: index %d, Unexpected state:%d\n",
                           __FUNCTION__, (int)index, transaction_global_info_ptr[index].header.state);
            break;
        }

    }

    /* update the service user table */
    for (index = 0; index < DATABASE_TRANSACTION_MAX_USER_ENTRY ; index++)
    {
        switch(transaction_user_table_ptr[index].header.state)
        {
        case DATABASE_CONFIG_ENTRY_CREATE:
            transaction_user_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            /*check if this is a system object entry and persistent the system user entry*/
            if (transaction_user_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_user_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE,
                    &transaction_user_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: System db persist failed to write entry, status: 0x%x\n",
                        __FUNCTION__, status);   
                }
            } else {
                status = fbe_database_persist_write_entry(persist_handle,
                                                          FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION,
                                                          (fbe_u8_t *)&transaction_user_table_ptr[index],
                                                          sizeof(transaction_user_table_ptr[index]),
                                                          &transaction_user_table_ptr[index].header.entry_id);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                        __FUNCTION__, (unsigned int)transaction_user_table_ptr[index].header.entry_id, status);    
                }

                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: obj 0x%x written to persist entry 0x%llx completed!\n",
                    __FUNCTION__,
                    transaction_user_table_ptr[index].header.object_id,
                    transaction_user_table_ptr[index].header.entry_id);
            }

            /* update the in-memory service table */
            fbe_database_config_table_update_user_entry(&database_service->user_table, &transaction_user_table_ptr[index]);
          

            break;
        case DATABASE_CONFIG_ENTRY_MODIFY:
            transaction_user_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            /*check if this is a system object entry and persistent the system user entry*/
            if (transaction_user_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_user_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE,
                    &transaction_user_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: System db persist failed to write entry, status: 0x%x\n",
                        __FUNCTION__, status);   
                }
            }
            else 
            {
                status = fbe_database_persist_modify_entry(persist_handle, 
                    (fbe_u8_t *)&transaction_user_table_ptr[index], 
                    sizeof(transaction_user_table_ptr[index]), 
                    transaction_user_table_ptr[index].header.entry_id);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                        __FUNCTION__, (unsigned int)transaction_user_table_ptr[index].header.entry_id, status);  
                }
            }

            fbe_database_config_table_update_user_entry(&database_service->user_table, &transaction_user_table_ptr[index]);

            break;

        case DATABASE_CONFIG_ENTRY_DESTROY:

            /*check if this is a system object entry and persistent the system user entry*/
            if (transaction_user_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_user_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE,
                    &transaction_user_table_ptr[index]);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: System db persist failed to delete entry, status: 0x%x\n",
                        __FUNCTION__, status);   
                }
            } else {
                /* send to persist */
                status = fbe_database_persist_delete_entry(persist_handle, transaction_user_table_ptr[index].header.entry_id);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Deleted persist entry 0x%x, status: 0x%x\n",
                        __FUNCTION__, (unsigned int)transaction_user_table_ptr[index].header.entry_id, status);  
                }
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: obj 0x%x destroy persist entry 0x%llx completed!\n",
                               __FUNCTION__,
                               transaction_user_table_ptr[index].header.object_id,
                               transaction_user_table_ptr[index].header.entry_id);
            }

            /* update the in-memory service table */			
            fbe_database_config_table_remove_user_entry(&database_service->user_table, &transaction_user_table_ptr[index]);

            break;
        case DATABASE_CONFIG_ENTRY_INVALID:  /* skip invalid entry*/
            break;
        case DATABASE_CONFIG_ENTRY_VALID:
        default:
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Trans. User Table: index %d, Unexpected state:%d\n",
                __FUNCTION__, (int)index, transaction_user_table_ptr[index].header.state);
            break;
        }
    }

    /* update the service edge table */
    for (index = 0; index < DATABASE_TRANSACTION_MAX_EDGES; index++)
    {
        switch(transaction_edge_table_ptr[index].header.state){
        case DATABASE_CONFIG_ENTRY_CREATE:
            transaction_edge_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
              /*check if this is a system object entry and persistent the system edge entry*/
              if (transaction_edge_table_ptr[index].header.object_id <= last_system_object_id) {
                  status = fbe_database_system_db_persist_edge_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE,
                      &transaction_edge_table_ptr[index]);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: System db persist failed to write entry, status: 0x%x\n",
                          __FUNCTION__, status);   
                  }
              } else {

                  status = fbe_database_persist_write_entry(persist_handle,
                                                            FBE_PERSIST_SECTOR_TYPE_SEP_EDGES,
                                                            (fbe_u8_t *)&transaction_edge_table_ptr[index],
                                                            sizeof(transaction_edge_table_ptr[index]),
                                                            &transaction_edge_table_ptr[index].header.entry_id);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                          __FUNCTION__, (unsigned int)transaction_edge_table_ptr[index].header.entry_id, status);    
                  }

                  database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: edge 0x%x-0x%x written to persist entry 0x%llx completed!\n",
                      __FUNCTION__, 
                      transaction_edge_table_ptr[index].header.object_id,
                      transaction_edge_table_ptr[index].client_index,
                      transaction_edge_table_ptr[index].header.entry_id);  

              }

              /* update the in-memory service table */
              fbe_database_config_table_update_edge_entry(&database_service->edge_table, &transaction_edge_table_ptr[index]);

              break;
          case DATABASE_CONFIG_ENTRY_MODIFY:
              transaction_edge_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
              if (transaction_edge_table_ptr[index].header.object_id <= last_system_object_id) {
                  status = fbe_database_system_db_persist_edge_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE,
                      &transaction_edge_table_ptr[index]);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: System db persist failed to write entry, status: 0x%x\n",
                          __FUNCTION__, status);   
                  }
              } else {
                  /* send to persist */
                  status = fbe_database_persist_modify_entry(persist_handle,
                      (fbe_u8_t *)&transaction_edge_table_ptr[index],
                      sizeof(transaction_edge_table_ptr[index]),
                      transaction_edge_table_ptr[index].header.entry_id);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                          __FUNCTION__, (unsigned int)transaction_edge_table_ptr[index].header.entry_id, status); 
                  }

              }

              /* update the in-memory service table */
              fbe_database_config_table_update_edge_entry(&database_service->edge_table, &transaction_edge_table_ptr[index]);

              break;
          case DATABASE_CONFIG_ENTRY_DESTROY:

              /*check if this is a system object entry and persistent the system edge entry*/
              if (transaction_edge_table_ptr[index].header.object_id <= last_system_object_id) {
                  status = fbe_database_system_db_persist_edge_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE,
                      &transaction_edge_table_ptr[index]);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: System db persist failed to write entry, status: 0x%x\n",
                          __FUNCTION__, status);   
                  }
              } else {
                  /* send to persist */
                  status = fbe_database_persist_delete_entry(persist_handle, transaction_edge_table_ptr[index].header.entry_id);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Persist failed to delete entry 0x%x, status: 0x%x\n",
                          __FUNCTION__, (unsigned int)transaction_edge_table_ptr[index].header.entry_id, status); 
                  }
                  database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: edge 0x%x-0x%x destroy persist entry 0x%llx completed!\n",
                      __FUNCTION__, 
                      transaction_edge_table_ptr[index].header.object_id,
                      transaction_edge_table_ptr[index].client_index,
                      transaction_edge_table_ptr[index].header.entry_id);  
              }

              /* update the in-memory service table */			
              fbe_database_config_table_remove_edge_entry(&database_service->edge_table, &transaction_edge_table_ptr[index]);
              break;
          case DATABASE_CONFIG_ENTRY_INVALID:  /* skip invalid entry*/
              break;
          case DATABASE_CONFIG_ENTRY_VALID:
          default:
              database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                  "%s: Trans. Edge Table: index %d, Unexpected state:%d\n",
                  __FUNCTION__, (int)index, transaction_edge_table_ptr[index].header.state);
              break;
        }
        /* zero the memory, so no leftover data on the buffer */
        //fbe_zero_memory(&transaction_edge_table_ptr[index], sizeof(database_edge_entry_t));
    }

    /* update the service object table */
    for (index = 0; index < DATABASE_TRANSACTION_MAX_OBJECTS; index++)
    {
        switch(transaction_object_table_ptr[index].header.state){
        case DATABASE_CONFIG_ENTRY_CREATE:
            transaction_object_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
            /*check if this is a system object entry and persistent the system object entry*/
              if (transaction_object_table_ptr[index].header.object_id <= last_system_object_id) {
                  status = fbe_database_system_db_persist_object_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE,
                      &transaction_object_table_ptr[index]);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: System db persist failed to write entry, status: 0x%x\n",
                          __FUNCTION__, status);   
                  }
              } else {
                  status = fbe_database_persist_write_entry(persist_handle,
                                                            FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS,
                                                            (fbe_u8_t *)&transaction_object_table_ptr[index],
                                                            sizeof(transaction_object_table_ptr[index]),
                                                            &transaction_object_table_ptr[index].header.entry_id);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                          __FUNCTION__, (unsigned int)transaction_object_table_ptr[index].header.entry_id, status);    
                  } 

                  database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: object 0x%x written to persist entry 0x%llx completed!\n",
                      __FUNCTION__, 
                      transaction_object_table_ptr[index].header.object_id,
                      transaction_object_table_ptr[index].header.entry_id);
              }            

              /* update the in-memory service table */            
              fbe_database_config_table_update_object_entry(&database_service->object_table, 
                                                            &transaction_object_table_ptr[index]);


              break;
        case DATABASE_CONFIG_ENTRY_MODIFY:
              transaction_object_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;
              /*check if this is a system object entry and persistent the system object entry*/
              if (transaction_object_table_ptr[index].header.object_id <= last_system_object_id) {
                  status = fbe_database_system_db_persist_object_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE,
                      &transaction_object_table_ptr[index]);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: System db persist failed to write entry, status: 0x%x\n",
                          __FUNCTION__, status);   
                  }
              } else {
                  /* send to persist */
                  status = fbe_database_persist_modify_entry(persist_handle,
                      (fbe_u8_t *)&transaction_object_table_ptr[index],
                      sizeof(transaction_object_table_ptr[index]),
                      transaction_object_table_ptr[index].header.entry_id);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                          __FUNCTION__, (unsigned int)transaction_object_table_ptr[index].header.entry_id, status);   
                  }
              }

              /* update the in-memory service table */
              fbe_database_config_table_update_object_entry(&database_service->object_table, 
                                                            &transaction_object_table_ptr[index]);
              break;
          case DATABASE_CONFIG_ENTRY_DESTROY:

              /*check if this is a system object entry and persistent the system object entry*/
              if (transaction_object_table_ptr[index].header.object_id <= last_system_object_id) {
                  status = fbe_database_system_db_persist_object_entry(system_db_persist_handle, FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE,
                      &transaction_object_table_ptr[index]);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: System db persist failed to write entry, status: 0x%x\n",
                          __FUNCTION__, status);   
                  }
              } else {
                  /* send to persist */                     
                  status = fbe_database_persist_delete_entry(persist_handle, transaction_object_table_ptr[index].header.entry_id);
                  if (status != FBE_STATUS_OK )
                  {
                      database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Persist failed to delete entry 0x%x, status: 0x%x\n",
                          __FUNCTION__, (unsigned int)transaction_object_table_ptr[index].header.entry_id, status);   
                  }
                  database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: object 0x%x destroy persist entry 0x%llx completed!\n",
                      __FUNCTION__, 
                      transaction_object_table_ptr[index].header.object_id,
                      transaction_object_table_ptr[index].header.entry_id);
              }

              /* update the in-memory service table */				
              fbe_database_config_table_remove_object_entry(&database_service->object_table, 
                                                            &transaction_object_table_ptr[index]);

              break;
          case DATABASE_CONFIG_ENTRY_INVALID:  /* skip invalid entry*/
              break;
          case DATABASE_CONFIG_ENTRY_VALID:
          default:
              database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                  "%s: Trans. Object Table: index %d, Unexpected state:%d\n",
                  __FUNCTION__, (int)index, transaction_object_table_ptr[index].header.state);
              break;
        }
    }


    if (database_common_cmi_is_active()) {

        status = fbe_database_persist_commit_transaction(persist_handle, 
            database_transaction_commit_completion_function,
            &persist_transaction_commit_context);
        /* TODO:  need to revisit how the asynch is handled. */
        wait_status = fbe_semaphore_wait(&persist_transaction_commit_context.commit_semaphore, NULL);
        if (wait_status == FBE_STATUS_TIMEOUT || (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
            || persist_transaction_commit_context.commit_status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: commit persist tran fail. wait_status:%d send_req_status:%d commit_status:%d\n",
                __FUNCTION__, wait_status, status, persist_transaction_commit_context.commit_status);

            fbe_database_transaction_abort(database_service);
            /*abort the system db persist transaction*/
            fbe_database_system_db_persist_abort(system_db_persist_handle);

            fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);

            return FBE_STATUS_GENERIC_FAILURE;

        } else {
            wait_status = fbe_database_system_db_persist_commit(system_db_persist_handle);
            if (wait_status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: system db persist transaction commit failed \n", __FUNCTION__);
            }
        }         


        fbe_zero_memory(database_service->transaction_ptr, sizeof(database_transaction_t));/*clean after ourselvs no matter what*/
        transaction->state = DATABASE_TRANSACTION_STATE_INACTIVE;
    }

    fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);

    if (wait_status == FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Persist transaction committed successfully!\n",
            __FUNCTION__);
    }

    return wait_status;
}


/*!***************************************************************
 * @fn fbe_database_config_commit_global_info
 *****************************************************************
 * @brief
 *   This function commit global info table 
 *
 * @param in_table_ptr - pointer to the database table
 *
 * @return fbe_status_t 
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_config_commit_global_info(database_table_t *in_table_ptr)
{
    fbe_u32_t retry_count, index = 0;
    database_table_header_t * header_ptr = NULL;
    database_global_info_entry_t *global_info_ptr = NULL;
    fbe_persist_transaction_handle_t persist_handle; 
    database_transaction_commit_context_t    persist_transaction_context;
    fbe_status_t status, wait_status;
    fbe_persist_entry_id_t new_entry_id = 0;

    global_info_ptr = in_table_ptr->table_content.global_info_ptr;

    if ((in_table_ptr->table_type != DATABASE_CONFIG_TABLE_GLOBAL_INFO) ||
        (global_info_ptr == NULL)){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (database_common_cmi_is_active() == FBE_FALSE)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: only active side can do commit!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* open a persist service transaction get a handle back */
    retry_count = 0;
    do{
        status =  fbe_database_persist_start_transaction(&persist_handle);
        database_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_INFO, 
                        "DB start persist transaction, status:0x%x, retry: %d\n",
                        status, (int)retry_count);
        if (status == FBE_STATUS_BUSY)
        {
            /* Wait 3 second */
            fbe_thread_delay(FBE_START_TRANSACTION_RETRY_INTERVAL);
            retry_count++;
        }
    }while((status == FBE_STATUS_BUSY) && (retry_count < FBE_START_TRANSACTION_RETRIES));

    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s :start persist transaction failed, status: 0x%x\n",
                        __FUNCTION__, status);
        return status;
    }

    /* can't hold spinlock to do all these work */
    /* don't have to check the last entry */
    for(index = 0; index < in_table_ptr->table_size - 1; index++)
    {
        header_ptr = (database_table_header_t *)global_info_ptr;
        if(header_ptr->state == DATABASE_CONFIG_ENTRY_VALID)
        {
            if(header_ptr->entry_id == 0)
            {
                new_entry_id = 0;
                status = fbe_database_persist_write_entry(persist_handle,
                                                          FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA,
                                                          (fbe_u8_t *)global_info_ptr,
                                                          sizeof(database_global_info_entry_t),
                                                          &new_entry_id);
                header_ptr->entry_id = new_entry_id;
            }
            else
            {
			    status = fbe_database_persist_modify_entry(persist_handle,
													       (fbe_u8_t *)global_info_ptr,
													       header_ptr->version_header.size,
													       header_ptr->entry_id);
            }
			if (status != FBE_STATUS_OK )
			{
				database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
					"%s: Persist failed to modify entry 0x%llx, status: 0x%x\n",
					__FUNCTION__, (unsigned long long)header_ptr->entry_id, status);	
			}
		
        }
        global_info_ptr++;
    }

    fbe_semaphore_init(&persist_transaction_context.commit_semaphore, 0, 1);

    status = fbe_database_persist_commit_transaction(persist_handle, 
                                                     database_transaction_commit_completion_function,
                                                     &persist_transaction_context);

    wait_status = fbe_semaphore_wait_ms(&persist_transaction_context.commit_semaphore, 60000);
    if (wait_status == FBE_STATUS_TIMEOUT || (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
        || persist_transaction_context.commit_status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: persist failed. wait_status:%d send_req_status:%d commit_status:%d\n",
            __FUNCTION__, wait_status, status, persist_transaction_context.commit_status);

        
        status = FBE_STATUS_GENERIC_FAILURE;
        
    }
    fbe_semaphore_destroy(&persist_transaction_context.commit_semaphore);

    return status;
}


/*!***************************************************************
 * @fn fbe_database_config_commit_object_table
 *****************************************************************
 * @brief
 *   This function commit object table
 *
 * @param in_table_ptr - pointer to the database table
 *
 * @return fbe_bool_t 
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_config_commit_object_table(database_table_t *in_table_ptr)
{
    database_object_entry_t *   object_entry_ptr = NULL;
    fbe_u32_t retry_count, index = 0;
    fbe_persist_transaction_handle_t persist_handle; 
    database_transaction_commit_context_t    persist_transaction_context;
    fbe_database_system_db_persist_handle_t system_db_persist_handle;
    fbe_object_id_t last_system_object_id;
    fbe_u32_t object_index = 0;
    database_table_header_t * header_ptr = NULL;
    fbe_status_t status, wait_status;
    fbe_bool_t transaction_started = FBE_FALSE;

    object_entry_ptr = in_table_ptr->table_content.object_entry_ptr;

    if ((in_table_ptr->table_type != DATABASE_CONFIG_TABLE_OBJECT) ||
        (object_entry_ptr == NULL)){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (database_common_cmi_is_active() == FBE_FALSE)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: only active side can do commit!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_get_last_system_object_id(&last_system_object_id);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s :failed to get last system obj id status: 0x%x\n",
            __FUNCTION__, status);
        return status;
    }

    /* take care of system objects first*/
    object_index =  0;
    do 
    {
        status = fbe_database_system_db_start_persist_transaction(&system_db_persist_handle);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: start system db persist transaction, status:0x%x\n",
                           __FUNCTION__, status);
            return status;
        }


        for(index = 0; 
            (object_index <= last_system_object_id) && (index < MAX_DATABASE_SYSTEM_DB_PERSIST_ENTRY_NUM); 
            object_index ++)
        {
            header_ptr = (database_table_header_t *)object_entry_ptr;
            if(header_ptr->state == DATABASE_CONFIG_ENTRY_VALID)
            {
                index ++;
                status = fbe_database_system_db_persist_object_entry(system_db_persist_handle, 
                                                                    FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE,
                                                                    object_entry_ptr);
                if (status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry %d, status: 0x%x\n",
							   __FUNCTION__, object_index, status);	

                    fbe_database_system_db_persist_abort(system_db_persist_handle);
                    return status;
			    }
            }
            object_entry_ptr++;
        }

        /* if we have anything need to persist */
        if (index != 0)
        {
            wait_status = fbe_database_system_db_persist_commit(system_db_persist_handle);
            if (wait_status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: system db persist transaction commit failed \n", __FUNCTION__);
                fbe_database_system_db_persist_abort(system_db_persist_handle);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }  /* until all object table entry processed */
    while (object_index <= last_system_object_id);

    fbe_semaphore_init(&persist_transaction_context.commit_semaphore, 0, 1);

    /* user objects now, 
     * object_index and object_entry_ptr are carried over from previous iteration
     * which guaranteed they are beyond system objects area
     */
    do 
    {

        for(index = 0; 
            (object_index < in_table_ptr->table_size) && (index < FBE_PERSIST_TRAN_ENTRY_MAX); 
            object_index ++)
        {
            header_ptr = (database_table_header_t *)object_entry_ptr;
            if(header_ptr->state == DATABASE_CONFIG_ENTRY_VALID)
            {
				if (!transaction_started)
				{
					/* open a persist service transaction get a handle back */
					retry_count = 0;
					do
					{
						status =  fbe_database_persist_start_transaction(&persist_handle);
						database_trace(FBE_TRACE_LEVEL_INFO, 
										FBE_TRACE_MESSAGE_ID_INFO, 
										"DB start persist transaction, status:0x%x, retry: %d\n",
										status, (int)retry_count);
						if (status == FBE_STATUS_BUSY)
						{
							/* Wait 3 second */
							fbe_thread_delay(FBE_START_TRANSACTION_RETRY_INTERVAL);
							retry_count++;
						}
					}while((status == FBE_STATUS_BUSY) && (retry_count < FBE_START_TRANSACTION_RETRIES));

					if (status != FBE_STATUS_OK)
					{
						database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
										"%s :start persist transaction failed, status: 0x%x\n",
										__FUNCTION__, status);
						fbe_semaphore_destroy(&persist_transaction_context.commit_semaphore);
						return status;
					}

					transaction_started = FBE_TRUE;
				}

                index ++;
			    status = fbe_database_persist_modify_entry(persist_handle,
													       (fbe_u8_t *)object_entry_ptr,
													       header_ptr->version_header.size,
													       header_ptr->entry_id);
			    if (status != FBE_STATUS_OK )
			    {
				    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
					    "%s: Persist failed to modify object entry 0x%llx, status: 0x%x\n",
					    __FUNCTION__, (unsigned long long)header_ptr->entry_id, status);	
                    fbe_semaphore_destroy(&persist_transaction_context.commit_semaphore);
                    return status;
			    }
            }
            object_entry_ptr++;
        }

        /* if we have anything need to persist */
        if (index != 0)
        {
            status = fbe_database_persist_commit_transaction(persist_handle, 
                                                             database_transaction_commit_completion_function,
                                                             &persist_transaction_context);

            wait_status = fbe_semaphore_wait_ms(&persist_transaction_context.commit_semaphore, 60000);
            if (wait_status == FBE_STATUS_TIMEOUT || (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
                || persist_transaction_context.commit_status != FBE_STATUS_OK)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: persist failed. wait_status:%d send_req_status:%d commit_status:%d\n",
                    __FUNCTION__, wait_status, status, persist_transaction_context.commit_status);

                fbe_semaphore_destroy(&persist_transaction_context.commit_semaphore);                
                return FBE_STATUS_GENERIC_FAILURE;
            }

			transaction_started = FBE_FALSE;
        }
    }  /* until all object table entry processed */
    while (object_index < in_table_ptr->table_size);

    fbe_semaphore_destroy(&persist_transaction_context.commit_semaphore);


    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_transaction_rollback(fbe_database_service_t *database_service)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       size;
    database_transaction_t         *transaction = NULL;
    database_user_entry_t          *transaction_user_table_ptr = NULL;
    database_object_entry_t        *transaction_object_table_ptr = NULL;
    database_edge_entry_t          *transaction_edge_table_ptr = NULL;
    database_global_info_entry_t   *transaction_global_info_ptr = NULL;
    database_object_entry_t        *existing_object_entry=NULL;
    database_object_entry_t         temp_existing_object_entry;
    database_edge_entry_t          *existing_edge_entry=NULL;
    database_user_entry_t          *existing_user_entry=NULL;
    database_global_info_entry_t   *existing_global_info_entry = NULL; 
    fbe_persist_transaction_handle_t persist_handle; 
    fbe_status_t			        wait_status = FBE_STATUS_OK;
    fbe_u32_t                       retry_count=0;
    fbe_s32_t                       index;
    fbe_object_id_t                 last_system_object_id;
    fbe_database_system_db_persist_handle_t system_db_persist_handle;
    fbe_bool_t                      is_active_sp = database_common_cmi_is_active();
    fbe_bool_t                      b_is_entry_in_use = FBE_FALSE;

    fbe_semaphore_init(&persist_transaction_commit_context.commit_semaphore, 0, 1);

    transaction = database_service->transaction_ptr;
    transaction_user_table_ptr = transaction->user_entries;
    transaction_object_table_ptr = transaction->object_entries;
    transaction_edge_table_ptr = transaction->edge_entries;
    transaction_global_info_ptr = transaction->global_info_entries;

    if (is_peer_update_allowed(database_service)) {
        status = fbe_database_transaction_operation_on_peer(FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT, database_service);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Transaction 0x%llx can't start on the peer but it's present, no config change allowed\n",
                __FUNCTION__, (unsigned long long)transaction->transaction_id);
    
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Can not rollback if the transaction is not in active state or committing */
    if ((transaction->state != DATABASE_TRANSACTION_STATE_ACTIVE) &&
        (transaction->state != DATABASE_TRANSACTION_STATE_COMMIT)    ) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Transaction 0x%llx is in %d state, can't rollback transaction.\n",
            __FUNCTION__, (unsigned long long)transaction->transaction_id,
	    transaction->state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*check hook*/
    database_check_and_run_hook(NULL,FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_ROLLBACK_TRANSACTION, NULL);
    database_check_and_run_hook(NULL,FBE_DATABASE_HOOK_TYPE_PANIC_IN_UPDATE_ROLLBACK_TRANSACTION, NULL);

    transaction->state = DATABASE_TRANSACTION_STATE_ROLLBACK;

    /* start commit */
    status = fbe_database_system_db_start_persist_transaction(&system_db_persist_handle);
    if (status != FBE_STATUS_OK) {
		fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: start system db persist transaction, status:0x%x\n",
                       __FUNCTION__, status);
        return status;
    }

    status = fbe_database_get_last_system_object_id(&last_system_object_id);
    if (status != FBE_STATUS_OK) {
		fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "%s :failed to get last system obj id status: 0x%x\n",
            __FUNCTION__, status);
        return status;
    }

    /* open a persist service transaction get a handle back */
    do {
        status =  fbe_database_persist_start_transaction(&persist_handle);
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                        FBE_TRACE_MESSAGE_ID_INFO, 
                        "DB transaction rollback:start persist transaction, status:%d, retry: %d\n",
                        status, (int)retry_count);
        if (status == FBE_STATUS_BUSY) {
            /* Wait 3 second */
            fbe_thread_delay(FBE_START_TRANSACTION_RETRY_INTERVAL);
            retry_count++;
        }
    }while((status == FBE_STATUS_BUSY) && (retry_count < FBE_START_TRANSACTION_RETRIES));
    if (status != FBE_STATUS_OK) {
        fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s :start persist transaction failed, status: 0x%x\n",
                        __FUNCTION__, status);

        /*abort the system db persist transaction*/
        fbe_database_system_db_persist_abort(system_db_persist_handle);
        return status;
    }

    /* start rolling back */
    /* rollback should go the reverse order of what has been done, so start from the bottom of the table */
    /* for create, we destroy everything created so far*/
    /* for destory, we recreated what was destroyed base on the service tables */
    /* for modify, we change back to what was in the service tables */
    /* Always use the entry id from the transaction since that is the location it was persisted to*/
    /* rollback the object first */
    for (index = DATABASE_TRANSACTION_MAX_OBJECTS-1; index >= 0; index--) {

        switch (transaction_object_table_ptr[index].header.state) {
        case DATABASE_CONFIG_ENTRY_CREATE:
            /* If the entry_id is not validate we must populate it from the persist.
             */
            if (transaction_object_table_ptr[index].header.entry_id == 0) {
                status = fbe_database_populate_transaction_entry_ids_from_persist(DATABASE_CONFIG_TABLE_OBJECT,
                                                                                 (database_table_header_t *)transaction_object_table_ptr);
                if ((status != FBE_STATUS_OK)                                  ||
                    (transaction_object_table_ptr[index].header.entry_id == 0)    ) {
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: Transacation 0x%x Failed to get entry_id for obj: 0x%x.\n",
                                   __FUNCTION__, (unsigned int)transaction->transaction_id, 
                                   transaction_object_table_ptr[index].header.object_id);
                }
            } else {
                status = fbe_database_persist_validate_entry(persist_handle,
                                                             transaction_object_table_ptr[index].header.entry_id);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: Transacation 0x%x failed to validate entry: 0x%llx obj: 0x%x\n",
                                   __FUNCTION__, (unsigned int)transaction->transaction_id,
                                   transaction_object_table_ptr[index].header.entry_id,
                                   transaction_object_table_ptr[index].header.object_id);
                }
            }
            /*! @todo DE168 -  The check below is a temporary work-around so 
             *        so that we don't attempt to destroy an object that was
             *        never created.  The correct fix to not even attempt
             *        to allocate etc, unless the object can be created.
             */
            status = fbe_database_validate_object(transaction_object_table_ptr[index].header.object_id);
            if (status != FBE_STATUS_OK) {
                /*! @todo DE168 - Have found that the same object id is in the 
                 *        transaction table multiple times.  Since this is a
                 *        known issue the trace level is warning.
                 */
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: DE168 - Transaction 0x%llx index: %lld duplicate object id: 0x%x found.\n",
                               __FUNCTION__, (unsigned long long)transaction->transaction_id, (long long)index, 
                               transaction_object_table_ptr[index].header.object_id);
            } else {
                /* Else destroy the object.
                 */
                status = database_common_destroy_object_from_table_entry(&transaction_object_table_ptr[index]);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: Transacation 0x%x failed to destory object 0x%x.\n",
                                   __FUNCTION__, (unsigned int)transaction->transaction_id, 
                                   transaction_object_table_ptr[index].header.object_id);
                }
                status = database_common_wait_destroy_object_complete();
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: Transacation 0x%x Waiting for destory object failed 0x%x.\n",
                                   __FUNCTION__, (unsigned int)transaction->transaction_id, 
                                   transaction_object_table_ptr[index].header.object_id);
                }

                /*check if this is a system object entry and persistent the system object entry*/
                if (transaction_object_table_ptr[index].header.object_id <= last_system_object_id) {
                    status = fbe_database_system_db_persist_object_entry(system_db_persist_handle, 
                                                                         FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE,
                                                                         &transaction_object_table_ptr[index]);
                    if (status != FBE_STATUS_OK ) {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                    }
                } else {
                    /* send to persist */
                    status = fbe_database_persist_delete_entry(persist_handle, 
                                                               transaction_object_table_ptr[index].header.entry_id);
                    if (status != FBE_STATUS_OK ) {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: Persist failed to delete entry 0x%x, status: 0x%x\n",
                                       __FUNCTION__, (unsigned int)transaction_object_table_ptr[index].header.entry_id, status);	
                    }

                    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: object 0x%x destroy persist entry 0x%llx completed!\n",
                           __FUNCTION__, 
                           transaction_object_table_ptr[index].header.object_id,
                           (unsigned long long)transaction_object_table_ptr[index].header.entry_id);
                }
            }

            /* Last step (after removing object from persist) is to remove the
             * object from the in-memory object database.
             */
            fbe_database_config_table_remove_object_entry(&database_service->object_table, &transaction_object_table_ptr[index]);
            break;
        case DATABASE_CONFIG_ENTRY_MODIFY:			
            /* get the entry in the service table base on the object id*/
            fbe_spinlock_lock(&database_service->object_table.table_lock);
            status = fbe_database_config_table_get_object_entry_by_id(&database_service->object_table,
                                                                    transaction_object_table_ptr[index].header.object_id,
                                                                    &existing_object_entry);	
            // we may need to hold this lock longer
            fbe_spinlock_unlock(&database_service->object_table.table_lock);
            			
            /*! @note Update the object configuration but there is a case where
             *        the in-memory object does not have the update information.
             *        If needed use a temporary object and add the update
             *        information to the data from the in-memory entry.
             */
            if (fbe_database_transaction_abort_populate_update_from_transaction_if_needed(&temp_existing_object_entry,
                                                                                          &transaction_object_table_ptr[index],
                                                                                          existing_object_entry)) {
                /* Use the temporary updated object entry.
                 */
                status = database_common_update_config_from_table_entry(&temp_existing_object_entry);
            } else {
                status = database_common_update_config_from_table_entry(existing_object_entry);
            }

            /* Persist the current in-memory object data (i.e. revert the transaction).
             * The object size has already been updated in the in-memory object.
             */
            if (transaction_object_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_object_entry(system_db_persist_handle, 
                                                                     FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE,
                                                                     existing_object_entry);
                if (status != FBE_STATUS_OK ) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            } else {
                /* send to persist */
                status = fbe_database_persist_modify_entry(persist_handle,
                                                           (fbe_u8_t *)existing_object_entry,
                                                           sizeof(transaction_object_table_ptr[index]),
                                                           transaction_object_table_ptr[index].header.entry_id);
                if (status != FBE_STATUS_OK ) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                        __FUNCTION__, (unsigned int)transaction_object_table_ptr[index].header.object_id, status);	
                }
            }

            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: object 0x%x modify persist entry 0x%llx completed!\n",
                           __FUNCTION__, 
                           transaction_object_table_ptr[index].header.object_id,
                           (unsigned long long)transaction_object_table_ptr[index].header.entry_id);

            /* Commit the changes.
             */
            fbe_database_commit_object(existing_object_entry->header.object_id, existing_object_entry->db_class_id, FBE_FALSE);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Transacation 0x%x failed to update_config of object 0x%x.\n",
                __FUNCTION__, (unsigned int)transaction->transaction_id, transaction_object_table_ptr[index].header.object_id);
            }
            break;
        case DATABASE_CONFIG_ENTRY_DESTROY:
            /* Perform the following steps:
             *  1) Validate that there is no persist entry for this object.
             *  2) Add the object to the persist pending list.
             *  3) Update the in-memory object table.
             *  4) `Commit' the object to the class.
             */
            /* Validate that there is not persist entry for this object.
             */
            status = fbe_database_util_get_persist_entry_info((database_table_header_t *)&transaction_object_table_ptr[index], 
                                                              &b_is_entry_in_use);
            if ((status != FBE_STATUS_OK)       ||
                (b_is_entry_in_use == FBE_TRUE)    ) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: destroyed object 0x%x persist in-use: %d entry 0x%llx status: 0x%x\n",
                               __FUNCTION__, 
                               transaction_object_table_ptr[index].header.object_id,
                               b_is_entry_in_use,
                               (unsigned long long)transaction_object_table_ptr[index].header.entry_id,
                               status);
            } else {
                /* Else clear the entry id and set the state to valid.
                 */
                transaction_object_table_ptr[index].header.entry_id = 0;
                transaction_object_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;

                /* If the committed size is bigger , update it*/
                database_common_get_committed_object_entry_size(transaction_object_table_ptr[index].db_class_id, &size);
                if (size > transaction_object_table_ptr[index].header.version_header.size) {
                    transaction_object_table_ptr[index].header.version_header.size = size;
                }

                /* Write the entry to the persist pending list.
                 */
                if (transaction_object_table_ptr[index].header.object_id <= last_system_object_id) {
                    status = fbe_database_system_db_persist_object_entry(system_db_persist_handle, 
                                                                     FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE,
                                                                     &transaction_object_table_ptr[index]);
                    if (status != FBE_STATUS_OK ) {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: System db persist failed to write entry, status: 0x%x\n",
                                       __FUNCTION__, status);	
                    }
                } else {
                    status = fbe_database_persist_write_entry(persist_handle,
                                                              FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS,
                                                              (fbe_u8_t *)&transaction_object_table_ptr[index],
                                                              sizeof(transaction_object_table_ptr[index]),
                                                              &transaction_object_table_ptr[index].header.entry_id);
                    if (status != FBE_STATUS_OK ) {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Persist failed to write entry, status: 0x%x\n",
                            __FUNCTION__, status);	
                    }
                }
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: object 0x%x written to persist entry 0x%llx completed!\n",
                               __FUNCTION__, 
                               transaction_object_table_ptr[index].header.object_id,
                               (unsigned long long)transaction_object_table_ptr[index].header.entry_id);

                /* Re-create the in-memory object from the transaction entry.
                 */
                status = database_common_create_object_from_table_entry(&transaction_object_table_ptr[index]);
                if (status != FBE_STATUS_OK) {
                      database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: Failed to recreate object 0x%x\n",
                                     __FUNCTION__,transaction_object_table_ptr[index].header.object_id);           
                } else {            
                    status = database_common_set_config_from_table_entry(&transaction_object_table_ptr[index]);
                    if (status != FBE_STATUS_OK) {
                      database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: Failed to config object 0x%x\n",
                                     __FUNCTION__,transaction_object_table_ptr[index].header.object_id);          
                    }
                }

                /* update the in-memory service table */
                fbe_database_config_table_update_object_entry(&database_service->object_table, 
                                                              &transaction_object_table_ptr[index]);
                if (transaction_object_table_ptr[index].db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE) {
                    /*If it's a PVD then connect it to the LDO*/
                    status = fbe_database_add_pvd_to_be_connected(&database_service->pvd_operation, transaction_object_table_ptr[index].header.object_id);
                    if (status != FBE_STATUS_OK) {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: Failed to connect PVD to LDO! Status=0x%x\n",
                                       __FUNCTION__,status);			
                    }
                 }

                /* Commit the object to the class.
                 */
                fbe_database_commit_object(transaction_object_table_ptr[index].header.object_id, transaction_object_table_ptr[index].db_class_id, FBE_FALSE);
            }
            break;
        case DATABASE_CONFIG_ENTRY_VALID:
        case DATABASE_CONFIG_ENTRY_INVALID:
        default:
            break;
        }
    }

    /* TODO: may need to keep the sequence of object and edge */
    for (index = DATABASE_TRANSACTION_MAX_EDGES-1; index >= 0; index--) { 
        switch (transaction_edge_table_ptr[index].header.state){
        case DATABASE_CONFIG_ENTRY_CREATE:
            /*! @note Since we destroy the objects before destroying the edges
             *        (should be the reverse order: destroy edges then objects),
             *        we must confirm if the object is still there before
             *        attempting to destroy the edge.
             */
            /* Else destroy the edge. First get the valid entry id.
             */
            status = fbe_database_populate_transaction_entry_ids_from_persist(DATABASE_CONFIG_TABLE_EDGE,
                                                                              (database_table_header_t *)transaction_edge_table_ptr);
            if ((status != FBE_STATUS_OK)                                  ||
                (transaction_edge_table_ptr[index].header.entry_id == 0)    ) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Transacation 0x%x Failed to get entry_id for obj: 0x%x.\n",
                               __FUNCTION__, (unsigned int)transaction->transaction_id, 
                               transaction_edge_table_ptr[index].header.object_id);
            } else {
                status = fbe_database_persist_validate_entry(persist_handle,
                                                             transaction_edge_table_ptr[index].header.entry_id);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Transacation 0x%x failed to validate entry: 0x%llx obj: 0x%x\n",
                               __FUNCTION__, (unsigned int)transaction->transaction_id,
                               transaction_edge_table_ptr[index].header.entry_id,
                               transaction_edge_table_ptr[index].header.object_id);
                }
            }
            if (fbe_database_transaction_does_object_exist(transaction_edge_table_ptr[index].header.object_id)) {
                /* Since we are processing the entries in reverse order we will
                 * destroy any created edges before attempting to re-create the 
                 * original edge.
                 */
                status = database_common_destroy_edge_from_table_entry(&transaction_edge_table_ptr[index]);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Transaction 0x%llx failed to destroy edge object 0x%x, ind 0x%x.\n",
                                    __FUNCTION__,
                                   (unsigned long long)transaction->transaction_id, 
                                    transaction_edge_table_ptr[index].header.object_id, 
                                    transaction_edge_table_ptr[index].client_index);
                }

                /*! @note Validate that the edge is not in the in-memory table.
                 */
                status = fbe_database_config_table_get_edge_entry(&database_service->edge_table,
                                                                  transaction_edge_table_ptr[index].header.object_id,
                                                                  transaction_edge_table_ptr[index].client_index,
                                                                  &existing_edge_entry);
                if (status == FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: Transaction 0x%llx destroy edge object 0x%x, ind 0x%x unexpected edge found\n",
                                    __FUNCTION__,
                                   (unsigned long long)transaction->transaction_id, 
                                    transaction_edge_table_ptr[index].header.object_id, 
                                    transaction_edge_table_ptr[index].client_index);
                }
            } /* end if client object exist */

            /*check if this is a system object entry and persistent the system edge entry*/
            if (transaction_edge_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_edge_entry(system_db_persist_handle, 
                                                                   FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE,
                                                                   &transaction_edge_table_ptr[index]);
                if (status != FBE_STATUS_OK ) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
						   "%s: System db persist failed to write entry, status: 0x%x\n",
						   __FUNCTION__, status);	
                }
            } else {
                /* send to persist */
                status = fbe_database_persist_delete_entry(persist_handle, 
                                                           transaction_edge_table_ptr[index].header.entry_id);
                if (status != FBE_STATUS_OK ) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Persist failed to delete entry 0x%x, status: 0x%x\n",
                    __FUNCTION__, (unsigned int)transaction_edge_table_ptr[index].header.entry_id, status);	
                }

                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: edge 0x%x-0x%x destroy persist entry 0x%llx completed!\n",
                           __FUNCTION__, 
                           transaction_edge_table_ptr[index].header.object_id,
                           transaction_edge_table_ptr[index].client_index,
                           (unsigned long long)transaction_edge_table_ptr[index].header.entry_id);	
            }
            break;

        case DATABASE_CONFIG_ENTRY_DESTROY:
            /* Perform the following steps:
             *  1) Validate that there is no persist entry for this object.
             *  2) Add the object to the persist pending list.
             *  3) Update the in-memory object table.
             *  4) `Commit' the object to the class.
             */
            /* Validate that there is not persist entry for this object.
             */
            status = fbe_database_util_get_persist_entry_info((database_table_header_t *)&transaction_edge_table_ptr[index], 
                                                              &b_is_entry_in_use);
            if ((status != FBE_STATUS_OK)       ||
                (b_is_entry_in_use == FBE_TRUE)    ) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: destroyed edge 0x%x persist in-use: %d entry 0x%llx status: 0x%x\n",
                               __FUNCTION__, 
                               transaction_edge_table_ptr[index].header.object_id,
                               b_is_entry_in_use,
                               (unsigned long long)transaction_edge_table_ptr[index].header.entry_id,
                               status);
            } else {
                /* Else clear the entry id and set the state to valid.
                 */
                transaction_edge_table_ptr[index].header.entry_id = 0;
                transaction_edge_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;

                /* If the committed size is bigger , update it*/
                database_common_get_committed_edge_entry_size(&size);
                if (size > transaction_edge_table_ptr[index].header.version_header.size) {
                    transaction_edge_table_ptr[index].header.version_header.size = size;
                }
                	
                /* Write the entry to the the persist pending list.
                 */
                if (transaction_edge_table_ptr[index].header.object_id <= last_system_object_id) {
                    status = fbe_database_system_db_persist_edge_entry(system_db_persist_handle, 
                                                                       FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE,
                                                                       &transaction_edge_table_ptr[index]);
                    if (status != FBE_STATUS_OK ) {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: System db persist failed to write entry, status: 0x%x\n",
                                       __FUNCTION__, status);	
                    }
                } else {
                    status = fbe_database_persist_write_entry(persist_handle, 
                                                              FBE_PERSIST_SECTOR_TYPE_SEP_EDGES,
                                                              (fbe_u8_t *)&transaction_edge_table_ptr[index],
                                                              sizeof(transaction_edge_table_ptr[index]),
                                                              &transaction_edge_table_ptr[index].header.entry_id);
                    if (status != FBE_STATUS_OK ) {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Persist failed to write entry, status: 0x%x\n",
                            __FUNCTION__, status);	
                    }
                }
                
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: edge 0x%x-0x%x written to persist entry 0x%llx completed!\n",
                               __FUNCTION__, 
                               transaction_edge_table_ptr[index].header.object_id,
                               transaction_edge_table_ptr[index].client_index,
                               (unsigned long long)transaction_edge_table_ptr[index].header.entry_id);
                	
                /*we have already saved the edges in the transaction table. create edges frm the table*/
                status = database_common_create_edge_from_table_entry(&transaction_edge_table_ptr[index]);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                   "   %s: Transaction 0x%llx failed to create edge object 0x%x, ind 0x%x.\n",
                                   __FUNCTION__,
                                   (unsigned long long)transaction->transaction_id, 
                                   transaction_edge_table_ptr[index].header.object_id, 
                                   transaction_edge_table_ptr[index].client_index);
                }

                /* `Commit' the in-memory edge entry.
                 */
                fbe_database_config_table_update_edge_entry(&database_service->edge_table, &transaction_edge_table_ptr[index]);
            }
            break;

        case DATABASE_CONFIG_ENTRY_MODIFY: /* edge can not be modified */
        case DATABASE_CONFIG_ENTRY_VALID:
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Transaction 0x%llx: edge entry has unexpected state 0x%x.\n",
                __FUNCTION__, (unsigned long long)transaction->transaction_id,
		transaction_edge_table_ptr[index].header.state);
            break;
        case DATABASE_CONFIG_ENTRY_INVALID: /* skip the empty entry*/
        default:
            break;
        }
    }

    /*the user entries rollback only has effects on system spare entries*/
    for (index = DATABASE_TRANSACTION_MAX_USER_ENTRY - 1; index >= 0; index--)
    {
        switch(transaction_user_table_ptr[index].header.state)
        {
        case DATABASE_CONFIG_ENTRY_CREATE:
            /* Else destroy the user entry. First get the valid entry id.
             */
            status = fbe_database_populate_transaction_entry_ids_from_persist(DATABASE_CONFIG_TABLE_USER,
                                                                              (database_table_header_t *)transaction_user_table_ptr);
            if ((status != FBE_STATUS_OK)                                  ||
                (transaction_user_table_ptr[index].header.entry_id == 0)    ) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Transacation 0x%x Failed to get entry_id for obj: 0x%x.\n",
                               __FUNCTION__, (unsigned int)transaction->transaction_id, 
                               transaction_user_table_ptr[index].header.object_id);
            } else {
                status = fbe_database_persist_validate_entry(persist_handle,
                                                             transaction_user_table_ptr[index].header.entry_id);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Transacation 0x%x failed to validate entry: 0x%llx obj: 0x%x\n",
                               __FUNCTION__, (unsigned int)transaction->transaction_id,
                               transaction_user_table_ptr[index].header.entry_id,
                               transaction_user_table_ptr[index].header.object_id);
                }
            }

            /*check if this is a system object entry and persistent the system user entry*/
            if (transaction_user_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_user_entry(system_db_persist_handle, 
                                                                   FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE,
                                                                   &transaction_user_table_ptr[index]);
                if (status != FBE_STATUS_OK ) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to delete entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            } else {
                /* send to persist */
                status = fbe_database_persist_delete_entry(persist_handle, 
                                                           transaction_user_table_ptr[index].header.entry_id);
                if (status != FBE_STATUS_OK ) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: Deleted persist entry 0x%x, status: 0x%x\n",
							   __FUNCTION__, (unsigned int)transaction_user_table_ptr[index].header.entry_id, status);	
                }
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: obj 0x%x destroy persist entry 0x%llx completed!\n", 
                               __FUNCTION__, 
                               transaction_user_table_ptr[index].header.object_id, 
                               (unsigned long long)transaction_user_table_ptr[index].header.entry_id);	
            }

            /*we delete the corresponding entries in the database service table, for system drive
              spare clone hack*/			
            fbe_database_config_table_remove_user_entry(&database_service->user_table, &transaction_user_table_ptr[index]);
            break;
        case DATABASE_CONFIG_ENTRY_MODIFY:
            /* get the entry in the service table base on the object id*/
            fbe_spinlock_lock(&database_service->user_table.table_lock);
            status = fbe_database_config_table_get_user_entry_by_object_id(&database_service->user_table,
                                                                    transaction_user_table_ptr[index].header.object_id,
                                                                    &existing_user_entry);
            fbe_spinlock_unlock(&database_service->user_table.table_lock);

            /* The on-disk entry size is already correct.
             */

            /*check if this is a system object entry and persistent the system user entry*/
            if (transaction_user_table_ptr[index].header.object_id <= last_system_object_id) {
                status = fbe_database_system_db_persist_user_entry(system_db_persist_handle, 
                                                                   FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE,
                                                                   existing_user_entry);
                if (status != FBE_STATUS_OK ) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                }
            } else  {
                if (existing_user_entry != NULL) {
                    /* send to persist */
                    if (existing_user_entry->header.entry_id == 0) {
                        /* write entry and get a valid entry id*/
                        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: first time modify obj 0x%x user data, create any entry in persist!\n",
                                   __FUNCTION__, 
                                   existing_user_entry->header.object_id);	
                    
                        status = fbe_database_persist_write_entry(persist_handle, 
                                                                  FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION, 
                                                                  (fbe_u8_t *)existing_user_entry, 
                                                                  sizeof(transaction_user_table_ptr[index]), 
                                                                  &transaction_user_table_ptr[index].header.entry_id);
                        if (status != FBE_STATUS_OK ) {
                            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                           "%s: Persist failed to write entry, status: 0x%x\n", 
                                           __FUNCTION__, status);	
                        }
                        /* update the in-memory service table */
                        existing_user_entry->header.entry_id = transaction_user_table_ptr[index].header.entry_id;

                    } else {
                        status = fbe_database_persist_modify_entry(persist_handle, 
                                                                   (fbe_u8_t *)existing_user_entry, 
                                                                   sizeof(transaction_user_table_ptr[index]), 
                                                                   transaction_user_table_ptr[index].header.entry_id);
                        if (status != FBE_STATUS_OK ) {
                            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                           "%s: Persist failed to modify entry 0x%x, status: 0x%x\n",
                                           __FUNCTION__, (unsigned int)existing_user_entry->header.entry_id, status);	
                        }

                    }

                    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                                   "%s: obj 0x%x written to persist entry 0x%llx completed!\n", 
                                   __FUNCTION__, 
                                   transaction_user_table_ptr[index].header.object_id, 
                                   (unsigned long long)transaction_user_table_ptr[index].header.entry_id);	
                }
            }
            
            /* we need to send notification to admin shim*/		
            fbe_database_commit_object(transaction_user_table_ptr[index].header.object_id, transaction_user_table_ptr->db_class_id, FBE_FALSE);
            break;
        case DATABASE_CONFIG_ENTRY_DESTROY:
            /* Perform the following steps:
             *  1) Validate that there is no persist entry for this object.
             *  2) Add the object to the persist pending list.
             *  3) Update the in-memory object table.
             *  4) `Commit' the object to the class.
             */
            /* Validate that there is not persist entry for this object.
             */
            status = fbe_database_util_get_persist_entry_info((database_table_header_t *)&transaction_user_table_ptr[index], 
                                                              &b_is_entry_in_use);
            if ((status != FBE_STATUS_OK)       ||
                (b_is_entry_in_use == FBE_TRUE)    ) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: destroyed user 0x%x persist in-use: %d entry 0x%llx status: 0x%x\n",
                               __FUNCTION__, 
                               transaction_user_table_ptr[index].header.object_id,
                               b_is_entry_in_use,
                               (unsigned long long)transaction_user_table_ptr[index].header.entry_id,
                               status);
            } else {
                /* Else clear the entry id and set the state to valid.
                 */
                transaction_user_table_ptr[index].header.entry_id = 0;
                transaction_user_table_ptr[index].header.state = DATABASE_CONFIG_ENTRY_VALID;

                /* If the committed size is bigger , update it*/
                database_common_get_committed_user_entry_size(transaction_user_table_ptr[index].db_class_id, &size);
                if (size > transaction_user_table_ptr[index].header.version_header.size) {
                    transaction_user_table_ptr[index].header.version_header.size = size;
                }

                /* Write the entry to the persist pending list.
                 */
                if (transaction_user_table_ptr[index].header.object_id <= last_system_object_id) {
                    status = fbe_database_system_db_persist_user_entry(system_db_persist_handle, 
                                                                   FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE,
                                                                   &transaction_user_table_ptr[index]);
                    if (status != FBE_STATUS_OK ) {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: System db persist failed to write entry, status: 0x%x\n",
							   __FUNCTION__, status);	
                    }
                } else {
                    /*For create, we also need to judge whether entry id is initialized. This considers the following case:
                    *Active SP runs db transaction commit logic, where the newly created entries are assigned new entry id by persist service
                    *Active SP sends db transaction commit cmd to passive SP, where the new entry ids are copied to the entries on passive SP
                    *Before or in the middle of persist on active SP, it panics
                    *When passive SP becomes active SP, it should commit the incomplete transaction, where it should use the already allocated
                    *entry id to do the persist, rather than allocating new entry ids
                    *Once persist service removes entry bitmap and begin to use object id as the index of persist location, this can
                    *be removed*/
                    status = fbe_database_persist_write_entry(persist_handle, 
    													  FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION,
    													  (fbe_u8_t *)&transaction_user_table_ptr[index],
    													  sizeof(transaction_user_table_ptr[index]),
    													  &transaction_user_table_ptr[index].header.entry_id);
                    
                    if (status != FBE_STATUS_OK) {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
    							   "%s: Persist failed to write entry, status: 0x%x\n",
    							   __FUNCTION__, status);	
                    }
                    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: obj 0x%x written to persist entry 0x%llx completed!\n",
                               __FUNCTION__,
                               transaction_user_table_ptr[index].header.object_id,
                               transaction_user_table_ptr[index].header.entry_id);
                }

                /* `Commit' the user entry to the in-memory table.
                 */
                fbe_database_config_table_update_user_entry(&database_service->user_table, &transaction_user_table_ptr[index]);
            }
            break;
        default:
            break;
        }
    }

    for (index = DATABASE_TRANSACTION_MAX_GLOBAL_INFO-1; index >= 0; index--)
    {
        switch (transaction_global_info_ptr[index].header.state){
        case DATABASE_CONFIG_ENTRY_CREATE:
        case DATABASE_CONFIG_ENTRY_VALID:
        case DATABASE_CONFIG_ENTRY_DESTROY:

            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Transaction 0x%llx: Global Info entry has unexpected state 0x%x.\n",
                __FUNCTION__, (unsigned long long)transaction->transaction_id,
		transaction_global_info_ptr[index].header.state);
            break;  

        case DATABASE_CONFIG_ENTRY_MODIFY: 
             /* Get the Global Info entry from the service table */	  
            fbe_spinlock_lock(&database_service->global_info.table_lock);         
            status = fbe_database_config_table_get_global_info_entry(&database_service->global_info, 
                                                             transaction_global_info_ptr[index].type, 
                                                             &existing_global_info_entry);
            fbe_spinlock_unlock(&database_service->global_info.table_lock); 

            /* If the committed size is bigger , update it*/
            database_common_get_committed_global_info_entry_size(transaction_global_info_ptr[index].type, &size);
            if (size > transaction_global_info_ptr[index].header.version_header.size) {
                transaction_global_info_ptr[index].header.version_header.size = size;
            }

            /* The check for entry-id is required, coz, if it is an INVALID-id, it means,
             * there is no valid DATA in the persistence DB. So do a write straight-away.
             * If not, then we have some data to modify or overwrite.  
             */
            if(existing_global_info_entry->header.entry_id == 0) {
                /* Send to persist */
				status = fbe_database_persist_write_entry(persist_handle,
														  FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA,
														  (fbe_u8_t *)existing_global_info_entry,
														  sizeof(transaction_global_info_ptr[index]),
														  &transaction_global_info_ptr[index].header.entry_id);
				if (status != FBE_STATUS_OK) {
					database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: Persist failed to write entry, status: 0x%x\n",
						__FUNCTION__, status);	
				}

				database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
							   "%s: Write persist entry 0x%llx completed for Global Info type 0x%x!\n",
							   __FUNCTION__, (unsigned long long)transaction_global_info_ptr[index].header.entry_id,
							   transaction_global_info_ptr[index].type);	
				
            } else {
                /* Send to persist */
				status = fbe_database_persist_modify_entry(persist_handle,
														   (fbe_u8_t *)existing_global_info_entry,
														   sizeof(transaction_global_info_ptr[index]),
														   transaction_global_info_ptr[index].header.entry_id);
				if (status != FBE_STATUS_OK) {
					database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
						"%s: Persist failed to modify entry 0x%llx, status: 0x%x\n",
						__FUNCTION__, (unsigned long long)transaction_global_info_ptr[index].header.entry_id, status);	
				}

				database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
						   "%s: Modifying persist entry 0x%llx completed for Global Info!\n",
						   __FUNCTION__, (unsigned long long)transaction_global_info_ptr[index].header.entry_id);	
				
            }

            /* Update the in-memory */
            status = database_common_update_global_info_from_table_entry(existing_global_info_entry);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Transaction 0x%llx failed to update_global_info for type 0x%x.\n",
                               __FUNCTION__, (unsigned long long)transaction->transaction_id,
                               transaction_global_info_ptr[index].type);
            }
            break;
                                  
        case DATABASE_CONFIG_ENTRY_INVALID: /* skip the empty entry*/
        default:
            break;
        }
    }

    /*check hook*/
    database_check_and_run_hook(NULL,FBE_DATABASE_HOOK_TYPE_WAIT_BEFORE_ROLLBACK_TRANSACTION_PERSIST, NULL);
    database_check_and_run_hook(NULL,FBE_DATABASE_HOOK_TYPE_PANIC_BEFORE_ROLLBACK_TRANSACTION_PERSIST, NULL);
    
    if (is_active_sp) {
        /*
          two independent transactions need commit here, we expect them commit in an
          atomic way. The system db persist operates raw-3way-mirror, while the persist
          service operates the DB lun. They use different journals. We must work out
          a solution to handle the case where one transaction committed and the other failed 
          (system panic here).currently we plan to handle this kind of failure case in the 
          database service boot path.
        */
        status = fbe_database_persist_commit_transaction(persist_handle, 
                                                         database_transaction_commit_completion_function,
                                                         &persist_transaction_commit_context);
        /* TODO:  need to revisit how the asynch is handled. */
        wait_status = fbe_semaphore_wait(&persist_transaction_commit_context.commit_semaphore, NULL);
        if (wait_status == FBE_STATUS_TIMEOUT || (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
            || persist_transaction_commit_context.commit_status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: commit persist tran fail. wait_status:%d send_req_status:%d commit_status:%d\n",
                __FUNCTION__, wait_status, status, persist_transaction_commit_context.commit_status);

            fbe_database_transaction_abort(database_service);
            /*abort the system db persist transaction*/
            fbe_database_system_db_persist_abort(system_db_persist_handle);

            fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);
            
            return FBE_STATUS_GENERIC_FAILURE;
            
        } else {
            wait_status = fbe_database_system_db_persist_commit(system_db_persist_handle);
            if (wait_status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: system db persist transaction commit failed \n", __FUNCTION__);
            }
        }
        
        /* We have persisted and committed on the active SP.  Now commit
         * (i.e. update the in-memory entries) on the peer.
         */
        if(is_peer_update_allowed(database_service))
        {
            /* First commit (update the in-memory entries) and push to objects
             * the peer entries.
             */
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Persist transaction committed in memory, synching to peer...\n",
                           __FUNCTION__);

            status = fbe_database_transaction_operation_on_peer(FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT,
                                                                database_service);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Transacation 0x%x can't be committed on the peer but it's present\n",
                    __FUNCTION__, (unsigned int)transaction->transaction_id);
                wait_status = status;
        
                /*Do not abort as
                 *(1) cfg is already consistent between both SPs and between in-memory cfg and on-disk data
                 *(2) transaction on peer would be cleared when start next db transaction
                 *(3) if abort, need to rollback the on-disk cfg too, which may cause more error.
                 */
            }

            /*only active side leads the transaction invalidation operation.
            *passive side would not invalidate transaction by itself here. Only when
            *active side decides to invalidate db transaction can it do the invalidation.
            *The reason is that when active SP panics when it does persist, passive SP
            *can continue the persist when it becomes ACTIVE
            */    
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: begin to invalidate db transaction, synching to peer\n",
                           __FUNCTION__);
        
            status = fbe_database_transaction_operation_on_peer(FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_INVALIDATE,
                                                                database_service);
        
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Transacation 0x%x can't be invalidated on the peer but it's present\n",
                    __FUNCTION__, (unsigned int)transaction->transaction_id);
                wait_status = status;
        
                /*Do not abort as
                 *(1) cfg is already consistent between both SPs and between in-memory cfg and on-disk data
                 *(2) transaction on peer would be cleared when start next db transaction
                 *(3) if abort, need to rollback the on-disk cfg too, which may cause more error.
                 */
            }
        
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: transaction invalidated on peer, now invalidate ourselves'\n",
                __FUNCTION__);
        
        }
    
        fbe_zero_memory(database_service->transaction_ptr, sizeof(database_transaction_t));/*clean after ourselvs no matter what*/
        transaction->state = DATABASE_TRANSACTION_STATE_INACTIVE;
    } /*end if (database_common_cmi_is_active())*/
    else
    {
        /*passive side would not invalidate transaction by itself here. Only when
        *active side finishes all the transaction work can it do the invalidation.
        *The reason is that when active SP panics when it does persist, passive SP
        *can continue the persist when it becomes ACTIVE
        */    
    }

    fbe_semaphore_destroy(&persist_transaction_commit_context.commit_semaphore);

    if (wait_status == FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Persist transaction rolled-back successfully!\n",
            __FUNCTION__);
    }
    
    return wait_status;
}
/*!***************************************************************
 * @fn fbe_database_transaction_commit_dma_from_peer
 *****************************************************************
 * @brief
 *   This function processes msg from active peer to commit a transaction.
 *
 * @param db_service - pointer to the database service
 *
 * @return fbe_status_t 
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_transaction_commit_dma_from_peer(fbe_database_service_t *db_service,
                                                           fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_status_t					status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_cmi_msg_t * 		msg_memory = NULL;
    fbe_database_state_t 			database_state;
    fbe_u32_t						entry_id_idx = 0;
    fbe_u32_t                       msg_size;

    msg_size = fbe_database_cmi_get_msg_size(db_cmi_msg->msg_type);
    /* If the received cmi msg has a larger size, version information will be sent back in the confirm message */
    if(db_cmi_msg->version_header.size > msg_size) {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT_CONFIRM;
        msg_memory->completion_callback = fbe_database_transaction_commit_request_from_peer_completion;
        msg_memory->payload.transaction_confirm.status = FBE_STATUS_GENERIC_FAILURE;

        msg_memory->payload.transaction_confirm.err_type = FBE_CMI_MSG_ERR_TYPE_LARGER_MSG_SIZE;
        msg_memory->payload.transaction_confirm.received_msg_type = db_cmi_msg->msg_type;
        msg_memory->payload.transaction_confirm.sep_version = SEP_PACKAGE_VERSION;

		database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
					   "%s: received msg 0x%x, size(%d) is larger than local cmi msg size(%d)!!\n",
					   __FUNCTION__, db_cmi_msg->msg_type,
                       db_cmi_msg->version_header.size, msg_size);	
        fbe_database_cmi_send_message(msg_memory, NULL);
        return FBE_STATUS_OK;
    }

    get_database_service_state(db_service,  &database_state);
    if (database_state != FBE_DATABASE_STATE_READY) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: received msg 0x%x, When DB is in %d state. !!\n",
                       __FUNCTION__, db_cmi_msg->msg_type, database_state);	
        status = FBE_STATUS_OK;  /* DB service is not ready yet.*/
    }else{
        status = FBE_STATUS_OK;
        
        if(db_service->transaction_ptr->job_number!= db_cmi_msg->payload.db_transaction_dma.job_number)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: received job no 0x%x != current job no 0x%x !\n",
                           __FUNCTION__, 
                           (unsigned int)db_cmi_msg->payload.db_transaction_dma.job_number,
                           (unsigned int)db_service->transaction_ptr->job_number);
            
            msg_memory = fbe_database_cmi_get_msg_memory();
            if (msg_memory == NULL) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);
            
                return FBE_STATUS_GENERIC_FAILURE;
            }
            
            /*we'll return the status as a handshake to the master*/
            msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT_CONFIRM;
            msg_memory->completion_callback = fbe_database_transaction_abort_request_from_peer_completion;
            msg_memory->payload.transaction_confirm.status = FBE_STATUS_GENERIC_FAILURE;

            msg_memory->payload.transaction_confirm.err_type = FBE_CMI_MSG_ERR_TYPE_DISMATCH_TRANSACTION;
            msg_memory->payload.transaction_confirm.received_msg_type = db_cmi_msg->msg_type;
            msg_memory->payload.transaction_confirm.sep_version = SEP_PACKAGE_VERSION;

            fbe_database_cmi_send_message(msg_memory, NULL);
            return FBE_STATUS_OK;
        }

        /*before the commit, let's copy the transaction table the peer send us since it contains
        all the entry id's for the persist service which we'll use in case the peer goes down and the user want's
        to remove or modify an entry*/
        for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_USER_ENTRY; entry_id_idx++) {
            db_service->transaction_ptr->user_entries[entry_id_idx].header.entry_id = 
                db_service->transaction_backup_ptr->user_entries[entry_id_idx].header.entry_id;
        }

        for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_OBJECTS; entry_id_idx++) {
            db_service->transaction_ptr->object_entries[entry_id_idx].header.entry_id = 
                db_service->transaction_backup_ptr->object_entries[entry_id_idx].header.entry_id;
        }

        for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_EDGES; entry_id_idx++) {
            db_service->transaction_ptr->edge_entries[entry_id_idx].header.entry_id = 
                db_service->transaction_backup_ptr->edge_entries[entry_id_idx].header.entry_id;
        }

        /* we need to copy everything over */
        for (entry_id_idx = 0; entry_id_idx < DATABASE_TRANSACTION_MAX_GLOBAL_INFO; entry_id_idx++) {
            db_service->transaction_ptr->global_info_entries[entry_id_idx] = 
                db_service->transaction_backup_ptr->global_info_entries[entry_id_idx];
        }

        status = fbe_database_transaction_commit(db_service);
    }

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT_CONFIRM;
    msg_memory->completion_callback = fbe_database_transaction_commit_request_from_peer_completion;
    msg_memory->payload.transaction_confirm.status = status;

    fbe_database_cmi_send_message(msg_memory, NULL);
    
    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_transaction_operation_dma_completion
 *****************************************************************
 * @brief
 *   This is the callback function when DMA is done.
 *
 * @param returned_msg - pointer to the msg
 *
 * @return fbe_status_t 
 *
 * @version
 *
 ****************************************************************/
static fbe_status_t fbe_database_transaction_operation_dma_completion (fbe_database_cmi_msg_t *returned_msg,
                                                                       fbe_status_t completion_status,
                                                                       void *context)
{
    fbe_database_peer_synch_context_t *	peer_synch_ptr = (fbe_database_peer_synch_context_t *)context;

    /*let's return the message memory first*/
    //fbe_database_cmi_return_msg_memory(returned_msg);

    if (completion_status != FBE_STATUS_OK) {
        /*we do nothing here, the CMI interrupt for a dead SP will trigger us doing the config read on our own*/
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to update peer transaction, peer state unknown\n", __FUNCTION__ );
    }

    peer_synch_ptr->status = completion_status;
    fbe_semaphore_release(&peer_synch_ptr->semaphore, 0, 1 ,FALSE);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_transaction_operation_dma_sync
 *****************************************************************
 * @brief
 *   This function sends memory through cmi.
 *
 * @param transaction - pointer to dma structure
 *
 * @return fbe_status_t 
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_transaction_operation_dma_sync(fbe_cmi_memory_t * transaction)
{
    fbe_status_t						wait_status;
    fbe_database_peer_synch_context_t	peer_synch;

    fbe_semaphore_init(&peer_synch.semaphore, 0, 1);
    fbe_semaphore_init(&peer_transaction_confirm.semaphore, 0, 1);

    /* Send to CMI */
    transaction->context = &peer_synch;
    fbe_cmi_send_memory(transaction);

    /*we are not done until the peer tells us it got it and it is happy*/
    wait_status = fbe_semaphore_wait_ms(&peer_synch.semaphore, 30000);
    fbe_semaphore_destroy(&peer_synch.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){

        fbe_semaphore_destroy(&peer_transaction_confirm.semaphore);
        /*peer may have been there when we started but just went away in mid transaction, in this case we just continue as ususal*/
        if (!database_common_peer_is_alive()) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s:peer died, we continue.\n",__FUNCTION__); 
            return FBE_STATUS_OK;
        }else{
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s:request on peer timed out!!!\n",__FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /*is the peer even there ?*/
    if (peer_synch.status == FBE_STATUS_NO_DEVICE) {
        fbe_semaphore_destroy(&peer_transaction_confirm.semaphore);
        /*nope, so let's go on*/
        return FBE_STATUS_OK;
    }

    if (peer_synch.status != FBE_STATUS_OK) {
        fbe_semaphore_destroy(&peer_transaction_confirm.semaphore);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to send command to peer!!!\n",__FUNCTION__);	
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now let's wait for the peer to confirm it was able to do it*/
    wait_status = fbe_semaphore_wait_ms(&peer_transaction_confirm.semaphore, 30000);
    fbe_semaphore_destroy(&peer_transaction_confirm.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){
        /*peer may have been there when we started but just went away in mid transaction, in this case we just continue as ususal*/
        if (!database_common_peer_is_alive()) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: Update config tables, peer died, we continue.\n",__FUNCTION__); 
            return FBE_STATUS_OK;
        }else{
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: Update config tables timed out!!!\n",__FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (peer_transaction_confirm.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Peer update failed.\n",
                       __FUNCTION__);
    }

    return peer_transaction_confirm.status;
}

/*!***************************************************************
 * @fn fbe_database_transaction_operation_dma
 *****************************************************************
 * @brief
 *   This function starts dma for transaction. Currently it is used
 *   for commit only.
 *
 * @param transaction_operation_type - operation type
 *
 * @return fbe_status_t 
 *
 * @version
 *
 ****************************************************************/
fbe_status_t 
fbe_database_transaction_operation_dma(fbe_cmi_msg_type_t transaction_operation_type,
                                       fbe_database_service_t *database_service)
{
    fbe_status_t					status;
    fbe_database_cmi_msg_t * 		msg_memory = NULL;
    fbe_cmi_memory_t                transaction;

    if (database_service->transaction_peer_physical_address == 0) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Currently we only support transer once */
    if (database_service->transaction_alloc_size > MAX_SEP_IO_MEM_LENGTH) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*get memory for the message if we did not get it on the previous loop and did not use it*/
    if (msg_memory == NULL) {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }


    /* Fill in message info */
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT_DMA;
    fbe_zero_memory(&msg_memory->version_header, sizeof(msg_memory->version_header));
    msg_memory->version_header.size = fbe_database_cmi_get_msg_size(msg_memory->msg_type);
    msg_memory->completion_callback = fbe_database_transaction_operation_dma_completion;

    msg_memory->payload.db_transaction_dma.transaction_id = database_service->transaction_ptr->transaction_id;
    msg_memory->payload.db_transaction_dma.state = database_service->transaction_ptr->state;
    msg_memory->payload.db_transaction_dma.transaction_type = database_service->transaction_ptr->transaction_type;
    msg_memory->payload.db_transaction_dma.job_number = database_service->transaction_ptr->job_number;

    /* Fill in structure for dma */
    transaction.client_id = FBE_CMI_CLIENT_ID_DATABASE;
    transaction.context = NULL;
    transaction.source_addr = fbe_cmi_io_get_physical_address((void *)database_service->transaction_ptr);
    transaction.dest_addr = database_service->transaction_peer_physical_address;
    transaction.data_length = database_service->transaction_alloc_size;
    transaction.message = (fbe_cmi_message_t)msg_memory;
    transaction.message_length = fbe_database_cmi_get_msg_size(msg_memory->msg_type);

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: type:%d, src %p dest %p len 0x%x\n", __FUNCTION__, 
                   transaction_operation_type, (void *)(fbe_ptrhld_t)transaction.source_addr, (void *)(fbe_ptrhld_t)transaction.dest_addr, transaction.data_length);
    status = fbe_database_transaction_operation_dma_sync(&transaction);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed for transaction type %d\n", __FUNCTION__, transaction_operation_type);
        fbe_database_cmi_return_msg_memory(msg_memory);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_database_cmi_return_msg_memory(msg_memory);
    msg_memory = NULL;
    return FBE_STATUS_OK;
}


