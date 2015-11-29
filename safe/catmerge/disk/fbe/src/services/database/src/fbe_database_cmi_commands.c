#include "fbe/fbe_cmi_interface.h"
#include "fbe_database_cmi_interface.h"
#include "fbe_database_private.h"
#include "fbe_cmi.h"
#include "fbe/fbe_base_config.h"
#include "fbe_database_config_tables.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe_database_metadata_interface.h"



/*******************/
/*local definitions*/
/*******************/


/*******************/
/* local arguments */
/*******************/

static volatile fbe_bool_t					fbe_database_cmi_stop_peer_update = FBE_FALSE;
static fbe_database_peer_synch_context_t	peer_config_update_confirm;
/*used to get confirmation from peer about updating system db header*/
static fbe_database_peer_synch_context_t peer_update_system_db_header_confirm; 
static fbe_database_peer_synch_context_t peer_online_planned_drive_confirm;
static fbe_database_peer_synch_context_t peer_update_fru_descriptor_confirm;
static fbe_database_peer_synch_context_t peer_update_config_table_confirm;
static fbe_database_peer_synch_context_t peer_update_drive_keys_confirm;
static fbe_database_peer_synch_context_t peer_setup_encryption_keys_confirm;

extern fbe_database_service_t fbe_database_service;

/*******************/
/* local function  */
/*******************/
static fbe_status_t fbe_database_ask_for_peer_db_update_completion(fbe_database_cmi_msg_t *returned_msg, fbe_status_t completion_status, void *context);
static fbe_status_t fbe_database_cmi_process_get_config_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_database_cmi_update_peer_tables_completion(fbe_database_cmi_msg_t *returned_msg, fbe_status_t completion_status, void *context);
static fbe_status_t fbe_database_cmi_send_update_config_to_peer_completion(fbe_database_cmi_msg_t *returned_msg,fbe_status_t completion_status, void *context);
static fbe_status_t fbe_database_cmi_send_get_be_port_info_completion(fbe_database_cmi_msg_t *returned_msg,fbe_status_t completion_status, void *context);
static fbe_status_t fbe_database_cmi_send_and_wait_for_peer_config_synch(fbe_database_cmi_msg_t *msg_memory, fbe_database_peer_synch_context_t *peer_confirm_p);

static fbe_cmi_msg_type_t fbe_database_config_change_request_get_confirm_msg(fbe_cmi_msg_type_t msg_type);
static fbe_status_t fbe_database_cmi_send_db_service_mode_to_peer_completion(fbe_database_cmi_msg_t *returned_msg, fbe_status_t completion_status, void *context);
static fbe_status_t fbe_database_cmi_update_peer_fru_descriptor_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                    fbe_status_t completion_status,
                                                                    void *context);
static fbe_status_t fbe_database_update_fru_descriptor_confirm_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context);
static fbe_status_t fbe_database_update_system_db_header_confirm_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context);
static fbe_status_t fbe_database_cmi_update_peer_system_db_header_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context);
/* This callback function is used for release the memory of CMI message after sending */
static fbe_status_t fbe_database_cmi_send_unknown_msg_response_to_peer_completion(fbe_database_cmi_msg_t *returned_msg,fbe_status_t completion_status, void *context);

/* When passive SP receives db table entries from peer, if the received entry's size is larger, return a warning message*/
static void database_check_table_entry_version_and_send_back_cmi_to_peer_if_needed(fbe_database_service_t * db_service, fbe_database_cmi_msg_t *cmi_msg, fbe_bool_t *is_send_cmi);

static fbe_status_t fbe_database_online_drive_confirm_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context);

static fbe_status_t fbe_database_cmi_sync_online_drive_request_to_peer_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context);

static fbe_status_t fbe_database_cmi_update_shared_emv_info_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context);
static fbe_status_t 
fbe_database_cmi_update_peer_tables_dma_per_table(database_config_table_type_t table_type,
                                                  fbe_u64_t source_addr,
                                                  fbe_u64_t destination_addr,
                                                  fbe_u32_t alloc_size,
                                                  fbe_u64_t	*total_updates_sent);
static fbe_status_t fbe_database_cmi_send_message_and_wait_sync(fbe_database_cmi_msg_t *msg_memory, 
                                                                fbe_cmi_msg_type_t msg_type,
                                                                fbe_database_cmi_sync_context_t *context);

static fbe_status_t fbe_database_cmi_wait_for_send_completion(fbe_database_cmi_msg_t *returned_msg,
                                                              fbe_status_t completion_status,
                                                              void *context);
static fbe_status_t fbe_database_cmi_wait_for_send_completion_no_op(fbe_database_cmi_msg_t *returned_msg,
                                                                    fbe_status_t completion_status,
                                                                    void *context);
static fbe_status_t fbe_database_cmi_process_invalid_vault_drive_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                     fbe_status_t completion_status,
                                                                     void *context);

                                                  

static fbe_status_t fbe_database_cmi_clear_user_modified_wwn_seed_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context);
/*************************************************************************************************************************************************************/

/*this code is running from the passive side, asking the master to get all the configuration to it's side*/
fbe_status_t fbe_database_ask_for_peer_db_update(void)
{
    fbe_database_cmi_msg_t * 	msg_memory = NULL;
    fbe_database_service_t *    database_service_ptr = &fbe_database_service;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    msg_memory->completion_callback = fbe_database_ask_for_peer_db_update_completion;
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_GET_CONFIG;

    msg_memory->payload.get_config.user_table_physical_address = 
        fbe_cmi_io_get_physical_address(database_service_ptr->user_table.table_content.user_entry_ptr);
    msg_memory->payload.get_config.object_table_physical_address = 
        fbe_cmi_io_get_physical_address(database_service_ptr->object_table.table_content.object_entry_ptr);
    msg_memory->payload.get_config.edge_table_physical_address = 
        fbe_cmi_io_get_physical_address(database_service_ptr->edge_table.table_content.edge_entry_ptr);
    msg_memory->payload.get_config.system_spare_table_physical_address = 
        fbe_cmi_io_get_physical_address(database_service_ptr->system_spare_table.table_content.system_spare_entry_ptr);
    msg_memory->payload.get_config.global_info_table_physical_address = 
        fbe_cmi_io_get_physical_address(database_service_ptr->global_info.table_content.global_info_ptr);
    msg_memory->payload.get_config.user_table_alloc_size = database_service_ptr->user_table.alloc_size;
    msg_memory->payload.get_config.object_table_alloc_size = database_service_ptr->object_table.alloc_size;
    msg_memory->payload.get_config.edge_table_alloc_size = database_service_ptr->edge_table.alloc_size;
    msg_memory->payload.get_config.system_spare_table_alloc_size = database_service_ptr->system_spare_table.alloc_size;
    msg_memory->payload.get_config.global_info_table_alloc_size = database_service_ptr->global_info.alloc_size;
    msg_memory->payload.get_config.encryption_key_table_physical_address = 
        fbe_cmi_io_get_physical_address(fbe_database_encryption_get_local_table_virtual_addr());
    msg_memory->payload.get_config.encryption_key_table_alloc_size = fbe_database_encryption_get_local_table_size();

    msg_memory->payload.get_config.transaction_alloc_size = database_service_ptr->transaction_alloc_size;
    msg_memory->payload.get_config.transaction_peer_physical_address = 
        fbe_cmi_io_get_physical_address(database_service_ptr->transaction_backup_ptr);

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: PASSIVE side is sending a requet for peer to get config tables\n", __FUNCTION__ );

    fbe_database_cmi_send_message(msg_memory, NULL);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_database_ask_for_peer_db_update_completion(fbe_database_cmi_msg_t *returned_msg, fbe_status_t completion_status, void *context)
{
    /*let's return the message memory first*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    /*and check the status*/
    if (completion_status == FBE_STATUS_BUSY) {
        /*let's retry*/
        fbe_database_ask_for_peer_db_update();
    }else if (completion_status != FBE_STATUS_OK) {
        /*we do nothing here, the CMI interrupt for a dead SP will trigger us doing the config read on our own*/
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to send get config message to peer, it must be dead\n", __FUNCTION__ );
    }

    return FBE_STATUS_OK;
}


/*respond to the peer asking us to get a copy of the configuration
Pay attention we don't care right now if we are initialized or not. Once the Job will come, if we are not ready,
we'll wait*/
void fbe_database_cmi_process_get_config(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_cmi_msg_get_config_t *              get_config = &db_cmi_msg->payload.get_config;
    fbe_u32_t                               base_size;

    database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: peer requested DB update\n", __FUNCTION__ );

    base_size = (fbe_u32_t)&((fbe_cmi_msg_get_config_t *)0)->global_info_table_alloc_size;
    /* Update peer table physical addresses */
    if ((db_cmi_msg->version_header.size > base_size) &&
        (get_config->user_table_alloc_size == db_service->user_table.alloc_size) &&
        (get_config->object_table_alloc_size == db_service->object_table.alloc_size) &&
        (get_config->edge_table_alloc_size == db_service->edge_table.alloc_size) &&
        (get_config->system_spare_table_alloc_size == db_service->system_spare_table.alloc_size) &&
        (get_config->global_info_table_alloc_size == db_service->global_info.alloc_size))
    {
        db_service->user_table.peer_table_start_address = get_config->user_table_physical_address;
        db_service->object_table.peer_table_start_address = get_config->object_table_physical_address;
        db_service->edge_table.peer_table_start_address = get_config->edge_table_physical_address;
        db_service->system_spare_table.peer_table_start_address = get_config->system_spare_table_physical_address;
        db_service->global_info.peer_table_start_address = get_config->global_info_table_physical_address;
    }
    else
    {
        // reset to NULL since the tables do not exist on the Peer SP. 
        db_service->user_table.peer_table_start_address = 0;
        db_service->object_table.peer_table_start_address = 0;
        db_service->edge_table.peer_table_start_address = 0;
        db_service->system_spare_table.peer_table_start_address = 0;
        db_service->global_info.peer_table_start_address = 0;
    }


    /* record key memory physical address, if peer provides the info */
    base_size = (fbe_u32_t)&((fbe_cmi_msg_get_config_t *)0)->encryption_key_table_alloc_size;
    if ((db_cmi_msg->version_header.size > base_size) &&
        (get_config->encryption_key_table_alloc_size == fbe_database_encryption_get_local_table_size()))
    {
        fbe_database_encryption_set_peer_table_addr(get_config->encryption_key_table_physical_address);
    }
    else
    {
        // reset to NULL since the table does not exist on the Peer SP. 
        fbe_database_encryption_set_peer_table_addr(0);
    }

    fbe_database_initiate_update_db_on_peer();
   
    base_size = (fbe_u32_t)&((fbe_database_cmi_msg_t *)0)->payload;
    base_size += (fbe_u32_t)&((fbe_cmi_msg_get_config_t *)0)->transaction_alloc_size;
    /* Update peer table physical addresses */
    if ((db_cmi_msg->version_header.size > base_size) &&
        (get_config->transaction_alloc_size == db_service->transaction_alloc_size))
    {
        db_service->transaction_peer_physical_address = get_config->transaction_peer_physical_address;
    }
    else
    {
        // reset to NULL since the tables do not exist on the Peer SP. 
        db_service->transaction_peer_physical_address = 0;
    }

    return;
}

void fbe_database_initiate_update_db_on_peer(void)
{
    fbe_job_service_update_db_on_peer_t	*	update_data_on_peer = NULL;
    fbe_packet_t *							packet = NULL;

    /*we allocate and don't initialize, the fbe_database_send_packet_to_service_asynch does it for us*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to allocate packet for peer update, peer DB will get stuck\n", __FUNCTION__ );

        return;
    }

    update_data_on_peer = fbe_transport_allocate_buffer();
    if (update_data_on_peer == NULL) {
        fbe_transport_release_packet(packet);
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to allocate memory for peer update, peer DB will get stuck\n", __FUNCTION__ );

        return;
    }

    /*we'll send this and forget about it. This will trigger our peer update when the job service will
    call us with usurper command FBE_DATABASE_CONTROL_CODE_UPDATE_PEER_CONFIG*/
    fbe_database_send_packet_to_service_asynch(packet,
                                               FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER,
                                               update_data_on_peer,
                                               sizeof(fbe_job_service_update_db_on_peer_t),
                                               FBE_SERVICE_ID_JOB_SERVICE,
                                               NULL,
                                               0,
                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                               fbe_database_cmi_process_get_config_completion,
                                               update_data_on_peer,
                                               FBE_PACKAGE_ID_SEP_0);
    return;
}

static fbe_status_t fbe_database_cmi_process_get_config_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_job_service_update_db_on_peer_t	*	update_data_on_peer = (fbe_job_service_update_db_on_peer_t *)context;
    fbe_status_t							packet_status = fbe_transport_get_status_code(packet);
    fbe_payload_control_status_t			control_status;
    fbe_payload_control_operation_t *   	control_operation = NULL;
    fbe_payload_ex_t *                		sep_payload = NULL;

    sep_payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /*did we have any errors ?*/
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (packet_status != FBE_STATUS_OK)) {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: control_status: 0x%x, packet_status: 0x%x.\n",
                        __FUNCTION__, control_status, packet_status);
    }
    
    /* free the memory allocated */
    fbe_transport_release_packet(packet);
    fbe_transport_release_buffer(update_data_on_peer);	

    return FBE_STATUS_OK;
}

/*go in a loop and updte the peer tables*/
fbe_status_t fbe_database_cmi_update_peer_tables(void)
{
    fbe_status_t					status;
    database_table_t *				table_ptr = NULL;
    database_config_table_type_t	table_type = DATABASE_CONFIG_TABLE_INVALID + 1;/*point to the first table*/
    database_table_size_t			table_entry;
    fbe_database_cmi_msg_t * 		msg_memory = NULL;
    fbe_u64_t						total_updates_sent = 0;
    fbe_u32_t						total_retries = 0;
    fbe_s32_t current_outstanding_count = 0;
    fbe_s32_t db_cmi_msg_count_threshold = fbe_database_cmi_get_msg_count_allocated() - 50;/*reserve 50 buffers*/
    fbe_database_cmi_stop_peer_update = FBE_FALSE;
    
    /*go over the tables and send the entries*/
    for (;table_type < DATABASE_CONFIG_TABLE_LAST; table_type++) {

        if (fbe_database_cmi_stop_peer_update) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        status = fbe_database_get_service_table_ptr(&table_ptr, table_type);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get table ptr for table type %d\n", __FUNCTION__, table_type);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*go over the entries in the table*/
        for (table_entry = 0; table_entry < table_ptr->table_size; table_entry++) {

            if (fbe_database_cmi_stop_peer_update) {
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /*get memory for the message if we did not get it on the previous loop and did not use it*/
            if (msg_memory == NULL) {
                total_retries = 0;
                msg_memory = fbe_database_cmi_get_msg_memory();
                /* we are calling this from job context, we don't want anyone proceed until we finish */
                while (msg_memory == NULL) {
                    if ((total_retries % 100)) {
                        database_trace(FBE_TRACE_LEVEL_ERROR, 
                                       FBE_TRACE_MESSAGE_ID_INFO, 
                                       "%s: failed to get CMI msg memory.  Waiting...\n", __FUNCTION__);
                    }
                    total_retries ++;
                    fbe_thread_delay(EMCPAL_MINIMUM_TIMEOUT_MSECS);
                    msg_memory = fbe_database_cmi_get_msg_memory();
                }
            }

            /*fill it*/
            switch (table_type) {
            case DATABASE_CONFIG_TABLE_EDGE:
                if (table_ptr->table_content.edge_entry_ptr[table_entry].header.state != DATABASE_CONFIG_ENTRY_VALID) {
                    continue;/*nothing here*/
                }

               
                fbe_copy_memory(&msg_memory->payload.db_cmi_update_table.update_peer_table_type.edge_entry,
                                &table_ptr->table_content.edge_entry_ptr[table_entry],
                                sizeof(database_edge_entry_t));

                break;
            case DATABASE_CONFIG_TABLE_USER:
                if (table_ptr->table_content.user_entry_ptr[table_entry].header.state != DATABASE_CONFIG_ENTRY_VALID) {
                    continue;/*nothing here*/
                }

                fbe_copy_memory(&msg_memory->payload.db_cmi_update_table.update_peer_table_type.user_entry,
                                &table_ptr->table_content.user_entry_ptr[table_entry],
                                sizeof(database_user_entry_t));

                break;
            case DATABASE_CONFIG_TABLE_OBJECT:
                if (table_ptr->table_content.object_entry_ptr[table_entry].header.state != DATABASE_CONFIG_ENTRY_VALID) {
                    continue;/*nothing here*/
                }

                fbe_copy_memory(&msg_memory->payload.db_cmi_update_table.update_peer_table_type.object_entry,
                                &table_ptr->table_content.object_entry_ptr[table_entry],
                                sizeof(database_object_entry_t));

                break;
            case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
                if (table_ptr->table_content.global_info_ptr[table_entry].header.state != DATABASE_CONFIG_ENTRY_VALID) {
                    continue;/*nothing here*/
                }
                
                fbe_copy_memory(&msg_memory->payload.db_cmi_update_table.update_peer_table_type.global_info_entry,
                                &table_ptr->table_content.global_info_ptr[table_entry],
                                sizeof(database_global_info_entry_t));

                break;
            case DATABASE_CONFIG_TABLE_SYSTEM_SPARE:
                if (table_ptr->table_content.system_spare_entry_ptr[table_entry].header.state != DATABASE_CONFIG_ENTRY_VALID) {
                    continue;/*nothing here*/
                }
                
                fbe_copy_memory(&msg_memory->payload.db_cmi_update_table.update_peer_table_type.system_spare_entry,
                                &table_ptr->table_content.system_spare_entry_ptr[table_entry],
                                sizeof(database_system_spare_entry_t));
                break;
            default:
                fbe_database_cmi_return_msg_memory(msg_memory);
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: unknown table type:%d\n", __FUNCTION__, table_type);

                return FBE_STATUS_GENERIC_FAILURE;

            }

            /*send it, no need to wait, we have a queue full of these messages and they are freed on completion*/
            msg_memory->payload.db_cmi_update_table.table_type = table_type;
            msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG;
            msg_memory->payload.db_cmi_update_table.table_entry = table_entry;
            msg_memory->completion_callback = fbe_database_cmi_update_peer_tables_completion;
            total_updates_sent++;
            fbe_database_cmi_send_message(msg_memory, NULL);

            database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: sending table type:%d, index: %d\n", __FUNCTION__, table_type, table_entry);

            /*to slow down and not bombard CMI too much, we'll delay for a bit until it's issues are fixed
            this is nasty and does not scale well with many objects but we have to do it now*/
            if (!(total_updates_sent % 10)) {
                fbe_thread_delay(EMCPAL_MINIMUM_TIMEOUT_MSECS);
            }
            /*
            further delay to make sure we are not sending too many to use up all DB CMI buffers and cause some critical event not 
            able to get its slot. At some rare case. Eg, this may happen when DB CMI msgs sent are bogged and contact lost is detected.
            */
            while((current_outstanding_count = fbe_database_cmi_get_outstanding_msg_count()) >= db_cmi_msg_count_threshold){/*Throttling*/
                database_trace(FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s:msg count %d>=%d after sending tbl(%d)idx(%d).Waiting...\n",
                               __FUNCTION__,current_outstanding_count,
                               db_cmi_msg_count_threshold,table_type,table_entry);
                fbe_thread_delay(500);
            }
            msg_memory = NULL;


        }
    }

    /* now, let's push the key memory over */
    if (fbe_database_encryption_get_peer_table_addr() != 0)
    {
        /*send the table over */
        status = fbe_database_cmi_update_peer_tables_dma_per_table(DATABASE_CONFIG_TABLE_KEY_MEMORY,
                                                                   fbe_cmi_io_get_physical_address((void *)fbe_database_encryption_get_local_table_virtual_addr()),
                                                                   fbe_database_encryption_get_peer_table_addr(),
                                                                   fbe_database_encryption_get_local_table_size(),
                                                                   &total_updates_sent);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to send table type %d\n", __FUNCTION__, table_type);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /*when we are done pushing the entries, we are sending the END_UPDATE message,
        This will trigger the object creation and commit on the peer*/

    /*if we allocated and did not use, we'll use it for the final message*/
    if (msg_memory == NULL) {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "%s: failed to get CMI msg memory\n", __FUNCTION__);
    
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: ACTIVE side sent %llu updates to peer\n", __FUNCTION__, (unsigned long long)total_updates_sent);

    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_DONE;
    msg_memory->completion_callback = fbe_database_cmi_update_peer_tables_completion;
    msg_memory->payload.update_done_msg.entries_sent = total_updates_sent;
    fbe_database_cmi_send_message(msg_memory, NULL);

    return FBE_STATUS_OK;


}

static fbe_status_t fbe_database_cmi_update_peer_tables_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context)
{
    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: complete msg type:%d table type:%d, index: %d\n", 
                   __FUNCTION__, 
                   returned_msg->msg_type, 
                   returned_msg->payload.db_cmi_update_table.table_type, 
                   returned_msg->payload.db_cmi_update_table.table_entry);

    /*let's return the message memory first*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    
    if (completion_status != FBE_STATUS_OK) {
        /*we do nothing here, the CMI interrupt for a dead SP will trigger us doing the config read on our own*/
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to update peer's table, peer state unknown\n", __FUNCTION__ );

        fbe_database_cmi_stop_peer_update = FBE_TRUE;
    }

    return FBE_STATUS_OK;

}

/* Called on the passive side when the active side is pushing the tables to it
 * When new version release, database receives new version of database table entry from peer SP. 
 * In this case, the passive SP should initialize the database entry first,   
 * then copy the data from peer by the size recorded in the data. 
 * If current SP is older version and peer SP is the newer version, don't let current SP boots up
*/
void fbe_database_cmi_process_update_config(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_update_peer_table_t *	update_table = &cmi_msg->payload.db_cmi_update_table;
    database_table_t *					table_ptr = NULL;
    fbe_status_t						status;
    fbe_bool_t                          is_send_cmi;

    status = fbe_database_get_service_table_ptr(&table_ptr, update_table->table_type);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to get table ptr for table type %d\n", __FUNCTION__, update_table->table_type);

        return;
    }
    
    /* If the entry size from Peer SP is larger, send a CMI message and return */
    database_check_table_entry_version_and_send_back_cmi_to_peer_if_needed(db_service, cmi_msg, &is_send_cmi);
    if (is_send_cmi == FBE_TRUE) {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: return warning CMI message to Peer\n", __FUNCTION__);

        return;
    }
    
    /*sanity check*/
    if (update_table->table_entry > table_ptr->table_size) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: entry index %d bigger than table max %d\n", __FUNCTION__, update_table->table_entry, db_service->object_table.table_size);

        return;

    }

    /*copy the table from the master to us*/
    switch (update_table->table_type) {
    case DATABASE_CONFIG_TABLE_EDGE:
        fbe_database_config_table_update_edge_entry(table_ptr, &update_table->update_peer_table_type.edge_entry);

        break;
    case DATABASE_CONFIG_TABLE_USER:
        fbe_database_config_table_update_user_entry(table_ptr, &update_table->update_peer_table_type.user_entry);

        break;
    case DATABASE_CONFIG_TABLE_OBJECT:
        fbe_database_config_table_update_object_entry(table_ptr, &update_table->update_peer_table_type.object_entry);
        break;
    case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
        fbe_database_config_table_update_global_info_entry(table_ptr, &update_table->update_peer_table_type.global_info_entry);
        break;
    case DATABASE_CONFIG_TABLE_SYSTEM_SPARE:
        fbe_database_config_table_update_system_spare_entry(table_ptr, &update_table->update_peer_table_type.system_spare_entry);

        break;

    default:
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal table type %d\n", __FUNCTION__, update_table->table_type);

    }

    /*increment the number of updates we got*/
    db_service->updates_received++;
    
}

static void database_check_table_entry_version_and_send_back_cmi_to_peer_if_needed(fbe_database_service_t * db_service, fbe_database_cmi_msg_t *cmi_msg, fbe_bool_t *is_send_cmi)
{
    fbe_cmi_msg_update_peer_table_t *	update_table = &cmi_msg->payload.db_cmi_update_table;

    database_object_entry_t             *object_entry;
    database_edge_entry_t               *edge_entry;
    database_user_entry_t               *user_entry;
    database_global_info_entry_t        *global_info_entry;
    fbe_u32_t                           local_entry_size = 0;
    fbe_u32_t                           version_size = 0;

    switch (update_table->table_type) {
    case DATABASE_CONFIG_TABLE_EDGE:
        edge_entry = &update_table->update_peer_table_type.edge_entry;
        version_size = edge_entry->header.version_header.size;
        local_entry_size = database_common_edge_entry_size();
        break;
    case DATABASE_CONFIG_TABLE_USER:
        user_entry = &update_table->update_peer_table_type.user_entry;
        version_size = user_entry->header.version_header.size;
        local_entry_size = database_common_user_entry_size(user_entry->db_class_id);
        break;
    case DATABASE_CONFIG_TABLE_OBJECT:
        object_entry = &update_table->update_peer_table_type.object_entry;
        version_size = object_entry->header.version_header.size;
        local_entry_size = database_common_object_entry_size(object_entry->db_class_id);
        break;
    case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
        global_info_entry = &update_table->update_peer_table_type.global_info_entry;
        version_size = global_info_entry->header.version_header.size;
        local_entry_size = database_common_global_info_entry_size(global_info_entry->type);
        break;

    default:
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal table type %d\n", __FUNCTION__, update_table->table_type);
        return;
    }

    if (version_size > local_entry_size) {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: type = %d, version_size is %d, software entry size is %d\n", 
                        __FUNCTION__, update_table->table_type,version_size, local_entry_size);
        fbe_database_handle_unknown_cmi_msg_size_from_peer(db_service, cmi_msg);
        /*We got bigger size database entry from Peer, return warning and enter into service mode */
        fbe_event_log_write(SEP_ERROR_CODE_NEWER_CONFIG_FROM_PEER,
                                        NULL, 0, "%x %x",
                                        version_size,
                                        local_entry_size);
        fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_PROBLEMATIC_DATABASE_VERSION);
        *is_send_cmi = FBE_TRUE;
    } else {
        *is_send_cmi = FBE_FALSE;
    }

    return;
}

/*the master sp tells us it completed sending us the tables*/
void fbe_database_cmi_process_update_config_done(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_u32_t   msg_size;

    db_service->updates_sent_by_active_side = db_cmi_msg->payload.update_done_msg.entries_sent;


    /* If the size in db_cmi_msg's version header is larger than the size of cmi message,
       we will send back a warning message to tell the sender */
    msg_size = fbe_database_cmi_get_msg_size(db_cmi_msg->msg_type);
    if (msg_size < db_cmi_msg->version_header.size) {
        fbe_database_handle_unknown_cmi_msg_size_from_peer(db_service, db_cmi_msg);
    }

    /* fix key memory table */
    fbe_database_encryption_reconstruct_key_tables();

    /* fix c4mirror table entries */
    fbe_database_reconstruct_export_lun_tables(db_service);

    /*we'll stay in FBE_DATABASE_STATE_WAITING_FOR_CONFIG for now since we want to finish it all*/
    
    /*at this point we'll let the database own thread take over and do the init*/
    fbe_database_config_thread_init_passive();

}

void fbe_database_cmi_process_update_config_passive_init_done(fbe_database_service_t *db_service)
{
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "db_cmi_proc_update_cfg_passive_init_done:Passive SP finish build cfg. Both Ready\n");

    set_database_service_state(db_service, FBE_DATABASE_STATE_READY);
    set_database_service_peer_state(db_service, FBE_DATABASE_STATE_READY);

    /*now we can complete the packet back to the job since we are safe*/
    database_control_complete_update_peer_packet(FBE_STATUS_OK);

}

fbe_status_t fbe_database_cmi_send_encryption_backup_state_to_peer(fbe_encryption_backup_state_t  encryption_backup_state)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_status_t                        status;
    fbe_database_peer_synch_context_t	peer_synch;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    msg_memory->payload.update_encryption_backup_state.new_state = encryption_backup_state;
    msg_memory->payload.update_encryption_backup_state.context = &peer_synch;

    msg_memory->completion_callback = fbe_database_cmi_wait_for_send_completion_no_op;
    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_BACKUP_STATE;

    fbe_semaphore_init(&peer_synch.semaphore, 0, 1);
    
    /*send it*/
    fbe_database_cmi_send_message(msg_memory, &peer_synch);

    /*we are not done until the peer tells us it got it and it is happy*/
    status = fbe_semaphore_wait_ms(&peer_synch.semaphore, 30000);
    fbe_semaphore_destroy(&peer_synch.semaphore);

    if (status == FBE_STATUS_TIMEOUT){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Timeout. failed to send command SET_ENCRYPTION_BACKUP_STATE to peer!!!\n",
                       __FUNCTION__);	
    }

    /*let's return the message memory */
    fbe_database_cmi_return_msg_memory(msg_memory);

    return status;
}

/*tell the peer there is a change in one of the PVDs*/
fbe_status_t fbe_database_cmi_send_get_be_port_info_to_peer(fbe_database_service_t * fbe_database_service,
                                                            fbe_database_control_get_be_port_info_t  *be_port_info)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_cmi_msg_database_get_be_port_info_t *cmi_be_port_info;
    fbe_database_cmi_sync_context_t *context;
    fbe_status_t status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    context = (fbe_database_cmi_sync_context_t *)fbe_transport_allocate_buffer();

    fbe_semaphore_init(&context->semaphore, 0, 1);
    context->context_buffer = be_port_info;

    cmi_be_port_info = &msg_memory->payload.get_be_port_info;
    cmi_be_port_info->context = context;
    fbe_copy_memory(&cmi_be_port_info->be_port_info, be_port_info, sizeof(fbe_database_control_get_be_port_info_t));

    /*and send it. */
    status = fbe_database_cmi_send_message_and_wait_sync(msg_memory, 
                                                         FBE_DATABASE_CMI_MSG_TYPE_GET_BE_PORT_INFO,
                                                         context);
    fbe_semaphore_destroy(&context->semaphore);
    fbe_transport_release_buffer(context);

    /*let's return the message memory */
    fbe_database_cmi_return_msg_memory(msg_memory);

    return status;
}

void fbe_database_cmi_process_get_be_port_info(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_cmi_msg_t *     		msg_memory = NULL;
    fbe_status_t    status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        return;
    }

    status = fbe_database_get_be_port_info(&cmi_msg->payload.get_be_port_info.be_port_info);

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_GET_BE_PORT_INFO_CONFIRM;
    msg_memory->completion_callback = fbe_database_cmi_process_send_confirm_completion;
    cmi_msg->payload.get_be_port_info.return_status = status;

    fbe_copy_memory(&msg_memory->payload.get_be_port_info, 
                    &cmi_msg->payload.get_be_port_info, 
                    sizeof(fbe_cmi_msg_database_get_be_port_info_t));
    fbe_database_cmi_send_message(msg_memory, NULL);
    return;
}

void fbe_database_cmi_process_set_encryption_backup_state_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_peer_synch_context_t *context = cmi_msg->payload.update_encryption_backup_state.context;

    fbe_semaphore_release(&context->semaphore, 0, 1 ,FALSE);

    return;
}

void fbe_database_cmi_process_set_encryption_backup_state(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_cmi_msg_t *     		msg_memory = NULL;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        return;
    }

    db_service->encryption_backup_state = cmi_msg->payload.update_encryption_backup_state.new_state;

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_BACKUP_STATE_CONFIRM;
    msg_memory->completion_callback = fbe_database_cmi_process_send_confirm_completion; // shared
    msg_memory->payload.update_encryption_backup_state.context = cmi_msg->payload.update_encryption_backup_state.context;

    fbe_database_cmi_send_message(msg_memory, NULL);
    return;
}

fbe_status_t fbe_database_cmi_process_send_confirm_completion(fbe_database_cmi_msg_t *returned_msg,
                                                              fbe_status_t completion_status,
                                                              void *context)
{
    /*let's return the message memory and for now that's all we need to do*/
    fbe_database_cmi_return_msg_memory(returned_msg);
    return FBE_STATUS_OK;
}

void fbe_database_cmi_process_get_be_port_info_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_database_get_be_port_info_t * be_port_info = &cmi_msg->payload.get_be_port_info;
    fbe_database_cmi_sync_context_t *context = be_port_info->context;
    fbe_database_control_get_be_port_info_t *port_info;

    context->status = be_port_info->return_status;
    port_info = (fbe_database_control_get_be_port_info_t *) context->context_buffer;

    fbe_copy_memory(port_info, &be_port_info->be_port_info, sizeof(fbe_database_control_get_be_port_info_t));
    fbe_semaphore_release(&context->semaphore, 0, 1 ,FALSE);

    return;
}

fbe_status_t fbe_database_cmi_send_setup_kek_to_peer (fbe_database_service_t * fbe_database_service,
                                                      fbe_database_control_setup_kek_t *key_encryption_info)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_cmi_msg_database_setup_kek_t *cmi_kek_info;
    fbe_database_cmi_sync_context_t *context;
    fbe_status_t status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    context = (fbe_database_cmi_sync_context_t *)fbe_transport_allocate_buffer();

    fbe_semaphore_init(&context->semaphore, 0, 1);
    context->context_buffer = key_encryption_info;

    cmi_kek_info = &msg_memory->payload.setup_kek;
    cmi_kek_info->context = context;
    fbe_copy_memory(&cmi_kek_info->kek_info, key_encryption_info, sizeof(fbe_database_control_setup_kek_t));

    /*and send it. */
    status = fbe_database_cmi_send_message_and_wait_sync(msg_memory, 
                                                         FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK,
                                                         context);
    fbe_semaphore_destroy(&context->semaphore);
    fbe_transport_release_buffer(context);

    /*let's return the message memory */
    fbe_database_cmi_return_msg_memory(msg_memory);

    return status;
}

void fbe_database_cmi_process_setup_kek(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_cmi_msg_t *     		msg_memory = NULL;
    fbe_status_t    status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        return;
    }

    status = fbe_database_setup_kek(&cmi_msg->payload.setup_kek.kek_info);

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_CONFIRM;
    msg_memory->completion_callback = fbe_database_cmi_process_send_confirm_completion;
    cmi_msg->payload.setup_kek.return_status = status;

    fbe_copy_memory(&msg_memory->payload.setup_kek, 
                    &cmi_msg->payload.setup_kek, 
                    sizeof(fbe_cmi_msg_database_setup_kek_t));
    fbe_database_cmi_send_message(msg_memory, NULL);
    return;
}

void fbe_database_cmi_process_setup_kek_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_database_setup_kek_t * cmi_setup_kek_info = &cmi_msg->payload.setup_kek;
    fbe_database_cmi_sync_context_t *context = cmi_setup_kek_info->context;
    fbe_database_control_setup_kek_t *kek_info;

    context->status = cmi_setup_kek_info->return_status;
    kek_info = (fbe_database_control_setup_kek_t *) context->context_buffer;

    fbe_copy_memory(kek_info, &cmi_setup_kek_info->kek_info, sizeof(fbe_database_control_setup_kek_t));
    fbe_semaphore_release(&context->semaphore, 0, 1 ,FALSE);

    return;
}

fbe_status_t fbe_database_cmi_send_destroy_kek_to_peer (fbe_database_service_t * fbe_database_service,
                                                        fbe_database_control_destroy_kek_t *key_encryption_info)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_cmi_msg_database_destroy_kek_t *cmi_kek_info;
    fbe_database_cmi_sync_context_t *context;
    fbe_status_t status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    context = (fbe_database_cmi_sync_context_t *)fbe_transport_allocate_buffer();

    fbe_semaphore_init(&context->semaphore, 0, 1);
    context->context_buffer = key_encryption_info;

    cmi_kek_info = &msg_memory->payload.destroy_kek;
    cmi_kek_info->context = context;
    fbe_copy_memory(&cmi_kek_info->kek_info, key_encryption_info, sizeof(fbe_database_control_destroy_kek_t));

    /*and send it. */
    status = fbe_database_cmi_send_message_and_wait_sync(msg_memory, 
                                                         FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK,
                                                         context);
    fbe_semaphore_destroy(&context->semaphore);
    fbe_transport_release_buffer(context);

    /*let's return the message memory */
    fbe_database_cmi_return_msg_memory(msg_memory);

    return status;
}

void fbe_database_cmi_process_destroy_kek(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_cmi_msg_t *     		msg_memory = NULL;
    fbe_status_t    status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        return;
    }

    status = fbe_database_destroy_kek(&cmi_msg->payload.destroy_kek.kek_info);

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_CONFIRM;
    msg_memory->completion_callback = fbe_database_cmi_process_send_confirm_completion;
    cmi_msg->payload.destroy_kek.return_status = status;

    fbe_copy_memory(&msg_memory->payload.destroy_kek, 
                    &cmi_msg->payload.destroy_kek, 
                    sizeof(fbe_cmi_msg_database_destroy_kek_t));
    fbe_database_cmi_send_message(msg_memory, NULL);
    return;
}

void fbe_database_cmi_process_destroy_kek_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_database_destroy_kek_t * cmi_destroy_kek_info = &cmi_msg->payload.destroy_kek;
    fbe_database_cmi_sync_context_t *context = cmi_destroy_kek_info->context;
    fbe_database_control_destroy_kek_t *kek_info;

    context->status = cmi_destroy_kek_info->return_status;
    kek_info = (fbe_database_control_destroy_kek_t *) context->context_buffer;

    fbe_copy_memory(kek_info, &cmi_destroy_kek_info->kek_info, sizeof(fbe_database_control_destroy_kek_t));
    fbe_semaphore_release(&context->semaphore, 0, 1 ,FALSE);

    return;
}

fbe_status_t fbe_database_cmi_send_setup_kek_kek_to_peer (fbe_database_service_t * fbe_database_service,
                                                      fbe_database_control_setup_kek_kek_t *key_encryption_info)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_cmi_msg_database_setup_kek_kek_t *cmi_kek_info;
    fbe_database_cmi_sync_context_t *context;
    fbe_status_t status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    context = (fbe_database_cmi_sync_context_t *)fbe_transport_allocate_buffer();

    fbe_semaphore_init(&context->semaphore, 0, 1);
    context->context_buffer = key_encryption_info;

    cmi_kek_info = &msg_memory->payload.setup_kek_kek;
    cmi_kek_info->context = context;
    fbe_copy_memory(&cmi_kek_info->kek_info, key_encryption_info, sizeof(fbe_database_control_setup_kek_kek_t));

    /*and send it. */
    status = fbe_database_cmi_send_message_and_wait_sync(msg_memory, 
                                                         FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_KEK,
                                                         context);
    fbe_semaphore_destroy(&context->semaphore);
    fbe_transport_release_buffer(context);

    /*let's return the message memory */
    fbe_database_cmi_return_msg_memory(msg_memory);

    return status;
}

void fbe_database_cmi_process_setup_kek_kek(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_cmi_msg_t *     		msg_memory = NULL;
    fbe_status_t    status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        return;
    }

    status = fbe_database_setup_kek_kek(&cmi_msg->payload.setup_kek_kek.kek_info);

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_KEK_CONFIRM;
    msg_memory->completion_callback = fbe_database_cmi_process_send_confirm_completion;
    cmi_msg->payload.setup_kek_kek.return_status = status;

    fbe_copy_memory(&msg_memory->payload.setup_kek_kek, 
                    &cmi_msg->payload.setup_kek_kek, 
                    sizeof(fbe_cmi_msg_database_setup_kek_kek_t));
    fbe_database_cmi_send_message(msg_memory, NULL);
    return;
}

void fbe_database_cmi_process_setup_kek_kek_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_database_setup_kek_kek_t * cmi_setup_kek_kek_info = &cmi_msg->payload.setup_kek_kek;
    fbe_database_cmi_sync_context_t *context = cmi_setup_kek_kek_info->context;
    fbe_database_control_setup_kek_kek_t *kek_info;

    context->status = cmi_setup_kek_kek_info->return_status;
    kek_info = (fbe_database_control_setup_kek_kek_t *) context->context_buffer;

    fbe_copy_memory(kek_info, &cmi_setup_kek_kek_info->kek_info, sizeof(fbe_database_control_setup_kek_kek_t));
    fbe_semaphore_release(&context->semaphore, 0, 1 ,FALSE);

    return;
}

fbe_status_t fbe_database_cmi_send_destroy_kek_kek_to_peer (fbe_database_service_t * fbe_database_service,
                                                            fbe_database_control_destroy_kek_kek_t *key_encryption_info)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_cmi_msg_database_destroy_kek_kek_t *cmi_kek_info;
    fbe_database_cmi_sync_context_t *context;
    fbe_status_t status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    context = (fbe_database_cmi_sync_context_t *)fbe_transport_allocate_buffer();

    fbe_semaphore_init(&context->semaphore, 0, 1);
    context->context_buffer = key_encryption_info;

    cmi_kek_info = &msg_memory->payload.destroy_kek_kek;
    cmi_kek_info->context = context;
    fbe_copy_memory(&cmi_kek_info->kek_info, key_encryption_info, sizeof(fbe_database_control_destroy_kek_kek_t));

    /*and send it. */
    status = fbe_database_cmi_send_message_and_wait_sync(msg_memory, 
                                                         FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_KEK,
                                                         context);
    fbe_semaphore_destroy(&context->semaphore);
    fbe_transport_release_buffer(context);

    /*let's return the message memory */
    fbe_database_cmi_return_msg_memory(msg_memory);

    return status;
}

void fbe_database_cmi_process_destroy_kek_kek(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_cmi_msg_t *     		msg_memory = NULL;
    fbe_status_t    status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        return;
    }

    status = fbe_database_destroy_kek_kek(&cmi_msg->payload.destroy_kek_kek.kek_info);

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_KEK_CONFIRM;
    msg_memory->completion_callback = fbe_database_cmi_process_send_confirm_completion;
    cmi_msg->payload.destroy_kek_kek.return_status = status;

    fbe_copy_memory(&msg_memory->payload.destroy_kek_kek, 
                    &cmi_msg->payload.destroy_kek_kek, 
                    sizeof(fbe_cmi_msg_database_destroy_kek_kek_t));
    fbe_database_cmi_send_message(msg_memory, NULL);
    return;
}

void fbe_database_cmi_process_destroy_kek_kek_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_database_destroy_kek_kek_t * cmi_destroy_kek_kek_info = &cmi_msg->payload.destroy_kek_kek;
    fbe_database_cmi_sync_context_t *context = cmi_destroy_kek_kek_info->context;
    fbe_database_control_destroy_kek_kek_t *kek_info;

    context->status = cmi_destroy_kek_kek_info->return_status;
    kek_info = (fbe_database_control_destroy_kek_kek_t *) context->context_buffer;

    fbe_copy_memory(kek_info, &cmi_destroy_kek_kek_info->kek_info, sizeof(fbe_database_control_destroy_kek_kek_t));
    fbe_semaphore_release(&context->semaphore, 0, 1 ,FALSE);

    return;
}

fbe_status_t fbe_database_cmi_send_reestablish_kek_kek_to_peer (fbe_database_service_t * fbe_database_service,
                                                            fbe_database_control_reestablish_kek_kek_t *key_encryption_info)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_cmi_msg_database_reestablish_kek_kek_t *cmi_kek_info;
    fbe_database_cmi_sync_context_t *context;
    fbe_status_t status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    context = (fbe_database_cmi_sync_context_t *)fbe_transport_allocate_buffer();

    fbe_semaphore_init(&context->semaphore, 0, 1);
    context->context_buffer = key_encryption_info;

    cmi_kek_info = &msg_memory->payload.reestablish_kek_kek;
    cmi_kek_info->context = context;
    fbe_copy_memory(&cmi_kek_info->kek_info, key_encryption_info, sizeof(fbe_database_control_reestablish_kek_kek_t));

    /*and send it. */
    status = fbe_database_cmi_send_message_and_wait_sync(msg_memory, 
                                                         FBE_DATABASE_CMI_MSG_TYPE_REESTABLISH_KEK_KEK,
                                                         context);
    fbe_semaphore_destroy(&context->semaphore);
    fbe_transport_release_buffer(context);

    /*let's return the message memory */
    fbe_database_cmi_return_msg_memory(msg_memory);

    return status;
}

void fbe_database_cmi_process_reestablish_kek_kek(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_cmi_msg_t *     		msg_memory = NULL;
    fbe_status_t    status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        return;
    }

    status = fbe_database_reestablish_kek_kek(&cmi_msg->payload.reestablish_kek_kek.kek_info);

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_REESTABLISH_KEK_KEK_CONFIRM;
    msg_memory->completion_callback = fbe_database_cmi_process_send_confirm_completion;
    cmi_msg->payload.reestablish_kek_kek.return_status = status;

    fbe_copy_memory(&msg_memory->payload.reestablish_kek_kek, 
                    &cmi_msg->payload.reestablish_kek_kek, 
                    sizeof(fbe_cmi_msg_database_reestablish_kek_kek_t));
    fbe_database_cmi_send_message(msg_memory, NULL);
    return;
}

void fbe_database_cmi_process_reestablish_kek_kek_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_database_reestablish_kek_kek_t * cmi_reestablish_kek_kek_info = &cmi_msg->payload.reestablish_kek_kek;
    fbe_database_cmi_sync_context_t *context = cmi_reestablish_kek_kek_info->context;
    fbe_database_control_reestablish_kek_kek_t *kek_info;

    context->status = cmi_reestablish_kek_kek_info->return_status;
    kek_info = (fbe_database_control_reestablish_kek_kek_t *) context->context_buffer;

    fbe_copy_memory(kek_info, &cmi_reestablish_kek_kek_info->kek_info, sizeof(fbe_database_control_reestablish_kek_kek_t));
    fbe_semaphore_release(&context->semaphore, 0, 1 ,FALSE);

    return;
}

fbe_status_t fbe_database_cmi_send_port_encryption_mode_to_peer (fbe_database_service_t * fbe_database_service,
                                                                 fbe_database_control_port_encryption_mode_t *mode_info,
                                                                 fbe_database_port_encryption_op_t op)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_cmi_msg_database_port_encryption_mode_t *cmi_mode_info;
    fbe_database_cmi_sync_context_t *context;
    fbe_status_t status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    context = (fbe_database_cmi_sync_context_t *)fbe_transport_allocate_buffer();

    fbe_semaphore_init(&context->semaphore, 0, 1);
    context->context_buffer = mode_info;

    cmi_mode_info = &msg_memory->payload.mode_info;
    cmi_mode_info->op = op;
    cmi_mode_info->context = context;
    fbe_copy_memory(&cmi_mode_info->mode_info, mode_info, sizeof(fbe_database_control_port_encryption_mode_t));

    /*and send it. */
    status = fbe_database_cmi_send_message_and_wait_sync(msg_memory, 
                                                         FBE_DATABASE_CMI_MSG_TYPE_PORT_ENCRYPTION_MODE,
                                                         context);
    fbe_semaphore_destroy(&context->semaphore);
    fbe_transport_release_buffer(context);

    /*let's return the message memory */
    fbe_database_cmi_return_msg_memory(msg_memory);

    return status;
}

void fbe_database_cmi_process_port_encryption_mode(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_cmi_msg_t *     		msg_memory = NULL;
    fbe_status_t    status;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        return;
    }

    if(cmi_msg->payload.mode_info.op == FBE_DATABASE_PORT_ENCRYPTION_OP_MODE_SET)
    {
        status = fbe_database_set_port_encryption_mode(&cmi_msg->payload.mode_info.mode_info);
    } 
    else
    {
        status = fbe_database_get_port_encryption_mode(&cmi_msg->payload.mode_info.mode_info);
    }


    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_PORT_ENCRYPTION_MODE_CONFIRM;
    msg_memory->completion_callback = fbe_database_cmi_process_send_confirm_completion;
    cmi_msg->payload.mode_info.return_status = status;

    fbe_copy_memory(&msg_memory->payload.mode_info, 
                    &cmi_msg->payload.mode_info, 
                    sizeof(fbe_cmi_msg_database_port_encryption_mode_t));
    fbe_database_cmi_send_message(msg_memory, NULL);
    return;
}

void fbe_database_cmi_process_port_encryption_mode_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_database_port_encryption_mode_t * cmi_set_encryption_info = &cmi_msg->payload.mode_info;
    fbe_database_cmi_sync_context_t *context = cmi_set_encryption_info->context;
    fbe_database_control_port_encryption_mode_t *mode_info;

    context->status = cmi_set_encryption_info->return_status;
    mode_info = (fbe_database_control_port_encryption_mode_t *) context->context_buffer;

    fbe_copy_memory(mode_info, &cmi_set_encryption_info->mode_info, sizeof(fbe_database_control_port_encryption_mode_t));
    fbe_semaphore_release(&context->semaphore, 0, 1 ,FALSE);

    return;
}

/*send a config command and wait for it to be completed on the peer*/
static fbe_status_t fbe_database_cmi_send_message_and_wait_sync(fbe_database_cmi_msg_t *msg_memory, 
                                                                fbe_cmi_msg_type_t msg_type,
                                                                fbe_database_cmi_sync_context_t *context)
{
    fbe_status_t						wait_status;
    fbe_database_peer_synch_context_t	peer_synch;


    msg_memory->completion_callback = fbe_database_cmi_wait_for_send_completion;
    msg_memory->msg_type = msg_type;

    fbe_semaphore_init(&peer_synch.semaphore, 0, 1);
    
    /*send it*/
    fbe_database_cmi_send_message(msg_memory, &peer_synch);

    /*we are not done until the peer tells us it got it and it is happy*/
    wait_status = fbe_semaphore_wait_ms(&peer_synch.semaphore, 30000);
    fbe_semaphore_destroy(&peer_synch.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Timeout. failed to send command %d to peer!!!\n",
                       __FUNCTION__,
                       msg_type);	
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*is the peer even there ?*/
    if (peer_synch.status == FBE_STATUS_NO_DEVICE) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Peer not present to send command :%d!!!\n",
                       __FUNCTION__, msg_type); 
        /*nope, so let's go on*/
        return FBE_STATUS_NO_DEVICE;
    }

    if (peer_synch.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to send command %d to peer!!!\n",
                       __FUNCTION__,
                       msg_type);	
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now let's wait for the peer to confirm it was able to do it*/
    wait_status = fbe_semaphore_wait_ms(&context->semaphore, 30000);
    
    if (wait_status == FBE_STATUS_TIMEOUT){
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Peer not died Command:%d!!!\n",
                           __FUNCTION__,
                           msg_type); 
            return FBE_STATUS_GENERIC_FAILURE;
    }

    if (context->status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Peer update failed. msg_type:0x%x. Status:%d\n",
                       __FUNCTION__,
                       msg_type, context->status);
    }

    return context->status;
}

/*called by CMI after the request to update an object was sent succefully to the peer*/
static fbe_status_t fbe_database_cmi_wait_for_send_completion_no_op(fbe_database_cmi_msg_t *returned_msg,
                                                                    fbe_status_t completion_status,
                                                                    void *context)
{
    if (completion_status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Peer update failed.  Status:%d\n",
                       __FUNCTION__,
                       completion_status);
    }

    return completion_status;
}

/*called by CMI after the request to update an object was sent succefully to the peer*/
static fbe_status_t fbe_database_cmi_wait_for_send_completion(fbe_database_cmi_msg_t *returned_msg,
                                                              fbe_status_t completion_status,
                                                              void *context)
{
    fbe_database_peer_synch_context_t *    peer_synch_ptr = (fbe_database_peer_synch_context_t *)context;

    peer_synch_ptr->status = completion_status;
    fbe_semaphore_release(&peer_synch_ptr->semaphore, 0, 1 ,FALSE);
    return FBE_STATUS_OK;

}


/*tell the peer there is a change in one of the PVDs*/
fbe_status_t fbe_database_cmi_send_update_config_to_peer(fbe_database_service_t * fbe_database_service,
                                                         void  *config_data,
                                                         fbe_cmi_msg_type_t config_cmd)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    
    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (config_cmd) {
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD:
        fbe_copy_memory(&msg_memory->payload.update_pvd, config_data, sizeof(fbe_database_control_update_pvd_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_BLOCK_SIZE:
        fbe_copy_memory(&msg_memory->payload.update_pvd_block_size, config_data, sizeof(fbe_database_control_update_pvd_block_size_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_PVD:
        fbe_copy_memory(&msg_memory->payload.create_pvd, config_data, sizeof(fbe_database_control_pvd_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_VD:
        fbe_copy_memory(&msg_memory->payload.create_vd, config_data, sizeof(fbe_database_control_vd_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_VD:
        fbe_copy_memory(&msg_memory->payload.update_vd, config_data, sizeof(fbe_database_control_update_vd_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN:
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL:
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN:
        fbe_copy_memory(&msg_memory->payload.destroy_object, config_data, sizeof(fbe_database_control_destroy_object_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_RAID:
        fbe_copy_memory(&msg_memory->payload.create_raid, config_data, sizeof(fbe_database_control_raid_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_RAID:
        fbe_copy_memory(&msg_memory->payload.update_raid, config_data, sizeof(fbe_database_control_update_raid_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_LUN:
        fbe_copy_memory(&msg_memory->payload.create_lun, config_data, sizeof(fbe_database_control_lun_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_LUN:
        fbe_copy_memory(&msg_memory->payload.update_lun, config_data, sizeof(fbe_database_control_update_lun_t));
        break;
            case FBE_DATABE_CMI_MSG_TYPE_CLONE_OBJECT:
                fbe_copy_memory(&msg_memory->payload.clone_object, config_data, sizeof(fbe_database_control_clone_object_t));
                break;
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_EDGE:
        fbe_copy_memory(&msg_memory->payload.create_edge, config_data, sizeof(fbe_database_control_create_edge_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_EDGE:
        fbe_copy_memory(&msg_memory->payload.destroy_edge, config_data, sizeof(fbe_database_control_destroy_edge_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_POWER_SAVE_INFO:
        fbe_copy_memory(&msg_memory->payload.power_save_info, config_data, sizeof(fbe_database_power_save_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SPARE_CONFIG:
        fbe_copy_memory(&msg_memory->payload.update_spare_config, config_data, sizeof(fbe_database_control_update_system_spare_config_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO:
        fbe_copy_memory(&msg_memory->payload.time_threshold_info, config_data, sizeof(fbe_database_time_threshold_t));
        break;

    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG:
        fbe_copy_memory(&msg_memory->payload.system_bg_service, config_data, sizeof(fbe_database_control_update_peer_system_bg_service_t));
        break;        
    case FBE_DATABE_CMI_MSG_TYPE_CONNECT_DRIVE:
        fbe_copy_memory(&msg_memory->payload.drive_connection, config_data, sizeof(fbe_database_control_drive_connection_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_ENCRYPTION_MODE:
        fbe_copy_memory(&msg_memory->payload.encryption_info, config_data, sizeof(fbe_database_encryption_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_DATABASE_COMMIT_UPDATE_TABLE:
        /* no payload */
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_ENCRYPTION_MODE:
        fbe_copy_memory(&msg_memory->payload.encryption_mode, config_data, sizeof(fbe_database_control_update_encryption_mode_t));
        break;
    case FBE_DATABE_CMI_MSG_TYPE_SET_GLOBAL_PVD_CONFIG:
        fbe_copy_memory(&msg_memory->payload.pvd_config, config_data, sizeof(fbe_global_pvd_config_t));
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_PAUSE:
        fbe_copy_memory(&msg_memory->payload.encryption_info, config_data, sizeof(fbe_database_encryption_t));
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL:
        fbe_copy_memory(&msg_memory->payload.create_ext_pool, config_data, sizeof(fbe_database_control_create_ext_pool_t));
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_LUN:
        fbe_copy_memory(&msg_memory->payload.create_ext_pool_lun, config_data, sizeof(fbe_database_control_create_ext_pool_lun_t));
        break;
    default:
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unknown CMI command %d\n", __FUNCTION__, config_cmd);

        return FBE_STATUS_GENERIC_FAILURE;

    }

    msg_memory->completion_callback = fbe_database_cmi_send_update_config_to_peer_completion;
    msg_memory->msg_type = config_cmd;
    
    /*and send it. */
    return fbe_database_cmi_send_and_wait_for_peer_config_synch(msg_memory, &peer_config_update_confirm);
}

/*called by CMI after the request to update an object was sent succefully to the peer*/
static fbe_status_t fbe_database_cmi_send_update_config_to_peer_completion(fbe_database_cmi_msg_t *returned_msg,
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

/*send a config command and wait for it to be completed on the peer*/
static fbe_status_t fbe_database_cmi_send_and_wait_for_peer_config_synch(fbe_database_cmi_msg_t *msg_memory, fbe_database_peer_synch_context_t *peer_confirm_p)
{
    fbe_status_t						wait_status;
    fbe_database_peer_synch_context_t	peer_synch;
    fbe_cmi_msg_type_t	msg_type = msg_memory->msg_type;
    fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_ERROR;

    fbe_semaphore_init(&peer_synch.semaphore, 0, 1);
    fbe_semaphore_init(&peer_confirm_p->semaphore, 0, 1);

    /*send it*/
    fbe_database_cmi_send_message(msg_memory, &peer_synch);

    /*we are not done until the peer tells us it got it and it is happy*/
    wait_status = fbe_semaphore_wait_ms(&peer_synch.semaphore, 30000);
    fbe_semaphore_destroy(&peer_synch.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){

        fbe_semaphore_destroy(&peer_confirm_p->semaphore);
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
        fbe_semaphore_destroy(&peer_confirm_p->semaphore);
        /*nope, so let's go on*/
        return FBE_STATUS_OK;
    }

    if (peer_synch.status != FBE_STATUS_OK) {
        fbe_semaphore_destroy(&peer_confirm_p->semaphore);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to send command to peer!!!\n",__FUNCTION__);	
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now let's wait for the peer to confirm it was able to do it*/
    wait_status = fbe_semaphore_wait_ms(&peer_confirm_p->semaphore, 30000);
    fbe_semaphore_destroy(&peer_confirm_p->semaphore);

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

        if (peer_confirm_p->status != FBE_STATUS_OK) {
                database_trace(trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Peer update failed. msg_type:0x%x!\n",
                                __FUNCTION__,
                                msg_type);
        }

    return peer_confirm_p->status;
}


fbe_status_t fbe_database_config_change_confirmed_from_peer(fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_cmi_msg_transaction_confirm_t * confirm = &db_cmi_msg->payload.transaction_confirm;

    peer_config_update_confirm.status = confirm->status;
    fbe_semaphore_release(&peer_config_update_confirm.semaphore, 0, 1 ,FALSE);

    return FBE_STATUS_OK;

}

fbe_status_t fbe_database_config_change_request_from_peer(fbe_database_service_t *db_service,fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_status_t						status;
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_cmi_msg_type_t					confirm_msg;
    fbe_database_state_t 				database_state;
    fbe_u32_t                           msg_size;
    fbe_bool_t                          need_response = TRUE;

    confirm_msg = fbe_database_config_change_request_get_confirm_msg(db_cmi_msg->msg_type);
    if (confirm_msg == FBE_DATABE_CMI_MSG_TYPE_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

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

        /*we'll return the status as a handshake to the master*/
        msg_memory->msg_type = confirm_msg;
        msg_memory->completion_callback = fbe_database_config_change_request_from_peer_completion;
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
        switch (db_cmi_msg->msg_type) {
        case FBE_DATABE_CMI_MSG_TYPE_UPDATE_VD:
            status = database_update_vd(&db_cmi_msg->payload.update_vd);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_CREATE_PVD:
            status = database_create_pvd(&db_cmi_msg->payload.create_pvd);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_CREATE_VD:
            status = database_create_vd(&db_cmi_msg->payload.create_vd);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD:
            status = database_update_pvd(&db_cmi_msg->payload.update_pvd);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_BLOCK_SIZE:
            status = database_update_pvd_block_size(&db_cmi_msg->payload.update_pvd_block_size);
            break;
        case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_ENCRYPTION_MODE:
            status = database_update_encryption_mode(&db_cmi_msg->payload.encryption_mode);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD:
            status = database_destroy_pvd(&db_cmi_msg->payload.destroy_object, TRUE);
            need_response = FALSE;            
            break;
        case FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD:
            status = database_destroy_vd(&db_cmi_msg->payload.destroy_object, TRUE);
            need_response = FALSE;            
            break;
        case FBE_DATABE_CMI_MSG_TYPE_CREATE_RAID:
            status = database_create_raid(&db_cmi_msg->payload.create_raid);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID:
            status = database_destroy_raid(&db_cmi_msg->payload.destroy_object, TRUE);
            need_response = FALSE;            
            break;
        case FBE_DATABE_CMI_MSG_TYPE_UPDATE_RAID:
            status = database_update_raid(&db_cmi_msg->payload.update_raid);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_CREATE_LUN:
            status = database_create_lun(&db_cmi_msg->payload.create_lun);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN:
            status = database_destroy_lun(&db_cmi_msg->payload.destroy_object, TRUE);
            need_response = FALSE;
            break;
        case FBE_DATABE_CMI_MSG_TYPE_UPDATE_LUN:
            status = database_update_lun(&db_cmi_msg->payload.update_lun);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_CLONE_OBJECT:
            status = database_clone_object(&db_cmi_msg->payload.clone_object);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_CREATE_EDGE:
            status = database_create_edge(&db_cmi_msg->payload.create_edge);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_DESTROY_EDGE:
            status = database_destroy_edge(&db_cmi_msg->payload.destroy_edge);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_POWER_SAVE_INFO:
            status = database_set_power_save(&db_cmi_msg->payload.power_save_info);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_ENCRYPTION_MODE:
            status = database_set_system_encryption_mode(db_cmi_msg->payload.encryption_info.system_encryption_info);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SPARE_CONFIG:
            status = database_update_system_spare_config(&db_cmi_msg->payload.update_spare_config);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO:
            status = database_set_time_threshold(&db_cmi_msg->payload.time_threshold_info);
            break;

        case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG:
            status = fbe_base_config_control_system_bg_service(&db_cmi_msg->payload.system_bg_service.system_bg_service);
            break;
            
        case FBE_DATABE_CMI_MSG_TYPE_DATABASE_COMMIT_UPDATE_TABLE:
            status = fbe_database_update_local_table_after_peer_commit();
            break;
        case FBE_DATABASE_CMI_MSG_TYPE_SETUP_ENCRYPTION_KEYS:
            status = database_setup_encryption_key(&db_cmi_msg->payload.setup_encryption_key);
            break;
        case FBE_DATABASE_CMI_MSG_TYPE_REKEY_ENCRYPTION_KEYS:
            status = database_rekey_encryption_key(&db_cmi_msg->payload.setup_encryption_key);
            break;
        case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_DRIVE_KEYS:
            status = database_update_drive_keys(&db_cmi_msg->payload.update_drive_key);
            break;
        case FBE_DATABE_CMI_MSG_TYPE_SET_GLOBAL_PVD_CONFIG:
            status = fbe_database_set_user_capacity_limit(db_cmi_msg->payload.pvd_config.user_capacity_limit);
            break;
        case FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_PAUSE:
            status = fbe_database_set_encryption_paused(db_cmi_msg->payload.encryption_info.system_encryption_info.encryption_paused);
            break;
        case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL:
            status = database_create_extent_pool(&db_cmi_msg->payload.create_ext_pool);
            break;
        case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_LUN:
            status = database_create_ext_pool_lun(&db_cmi_msg->payload.create_ext_pool_lun);
            break;
        case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL:
            status = database_destroy_extent_pool(&db_cmi_msg->payload.destroy_object, TRUE);
            need_response = FALSE;            
            break;
        case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN:
            status = database_destroy_ext_pool_lun(&db_cmi_msg->payload.destroy_object, TRUE);
            need_response = FALSE;            
            break;
        default:
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: unknown opcode 0x%x.\n", __FUNCTION__, db_cmi_msg->msg_type );
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Send confirm message to peer if it hasn't bee sent already.
     */
    if(need_response)
    {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*we'll return the status as a handshake to the master*/
        msg_memory->msg_type = confirm_msg;
        msg_memory->completion_callback = fbe_database_config_change_request_from_peer_completion;
        msg_memory->payload.transaction_confirm.status = status;

        fbe_database_cmi_send_message(msg_memory, NULL);
    }

    return FBE_STATUS_OK;

}


fbe_status_t fbe_database_config_change_request_from_peer_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                     fbe_status_t completion_status,
                                                                     void *context)
{
    /*let's return the message memory and for now that's all we need to do*/
    fbe_database_cmi_return_msg_memory(returned_msg);
    return FBE_STATUS_OK;
}

static fbe_cmi_msg_type_t fbe_database_config_change_request_get_confirm_msg(fbe_cmi_msg_type_t msg_type)
{
    switch (msg_type) {
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_VD:
        return FBE_DATABE_CMI_MSG_TYPE_UPDATE_VD_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_PVD:
        return FBE_DATABE_CMI_MSG_TYPE_CREATE_PVD_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_VD:
        return FBE_DATABE_CMI_MSG_TYPE_CREATE_VD_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD:
        return FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_BLOCK_SIZE:
        return FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_BLOCK_SIZE_CONFIRM;
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_ENCRYPTION_MODE:
        return FBE_DATABASE_CMI_MSG_TYPE_UPDATE_ENCRYPTION_MODE_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD:
        return FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD:
        return FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_RAID:
        return FBE_DATABE_CMI_MSG_TYPE_CREATE_RAID_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID:
        return FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_RAID:
        return FBE_DATABE_CMI_MSG_TYPE_UPDATE_RAID_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_LUN:
        return FBE_DATABE_CMI_MSG_TYPE_CREATE_LUN_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN:
        return FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_LUN:
        return FBE_DATABE_CMI_MSG_TYPE_UPDATE_LUN_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_CLONE_OBJECT:
        return FBE_DATABE_CMI_MSG_TYPE_CLONE_OBJECT_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_EDGE:
        return FBE_DATABE_CMI_MSG_TYPE_CREATE_EDGE_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_EDGE:
        return FBE_DATABE_CMI_MSG_TYPE_DESTROY_EDGE_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_POWER_SAVE_INFO:
        return FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_POWER_SAVE_INFO_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SPARE_CONFIG:
        return FBE_DATABE_CMI_MSG_TYPE_UPDATE_SPARE_CONFIG_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO:
        return FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG:
        return FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_CONNECT_DRIVE:
        return FBE_DATABE_CMI_MSG_TYPE_CONNECT_DRIVE_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_ENCRYPTION_MODE:
        return FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_ENCRYPTION_MODE_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_DATABASE_COMMIT_UPDATE_TABLE:
        return FBE_DATABE_CMI_MSG_TYPE_DATABASE_COMMIT_UPDATE_TABLE_CONFIRM;
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_ENCRYPTION_KEYS:
        return FBE_DATABASE_CMI_MSG_TYPE_SETUP_ENCRYPTION_KEYS_CONFIRM;
    case FBE_DATABASE_CMI_MSG_TYPE_REKEY_ENCRYPTION_KEYS:
        return FBE_DATABASE_CMI_MSG_TYPE_REKEY_ENCRYPTION_KEYS_CONFIRM;
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_DRIVE_KEYS:
        return FBE_DATABASE_CMI_MSG_TYPE_UPDATE_DRIVE_KEYS_CONFIRM;
    case FBE_DATABE_CMI_MSG_TYPE_SET_GLOBAL_PVD_CONFIG:
        return FBE_DATABE_CMI_MSG_TYPE_SET_GLOBAL_PVD_CONFIG_CONFIRM;
    case FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_PAUSE:
        return FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_PAUSE_CONFIRM;
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL:
        return FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_CONFIRM;
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_LUN:
        return FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_LUN_CONFIRM;
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL:
        return FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_CONFIRM;
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN:
        return FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN_CONFIRM;
    default:
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: unknown opcode 0x%x!!\n", __FUNCTION__, msg_type );
        return FBE_DATABE_CMI_MSG_TYPE_INVALID;
    }
}

/* When current database becomes service mode, send message to peer SP.
   wait for the message sent out to peer */
fbe_status_t fbe_database_cmi_send_db_service_mode_to_peer(void)
{
    fbe_database_cmi_msg_t *	msg_memory = NULL;

    /* First send handshake to peer by cmi service */
    fbe_cmi_send_handshake_to_peer();

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) { 
        database_trace (FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: allocate memory failed. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    msg_memory->completion_callback = fbe_database_cmi_send_db_service_mode_to_peer_completion;
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_DB_SERVICE_MODE;

    fbe_database_cmi_send_message(msg_memory, NULL);

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_database_cmi_send_db_service_mode_to_peer_completion(fbe_database_cmi_msg_t *returned_msg, fbe_status_t completion_status, void *context)
{
    /* Let return the message memory first */
    fbe_database_cmi_return_msg_memory(returned_msg);

    if (completion_status == FBE_STATUS_BUSY) {
        /* peer is not yet ready, let's try it again */
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: peer is not ready, let's try again\n", __FUNCTION__);
        fbe_database_cmi_send_db_service_mode_to_peer();
    } else if (completion_status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to send mainenance mode to peer.\n", __FUNCTION__);
    }

    return FBE_STATUS_OK;
}

/* After receiving the db cmi message(peer is service state) from peer,
   we change us state */
void fbe_database_db_service_mode_state_from_peer(fbe_database_service_t *db_service)
{
    fbe_cmi_sp_state_t sp_state;
    fbe_database_state_t	db_state;
    
    get_database_service_state(db_service, &db_state);

    fbe_cmi_get_current_sp_state(&sp_state);
    get_database_service_state(db_service,  &db_state);
    /*set peer to be in service mode*/
    fbe_cmi_set_peer_service_mode(FBE_TRUE);

    switch(sp_state) {
    case FBE_CMI_STATE_SERVICE_MODE:
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: current sp state is in degraded mode, do nothing and return\n",
                       __FUNCTION__);
        break;
    case FBE_CMI_STATE_PASSIVE:
        /* If current sp is still passive,
         * that means the handshake message arrives later than the database cmi message
         * set current sp active by database cmi message
         */
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: current sp state shouldn't be passive, change sp to active\n",
                       __FUNCTION__);
        fbe_cmi_set_current_sp_state(FBE_CMI_STATE_ACTIVE);
        fbe_database_process_lost_peer();
        break;
    case FBE_CMI_STATE_ACTIVE:
        /* when we received peer database is in service mode,
         * Process it just as the peer was lost.
         */
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: current sp state is active\n",
                       __FUNCTION__);
        if (db_state != FBE_DATABASE_STATE_READY) {
            fbe_database_process_lost_peer();
        }
        break;
    /* current SP must not be busy state */
    case FBE_CMI_STATE_BUSY:
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: current sp state shouldn't be busy\n",
                       __FUNCTION__);
        break;
    default:
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid current sp state: 0x%x\n",
                       __FUNCTION__, sp_state);
        break;
    }
}

/*!***************************************************************
 * @fn fbe_database_cmi_update_peer_fru_descriptor
 *****************************************************************
 * @brief
 *   update in-memory fru descriptor on peer.
 *
 * @param fbe_database_service_t *db_service

 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    9/25/2012:   created Jian Gao
 *
 ****************************************************************/
fbe_status_t fbe_database_cmi_update_peer_fru_descriptor
    (fbe_homewrecker_fru_descriptor_t*fru_descriptor_sent_to_peer)
{
    fbe_database_peer_synch_context_t   peer_synch;
    fbe_database_cmi_msg_t *    msg_memory = NULL;
    fbe_status_t                        wait_status;

    if (NULL == fru_descriptor_sent_to_peer) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: invalid parameter, fru_descriptor_sent_to_peer NULL.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*initialize semaphore and set default value*/
    fbe_semaphore_init(&peer_synch.semaphore, 0, 1);
    fbe_semaphore_init(&peer_update_fru_descriptor_confirm.semaphore, 0, 1);

    peer_update_fru_descriptor_confirm.status = FBE_STATUS_GENERIC_FAILURE;
    peer_synch.status = FBE_STATUS_GENERIC_FAILURE;

    /*get a cmi msg memory*/
    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        fbe_semaphore_destroy(&peer_synch.semaphore);
        fbe_semaphore_destroy(&peer_update_fru_descriptor_confirm.semaphore);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize this msg memory with correct type, payload and callback*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UPDATE_FRU_DESCRIPTOR;
    msg_memory->completion_callback = fbe_database_cmi_update_peer_fru_descriptor_completion;
    fbe_copy_memory(&(msg_memory->payload.fru_descriptor.fru_descriptor),
                                           fru_descriptor_sent_to_peer, 
                                           sizeof(fbe_homewrecker_fru_descriptor_t));

    database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: sending fru descriptor to peer.\n",
                       __FUNCTION__);

    /*send this cmi message and wait for the completion of the sending*/
    fbe_database_cmi_send_message(msg_memory, &peer_synch);

    wait_status = fbe_semaphore_wait_ms(&peer_synch.semaphore, 30000);
    fbe_semaphore_destroy(&peer_synch.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){
        fbe_semaphore_destroy(&peer_update_fru_descriptor_confirm.semaphore);

        /*peer may have been there when we started but just went away in mid, in this case we just continue as ususal*/
        if (!database_common_peer_is_alive()) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: peer died, we continue.\n",__FUNCTION__); 
            return FBE_STATUS_OK;
        }else{
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: update fru descriptor request on peer timed out!!!\n",__FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /*is the peer even there ?*/
    if (peer_synch.status == FBE_STATUS_NO_DEVICE) {
         fbe_semaphore_destroy(&peer_update_fru_descriptor_confirm.semaphore);
        /*peer is not there, so let's go on*/
        return FBE_STATUS_OK;
    }

    if (peer_synch.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to send command to peer!!!\n",__FUNCTION__);    

        fbe_semaphore_destroy(&peer_update_fru_descriptor_confirm.semaphore);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*we have to wait for peer's message to confirm it has updated the fru descriptor so that we can go on*/
    wait_status = fbe_semaphore_wait_ms(&peer_update_fru_descriptor_confirm.semaphore, 0);
    fbe_semaphore_destroy(&peer_update_fru_descriptor_confirm.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){
        /*peer may have been there when we started but just went away in mid, in this case we just continue as ususal*/
        if (!database_common_peer_is_alive()) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: peer died, we continue.\n",__FUNCTION__); 
            return FBE_STATUS_OK;
        }else{
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: update fru descriptor request on peer timed out!!!\n",__FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (peer_update_fru_descriptor_confirm.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: peer update fru descriptor failed!!!\n",__FUNCTION__);    
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_cmi_update_peer_fru_descriptor_completion
 *****************************************************************
 * @brief
 *   completion function of update the in-memory fru descriptor on peer.
 *
 * @param returned_msg - the cmi msg we sent out before
 * @param completion_status - the status of sending the cmi msg
 * @param context - NULL
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    9/25/2012:   created Jian Gao
 *
 ****************************************************************/
static fbe_status_t fbe_database_cmi_update_peer_fru_descriptor_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context)
{
    fbe_database_peer_synch_context_t * peer_synch_ptr = (fbe_database_peer_synch_context_t *)context;

    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: complete msg type:%d\n", 
                   __FUNCTION__, 
                   returned_msg->msg_type);

    /*let's return the message memory first*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    peer_synch_ptr->status = completion_status;
    fbe_semaphore_release(&peer_synch_ptr->semaphore, 0, 1 ,FALSE);

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_cmi_process_update_fru_descriptor
 *****************************************************************
 * @brief
 *   copy peer's fru descriptor into our own fru descriptor
 *   called on the passive side when the active side is pushing the fru descriptor to it.
 *
 * @param db_service - fbe_database_service global variable in our side
 * @param cmi_msg - fbe_cmi_msg_update_peer_fru_descriptor_t structure peer sends to us
 *
 * @return
 *
 * @version
 *    9/25/2012:   created Jian Gao
 *
 ****************************************************************/
void fbe_database_cmi_process_update_fru_descriptor(fbe_database_service_t* db_service, fbe_database_cmi_msg_t* cmi_msg)
{
    fbe_cmi_msg_update_peer_fru_descriptor_t*    cmi_msg_fru_descriptor = &cmi_msg->payload.fru_descriptor;
    fbe_database_cmi_msg_t *    msg_memory;
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u8_t index = 0;

    database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: fru descriptor received from active.\n",
                       __FUNCTION__);

    fbe_spinlock_lock(&db_service->pvd_operation.system_fru_descriptor_lock);
    /*first clean the sys db header memory*/
    fbe_zero_memory(&(db_service->pvd_operation.system_fru_descriptor), sizeof(fbe_homewrecker_fru_descriptor_t));
    
    /*then copy sys db header from peer according to the calculated size*/
    fbe_copy_memory(&(db_service->pvd_operation.system_fru_descriptor), &cmi_msg_fru_descriptor->fru_descriptor, sizeof(fbe_homewrecker_fru_descriptor_t));
    fbe_spinlock_unlock(&db_service->pvd_operation.system_fru_descriptor_lock);

    database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: fru descriptor updated. \n",
                       __FUNCTION__);
    database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "fru_dp.magic_string: %s. \n",
                       db_service->pvd_operation.system_fru_descriptor.magic_string);
    database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "fru_dp.wwn_seed: %d. \n",
                       db_service->pvd_operation.system_fru_descriptor.wwn_seed);
    for(; index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; index++) {
        database_trace(FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "fru_dp.SN[%d]: %s. \n",
                           index,
                           db_service->pvd_operation.system_fru_descriptor.system_drive_serial_number[index]);
    }
    database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "fru_dp.chassis_replacement_flag: %d. \n",
                       db_service->pvd_operation.system_fru_descriptor.chassis_replacement_movement);
        
    /* signal more work */
    fbe_semaphore_release(&db_service->pvd_operation.pvd_operation_thread_semaphore, 0, 1, FALSE);

    /*send confirm to peer*/
    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        return;
    }

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UPDATE_FRU_DESCRIPTOR_CONFIRM;
    msg_memory->completion_callback = fbe_database_update_fru_descriptor_confirm_completion;
    msg_memory->payload.fru_descriptor_confirm.status = status;

    fbe_database_cmi_send_message(msg_memory, NULL);
    
}

fbe_status_t fbe_database_cmi_update_fru_descriptor_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_cmi_msg_update_fru_descriptor_confirm_t* confirm = &db_cmi_msg->payload.fru_descriptor_confirm;
    peer_update_fru_descriptor_confirm.status = confirm->status;
    fbe_semaphore_release(&peer_update_fru_descriptor_confirm.semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_database_update_fru_descriptor_confirm_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context)
{
    /*let's return the message memory and for now that's all we need to do*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_cmi_update_peer_system_db_header
 *****************************************************************
 * @brief
 *   update the system db header on peer.
 *
 * @param system_db_header - the system db header sent to peer to update
 * @param do_validation - whether request the peer to do validation about the sent system db header

 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    4/18/2012:   created Zhipeng Hu
 *
 ****************************************************************/
fbe_status_t fbe_database_cmi_update_peer_system_db_header(database_system_db_header_t* system_db_header, fbe_u64_t *peer_sep_version)
{
    fbe_database_peer_synch_context_t   peer_synch;
    fbe_database_cmi_msg_t *    msg_memory = NULL;
    fbe_status_t                        wait_status;

    if(NULL == system_db_header || NULL == peer_sep_version)
        return FBE_STATUS_GENERIC_FAILURE;

    *peer_sep_version = INVALID_SEP_PACKAGE_VERSION;

    /*initialize semaphore and set default value*/
    fbe_semaphore_init(&peer_synch.semaphore, 0, 1);
    fbe_semaphore_init(&peer_update_system_db_header_confirm.semaphore, 0, 1);

    peer_update_system_db_header_confirm.status = FBE_STATUS_GENERIC_FAILURE;
    peer_synch.status = FBE_STATUS_GENERIC_FAILURE;

    /*get a cmi msg memory*/
    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize this msg memory with correct type, payload and callback*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UPDATE_SYSTEM_DB_HEADER;
    msg_memory->completion_callback = fbe_database_cmi_update_peer_system_db_header_completion;
    msg_memory->payload.system_db_header.system_db_header_size = system_db_header->version_header.size;// we may be loading smaller size from disk
    msg_memory->payload.system_db_header.current_sep_version = SEP_PACKAGE_VERSION;//may not be the same as persisted_sep_version
    fbe_copy_memory(&(msg_memory->payload.system_db_header.system_db_header),
                                system_db_header, system_db_header->version_header.size);

    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: system_db_header_size: %u \n",
                   __FUNCTION__,
                   (unsigned int)system_db_header->version_header.size);

    /*send this cmi message and wait for the completion of the sending*/
    fbe_database_cmi_send_message(msg_memory, &peer_synch);

    wait_status = fbe_semaphore_wait_ms(&peer_synch.semaphore, 30000);
    fbe_semaphore_destroy(&peer_synch.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){
        fbe_semaphore_destroy(&peer_update_system_db_header_confirm.semaphore);

        /*peer may have been there when we started but just went away in mid, in this case we just continue as ususal*/
        if (!database_common_peer_is_alive()) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: peer died, we continue.\n",__FUNCTION__); 
            return FBE_STATUS_OK;
        }else{
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: update system db header request on peer timed out!!!\n",__FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /*is the peer even there ?*/
    if (peer_synch.status == FBE_STATUS_NO_DEVICE) {
         fbe_semaphore_destroy(&peer_update_system_db_header_confirm.semaphore);
        /*peer is not there, so let's go on*/
        return FBE_STATUS_OK;
    }

    if (peer_synch.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to send command to peer!!!\n",__FUNCTION__);    

        fbe_semaphore_destroy(&peer_update_system_db_header_confirm.semaphore);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*we have to wait for peer's message to confirm it has updated the system db header so that we can go on*/
    wait_status = fbe_semaphore_wait_ms(&peer_update_system_db_header_confirm.semaphore, 0);
    fbe_semaphore_destroy(&peer_update_system_db_header_confirm.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){
        /*peer may have been there when we started but just went away in mid, in this case we just continue as ususal*/
        if (!database_common_peer_is_alive()) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: peer died, we continue.\n",__FUNCTION__); 
            return FBE_STATUS_OK;
        }else{
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: update system db header request on peer timed out!!!\n",__FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    

    if (peer_update_system_db_header_confirm.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: peer update system db header failed!!\n",__FUNCTION__);    
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *peer_sep_version = peer_update_system_db_header_confirm.peer_sep_version;

    fbe_database_metadata_set_ndu_in_progress(*peer_sep_version != SEP_PACKAGE_VERSION);

    fbe_database_cmi_set_peer_version(*peer_sep_version);

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_cmi_update_peer_system_db_header_completion
 *****************************************************************
 * @brief
 *   completion function of update the system db header on peer.
 *
 * @param returned_msg - the cmi msg we sent out before
 * @param completion_status - the status of sending the cmi msg
 * @param context - NULL
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    4/17/2012:   created Zhipeng Hu
 *
 ****************************************************************/
static fbe_status_t fbe_database_cmi_update_peer_system_db_header_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context)
{
    fbe_database_peer_synch_context_t * peer_synch_ptr = (fbe_database_peer_synch_context_t *)context;

    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: complete msg type:%d\n", 
                   __FUNCTION__, 
                   returned_msg->msg_type);

    /*let's return the message memory first*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    peer_synch_ptr->status = completion_status;
    fbe_semaphore_release(&peer_synch_ptr->semaphore, 0, 1 ,FALSE);

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_cmi_process_update_system_db_header
 *****************************************************************
 * @brief
 *   copy peer's system db header into our own system db header
 *   called on the passive side when the active side is pushing the system db header to it.
 *
 * @param db_service - fbe_database_service global variable in our side
 * @param cmi_msg - fbe_cmi_msg_update_peer_system_db_header_t structure peer sends to us
 *
 * @return
 *
 * @version
 *    4/17/2012:   created Zhipeng Hu
 *
 ****************************************************************/
void fbe_database_cmi_process_update_system_db_header(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_update_peer_system_db_header_t*    cmi_msg_sys_db_header = &cmi_msg->payload.system_db_header;
    fbe_database_cmi_msg_t *    msg_memory;
    fbe_status_t    status = FBE_STATUS_OK;

    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: received size :%d, local size:%u \n",
                   __FUNCTION__,
                   cmi_msg_sys_db_header->system_db_header_size,
                   (unsigned int)sizeof(database_system_db_header_t));
                       
    if(cmi_msg_sys_db_header->system_db_header_size > sizeof(database_system_db_header_t))
    {
        /*if peer is newer version sys db header*/
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: received system db header is bigger.(%d) > (%u)\n",
                       __FUNCTION__,
                       cmi_msg_sys_db_header->system_db_header_size,
                       (unsigned int)sizeof(database_system_db_header_t));

        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);

            return ;
        }

        /*we'll return the status as a handshake to the master*/
        msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UPDATE_SYSTEM_DB_HEADER_CONFIRM;
        msg_memory->completion_callback = fbe_database_update_system_db_header_confirm_completion;

        msg_memory->payload.system_db_header_confirm.status = FBE_STATUS_GENERIC_FAILURE;
        msg_memory->payload.system_db_header_confirm.current_sep_version = SEP_PACKAGE_VERSION;

        fbe_database_cmi_send_message(msg_memory, NULL);
		fbe_event_log_write(SEP_ERROR_CODE_NEWER_CONFIG_FROM_PEER,
										NULL, 0, "%x %x",
										cmi_msg_sys_db_header->system_db_header_size,
										(int)sizeof(database_system_db_header_t));
		fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_PROBLEMATIC_DATABASE_VERSION);
		return;
    }

    fbe_spinlock_lock(&db_service->system_db_header_lock);
    /*first clean the sys db header memory*/
    fbe_zero_memory(&(db_service->system_db_header), sizeof(database_system_db_header_t));
    
    /*then copy sys db header from peer according to the calculated size*/
    fbe_copy_memory(&(db_service->system_db_header), &cmi_msg_sys_db_header->system_db_header, cmi_msg_sys_db_header->system_db_header_size);
    fbe_spinlock_unlock(&db_service->system_db_header_lock);

    db_service->peer_sep_version = cmi_msg_sys_db_header->current_sep_version;

    fbe_database_metadata_set_ndu_in_progress(db_service->peer_sep_version != SEP_PACKAGE_VERSION);

    fbe_database_cmi_set_peer_version(db_service->peer_sep_version);

    /*send confirm to peer*/
    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        return;
    }

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UPDATE_SYSTEM_DB_HEADER_CONFIRM;
    msg_memory->completion_callback = fbe_database_update_system_db_header_confirm_completion;
    msg_memory->payload.system_db_header_confirm.status = status;
    msg_memory->payload.system_db_header_confirm.current_sep_version = SEP_PACKAGE_VERSION;

    fbe_database_cmi_send_message(msg_memory, NULL);
    
}

fbe_status_t fbe_database_cmi_update_system_db_header_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_cmi_msg_update_system_db_header_confirm_t * confirm = &db_cmi_msg->payload.system_db_header_confirm;

    peer_update_system_db_header_confirm.status = confirm->status;
    peer_update_system_db_header_confirm.peer_sep_version = confirm->current_sep_version;
    fbe_semaphore_release(&peer_update_system_db_header_confirm.semaphore, 0, 1 ,FALSE);
    
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_database_update_system_db_header_confirm_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context)
{
    /*let's return the message memory and for now that's all we need to do*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    return FBE_STATUS_OK;
}

/*****************************************************************
 * @brief 
 *  If receiving a message which type can't be recognized by current SP,
 *  send back an CMI message to notify the sender with message type:
 *  FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_MSG_TYPE
 * 
 * @author gaoh1 (5/18/2012)
 * 
 * @param db_service 
 * @param cmi_msg 
 *******************************************************************/
void fbe_database_handle_unknown_cmi_msg_type_from_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return ;
    }

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_MSG_TYPE;
    msg_memory->completion_callback = fbe_database_cmi_send_unknown_msg_response_to_peer_completion;
    msg_memory->payload.unknown_msg_res.err_type = FBE_CMI_MSG_ERR_TYPE_UNKNOWN_MSG_TYPE;
    msg_memory->payload.unknown_msg_res.received_msg_type = cmi_msg->msg_type;
    msg_memory->payload.unknown_msg_res.received_msg_size = fbe_database_cmi_get_msg_size(cmi_msg->msg_type);
    msg_memory->payload.unknown_msg_res.sep_version = SEP_PACKAGE_VERSION;
    msg_memory->payload.unknown_msg_res.status = FBE_STATUS_GENERIC_FAILURE;

    fbe_database_cmi_send_message(msg_memory, NULL);

    return ;

}

/*****************************************************************
 * @brief 
 *  If receiving a message which size is bigger than the one in current SP,
 *  send back an CMI message to notify the sender with message type:
 *  FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_MSG_SIZE;
 * 
 * @author gaoh1 (5/18/2012)
 * 
 * @param db_service 
 * @param cmi_msg 
 *******************************************************************/
void fbe_database_handle_unknown_cmi_msg_size_from_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return ;
    }

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_MSG_SIZE;
    msg_memory->completion_callback = fbe_database_cmi_send_unknown_msg_response_to_peer_completion;
    msg_memory->payload.unknown_msg_res.err_type = FBE_CMI_MSG_ERR_TYPE_UNKNOWN_MSG_TYPE;
    msg_memory->payload.unknown_msg_res.received_msg_type = cmi_msg->msg_type;
    msg_memory->payload.unknown_msg_res.received_msg_size = fbe_database_cmi_get_msg_size(cmi_msg->msg_type);
    msg_memory->payload.unknown_msg_res.sep_version = SEP_PACKAGE_VERSION;
    msg_memory->payload.unknown_msg_res.status = FBE_STATUS_GENERIC_FAILURE;

    fbe_database_cmi_send_message(msg_memory, NULL);

    return ;

}

static fbe_status_t fbe_database_cmi_send_unknown_msg_response_to_peer_completion(fbe_database_cmi_msg_t *returned_msg,fbe_status_t completion_status, void *context)
{
    /*let's return the message memory and for now that's all we need to do*/
    fbe_database_cmi_return_msg_memory(returned_msg);
    return FBE_STATUS_OK;
}

/****************************************************************
 * @brief 
 *  If peer SP can't recognized a cmi message, it will send back
 *  a cmi message to notify the sender. This is the handling
 *  function of the sender when receiving the notification
 *  message.
 * 
 * @author gaoh1 (5/18/2012)
 * 
 * @param db_service 
 * @param cmi_msg 
 * 
 * @return fbe_status_t 
 *****************************************************************/
fbe_status_t fbe_database_cmi_unknown_cmi_msg_by_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    /* If the cmi message can't be recognized by peer, the sender will be notified.
     * If the CMI message is a synchnized message, it can wake up the waiter here 
     */
    fbe_cmi_msg_unknown_msg_response_t *unknown_type_res = &cmi_msg->payload.unknown_msg_res;

    database_trace(FBE_TRACE_LEVEL_WARNING,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: send an unknown cmi message to peer\n",
                   __FUNCTION__);
    database_trace(FBE_TRACE_LEVEL_WARNING,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: the unknown msg type is %d, peer sep version is %d \n",
                   __FUNCTION__,
                   unknown_type_res->received_msg_type,
                   (int)unknown_type_res->sep_version);

    /* unknown_type_res->received_msg_type is the msg type which can't be recognized by Peer SP */
    switch (unknown_type_res->received_msg_type) {
    case FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_FOR_TEST:
        /* When We send FBE_DATABE_CMI_MSG_TYPE_UNKNOWN msg to peer for test,
         * we used the synchnized mode.
         * Here we set the status and wake up the waiter.
         */
        peer_config_update_confirm.status = FBE_STATUS_OK;
        fbe_semaphore_release(&peer_config_update_confirm.semaphore, 0, 1 ,FALSE);
        break;
    }
    return FBE_STATUS_OK;
}

/****************************************************************
 * @brief 
 *  If peer SP can't recognized the size of the cmi message,
 *  it will send back a cmi message to notify the sender.
 *  This is the handling function of the sender when receiving the notification
 *  message.
 * 
 * @author gaoh1 (5/18/2012)
 * 
 * @param db_service 
 * @param cmi_msg 
 * 
 * @return fbe_status_t 
 *****************************************************************/
fbe_status_t fbe_database_cmi_unknown_cmi_msg_size_by_peer(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    /* If the cmi message size can't be recognized by peer, the sender will be notified.
     * The sender can do the related handling here.
     */
    fbe_cmi_msg_unknown_msg_response_t *unknown_type_res = &cmi_msg->payload.unknown_msg_res;

    database_trace(FBE_TRACE_LEVEL_WARNING,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: send an cmi message to peer, the size is too large\n",
                   __FUNCTION__);
    database_trace(FBE_TRACE_LEVEL_WARNING,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: the sender's msg type is %d, peer sep version is %d \n",
                   __FUNCTION__,
                   unknown_type_res->received_msg_type,
                   (int)unknown_type_res->sep_version);

    /***********************************************************************
     * You can add the handling here after received this type of CMI message 
    ************************************************************************/

    return FBE_STATUS_OK;
}


/***************************************************************
 * @brief
 * Test the case that database in Peer SP handles an unknown cmi msg by 
 * sending an unknown type of CMI messsage to Peer SP. 
 * This is the callback function.
 * 
 * Only used for fbe_test
 * 
 * @version 
 *    created by gaohp 5/18/2012 
****************************************************************/
static fbe_status_t fbe_database_cmi_send_unknown_type_cmi_to_peer_completion(fbe_database_cmi_msg_t *returned_msg,
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

/*!***************************************************************
* @fn database_check_unknown_cmi_msg_handling_test
*****************************************************************
* @brief
* Test the case that peer SP handles an unknown CMI message by sending an new
* type of CMI message to peer and waiting for the confirm
* message
* NOTE:  Only for fbe_test usage
*
* @param void
*
* @return FBE_STATUS_OK if success
* 
****************************************************************/
fbe_status_t database_check_unknown_cmi_msg_handling_test(void)
{
    fbe_database_cmi_msg_t *msg_memory = NULL;

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) { 
        database_trace (FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: allocate memory failed. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    msg_memory->completion_callback = fbe_database_cmi_send_unknown_type_cmi_to_peer_completion;
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_FOR_TEST;
    database_trace (FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: unknown msg type is 0x%x\n",
                    __FUNCTION__, msg_memory->msg_type);

    return fbe_database_cmi_send_and_wait_for_peer_config_synch(msg_memory, &peer_config_update_confirm);
}




/*!***************************************************************
 * @fn fbe_database_cmi_sync_online_drive_request_to_peer
 *****************************************************************
 * @brief
 *   sync the online planned drive request to peer side
 *
 * @param[in] online_drive_request - the online drive request
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    07/05/2012:   created Zhipeng Hu
 *
 ****************************************************************/
fbe_status_t fbe_database_cmi_sync_online_drive_request_to_peer(fbe_database_control_online_planned_drive_t* online_drive_request)
{
    fbe_database_peer_synch_context_t   peer_synch;
    fbe_database_cmi_msg_t *    msg_memory = NULL;
    fbe_status_t                        wait_status;

    if(NULL == online_drive_request)
        return FBE_STATUS_GENERIC_FAILURE;

    /*initialize semaphore and set default value*/
    fbe_semaphore_init(&peer_synch.semaphore, 0, 1);
    fbe_semaphore_init(&peer_online_planned_drive_confirm.semaphore, 0, 1);

    peer_online_planned_drive_confirm.status = FBE_STATUS_GENERIC_FAILURE;
    peer_synch.status = FBE_STATUS_GENERIC_FAILURE;

    /*get a cmi msg memory*/
    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize this msg memory with correct type, payload and callback*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_MAKE_DRIVE_ONLINE;
    msg_memory->completion_callback = fbe_database_cmi_sync_online_drive_request_to_peer_completion;
    msg_memory->payload.make_drive_online = *online_drive_request;

    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: sync drive online drive to peer %d_%d_%d \n",
                   __FUNCTION__,
                   online_drive_request->port_number,
                   online_drive_request->enclosure_number,
                   online_drive_request->slot_number);

    /*send this cmi message and wait for the completion of the sending*/
    fbe_database_cmi_send_message(msg_memory, &peer_synch);

    wait_status = fbe_semaphore_wait_ms(&peer_synch.semaphore, 30000);
    fbe_semaphore_destroy(&peer_synch.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){
        fbe_semaphore_destroy(&peer_online_planned_drive_confirm.semaphore);

        /*peer may have been there when we started but just went away in mid, in this case we just continue as ususal*/
        if (!database_common_peer_is_alive()) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: peer died, we continue.\n",__FUNCTION__); 
            return FBE_STATUS_OK;
        }else{
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: send request to peer timed out!!!\n",__FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /*is the peer even there ?*/
    if (peer_synch.status == FBE_STATUS_NO_DEVICE) {
         fbe_semaphore_destroy(&peer_online_planned_drive_confirm.semaphore);
        /*peer is not there, so let's go on*/
        return FBE_STATUS_OK;
    }

    if (peer_synch.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to send command to peer!!!\n",__FUNCTION__);    

        fbe_semaphore_destroy(&peer_online_planned_drive_confirm.semaphore);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*we have to wait for peer's message to confirm it has processed the online planned drive work so that we can go on*/
    wait_status = fbe_semaphore_wait_ms(&peer_online_planned_drive_confirm.semaphore, 0);
    fbe_semaphore_destroy(&peer_online_planned_drive_confirm.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){
        /*peer may have been there when we started but just went away in mid, in this case we just continue as ususal*/
        if (!database_common_peer_is_alive()) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: peer died, we continue.\n",__FUNCTION__); 
            return FBE_STATUS_OK;
        }else{
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: peer does not give us confirm. Timeout!!!\n",__FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }    

    if (peer_online_planned_drive_confirm.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: peer make drive online failed!!\n",__FUNCTION__);    
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_cmi_sync_online_drive_request_to_peer_completion
 *****************************************************************
 * @brief
 *   completion function of sync online drive request to peer.
 *
 * @param returned_msg - the cmi msg we sent out before
 * @param completion_status - the status of sending the cmi msg
 * @param context - NULL
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    07/05/2012:   created Zhipeng Hu
 *
 ****************************************************************/
static fbe_status_t fbe_database_cmi_sync_online_drive_request_to_peer_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context)
{
    fbe_database_peer_synch_context_t * peer_synch_ptr = (fbe_database_peer_synch_context_t *)context;

    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: complete msg type:%d\n", 
                   __FUNCTION__, 
                   returned_msg->msg_type);

    /*let's return the message memory first*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    peer_synch_ptr->status = completion_status;
    fbe_semaphore_release(&peer_synch_ptr->semaphore, 0, 1 ,FALSE);

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_cmi_process_online_drive_request
 *****************************************************************
 * @brief
 *   Make the specified drive online as requested by peer
 *
 * @param db_service - fbe_database_service global variable in our side
 * @param cmi_msg - fbe_database_control_online_planned_drive_t structure peer sends to us
 *
 * @return
 *
 * @version
 *    07/05/2012:   created Zhipeng Hu
 *
 ****************************************************************/
void fbe_database_cmi_process_online_drive_request(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_control_online_planned_drive_t*    cmi_msg_online_drive_req = &cmi_msg->payload.make_drive_online;
    fbe_database_control_online_planned_drive_t      make_drive_online_cmd;
    fbe_database_cmi_msg_t *    msg_memory;
    fbe_status_t    status = FBE_STATUS_OK;

    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: process peer's make drive online request. %d_%d_%d \n",
                   __FUNCTION__,
                   cmi_msg_online_drive_req->port_number,
                   cmi_msg_online_drive_req->enclosure_number,
                   cmi_msg_online_drive_req->slot_number);

    make_drive_online_cmd = *cmi_msg_online_drive_req;
    make_drive_online_cmd.pdo_object_id = FBE_OBJECT_ID_INVALID;
    status = database_make_planned_drive_online(&make_drive_online_cmd);

    /*send confirm to peer*/
    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        return;
    }

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_MAKE_DRIVE_ONLINE_CONFIRM;
    msg_memory->completion_callback = fbe_database_online_drive_confirm_completion;
    msg_memory->payload.make_drive_online_confirm.status = status;

    fbe_database_cmi_send_message(msg_memory, NULL);
    
}

/*!***************************************************************
 * @fn fbe_database_cmi_online_drive_confirm_from_peer
 *****************************************************************
 * @brief
 *   This function is called when we receive the online drive comfirm from peer.
 *
 * @param db_cmi_msg - the cmi msg we receive
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    07/05/2012:   created Zhipeng Hu
 *
 ****************************************************************/
fbe_status_t fbe_database_cmi_online_drive_confirm_from_peer(fbe_database_cmi_msg_t *db_cmi_msg)
{
    fbe_database_online_planned_drive_confirm_t * confirm = &db_cmi_msg->payload.make_drive_online_confirm;

    peer_online_planned_drive_confirm.status = confirm->status;
    fbe_semaphore_release(&peer_online_planned_drive_confirm.semaphore, 0, 1 ,FALSE);
    
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_database_online_drive_confirm_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context)
{
    /*let's return the message memory and for now that's all we need to do*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    return FBE_STATUS_OK;
}



/*!***************************************************************
 * @fn fbe_database_cmi_update_peer_shared_emv_info
 *****************************************************************
 * @brief
 *   update the shared emv info on peer.
 *
 * @param shared_emv_info - the shared emv info to be updated

 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    7/16/2012:   created Zhipeng Hu
 *
 ****************************************************************/
fbe_status_t fbe_database_cmi_update_peer_shared_emv_info(fbe_database_emv_t* shared_emv_info)
{
    fbe_database_peer_synch_context_t   peer_synch;
    fbe_database_cmi_msg_t *    msg_memory = NULL;
    fbe_status_t                        wait_status;

    if(NULL == shared_emv_info)
        return FBE_STATUS_GENERIC_FAILURE;

    /*initialize semaphore and set default value*/
    fbe_semaphore_init(&peer_synch.semaphore, 0, 1);
    peer_synch.status = FBE_STATUS_GENERIC_FAILURE;

    /*get a cmi msg memory*/
    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        fbe_semaphore_destroy(&peer_synch.semaphore);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize this msg memory with correct type, payload and callback*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UPDATE_SHARED_EMV_INFO;
    msg_memory->completion_callback = fbe_database_cmi_update_shared_emv_info_completion;
    msg_memory->payload.shared_emv_info = *shared_emv_info;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: send update cmi msg to peer.\n", __FUNCTION__); 

    /*send this cmi message and wait for the completion of the sending*/
    fbe_database_cmi_send_message(msg_memory, &peer_synch);

    wait_status = fbe_semaphore_wait_ms(&peer_synch.semaphore, 30000);
    fbe_semaphore_destroy(&peer_synch.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){
        /*peer may have been there when we started but just went away in mid, in this case we just continue as ususal*/
        if (!database_common_peer_is_alive()) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: peer died, we continue.\n",__FUNCTION__); 
            return FBE_STATUS_OK;
        }else{
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: update shared emv info request on peer timed out!!!\n",__FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /*is the peer even there ?*/
    if (peer_synch.status == FBE_STATUS_NO_DEVICE) {
        /*peer is not there, so let's go on*/
        return FBE_STATUS_OK;
    }

    if (peer_synch.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to send command to peer!!!\n",__FUNCTION__);    
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_cmi_update_shared_emv_info_completion
 *****************************************************************
 * @brief
 *   completion function of update the system db header on peer.
 *
 * @param returned_msg - the cmi msg we sent out before
 * @param completion_status - the status of sending the cmi msg
 * @param context - NULL
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    7/16/2012:   created Zhipeng Hu
 *
 ****************************************************************/
static fbe_status_t fbe_database_cmi_update_shared_emv_info_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context)
{
    fbe_database_peer_synch_context_t * peer_synch_ptr = (fbe_database_peer_synch_context_t *)context;

    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: complete msg type:%d\n", 
                   __FUNCTION__, 
                   returned_msg->msg_type);

    /*let's return the message memory first*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    peer_synch_ptr->status = completion_status;
    fbe_semaphore_release(&peer_synch_ptr->semaphore, 0, 1 ,FALSE);

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_cmi_process_update_shared_emv_info
 *****************************************************************
 * @brief
 *   copy peer's shared emv info into our own buffer
 *   called on the passive side when the active side is pushing the shared emv info to it.
 *
 * @param db_service - fbe_database_service global variable in our side
 * @param cmi_msg - fbe_database_emv_t structure peer sends to us
 *
 * @return
 *
 * @version
 *    7/16/2012:   created Zhipeng Hu
 *
 ****************************************************************/
void fbe_database_cmi_process_update_shared_emv_info(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_emv_t*    cmi_msg_shared_emv_info = &cmi_msg->payload.shared_emv_info;
    fbe_status_t    status;
    
    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: receive peer's update shared emv info request. SEMV: 0x%x \n",
                   __FUNCTION__,
                   (unsigned int)cmi_msg_shared_emv_info->shared_expected_memory_info);
                       
    status = database_set_shared_expected_memory_info(db_service, *cmi_msg_shared_emv_info);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to set shared expected memory info \n",
                       __FUNCTION__);
    }
}

void fbe_database_cmi_process_connect_drives(fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_database_control_drive_connection_t*    cmi_msg_connect_drive_req = &cmi_msg->payload.drive_connection;
    fbe_database_cmi_msg_t *             msg_memory = NULL;
    fbe_cmi_msg_type_t                    confirm_msg;
    fbe_u32_t                           msg_size;
    fbe_status_t    status;

    if(NULL == cmi_msg)
    {
        return;
    }

    confirm_msg = fbe_database_config_change_request_get_confirm_msg(cmi_msg->msg_type);
    if (confirm_msg == FBE_DATABE_CMI_MSG_TYPE_INVALID) {
        return;
    }

    msg_size = fbe_database_cmi_get_msg_size(cmi_msg->msg_type);
    /* If the received cmi msg has a larger size, version information will be sent back in the confirm message */
    if(cmi_msg->version_header.size > msg_size) {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);

            return;
        }

        /*we'll return the status as a handshake to the master*/
        msg_memory->msg_type = confirm_msg;
        msg_memory->completion_callback = fbe_database_config_change_request_from_peer_completion;
        msg_memory->payload.transaction_confirm.status = FBE_STATUS_GENERIC_FAILURE;
        msg_memory->payload.transaction_confirm.err_type = FBE_CMI_MSG_ERR_TYPE_LARGER_MSG_SIZE;
        msg_memory->payload.transaction_confirm.received_msg_type = cmi_msg->msg_type;
        msg_memory->payload.transaction_confirm.sep_version = SEP_PACKAGE_VERSION;

        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: received msg 0x%x, size(%d) is larger than local cmi msg size(%d)\n",
                       __FUNCTION__, cmi_msg->msg_type,
                       cmi_msg->version_header.size, msg_size);    
        fbe_database_cmi_send_message(msg_memory, NULL);
        return;
    }

    /*now perform drive connection*/
    status = database_perform_drive_connection(cmi_msg_connect_drive_req);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: fail to process drive connection request\n",
                       __FUNCTION__);    
    }

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (NULL == msg_memory) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return;
    }

    /*we'll return the status as a handshake to the master*/
    msg_memory->msg_type = confirm_msg;
    msg_memory->completion_callback = fbe_database_config_change_request_from_peer_completion;
    msg_memory->payload.transaction_confirm.status = status;

    fbe_database_cmi_send_message(msg_memory, NULL);

}

/*!***************************************************************
 * @fn fbe_database_cmi_clear_user_modified_wwn_seed
 *****************************************************************
 * @brief
 *   update the shared emv info on peer.
 *
 * @param shared_emv_info - the shared emv info to be updated

 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/01/2014:   created bphilbin
 *
 ****************************************************************/
fbe_status_t fbe_database_cmi_clear_user_modified_wwn_seed(void)
{
    fbe_database_peer_synch_context_t   peer_synch;
    fbe_database_cmi_msg_t *    msg_memory = NULL;

    /*get a cmi msg memory*/
    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get CMI msg memory\n", __FUNCTION__);
        fbe_semaphore_destroy(&peer_synch.semaphore);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize this msg memory with correct type, payload and callback*/
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_CLEAR_USER_MODIFIED_WWN_SEED;
    msg_memory->completion_callback = fbe_database_cmi_clear_user_modified_wwn_seed_completion;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: send update cmi msg to peer.\n", __FUNCTION__); 

    /*send this cmi message and wait for the completion of the sending*/
    fbe_database_cmi_send_message(msg_memory, NULL);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_database_cmi_clear_user_modified_wwn_seed_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                   fbe_status_t completion_status,
                                                                    void *context)
{
    /*let's return the message memory that's all we need to do*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    return FBE_STATUS_OK;
}



static fbe_status_t fbe_database_cmi_update_peer_tables_dma_completion(fbe_database_cmi_msg_t *returned_msg,
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
                        "%s: failed to update peer's table, peer state unknown\n", __FUNCTION__ );

        fbe_database_cmi_stop_peer_update = FBE_TRUE;
    }

    peer_synch_ptr->status = completion_status;
    fbe_semaphore_release(&peer_synch_ptr->semaphore, 0, 1 ,FALSE);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_cmi_update_peer_tables_dma_sync(fbe_cmi_memory_t * update_table)
{
    fbe_status_t						wait_status;
    fbe_database_peer_synch_context_t	peer_synch;

    fbe_semaphore_init(&peer_synch.semaphore, 0, 1);
    fbe_semaphore_init(&peer_update_config_table_confirm.semaphore, 0, 1);

    /* Send to CMI */
    update_table->context = &peer_synch;
    fbe_cmi_send_memory(update_table);

    /*we are not done until the peer tells us it got it and it is happy*/
    wait_status = fbe_semaphore_wait_ms(&peer_synch.semaphore, 30000);
    fbe_semaphore_destroy(&peer_synch.semaphore);

    if (wait_status == FBE_STATUS_TIMEOUT){

        fbe_semaphore_destroy(&peer_update_config_table_confirm.semaphore);
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
        fbe_semaphore_destroy(&peer_update_config_table_confirm.semaphore);
        /*nope, so let's go on*/
        return FBE_STATUS_OK;
    }

    if (peer_synch.status != FBE_STATUS_OK) {
        fbe_semaphore_destroy(&peer_update_config_table_confirm.semaphore);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to send command to peer!!!\n",__FUNCTION__);	
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now let's wait for the peer to confirm it was able to do it*/
    wait_status = fbe_semaphore_wait_ms(&peer_update_config_table_confirm.semaphore, 30000);
    fbe_semaphore_destroy(&peer_update_config_table_confirm.semaphore);

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

    if (peer_update_config_table_confirm.status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Peer update failed.\n",
                       __FUNCTION__);
    }

    return peer_update_config_table_confirm.status;
}

fbe_status_t fbe_database_cmi_update_peer_tables_dma(void)
{
    fbe_status_t					status;
    database_table_t *				table_ptr = NULL;
    database_config_table_type_t	table_type = DATABASE_CONFIG_TABLE_INVALID + 1;/*point to the first table*/
    fbe_database_cmi_msg_t * 		msg_memory = NULL;
    //fbe_database_cmi_msg_t          cmi_msg;
    fbe_u64_t						total_updates_sent = 0;

    fbe_database_cmi_stop_peer_update = FBE_FALSE;
    
    //msg_memory = &cmi_msg;
    /*go over the tables and send the entries*/
    for (;table_type < DATABASE_CONFIG_TABLE_LAST; table_type++) {

        if (fbe_database_cmi_stop_peer_update) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        status = fbe_database_get_service_table_ptr(&table_ptr, table_type);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get table ptr for table type %d\n", __FUNCTION__, table_type);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (table_ptr->peer_table_start_address == 0) {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*send the table over */
        status = fbe_database_cmi_update_peer_tables_dma_per_table(table_type,
                                                                   fbe_cmi_io_get_physical_address((void *)(table_ptr->table_content.user_entry_ptr)),
                                                                   table_ptr->peer_table_start_address,
                                                                   table_ptr->alloc_size,
                                                                   &total_updates_sent);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to send table type %d\n", __FUNCTION__, table_type);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* now, let's push the key memory over */
    if (fbe_database_encryption_get_peer_table_addr() != 0)
    {
        /*send the table over */
        status = fbe_database_cmi_update_peer_tables_dma_per_table(DATABASE_CONFIG_TABLE_KEY_MEMORY,
                                                                   fbe_cmi_io_get_physical_address((void *)fbe_database_encryption_get_local_table_virtual_addr()),
                                                                   fbe_database_encryption_get_peer_table_addr(),
                                                                   fbe_database_encryption_get_local_table_size(),
                                                                   &total_updates_sent);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to send table type %d\n", __FUNCTION__, table_type);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /*when we are done pushing the entries, we are sending the END_UPDATE message,
      This will trigger the object creation and commit on the peer*/

    /*if we allocated and did not use, we'll use it for the final message*/
    if (msg_memory == NULL) {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);
    
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: ACTIVE side sent %llu updates to peer\n", __FUNCTION__, (unsigned long long)total_updates_sent);

    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_DONE;
    msg_memory->completion_callback = fbe_database_cmi_update_peer_tables_completion;
    msg_memory->payload.update_done_msg.entries_sent = total_updates_sent;
    fbe_database_cmi_send_message(msg_memory, NULL);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_database_cmi_update_peer_tables_dma_per_table(database_config_table_type_t table_type,
                                                  fbe_u64_t source_addr,
                                                  fbe_u64_t destination_addr,
                                                  fbe_u32_t alloc_size,
                                                  fbe_u64_t	*total_updates_sent)
{
    fbe_cmi_memory_t                update_table;
    fbe_status_t					status;
    fbe_database_cmi_msg_t * 		msg_memory = NULL;
    fbe_u32_t                       bytes_to_send;
    fbe_u32_t                       bytes_remaining;
    fbe_u64_t                       local_updates_sent = *total_updates_sent;

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
    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_UPDATE_CONFIG_TABLE;
    fbe_zero_memory(&msg_memory->version_header, sizeof(msg_memory->version_header));
    msg_memory->version_header.size = fbe_database_cmi_get_msg_size(msg_memory->msg_type);
    msg_memory->completion_callback = fbe_database_cmi_update_peer_tables_dma_completion;
    msg_memory->payload.update_table.table_type = table_type;

    /* Fill in structure for dma */
    update_table.client_id = FBE_CMI_CLIENT_ID_DATABASE;
    update_table.context = NULL;
    update_table.source_addr = source_addr;
    update_table.dest_addr = destination_addr;
    update_table.data_length = alloc_size;
    update_table.message = (fbe_cmi_message_t)msg_memory;
    update_table.message_length = fbe_database_cmi_get_msg_size(msg_memory->msg_type);

    bytes_remaining = alloc_size;
    while (bytes_remaining != 0)
    {
        bytes_to_send = FBE_MIN(bytes_remaining, MAX_SEP_IO_MEM_LENGTH);
        update_table.data_length = bytes_to_send;

        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: type:%d, src %p dest %p len 0x%x\n", __FUNCTION__, 
                       table_type, (void *)(fbe_ptrhld_t)update_table.source_addr, (void *)(fbe_ptrhld_t)update_table.dest_addr, update_table.data_length);
        status = fbe_database_cmi_update_peer_tables_dma_sync(&update_table);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed for table type %d\n", __FUNCTION__, table_type);
            fbe_database_cmi_return_msg_memory(msg_memory);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        local_updates_sent++;

        update_table.source_addr += bytes_to_send;
        update_table.dest_addr += bytes_to_send;
        bytes_remaining -= bytes_to_send;
    }
    fbe_database_cmi_return_msg_memory(msg_memory);
    msg_memory = NULL;
    *total_updates_sent = local_updates_sent;
    return FBE_STATUS_OK;
}

void fbe_database_cmi_process_update_config_table(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_update_config_table_t *	update_table = &cmi_msg->payload.update_table;
    database_table_t *					table_ptr = NULL;
    fbe_status_t						status = FBE_STATUS_OK;
    fbe_database_cmi_msg_t * 		    msg_memory = NULL;

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: table type %d\n", __FUNCTION__, update_table->table_type);

    if (update_table->table_type != DATABASE_CONFIG_TABLE_KEY_MEMORY)
    {
        status = fbe_database_get_service_table_ptr(&table_ptr, update_table->table_type);
        if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: failed to get table ptr for table type %d\n", __FUNCTION__, update_table->table_type);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /*increment the number of updates we got*/
    db_service->updates_received++;

    if (msg_memory == NULL) {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);
            return;
        }
    }

    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_UPDATE_CONFIG_TABLE_CONFIRM;
    msg_memory->completion_callback = fbe_database_cmi_update_peer_tables_completion;
    msg_memory->payload.transaction_confirm.status = status;

    fbe_database_cmi_send_message(msg_memory, NULL);
    
    return;
}

void fbe_database_cmi_process_update_config_table_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_transaction_confirm_t * confirm = &cmi_msg->payload.transaction_confirm;

    peer_update_config_table_confirm.status = confirm->status;
    fbe_semaphore_release(&peer_update_config_table_confirm.semaphore, 0, 1 ,FALSE);

    return;
}

fbe_status_t fbe_database_cmi_send_general_command_to_peer_sync(fbe_database_service_t * fbe_database_service,
                                                                void  *config_data,
                                                                fbe_cmi_msg_type_t config_cmd)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_database_peer_synch_context_t * sync_context_p = NULL;
    
    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (config_cmd) {
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_DRIVE_KEYS:
        fbe_copy_memory(&msg_memory->payload.update_drive_key, config_data, sizeof(fbe_database_control_update_drive_key_t));
        sync_context_p = &peer_update_drive_keys_confirm;
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_ENCRYPTION_KEYS:
    case FBE_DATABASE_CMI_MSG_TYPE_REKEY_ENCRYPTION_KEYS:
        fbe_copy_memory(&msg_memory->payload.setup_encryption_key, config_data, sizeof(fbe_database_control_setup_encryption_key_t));
        sync_context_p = &peer_setup_encryption_keys_confirm;
        break;
    default:
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unknown CMI command %d\n", __FUNCTION__, config_cmd);

        return FBE_STATUS_GENERIC_FAILURE;

    }

    msg_memory->completion_callback = fbe_database_cmi_send_update_config_to_peer_completion;
    msg_memory->msg_type = config_cmd;
    
    /*and send it. */
    return fbe_database_cmi_send_and_wait_for_peer_config_synch(msg_memory, sync_context_p);
}

void fbe_database_cmi_process_update_drive_keys_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_transaction_confirm_t * confirm = &cmi_msg->payload.transaction_confirm;

    peer_update_drive_keys_confirm.status = confirm->status;
    fbe_semaphore_release(&peer_update_drive_keys_confirm.semaphore, 0, 1 ,FALSE);

    return;
}

void fbe_database_cmi_process_setup_encryption_keys_confirm(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_cmi_msg_transaction_confirm_t * confirm = &cmi_msg->payload.transaction_confirm;

    peer_setup_encryption_keys_confirm.status = confirm->status;
    fbe_semaphore_release(&peer_setup_encryption_keys_confirm.semaphore, 0, 1 ,FALSE);
    return;
}

fbe_status_t fbe_database_cmi_send_kill_drive_request_to_peer(fbe_object_id_t pdo_object_id)
{
    fbe_database_cmi_msg_t * 			msg_memory = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_database_peer_synch_context_t	peer_synch;

    

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: failed to get CMI msg memory\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Tell peer to shoot drive 0x%x!!! %p\n",
                   __FUNCTION__, pdo_object_id, &peer_synch);	

    msg_memory->payload.kill_drive_info.pdo_id = pdo_object_id;
    msg_memory->payload.kill_drive_info.context = &peer_synch;

    msg_memory->completion_callback = fbe_database_cmi_process_invalid_vault_drive_completion;
    msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_KILL_VAULT_DRIVE;
    
    /*send it*/
    fbe_database_cmi_send_message(msg_memory, &peer_synch);

    return status;
}


static fbe_status_t fbe_database_cmi_process_invalid_vault_drive_completion(fbe_database_cmi_msg_t *returned_msg,
                                                                            fbe_status_t completion_status,
                                                                            void *context)
{
    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: complete msg type:%d pdo 0x%x\n", 
                   __FUNCTION__, 
                   returned_msg->msg_type, 
                   returned_msg->payload.kill_drive_info.pdo_id);

    /*let's return the message memory first*/
    fbe_database_cmi_return_msg_memory(returned_msg);
    
    if (completion_status != FBE_STATUS_OK) {
        /*we do nothing here*/
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Peer failed to process request to shoot drive\n", __FUNCTION__ );
    }

    return FBE_STATUS_OK;

}


void fbe_database_cmi_process_invalid_vault_drive(fbe_database_service_t *db_service, fbe_database_cmi_msg_t *cmi_msg)
{
    fbe_object_id_t     pdo_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t          drive_dead = FBE_FALSE;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Got request from peer to shoot drive 0x%x!!! %p\n",
                   __FUNCTION__, cmi_msg->payload.kill_drive_info.pdo_id, cmi_msg->payload.kill_drive_info.context);

    drive_dead = fbe_database_process_killed_drive_needed(&pdo_id);
    if (drive_dead && (pdo_id == cmi_msg->payload.kill_drive_info.pdo_id))
    {
        /* If this is true on the passive side, we've already processed a previous request to shoot the drive
         */
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Drive already shot 0x%x!!! %p\n",
                   __FUNCTION__, cmi_msg->payload.kill_drive_info.pdo_id, cmi_msg->payload.kill_drive_info.context);
    }

    /* go and kill the drive..... */
    fbe_database_shoot_drive(cmi_msg->payload.kill_drive_info.pdo_id);

    return;
}
